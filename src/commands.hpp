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
 * Provides hashdb commands.
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include "../src_libhashdb/hashdb.hpp"
#include "import_tab.hpp"
#include "import_json.hpp"
#include "export_json.hpp"
#include "scan_hashes.hpp"
#include "adder.hpp"
#include "adder_set.hpp"
#include "hex_helper.hpp"
//#include "expand_manager.hpp"
//#include "dfxml_scan_consumer.hpp"
//#include "dfxml_scan_expanded_consumer.hpp"
//#include "dfxml_hashdigest_writer.hpp"

// Standard includes
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

// leave alone else create using existing settings if new
void create_if_new(const std::string& hashdb_dir,
                   const std::string& from_hashdb_dir,
                   const std::string& command_string) {

  std::pair<bool, std::string> pair;
  hashdb::settings_t settings;

  // try to read hashdb_dir settings
  pair = hashdb::read_settings(hashdb_dir, settings);
  if (pair.first == true) {
    // hashdb_dir already exists
    return;
  }

  // no hashdb_dir, so read from_hashdb_dir settings
  pair = hashdb::read_settings(from_hashdb_dir, settings);
  if (pair.first == false) {
    // bad since from_hashdb_dir is not valid
    std::cout << "Error: " << pair.second << "\n";
    exit(1);
  }

  // create hashdb_dir using from_hashdb_dir settings
  pair = hashdb::create_hashdb(hashdb_dir, settings, command_string);
  if (pair.first == false) {
    // bad since from_hashdb_dir is not valid
    std::cout << "Error: " << pair.second << "\n";
    exit(1);
  }
}

// require hashdb_dir else fail
static void require_hashdb_dir(const std::string& hashdb_dir) {
  std::pair<bool, std::string> pair;
  hashdb::settings_t settings;
  pair = hashdb::read_settings(hashdb_dir, settings);
  if (pair.first == false) {
    std::cout << "Error: " << pair.second << "\n";
    exit(1);
  }
}

static void print_header(const std::string& command_id,
                         const std::string& cmd) {
  std::cout << "# hashdb-Version: " << PACKAGE_VERSION << "\n"
            << "# " << command_id << "\n"
            << "# command_line: " << cmd << "\n";
}

namespace commands {

  // ************************************************************
  // new database
  // ************************************************************
  void create(const std::string& hashdb_dir,
              const hashdb::settings_t& settings,
              const std::string& cmd) {

    std::pair<bool, std::string> pair = hashdb::create_hashdb(
                                             hashdb_dir, settings, cmd);

    if (pair.first == true) {
      std::cout << "New database created.\n";
    } else {
      std::cout << "Error: " << pair.second << "\n";
    }
  }

  // ************************************************************
  // import/export
  // ************************************************************
  // import
  static void import(const std::string& hashdb_dir,
                     const std::string& import_dir,
                     const std::string& repository_name,
                     const std::string& whitelist_dir,
                     const std::string& cmd) {
    // import_dir.hpp
    std::cout << "TBD\n";
  }

