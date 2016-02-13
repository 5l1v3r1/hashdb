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
 * Manage the LMDB hash data store.  Threadsafe.
 *
 * lmdb_hash_data_manager_t supports three types of data fields:
 *
 * Type 1: only one entry for this hash:
 *         source_id, file_offset, entropy, block_label
 *
 * Type 2: first line of multi-entry hash:
 *         NULL, entropy, block_label
 *
 * Type 3: remaining lines of multi-entry hash:
 *         source_id, file_offset
 *
 * NOTES:
 *   * Source ID must be > 0 because this field also distinguishes between
 *     type 1 and Type 2 data.
 *   * LMDB sorts Type 2 before Type 3 records because of the sort order
 *     imposed by NULL.
 *   * On insert, if the file offset is invalid, nothing changes and 0 is
 *     returned.
 */

#ifndef LMDB_HASH_DATA_MANAGER_HPP
#define LMDB_HASH_DATA_MANAGER_HPP

//#define DEBUG_LMDB_HASH_DATA_MANAGER_HPP

#include "file_modes.h"
#include "lmdb.h"
#include "lmdb_helper.h"
#include "lmdb_context.hpp"
#include "lmdb_changes.hpp"
#include <vector>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>
#include <set>
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
#include "print_lmdb_val.hpp"
#endif

// no concurrent writes
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "mutex_lock.hpp"

namespace hashdb {

class lmdb_hash_data_manager_t {

  private:
  typedef std::pair<uint64_t, uint64_t> id_offset_pair_t;
  typedef std::set<id_offset_pair_t>    id_offset_pairs_t;

  const std::string hashdb_dir;
  const hashdb::file_mode_type_t file_mode;
  const uint32_t sector_size;
  const uint32_t max_id_offset_pairs;
  id_offset_pairs_t* id_offset_pairs;
  MDB_env* env;

#ifdef HAVE_PTHREAD
  mutable pthread_mutex_t M;                  // mutext
#else
  mutable int M;                              // placeholder
#endif

  // do not allow copy or assignment
  lmdb_hash_data_manager_t(const lmdb_hash_data_manager_t&);
  lmdb_hash_data_manager_t& operator=(const lmdb_hash_data_manager_t&);

  // delete element at current cursor
  void delete_cursor_entry(hashdb::lmdb_context_t& context) {
    int rc = mdb_cursor_del(context.cursor, 0);

    // the removal must work
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // get count at current cursor
  size_t get_cursor_count(hashdb::lmdb_context_t& context) const {
    size_t cursor_count;
    int rc = mdb_cursor_count(context.cursor, &cursor_count);
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
    return cursor_count;
  }

  // write Type 1 context.data, use existing context.key, call mdb_put.
  int put_type1(hashdb::lmdb_context_t& context,
                const uint64_t source_id, const uint64_t file_offset,
                const uint64_t entropy, const std::string& block_label) {

    // prepare Type 1 entry:

    // calculate file_offset_index
    if (file_offset % sector_size != 0) {
      assert(0);
    }
    uint64_t file_offset_index = file_offset / sector_size;

    // make data with enough space for fields
    const size_t block_label_size = block_label.size();
    uint8_t data[10 + 10 +10 + (10 + block_label_size)];
    uint8_t* p = data;

    // set fields
    p = lmdb_helper::encode_uint64_t(source_id, p);
    p = lmdb_helper::encode_uint64_t(file_offset_index, p);
    p = lmdb_helper::encode_uint64_t(entropy, p);
    p = lmdb_helper::encode_uint64_t(block_label_size, p);
    std::memcpy(p, block_label.c_str(), block_label_size);
    p += block_label_size;

    // store data at new key
    context.data.mv_size = p - data;
    context.data.mv_data = data;
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager put_type1 key", context.key);
print_mdb_val("hash_data_manager put_type1 data", context.data);
#endif
    return rc;
  }

  // parse Type 1 context.data into these parameters
  void decode_type1(hashdb::lmdb_context_t& context,
                    uint64_t& source_id, uint64_t& file_offset,
                    uint64_t& entropy, std::string& block_label) const {

    // prepare to read Type 1 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // file offset
    uint64_t file_offset_index;
    p = lmdb_helper::decode_uint64_t(p, file_offset_index);
    file_offset = file_offset_index * sector_size;

    // hash data entropy
    p = lmdb_helper::decode_uint64_t(p, entropy);

    // read the hash data block_label size
    uint64_t block_label_size;
    p = lmdb_helper::decode_uint64_t(p, block_label_size);

    // read the hash data block_label
    block_label =
       std::string(reinterpret_cast<const char*>(p), block_label_size);
    p += block_label_size;

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      assert(0);
    }
  }

