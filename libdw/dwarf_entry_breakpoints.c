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
	  if (lines->info[idx].addr < low)
	    l = idx + 1;
	  else if (lines->info[idx].addr > low)
	    u = idx;
	  else if (lines->info[idx].end_sequence)
	    l = idx + 1;
	  else
	    {
	      l = idx;
	      break;
	    }
	}
      if (l < u)
	{
	  if (dwarf)
	    for (size_t i = l; i < u && lines->info[i].addr < high; ++i)
	      if (lines->info[i].prologue_end
		  && add_bkpt (lines->info[i].addr) < 0)
		return -1;
	  if (adhoc && nbkpts == 0)
	    while (++l < nlines && lines->info[l].addr < high)
	      if (!lines->info[l].end_sequence)
		return add_bkpt (lines->info[l].addr);
	  return nbkpts;
	}
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  /* Search each contiguous address range for DWARF prologue_end markers.  */

  Dwarf_Addr base;
  Dwarf_Addr begin;
  Dwarf_Addr end;
  ptrdiff_t offset = INTUSE(dwarf_ranges) (die, 0, &base, &begin, &end);
  if (offset < 0)
    return -1;

  /* Most often there is a single contiguous PC range for the DIE.  */
  if (offset == 1)
    return search_range (begin, end, true, true) ?: entrypc_bkpt ();

  Dwarf_Addr lowpc = (Dwarf_Addr) -1l;
  Dwarf_Addr highpc = (Dwarf_Addr) -1l;
  while (offset > 0)
    {
      /* We have an address range entry.  */
      if (search_range (begin, end, true, false) < 0)
	return -1;

      if (begin < lowpc)
	{
	  lowpc = begin;
	  highpc = end;
	}

      offset = INTUSE(dwarf_ranges) (die, offset, &base, &begin, &end);
    }

  /* If we didn't find any proper DWARF markers, then look in the
     lowest-addressed range for an ad hoc marker.  Failing that,
     fall back to just using the entrypc value.  */
  return (nbkpts
	  ?: (lowpc == (Dwarf_Addr) -1l ? 0
	      : search_range (lowpc, highpc, false, true))
	  ?: entrypc_bkpt ());
}
