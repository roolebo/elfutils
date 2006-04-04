/* Get information about the function range.
   Copyright (C) 2001, 2002 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

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

#include "libdwarfP.h"


int
dwarf_get_fde_range (fde, low_pc, func_length, fde_bytes, fde_byte_length,
		     cie_offset, cie_index, fde_offset, error)
     Dwarf_Fde fde;
     Dwarf_Addr *low_pc;
     Dwarf_Unsigned *func_length;
     Dwarf_Ptr *fde_bytes;
     Dwarf_Unsigned *fde_byte_length;
     Dwarf_Off *cie_offset;
     Dwarf_Signed *cie_index;
     Dwarf_Off *fde_offset;
     Dwarf_Error *error;
{
  *low_pc = fde->initial_location;
  *func_length = fde->address_range;
  *fde_bytes = fde->fde_bytes;
  *fde_byte_length = fde->fde_byte_length;
  *cie_offset = fde->cie->offset;
  *cie_index = fde->cie->index;
  *fde_offset = fde->offset;

  return DW_DLV_OK;
}