  // write Type 2 context.data, use existing context.key, call mdb_put.
  int put_type2(hashdb::lmdb_context_t& context,
                const uint64_t entropy, const std::string& block_label) {

    // prepare Type 2 entry:

    // make data with enough space for fields
    const size_t block_label_size = block_label.size();
    uint8_t data[1 + 10 + (10 + block_label_size)];
    uint8_t* p = data;

    // add NULL byte
    p[0] = 0;
    ++p;

    // set fields
    p = lmdb_helper::encode_uint64_t(entropy, p);
    p = lmdb_helper::encode_uint64_t(block_label_size, p);
    std::memcpy(p, block_label.c_str(), block_label_size);
    p += block_label_size;

    // store data at new key
    context.data.mv_size = p - data;
    context.data.mv_data = data;
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager put_type2 key", context.key);
print_mdb_val("hash_data_manager put_type2 data", context.data);
#endif

    return rc;
  }

  // parse Type 2 context.data into these parameters
  void decode_type2(hashdb::lmdb_context_t& context,
                    uint64_t& entropy, std::string& block_label) const {

#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager decode_type2 key", context.key);
print_mdb_val("hash_data_manager decode_type2 data", context.data);
#endif
    // prepare to read Type 2 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // move past the NULL byte
    ++p;

    // hash data entropy
    p = lmdb_helper::decode_uint64_t(p, entropy);

    // read the hash data block_label size
    uint64_t block_label_size;
    p = lmdb_helper::decode_uint64_t(p, block_label_size);

    // read the hash data block_label
    block_label =
       std::string(reinterpret_cast<const char*>(p), block_label_size);
    p += block_label_size;

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      assert(0);
    }
  }

  // write Type 3 context.data, use existing context.key, call mdb_put.
  int put_type3(hashdb::lmdb_context_t& context,
                const uint64_t source_id, const uint64_t file_offset) {

    // prepare Type 3 entry:

    // calculate file_offset_index
    if (file_offset % sector_size != 0) {
      assert(0);
    }
    uint64_t file_offset_index = file_offset / sector_size;

    // make data with enough space for fields
    uint8_t data[10 + 10];
    uint8_t* p = data;

    // set fields
    p = lmdb_helper::encode_uint64_t(source_id, p);
    p = lmdb_helper::encode_uint64_t(file_offset_index, p);

    // store data at new key
    context.data.mv_size = p - data;
    context.data.mv_data = data;
    int rc = mdb_put(context.txn, context.dbi,
                     &context.key, &context.data, MDB_NODUPDATA);
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager put_type3 key", context.key);
print_mdb_val("hash_data_manager put_type3 data", context.data);
#endif
    return rc;
  }

  // parse Type 3 context.data into these parameters
  void decode_type3(hashdb::lmdb_context_t& context,
                    uint64_t& source_id, uint64_t& file_offset) const {

    // prepare to read Type 3 entry:
    const uint8_t* const p_start = static_cast<uint8_t*>(context.data.mv_data);
    const uint8_t* p = p_start;

    // source ID
    p = lmdb_helper::decode_uint64_t(p, source_id);

    // file offset
    uint64_t file_offset_index;
    p = lmdb_helper::decode_uint64_t(p, file_offset_index);
    file_offset = file_offset_index * sector_size;

    // read must align to data record
    if (p != p_start + context.data.mv_size) {
      assert(0);
    }
  }

