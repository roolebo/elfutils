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

export LD_LIBRARY_PATH=../libebl:../libelf${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH

runtest() {
# Uncomment for debuging
#  echo $1
  if [ -f $1 ]; then
    ../src/elflint --quiet --gnu-ld $1
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
