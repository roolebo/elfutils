#! /bin/sh
# Copyright (C) 2013 Red Hat, Inc.
# This file is part of elfutils.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. $srcdir/test-subr.sh

# See the source files testfile_const_type.c testfile_implicit_value.c
# testfile_entry_value.c testfile_parameter_ref.c testfile_implicit_pointer.c
# how to regenerate the test files (needs GCC 4.8+).

testfiles testfile_const_type testfile_implicit_value testfile_entry_value
testfiles testfile_parameter_ref testfile_implicit_pointer

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_const_type <<\EOF
module 'testfile_const_type'
[b] CU 'const_type.c'@0
  [33] function 'f1'@80483f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
    [4b] parameter 'd'
      [80483f0,804841b) {fbreg(0)}
    [57] variable 'w'
      [80483f0,804841b) {fbreg(0), GNU_deref_type(8){long long int,signed,64@[25]}, GNU_const_type{long long int,signed,64@[25]}(8)[0000806745230100], div, GNU_convert{long long unsigned int,unsigned,64@[2c]}, stack_value}
  [7d] function 'main'@80482f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_implicit_value <<\EOF
module 'testfile_implicit_value'
[b] CU 'implicit_value.c'@0
  [25] function 'foo'@80483f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
    [3e] variable 'a'
      [80483f0,80483f6) {implicit_value(8){0200000000000000}, piece(8), implicit_value(8){1500000000000000}, piece(8)}
  [86] function 'main'@80482f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_entry_value <<\EOF
module 'testfile_entry_value'
[b] CU 'entry_value.c'@0
  [29] function 'foo'@400500
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [4a] parameter 'x'
      [400500,400504) {reg5}
    [55] parameter 'y'
      [400500,400504) {reg4}
  [68] function 'bar'@400510
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [89] parameter 'x'
      [400510,40051c) {reg5}
      [40051c,40052b) {reg6}
      [40052b,400531) {GNU_entry_value(1) {reg5}, stack_value}
    [96] parameter 'y'
      [400510,40051c) {reg4}
      [40051c,40052a) {reg3}
      [40052a,400531) {GNU_entry_value(1) {reg4}, stack_value}
    [a3] variable 'z'
      [400524,400528) {reg0}
      [400528,400529) {reg12}
      [400529,40052e) {breg0(0), breg12(0), plus, stack_value}
      [40052e,400531) {reg0}
  [e9] function 'main'@400400
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [10a] parameter 'argc'
      [400400,400406) {reg5}
      [400406,40040a) {breg5(-1), stack_value}
      [40040a,40040b) {GNU_entry_value(1) {reg5}, stack_value}
    [119] parameter 'argv'
      [400400,400403) {reg4}
      [400403,40040b) {GNU_entry_value(1) {reg4}, stack_value}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_parameter_ref <<\EOF
module 'testfile_parameter_ref'
[b] CU 'parameter_ref.c'@0
  [77] function 'foo'@400510
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [92] parameter 'x'
      [400510,400523) {reg5}
    [99] parameter 'y'
      [400510,400523) {GNU_parameter_ref[42], stack_value}
    [a5] variable 'a'
      [400510,400523) {breg5(0), lit1, shl, stack_value}
    [b0] variable 'b'
      [400510,400523) {GNU_parameter_ref[42], lit1, shl, stack_value}
    [be] variable 'c'
      <constant value>
    [c4] parameter 'z'
      <constant value>
  [cb] function 'main'@400400
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [ec] parameter 'x'
      [400400,400408) {reg5}
      [400408,400421) {reg3}
      [400421,400423) {GNU_entry_value(1) {reg5}, stack_value}
    [f9] parameter 'argv'
      [400400,400408) {reg4}
      [400408,400423) {GNU_entry_value(1) {reg4}, stack_value}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_implicit_pointer <<\EOF
module 'testfile_implicit_pointer'
[b] CU 'implicit_pointer.c'@0
  [29] function 'foo'@400500
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [4a] parameter 'i'
      [400500,400503) {reg5}
    [55] variable 'p'
      [400500,400503) {GNU_implicit_pointer([4a],0) {reg5}}
  [73] function 'main'@400400
    frame_base: {call_frame_cfa {bregx(7,8)}}
EOF

