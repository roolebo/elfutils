/* Return information about a module.
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

const char *
dwfl_module_info (Dwfl_Module *mod, void ***userdata,
		  Dwarf_Addr *start, Dwarf_Addr *end,
		  Dwarf_Addr *dwbias, Dwarf_Addr *symbias,
		  const char **mainfile, const char **debugfile)
{
  if (mod == NULL)
    return NULL;

  if (userdata)
    *userdata = &mod->userdata;
  if (start)
    *start = mod->low_addr;
  if (end)
    *end = mod->high_addr;

  if (dwbias)
    *dwbias = mod->debug.elf == NULL ? (Dwarf_Addr) -1 : mod->debug.bias;
  if (symbias)
    *symbias = mod->symfile == NULL ? (Dwarf_Addr) -1 : mod->symfile->bias;

  if (mainfile)
    *mainfile = mod->main.name;

  if (debugfile)
    *debugfile = mod->debug.name;

  return mod->name;
}
