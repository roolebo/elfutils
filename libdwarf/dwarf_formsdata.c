/* Return signed constant represented by attribute.
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

#include <dwarf.h>

#include <libdwarfP.h>


int
dwarf_formsdata (attr, return_sval, error)
     Dwarf_Attribute attr;
     Dwarf_Signed *return_sval;
     Dwarf_Error *error;
{
  Dwarf_Signed u128;

  switch (attr->form)
    {
    case DW_FORM_data1:
      *return_sval = *attr->valp;
      break;

    case DW_FORM_data2:
      *return_sval = read_2ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_data4:
      *return_sval = read_4ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_data8:
      *return_sval = read_8ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_sdata:
      {
	Dwarf_Small *attrp = attr->valp;
	get_sleb128 (u128, attrp);
	*return_sval = u128;
      }
      break;

    case DW_FORM_udata:
      {
	Dwarf_Small *attrp = attr->valp;
	get_uleb128 (u128, attrp);
	*return_sval = u128;
      }
      break;

    default:
      __libdwarf_error (attr->cu->dbg, error, DW_E_NO_CONSTANT);
      return DW_DLV_ERROR;
    }

  return DW_DLV_OK;
}