  public:
  lmdb_hash_data_manager_t(const std::string& p_hashdb_dir,
                           const hashdb::file_mode_type_t p_file_mode,
                           const uint32_t p_sector_size,
                           const uint32_t p_max_id_offset_pairs) :
       hashdb_dir(p_hashdb_dir),
       file_mode(p_file_mode),
       sector_size(p_sector_size),
       max_id_offset_pairs(p_max_id_offset_pairs),
       id_offset_pairs(new id_offset_pairs_t),
       env(lmdb_helper::open_env(hashdb_dir + "/lmdb_hash_data_store",
                                                                file_mode)),
       M() {

    MUTEX_INIT(&M);

    // require valid parameters
    if (sector_size == 0) {
      std::cerr << "invalid hash data store configuration\n";
      assert(0);
    }
  }

  ~lmdb_hash_data_manager_t() {
    // close the lmdb_hash_store DB environment
    mdb_env_close(env);
    delete id_offset_pairs;

    MUTEX_DESTROY(&M);
  }

  /**
   * Insert hash with source data and metadata.  Overwrite data if there
   * and changed.  Return source count.
   */
  size_t insert(const std::string& binary_hash,
                const uint64_t source_id,
                const uint64_t file_offset,
                const uint64_t entropy,
                const std::string& block_label,
                hashdb::lmdb_changes_t& changes) {

    // program error if source ID is 0 since NULL distinguishes between
    // type 1 and type 2 data.
    if (source_id == 0) {
      assert(0);
    }

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // reject invalid file_offset
    if (file_offset % sector_size != 0) {
      ++changes.hash_data_invalid_file_offset;
      return 0;
    }

    MUTEX_LOCK(&M);

    // maybe grow the DB
    lmdb_helper::maybe_grow(env);

    // get context
    hashdb::lmdb_context_t context(env, true, true);
    context.open();
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager insert", context.cursor);
#endif

    // set key
    context.key.mv_size = binary_hash.size();
    context.key.mv_data =
               static_cast<void*>(const_cast<char*>(binary_hash.c_str()));

    // see if hash is already there
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == MDB_NOTFOUND) {
      // hash is not there
      rc = put_type1(context, source_id, file_offset, entropy, block_label);
      if (rc != 0) {
        std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
        assert(0);
      }
      ++changes.hash_data_data_inserted;
      ++changes.hash_data_source_inserted;
      context.close();
      MUTEX_UNLOCK(&M);
      return 1;
 
    } else if (rc == 0) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert found key", context.key);
print_mdb_val("hash_data_manager insert found data", context.data);
#endif
      // require data to have size
      if (context.data.mv_size == 0) {
        assert(0);
      }

