## Process this file with automake to create Makefile.in
##
## Copyright (C) 1996-2002, 2003, 2004, 2005 Red Hat, Inc.
##
## This program is Open Source software; you can redistribute it and/or
## modify it under the terms of the Open Software License version 1.0 as
## published by the Open Source Initiative.
##
## You should have received a copy of the Open Software License along
## with this program; if not, you may obtain a copy of the Open Software
## License version 1.0 from http://www.opensource.org/licenses/osl.php or
## by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
## 3001 King Ranch Road, Ukiah, CA 95482.
##
DEFS = -DHAVE_CONFIG_H -D_GNU_SOURCE
if MUDFLAP
AM_CFLAGS = -Wall -Werror -Wextra -std=gnu99 -fmudflap\
	    $(if $($(*F)_no_Wformat),-Wno-format,-Wformat=2)
AM_LDFLAGS = -Wl,-rpath,\$$ORIGIN/../libebl
else
AM_CFLAGS = -Wall -Werror -Wextra -std=gnu99 \
	    $(if $($(*F)_no_Wformat),-Wno-format,-Wformat=2)
AM_LDFLAGS = -Wl,-rpath,\$$ORIGIN/../libasm:\$$ORIGIN/../libdw:\$$ORIGIN/../libebl:\$$ORIGIN/../libelf
endif
INCLUDES = -I$(top_srcdir)/libasm -I$(top_srcdir)/libdw \
	   -I$(top_srcdir)/libdwfl \
	   -I$(top_srcdir)/libebl -I$(top_srcdir)/libelf \
	   -I$(top_srcdir)/lib -I..

noinst_PROGRAMS = arextract arsymtest newfile saridx scnnames sectiondump \
		  showptable update1 update2 update3 update4 test-nlist \
		  show-die-info get-files get-lines get-pubnames \
		  get-aranges allfcts line2addr addrscopes funcscopes \
		  show-abbrev hash asm-tst1 asm-tst2 asm-tst3 \
		  asm-tst4 asm-tst5 asm-tst6 asm-tst7 asm-tst8 asm-tst9 \
		  msg_tst newscn ecp dwflmodtest
# get-ciefde

TESTS = run-arextract.sh run-arsymtest.sh newfile test-nlist \
	update1 update2 update3 update4 \
	run-show-die-info.sh run-get-files.sh run-get-lines.sh \
	run-get-pubnames.sh run-get-aranges.sh run-allfcts.sh \
	run-show-abbrev.sh run-line2addr.sh hash asm-tst1 asm-tst2 \
	asm-tst3 asm-tst4 asm-tst5 asm-tst6 asm-tst7 asm-tst8 asm-tst9 \
	msg_tst newscn run-strip-test.sh run-strip-test2.sh \
	run-strip-test3.sh run-strip-test4.sh run-strip-test5.sh \
	run-strip-test6.sh run-ecp-test.sh run-ecp-test2.sh \
	run-elflint-test.sh run-elflint-self.sh run-ranlib-test.sh \
	run-ranlib-test2.sh run-ranlib-test3.sh run-ranlib-test4.sh \
	run-addrscopes.sh run-strings-test.sh run-funcscopes.sh
# run-show-ciefde.sh

EXTRA_DIST = run-arextract.sh run-arsymtest.sh \
	     run-show-die-info.sh run-get-files.sh run-get-lines.sh \
	     run-get-pubnames.sh run-get-aranges.sh \
	     run-show-ciefde.sh run-show-abbrev.sh run-strip-test.sh \
	     run-strip-test2.sh run-ecp-test.sh run-ecp-test2.sh \
	     testfile.bz2 testfile2.bz2 testfile3.bz2 testfile4.bz2 \
	     testfile5.bz2 testfile6.bz2 testfile7.bz2 testfile8.bz2 \
	     testfile9.bz2 testfile10.bz2 testfile11.bz2 testfile12.bz2 \
	     testfile13.bz2 run-strip-test3.sh run-allfcts.sh \
	     run-line2addr.sh run-elflint-test.sh testfile14.bz2 \
	     run-strip-test4.sh run-strip-test5.sh run-strip-test6.sh \
	     run-elflint-self.sh run-ranlib-test.sh run-ranlib-test2.sh \
	     run-ranlib-test3.sh run-ranlib-test4.sh \
	     run-addrscopes.sh run-strings-test.sh run-funcscopes.sh \
	     testfile15.bz2 testfile15.debug.bz2 \
	     testfile16.bz2 testfile16.debug.bz2 \
	     testfile17.bz2 testfile17.debug.bz2 \
	     testfile18.bz2 testfile19.bz2 testfile19.index.bz2 \
	     testfile20.bz2 testfile20.index.bz2 \
	     testfile21.bz2 testfile21.index.bz2 \
	     testfile22.bz2 testfile23.bz2 testfile24.bz2 testfile25.bz2

if MUDFLAP
static_build=yes
libmudflap = -lmudflap
endif

if BUILD_STATIC
libdw = ../libdw/libdw.a $(libelf) $(libebl) -ldl
libelf = ../libelf/libelf.a
libasm = ../libasm/libasm.a
else
libdw = ../libdw/libdw.so
libelf = ../libelf/libelf.so
libasm = ../libasm/libasm.so
endif
libebl = ../libebl/libebl.a

arextract_LDADD = $(libelf) $(libmudflap)
arsymtest_LDADD = $(libelf) $(libmudflap)
newfile_LDADD = $(libelf) $(libmudflap)
saridx_LDADD = $(libelf) $(libmudflap)
scnnames_LDADD = $(libelf) $(libmudflap)
sectiondump_LDADD = $(libelf) $(libmudflap)
showptable_LDADD = $(libelf) $(libmudflap)
hash_LDADD = $(libelf) $(libmudflap)
test_nlist_LDADD = $(libelf) $(libmudflap)
msg_tst_LDADD = $(libelf) $(libmudflap)
newscn_LDADD = $(libelf) $(libmudflap)
ecp_LDADD = $(libelf) $(libmudflap)
update1_LDADD = $(libelf) $(libmudflap)
update2_LDADD = $(libelf) $(libmudflap)
update3_LDADD = $(libebl) $(libelf) $(libmudflap)
update4_LDADD = $(libebl) $(libelf) $(libmudflap)
show_die_info_LDADD = $(libdw) $(libelf) $(libmudflap)
get_pubnames_LDADD = $(libdw) $(libelf) $(libmudflap)
show_abbrev_LDADD = $(libdw) $(libelf) $(libmudflap)
get_lines_LDADD = $(libdw) $(libelf) $(libmudflap)
get_files_LDADD = $(libdw) $(libelf) $(libmudflap)
get_aranges_LDADD = $(libdw) $(libelf) $(libmudflap)
allfcts_LDADD = $(libdw) $(libelf) $(libmudflap)
line2addr_no_Wformat = yes
line2addr_LDADD = $(libdw) $(libmudflap)
addrscopes_LDADD = $(libdw) $(libmudflap)
funcscopes_LDADD = $(libdw) $(libmudflap)
#show_ciefde_LDADD = ../libdwarf/libdwarf.so $(libelf) $(libmudflap)
asm_tst1_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst2_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst3_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst4_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst5_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst6_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst7_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst8_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst9_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
dwflmodtest_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl

CLEANFILES = xxx *.gcno *.gcda *gconv
