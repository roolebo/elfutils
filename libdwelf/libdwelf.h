/* Interfaces for libdwelf. DWARF ELF Low-level Functions.
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifndef _LIBDWELF_H
#define _LIBDWELF_H	1

#include "libdw.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DWARF ELF Low-level Functions (dwelf).
   Functions starting with dwelf_elf will take a (libelf) Elf object as
   first argument and might set elf_errno on error.  Functions starting
   with dwelf_dwarf will take a (libdw) Dwarf object as first argument
   and might set dwarf_errno on error.  */

/* Returns the name and the CRC32 of the separate debug file from the
   .gnu_debuglink section if found in the ELF.  Return NULL if the ELF
   file didn't have a .gnu_debuglink section, had malformed data in the
   section or some other error occured.  */
extern const char *dwelf_elf_gnu_debuglink (Elf *elf, GElf_Word *crc);

#ifdef __cplusplus
}
#endif

#endif	/* libdwelf.h */
