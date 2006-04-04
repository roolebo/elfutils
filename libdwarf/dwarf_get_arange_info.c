/* Return start, length, and CU DIE offset of range.
   Copyright (C) 2000, 2002 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libdwarfP.h>


int
dwarf_get_arange_info (arange, start, length, cu_die_offset, error)
     Dwarf_Arange arange;
     Dwarf_Addr *start;
     Dwarf_Unsigned *length;
     Dwarf_Off *cu_die_offset;
     Dwarf_Error *error;
{
  Dwarf_Small *cu_header;
  unsigned int offset_size;

  *start = arange->address;
  *length = arange->length;

  /* Determine the size of the CU header.  */
  cu_header =
    ((Dwarf_Small *) arange->info->dbg->sections[IDX_debug_info].addr
     + arange->info->offset);
  if (read_4ubyte_unaligned_noncvt (cu_header) == 0xffffffff)
    offset_size = 8;
  else
    offset_size = 4;

  *cu_die_offset = arange->info->offset + 3 * offset_size - 4 + 3;

  return DW_DLV_OK;
}
