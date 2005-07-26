/* Find source location for PC address in module.
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
#include "../libdw/libdwP.h"

Dwfl_Line *
dwfl_module_getsrc (Dwfl_Module *mod, Dwarf_Addr addr)
{
  Dwarf_Addr bias;
  if (INTUSE(dwfl_module_getdwarf) (mod, &bias) == NULL)
    return NULL;

  struct dwfl_cu *cu;
  Dwfl_Error error = __libdwfl_addrcu (mod, addr, &cu);
  if (likely (error == DWFL_E_NOERROR))
    error = __libdwfl_cu_getsrclines (cu);
  if (likely (error == DWFL_E_NOERROR))
    {
      /* The lines are sorted by address, so we can use binary search.  */
      size_t l = 0, u = cu->die.cu->lines->nlines;
      while (l < u)
	{
	  size_t idx = (l + u) / 2;
	  if (addr < cu->die.cu->lines->info[idx].addr)
	    u = idx;
	  else if (addr > cu->die.cu->lines->info[idx].addr)
	    l = idx + 1;
	  else
	    return &cu->lines->idx[idx];
	}

      if (cu->die.cu->lines->nlines > 0)
	assert (cu->die.cu->lines->info
		[cu->die.cu->lines->nlines - 1].end_sequence);

      /* If none were equal, the closest one below is what we want.
	 We never want the last one, because it's the end-sequence
	 marker with an address at the high bound of the CU's code.  */
      if (u > 0 && u < cu->die.cu->lines->nlines
	  && addr > cu->die.cu->lines->info[u - 1].addr)
	return &cu->lines->idx[u - 1];

      error = DWFL_E_ADDR_OUTOFRANGE;
    }

  __libdwfl_seterrno (error);
  return NULL;
}
INTDEF (dwfl_module_getsrc)
