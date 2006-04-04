/* Find matching range for address.
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
dwarf_get_arange (aranges, arange_count, address, return_arange, error)
     Dwarf_Arange *aranges;
     Dwarf_Unsigned arange_count;
     Dwarf_Addr address;
     Dwarf_Arange *return_arange;
     Dwarf_Error *error;
{
  Dwarf_Unsigned cnt;

  for (cnt = 0; cnt < arange_count; ++cnt)
    if (aranges[cnt]->address <= address
	&& address < aranges[cnt]->address + aranges[cnt]->length)
      {
	*return_arange = aranges[cnt];
	return DW_DLV_OK;
      }

  return DW_DLV_NO_ENTRY;
}
