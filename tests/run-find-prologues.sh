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

files="testfile testfile11 testfile22 testfile24 \
testfile25 testfile3 testfile4 testfile5 testfile6"

for file in $files; do
  # Don't fail if we cannot decompress the file.
  bunzip2 -c $srcdir/$file.bz2 > $file 2>/dev/null || exit 77
done

for file in $files; do
  ./find-prologues -e $file || :
done > find-prologues-test.out 2>&1

diff -Bbu find-prologues-test.out - <<\EOF
main             0x000000000804842c 0x0000000008048432
bar              0x000000000804845c 0x000000000804845f
foo              0x0000000008048468 0x000000000804846b
main             0x00000000080489b8 0x00000000080489cd
gnu_obj_2        0x0000000008048c9e 0x0000000008048ca4
gnu_obj_3        0x0000000008048cd8 0x0000000008048cde
gnu_obj_2        0x0000000008048cf4 0x0000000008048cfa
~invalid_argument 0x0000000008048d2e 0x0000000008048d34
gnu_obj_1        0x0000000008048d62 0x0000000008048d65
gnu_obj_1        0x0000000008048d8a 0x0000000008048d8d
~invalid_argument 0x0000000008048db2 0x0000000008048db8
function         0x0000000008048348 0x000000000804834e
main             0x000000000804835b 0x0000000008048377
incr             0x0000000008048348 0x000000000804834e
main             0x0000000008048354 0x0000000008048360
incr             0x0000000008048348 0x000000000804834c
main             0x000000000804842c 0x0000000008048433
bar              0x0000000008048458 0x000000000804845b
foo              0x0000000008048464 0x0000000008048467
get              0x00000000080493fc 0x0000000008049402
main             0x0000000008049498 0x000000000804949e
a                0x000000000804d85c 0x000000000804d85c
__tfPCc          0x000000000804d86c 0x000000000804d872
__tfCc           0x000000000804d8a4 0x000000000804d8a4
bar              0x000000000804842c 0x000000000804842f
foo              0x0000000008048438 0x000000000804843b
main             0x0000000008048444 0x000000000804844a
main             0x00000000080489b8 0x00000000080489cd
gnu_obj_2        0x0000000008048c9e 0x0000000008048ca4
gnu_obj_3        0x0000000008048cd8 0x0000000008048cde
gnu_obj_2        0x0000000008048cf4 0x0000000008048cfa
~invalid_argument 0x0000000008048d2e 0x0000000008048d34
gnu_obj_1        0x0000000008048d62 0x0000000008048d65
gnu_obj_1        0x0000000008048d8a 0x0000000008048d8d
~invalid_argument 0x0000000008048db2 0x0000000008048db8
EOF

rm -f find-prologues-test.out $files
