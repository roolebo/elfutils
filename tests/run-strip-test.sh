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
. $srcdir/test-subr.sh

original=${original:-testfile11}
stripped=${stripped:-testfile7}
debugout=${debugfile:+-f testfile.debug.temp -F $debugfile}

testfiles $original $stripped $debugfile

tempfiles testfile.temp testfile.debug.temp

testrun ../src/strip -o testfile.temp $debugout $original

cmp $stripped testfile.temp

# Check elflint and the expected result.
testrun ../src/elflint -q testfile.temp

test -z "$debugfile" || {
cmp $debugfile testfile.debug.temp

# Check elflint and the expected result.
testrun ../src/elflint -q -d testfile.debug.temp
}

exit 0
