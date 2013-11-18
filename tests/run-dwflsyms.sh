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

# Tests dwfl_module_{addrsym,getsym,relocate_address}
# See run-readelf-s.sh for how to generate test binaries.
# In addition, *_pl files were created from their base file
# with prelink -N, and *_plr with prelink -r 0x4200000000.

testfiles testfilebaztab
testfiles testfilebazdbg testfilebazdbg.debug
testfiles testfilebazdbg_pl
testfiles testfilebazdbg_plr
testfiles testfilebazdyn
testfiles testfilebazmdb
testfiles testfilebazmin
testfiles testfilebazmin_pl
testfiles testfilebazmin_plr
testfiles testfilebasmin

tempfiles testfile.dynsym.in testfile.symtab.in testfile.minsym.in dwflsyms.out
tempfiles testfile.symtab_pl.in testfile.minsym_pl.in 

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
  35: FUNC	LOCAL	deregister_tm_clones (0) 0x710, rel: 0x710
  36: FUNC	LOCAL	register_tm_clones (0) 0x740, rel: 0x740
  37: FUNC	LOCAL	__do_global_dtors_aux (0) 0x780, rel: 0x780
  38: OBJECT	LOCAL	completed.6137 (1) 0x20103c
  39: OBJECT	LOCAL	__do_global_dtors_aux_fini_array_entry (0) 0x200dd8
  40: FUNC	LOCAL	frame_dummy (0) 0x7c0, rel: 0x7c0
  41: OBJECT	LOCAL	__frame_dummy_init_array_entry (0) 0x200dd0
  42: FILE	LOCAL	foo.c (0) 0
  43: FILE	LOCAL	bar.c (0) 0
  44: OBJECT	LOCAL	b1 (4) 0x201034
  45: FUNC	LOCAL	foo (20) 0x814, rel: 0x814
  46: FILE	LOCAL	crtstuff.c (0) 0
  47: OBJECT	LOCAL	__FRAME_END__ (0) 0xa58
  48: OBJECT	LOCAL	__JCR_END__ (0) 0x200de0
  49: FILE	LOCAL	 (0) 0
  50: NOTYPE	LOCAL	__init_array_end (0) 0x200dd8
  51: OBJECT	LOCAL	_DYNAMIC (0) 0x200df0
  52: NOTYPE	LOCAL	__init_array_start (0) 0x200dd0
  53: OBJECT	LOCAL	_GLOBAL_OFFSET_TABLE_ (0) 0x201000
  54: FUNC	GLOBAL	__libc_csu_fini (2) 0x8f0, rel: 0x8f0
  55: NOTYPE	WEAK	_ITM_deregisterTMCloneTable (0) 0
  56: NOTYPE	WEAK	data_start (0) 0x201030
  57: NOTYPE	GLOBAL	_edata (0) 0x20103c
  58: FUNC	GLOBAL	bar (44) 0x828, rel: 0x828
  59: FUNC	GLOBAL	_fini (0) 0x8f4, rel: 0x8f4
  60: FUNC	GLOBAL	__libc_start_main@@GLIBC_2.2.5 (0) 0
  61: NOTYPE	GLOBAL	__data_start (0) 0x201030
  62: NOTYPE	WEAK	__gmon_start__ (0) 0
  63: OBJECT	GLOBAL	__dso_handle (0) 0x200de8
  64: OBJECT	GLOBAL	_IO_stdin_used (4) 0x900
  65: OBJECT	GLOBAL	b2 (4) 0x201038
  66: FUNC	GLOBAL	__libc_csu_init (137) 0x860, rel: 0x860
  67: NOTYPE	GLOBAL	_end (0) 0x201040
  68: FUNC	GLOBAL	_start (0) 0x6e0, rel: 0x6e0
  69: NOTYPE	GLOBAL	__bss_start (0) 0x20103c
  70: FUNC	GLOBAL	main (35) 0x7f0, rel: 0x7f0
  71: NOTYPE	WEAK	_Jv_RegisterClasses (0) 0
  72: OBJECT	GLOBAL	__TMC_END__ (0) 0x201040
  73: NOTYPE	WEAK	_ITM_registerTMCloneTable (0) 0
  74: FUNC	WEAK	__cxa_finalize@@GLIBC_2.2.5 (0) 0
  75: FUNC	GLOBAL	_init (0) 0x680, rel: 0x680
