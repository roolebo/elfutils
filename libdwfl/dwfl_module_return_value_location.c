/* Return location expression to find return value given a function type DIE.
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


int
dwfl_module_return_value_location (mod, functypedie, locops)
     Dwfl_Module *mod;
     Dwarf_Die *functypedie;
     const Dwarf_Op **locops;
{
  if (mod == NULL)
    return -1;

  if (mod->ebl == NULL)
    {
      mod->ebl = ebl_openbackend (mod->main.elf);
      if (mod->ebl == NULL)
	{
	  __libdwfl_seterrno (DWFL_E_LIBEBL);
	  return -1;
	}
    }

  int nops = ebl_return_value_location (mod->ebl, functypedie, locops);
  if (unlikely (nops < 0))
    {
      if (nops == -1)
	__libdwfl_seterrno (DWFL_E_LIBDW);
      else if (nops == -2)
	__libdwfl_seterrno (DWFL_E_WEIRD_TYPE);
      else
	__libdwfl_seterrno (DWFL_E_LIBEBL);
      nops = -1;
    }

  return nops;
}
