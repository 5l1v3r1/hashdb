#!/usr/bin/env python3
#
# Test database maniplation commands

import shutil
import helpers as H

json_db1 = [
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_sub_counts":["1111111111111111",1]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"","source_sub_counts":["0000000000000000",1,"0011223344556677",2]}',
'{"block_hash":"ffffffffffffffff","k_entropy":0,"block_label":"","source_sub_counts":["0011223344556677",1]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab","repository2", "second_temp_1.tab"]}'
]

json_out1 = [
'# command: ', '# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_sub_counts":["1111111111111111",1]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"","source_sub_counts":["0000000000000000",1,"0011223344556677",2]}',
'{"block_hash":"ffffffffffffffff","k_entropy":0,"block_label":"","source_sub_counts":["0011223344556677",1]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab","repository2","second_temp_1.tab"]}'
]

json_set_db1 = [
\
'{"block_hash":"1111111111111111","k_entropy":1,"block_label":"bl1","source_sub_counts":["11",1]}',
'{"block_hash":"2222222222222222","k_entropy":2,"block_label":"bl2","source_sub_counts":["11",1,"22",2]}',
'{"file_hash":"11","filesize":1,"file_type":"A","zero_count":11,"nonprobative_count":1,"name_pairs":["r1","f1"]}',
'{"file_hash":"22","filesize":2,"file_type":"B","zero_count":12,"nonprobative_count":2,"name_pairs":["r1","f1"]}'
]

json_set_db2 = [
\
'{"block_hash":"2222222222222222","k_entropy":2,"block_label":"bl2","source_sub_counts":["22",2,"33",1]}',
'{"block_hash":"3333333333333333","k_entropy":3,"block_label":"bl3","source_sub_counts":["33",1]}',
'{"file_hash":"22","filesize":2,"file_type":"B","zero_count":12,"nonprobative_count":2,"name_pairs":["r2","f2"]}',
'{"file_hash":"33","filesize":3,"file_type":"C","zero_count":13,"nonprobative_count":3,"name_pairs":["r2","f2"]}'
]

def test_add():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_out1)
    H.rm_tempdir("temp_2.hdb")

    # add to new temp_2.hdb
    H.hashdb(["add", "temp_1.hdb", "temp_2.hdb"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, json_out1)

    # add to existing temp_2.hdb
    H.hashdb(["add", "temp_1.hdb", "temp_2.hdb"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, json_out1)

def test_add_multiple():
    json_db1 = [
'{"file_hash":"11","filesize":1,"file_type":"ft1","zero_count":15,"nonprobative_count":111,"name_pairs":["rn1","fn1"]}',
'{"block_hash":"11111111","k_entropy":101,"block_label":"bl1","source_sub_counts":["11",1]}']
    json_db2 = [
'{"file_hash":"22","filesize":2,"file_type":"ft2","zero_count":16,"nonprobative_count":222,"name_pairs":["rn2","fn2"]}',
'{"block_hash":"22222222","k_entropy":202,"block_label":"bl2","source_sub_counts":["22",1]}']
    json3_db3 = [
'# command: ','# hashdb-Version: ',
'{"block_hash":"11111111","k_entropy":101,"block_label":"bl1","source_sub_counts":["11",1]}',
'{"block_hash":"22222222","k_entropy":202,"block_label":"bl2","source_sub_counts":["22",1]}',
'{"file_hash":"11","filesize":1,"file_type":"ft1","zero_count":15,"nonprobative_count":111,"name_pairs":["rn1","fn1"]}',
'{"file_hash":"22","filesize":2,"file_type":"ft2","zero_count":16,"nonprobative_count":222,"name_pairs":["rn2","fn2"]}'
]



    # create DBs
    H.make_hashdb("temp_1.hdb", json_db1)
    H.make_hashdb("temp_2.hdb", json_db2)
    H.rm_tempdir("temp_3.hdb")

    # add 1 and 2 into 3
    H.hashdb(["add_multiple", "temp_1.hdb", "temp_2.hdb", "temp_3.hdb"])

    # check temp_3.hdb
    H.hashdb(["export", "temp_3.hdb", "temp_3.json"])
    json_in3 = H.read_file("temp_3.json")
    H.lines_equals(json_in3, json3_db3)

def test_add_repository():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_out1)
    H.rm_tempdir("temp_2.hdb")

    # add to new temp_2.hdb
    H.hashdb(["add_repository", "temp_1.hdb", "temp_2.hdb", "repository1"])

    # temp_2.hdb should only have hashes and sources with repository1
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_sub_counts":["1111111111111111",1]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"","source_sub_counts":["0000000000000000",1,"0011223344556677",2]}',
'{"block_hash":"ffffffffffffffff","k_entropy":0,"block_label":"","source_sub_counts":["0011223344556677",1]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}'
])

    # add to new temp_2.hdb
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["add_repository", "temp_1.hdb", "temp_2.hdb", "repository2"])

    # temp_2.hdb should only have hashes and sources with repository2
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_sub_counts":["1111111111111111",1]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository2","second_temp_1.tab"]}'
])

