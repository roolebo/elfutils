#! /bin/sh
# Copyright (C) 2012 Red Hat, Inc.
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

# common.h
#
# #include <stdio.h>
#
# struct foobar
# {
#   int foo;
#   struct foobar *bar;
# };
#
# extern int call_foo(struct foobar *foobar_struct_ptr);

# main.c
#
# #include "common.h"
#
# int main(int argc, char ** argv)
# {
#   struct foobar b;
#   b.foo = 42;
#   b.bar = &b;
#
#   return call_foo(b.bar);
# }

# shared.c
#
# #include "common.h"
#
# int call_foo(struct foobar *fb)
# {
#   return fb->bar->foo - 42;
# }

# gcc -fPIC -g -c -Wall shared.c
# gcc -shared -o libtestfile_multi_shared.so shared.o
# gcc -g -o testfile_multi_main -L. -ltestfile_multi_shared main.c -Wl,-rpath,.
# dwz -m testfile_multi.dwz testfile_multi_main libtestfile_multi_shared.so

testfiles libtestfile_multi_shared.so testfile_multi_main testfile_multi.dwz

testrun_compare ../src/readelf --debug-dump=info testfile_multi_main <<\EOF

DWARF section [28] '.debug_info' at offset 0x1078:
 [Offset]
 Compilation unit at offset 0:
 Version: 4, Abbreviation section offset: 0, Address size: 8, Offset size: 4
 [     b]  compile_unit
           producer             (strp) "GNU C 4.7.0 20120507 (Red Hat 4.7.0-5) -mtune=generic -march=x86-64 -g"
           language             (data1) C89 (1)
           name                 (strp) "main.c"
           comp_dir             (GNU_strp_alt) "/home/mark/src/tests/dwz"
           low_pc               (addr) 0x00000000004006ac <main>
           high_pc              (udata) 44
           stmt_list            (sec_offset) 0
 [    26]    imported_unit
             import               (GNU_ref_alt) [     b]
 [    2b]    pointer_type
             byte_size            (data1) 8
             type                 (GNU_ref_alt) [    53]
 [    31]    subprogram
             external             (flag_present) 
             name                 (strp) "main"
             decl_file            (data1) 1
             decl_line            (data1) 3
             prototyped           (flag_present) 
             type                 (GNU_ref_alt) [    3e]
             low_pc               (addr) 0x00000000004006ac <main>
             high_pc              (udata) 44
             frame_base           (exprloc) 
              [   0] call_frame_cfa
             GNU_all_tail_call_sites (flag_present) 
             sibling              (ref_udata) [    6e]
 [    48]      formal_parameter
               name                 (strp) "argc"
               decl_file            (data1) 1
               decl_line            (data1) 3
               type                 (GNU_ref_alt) [    3e]
               location             (exprloc) 
                [   0] fbreg -36
 [    56]      formal_parameter
               name                 (strp) "argv"
               decl_file            (data1) 1
               decl_line            (data1) 3
               type                 (ref_udata) [    6e]
               location             (exprloc) 
                [   0] fbreg -48
 [    61]      variable
               name                 (string) "b"
               decl_file            (data1) 1
               decl_line            (data1) 5
               type                 (GNU_ref_alt) [    5a]
               location             (exprloc) 
                [   0] fbreg -32
 [    6e]    pointer_type
             byte_size            (data1) 8
             type                 (ref_udata) [    2b]
EOF

testrun_compare ../src/readelf --debug-dump=info libtestfile_multi_shared.so <<\EOF

DWARF section [25] '.debug_info' at offset 0x106c:
 [Offset]
 Compilation unit at offset 0:
 Version: 4, Abbreviation section offset: 0, Address size: 8, Offset size: 4
 [     b]  compile_unit
           producer             (strp) "GNU C 4.7.0 20120507 (Red Hat 4.7.0-5) -fpreprocessed -mtune=generic -march=x86-64 -g -fPIC"
           language             (data1) C89 (1)
           name                 (strp) "shared.c"
           comp_dir             (GNU_strp_alt) "/home/mark/src/tests/dwz"
           low_pc               (addr) +0x0000000000000670 <call_foo>
           high_pc              (udata) 23
           stmt_list            (sec_offset) 0
 [    26]    imported_unit
             import               (GNU_ref_alt) [     b]
 [    2b]    subprogram
             external             (flag_present) 
             name                 (strp) "call_foo"
             decl_file            (data1) 1
             decl_line            (data1) 3
             prototyped           (flag_present) 
             type                 (GNU_ref_alt) [    3e]
             low_pc               (addr) +0x0000000000000670 <call_foo>
             high_pc              (udata) 23
             frame_base           (exprloc) 
              [   0] call_frame_cfa
             GNU_all_call_sites   (flag_present) 
 [    41]      formal_parameter
               name                 (string) "fb"
               decl_file            (data1) 1
               decl_line            (data1) 3
               type                 (GNU_ref_alt) [    76]
               location             (exprloc) 
                [   0] fbreg -24
EOF

exit 0
