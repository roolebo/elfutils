/* Enumerate the PC ranges covered by a DIE.
   Copyright (C) 2005, 2007, 2009 Red Hat, Inc.
   This file is part of Red Hat elfutils.

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

#include "libdwP.h"
#include <dwarf.h>
#include <assert.h>

/* Read up begin/end pair and increment read pointer.
    - If it's normal range record, set up `*beginp' and `*endp' and return 0.
    - If it's base address selection record, set up `*basep' and return 1.
    - If it's end of rangelist, don't set anything and return 2
    - If an error occurs, don't set anything and return -1.  */
internal_function int
__libdw_read_begin_end_pair_inc (Dwarf *dbg, int sec_index,
				 unsigned char **addrp, int width,
				 Dwarf_Addr *beginp, Dwarf_Addr *endp,
				 Dwarf_Addr *basep)
{
  Dwarf_Addr escape = (width == 8 ? (Elf64_Addr) -1
		       : (Elf64_Addr) (Elf32_Addr) -1);
  Dwarf_Addr begin;
  Dwarf_Addr end;

  unsigned char *addr = *addrp;
  bool begin_relocated = READ_AND_RELOCATE (__libdw_relocate_address, begin);
  bool end_relocated = READ_AND_RELOCATE (__libdw_relocate_address, end);
  *addrp = addr;

  /* Unrelocated escape for begin means base address selection.  */
  if (begin == escape && !begin_relocated)
    {
      if (unlikely (end == escape))
	{
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return -1;
	}

      if (basep != NULL)
	*basep = end;
      return 1;
    }

  /* Unrelocated pair of zeroes means end of range list.  */
  if (begin == 0 && end == 0 && !begin_relocated && !end_relocated)
    return 2;

  /* Don't check for begin_relocated == end_relocated.  Serve the data
     to the client even though it may be buggy.  */
  *beginp = begin;
  *endp = end;

  return 0;
}

ptrdiff_t
dwarf_ranges (Dwarf_Die *die, ptrdiff_t offset, Dwarf_Addr *basep,
	      Dwarf_Addr *startp, Dwarf_Addr *endp)
{
  if (die == NULL)
    return -1;

  if (offset == 0
      /* Usually there is a single contiguous range.  */
      && INTUSE(dwarf_highpc) (die, endp) == 0
      && INTUSE(dwarf_lowpc) (die, startp) == 0)
    /* A offset into .debug_ranges will never be 1, it must be at least a
       multiple of 4.  So we can return 1 as a special case value to mark
       there are no ranges to look for on the next call.  */
    return 1;

  if (offset == 1)
    return 0;

  /* We have to look for a noncontiguous range.  */

  const Elf_Data *d = die->cu->dbg->sectiondata[IDX_debug_ranges];
  if (d == NULL && offset != 0)
    {
      __libdw_seterrno (DWARF_E_NO_DEBUG_RANGES);
      return -1;
    }

  unsigned char *readp;
  unsigned char *readendp;
  if (offset == 0)
    {
      Dwarf_Attribute attr_mem;
      Dwarf_Attribute *attr = INTUSE(dwarf_attr) (die, DW_AT_ranges,
						  &attr_mem);
      if (attr == NULL)
	/* No PC attributes in this DIE at all, so an empty range list.  */
	return 0;

      Dwarf_Word start_offset;
      if ((readp = __libdw_formptr (attr, IDX_debug_ranges,
				    DWARF_E_NO_DEBUG_RANGES,
				    &readendp, &start_offset)) == NULL)
	return -1;

      offset = start_offset;
      assert ((Dwarf_Word) offset == start_offset);

      /* Fetch the CU's base address.  */
      Dwarf_Die cudie = CUDIE (attr->cu);

      /* Find the base address of the compilation unit.  It will
	 normally be specified by DW_AT_low_pc.  In DWARF-3 draft 4,
	 the base address could be overridden by DW_AT_entry_pc.  It's
	 been removed, but GCC emits DW_AT_entry_pc and not DW_AT_lowpc
	 for compilation units with discontinuous ranges.  */
      if (unlikely (INTUSE(dwarf_lowpc) (&cudie, basep) != 0)
	  && INTUSE(dwarf_formaddr) (INTUSE(dwarf_attr) (&cudie,
							 DW_AT_entry_pc,
							 &attr_mem),
				     basep) != 0)
	{
	  if (INTUSE(dwarf_errno) () == 0)
	    {
	    invalid:
	      __libdw_seterrno (DWARF_E_INVALID_DWARF);
	    }
	  return -1;
	}
    }
  else
    {
      if (__libdw_offset_in_section (die->cu->dbg,
				     IDX_debug_ranges, offset, 1))
	return -1l;

      readp = d->d_buf + offset;
      readendp = d->d_buf + d->d_size;
    }

 next:
  if (readendp - readp < die->cu->address_size * 2)
    goto invalid;

  Dwarf_Addr begin;
  Dwarf_Addr end;

  switch (__libdw_read_begin_end_pair_inc (die->cu->dbg, IDX_debug_ranges,
					   &readp, die->cu->address_size,
					   &begin, &end, basep))
    {
    case 0:
      break;
    case 1:
      goto next;
    case 2:
      return 0;
    default:
      return -1l;
    }

  /* We have an address range entry.  */
  *startp = *basep + begin;
  *endp = *basep + end;
  return readp - (unsigned char *) d->d_buf;
}
INTDEF (dwarf_ranges)
