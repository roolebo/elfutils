/* Get address range which includes given address.
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

#include <libdwP.h>


Dwarf_Arange *
dwarf_getarange_addr (aranges, addr)
     Dwarf_Aranges *aranges;
     Dwarf_Addr addr;
{
  if (aranges == NULL)
    return NULL;

  /* The ranges are sorted by address, so we can use binary search.  */
  size_t l = 0, u = aranges->naranges;
  while (l < u)
    {
      size_t idx = (l + u) / 2;
      if (addr < aranges->info[idx].addr)
	u = idx;
      else if (addr > aranges->info[idx].addr
	       && addr - aranges->info[idx].addr >= aranges->info[idx].length)
	l = idx + 1;
      else
	return &aranges->info[idx];
    }

  __libdw_seterrno (DWARF_E_NO_MATCH);
  return NULL;
}
INTDEF(dwarf_getarange_addr)