  // import_tab
  static void import_tab(const std::string& hashdb_dir,
                     const std::string& tab_file,
                     const std::string& repository_name,
                     const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the tab file for reading
    std::ifstream in(tab_file.c_str());
    if (!in.is_open()) {
      std::cout << "Error: Cannot open " << tab_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    import_tab_t::read(hashdb_dir, tab_file, repository_name, cmd, in);

    // done
    in.close();
    std::cout << "import_tab completed.\n";
  }

  // import_json
  static void import_json(const std::string& hashdb_dir,
                          const std::string& json_file,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the JSON file for reading
    std::ifstream in(json_file.c_str());
    if (!in.is_open()) {
      std::cout << "Error: Cannot open " << json_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    // import the hashdb
    import_json_t::read(hashdb_dir, cmd, in);

    // done
    in.close();
    std::cout << "import_json completed.\n";
  }

  // export_json
  static void export_json(const std::string& hashdb_dir,
                          const std::string& json_file,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the JSON file for writing
    std::ofstream out(json_file.c_str());
    if (!out.is_open()) {
      std::cout << "Error: Cannot open " << json_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    // export the hashdb
    export_json_t::write(hashdb_dir, cmd, out);

    // done
    out.close();
    std::cout << "export_json completed.\n";
  }

  // ************************************************************
  // database manipulation
  // ************************************************************
  // add
  static void add(const std::string& hashdb_dir,
                  const std::string& dest_dir,
                  const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    adder_t adder(&manager_a, &manager_b);

    // add data for binary_hash from A to B
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {
      // add the hash
      adder.add(pair.second);
      pair = manager_a.hash_next(pair.second);
    }
  }

  // add_multiple
  // Flow:
  //   1) Create an ordered multimap of key=hash, value=producer_t
  //      where key is the first key from a producer.
  //   2) Consume elements from the ordered multimap and copy them
  //      until the producers are depleted.  Do not enque when a producer
  //      is depleted.  Done when the ordered multimap becomes empty.
  static void add_multiple(const std::vector<std::string>& p_hashdb_dirs,
                           const std::string& cmd) {

    std::vector<std::string> hashdb_dirs = p_hashdb_dirs;

    // read then strip off dest_dir from end of list
    const std::string dest_dir = hashdb_dirs.back();
    hashdb_dirs.pop_back();

    // validate hashdb directories, maybe make dest_dir
    for (std::vector<std::string>::const_iterator it = hashdb_dirs.begin();
                    it != hashdb_dirs.end(); ++it) {
      require_hashdb_dir(*it);
    }
    create_if_new(dest_dir, hashdb_dirs[0], cmd);

    // open the consumer at dest_dir
    hashdb::import_manager_t consumer(dest_dir, cmd);

    // define the ordered multimap of key=hash, value=producer_t
    typedef std::pair<hashdb::scan_manager_t*, adder_t*> producer_t;
    typedef std::pair<std::string, producer_t> ordered_producers_value_t;
    typedef std::multimap<std::string, producer_t> ordered_producers_t;

    // create the multimap of ordered producers
    ordered_producers_t ordered_producers;

    // hash_pair<bool, hash> from a scan_manager read
    std::pair<bool, std::string> hash_pair;

    // open the producers
    for (std::vector<std::string>::const_iterator it = hashdb_dirs.begin();
                    it != hashdb_dirs.end(); ++it) {
      std::string hashdb_dir = *it;
      hashdb::scan_manager_t* producer = new hashdb::scan_manager_t(hashdb_dir);
      hash_pair = producer->hash_begin();
      if (hash_pair.first == true) {
        // the producer is not empty, so enqueue it
        // create the adder
        adder_t* adder = new adder_t(producer, &consumer);
        ordered_producers.insert(ordered_producers_value_t(hash_pair.second,
                                      producer_t(producer, adder)));

      } else {
        // no hashes for this producer so close it
        delete producer;
      }
    }
      
    // add ordered hashes from producers until all hashes are consumed
    while (ordered_producers.size() != 0) {
      // get the hash, producer, and adder for the first hash
      ordered_producers_t::iterator it = ordered_producers.begin();
      hashdb::scan_manager_t* producer = it->second.first;
      adder_t* adder = it->second.second;

      // add the hash to the consumer
      adder->add(it->first);

      // get the next hash from this producer
      hash_pair = producer->hash_next(it->first);

      // remove this hash, producer_t entry
      ordered_producers.erase(it);

      if (hash_pair.first) {
        // hash exists so add the hash, producer, and adder
        ordered_producers.insert(ordered_producers_value_t(hash_pair.second,
                                      producer_t(producer, adder)));
      } else {
        // no hashes for this producer so close it
        delete producer;
        delete adder;
      }
    }
  }

  // add_repository
  static void add_repository(const std::string& hashdb_dir,
                             const std::string& dest_dir,
                             const std::string& repository_name,
                             const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    adder_t adder(&manager_a, &manager_b, repository_name);

    // add data for binary_hash from A to B
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {
      // add the hash
      adder.add_repository(pair.second);
      pair = manager_a.hash_next(pair.second);
    }
  }

  // intersect A and B into C
  static void intersect(const std::string& hashdb_dir1,
                        const std::string& hashdb_dir2,
                        const std::string& dest_dir,
                        const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c);

    // iterate A to intersect A and B into C
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {

      // intersect if hash is in B
      size_t count = manager_b.find_hash_count(pair.second);
      if (count > 0) {
        // intersect
        adder_set.intersect(pair.second);
      }

      pair = manager_a.hash_next(pair.second);
    }
  }

  // intersect_hash
  static void intersect_hash(const std::string& hashdb_dir1,
                             const std::string& hashdb_dir2,
                             const std::string& dest_dir,
                             const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c);

    // iterate A to intersect_hash A and B into C
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {

      // intersect if hash is in B
      size_t count = manager_b.find_hash_count(pair.second);
      if (count > 0) {
        // intersect_hash
        adder_set.intersect_hash(pair.second);
      }

      pair = manager_a.hash_next(pair.second);
    }
  }

  // subtract
  static void subtract(const std::string& hashdb_dir1,
                       const std::string& hashdb_dir2,
                       const std::string& dest_dir,
                       const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c);

    // iterate A to add A to C if A hash and source not in B
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {

      // add A to C if A hash and source not in B
      adder_set.subtract(pair.second);

      pair = manager_a.hash_next(pair.second);
    }
  }

