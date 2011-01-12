#! /bin/sh
# Copyright (C) 2011 Red Hat, Inc.
# This file is part of Red Hat elfutils.
#
# Red Hat elfutils is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# Red Hat elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with Red Hat elfutils; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.
#
# Red Hat elfutils is an included package of the Open Invention Network.
# An included package of the Open Invention Network is a package for which
# Open Invention Network licensees cross-license their patents.  No patent
# license is granted, either expressly or impliedly, by designation as an
# included package.  Should you wish to participate in the Open Invention
# Network licensing program, please visit www.openinventionnetwork.com
# <http://www.openinventionnetwork.com>.

. $srcdir/test-subr.sh


# testfile52.c:
#   #include <stdlib.h>
#   int foo() { exit(0); }
#
# gcc -m32 -g -shared testfile52-32.c -o testfile52-32.so
# eu-strip -f testfile52-32.so.debug testfile52-32.so
# cp testfile52-32.so testfile52-32.prelink.so
# prelink -N testfile52-32.prelink.so
# cp testfile52-32.so testfile52-32.noshdrs.so
# prelink -r 0x42000000 testfile52-32.noshdrs.so
# eu-strip --remove-comment --strip-sections testfile52-32.noshdrs.so

testfiles testfile52-32.so testfile52-32.so.debug
testfiles testfile52-32.prelink.so testfile52-32.noshdrs.so
tempfiles testmaps52-32

cat > testmaps52-32 <<EOF
00111000-00112000 r-xp 00000000 fd:01 1 `pwd`/testfile52-32.so
00112000-00113000 rw-p 00000000 fd:01 1 `pwd`/testfile52-32.so
41000000-41001000 r-xp 00000000 fd:01 2 `pwd`/testfile52-32.prelink.so
41001000-41002000 rw-p 00000000 fd:01 2 `pwd`/testfile52-32.prelink.so
42000000-42001000 r-xp 00000000 fd:01 3 `pwd`/testfile52-32.noshdrs.so
42001000-42002000 rw-p 00000000 fd:01 3 `pwd`/testfile52-32.noshdrs.so
EOF

# Prior to commit 1743d7f, libdwfl would fail on the second address,
# because it didn't notice that prelink added a 0x20-byte offset from
# what the .debug file reports.
testrun_compare ../src/addr2line -S -M testmaps52-32 \
    0x11140c 0x4100042d 0x4200040e <<\EOF
foo
/home/jistone/src/elfutils/tests/testfile52-32.c:2
foo+0x1
/home/jistone/src/elfutils/tests/testfile52-32.c:2
foo+0x2
??:0
EOF

# Repeat testfile52 for -m64.  The particular REL>RELA issue doesn't exist, but
# we'll make sure the rest works anyway.
testfiles testfile52-64.so testfile52-64.so.debug
testfiles testfile52-64.prelink.so testfile52-64.noshdrs.so
tempfiles testmaps52-64

cat > testmaps52-64 <<EOF
1000000000-1000001000 r-xp 00000000 fd:11 1 `pwd`/testfile52-64.so
1000001000-1000200000 ---p 00001000 fd:11 1 `pwd`/testfile52-64.so
1000200000-1000201000 rw-p 00000000 fd:11 1 `pwd`/testfile52-64.so
3000000000-3000001000 r-xp 00000000 fd:11 2 `pwd`/testfile52-64.prelink.so
3000001000-3000200000 ---p 00001000 fd:11 2 `pwd`/testfile52-64.prelink.so
3000200000-3000201000 rw-p 00000000 fd:11 2 `pwd`/testfile52-64.prelink.so
3800000000-3800001000 r-xp 00000000 fd:11 3 `pwd`/testfile52-64.noshdrs.so
3800001000-3800200000 ---p 00001000 fd:11 3 `pwd`/testfile52-64.noshdrs.so
3800200000-3800201000 rw-p 00000000 fd:11 3 `pwd`/testfile52-64.noshdrs.so
EOF

testrun_compare ../src/addr2line -S -M testmaps52-64 \
    0x100000056c 0x300000056d 0x380000056e <<\EOF
foo
/home/jistone/src/elfutils/tests/testfile52-64.c:2
foo+0x1
/home/jistone/src/elfutils/tests/testfile52-64.c:2
foo+0x2
??:0
EOF


# testfile53.c:
#   char foo[2000];
#   int main() { return 0; }
#
# gcc -g testfile53.c -o testfile53
# eu-strip -f testfile53.debug testfile53
# cp testfile53 testfile53.prelinked
# prelink -N testfile53.prelinked
testfiles testfile53 testfile53.prelink testfile53.debug

testrun_compare ../src/addr2line -S -e testfile53 0x400474 0x400475 <<\EOF
main
/home/jistone/src/elfutils/tests/testfile53.c:2
main+0x1
/home/jistone/src/elfutils/tests/testfile53.c:2
EOF

# prelink shuffled some of the sections, but .text is in the same place.
testrun_compare ../src/addr2line -S -e testfile53.prelink 0x400476 0x400477 <<\EOF
main+0x2
/home/jistone/src/elfutils/tests/testfile53.c:2
main+0x3
/home/jistone/src/elfutils/tests/testfile53.c:2
EOF
