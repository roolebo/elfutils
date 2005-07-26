/* Iterate through DWARF compilation units across all modules.
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

Dwarf_Die *
dwfl_nextcu (Dwfl *dwfl, Dwarf_Die *lastcu, Dwarf_Addr *bias)
{
  if (dwfl == NULL)
    return NULL;

  struct dwfl_cu *cu = (struct dwfl_cu *) lastcu;
  Dwfl_Module *mod;

  if (cu == NULL)
    {
      mod = dwfl->modulelist;
      goto nextmod;
    }
  else
    mod = cu->mod;

  Dwfl_Error error;
  while ((error = __libdwfl_nextcu (mod, cu, &cu)) == DWFL_E_NOERROR)
    {
      if (cu != NULL)
	{
	  *bias = mod->debug.bias;
	  return &cu->die;
	}

      mod = mod->next;

    nextmod:
      if (mod == NULL || INTUSE(dwfl_module_getdwarf) (mod, bias) == NULL)
	return NULL;
    }

  __libdwfl_seterrno (error);
  return NULL;
}
INTDEF (dwfl_nextcu)