# Multi CU DWARF5. See run-dwarf-ranges.sh.
testfiles testfileranges5.debug
testrun_compare ${abs_top_builddir}/tests/varlocs --debug -e testfileranges5.debug <<\EOF
module 'testfileranges5.debug'
[c] CU 'hello.c'@0
  [2a] function 'no_say'@401160
    frame_base: {call_frame_cfa {...}}
    [4a] parameter 'prefix'
      [401160,401169) {reg5}
      [401169,40116a) {entry_value(1) {reg5}, stack_value}
      [40116a,401175) {reg5}
      [401175,40117a) {entry_value(1) {reg5}, stack_value}
    [59] variable 'world'
      [401160,40117a) {addr(0x402004), stack_value}
  [bd] function 'main'@401050
    frame_base: {call_frame_cfa {...}}
    [dd] parameter 'argc'
      [401050,401062) {reg5}
      [401062,401067) {entry_value(1) {reg5}, stack_value}
    [ec] parameter 'argv'
      [401050,401066) {reg4}
      [401066,401067) {entry_value(1) {reg4}, stack_value}
  [fb] inlined function 'subject'@401053
    [117] parameter 'count'
      [401053,40105f) {reg5}
    [120] parameter 'word'
      [401053,40105f) {reg0}
  [168] function 'subject'@401150
    frame_base: {call_frame_cfa {...}}
    [183] parameter 'word'
      [401150,401160) {reg5}
    [18a] parameter 'count'
      [401150,401160) {reg4}
module 'testfileranges5.debug'
[1ab] CU 'world.c'@401180
  [1cd] function 'no_main'@4011d0
    frame_base: {call_frame_cfa {...}}
    [1ef] parameter 'argc'
      [4011d0,4011e2) {reg5}
      [4011e2,4011e7) {entry_value(1) {reg5}, stack_value}
    [1fe] parameter 'argv'
      [4011d0,4011e6) {reg4}
      [4011e6,4011e7) {entry_value(1) {reg4}, stack_value}
  [20d] inlined function 'no_subject'@4011d3
    [229] parameter 'count'
      [4011d3,4011df) {reg5}
    [232] parameter 'word'
      [4011d3,4011df) {reg0}
  [28d] function 'say'@401180
    frame_base: {call_frame_cfa {...}}
    [2af] parameter 'prefix'
      [401180,40118e) {reg5}
      [40118e,40119c) {reg3}
      [40119c,4011a7) {entry_value(1) {reg5}, stack_value}
      [4011a7,4011b5) {reg3}
      [4011b5,4011c0) {entry_value(1) {reg5}, stack_value}
    [2be] variable 'world'
      [401193,40119b) {reg0}
      [4011a7,4011b4) {reg0}
  [2ce] inlined function 'happy'@40119b
    [2e6] parameter 'w'
      [4011a7,4011b4) {reg0}
  [2ef] inlined function 'sad'@40119b
    [303] parameter 'c'
      [40119b,4011a6) {reg0}
      [4011a6,4011a7) {entry_value(1) {reg5}}
      [4011b4,4011bf) {reg0}
  [36b] function 'no_subject'@4011c0
    frame_base: {call_frame_cfa {...}}
    [386] parameter 'word'
      [4011c0,4011d0) {reg5}
    [38d] parameter 'count'
      [4011c0,4011d0) {reg4}
EOF

# Multi CU Split DWARF5. See run-dwarf-ranges.sh.
# Note that the DIE numbers change, but the actual location addresses are
# the same as above, even though the representation is totally different.
testfiles testfilesplitranges5.debug
testfiles testfile-ranges-hello5.dwo testfile-ranges-world5.dwo
testrun_compare ${abs_top_builddir}/tests/varlocs --debug -e testfilesplitranges5.debug <<\EOF
module 'testfilesplitranges5.debug'
[14] CU 'hello.c'
  [1d] function 'no_say'@401160
    frame_base: {call_frame_cfa {...}}
    [33] parameter 'prefix'
      [401160,401169) {reg5}
      [401169,40116a) {entry_value(1) {reg5}, stack_value}
      [40116a,401175) {reg5}
      [401175,40117a) {entry_value(1) {reg5}, stack_value}
    [3c] variable 'world'
      [401160,40117a) {addr: 0x402004, stack_value}
  [7e] function 'main'@401050
    frame_base: {call_frame_cfa {...}}
    [94] parameter 'argc'
      [401050,401062) {reg5}
      [401062,401067) {entry_value(1) {reg5}, stack_value}
    [9d] parameter 'argv'
      [401050,401066) {reg4}
      [401066,401067) {entry_value(1) {reg4}, stack_value}
  [a6] inlined function 'subject'@401053
    [bb] parameter 'count'
      [401053,40105f) {reg5}
    [c1] parameter 'word'
      [401053,40105f) {reg0}
  [f6] function 'subject'@401150
    frame_base: {call_frame_cfa {...}}
    [10a] parameter 'word'
      [401150,401160) {reg5}
    [111] parameter 'count'
      [401150,401160) {reg4}
