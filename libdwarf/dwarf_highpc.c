/* Return high program counter value associated with die.
   Copyright (C) 2000, 2001, 2002 Red Hat, Inc.
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
dwarf_highpc (die, return_highpc, error)
     Dwarf_Die die;
     Dwarf_Addr *return_highpc;
     Dwarf_Error *error;
{
  Dwarf_Debug dbg = die->cu->dbg;
  Dwarf_Small *die_addr;
  Dwarf_Word u128;
  Dwarf_Abbrev abbrev;
  Dwarf_Small *attrp;

  /* Address of the given die.  */
  die_addr = die->addr;

  /* Get abbrev code.  */
  get_uleb128 (u128, die_addr);
  /*  And get the abbreviation itself.  */
  abbrev = __libdwarf_get_abbrev (dbg, die->cu, u128, error);
  if (abbrev == NULL)
    return DW_DLV_ERROR;

  /* This is where the attributes start.  */
  attrp = abbrev->attrp;

  /* Search the name attribute.  */
  while (1)
    {
      Dwarf_Word attr_name;
      Dwarf_Word attr_form;

      /* Are we still in bounds?  */
      if (unlikely  (attrp
		     >= ((Dwarf_Small *) dbg->sections[IDX_debug_abbrev].addr
			 + dbg->sections[IDX_debug_abbrev].size)))
	{
	  __libdwarf_error (dbg, error, DW_E_INVALID_DWARF);
	  return DW_DLV_ERROR;
	}

      /* Get attribute name and form.

	 XXX We don't check whether this reads beyond the end of the
	 section.  */
      get_uleb128 (attr_name, attrp);
      get_uleb128 (attr_form, attrp);

      /* We can stop if we found the attribute with value zero.  */
      if (attr_name == 0 && attr_form == 0)
	break;

      /* Is this the name attribute?  */
      if (unlikely (attr_name == DW_AT_high_pc))
	{
	  if (unlikely (attr_form != DW_FORM_addr))
	    {
	      __libdwarf_error (dbg, error, DW_E_INVALID_DWARF);
	      return DW_DLV_ERROR;
	    }

	  if (die->cu->address_size == 4)
	    *return_highpc = read_4ubyte_unaligned (dbg, die_addr);
	  else
	    *return_highpc = read_8ubyte_unaligned (dbg, die_addr);

	  return DW_DLV_OK;
	}

      /* Skip over the rest of this attribute (if there is any).  */
      if (attr_form != 0)
	{
	  size_t len;

	  if (unlikely (__libdwarf_form_val_len (dbg, die->cu, attr_form,
						 die_addr, &len, error)
			!= DW_DLV_OK))
	    return DW_DLV_ERROR;

	  die_addr += len;
	}
    }

  /* No such attribute present.  */
  return DW_DLV_NO_ENTRY;
}
