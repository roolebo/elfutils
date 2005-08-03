#! /bin/sh
# Copyright (C) 1999, 2000, 2002 Red Hat, Inc.
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

lib=../libelf/libelf.a
okfile=arsymtest.ok
tmpfile=arsymtest.tmp
testfile=arsymtest.test

result=0
if test -f $lib; then
    # Generate list using `nm' we check against.
    nm -s $lib |
    sed -e '1,/^Arch/d' -e '/^$/,$d' |
    sort > $okfile

    # Now run our program using libelf.
    ./arsymtest $lib $tmpfile || exit 1
    sort $tmpfile > $testfile
    rm $tmpfile

    # Compare the outputs.
    if cmp $okfile $testfile; then
	result=0
	rm $testfile $okfile
    else
	result=1
    fi
fi

exit $result