EOF

cat > testfile.symtab_pl.in <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: SECTION	LOCAL	 (0) 0x3000000238
   2: SECTION	LOCAL	 (0) 0x3000000254
   3: SECTION	LOCAL	 (0) 0x3000000274
   4: SECTION	LOCAL	 (0) 0x3000000298
   5: SECTION	LOCAL	 (0) 0x30000002d8
   6: SECTION	LOCAL	 (0) 0x3000000428
   7: SECTION	LOCAL	 (0) 0x30000004f2
   8: SECTION	LOCAL	 (0) 0x3000000510
   9: SECTION	LOCAL	 (0) 0x3000000530
  10: SECTION	LOCAL	 (0) 0x3000000638
  11: SECTION	LOCAL	 (0) 0x3000000680
  12: SECTION	LOCAL	 (0) 0x30000006a0
  13: SECTION	LOCAL	 (0) 0x30000006e0
  14: SECTION	LOCAL	 (0) 0x30000008f4
  15: SECTION	LOCAL	 (0) 0x3000000900
  16: SECTION	LOCAL	 (0) 0x3000000904
  17: SECTION	LOCAL	 (0) 0x3000000948
  18: SECTION	LOCAL	 (0) 0x3000200dd0
  19: SECTION	LOCAL	 (0) 0x3000200dd8
  20: SECTION	LOCAL	 (0) 0x3000200de0
  21: SECTION	LOCAL	 (0) 0x3000200de8
  22: SECTION	LOCAL	 (0) 0x3000200df0
  23: SECTION	LOCAL	 (0) 0x3000200fc0
  24: SECTION	LOCAL	 (0) 0x3000201000
  25: SECTION	LOCAL	 (0) 0x3000201030
  26: SECTION	LOCAL	 (0) 0x300020103c
  27: SECTION	LOCAL	 (0) 0
  28: SECTION	LOCAL	 (0) 0
  29: SECTION	LOCAL	 (0) 0
  30: SECTION	LOCAL	 (0) 0
  31: SECTION	LOCAL	 (0) 0
  32: SECTION	LOCAL	 (0) 0
  33: FILE	LOCAL	crtstuff.c (0) 0
  34: OBJECT	LOCAL	__JCR_LIST__ (0) 0x3000200de0
  35: FUNC	LOCAL	deregister_tm_clones (0) 0x3000000710, rel: 0x710
  36: FUNC	LOCAL	register_tm_clones (0) 0x3000000740, rel: 0x740
  37: FUNC	LOCAL	__do_global_dtors_aux (0) 0x3000000780, rel: 0x780
  38: OBJECT	LOCAL	completed.6137 (1) 0x300020103c
  39: OBJECT	LOCAL	__do_global_dtors_aux_fini_array_entry (0) 0x3000200dd8
  40: FUNC	LOCAL	frame_dummy (0) 0x30000007c0, rel: 0x7c0
  41: OBJECT	LOCAL	__frame_dummy_init_array_entry (0) 0x3000200dd0
  42: FILE	LOCAL	foo.c (0) 0
  43: FILE	LOCAL	bar.c (0) 0
  44: OBJECT	LOCAL	b1 (4) 0x3000201034
  45: FUNC	LOCAL	foo (20) 0x3000000814, rel: 0x814
  46: FILE	LOCAL	crtstuff.c (0) 0
  47: OBJECT	LOCAL	__FRAME_END__ (0) 0x3000000a58
  48: OBJECT	LOCAL	__JCR_END__ (0) 0x3000200de0
  49: FILE	LOCAL	 (0) 0
  50: NOTYPE	LOCAL	__init_array_end (0) 0x3000200dd8
  51: OBJECT	LOCAL	_DYNAMIC (0) 0x3000200df0
  52: NOTYPE	LOCAL	__init_array_start (0) 0x3000200dd0
  53: OBJECT	LOCAL	_GLOBAL_OFFSET_TABLE_ (0) 0x3000201000
  54: FUNC	GLOBAL	__libc_csu_fini (2) 0x30000008f0, rel: 0x8f0
  55: NOTYPE	WEAK	_ITM_deregisterTMCloneTable (0) 0
  56: NOTYPE	WEAK	data_start (0) 0x3000201030
  57: NOTYPE	GLOBAL	_edata (0) 0x300020103c
  58: FUNC	GLOBAL	bar (44) 0x3000000828, rel: 0x828
  59: FUNC	GLOBAL	_fini (0) 0x30000008f4, rel: 0x8f4
  60: FUNC	GLOBAL	__libc_start_main@@GLIBC_2.2.5 (0) 0
  61: NOTYPE	GLOBAL	__data_start (0) 0x3000201030
  62: NOTYPE	WEAK	__gmon_start__ (0) 0
  63: OBJECT	GLOBAL	__dso_handle (0) 0x3000200de8
  64: OBJECT	GLOBAL	_IO_stdin_used (4) 0x3000000900
  65: OBJECT	GLOBAL	b2 (4) 0x3000201038
  66: FUNC	GLOBAL	__libc_csu_init (137) 0x3000000860, rel: 0x860
  67: NOTYPE	GLOBAL	_end (0) 0x3000201040
  68: FUNC	GLOBAL	_start (0) 0x30000006e0, rel: 0x6e0
  69: NOTYPE	GLOBAL	__bss_start (0) 0x300020103c
  70: FUNC	GLOBAL	main (35) 0x30000007f0, rel: 0x7f0
  71: NOTYPE	WEAK	_Jv_RegisterClasses (0) 0
  72: OBJECT	GLOBAL	__TMC_END__ (0) 0x3000201040
  73: NOTYPE	WEAK	_ITM_registerTMCloneTable (0) 0
  74: FUNC	WEAK	__cxa_finalize@@GLIBC_2.2.5 (0) 0
  75: FUNC	GLOBAL	_init (0) 0x3000000680, rel: 0x680
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
  10: FUNC	GLOBAL	__libc_csu_init (137) 0x860, rel: 0x860
  11: NOTYPE	GLOBAL	__bss_start (0) 0x20103c
  12: FUNC	GLOBAL	main (35) 0x7f0, rel: 0x7f0
  13: FUNC	GLOBAL	__libc_csu_fini (2) 0x8f0, rel: 0x8f0
EOF

cat > testfile.minsym.in <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: SECTION	LOCAL	 (0) 0x238
   2: FUNC	LOCAL	deregister_tm_clones (0) 0x710, rel: 0x710
   3: FUNC	LOCAL	register_tm_clones (0) 0x740, rel: 0x740
   4: FUNC	LOCAL	__do_global_dtors_aux (0) 0x780, rel: 0x780
   5: OBJECT	LOCAL	__do_global_dtors_aux_fini_array_entry (0) 0x200dd8
   6: FUNC	LOCAL	frame_dummy (0) 0x7c0, rel: 0x7c0
   7: OBJECT	LOCAL	__frame_dummy_init_array_entry (0) 0x200dd0
   8: FUNC	LOCAL	foo (20) 0x814, rel: 0x814
   9: NOTYPE	LOCAL	__init_array_end (0) 0x200dd8
  10: NOTYPE	LOCAL	__init_array_start (0) 0x200dd0
  11: SECTION	LOCAL	 (0) 0x238
  12: SECTION	LOCAL	 (0) 0x254
  13: SECTION	LOCAL	 (0) 0x274
  14: SECTION	LOCAL	 (0) 0x298
  15: SECTION	LOCAL	 (0) 0x2d8
  16: SECTION	LOCAL	 (0) 0x428
  17: SECTION	LOCAL	 (0) 0x4f2
  18: SECTION	LOCAL	 (0) 0x510
  19: SECTION	LOCAL	 (0) 0x530
  20: SECTION	LOCAL	 (0) 0x638
  21: SECTION	LOCAL	 (0) 0x680
  22: SECTION	LOCAL	 (0) 0x6a0
  23: SECTION	LOCAL	 (0) 0x6e0
  24: SECTION	LOCAL	 (0) 0x8f4
  25: SECTION	LOCAL	 (0) 0x900
  26: SECTION	LOCAL	 (0) 0x904
  27: SECTION	LOCAL	 (0) 0x948
  28: SECTION	LOCAL	 (0) 0x200dd0
  29: SECTION	LOCAL	 (0) 0x200dd8
  30: SECTION	LOCAL	 (0) 0x200de0
  31: SECTION	LOCAL	 (0) 0x200de8
  32: SECTION	LOCAL	 (0) 0x200df0
  33: SECTION	LOCAL	 (0) 0x200fc0
  34: SECTION	LOCAL	 (0) 0x201000
  35: SECTION	LOCAL	 (0) 0x201030
  36: SECTION	LOCAL	 (0) 0x20103c
  37: NOTYPE	WEAK	_ITM_deregisterTMCloneTable (0) 0
  38: FUNC	GLOBAL	__libc_start_main (0) 0
  39: NOTYPE	WEAK	__gmon_start__ (0) 0
  40: NOTYPE	WEAK	_Jv_RegisterClasses (0) 0
  41: NOTYPE	WEAK	_ITM_registerTMCloneTable (0) 0
  42: FUNC	WEAK	__cxa_finalize (0) 0
  43: NOTYPE	GLOBAL	_edata (0) 0x20103c
  44: NOTYPE	GLOBAL	_end (0) 0x201040
  45: FUNC	GLOBAL	__libc_csu_init (137) 0x860, rel: 0x860
  46: NOTYPE	GLOBAL	__bss_start (0) 0x20103c
  47: FUNC	GLOBAL	main (35) 0x7f0, rel: 0x7f0
  48: FUNC	GLOBAL	__libc_csu_fini (2) 0x8f0, rel: 0x8f0
  49: FUNC	GLOBAL	bar (44) 0x828, rel: 0x828
  50: FUNC	GLOBAL	_fini (0) 0x8f4, rel: 0x8f4
  51: FUNC	GLOBAL	_start (0) 0x6e0, rel: 0x6e0
  52: FUNC	GLOBAL	_init (0) 0x680, rel: 0x680
