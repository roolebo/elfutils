## Process this file with automake to create Makefile.in
##
## Copyright (C) 2002-2011 Red Hat, Inc.
## This file is part of Red Hat elfutils.
##
## Red Hat elfutils is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by the
## Free Software Foundation; version 2 of the License.
##
## Red Hat elfutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with Red Hat elfutils; if not, write to the Free Software Foundation,
## Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.
##
## Red Hat elfutils is an included package of the Open Invention Network.
## An included package of the Open Invention Network is a package for which
## Open Invention Network licensees cross-license their patents.  No patent
## license is granted, either expressly or impliedly, by designation as an
## included package.  Should you wish to participate in the Open Invention
## Network licensing program, please visit www.openinventionnetwork.com
## <http://www.openinventionnetwork.com>.
##
include $(top_srcdir)/config/eu.am
INCLUDES += -I$(srcdir)/../libelf -I$(srcdir)/../libebl \
	    -I$(srcdir)/../libdw -I$(srcdir)/../libasm
AM_CFLAGS += -fpic -fdollars-in-identifiers
LEXCOMPILE = $(LEX) $(LFLAGS) $(AM_LFLAGS) -P$(<F:lex.l=)
LEX_OUTPUT_ROOT = lex.$(<F:lex.l=)
AM_YFLAGS = -p$(<F:parse.y=)

noinst_LIBRARIES = libcpu_i386.a libcpu_x86_64.a

libcpu_i386_a_SOURCES = i386_disasm.c i386_dis.h
libcpu_x86_64_a_SOURCES = x86_64_disasm.c x86_64_dis.h

i386_gendis_SOURCES = i386_gendis.c i386_lex.l i386_parse.y

i386_disasm.o: i386.mnemonics $(srcdir)/i386_dis.h
x86_64_disasm.o: x86_64.mnemonics $(srcdir)/x86_64_dis.h

%_defs: $(srcdir)/defs/i386
	m4 -D$* -DDISASSEMBLER $< > $@

if MAINTAINER_MODE
noinst_HEADERS = memory-access.h i386_parse.h i386_data.h

noinst_PROGRAMS = i386_gendis

$(srcdir)/%_dis.h: %_defs i386_gendis
	./i386_gendis $< > $@
endif

%.mnemonics: %_defs
	sed '1,/^%%/d;/^#/d;/^[[:space:]]*$$/d;s/[^:]*:\([^[:space:]]*\).*/MNE(\1)/;s/{[^}]*}//g;/INVALID/d' \
	  $< | sort -u > $@

i386_lex_no_Werror = yes

libeu = ../lib/libeu.a

i386_lex_CFLAGS = -Wno-unused-label -Wno-unused-function -Wno-sign-compare
i386_parse.o: i386_parse.c i386.mnemonics
i386_parse_CFLAGS = -DNMNES="`wc -l < i386.mnemonics`"
i386_lex.o: i386_parse.h
i386_gendis_LDADD = $(libeu) -lm $(libmudflap)

i386_parse.h: i386_parse.c ;

EXTRA_DIST = defs/i386

CLEANFILES += $(foreach P,i386 x86_64,$P_defs $P.mnemonics $P_dis.h)
