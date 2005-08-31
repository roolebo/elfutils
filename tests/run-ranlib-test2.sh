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

original=${original:-testfile19}
indexed=${indexed:-testfile19.index}

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/$original.bz2 > $original 2>/dev/null || exit 77

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/$indexed.bz2 > $indexed 2>/dev/null || exit 77

LD_LIBRARY_PATH=../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ../src/ranlib $original

if test -z "$noindex"; then
  # The data in the index is different.  The reference file has it blanked
  # out, we do the same here.
  echo "            " |
  dd of=$original seek=24 bs=1 count=12 conv=notrunc 2>/dev/null
fi

cmp $original $indexed

rm -f $original $indexed

exit 0
