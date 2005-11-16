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
. $srcdir/test-subr.sh

original=${original:-testfile19}
indexed=${indexed:-testfile19.index}

testfiles $original $indexed

testrun ../src/ranlib $original

if test -z "$noindex"; then
  # The data in the index is different.  The reference file has it blanked
  # out, we do the same here.
  echo "            " |
  dd of=$original seek=24 bs=1 count=12 conv=notrunc 2>/dev/null
fi

cmp $original $indexed

exit 0
