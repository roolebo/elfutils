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

# https://bugzilla.redhat.com/show_bug.cgi?id=805447
# eu-unstrip emits garbage for librt.so.1
#
# #include <stdio.h>
# #include <sys/mman.h>
# 
# int main(int argc, char **argv)
# {
#  // Yes, this crashes... That is the point.
#  return shm_open(argv[1], 0, 0);
# }
# 
# gcc -m32 -o rt_crash -lrt rt_crash.c

testfiles testcore-rtlib

testrun_compare ../src/unstrip -n --core=testcore-rtlib <<\EOF
0x8048000+0x2000 f1c600bc36cb91bf01f9a63a634ecb79aa4c3199@0x8048178 . - [exe]
0xf77d6000+0x1000 676560b1b765cde9c2e53f134f4ee354ea894747@0xf77d6210 . - linux-gate.so.1
0xf77b3000+0x9000 c6c5b5e35ab9589d4762ac85b4bd56b1b2720e37@0xf77b3164 /lib/librt.so.1 - librt.so.1
0xf7603000+0x1b0000 0b9bf374699e141e5dfc14757ff42b8c2373b4de@0xf7603184 /lib/libc.so.6 - libc.so.6
0xf75e9000+0x1a000 29a103420abe341e92072fb14274e250e4072148@0xf75e9164 /lib/libpthread.so.0 - libpthread.so.0
0xf77d7000+0x21000 6d2cb32650054f1c176d01d48713a4a5e5e84c1a@0xf77d7124 /lib/ld-linux.so.2 - ld-linux.so.2
EOF

exit 0