module 'testfilesplitranges5.debug'
[14] CU 'world.c'
  [1d] function 'no_main'@4011d0
    frame_base: {call_frame_cfa {...}}
    [35] parameter 'argc'
      [4011d0,4011e2) {reg5}
      [4011e2,4011e7) {entry_value(1) {reg5}, stack_value}
    [3e] parameter 'argv'
      [4011d0,4011e6) {reg4}
      [4011e6,4011e7) {entry_value(1) {reg4}, stack_value}
  [47] inlined function 'no_subject'@4011d3
    [5c] parameter 'count'
      [4011d3,4011df) {reg5}
    [62] parameter 'word'
      [4011d3,4011df) {reg0}
  [a7] function 'say'@401180
    frame_base: {call_frame_cfa {...}}
    [c2] parameter 'prefix'
      [401180,40118e) {reg5}
      [40118e,40119c) {reg3}
      [40119c,4011a7) {entry_value(1) {reg5}, stack_value}
      [4011a7,4011b5) {reg3}
      [4011b5,4011c0) {entry_value(1) {reg5}, stack_value}
    [cb] variable 'world'
      [401193,40119b) {reg0}
      [4011a7,4011b4) {reg0}
  [d5] inlined function 'happy'@40119b
    [e3] parameter 'w'
      [4011a7,4011b4) {reg0}
  [e9] inlined function 'sad'@40119b
    [f3] parameter 'c'
      [40119b,4011a6) {reg0}
      [4011a6,4011a7) {entry_value(1) {reg5}}
      [4011b4,4011bf) {reg0}
  [147] function 'no_subject'@4011c0
    frame_base: {call_frame_cfa {...}}
    [15b] parameter 'word'
      [4011c0,4011d0) {reg5}
    [162] parameter 'count'
      [4011c0,4011d0) {reg4}
EOF

# DW_OP_addrx and DW_OP_constx testcases.
#
# int i, j, k;
# __thread int l, m, n;
#
# int main ()
# {
#   int r1 = i + j + k;
#   int r2 = l + m + n;
#   int res = r1 + r2;
#
#   return res;
# }
#
# gcc -O2 -gdwarf-5 -gsplit-dwarf -o addrx_constx-5.o -c addrx_constx.c
# gcc -O2 -gdwarf-5 -gsplit-dwarf -o testfile-addrx_constx-5 addrx_constx-5.o
# gcc -O2 -gdwarf-4 -gsplit-dwarf -o addrx_constx-4.o -c addrx_constx.c
# gcc -O2 -gdwarf-4 -gsplit-dwarf -o testfile-addrx_constx-4 addrx_constx-4.o

