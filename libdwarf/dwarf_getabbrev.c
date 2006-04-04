/* Internal abbrev list handling
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
#include <stdlib.h>

#include <libdwarfP.h>


Dwarf_Abbrev
__libdwarf_get_abbrev (dbg, cu, code, error)
     Dwarf_Debug dbg;
     Dwarf_CU_Info cu;
     Dwarf_Word code;
     Dwarf_Error *error;
{
  Dwarf_Abbrev ent;

  /* See whether the entry is already in the hash table.  */
  ent = Dwarf_Abbrev_Hash_find (&cu->abbrev_hash, code, NULL);
  if (ent != NULL)
    return ent;

  while (1)
    {
      Dwarf_Unsigned length;
      Dwarf_Unsigned attr_count;

      if (dwarf_get_abbrev (dbg, cu->last_abbrev_offset, &ent, &length,
			    &attr_count, error) != DW_DLV_OK)
	return NULL;

      if (length == 1)
	/* This is the end of the list.  */
	break;

      /* Update the offset to the next record.  */
      cu->last_abbrev_offset += length;

      /* Insert the new entry into the hashing table.  */
      if (unlikely (Dwarf_Abbrev_Hash_insert (&cu->abbrev_hash, ent->code, ent)
		    != 0))
	{
	  free (ent);
	  __libdwarf_error (dbg, error, DW_E_NOMEM);
	  return NULL;
	}

      /* Is this the code we are looking for?  */
      if (ent->code == code)
	/* Yes!  */
	return ent;
    }

  /* If we come here we haven't found anything.  */
  __libdwarf_error (dbg, error, DW_E_NO_ABBR);
  return NULL;
}