EOF

cat > testfile.minsym_pl.in <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: SECTION	LOCAL	 (0) 0x3000000238
   2: FUNC	LOCAL	deregister_tm_clones (0) 0x3000000710, rel: 0x710
   3: FUNC	LOCAL	register_tm_clones (0) 0x3000000740, rel: 0x740
   4: FUNC	LOCAL	__do_global_dtors_aux (0) 0x3000000780, rel: 0x780
   5: OBJECT	LOCAL	__do_global_dtors_aux_fini_array_entry (0) 0x3000200dd8
   6: FUNC	LOCAL	frame_dummy (0) 0x30000007c0, rel: 0x7c0
   7: OBJECT	LOCAL	__frame_dummy_init_array_entry (0) 0x3000200dd0
   8: FUNC	LOCAL	foo (20) 0x3000000814, rel: 0x814
   9: NOTYPE	LOCAL	__init_array_end (0) 0x3000200dd8
  10: NOTYPE	LOCAL	__init_array_start (0) 0x3000200dd0
  11: SECTION	LOCAL	 (0) 0x3000000238
  12: SECTION	LOCAL	 (0) 0x3000000254
  13: SECTION	LOCAL	 (0) 0x3000000274
  14: SECTION	LOCAL	 (0) 0x3000000298
  15: SECTION	LOCAL	 (0) 0x30000002d8
  16: SECTION	LOCAL	 (0) 0x3000000428
  17: SECTION	LOCAL	 (0) 0x30000004f2
  18: SECTION	LOCAL	 (0) 0x3000000510
  19: SECTION	LOCAL	 (0) 0x3000000530
  20: SECTION	LOCAL	 (0) 0x3000000638
  21: SECTION	LOCAL	 (0) 0x3000000680
  22: SECTION	LOCAL	 (0) 0x30000006a0
  23: SECTION	LOCAL	 (0) 0x30000006e0
  24: SECTION	LOCAL	 (0) 0x30000008f4
  25: SECTION	LOCAL	 (0) 0x3000000900
  26: SECTION	LOCAL	 (0) 0x3000000904
  27: SECTION	LOCAL	 (0) 0x3000000948
  28: SECTION	LOCAL	 (0) 0x3000200dd0
  29: SECTION	LOCAL	 (0) 0x3000200dd8
  30: SECTION	LOCAL	 (0) 0x3000200de0
  31: SECTION	LOCAL	 (0) 0x3000200de8
  32: SECTION	LOCAL	 (0) 0x3000200df0
  33: SECTION	LOCAL	 (0) 0x3000200fc0
  34: SECTION	LOCAL	 (0) 0x3000201000
  35: SECTION	LOCAL	 (0) 0x3000201030
  36: SECTION	LOCAL	 (0) 0x300020103c
  37: NOTYPE	WEAK	_ITM_deregisterTMCloneTable (0) 0
  38: FUNC	GLOBAL	__libc_start_main (0) 0
  39: NOTYPE	WEAK	__gmon_start__ (0) 0
  40: NOTYPE	WEAK	_Jv_RegisterClasses (0) 0
  41: NOTYPE	WEAK	_ITM_registerTMCloneTable (0) 0
  42: FUNC	WEAK	__cxa_finalize (0) 0
  43: NOTYPE	GLOBAL	_edata (0) 0x300020103c
  44: NOTYPE	GLOBAL	_end (0) 0x3000201040
  45: FUNC	GLOBAL	__libc_csu_init (137) 0x3000000860, rel: 0x860
  46: NOTYPE	GLOBAL	__bss_start (0) 0x300020103c
  47: FUNC	GLOBAL	main (35) 0x30000007f0, rel: 0x7f0
  48: FUNC	GLOBAL	__libc_csu_fini (2) 0x30000008f0, rel: 0x8f0
  49: FUNC	GLOBAL	bar (44) 0x3000000828, rel: 0x828
  50: FUNC	GLOBAL	_fini (0) 0x30000008f4, rel: 0x8f4
  51: FUNC	GLOBAL	_start (0) 0x30000006e0, rel: 0x6e0
  52: FUNC	GLOBAL	_init (0) 0x3000000680, rel: 0x680