def test_add_range():
    colon_one = [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_sub_counts":["1111111111111111",1]}',
'{"block_hash":"ffffffffffffffff","k_entropy":0,"block_label":"","source_sub_counts":["0011223344556677",1]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab","repository2","second_temp_1.tab"]}'
]
    two_colon_two = [
'# command: ',
'# hashdb-Version: '
]
    two_colon = [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"","source_sub_counts":["0000000000000000",1,"0011223344556677",2]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}'
]

    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_out1)

    # add_range to new temp_2.hdb using ":1"
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["add_range", "temp_1.hdb", "temp_2.hdb", ":1"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, colon_one)

    # add_range to new temp_2.hdb using "0:1"
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["add_range", "temp_1.hdb", "temp_2.hdb", "0:1"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, colon_one)

    # add_range to new temp_2.hdb using "1:1"
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["add_range", "temp_1.hdb", "temp_2.hdb", "1:1"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, colon_one)

    # add_range to new temp_2.hdb using "2:"
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["add_range", "temp_1.hdb", "temp_2.hdb", "2:"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, two_colon)

    # add_range to new temp_2.hdb using "2:2"
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["add_range", "temp_1.hdb", "temp_2.hdb", "2:2"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, two_colon_two)

    # add_range to new temp_2.hdb using "3:3"
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["add_range", "temp_1.hdb", "temp_2.hdb", "3:3"])

    # temp_2.hdb should match
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, two_colon)

def test_intersect():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_set_db1)
    H.make_hashdb("temp_2.hdb", json_set_db2)
    H.rm_tempdir("temp_3.hdb")

    # intersect
    H.hashdb(["intersect", "temp_1.hdb", "temp_2.hdb", "temp_3.hdb"])
    H.hashdb(["export", "temp_3.hdb", "temp_3.json"])
    json3 = H.read_file("temp_3.json")
    H.lines_equals(json3, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":2,"block_label":"bl2","source_sub_counts":["22",2]}',
'{"file_hash":"22","filesize":2,"file_type":"B","zero_count":12,"nonprobative_count":2,"name_pairs":["r1","f1","r2","f2"]}'
])

def test_intersect_hash():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_set_db1)
    H.make_hashdb("temp_2.hdb", json_set_db2)
    H.rm_tempdir("temp_3.hdb")

    # intersect_hash
    H.hashdb(["intersect_hash", "temp_1.hdb", "temp_2.hdb", "temp_3.hdb"])
    H.hashdb(["export", "temp_3.hdb", "temp_3.json"])
    json3 = H.read_file("temp_3.json")
    H.lines_equals(json3, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":2,"block_label":"bl2","source_sub_counts":["11",1,"22",2,"33",1]}',
'{"file_hash":"11","filesize":1,"file_type":"A","zero_count":11,"nonprobative_count":1,"name_pairs":["r1","f1"]}',
'{"file_hash":"22","filesize":2,"file_type":"B","zero_count":12,"nonprobative_count":2,"name_pairs":["r1","f1","r2","f2"]}',
'{"file_hash":"33","filesize":3,"file_type":"C","zero_count":13,"nonprobative_count":3,"name_pairs":["r2","f2"]}'
])

def test_subtract():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_set_db1)
    H.make_hashdb("temp_2.hdb", json_set_db2)
    H.rm_tempdir("temp_3.hdb")

    # db1 - db2
    H.hashdb(["subtract", "temp_1.hdb", "temp_2.hdb", "temp_3.hdb"])
    H.hashdb(["export", "temp_3.hdb", "temp_3.json"])
    json3 = H.read_file("temp_3.json")
    H.lines_equals(json3, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"1111111111111111","k_entropy":1,"block_label":"bl1","source_sub_counts":["11",1]}',
