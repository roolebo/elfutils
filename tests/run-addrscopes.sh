#! /bin/sh
# Copyright (C) 2005 Red Hat, Inc.
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
bunzip2 -c $srcdir/testfile22.bz2 > testfile22 2>/dev/null || exit 0

LD_LIBRARY_PATH=../libdw:../libebl:../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ./addrscopes -e testfile22 0x8048353 >& addrscopes-test.out || :

diff -Bbu addrscopes-test.out - <<\EOF
0x8048353:
    tests/foo.c (0x11): 0x8048348 (tests/foo.c:5) .. 0x804837e (tests/foo.c:16)
        global                        [    be]
        function (0x2e): 0x8048348 (tests/foo.c:5) .. 0x804835b (tests/foo.c:14)
            local                         [    8f]
EOF

rm -f testfile22 addrscopes-test.out

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile24.bz2 > testfile24 2>/dev/null || exit 0

LD_LIBRARY_PATH=../libdw:../libebl:../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ./addrscopes -e testfile24 0x804834e >& addrscopes-test.out || :

diff -Bbu addrscopes-test.out - <<\EOF
0x804834e:
    inline-test.c (0x11): 0x8048348 (/home/roland/build/stock-elfutils/inline-test.c:7) .. 0x8048364 (/home/roland/build/stock-elfutils/inline-test.c:16)
        add (0x1d): 0x804834e (/home/roland/build/stock-elfutils/inline-test.c:3) .. 0x8048350 (/home/roland/build/stock-elfutils/inline-test.c:9)
            y                             [    9d]
            x                             [    a2]
            x (abstract)
            y (abstract)
EOF

rm -f testfile24 addrscopes-test.out

exit 0