      // find the existing entry type
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {

        // existing entry is Type 1:
        uint64_t p_source_id;
        uint64_t p_file_offset;
        uint64_t p_entropy;
        std::string p_block_label;
        decode_type1(context,
                     p_source_id, p_file_offset, p_entropy, p_block_label);

        // note if the source portion is the same
        const bool source_same = source_id == p_source_id &&
                               file_offset == p_file_offset;

        // note if the data portion is the same
        const bool data_same = entropy == p_entropy &&
                               block_label == p_block_label;

        // note if at max duplicates
        const bool at_max = (max_id_offset_pairs != 0 &&
                             1 >= max_id_offset_pairs);

        // tally changes
        if (data_same) {
          ++changes.hash_data_data_same;
        } else {
          ++changes.hash_data_data_changed;
        }
        if (at_max) {
          ++changes.hash_data_source_at_max;
        } else if (source_same) {
          ++changes.hash_data_source_already_present;
        } else {
          ++changes.hash_data_source_inserted;
        }

        if ((source_same || at_max) && data_same) {
          // no chane
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert no, same key", context.key);
print_mdb_val("hash_data_manager insert no, same data", context.data);
#endif
          context.close();
          MUTEX_UNLOCK(&M);
          return 1;

        } else if ((source_same || at_max) && !data_same) {
          // source same, data changed, so replace Type 1
          delete_cursor_entry(context);
          rc = put_type1(context, source_id, file_offset, entropy, block_label);
          if (rc != 0) {
            std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }
          context.close();
          MUTEX_UNLOCK(&M);
          return 1;

        } else {
          // add new source so remove Type 1 and add Type 2 and Type 3

          // remove Type 1
          delete_cursor_entry(context);
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager insert check key", context.key);
print_mdb_val("hash_data_manager insert check data", context.data);
#endif

          // add Type 2 and two Type 3
          rc = put_type2(context, entropy, block_label);
          if (rc != 0) {
            std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }
          rc = put_type3(context, p_source_id, p_file_offset);
          if (rc != 0) {
            std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }
          rc = put_type3(context, source_id, file_offset);
          if (rc != 0) {
            std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }

          context.close();
          MUTEX_UNLOCK(&M);
          return 2;
        }

      } else {
        // existing entry is type 2

        // get count before add and before disrupting the cursor,
        // subtracting the one Type 2 entry
        size_t count = get_cursor_count(context) - 1;

        // check data and maybe change it
        uint64_t p_entropy;
        std::string p_block_label;
        decode_type2(context, p_entropy, p_block_label);
        const bool data_same = entropy == p_entropy &&
                               block_label == p_block_label;
        if (data_same) {
          // data same
          ++changes.hash_data_data_same;
        } else {
          // change the Type 2 entry to contain the changed data
          ++changes.hash_data_data_changed;
          delete_cursor_entry(context);
          rc = put_type2(context, entropy, block_label);
        }

        if (count >= max_id_offset_pairs) {
          // at max
          ++changes.hash_data_source_at_max;

        } else {
          // add source and see if it is new and adds or not
          rc = put_type3(context, source_id, file_offset);
          if (rc == 0) {
            ++changes.hash_data_source_inserted;
            ++count;
          } else if (rc == MDB_KEYEXIST) {
            ++changes.hash_data_source_already_present;
          } else {
            std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
            assert(0);
          }
        }

        context.close();
        MUTEX_UNLOCK(&M);
        return count;
      }

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Read data for the hash.  False if the hash does not exist.
   */
  bool find(const std::string& binary_hash,
            uint64_t& entropy,
            std::string& block_label,
            id_offset_pairs_t& pairs) const {

    // clear any previous values
    pairs.clear();

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_whole_mdb("hash_data_manager find", context.cursor);
#endif

//void* zz1 = static_cast<void*>(const_cast<char*>(binary_hash.c_str()));
//uint8_t* zz2 = static_cast<uint8_t*>(zz1);
//uint8_t* zz3 = static_cast<uint8_t*>(static_cast<void*>(const_cast<char*>(binary_hash.c_str())));

    // set key
    const size_t key_size = binary_hash.size();
    uint8_t* key_start = static_cast<uint8_t*>(
                 static_cast<void*>(const_cast<char*>(binary_hash.c_str())));
    context.key.mv_size = key_size;
    context.key.mv_data = key_start;

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find start at key", context.key);
print_mdb_val("hash_data_manager find start at data", context.data);
#endif

    if (rc == MDB_NOTFOUND) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find did not find key", context.key);
#endif
      // no hash
      context.close();
      entropy = 0;
      block_label = "";
      return false;

    } else if (rc == 0) {
      // check first byte to see if the entry is Type 1 or Type 2
      // require data to have size
      if (context.data.mv_size == 0) {
        assert(0);
      }

      // find the existing entry type
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {
        // existing entry is Type 1 so read it and be done:
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 1 key", context.key);
print_mdb_val("hash_data_manager find Type 1 data", context.data);
#endif
        uint64_t p_source_id;
        uint64_t p_file_offset;
        decode_type1(context,
                     p_source_id, p_file_offset, entropy, block_label);
        pairs.insert(id_offset_pair_t(p_source_id, p_file_offset));
        context.close();
        return true;

      } else {
        // existing entry is Type 2 so read all entries for this hash
        // read this Type 2 entry
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 2 key", context.key);
print_mdb_val("hash_data_manager find Type 2 data", context.data);
#endif
        decode_type2(context, entropy, block_label);

        // read pairs from Type 3 entries while data available and key matches
        while (true) {
          rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                              MDB_NEXT);
          if (rc == 0 &&
              context.key.mv_size == key_size &&
              memcmp(context.key.mv_data, key_start, key_size) == 0) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 3 key", context.key);
print_mdb_val("hash_data_manager find Type 3 data", context.data);
#endif

            // add the pair for this next Type 3 entry
            uint64_t p_source_id;
            uint64_t p_file_offset;
            decode_type3(context, p_source_id, p_file_offset);
            pairs.insert(id_offset_pair_t(p_source_id, p_file_offset));
          } else {
            // no more entries with this key
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find Type 3 done, key", context.key);
print_mdb_val("hash_data_manager find Type 3 done, data", context.data);
#endif
            break;
          }
        }

