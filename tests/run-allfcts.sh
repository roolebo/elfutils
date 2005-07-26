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

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile.bz2 > testfile 2>/dev/null || exit 0

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile2.bz2 > testfile2 2>/dev/null || exit 0

# Don't fail if we cannot decompress the file.
bunzip2 -c $srcdir/testfile8.bz2 > testfile8 2>/dev/null || exit 0

./allfcts testfile testfile2 testfile8 > allfcts.out

diff -u allfcts.out - <<"EOF"
/home/drepper/gnu/new-bu/build/ttt/m.c:5:main
/home/drepper/gnu/new-bu/build/ttt/b.c:4:bar
/home/drepper/gnu/new-bu/build/ttt/f.c:3:foo
/shoggoth/drepper/b.c:4:bar
/shoggoth/drepper/f.c:3:foo
/shoggoth/drepper/m.c:5:main
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:107:main
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:159:print_version
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:173:parse_opt
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:201:more_help
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:217:process_file
/usr/include/sys/stat.h:375:stat64
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:291:crc32_file
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:313:handle_elf
EOF

rm -f testfile testfile2 testfile8 allfcts.out

exit 0
