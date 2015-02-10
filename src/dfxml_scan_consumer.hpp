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
 * Unfortunately, the existing hashdigest reader output is hard to consume.
 * To get by, we use this consumer, which contains the pointer to the
 * scan input data structure.
 */

#ifndef DFXML_SCAN_CONSUMER_HPP
#define DFXML_SCAN_CONSUMER_HPP
#include <string>
#include "lmdb_source_data.hpp"

class dfxml_scan_consumer_t {

  private:
  std::vector<std::string>* scan_input;

  // do not allow copy or assignment
  dfxml_scan_consumer_t(const dfxml_scan_consumer_t&);
  dfxml_scan_consumer_t& operator=(const dfxml_scan_consumer_t&);

  public:
  dfxml_scan_consumer_t(
              std::vector<std::string>* p_scan_input) :
        scan_input(p_scan_input) {
  }

  // end_fileobject_filename
  void end_fileobject_filename(const std::string& filename) {
    // no action for this consumer
  }

  // end_byte_run
  void end_byte_run(const std::string& binary_hash,
                    uint64_t file_offset,
                    const lmdb_source_data_t& source_data) {

    // consume the hashdb_element by adding it to scan_input
    scan_input->push_back(binary_hash);
  }

  // end_fileobject
  void end_fileobject(const lmdb_source_data_t& source_data) {

    // no action for this consumer
  }
};

#endif

