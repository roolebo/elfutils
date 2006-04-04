/* Find FDE for given address.
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
dwarf_get_fde_at_pc (fde_data, pc_of_interest, returned_fde, lopc, hipc, error)
     Dwarf_Fde *fde_data;
     Dwarf_Addr pc_of_interest;
     Dwarf_Fde *returned_fde;
     Dwarf_Addr *lopc;
     Dwarf_Addr *hipc;
     Dwarf_Error *error;
{
  Dwarf_Debug dbg = fde_data[0]->cie->dbg;
  int low = 0;
  int high = dbg->fde_cnt - 1;

  /* Since the FDEs are sorted by their addresses and since there can
     potentially be many FDEs we better use binary search.  */
  while (low <= high)
    {
      int curidx = (low + high) / 2;
      Dwarf_Fde cur = fde_data[curidx];

      if (pc_of_interest < cur->initial_location)
	high = curidx - 1;
      else if (likely (cur->initial_location + cur->address_range
		       <= pc_of_interest))
	low = curidx + 1;
      else
	{
	  *returned_fde = cur;
	  *lopc = cur->initial_location;
	  *hipc = cur->initial_location + cur->address_range - 1;
	  return DW_DLV_OK;
	}
    }

  return DW_DLV_NO_ENTRY;
}
