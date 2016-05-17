#!/usr/bin/env python

# This script tests the Python version and hashdb library functions.
# This script creates filenames starting with "temp_" in the local directory.

# ############################################################
# test Python version
# ############################################################
import sys
# require Python version 2.7
if sys.version_info.major != 2 and sys.version_info.minor != 7:
    print("Error: Invalid Python version: 2.7 is required.")
    print(sys.version_info)
    sys.exit(1)

# require 64-bit Python
if sys.maxsize != 2**63 - 1:
    print("Error: 64-bit Python is required.")
    print("found %d but expected %d" % (sys.maxsize, 2**64))
    sys.exit(1)

import hashdb
import shutil
from struct import pack

# require equality, adapted from ../test_py/helpers.py
def str_equals(a,b):
    if a != b:
        print("a", ":".join("{:02x}".format(ord(c)) for c in a))
        print("b", ":".join("{:02x}".format(ord(c)) for c in b))
        raise ValueError("'%s' not equal to '%s'" % (a,b))
def bool_equals(a,b):
    if a != b:
        raise ValueError(a + " not equal to " + b)
def int_equals(a,b):
    if a != b:
        raise ValueError(str(a) + " not equal to " + str(b))


# ############################################################
# test Support functions
# ############################################################
# Support function: Version
str_equals(hashdb.version()[:2], "3.")

# Support function: Create new temp_1.hdb
shutil.rmtree("temp_1.hdb", True)
cmd="my command string"
settings = hashdb.settings_t()
str_equals(hashdb.create_hashdb("temp_1.hdb", settings, cmd), "")

# Support function: read settings
str_equals(hashdb.read_settings("temp_1.hdb", settings), "")

# Support function: test conversion to binary and back
binary_string = hashdb.hex_to_bin("0000000000000000")
hex_string = hashdb.bin_to_hex(binary_string)
str_equals(hex_string, "0000000000000000")

# ############################################################
# test import functions
# ############################################################
import_manager = hashdb.import_manager_t("temp_1.hdb", "insert test data")
import_manager.insert_source_name("hhhhhhhh", "rn1", "fn1")
import_manager.insert_source_name("hhhhhhhh", "rn2", "fn2")
import_manager.insert_source_data("hhhhhhhh", 100, "ft1", 1)
import_manager.insert_hash("hhhhhhhh","gggggggg", 512, 2, "block label")
import_manager.import_json('{"block_hash":"6868686868686868","entropy":2,"block_label":"block label","source_offset_pairs":["6767676767676767",512]}')
import_manager.import_json('"file_hash":"6767676767676767","filesize":0,"file_type":"","nonprobative_count":0,"name_pairs":[]')
str_equals(import_manager.size(), '{"hash_data_store":1, "hash_store":1, "source_data_store":2, "source_id_store":2, "source_name_store":2}')

# ############################################################
# test scan functions
# ############################################################
scan_manager = hashdb.scan_manager_t("temp_1.hdb")
expanded_hash = scan_manager.find_expanded_hash_json("hhhhhhhh")
str_equals(expanded_hash,
'{"block_hash":"6868686868686868","entropy":2,"block_label":"block label","source_list_id":3724381083,"sources":[{"file_hash":"6767676767676767","filesize":0,"file_type":"","nonprobative_count":0,"name_pairs":[]}],"source_offset_pairs":["6767676767676767",512]}'
)

json_hash_string = scan_manager.export_hash_json("hhhhhhhh")
str_equals(json_hash_string, '{"block_hash":"6868686868686868","entropy":2,"block_label":"block label","source_offset_pairs":["6767676767676767",512]}')

int_equals(scan_manager.find_hash_count("hhhhhhhh"), 1)

str_equals(scan_manager.find_hash_count_json("hhhhhhhh"), '{"block_hash":"6868686868686868","count":1}')

int_equals(scan_manager.find_approximate_hash_count("hhhhhhhh"), 1)

str_equals(scan_manager.find_approximate_hash_count_json("hhhhhhhh"), '{"block_hash":"6868686868686868","approximate_count":1}')

has_source_data, filesize, file_type, nonprobative_count = scan_manager.find_source_data("hhhhhhhh")
bool_equals(has_source_data, True)
int_equals(filesize, 100)
str_equals(file_type, "ft1")
int_equals(nonprobative_count, 1)

str_equals(scan_manager.export_source_json("gggggggg"), '{"file_hash":"6767676767676767","filesize":0,"file_type":"","nonprobative_count":0,"name_pairs":[]}')

first_binary_hash = scan_manager.first_hash()
str_equals(hashdb.bin_to_hex(first_binary_hash), "6868686868686868")

next_binary_hash = scan_manager.next_hash(first_binary_hash)
str_equals(hashdb.bin_to_hex(next_binary_hash), "")

## next after end is invalid and currently calls assert which is extreme.
#next_binary_hash = scan_manager.next_hash(next_binary_hash)

first_binary_source = scan_manager.first_source()
str_equals(hashdb.bin_to_hex(first_binary_source), "6767676767676767")

next_binary_source = scan_manager.next_source(first_binary_source)
str_equals(hashdb.bin_to_hex(next_binary_source), "6868686868686868")

next_binary_source = scan_manager.next_source(next_binary_source)
str_equals(hashdb.bin_to_hex(next_binary_source), "")

str_equals(scan_manager.size(), '{"hash_data_store":1, "hash_store":1, "source_data_store":2, "source_id_store":2, "source_name_store":2}')
int_equals(scan_manager.size_hashes(), 1)
int_equals(scan_manager.size_sources(), 2)

def temp_out_equals(a):
    infile = open("temp_out", "r")
    str_equals(infile.read().strip(), a)
    infile.close()


# Settings
settings.byte_alignment = 1
settings.block_size = 2
settings.max_source_offset_pairs = 3
settings.hash_prefix_bits = 4
settings.hash_suffix_bytes = 5
str_equals(settings.settings_string(), '{"settings_version":3, "byte_alignment":1, "block_size":2, "max_source_offset_pairs":3, "hash_prefix_bits":4, "hash_suffix_bytes":5}')

# Timestamp
ts = hashdb.timestamp_t()
str_equals(ts.stamp("time1")[:15], '{"name":"time1"')
str_equals(ts.stamp("time2")[:15], '{"name":"time2"')

# ############################################################
# test scan_stream in binary mode
# ############################################################
# setup for scan_stream, end with EOF
temp_in = open("temp_in.bin", "w")
in_bytes = pack('8sQ', 'aaaaaaaa', 1)
temp_in.write(in_bytes)
in_bytes = pack('8sQ', 'hhhhhhhh', 1)
temp_in.write(in_bytes)
temp_in.close()

# scan_stream EXPANDED_HASH
in_file_object = open("temp_in.bin", "r")
in_fd = in_file_object.fileno()
out_file_object = open("temp_out", "w")
out_fd = out_file_object.fileno()
status = scan_manager.scan_stream(in_fd, out_fd, 8, 8, hashdb.EXPANDED_HASH,
                                  hashdb.TEXT_OUTPUT)
str_equals(status, "")
temp_out_equals('0100000000000000{"block_hash":"6868686868686868"}')
in_file_object.close()
out_file_object.close()

# scan_stream HASH_COUNT
in_file_object = open("temp_in.bin", "r")
in_fd = in_file_object.fileno()
out_file_object = open("temp_out", "w")
out_fd = out_file_object.fileno()
status = scan_manager.scan_stream(in_fd, out_fd, 8, 8, hashdb.HASH_COUNT,
                                  hashdb.TEXT_OUTPUT)
str_equals(status, "")
temp_out_equals('0100000000000000{"block_hash":"6868686868686868","count":1}')
in_file_object.close()
out_file_object.close()