EOF

cat testfile.symtab.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebaztab

cat testfile.symtab.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazdbg

cat testfile.symtab_pl.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazdbg_pl

sed s/0x3000/0x4200/g testfile.symtab_pl.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazdbg_plr

cat testfile.dynsym.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazdyn

cat testfile.symtab.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazmdb

cat testfile.minsym.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazmin

cat testfile.minsym_pl.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazmin_pl

sed s/0x3000/0x4200/g testfile.minsym_pl.in \
  | testrun_compare ${abs_builddir}/dwflsyms -e testfilebazmin_plr

testrun_compare ${abs_builddir}/dwflsyms -e testfilebasmin <<\EOF
   0: NOTYPE	LOCAL	 (0) 0
   1: FUNC	LOCAL	foo (18) 0x400168, rel: 0x400168
   2: SECTION	LOCAL	 (0) 0x400120
   3: SECTION	LOCAL	 (0) 0x400144
   4: SECTION	LOCAL	 (0) 0x4001c0
   5: SECTION	LOCAL	 (0) 0x600258
   6: FUNC	GLOBAL	_start (21) 0x4001a8, rel: 0x4001a8
   7: FUNC	GLOBAL	main (33) 0x400144, rel: 0x400144
   8: FUNC	GLOBAL	bar (44) 0x40017a, rel: 0x40017a
EOF

exit 0
