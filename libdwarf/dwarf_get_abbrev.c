/* Get abbreviation record.
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


int
dwarf_get_abbrev (dbg, offset, return_abbrev, length, attr_count, error)
     Dwarf_Debug dbg;
     Dwarf_Unsigned offset;
     Dwarf_Abbrev *return_abbrev;
     Dwarf_Unsigned *length;
     Dwarf_Unsigned *attr_count;
     Dwarf_Error *error;
{
  Dwarf_Abbrev ent;
  Dwarf_Small *abbrevp;
  Dwarf_Small *start_abbrevp;

  /* Address in memory.  */
  abbrevp = (Dwarf_Small *) dbg->sections[IDX_debug_abbrev].addr + offset;

  /* Remember where we started.  */
  start_abbrevp = abbrevp;

  if (*abbrevp != '\0')
    {
      /* 7.5.3 Abbreviations Tables

	 [...] Each declaration begins with an unsigned LEB128 number
	 representing the abbreviation code itself.  [...]  The
	 abbreviation code is followed by another unsigned LEB128
	 number that encodes the entry's tag.  [...]

	 [...] Following the tag encoding is a 1-byte value that
	 determines whether a debugging information entry using this
	 abbreviation has child entries or not. [...]

	 [...] Finally, the child encoding is followed by a series of
	 attribute specifications. Each attribute specification
	 consists of two parts. The first part is an unsigned LEB128
	 number representing the attribute's name. The second part is
	 an unsigned LEB128 number representing the attribute s form.  */
      Dwarf_Word abbrev_code;
      Dwarf_Word abbrev_tag;
      Dwarf_Word attr_name;
      Dwarf_Word attr_form;
      Dwarf_Unsigned attrcnt;

      /* XXX We have no tests for crossing the section boundary here.
	 We should compare with dbg->sections[IDX_debug_abbrev].size.  */
      get_uleb128 (abbrev_code, abbrevp);
      get_uleb128 (abbrev_tag, abbrevp);

      /* Get memory for the result.  */
      ent = (Dwarf_Abbrev) malloc (sizeof (struct Dwarf_Abbrev_s));
      if (ent == NULL)
	{
	  __libdwarf_error (dbg, error, DW_E_NOMEM);
	  return DW_DLV_ERROR;
	}

      ent->code = abbrev_code;
      ent->tag = abbrev_tag;
      ent->has_children = *abbrevp++ == DW_CHILDREN_yes;
      ent->attrp = abbrevp;
      ent->offset = offset;

      /* Skip over all the attributes.  */
      attrcnt = 0;
      do
	{
	  get_uleb128 (attr_name, abbrevp);
	  get_uleb128 (attr_form, abbrevp);
	}
      while (attr_name != 0 && attr_form != 0 && ++attrcnt);

      /* Number of attributes.  */
      *attr_count = ent->attrcnt = attrcnt;

      /* Store the actual abbreviation record.  */
      *return_abbrev = ent;
    }
  else
    /* Read over the NUL byte.  */
    ++abbrevp;

  /* Length of the entry.  */
  *length = abbrevp - start_abbrevp;

  /* If we come here we haven't found anything.  */
  return DW_DLV_OK;
}
