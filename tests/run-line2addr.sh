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

./line2addr testfile:f.c:4 testfile:f.c:8 testfile2:m.c:6 testfile2:b.c:1 testfile8:strip.c:953 testfile8:strip.c:365 testfile14:v.c:6 > line2addr.out

diff -u line2addr.out - <<"EOF"
testfile:f.c:4 -> 0x804846b
testfile2:m.c:6 -> 0x100004cc
testfile2:b.c:1 -> 0x10000470
testfile8:strip.c:953 -> 0x169f
testfile8:strip.c:953 -> 0x16aa
testfile8:strip.c:365 -> 0x278b
testfile8:strip.c:365 -> 0x2797
testfile14:v.c:6 -> 0x400468
testfile14:v.c:6 -> 0x400487
EOF

rm -f testfile testfile2 testfile8 testfile14 line2addr.out

exit 0
