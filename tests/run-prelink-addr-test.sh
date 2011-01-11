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
# gcc -m32 -g -shared testfile52.c -o testfile52.so
# eu-strip -f testfile52.so.debug testfile52.so
# cp testfile52.so testfile52.prelink.so
# prelink -N testfile52.prelink.so

testfiles testfile52.so testfile52.prelink.so testfile52.so.debug
tempfiles testmaps52

cat > testmaps52 <<EOF
00110000-00111000 r-xp 00000000 00:00 0 [vdso]
00111000-00112000 r-xp 00000000 fd:01 1 `pwd`/testfile52.so
00112000-00113000 rw-p 00000000 fd:01 1 `pwd`/testfile52.so
41000000-41001000 r-xp 00000000 fd:01 2 `pwd`/testfile52.prelink.so
41001000-41002000 rw-p 00000000 fd:01 2 `pwd`/testfile52.prelink.so
4718e000-47191000 rw-p 00000000 00:00 0
f7fda000-f7fdb000 rw-p 00000000 00:00 0
f7ffd000-f7ffe000 rw-p 00000000 00:00 0
fffdd000-ffffe000 rw-p 00000000 00:00 0 [stack]
EOF

# Prior to commit 1743d7f, libdwfl would fail on the second address,
# because it didn't notice that prelink added a 0x20-byte offset from
# what the .debug file reports.
testrun_compare ../src/addr2line -S -M testmaps52 0x11140c 0x4100042d <<\EOF
foo
/home/jistone/src/elfutils/tests/testfile52.c:2
foo+0x1
/home/jistone/src/elfutils/tests/testfile52.c:2
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
