## Process this file with automake to produce Makefile.in -*-Makefile-*-
## Configure input file for elfutils.
##
## Copyright (C) 2004, 2005 Red Hat, Inc.
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
EXTRA_DIST = elfutils.spec.in

$(srcdir)/elfutils.spec.in: $(top_srcdir)/NEWS
	@tmpname=$$(mktemp $${TMPDIR:-/tmp}/elfutils.XXXXXX); \
	date +'* %a %b %e %Y' | tr '[\n]' '[ ]' > $$tmpname; \
	getent passwd "$$(whoami)" | \
	  awk 'BEGIN {FS=":"} { printf $$5; exit 0}' >> $$tmpname; \
	echo -n " <$(whoami)@redhat.com> " >> $$tmpname; \
	sed 's/Version \(.*\):$$/\1-1/;q' $(top_srcdir)/NEWS >> $$tmpname; \
	sed '2,/^Version /p;d' $(top_srcdir)/NEWS | \
	  head -n -1 | \
	  awk '{ if ($$0 == "") { if (line != "") { printf "- "; fflush(); system("echo \"" line "\" | fold -w 70"); line=""; } } else { line=line $$0; }} END { if (line != "") { printf "- "; system("echo \"" line "\" | fold -w 70")}}' >> $$tmpname; \
	echo >> $$tmpname; \
	sed "/^%changelog/r $$tmpname" $@ > $@.new; \
	rm -f $$tmpname; \
	mv -f $@.new $@
	-@if [ -d $(srcdir)/CVS ]; then \
	  cd $(srcdir); \
	  cvs ci -m "Added changelog." $(@F); \
	fi