testfiles testfile-addrx_constx-5 addrx_constx-5.dwo
testrun_compare ${abs_top_builddir}/tests/varlocs --exprlocs -e testfile-addrx_constx-5 <<\EOF
module 'testfile-addrx_constx-5'
[14] CU 'addrx_constx.c'
  producer (strx)
  language (data1)
  name (strx)
  comp_dir (strx)
  [19] variable "i"
    name (string)
    decl_file (implicit_const)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {addr: 0x404038}
  [25] base_type "int"
    byte_size (data1)
    encoding (data1)
    name (string)
  [2c] variable "j"
    name (string)
    decl_file (implicit_const)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {addr: 0x404034}
  [38] variable "k"
    name (string)
    decl_file (implicit_const)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {addr: 0x40403c}
  [44] variable "l"
    name (string)
    decl_file (implicit_const)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {const: 0x403e10, form_tls_address}
  [51] variable "m"
    name (string)
    decl_file (implicit_const)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {const: 0x403e0c, form_tls_address}
  [5e] variable "n"
    name (string)
    decl_file (implicit_const)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {const: 0x403e08, form_tls_address}
  [6b] subprogram "main"
    external (flag_present)
    name (strx)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    low_pc (addrx)
    high_pc (data8)
    frame_base (exprloc) {call_frame_cfa {bregx(7,8)}}
    call_all_calls (flag_present)
    [7f] variable "r1"
      name (string)
      decl_file (implicit_const)
      decl_line (data1)
      decl_column (implicit_const)
      type (ref4)
      location (exprloc) {addr: 0x404038, deref_size(4), addr: 0x404034, deref_size(4), plus, addr: 0x40403c, deref_size(4), plus, stack_value}
    [98] variable "r2"
      name (string)
      decl_file (implicit_const)
      decl_line (data1)
      decl_column (implicit_const)
      type (ref4)
      location (exprloc) {form_tls_address, const: 0x403e10, deref_size(4), form_tls_address, const: 0x403e0c, deref_size(4), plus, form_tls_address, const: 0x403e08, deref_size(4), plus, stack_value}
    [b4] variable "res"
      name (string)
      decl_file (implicit_const)
      decl_line (data1)
      decl_column (implicit_const)
      type (ref4)
      location (exprloc) {addr: 0x404038, deref_size(4), form_tls_address, const: 0x403e08, deref_size(4), plus, form_tls_address, const: 0x403e0c, deref_size(4), plus, form_tls_address, const: 0x403e10, deref_size(4), plus, addr: 0x404034, deref_size(4), plus, addr: 0x40403c, deref_size(4), plus, stack_value}
EOF

testfiles testfile-addrx_constx-4 addrx_constx-4.dwo
testrun_compare ${abs_top_builddir}/tests/varlocs --exprlocs -e testfile-addrx_constx-4 <<\EOF
module 'testfile-addrx_constx-4'
[b] CU 'addrx_constx.c'
  producer (GNU_str_index)
  language (data1)
  name (GNU_str_index)
  comp_dir (GNU_str_index)
  GNU_dwo_id (data8)
  [18] variable "i"
    name (string)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {addr: 0x404038}
  [25] base_type "int"
    byte_size (data1)
    encoding (data1)
    name (string)
  [2c] variable "j"
    name (string)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {addr: 0x404034}
  [39] variable "k"
    name (string)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {addr: 0x40403c}
  [46] variable "l"
    name (string)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {const: 0x403e10, GNU_push_tls_address}
  [54] variable "m"
    name (string)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {const: 0x403e0c, GNU_push_tls_address}
  [62] variable "n"
    name (string)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    external (flag_present)
    location (exprloc) {const: 0x403e08, GNU_push_tls_address}
  [70] subprogram "main"
    external (flag_present)
    name (GNU_str_index)
    decl_file (data1)
    decl_line (data1)
    decl_column (data1)
    type (ref4)
    low_pc (GNU_addr_index)
    high_pc (data8)
    frame_base (exprloc) {call_frame_cfa {bregx(7,8)}}
    GNU_all_call_sites (flag_present)
    [84] variable "r1"
      name (string)
      decl_file (data1)
      decl_line (data1)
      decl_column (data1)
      type (ref4)
      location (exprloc) {addr: 0x404038, deref_size(4), addr: 0x404034, deref_size(4), plus, addr: 0x40403c, deref_size(4), plus, stack_value}
    [9f] variable "r2"
      name (string)
      decl_file (data1)
      decl_line (data1)
      decl_column (data1)
      type (ref4)
      location (exprloc) {GNU_push_tls_address, const: 0x403e10, deref_size(4), GNU_push_tls_address, const: 0x403e0c, deref_size(4), plus, GNU_push_tls_address, const: 0x403e08, deref_size(4), plus, stack_value}
    [bd] variable "res"
      name (string)
      decl_file (data1)
      decl_line (data1)
      decl_column (data1)
      type (ref4)
      location (exprloc) {addr: 0x404038, deref_size(4), GNU_push_tls_address, const: 0x403e08, deref_size(4), plus, GNU_push_tls_address, const: 0x403e0c, deref_size(4), plus, GNU_push_tls_address, const: 0x403e10, deref_size(4), plus, addr: 0x404034, deref_size(4), plus, addr: 0x40403c, deref_size(4), plus, stack_value}
EOF

exit 0
