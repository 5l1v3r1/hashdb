// Author:  Bruce Allen
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
 * Provides the service of importing hash data from a file formatted
 * using tab delimited fields, specifically:
 * <file hash>\t<block hash>\t<block offset>\n
 */

#ifndef IMPORT_TAB_HPP
#define IMPORT_TAB_HPP

#include <iostream>
#include "../src_libhashdb/hashdb.hpp"
#include "s_to_uint64.hpp"
#include "progress_tracker.hpp"

void import_tab(hashdb::import_manager_t& manager,
                const std::string& repository_name,
                const std::string& filename,
                const hashdb::scan_manager_t* const whitelist_manager,
                progress_tracker_t& progress_tracker,
                std::istream& in);

#endif

