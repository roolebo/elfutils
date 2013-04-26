#! /bin/sh
# Copyright (C) 2013 Red Hat, Inc.
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

# Tests dwfl_module_addrsym and dwfl_module_getsym.
# See run-readelf-s.sh for how to generate test binaries.

testfiles testfilebaztab
testfiles testfilebazdbg testfilebazdbg.debug
testfiles testfilebazdyn
testfiles testfilebazmdb
testfiles testfilebazmin
testfiles testfilebasmin

tempfiles testfile.dynsym.in testfile.symtab.in testfile.minsym.in dwflsyms.out

cat > testfile.symtab.in <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: SECTION	LOCAL	 (0) 0x238
   2: SECTION	LOCAL	 (0) 0x254
   3: SECTION	LOCAL	 (0) 0x274
   4: SECTION	LOCAL	 (0) 0x298
   5: SECTION	LOCAL	 (0) 0x2d8
   6: SECTION	LOCAL	 (0) 0x428
   7: SECTION	LOCAL	 (0) 0x4f2
   8: SECTION	LOCAL	 (0) 0x510
   9: SECTION	LOCAL	 (0) 0x530
  10: SECTION	LOCAL	 (0) 0x638
  11: SECTION	LOCAL	 (0) 0x680
  12: SECTION	LOCAL	 (0) 0x6a0
  13: SECTION	LOCAL	 (0) 0x6e0
  14: SECTION	LOCAL	 (0) 0x8f4
  15: SECTION	LOCAL	 (0) 0x900
  16: SECTION	LOCAL	 (0) 0x904
  17: SECTION	LOCAL	 (0) 0x948
  18: SECTION	LOCAL	 (0) 0x200dd0
  19: SECTION	LOCAL	 (0) 0x200dd8
  20: SECTION	LOCAL	 (0) 0x200de0
  21: SECTION	LOCAL	 (0) 0x200de8
  22: SECTION	LOCAL	 (0) 0x200df0
  23: SECTION	LOCAL	 (0) 0x200fc0
  24: SECTION	LOCAL	 (0) 0x201000
  25: SECTION	LOCAL	 (0) 0x201030
  26: SECTION	LOCAL	 (0) 0x20103c
  27: SECTION	LOCAL	 (0) 0
  28: SECTION	LOCAL	 (0) 0
  29: SECTION	LOCAL	 (0) 0
  30: SECTION	LOCAL	 (0) 0
  31: SECTION	LOCAL	 (0) 0
  32: SECTION	LOCAL	 (0) 0
  33: FILE	LOCAL	crtstuff.c (0) 0
  34: OBJECT	LOCAL	__JCR_LIST__ (0) 0x200de0
  35: FUNC	LOCAL	deregister_tm_clones (0) 0x710
  36: FUNC	LOCAL	register_tm_clones (0) 0x740
  37: FUNC	LOCAL	__do_global_dtors_aux (0) 0x780
  38: OBJECT	LOCAL	completed.6137 (1) 0x20103c
  39: OBJECT	LOCAL	__do_global_dtors_aux_fini_array_entry (0) 0x200dd8
  40: FUNC	LOCAL	frame_dummy (0) 0x7c0
  41: OBJECT	LOCAL	__frame_dummy_init_array_entry (0) 0x200dd0
  42: FILE	LOCAL	foo.c (0) 0
  43: FILE	LOCAL	bar.c (0) 0
  44: OBJECT	LOCAL	b1 (4) 0x201034
  45: FUNC	LOCAL	foo (20) 0x814
  46: FILE	LOCAL	crtstuff.c (0) 0
  47: OBJECT	LOCAL	__FRAME_END__ (0) 0xa58
  48: OBJECT	LOCAL	__JCR_END__ (0) 0x200de0
  49: FILE	LOCAL	 (0) 0
  50: NOTYPE	LOCAL	__init_array_end (0) 0x200dd8
  51: OBJECT	LOCAL	_DYNAMIC (0) 0x200df0
  52: NOTYPE	LOCAL	__init_array_start (0) 0x200dd0
  53: OBJECT	LOCAL	_GLOBAL_OFFSET_TABLE_ (0) 0x201000
  54: FUNC	GLOBAL	__libc_csu_fini (2) 0x8f0
  55: NOTYPE	WEAK	_ITM_deregisterTMCloneTable (0) 0
  56: NOTYPE	WEAK	data_start (0) 0x201030
  57: NOTYPE	GLOBAL	_edata (0) 0x20103c
  58: FUNC	GLOBAL	bar (44) 0x828
  59: FUNC	GLOBAL	_fini (0) 0x8f4
  60: FUNC	GLOBAL	__libc_start_main@@GLIBC_2.2.5 (0) 0
  61: NOTYPE	GLOBAL	__data_start (0) 0x201030
  62: NOTYPE	WEAK	__gmon_start__ (0) 0
  63: OBJECT	GLOBAL	__dso_handle (0) 0x200de8
  64: OBJECT	GLOBAL	_IO_stdin_used (4) 0x900
  65: OBJECT	GLOBAL	b2 (4) 0x201038
  66: FUNC	GLOBAL	__libc_csu_init (137) 0x860
  67: NOTYPE	GLOBAL	_end (0) 0x201040
  68: FUNC	GLOBAL	_start (0) 0x6e0
  69: NOTYPE	GLOBAL	__bss_start (0) 0x20103c
  70: FUNC	GLOBAL	main (35) 0x7f0
  71: NOTYPE	WEAK	_Jv_RegisterClasses (0) 0
  72: OBJECT	GLOBAL	__TMC_END__ (0) 0x201040
  73: NOTYPE	WEAK	_ITM_registerTMCloneTable (0) 0
  74: FUNC	WEAK	__cxa_finalize@@GLIBC_2.2.5 (0) 0
  75: FUNC	GLOBAL	_init (0) 0x680
EOF

cat > testfile.dynsym.in <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: SECTION	LOCAL	 (0) 0x238
   2: NOTYPE	WEAK	_ITM_deregisterTMCloneTable (0) 0
   3: FUNC	GLOBAL	__libc_start_main (0) 0
   4: NOTYPE	WEAK	__gmon_start__ (0) 0
   5: NOTYPE	WEAK	_Jv_RegisterClasses (0) 0
   6: NOTYPE	WEAK	_ITM_registerTMCloneTable (0) 0
   7: FUNC	WEAK	__cxa_finalize (0) 0
   8: NOTYPE	GLOBAL	_edata (0) 0x20103c
   9: NOTYPE	GLOBAL	_end (0) 0x201040
  10: FUNC	GLOBAL	__libc_csu_init (137) 0x860
  11: NOTYPE	GLOBAL	__bss_start (0) 0x20103c
  12: FUNC	GLOBAL	main (35) 0x7f0
  13: FUNC	GLOBAL	__libc_csu_fini (2) 0x8f0
EOF

cat > testfile.minsym.in <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: SECTION	LOCAL	 (0) 0x238
   2: FUNC	LOCAL	call_gmon_start (0) 0x4003bc
   3: FUNC	LOCAL	__do_global_dtors_aux (0) 0x4003e0
   4: FUNC	LOCAL	frame_dummy (0) 0x400450
   5: FUNC	LOCAL	__do_global_ctors_aux (0) 0x400580
   6: FUNC	LOCAL	foo (18) 0x400498
   7: SECTION	LOCAL	 (0) 0x400200
   8: SECTION	LOCAL	 (0) 0x40021c
   9: SECTION	LOCAL	 (0) 0x40023c
  10: SECTION	LOCAL	 (0) 0x400260
  11: SECTION	LOCAL	 (0) 0x400280
  12: SECTION	LOCAL	 (0) 0x4002c8
  13: SECTION	LOCAL	 (0) 0x400300
  14: SECTION	LOCAL	 (0) 0x400308
  15: SECTION	LOCAL	 (0) 0x400328
  16: SECTION	LOCAL	 (0) 0x400340
  17: SECTION	LOCAL	 (0) 0x400358
  18: SECTION	LOCAL	 (0) 0x400370
  19: SECTION	LOCAL	 (0) 0x400390
  20: SECTION	LOCAL	 (0) 0x4005b8
  21: SECTION	LOCAL	 (0) 0x4005c8
  22: SECTION	LOCAL	 (0) 0x4005d8
  23: SECTION	LOCAL	 (0) 0x400610
  24: SECTION	LOCAL	 (0) 0x6006d0
  25: SECTION	LOCAL	 (0) 0x6006e0
  26: SECTION	LOCAL	 (0) 0x6006f0
  27: SECTION	LOCAL	 (0) 0x6006f8
  28: SECTION	LOCAL	 (0) 0x600888
  29: SECTION	LOCAL	 (0) 0x600890
  30: SECTION	LOCAL	 (0) 0x6008b0
  31: SECTION	LOCAL	 (0) 0x6008c0
  32: NOTYPE	WEAK	_ITM_deregisterTMCloneTable (0) 0
  33: FUNC	GLOBAL	__libc_start_main (0) 0
  34: NOTYPE	WEAK	__gmon_start__ (0) 0
  35: NOTYPE	WEAK	_Jv_RegisterClasses (0) 0
  36: NOTYPE	WEAK	_ITM_registerTMCloneTable (0) 0
  37: FUNC	WEAK	__cxa_finalize (0) 0
  38: NOTYPE	GLOBAL	_edata (0) 0x20103c
  39: NOTYPE	GLOBAL	_end (0) 0x201040
  40: FUNC	GLOBAL	__libc_csu_init (137) 0x860
  41: NOTYPE	GLOBAL	__bss_start (0) 0x20103c
  42: FUNC	GLOBAL	main (35) 0x7f0
  43: FUNC	GLOBAL	__libc_csu_fini (2) 0x8f0
  44: FUNC	GLOBAL	_start (0) 0x400390
  45: FUNC	GLOBAL	bar (44) 0x4004aa
  46: FUNC	GLOBAL	_fini (0) 0x4005b8
  47: FUNC	GLOBAL	_init (0) 0x400358
EOF

cat testfile.symtab.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebaztab

cat testfile.symtab.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazdbg

cat testfile.dynsym.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazdyn

cat testfile.symtab.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazmdb

cat testfile.minsym.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazmin

testrun_compare ${abs_builddir}/dwflsyms -e testfilebasmin <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: FUNC	LOCAL	foo (18) 0x400168
   2: SECTION	LOCAL	 (0) 0x400120
   3: SECTION	LOCAL	 (0) 0x400144
   4: SECTION	LOCAL	 (0) 0x4001c0
   5: SECTION	LOCAL	 (0) 0x600258
   6: FUNC	GLOBAL	_start (21) 0x4001a8
   7: FUNC	GLOBAL	main (33) 0x400144
   8: FUNC	GLOBAL	bar (44) 0x40017a
EOF

exit 0
