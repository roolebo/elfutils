#! /bin/sh
# Copyright (C) 1999, 2000, 2002 Red Hat, Inc.
# Written by Ulrich Drepper <drepper@redhat.com>, 1999.
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

./get-aranges testfile testfile2 > get-aranges.out

cmp get-aranges.out - <<"EOF"
0x804842b: not in range
CU name: "m.c"
CU name: "m.c"
CU name: "m.c"
0x804845a: not in range
0x804845b: not in range
CU name: "b.c"
CU name: "b.c"
CU name: "b.c"
0x8048466: not in range
0x8048467: not in range
CU name: "f.c"
CU name: "f.c"
CU name: "f.c"
0x8048472: not in range
 [ 0] start: 0x804842c, length: 46, cu: 11
CU name: "m.c"
 [ 1] start: 0x804845c, length: 10, cu: 202
CU name: "b.c"
 [ 2] start: 0x8048468, length: 10, cu: 5628
CU name: "f.c"
0x804842b: not in range
0x804842c: not in range
0x804843c: not in range
0x8048459: not in range
0x804845a: not in range
0x804845b: not in range
0x804845c: not in range
0x8048460: not in range
0x8048465: not in range
0x8048466: not in range
0x8048467: not in range
0x8048468: not in range
0x8048470: not in range
0x8048471: not in range
0x8048472: not in range
 [ 0] start: 0x10000470, length: 32, cu: 11
CU name: "b.c"
 [ 1] start: 0x10000490, length: 32, cu: 2429
CU name: "f.c"
 [ 2] start: 0x100004b0, length: 100, cu: 2532
CU name: "m.c"
EOF

rm -f testfile testfile2 get-aranges.out

exit 0