# scan_stream EXPANDED_HASH_COUNT
in_file_object = open("temp_in.bin", "r")
in_fd = in_file_object.fileno()
out_file_object = open("temp_out", "w")
out_fd = out_file_object.fileno()
status = scan_manager.scan_stream(in_fd, out_fd, 8, 8,
                                  hashdb.APPROXIMATE_HASH_COUNT,
                                  hashdb.TEXT_OUTPUT)
str_equals(status, "")
temp_out_equals('0100000000000000{"block_hash":"6868686868686868","approximate_count":1}')
in_file_object.close()
out_file_object.close()

# setup for scan_stream, end with 0x00...
temp_in = open("temp_in.bin", "w")
in_bytes = pack('8sQ', '\0\0\0\0\0\0\0\0', 1)
temp_in.write(in_bytes)
in_bytes = pack('8sQ', 'hhhhhhhh', 1)
temp_in.write(in_bytes)
temp_in.close()

# scan_stream ends early with 0x00...
in_file_object = open("temp_in.bin", "r")
in_fd = in_file_object.fileno()
out_file_object = open("temp_out", "w")
out_fd = out_file_object.fileno()
status = scan_manager.scan_stream(in_fd, out_fd, 8, 8, hashdb.EXPANDED_HASH,
                                  hashdb.TEXT_OUTPUT)
str_equals(status, "")
temp_out_equals("")
in_file_object.close()
out_file_object.close()

# setup for scan_stream, end with invalid alignment of one extra byte
temp_in = open("temp_in.bin", "w")
in_bytes = pack('8sQ', 'aaaaaaaa', 1)
temp_in.write(in_bytes)
in_bytes = pack('8sQ', 'hhhhhhhh', 1)
temp_in.write(in_bytes)
temp_in.write('\0')
temp_in.close()

# scan_stream ends with invalid alignment
in_file_object = open("temp_in.bin", "r")
in_fd = in_file_object.fileno()
out_file_object = open("temp_out", "w")
out_fd = out_file_object.fileno()
status = scan_manager.scan_stream(in_fd, out_fd, 8, 8, hashdb.EXPANDED_HASH,
                                  hashdb.TEXT_OUTPUT)
str_equals(status, "unexpected input size 1 is not 16 in scan stream")
temp_out_equals('0100000000000000{"block_hash":"6868686868686868"}')
in_file_object.close()
out_file_object.close()

# ############################################################
# test scan_stream in text mode
# ############################################################
# setup for scan_stream, end with EOF
temp_in = open("temp_in.bin", "w")
in_bytes = pack('8sQ', 'aaaaaaaa', 1)
temp_in.write(in_bytes)
in_bytes = pack('8sQ', 'hhhhhhhh', 1)
temp_in.write(in_bytes)
temp_in.close()

# scan_stream EXPANDED_HASH
in_file_object = open("temp_in.bin", "r")
in_fd = in_file_object.fileno()
out_file_object = open("temp_out", "w")
out_fd = out_file_object.fileno()
status = scan_manager.scan_stream(in_fd, out_fd, 8, 8, hashdb.EXPANDED_HASH,
                                  hashdb.BINARY_OUTPUT)
str_equals(status, "")
# 0000000: 3100 0000 0000 0000 6868 6868 6868 6868  .......1hhhhhhhh
# 0000010: 0100 0000 0000 0000 7b22 626c 6f63 6b5f  ........{"block_
# 0000020: 6861 7368 223a 2236 3836 3836 3836 3836  hash":"686868686
# 0000030: 3836 3836 3836 3822 7d                   8686868"}
temp_out_equals(
     '\x31\x00\x00\x00\x00\x00\x00\x00\x68\x68\x68\x68\x68\x68\x68\x68'
     '\x01\x00\x00\x00\x00\x00\x00\x00\x7b\x22\x62\x6c\x6f\x63\x6b\x5f'
     '\x68\x61\x73\x68\x22\x3a\x22\x36\x38\x36\x38\x36\x38\x36\x38\x36'
     '\x38\x36\x38\x36\x38\x36\x38\x22\x7d'
)
in_file_object.close()
out_file_object.close()

