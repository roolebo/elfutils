## Process this file with automake to create Makefile.in
## Configure input file for elfutils.
##
## Copyright (C) 1996-2001, 2002, 2005 Red Hat, Inc.
##
## This program is Open Source software; you can redistribute it and/or
## modify it under the terms of the Open Software License version 1.0 as
## published by the Open Source Initiative.
##
## You should have received a copy of the Open Software License along
## with this program; if not, you may obtain a copy of the Open Software
## License version 1.0 from http://www.opensource.org/licenses/osl.php or
## by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
## 3001 King Ranch Road, Ukiah, CA 95482.   */
##
EXTRA_DIST = elfutils.sgml

CLEANFILES = elfutils.dvi

# We need only a few special rules to generate the various output formats
# from the SGML sources.
.PHONY: dvi pdf html
pdf: $(srcdir)elfutils.pdf
dvi: $(srcdir)elfutils.dvi

$(srcdir)%.dvi: %.sgml
	db2dvi $^
$(srcdir)%.pdf: %.sgml
	db2pdf $^
