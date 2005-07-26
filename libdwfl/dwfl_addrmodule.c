/* Find module containing address.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include "libdwflP.h"

Dwfl_Module *
dwfl_addrmodule (Dwfl *dwfl, Dwarf_Addr address)
{
  if (dwfl == NULL)
    return NULL;

  /* Do binary search on the array indexed by module load address.  */
  size_t l = 0, u = dwfl->nmodules;
  while (l < u)
    {
      size_t idx = (l + u) / 2;
      Dwfl_Module *m = dwfl->modules[idx];
      if (address < m->low_addr)
	u = idx;
      else if (address >= m->high_addr)
	l = idx + 1;
      else
	return m;
    }

  return NULL;
}
INTDEF (dwfl_addrmodule)
