/* Return die at given offset.
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
   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

   In addition, as a special exception, Red Hat, Inc. gives You the
   additional right to link the code of Red Hat elfutils with code licensed
   under any Open Source Initiative certified open source license
   (http://www.opensource.org/licenses/index.php) which requires the
   distribution of source code with any binary distribution and to
   distribute linked combinations of the two.  Non-GPL Code permitted under
   this exception must only link to the code of Red Hat elfutils through
   those well defined interfaces identified in the file named EXCEPTION
   found in the source code files (the "Approved Interfaces").  The files
   of Non-GPL Code may instantiate templates or use macros or inline
   functions from the Approved Interfaces without causing the resulting
   work to be covered by the GNU General Public License.  Only Red Hat,
   Inc. may make changes or additions to the list of Approved Interfaces.
   Red Hat's grant of this exception is conditioned upon your not adding
   any new exceptions.  If you wish to add a new Approved Interface or
   exception, please contact Red Hat.  You must obey the GNU General Public
   License in all respects for all of the Red Hat elfutils code and other
   code used in conjunction with Red Hat elfutils except the Non-GPL Code
   covered by this exception.  If you modify this file, you may extend this
   exception to your version of the file, but you are not obligated to do
   so.  If you do not wish to provide this exception without modification,
   you must delete this exception statement from your version and license
   this file solely under the GPL without exception.

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


/* XXX This function will have to be optimized.  The search is too linear
   to be performed too often -> O(n²).  */
static Dwarf_CU_Info
find_cu (Dwarf_Debug dbg, Dwarf_Off offset, Dwarf_Error *error)
{
  Dwarf_CU_Info cu;
  Dwarf_Word cu_offset;

  /* Search in the CUs already known.  */
  for (cu = dbg->cu_list; cu != NULL; cu = cu->next)
    if (cu->offset <= offset
	&& cu->offset + 2 * cu->offset_size - 4 + cu->length > offset)
      return cu;

  /* The CU is not yet loaded.  Do this now.  */
  if (dbg->cu_list_tail == NULL)
    cu_offset = 0;
  else
    cu_offset = (dbg->cu_list_tail->offset
		 + 2 * dbg->cu_list_tail->offset_size - 4
		 + dbg->cu_list_tail->length);

  while (1)
    {
      /* Get next CU and add it to the end of the list.  */
      if (__libdwarf_get_cu_at_offset (dbg, cu_offset, &cu, error)
	  != DW_DLV_OK)
	return NULL;

      /* Offset of next CU.  */
      cu_offset += 2 * cu->offset_size - 4 + cu->length;

      /* If this the CU we are looking for?  */
      if (offset < cu_offset)
	return cu;
    }
}


int
dwarf_offdie (dbg, offset, return_die, error)
     Dwarf_Debug dbg;
     Dwarf_Off offset;
     Dwarf_Die *return_die;
     Dwarf_Error *error;
{
  Dwarf_CU_Info cu;
  Dwarf_Die new_die;
  Dwarf_Small *die_addr;
  Dwarf_Word u128;

  if (offset >= dbg->sections[IDX_debug_info].size)
    {
      /* Completely out of bounds.  */
      __libdwarf_error (dbg, error, DW_E_INVALID_OFFSET);
      return DW_DLV_ERROR;
    }

  /* Find the compile unit this address belongs to.  */
  cu = find_cu (dbg, offset, error);
  if (cu == NULL)
    return DW_DLV_ERROR;

  /* Creata a new die.  */
  new_die = (Dwarf_Die) malloc (sizeof (struct Dwarf_Die_s));
  if (new_die == NULL)
    {
      __libdwarf_error (dbg, error, DW_E_NOMEM);
      return DW_DLV_ERROR;
    }

#ifdef DWARF_DEBUG
  new_die->memtag = DW_DLA_DIE;
#endif

  /* Remember the address.  */
  die_addr = (Dwarf_Small *) dbg->sections[IDX_debug_info].addr + offset;
  new_die->addr = die_addr;

  /* And the compile unit.  */
  new_die->cu = cu;

  /* 7.5.2  Debugging Information Entry

     Each debugging information entry begins with an unsigned LEB128
     number containing the abbreviation code for the entry.  */
  get_uleb128 (u128, die_addr);

  /* Find the abbreviation.  */
  new_die->abbrev = __libdwarf_get_abbrev (dbg, cu, u128, error);
  if (new_die->abbrev == NULL)
    {
      free (new_die);
      return DW_DLV_ERROR;
    }

  *return_die = new_die;
  return DW_DLV_OK;
}
