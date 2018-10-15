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

# - testfile-gnu-property-note.c
# int
# main ()
# {
#   return 0;
# }
#
# gcc -fcf-protection -c testfile-gnu-property-note.c
# gcc -o testfile-gnu-property-note testfile-gnu-property-note.o
# eu-strip --strip-sections testfile-gnu-property-note

testfiles testfile-gnu-property-note.o testfile-gnu-property-note

# Test reading notes through sections
testrun_compare ${abs_top_builddir}/src/readelf -n testfile-gnu-property-note.o << EOF

Note section [ 6] '.note.gnu.property' of 32 bytes at offset 0x80:
  Owner          Data size  Type
  GNU                   16  GNU_PROPERTY_TYPE_0
    X86 FEATURE_1_AND: 00000003 IBT SHSTK
EOF

# Test reading notes through segments
testrun_compare ${abs_top_builddir}/src/readelf -n testfile-gnu-property-note << EOF

Note segment of 32 bytes at offset 0x300:
  Owner          Data size  Type
  GNU                   16  GNU_PROPERTY_TYPE_0
    X86 FEATURE_1_AND: 00000003 IBT SHSTK

Note segment of 68 bytes at offset 0x320:
  Owner          Data size  Type
  GNU                   16  VERSION
    OS: Linux, ABI: 3.2.0
  GNU                   20  GNU_BUILD_ID
    Build ID: 83cb2229fabd2065d1361f5b46424cd75270f94b
EOF
