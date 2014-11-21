#! /bin/sh
# Copyright (C) 2014 Red Hat, Inc.
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

# See run-addrcfi.sh for testfilearm.

# = testfileppc32attrs.s =
# .gnu_attribute 8,1
# .gnu_attribute 12,1
#
# gcc -m32 -c testfileppc32attrs.s

testfiles testfilearm testfileppc32attrs.o

testrun_compare ${abs_top_builddir}/src/readelf -A testfilearm <<\EOF

Object attributes section [27] '.ARM.attributes' of 53 bytes at offset 0x718:
  Owner          Size
  aeabi            52
    File:          42
      CPU_name: 7-A
      CPU_arch: v7
      CPU_arch_profile: Application
      ARM_ISA_use: Yes
      THUMB_ISA_use: Thumb-2
      VFP_arch: VFPv3-D16
      ABI_PCS_wchar_t: 4
      ABI_FP_rounding: Needed
      ABI_FP_denormal: Needed
      ABI_FP_exceptions: Needed
      ABI_FP_number_model: IEEE 754
      ABI_align8_needed: Yes
      ABI_align8_preserved: Yes, except leaf SP
      ABI_enum_size: int
      ABI_HardFP_use: SP and DP
      ABI_VFP_args: VFP registers
      CPU_unaligned_access: v6
EOF

testrun_compare ${abs_top_builddir}/src/readelf -A testfileppc32attrs.o <<\EOF

Object attributes section [ 4] '.gnu.attributes' of 18 bytes at offset 0x34:
  Owner          Size
  gnu              17
    File:           9
      GNU_Power_ABI_Vector: Generic
      GNU_Power_ABI_Struct_Return: r3/r4
EOF

exit 0
