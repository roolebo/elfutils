/* Find entry breakpoint locations for a function.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "libdwP.h"
#include <dwarf.h>
#include <stdlib.h>


int
dwarf_entry_breakpoints (die, bkpts)
     Dwarf_Die *die;
     Dwarf_Addr **bkpts;
{
  int nbkpts = 0;
  *bkpts = NULL;

  /* Add one breakpoint location to the result vector.  */
  inline int add_bkpt (Dwarf_Addr pc)
    {
      Dwarf_Addr *newlist = realloc (*bkpts, ++nbkpts * sizeof newlist[0]);
      if (newlist == NULL)
	{
	  free (*bkpts);
	  *bkpts = NULL;
	  __libdw_seterrno (DWARF_E_NOMEM);
	  return -1;
	}
      newlist[nbkpts - 1] = pc;
      *bkpts = newlist;
      return nbkpts;
    }

  /* Fallback result, break at the entrypc/lowpc value.  */
  inline int entrypc_bkpt (void)
    {
      Dwarf_Addr pc;
      return INTUSE(dwarf_entrypc) (die, &pc) < 0 ? -1 : add_bkpt (pc);
    }

  /* Fetch the CU's line records to look for this DIE's addresses.  */
  Dwarf_Die cudie =
    {
      .cu = die->cu,
      .addr = ((char *) die->cu->dbg->sectiondata[IDX_debug_info]->d_buf
	       + die->cu->start + 3 * die->cu->offset_size - 4 + 3),
    };
  Dwarf_Lines *lines;
  size_t nlines;
  if (INTUSE(dwarf_getsrclines) (&cudie, &lines, &nlines) < 0)
    {
      int error = INTUSE (dwarf_errno) ();
      if (error == DWARF_E_NO_DEBUG_LINE)
	return entrypc_bkpt ();
      __libdw_seterrno (error);
      return -1;
    }

  /* Search a contiguous PC range for prologue-end markers.
     If DWARF, look for proper markers.
     Failing that, if ADHOC, look for the ad hoc convention.  */
  inline int search_range (Dwarf_Addr low, Dwarf_Addr high,
			   bool dwarf, bool adhoc)
    {
      size_t l = 0, u = nlines;
      while (l < u)
	{
	  size_t idx = (l + u) / 2;
	  if (low < lines->info[idx].addr)
	    u = idx;
	  else
	    {
	      l = idx;
	      if (low > lines->info[idx].addr)
		{
		  if (lines->info[idx].addr < high)
		    break;
		  ++l;
		}
	      else
		break;
	    }
	}
      if (l < u)
	{
	  if (dwarf)
	    while (l < u && lines->info[l].addr < high)
	      if (lines->info[l].prologue_end
		  && add_bkpt (lines->info[l].addr) < 0)
		return -1;
	  if (adhoc && nbkpts == 0
	      && l + 1 < nlines
	      && lines->info[l + 1].line == lines->info[l].line
	      && lines->info[l + 1].file == lines->info[l].file
	      && lines->info[l + 1].column == lines->info[l].column)
	    return add_bkpt (lines->info[l + 1].addr);
	  return nbkpts;
	}
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  /* Most often there is a single contiguous PC range for the DIE.  */
  Dwarf_Addr lowpc;
  Dwarf_Addr highpc;
  if (INTUSE(dwarf_lowpc) (die, &lowpc) == 0)
    return (INTUSE(dwarf_highpc) (die, &highpc)
	    ?: search_range (lowpc, highpc, true, true));


  /* We have to look for a noncontiguous range.  */
  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = INTUSE(dwarf_attr) (die, DW_AT_ranges, &attr_mem);
  if (attr == NULL)
    return -1;

  /* Must have the form data4 or data8 which act as an offset.  */
  Dwarf_Word offset;
  if (INTUSE(dwarf_formudata) (attr, &offset) != 0)
    return -1;

  const Elf_Data *d = die->cu->dbg->sectiondata[IDX_debug_ranges];
  if (d == NULL)
    {
      __libdw_seterrno (DWARF_E_NO_DEBUG_RANGES);
      return -1;
    }

  /* Fetch the CU's base address.  */
  Dwarf_Addr base;
  if (INTUSE(dwarf_lowpc) (&cudie, &base) != 0)
    return -1;

  /* Search each contiguous address range for DWARF prologue_end markers.  */
  unsigned char *readp = d->d_buf + offset;
  Dwarf_Addr begin;
  Dwarf_Addr end;
  lowpc = highpc = (Dwarf_Addr) -1l;
  do
    {
    next:
      if ((unsigned char *) d->d_buf + d->d_size - readp
	  < die->cu->address_size * 2)
	{
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  free (*bkpts);
	  *bkpts = NULL;
	  return -1;
	}

      if (die->cu->address_size == 8)
	{
	  begin = read_8ubyte_unaligned_inc (die->cu->dbg, readp);
	  end = read_8ubyte_unaligned_inc (die->cu->dbg, readp);
	}
      else
	{
	  begin = (Dwarf_Sword) read_4sbyte_unaligned_inc (die->cu->dbg,
							   readp);
	  end = read_4ubyte_unaligned_inc (die->cu->dbg, readp);
	}

      if (begin == (Dwarf_Addr) -1l) /* Base address entry.  */
	{
	  base = end;
	  goto next;
	}

      if (begin == 0 && end == 0) /* End of list entry.  */
	break;

      /* We have an address range entry.  */
      if (begin < lowpc)
	{
	  lowpc = begin;
	  highpc = end;
	}
    }
  while (search_range (begin, end, true, false) >= 0);

  /* If we didn't find any proper DWARF markers, then look in the
     lowest-addressed range for an ad hoc marker.  Failing that,
     fall back to just using the entrypc value.  */
  return (nbkpts
	  ?: (lowpc == (Dwarf_Addr) -1l ? 0
	      : search_range (lowpc, highpc, false, true))
	  ?: entrypc_bkpt ());
}
