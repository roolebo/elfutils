#! /bin/sh
# Copyright (C) 2011 Red Hat, Inc.
# This file is part of Red Hat elfutils.
#
# Red Hat elfutils is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# Red Hat elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with Red Hat elfutils; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.
#
# Red Hat elfutils is an included package of the Open Invention Network.
# An included package of the Open Invention Network is a package for which
# Open Invention Network licensees cross-license their patents.  No patent
# license is granted, either expressly or impliedly, by designation as an
# included package.  Should you wish to participate in the Open Invention
# Network licensing program, please visit www.openinventionnetwork.com
# <http://www.openinventionnetwork.com>.

. $srcdir/test-subr.sh

# This will produce "testfile-alldts" file
./alldts

testrun_compare ../src/readelf -d testfile-alldts <<\EOF

Dynamic segment contains 66 entries:
 Addr: 0x000001a0  Offset: 0x000078  Link to section: [ 0] ''
  Type              Value
  NULL              
  NEEDED            Shared library: [(null)]
  PLTRELSZ          3735928559 (bytes)
  PLTGOT            0xdeadbeef
  HASH              0xdeadbeef
  STRTAB            0xdeadbeef
  SYMTAB            0xdeadbeef
  RELA              0xdeadbeef
  RELASZ            3735928559 (bytes)
  RELAENT           3735928559 (bytes)
  STRSZ             3735928559 (bytes)
  SYMENT            3735928559 (bytes)
  INIT              0xdeadbeef
  FINI              0xdeadbeef
  SONAME            Library soname: [(null)]
  RPATH             Library rpath: [(null)]
  SYMBOLIC          0xdeadbeef
  REL               0xdeadbeef
  RELSZ             3735928559 (bytes)
  RELENT            3735928559 (bytes)
  PLTREL            ???
  DEBUG             
  TEXTREL           
  JMPREL            0xdeadbeef
  BIND_NOW          
  INIT_ARRAY        0xdeadbeef
  FINI_ARRAY        0xdeadbeef
  INIT_ARRAYSZ      3735928559 (bytes)
  FINI_ARRAYSZ      3735928559 (bytes)
  RUNPATH           Library runpath: [(null)]
  FLAGS             ORIGIN SYMBOLIC TEXTREL BIND_NOW 0xdeadbee0
  PREINIT_ARRAY     0xdeadbeef
  PREINIT_ARRAY     0xdeadbeef
  PREINIT_ARRAYSZ   0xdeadbeef
  VERSYM            0xdeadbeef
  GNU_PRELINKED     0xdeadbeef
  GNU_CONFLICTSZ    3735928559 (bytes)
  GNU_LIBLISTSZ     3735928559 (bytes)
  CHECKSUM          0xdeadbeef
  PLTPADSZ          3735928559 (bytes)
  MOVEENT           3735928559 (bytes)
  MOVESZ            3735928559 (bytes)
  FEATURE_1         PARINIT CONFEXP 0xdeadbeec
  POSFLAG_1         LAZYLOAD GROUPPERM 0xdeadbeec
  SYMINSZ           3735928559 (bytes)
  SYMINENT          3735928559 (bytes)
  GNU_HASH          0xdeadbeef
  TLSDESC_PLT       0xdeadbeef
  TLSDESC_GOT       0xdeadbeef
  GNU_CONFLICT      0xdeadbeef
  GNU_LIBLIST       0xdeadbeef
  CONFIG            0xdeadbeef
  DEPAUDIT          0xdeadbeef
  AUDIT             0xdeadbeef
  PLTPAD            0xdeadbeef
  MOVETAB           0xdeadbeef
  SYMINFO           0xdeadbeef
  RELACOUNT         3735928559
  RELCOUNT          3735928559
  FLAGS_1           NOW GLOBAL GROUP NODELETE INITFIRST NOOPEN ORIGIN TRANS INTERPOSE NODEFLIB NODUMP CONFALT DISPRELDNE DISPRELPND 0xdeac0000
  VERDEF            0xdeadbeef
  VERDEFNUM         3735928559
  VERNEED           0xdeadbeef
  VERNEEDNUM        3735928559
  AUXILIARY         0xdeadbeef
  FILTER            0xdeadbeef
EOF

rm -f testfile-alldts

exit 0