  // subtract_hash
  static void subtract_hash(const std::string& hashdb_dir1,
                            const std::string& hashdb_dir2,
                            const std::string& dest_dir,
                            const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir1);
    require_hashdb_dir(hashdb_dir2);
    create_if_new(dest_dir, hashdb_dir1, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir1);
    hashdb::scan_manager_t manager_b(hashdb_dir2);
    hashdb::import_manager_t manager_c(dest_dir, cmd);
    adder_set_t adder_set(&manager_a, &manager_b, &manager_c);

    // iterate A to add A to C if A hash not in B
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {

      // add A to C if A hash not in B
      adder_set.subtract_hash(pair.second);

      pair = manager_a.hash_next(pair.second);
    }
  }

  // subtract_repository
  static void subtract_repository(const std::string& hashdb_dir,
                                  const std::string& dest_dir,
                                  const std::string& repository_name,
                                  const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    adder_t adder(&manager_a, &manager_b, repository_name);

    // add data for binary_hash from A to B
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {
      // add the hash
      adder.add_non_repository(pair.second);
      pair = manager_a.hash_next(pair.second);
    }
  }

  // deduplicate
  static void deduplicate(const std::string& hashdb_dir,
                          const std::string& dest_dir,
                          const std::string& cmd) {

    // validate hashdb directories, maybe make dest_dir
    require_hashdb_dir(hashdb_dir);
    create_if_new(dest_dir, hashdb_dir, cmd);

    // resources
    hashdb::scan_manager_t manager_a(hashdb_dir);
    hashdb::import_manager_t manager_b(dest_dir, cmd);
    adder_t adder(&manager_a, &manager_b);

    // add data for binary_hash from A to B
    std::pair<bool, std::string> pair = manager_a.hash_begin();
    while (pair.first != false) {
      // add the hash
      adder.deduplicate(pair.second);
      pair = manager_a.hash_next(pair.second);
    }
  }

  // ************************************************************
  // scan
  // ************************************************************
  // scan
  static void scan(const std::string& hashdb_dir,
                   const std::string& hashes_file,
                   const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open the hashes file for reading
    std::ifstream in(hashes_file.c_str());
    if (!in.is_open()) {
      std::cout << "Error: Cannot open " << hashes_file
                << ": " << strerror(errno) << "\n";
      exit(1);
    }

    scan_hashes_t::read(hashdb_dir, cmd, in);

    // done
    in.close();
    std::cout << "# scan completed.\n";
  }

  // scan_hash
  static void scan_hash(const std::string& hashdb_dir,
                        const std::string& hex_block_hash,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // get the binary hash
    std::string binary_hash = hex_to_bin(hex_block_hash);

    // reject invalid input
    if (binary_hash == "") {
      std::cerr << "Error: Invalid hash: '" << hex_block_hash << "'\n";
      exit(1);
    }

    // open DB
    hashdb::scan_manager_t scan_manager(hashdb_dir);

    // scan
    std::string* expanded_text = new std::string;
    bool found = scan_manager.find_expanded_hash(binary_hash, *expanded_text);

    if (found == true) {
      std::cout << *expanded_text << std::endl;
    } else {
      std::cout << "Hash not found for '" << hex_block_hash << "'\n";
    }
    delete expanded_text;
  }

  // ************************************************************
  // statistics
  // ************************************************************
  // sizes
  static void sizes(const std::string& hashdb_dir,
                   const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    std::cout << manager.sizes() << std::endl;
  }

  // sources
  static void sources(const std::string& hashdb_dir,
                      const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // print the sources
    export_json_t::print_sources(hashdb_dir);
  }

  // histogram
  static void histogram(const std::string& hashdb_dir,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // print header information
    print_header("histogram-command-Version: 2", cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size(), cmd);

    // total number of hashes in the database
    uint64_t total_hashes = 0;

    // total number of distinct hashes
    uint64_t total_distinct_hashes = 0;

    // hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>* hash_histogram =
                new std::map<uint32_t, uint64_t>();

    // space for variables
    uint64_t entropy;
    std::string block_label;
    hashdb::source_offset_pairs_t* source_offset_pairs =
                                        new hashdb::source_offset_pairs_t;

    // iterate over hashdb and set variables for calculating the histogram
    std::pair<bool, std::string> pair = manager.hash_begin();

    // note if the DB is empty
    if (pair.first == false) {
      std::cout << "The map is empty.\n";
    }

    while (pair.first == true) {
      manager.find_hash(pair.second, entropy, block_label,
                            *source_offset_pairs);
      uint64_t count = source_offset_pairs->size();
      // update total hashes observed
      total_hashes += count;
      // update total distinct hashes
      if (count == 1) {
        ++total_distinct_hashes;
      }

      // update hash_histogram information
      // look for existing entry
      std::map<uint32_t, uint64_t>::iterator hash_histogram_it = hash_histogram->find(count);
      if (hash_histogram_it == hash_histogram->end()) {

        // this is the first hash found with this count value
        // so start a new element for it
        hash_histogram->insert(std::pair<uint32_t, uint64_t>(count, 1));

      } else {

        // increment existing value for number of hashes with this count
        uint64_t old_number = hash_histogram_it->second;
        hash_histogram->erase(count);
        hash_histogram->insert(std::pair<uint32_t, uint64_t>(
                                           count, old_number + 1));
      }

      // move forward
      pair = manager.hash_next(pair.second);
      progress_tracker.track_hash_data(*source_offset_pairs);
    }

    // show totals
    std::cout << "{\"total_hashes\": " << total_hashes << ", "
              << "\"total_distinct_hashes\": " << total_distinct_hashes << "}\n";

    // show hash histogram as <count, number of hashes with count>
    std::map<uint32_t, uint64_t>::iterator hash_histogram_it2;
    for (hash_histogram_it2 = hash_histogram->begin();
         hash_histogram_it2 != hash_histogram->end(); ++hash_histogram_it2) {
      std::cout << "{\"duplicates\":" << hash_histogram_it2->first
                << ", \"distinct_hashes\":" << hash_histogram_it2->second
                << ", \"total\":" << hash_histogram_it2->first *
                                 hash_histogram_it2->second << "}\n";
    }
    delete hash_histogram;
    delete source_offset_pairs;
  }

  // duplicates
  static void duplicates(const std::string& hashdb_dir,
                         const std::string& number_string,
                         const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert duplicates string to number
    uint32_t number = atoi(number_string.c_str());

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // there is nothing to report if the map is empty
    if (manager.size() == 0) {
      std::cout << "The map is empty.\n";
      return;
    }

    // print header information
    print_header("duplicates-command-Version: 2", cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size(), cmd);

    bool any_found = false;

    // space for variables
    uint64_t entropy;
    std::string block_label;
    hashdb::source_offset_pairs_t* source_offset_pairs =
                                     new hashdb::source_offset_pairs_t;
    std::string* expanded_text = new std::string;

    // iterate over hashdb and set variables for finding duplicates
    std::pair<bool, std::string> pair = manager.hash_begin();

    while (pair.first == true) {
      manager.find_hash(pair.second, entropy, block_label,
                                  *source_offset_pairs);
      if (source_offset_pairs->size() == number) {
        // show hash with requested duplicates number
        manager.find_expanded_hash(pair.second, *expanded_text);
        std::cout << bin_to_hex(pair.second) << "\t" << *expanded_text << "\n";
        any_found = true;
      }

      // move forward
      pair = manager.hash_next(pair.second);
      progress_tracker.track_hash_data(*source_offset_pairs);
    }

    // say so if nothing was found
    if (!any_found) {
      std::cout << "No hashes were found with this count.\n";
      return;
    }

    delete source_offset_pairs;
    delete expanded_text;
  }

  // hash_table
  static void hash_table(const std::string& hashdb_dir,
                         const std::string& hex_file_hash,
                         const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // open DB
    hashdb::scan_manager_t manager(hashdb_dir);

    // source data
    std::string file_binary_hash = hex_to_bin(hex_file_hash);
    uint64_t filesize = 0;
    std::string file_type = "";
    uint64_t nonprobative_count = 0;

    // see if this source is even present
    bool has_source_data = manager.find_source_data(file_binary_hash,
                                filesize, file_type, nonprobative_count);
    if (has_source_data == false) {
      // the source is not present
      std::cout << "There is no source with this file hash\n";
      return;
    }

    // print header information
    print_header("hash-table-command-Version: 3", cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, manager.size(), cmd);

    // space for variables
    uint64_t entropy;
    std::string block_label;
    hashdb::source_offset_pairs_t* source_offset_pairs =
                                       new hashdb::source_offset_pairs_t;
    std::string* expanded_text = new std::string;

    // look for hashes that belong to this source
    // get the first hash
    std::pair<bool, std::string> pair = manager.hash_begin();
    while (pair.first == true) {

      // read hash data for the hash
      manager.find_hash(pair.second, entropy, block_label,
                                                    *source_offset_pairs);

      // find references to this source
      for (hashdb::source_offset_pairs_t::const_iterator it =
                       source_offset_pairs->begin();
                       it!= source_offset_pairs->end(); ++it) {
        if (it->first == file_binary_hash) {
          // the source matches so print the hash and move on
          manager.find_expanded_hash(pair.second, *expanded_text);
          std::cout << bin_to_hex(pair.second) << "\t" << *expanded_text
                    << "\n";
          break;
        }
      }

      // move forward
      pair = manager.hash_next(pair.second);
      progress_tracker.track_hash_data(*source_offset_pairs);
    }
    delete source_offset_pairs;
    delete expanded_text;
  }

  // ************************************************************
  // performance analysis
  // ************************************************************
  // add_random
  static void add_random(const std::string& hashdb_dir,
                         const std::string& count_string,
                         const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // get sector size
    hashdb::settings_t settings;
    std::pair<bool, std::string> pair =
                        hashdb::read_settings(hashdb_dir, settings);
    const uint64_t sector_size = settings.sector_size;

    // initialize random seed
    srand (time(NULL));

    // open manager
    hashdb::import_manager_t manager(hashdb_dir, cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // set up the source
    std::string file_binary_hash = hex_to_bin("00");
    manager.insert_source_name(file_binary_hash, "add_random_repository_name",
                               "add_random_filename");
    manager.insert_source_data(file_binary_hash, 0, "", 0);

    // insert count random hshes into the database
    for (uint64_t i=0; i<count; i++) {

      // update progress tracker
      progress_tracker.track();

      // add hash
      manager.insert_hash(random_binary_hash(), file_binary_hash,
                          i*sector_size, 0, "");
    }
  }

  // scan_random
  static void scan_random(const std::string& hashdb_dir,
                          const std::string& count_string,
                          const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // initialize random seed
    srand (time(NULL)+1); // ensure seed is different by advancing 1 second

    // open manager
    hashdb::scan_manager_t manager(hashdb_dir);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // space for match
    std::string* expanded_text = new std::string;

    // scan random hashes where hash values are unlikely to match
    for (uint64_t i=1; i<=count; ++i) {
      std::string binary_hash = random_binary_hash();

      bool found = manager.find_expanded_hash(binary_hash, *expanded_text);

      if (found) {
        std::cout << "Match found, hash "
                  << bin_to_hex(binary_hash)
                  << ": " << *expanded_text << "\n";
      }

      // update progress tracker
      progress_tracker.track();
    }

    delete expanded_text;
  }

  // add_same
  // add same hash but different source offset
  static void add_same(const std::string& hashdb_dir,
                       const std::string& count_string,
                       const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // get sector size
    hashdb::settings_t settings;
    std::pair<bool, std::string> pair =
                        hashdb::read_settings(hashdb_dir, settings);
    const uint64_t sector_size = settings.sector_size;

    // open manager
    hashdb::import_manager_t manager(hashdb_dir, cmd);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // set up the source
    std::string file_binary_hash = hex_to_bin("00");
    manager.insert_source_name(file_binary_hash, "add_same_repository_name",
                               "add_same_filename");
    manager.insert_source_data(file_binary_hash, 0, "", 0);

    // hash to use
    std::string binary_hash = hex_to_bin("8000000000000000000000000000000000");

    // insert count same hshes into the database
    for (uint64_t i=0; i<count; i++) {

      // update progress tracker
      progress_tracker.track();

      // add hash
      manager.insert_hash(binary_hash, file_binary_hash, i*sector_size, 0, "");
    }
  }

  // scan_same
  static void scan_same(const std::string& hashdb_dir,
                        const std::string& count_string,
                        const std::string& cmd) {

    // validate hashdb_dir path
    require_hashdb_dir(hashdb_dir);

    // convert count string to number
    const uint64_t count = atol(count_string.c_str());

    // open manager
    hashdb::scan_manager_t manager(hashdb_dir);

    // start progress tracker
    progress_tracker_t progress_tracker(hashdb_dir, count, cmd);

    // space for match
    std::string* expanded_text = new std::string;

    // hash to use
    std::string binary_hash = hex_to_bin("8000000000000000000000000000000000");

    // scan same hash repeatedly
    for (uint64_t i=1; i<=count; ++i) {
      bool found = manager.find_expanded_hash(binary_hash, *expanded_text);

      if (!found) {
        std::cout << "Match not found, hash "
                  << bin_to_hex(binary_hash)
                  << ": " << expanded_text << "\n";
      }

      // update progress tracker
      progress_tracker.track();
    }

    delete expanded_text;
  }
}

#endif

