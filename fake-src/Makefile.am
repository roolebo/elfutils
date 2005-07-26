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
bin_SCRIPTS = readelf nm size strip elflint addr2line
EXTRA_DIST = readelf nm size strip elflint.in addr2line FULL
include_HEADERS = dwarf.h

elflint: $(srcdir)/elflint.in
	sed "s/@EUVERSION@/$(VERSION)/" $< > $@
