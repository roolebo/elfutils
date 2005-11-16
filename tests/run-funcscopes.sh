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
. $srcdir/test-subr.sh

testfiles testfile25

testrun_compare ./funcscopes -e testfile25 incr <<\EOF
testfile25: 0x8048000 .. 0x8049528
    inline-test.c (0x11): 0x8048348 (/home/roland/build/stock-elfutils/inline-test.c:7) .. 0x804834f (/home/roland/build/stock-elfutils/inline-test.c:9)
        incr (0x2e): 0x8048348 (/home/roland/build/stock-elfutils/inline-test.c:7) .. 0x804834f (/home/roland/build/stock-elfutils/inline-test.c:9)
            x                             [    66]
EOF

exit 0
