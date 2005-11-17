## Process this file with automake to create Makefile.in
## Configure input file for elfutils.
##
## Copyright (C) 1996-2002, 2003, 2004, 2005 Red Hat, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, version 2.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software Foundation,
## Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##
ACLOCAL_AMFLAGS = -I m4

mini_SUBDIRS = config m4 lib libelf libelf-po
# Add doc back when we have some real content.
all_SUBDIRS = libebl libdwfl libdw libcpu libasm backends src po tests
SUBDIRS = $(mini_SUBDIRS) $(all_SUBDIRS)

EXTRA_DIST = elfutils.spec GPG-KEY NOTES COPYING.GPL

distcheck-hook:
	chmod -R u+w $(distdir)

rpm: dist
	rpmbuild -ts --sign elfutils-@PACKAGE_VERSION@.tar.gz

# Tell version 3.79 and up of GNU make to not build goals in this
# directory in parallel.
.NOTPARALLEL:
