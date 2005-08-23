#! /bin/sh
# Copyright (C) 2005 Red Hat, Inc.
# Written by Ulrich Drepper <drepper@redhat.com>, 2005.
#
# This program is Open Source software; you can redistribute it and/or
# modify it under the terms of the Open Software License version 1.0 as
# published by the Open Source Initiative.
#
# You should have received a copy of the Open Software License along
# with this program; if not, you may obtain a copy of the Open Software
# License version 1.0 from http://www.opensource.org/licenses/osl.php or
# by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
# 3001 King Ranch Road, Ukiah, CA 95482.
set -e

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile.bz2 > testfile 2>/dev/null || exit 0

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile2.bz2 > testfile2 2>/dev/null || exit 0

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile8.bz2 > testfile8 2>/dev/null || exit 0

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile14.bz2 > testfile14 2>/dev/null || exit 0

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile23.bz2 > testfile23 2>/dev/null || exit 0

(./line2addr -e testfile f.c:4 testfile f.c:8
 ./line2addr -e testfile2 m.c:6 b.c:1
 ./line2addr -e testfile8 strip.c:953 strip.c:365
 ./line2addr -e testfile14 v.c:6
 ./line2addr -e testfile23 foo.c:2 foo.c:6
) > line2addr.out

diff -u line2addr.out - <<"EOF"
f.c:4 -> 0x804846b (/home/drepper/gnu/new-bu/build/ttt/f.c:4)
m.c:6 -> 0x100004cc (/shoggoth/drepper/m.c:6)
b.c:1 -> 0x10000470 (/shoggoth/drepper/b.c:4)
strip.c:953 -> (.text)+0x169f (/home/drepper/gnu/elfutils/build/src/../../src/strip.c:953)
strip.c:953 -> (.text)+0x16aa (/home/drepper/gnu/elfutils/build/src/../../src/strip.c:953)
strip.c:365 -> (.text)+0x278b (/home/drepper/gnu/elfutils/build/src/../../src/strip.c:365)
strip.c:365 -> (.text)+0x2797 (/home/drepper/gnu/elfutils/build/src/../../src/strip.c:365)
v.c:6 -> 0x400468 (/home/drepper/local/elfutils-build/20050425/v.c:6)
v.c:6 -> 0x400487 (/home/drepper/local/elfutils-build/20050425/v.c:6)
foo.c:2 -> (.init.text)+0xc (/home/roland/stock-elfutils-build/foo.c:2)
foo.c:6 -> (.text)+0xc (/home/roland/stock-elfutils-build/foo.c:6)
EOF

rm -f testfile testfile2 testfile8 testfile14 testfile22 line2addr.out

exit 0
