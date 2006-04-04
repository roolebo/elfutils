#! /bin/sh
# Copyright (C) 2006 Red Hat, Inc.
# Written by Ulrich Drepper <drepper@redhat.com>, 2006.
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

testfiles testfile28 testfile28.rdwr

testrun ./rdwrmmap testfile28

cmp testfile28 testfile28.rdwr

exit 0
