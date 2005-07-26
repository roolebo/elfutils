## Makefile.am for libdwfl library subdirectory in elfutils.
##
## Process this file with automake to create Makefile.in
##
## Copyright (C) 2005 Red Hat, Inc.
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
DEFS = -D_GNU_SOURCE -DHAVE_CONFIG_H
if MUDFLAP
AM_CFLAGS = -fmudflap
else
AM_CFLAGS =
endif
AM_CFLAGS += -Wall -Werror -Wshadow -Wunused -Wformat=2 -Wextra -std=gnu99
INCLUDES = -I. -I$(srcdir) -I$(srcdir)/../libelf -I$(srcdir)/../libebl \
	   -I$(srcdir)/../libdw -I.. -I$(srcdir)/../lib
VERSION = 1

noinst_PROGRAMS = ptest test2

test2_SOURCES = test2.c loc2c.c

lib_LIBRARIES = libdwfl.a
if !MUDFLAP
noinst_LIBRARIES = libdwfl_pic.a
noinst_PROGRAMS += $(noinst_LIBRARIES:_pic.a=.so)
endif

euincludedir = ${includedir}/elfutils
euinclude_HEADERS = libdwfl.h

libdwfl_a_SOURCES = dwfl_begin.c dwfl_end.c dwfl_error.c \
		    dwfl_module.c dwfl_report_elf.c relocate.c \
		    dwfl_module_info.c  dwfl_getmodules.c \
		    dwfl_module_getdwarf.c dwfl_getdwarf.c \
		    argp-std.c find-debuginfo.c \
		    linux-kernel-modules.c linux-proc-maps.c \
		    dwfl_addrmodule.c dwfl_addrdwarf.c \
		    cu.c dwfl_module_nextcu.c dwfl_nextcu.c dwfl_cumodule.c \
		    dwfl_module_addrdie.c dwfl_addrdie.c \
		    lines.c dwfl_lineinfo.c dwfl_linemodule.c \
		    dwfl_module_getsrc.c dwfl_getsrc.c \
		    dwfl_module_getsrc_file.c \
		    elf-from-memory.c


if MUDFLAP
libdwfl = libdwfl.a $(libdw) $(libebl) $(libelf) $(libeu)
libdw = ../libdw/libdw.a
libelf = ../libelf/libelf.a
libmudflap = -lmudflap
else
libdwfl = libdwfl.so
libdw = ../libdw/libdw.so
libelf = ../libelf/libelf.so
endif
libebl = ../libebl/libebl.a
libeu = ../lib/libeu.a


if !MUDFLAP
libdwfl_pic_a_SOURCES =
am_libdwfl_pic_a_OBJECTS = $(libdwfl_a_SOURCES:.c=.os)

libdwfl_so_SOURCES =
libdwfl_LIBS = $(libeu) $(libdw) $(libebl) $(libelf)
libdwfl_so_LDADD = -ldl
libdwfl.so: libdwfl_pic.a $(srcdir)/libdwfl.map $(libdwfl_LIBS)
	$(CC) -shared -o $@ -Wl,--whole-archive,$<,--no-whole-archive \
	      -Wl,--version-script,$(srcdir)/libdwfl.map,--no-undefined \
	      -Wl,--soname,$@.$(VERSION),-z,defs \
	      $(libdwfl_LIBS) $(libdwfl_so_LDADD)
	if readelf -d $@ | fgrep -q TEXTREL; then exit 1; fi
	ln -fs $@ $@.$(VERSION)


%.os: %.c %.o
	if $(COMPILE) -c -o $@ -fpic -DPIC -DSHARED -MT $@ -MD -MP \
	   -MF "$(DEPDIR)/$*.Tpo" `test -f '$<' || echo '$(srcdir)/'`$<; \
	then cat "$(DEPDIR)/$*.Tpo" >> "$(DEPDIR)/$*.Po"; \
	     rm -f "$(DEPDIR)/$*.Tpo"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; \
	fi

install: install-am libdwfl.so
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL_PROGRAM) libdwfl.so $(DESTDIR)$(libdir)/libdwfl-$(PACKAGE_VERSION).so
	ln -fs libdwfl-$(PACKAGE_VERSION).so $(DESTDIR)$(libdir)/libdwfl.so.$(VERSION)
	ln -fs libdwfl.so.$(VERSION) $(DESTDIR)$(libdir)/libdwfl.so

uninstall: uninstall-am
	rm -f $(DESTDIR)$(libdir)/libdwfl-$(PACKAGE_VERSION).so
	rm -f $(DESTDIR)$(libdir)/libdwfl.so.$(VERSION)
	rm -f $(DESTDIR)$(libdir)/libdwfl.so
	rmdir --ignore-fail-on-non-empty $(DESTDIR)$(includedir)/elfutils
endif

noinst_HEADERS = libdwflP.h loc2c.h

EXTRA_DIST = libdwfl.map

CLEANFILES = $(am_libdwfl_pic_a_OBJECTS)

ptest_LDADD = $(libdwfl) $(libdw) $(libmudflap)
test2_LDADD = $(libdwfl) $(libdw) $(libmudflap)
