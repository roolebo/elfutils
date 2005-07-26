#! /bin/sh
# Copyright (C) 1999, 2000, 2002, 2005 Red Hat, Inc.
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

archive=../libelf/libelf.a
if test -f $archive; then
    # The file is really available (i.e., no shared-only built).
    echo -n "Extracting symbols... $ac_c"

    # The files we are looking at.
    for f in ../libelf/*.o; do
	./arextract $archive `basename $f` arextract.test || exit 1
	cmp $f arextract.test || {
	    echo "Extraction of $1 failed"
	    exit 1
	}
	rm -f ${objpfx}arextract.test
    done

    echo "done"
fi

exit 0
