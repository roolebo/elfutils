#! /bin/sh
# Copyright (C) 2005, 2007 Red Hat, Inc.
# This file is part of elfutils.
# Written by Ulrich Drepper <drepper@redhat.com>, 2005.
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

status=0
runtest() {
# Uncomment for debuging
#  echo $1
  if [ -f $1 ]; then
    testrun ../src/elflint --quiet --gnu-ld $1 ||
    { echo "*** failure in $1"; status=1; }
  fi
}

runtest ../src/addr2line
runtest ../src/elfcmp
runtest ../src/elflint
runtest ../src/findtextrel
runtest ../src/ld
runtest ../src/nm
runtest ../src/objdump
runtest ../src/readelf
runtest ../src/size
runtest ../src/strip
runtest ../libelf/libelf.so
runtest ../libdw/libdw.so
runtest ../libasm/libasm.so
runtest ../libebl/libebl_alpha.so
runtest ../libebl/libebl_arm.so
runtest ../libebl/libebl_i386.so
runtest ../libebl/libebl_ia64.so
runtest ../libebl/libebl_ppc.so
runtest ../libebl/libebl_ppc64.so
runtest ../libebl/libebl_sh.so
runtest ../libebl/libebl_sparc.so
runtest ../libebl/libebl_x86_64.so

exit $status