        // make sure rc is valid
        if (rc == 0 || rc == MDB_NOTFOUND) {
          // good, at next key or at end of DB
          context.close();
          return true;

        } else {
          // invalid rc
          std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
          assert(0);
        }
      }
    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Return source count for this hash.
   */
  size_t find_count(const std::string& binary_hash) const {

    // require valid binary_hash
    if (binary_hash.size() == 0) {
      std::cerr << "empty key\n";
      assert(0);
    }

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();

    // set key
    context.key.mv_size = binary_hash.size();
    context.key.mv_data =
                 static_cast<void*>(const_cast<char*>(binary_hash.c_str()));

    // set the cursor to this key
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    if (rc == 0) {
      // hash found
      // check first byte to see if the entry is Type 1 or Type 2
      // require data to have size
      if (context.data.mv_size == 0) {
        assert(0);
      }

      // find the existing entry type
      if (static_cast<uint8_t*>(context.data.mv_data)[0] != 0) {
        // Type 1 so count is 1
        context.close();
        return 1;
      } else {
        // Type 2 so use cursor count
        size_t cursor_count = get_cursor_count(context);
        // value of 1 is not valid
        if (cursor_count == 1) {
          assert(0);
        }
        context.close();
        return cursor_count - 1;
      }

    } else if (rc == MDB_NOTFOUND) {
      // this hash is not in the DB
      context.close();
      return 0;

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Return first hash.
   */
  std::pair<bool, std::string> find_begin() const {

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();

    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_FIRST);

    if (rc == 0) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find_begin key", context.key);
print_mdb_val("hash_data_manager find_begin data", context.data);
#endif
      // return the key
      std::string binary_hash = std::string(
              static_cast<char*>(context.key.mv_data), context.key.mv_size);
      context.close();
      return std::pair<bool, std::string>(true, binary_hash);

    } else if (rc == MDB_NOTFOUND) {
      // no hash
      context.close();
      return std::pair<bool, std::string>(false, std::string(""));

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  /**
   * Return next hash.  Error if no next.
   */
  std::pair<bool, std::string> find_next(
                        const std::string& last_binary_hash) const {

    if (last_binary_hash == "") {
      // program error to ask for next when at end
      std::cerr << "find_next: already at end\n";
      assert(0);
    }

    // get context
    hashdb::lmdb_context_t context(env, false, true);
    context.open();

    // set the cursor to last hash
    context.key.mv_size = last_binary_hash.size();
    context.key.mv_data =
             static_cast<void*>(const_cast<char*>(last_binary_hash.c_str()));
    int rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                            MDB_SET_KEY);

    // the last hash must exist
    if (rc != 0) {
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }

    // move cursor to this hash
    rc = mdb_cursor_get(context.cursor, &context.key, &context.data,
                        MDB_NEXT_NODUP);

    if (rc == MDB_NOTFOUND) {
      // no values for this hash
      context.close();
      return std::pair<bool, std::string>(false, std::string(""));

    } else if (rc == 0) {
#ifdef DEBUG_LMDB_HASH_DATA_MANAGER_HPP
print_mdb_val("hash_data_manager find_next key", context.key);
print_mdb_val("hash_data_manager find_next data", context.data);
#endif

      // return this hash
      std::string binary_hash = std::string(
              static_cast<char*>(context.key.mv_data), context.key.mv_size);
      context.close();
      return std::pair<bool, std::string>(true, binary_hash);

    } else {
      // invalid rc
      std::cerr << "LMDB error: " << mdb_strerror(rc) << "\n";
      assert(0);
    }
  }

  // call this from a lock to prevent getting an unstable answer.
  size_t size() const {
    return lmdb_helper::size(env);
  }
};

} // end namespace hashdb

#endif

