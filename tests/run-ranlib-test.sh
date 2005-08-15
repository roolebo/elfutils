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

cat > ranlib-test.a <<"EOF"
!<arch>
foo/            1124128960  500   500   100664  4         `
foo
bar/            1124128965  500   500   100664  4         `
bar
EOF

cp ranlib-test.a ranlib-test.a-copy

LD_LIBRARY_PATH=../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ../src/ranlib ranlib-test.a

# The ranlib call should not have changed anything.
cmp ranlib-test.a ranlib-test.a-copy

rm -f ranlib-test.a ranlib-test.a-copy

exit 0
