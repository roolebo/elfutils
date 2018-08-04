#! /bin/sh
# Copyright (C) 2018 Red Hat, Inc.
# This file is part of elfutils.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. $srcdir/test-subr.sh

# Use the original file from run-strip-test.sh but with many sections
testfiles testfile
tempfiles testfile.strip testfile.debug testfile.unstrip

echo "Adding sections to testfile"
testrun ${abs_builddir}/addsections 65535 testfile

echo "Testing strip -o"
testrun ${abs_top_builddir}/src/strip -o testfile.strip -f testfile.debug testfile

# Do the parts check out?
echo "elflint testfile.strip"
testrun ${abs_top_builddir}/src/elflint --gnu -q testfile.strip
echo "elflint testfile.debug"
testrun ${abs_top_builddir}/src/elflint --gnu -q -d testfile.debug

# Now test unstrip recombining those files.
echo "unstrip"
testrun ${abs_top_builddir}/src/unstrip -o testfile.unstrip testfile.strip testfile.debug
echo "elfcmp"
testrun ${abs_top_builddir}/src/elfcmp testfile testfile.unstrip

# test strip -g
echo "Testing strip -g"
testrun ${abs_top_builddir}/src/strip -g -o testfile.strip -f testfile.debug testfile

# Do the parts check out?
echo "elflint testfile.strip"
testrun ${abs_top_builddir}/src/elflint --gnu -q testfile.strip
echo "elflint testfile.debug"
testrun ${abs_top_builddir}/src/elflint --gnu -q -d testfile.debug

# Now strip "in-place" and make sure it is smaller.
echo "TEsting strip in-place"
SIZE_original=$(stat -c%s testfile)
echo "original size $SIZE_original"

testrun ${abs_top_builddir}/src/strip testfile
SIZE_stripped=$(stat -c%s testfile)
echo "stripped size $SIZE_stripped"
test $SIZE_stripped -lt $SIZE_original ||
  { echo "*** failure in-place strip file not smaller $original"; exit 1; }

echo "elflint in-place"
testrun ${abs_top_builddir}/src/elflint --gnu -q testfile

exit 0
