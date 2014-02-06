// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * The hashdb manager provides access to the hashdb.
 */

#ifndef MAP_MULTIMAP_MANAGER_HPP
#define MAP_MULTIMAP_MANAGER_HPP

#include "map_manager.hpp"
#include "map_iterator.hpp"
#include "multimap_manager.hpp"
#include "multimap_iterator.hpp"
#include "bloom_filter_manager.hpp"
#include "file_modes.h"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <cassert>

/**
 * The map_multimap_manager<T> treats map_manager<T> and multimap_manager<T>
 * as a single managed database.
 */
template<class T>
class map_multimap_manager_t {
  private:
  const std::string hashdb_dir;
  const file_mode_type_t file_mode_type;

  const hashdb_settings_t settings;
  map_manager_t<T> map_manager;
  multimap_manager_t<T> multimap_manager;
  bloom_filter_manager_t<T> bloom_filter_manager;

  // do not allow these
  map_multimap_manager_t(const map_multimap_manager_t&);
  map_multimap_manager_t& operator=(const map_multimap_manager_t& that);

  // helper
  void map_emplace(const T& key, uint64_t source_lookup_encoding) {
    std::pair<map_iterator_t<T>, bool> emplace_pair =
                             map_manager.emplace(key, source_lookup_encoding);
    if (emplace_pair.second != true) {
      // really bad if emplace fails
      throw std::runtime_error("map emplace failure");
    }
  }

  // helper
  void multimap_emplace(const T& key, uint64_t source_lookup_encoding) {
    bool did_emplace = multimap_manager.emplace(key, source_lookup_encoding);
    if (did_emplace != true) {
      // really bad if emplace fails
      throw std::runtime_error("multimap emplace failure");
    }
  }

  // helper
  void map_change(const T& key, uint64_t source_lookup_encoding) {
    std::pair<map_iterator_t<T>, bool> change_pair =
                             map_manager.change(key, source_lookup_encoding);
    if (change_pair.second != true) {
      // really bad if change fails
      throw std::runtime_error("map change failure");
    }
  }

  // helper
  void map_erase(const T& key) {
    bool did_erase = map_manager.erase(key);
    if (did_erase != true) {
      // really bad if erase fails
      throw std::runtime_error("map erase failure");
    }
  }

  // helper
  void multimap_erase(const T& key, uint64_t pay) {
    bool did_erase = multimap_manager.erase(key, pay);
    if (did_erase != true) {
      // really bad if erase fails
      throw std::runtime_error("multimap erase failure");
    }
  }

  public:
  map_multimap_manager_t(const std::string& p_hashdb_dir,
                   file_mode_t p_file_mode_type) :
          hashdb_dir(p_hashdb_dir),
          file_mode(p_file_mode_type),
          settings(hashdb_dir),
          map_manager(hashdb_dir, file_mode, settings.map_type),
          multimap_manager(hashdb_dir, file_mode, settings.multimap_type),
          bloom_filter_manager(hashdb_dir, file_mode,
                               settings.bloom1_is_used,
                               settings.bloom1_M_hash_size,
                               settings.bloom1_k_hash_functions,
                               settings.bloom2_is_used,
                               settings.bloom2_M_hash_size,
                               settings.bloom2_k_hash_functions) {
  }

  void emplace(const T& key, uint64_t source_lookup_encoding,
               hashdb_changes_t& changes) {

    // if key not in bloom filter then emplace directly
    if (!bloom_filter_manager.is_positive(key)) {
      // key not present so add it in bloom filter and in map
      bloom_filter_manager.add_hash_value(key);
      map_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;
      return;
    }

    // bloom filter gave positive, so see if this key is in map
    map_iterator_t<T> map_iterator = map_manager.find(key);
    if (map_iterator == map_manager.end()) {
      // key not in map so add element to map
      map_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;
      return;
    }

    // key was in map, so add key to multimap, potentially changing pay in map
    uint32_t count = source_lookup_encoding::get_count(map_iterator->second);
    if (count == 1) {
      // move element in map to multimap
      multimap_emplace(key, map_iterator->second);
      map_change(key, source_lookup_encoding::get_source_lookup_encoding(2));

      // add key to multimap
      multimap_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;

    } else {
      // increment count in map
      map_change(key,
                 source_lookup_encoding::get_source_lookup_encoding(count + 1);

      // add key to multimap
      multimap_emplace(key, source_lookup_encoding);
      ++changes.hashes_inserted;
    }
  }

  void remove(const T& key, uint64_t source_lookup_encoding,
              hashdb_changes_t& changes) {

    // approach depends on count
    map_iterator_t<T> map_iterator = map_manager.find(key);
    if (map_iterator == map_manager.end()) {
      // no key
      ++changes.hashes_not_removed_no_element;
      return;
    }

    uint32_t count = source_lookup_encoding::get_count(map_iterator->second);
    if (count == 1) {
      // remove element from map if pay matches
      if (map_iterator->second == source_lookup_encoding) {
        // matches
        map_erase(key);
        ++changes.hashes_removed;
      } else {
        // the one element in map does not match
        ++changes.hashes_not_removed_no_element;
      }

    } else if (count == 2) {
      // remove element from multimap if pay matches
      bool did_erase = multimap.erase(key, source_lookup_encoding);
      if (did_erase) {
        ++changes.hashes_removed;

        // also move last remaining element in multimap to map
        std::pair<multimap_iterator_t<T>, multimap_iterator_t<T> >
                                                        equal_range(key);
        map_replace(equal_range.first->first, equal_range.first->second);
        multimap_erase(equal_range.first->first, equal_range.first->second);

        // lets also verify that multimap is now empty for this key
        if (multimap.has_range(key) {
          throw std::runtime_error("corrupted multimap state failure");
        }
      } else {
        // pay wasn't in multimap either
        ++changes.hashes_not_removed_no_element;
      }
    } else {
      // count > 2 so just try to remove element from multimap
      bool did_erase2 = multimap.erase(key, source_lookup_encoding);
      if (did_erase2 == true) {
        ++changes.hashes_removed;
      } else {
        // pay wasn't in multimap either
        ++changes.hashes_not_removed_no_element;
      }
    }
  }

  void remove_key(const T& key, hashdb_changes_t& changes) {
    // approach depends on count
    map_iterator_t<T> map_iterator = map_manager.find(key);
    if (map_iterator == map_manager.end()) {
      // no key
      ++changes.hashes_not_removed_no_key;
      return;
    }

    uint32_t count = source_lookup_encoding::get_count(map_iterator->second);
    if (count == 1) {
      // remove element from map
      map_erase(key);
    } else {
      // remove multiple elements from multimap
      multimap_erase(key);
    }
    changes.hashes_removed += count;
  }

  bool has_key(const T& key) {
    // if key not in bloom filter then check directly
    if (!bloom_filter_manager.is_positive(key)) {
      // key not present in bloom filter
      return false;
    }

    // check for presence in map
    return map.has(key);
  }

  size_t size() {
    return map.size();
  }

  hashdb_iterator_t<T> begin() {
    return map_multimap_iterator_t<T>(&map_manager, &multimap_manager, false);
  }

  hashdb_iterator_t<T> end() {
    return map_multimap_iterator_t<T>(&map_manager, &multimap_manager, true);
  }
};

#endif
