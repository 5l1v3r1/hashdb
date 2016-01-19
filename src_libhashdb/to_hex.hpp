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
 * Provide a binary to hexadecimal formatter.
 */

#ifndef TO_HEX_HPP
#define TO_HEX_HPP

#include<string>

static inline uint8_t tohex(uint8_t c) {
  switch(c) {
    case 0 : return '0'; break;
    case 1 : return '1'; break;
    case 2 : return '2'; break;
    case 3 : return '3'; break;
    case 4 : return '4'; break;
    case 5 : return '5'; break;
    case 6 : return '6'; break;
    case 7 : return '7'; break;
    case 8 : return '8'; break;
    case 9 : return '9'; break;
    case 10 : return 'a'; break;
    case 11 : return 'b'; break;
    case 12 : return 'c'; break;
    case 13 : return 'd'; break;
    case 14 : return 'e'; break;
    case 15 : return 'f'; break;
    default:
      std::cerr << "char " << (uint32_t)c << "\n";
      assert(0);
      return 0; // for mingw compiler
  }
}

/**
 * Return the hexadecimal representation of the binary string.
 */
std::string to_hex(const std::string& binary_string) {
  std::stringstream ss;
  for (size_t i=0; i<binary_string.size(); i++) {
    uint8_t c = binary_string.c_str()[i];
    ss << tohex(c>>4) << tohex(c&0x0f);
  }
  return ss.str();
}

#endif

