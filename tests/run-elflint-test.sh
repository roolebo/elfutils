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
bunzip2 -c $srcdir/testfile18.bz2 > testfile18 2>/dev/null || exit 0

LD_LIBRARY_PATH=../libebl:../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ../src/elflint --gnu testfile18 >& elflint-test.out || :

diff -u elflint-test.out - <<"EOF"
section [ 8] '.rela.dyn': relocation 1: copy relocation against symbol of type FUNC
EOF

rm -f testfile18 elflint-test.out

exit 0
