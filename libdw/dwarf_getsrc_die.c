/* Find line information for address.
   Copyright (C) 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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
#include <assert.h>


Dwarf_Line *
dwarf_getsrc_die (Dwarf_Die *cudie, Dwarf_Addr addr)
{
  Dwarf_Lines *lines;
  size_t nlines;

  if (INTUSE(dwarf_getsrclines) (cudie, &lines, &nlines) != 0)
    return NULL;

  /* The lines are sorted by address, so we can use binary search.  */
  size_t l = 0, u = nlines;
  while (l < u)
    {
      size_t idx = (l + u) / 2;
      if (addr < lines->info[idx].addr)
	u = idx;
      else if (addr > lines->info[idx].addr || lines->info[idx].end_sequence)
	l = idx + 1;
      else
	return &lines->info[idx];
    }

  if (nlines > 0)
    assert (lines->info[nlines - 1].end_sequence);

  /* If none were equal, the closest one below is what we want.  We
     never want the last one, because it's the end-sequence marker
     with an address at the high bound of the CU's code.  If the debug
     information is faulty and no end-sequence marker is present, we
     still ignore it.  */
  if (u > 0 && u < nlines && addr > lines->info[u - 1].addr)
    {
      while (lines->info[u - 1].end_sequence && u > 0)
	--u;
      if (u > 0)
	return &lines->info[u - 1];
    }

  __libdw_seterrno (DWARF_E_ADDR_OUTOFRANGE);
  return NULL;
}