'{"block_hash":"2222222222222222","k_entropy":2,"block_label":"bl2","source_sub_counts":["11",1]}',
'{"file_hash":"11","filesize":1,"file_type":"A","zero_count":11,"nonprobative_count":1,"name_pairs":["r1","f1"]}'
])

    # db2 - db1
    H.rm_tempdir("temp_3.hdb")
    H.hashdb(["subtract", "temp_2.hdb", "temp_1.hdb", "temp_3.hdb"])
    H.hashdb(["export", "temp_3.hdb", "temp_3.json"])
    json3 = H.read_file("temp_3.json")
    H.lines_equals(json3, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":2,"block_label":"bl2","source_sub_counts":["33",1]}',
'{"block_hash":"3333333333333333","k_entropy":3,"block_label":"bl3","source_sub_counts":["33",1]}',
'{"file_hash":"33","filesize":3,"file_type":"C","zero_count":13,"nonprobative_count":3,"name_pairs":["r2","f2"]}'
])

def test_subtract_hash():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_set_db1)
    H.make_hashdb("temp_2.hdb", json_set_db2)
    H.rm_tempdir("temp_3.hdb")

    # db1 - db2 hash
    H.hashdb(["subtract_hash", "temp_1.hdb", "temp_2.hdb", "temp_3.hdb"])
    H.hashdb(["export", "temp_3.hdb", "temp_3.json"])
    json3 = H.read_file("temp_3.json")
    H.lines_equals(json3, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"1111111111111111","k_entropy":1,"block_label":"bl1","source_sub_counts":["11",1]}',
'{"file_hash":"11","filesize":1,"file_type":"A","zero_count":11,"nonprobative_count":1,"name_pairs":["r1","f1"]}'
])

    # db2 - db1 hash
    H.rm_tempdir("temp_3.hdb")
    H.hashdb(["subtract_hash", "temp_2.hdb", "temp_1.hdb", "temp_3.hdb"])
    H.hashdb(["export", "temp_3.hdb", "temp_3.json"])
    json3 = H.read_file("temp_3.json")
    H.lines_equals(json3, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"3333333333333333","k_entropy":3,"block_label":"bl3","source_sub_counts":["33",1]}',
'{"file_hash":"33","filesize":3,"file_type":"C","zero_count":13,"nonprobative_count":3,"name_pairs":["r2","f2"]}'
])

def test_subtract_repository():
    # create new hashdb
    H.make_hashdb("temp_1.hdb", json_out1)
    H.rm_tempdir("temp_2.hdb")

    # add to new temp_2.hdb
    H.hashdb(["subtract_repository", "temp_1.hdb", "temp_2.hdb", "repository1"])

    # temp_2.hdb should only have hashes and sources with repository2
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_sub_counts":["1111111111111111",1]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository2","second_temp_1.tab"]}'
])

    # add to new temp_2.hdb
    H.rm_tempdir("temp_2.hdb")
    H.hashdb(["subtract_repository", "temp_1.hdb", "temp_2.hdb", "repository2"])

    # temp_2.hdb should only have hashes and sources with repository1
    H.hashdb(["export", "temp_2.hdb", "temp_2.json"])
    json2 = H.read_file("temp_2.json")
    H.lines_equals(json2, [
'# command: ',
'# hashdb-Version: ',
'{"block_hash":"2222222222222222","k_entropy":0,"block_label":"","source_sub_counts":["1111111111111111",1]}',
'{"block_hash":"8899aabbccddeeff","k_entropy":0,"block_label":"","source_sub_counts":["0000000000000000",1,"0011223344556677",2]}',
'{"block_hash":"ffffffffffffffff","k_entropy":0,"block_label":"","source_sub_counts":["0011223344556677",1]}',
'{"file_hash":"0000000000000000","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"0011223344556677","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}',
'{"file_hash":"1111111111111111","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":["repository1","temp_1.tab"]}'
])

if __name__=="__main__":
    test_add()
    test_add_multiple()
    test_add_repository()
    test_add_range()
    test_intersect()
    test_intersect_hash()
    test_subtract()
    test_subtract_hash()
    test_subtract_repository()
    print("Test Done.")

