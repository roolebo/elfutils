#! /bin/sh
# Copyright (C) 1999, 2000, 2002, 2003, 2005 Red Hat, Inc.
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

original=${original:-testfile11}
stripped=${stripped:-testfile7}
debugout=${debugfile:+-f testfile.debug.temp -F $debugfile}

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/$original.bz2 > $original 2>/dev/null || exit 77

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/$stripped.bz2 > $stripped 2>/dev/null || exit 77

# Don't fail if we cannot decompress the file.
test -z "$debugfile" ||
bunzip2 -c $srcdir/$debugfile.bz2 > $debugfile 2>/dev/null || exit 77

LD_LIBRARY_PATH=../libebl:../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ../src/strip -o testfile.temp $debugout $original

cmp $stripped testfile.temp

# Check elflint and the expected result.
LD_LIBRARY_PATH=../libebl:../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ../src/elflint -q testfile.temp

test -z "$debugfile" || {
cmp $debugfile testfile.debug.temp

# Check elflint and the expected result.
LD_LIBRARY_PATH=../libebl:../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH \
  ../src/elflint -q -d testfile.debug.temp

rm -f "$debugfile"
}

rm -f $original $stripped testfile.temp testfile.debug.temp

exit 0
