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

# This tests all the miscellaneous components of backend support
# against whatever this build is running on.  A platform will fail
# this test if it is missing parts of the backend implementation.
#
# As new backend code is added to satisfy the test, be sure to update
# the fixed test cases (run-allregs.sh et al) to test that backend
# in all builds.

tempfiles native.c native
echo 'main () { while (1) pause (); }' > native.c

native=0
native_cleanup()
{
  test $native -eq 0 || kill -9 $native || :
  rm -f $remove_files
}
trap native_cleanup 0 1 2 15

for cc in "$HOSTCC" "$HOST_CC" cc gcc "$CC"; do
  test "x$cc" != x || continue
  $cc -o native -g native.c > /dev/null 2>&1 &&
  ./native > /dev/null 2>&1 & native=$! &&
  sleep 1 && kill -0 $native 2> /dev/null &&
  break ||
  native=0
done

native_test()
{
  # Try the build against itself, i.e. $config_host.
  testrun "$@" -e $1 > /dev/null

  # Try the build against a presumed native process, running this sh.
  # For tests requiring debug information, this may not test anything.
  testrun "$@" -p $$ > /dev/null

  # Try the build against the trivial native program we just built with -g.
  test $native -eq 0 || testrun "$@" -p $native > /dev/null
}

native_test ./allregs
native_test ./funcretval
