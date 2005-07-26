/* Iterate through modules to fetch Dwarf information.
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

ptrdiff_t
dwfl_getdwarf (Dwfl *dwfl,
	       int (*callback) (Dwfl_Module *, void **,
				const char *, Dwarf_Addr,
				Dwarf *, Dwarf_Addr, void *),
	       void *arg,
	       ptrdiff_t offset)
{
  if (dwfl == NULL)
    return -1;

  if ((size_t) offset > dwfl->nmodules)
    return -1;

  while ((size_t) offset < dwfl->nmodules)
    {
      Dwfl_Module *mod = dwfl->modules[offset++];
      Dwarf_Addr bias = 0;
      Dwarf *dw = INTUSE(dwfl_module_getdwarf) (mod, &bias);
      if ((*callback) (MODCB_ARGS (mod), dw, bias, arg) != DWARF_CB_OK)
	return offset;
    }

  return 0;
}
