## Process this file with automake to create Makefile.in
##
## Copyright (C) 1996-2001, 2002, 2004, 2005 Red Hat, Inc.
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
DEFS = -D_GNU_SOURCE -DHAVE_CONFIG_H
if MUDFLAP
AM_CFLAGS = -fmudflap
else
AM_CFLAGS =
endif
AM_CFLAGS += -fpic -Wall -Wshadow -Werror -Wunused -Wextra $($(*F)_CFLAGS)
INCLUDES = -I$(srcdir)/../libelf -I..

noinst_LIBRARIES = libeu.a

libeu_a_SOURCES = xstrdup.c xstrndup.c xmalloc.c next_prime.c \
		  crc32.c crc32_file.c

noinst_HEADERS = fixedsizehash.h system.h dynamicsizehash.h list.h
EXTRA_DIST = dynamicsizehash.c

if !GPROF
xmalloc_CFLAGS = -ffunction-sections
endif
