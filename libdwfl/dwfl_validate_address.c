/* Validate an address and the relocatability of an offset from it.
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
dwfl_validate_address (Dwfl *dwfl, Dwarf_Addr address, Dwarf_Sword offset)
{
  Dwfl_Module *mod = INTUSE(dwfl_addrmodule) (dwfl, address);
  if (mod == NULL)
    return -1;

  Dwarf_Addr relative = address;
  int idx = INTUSE(dwfl_module_relocate_address) (mod, &relative);
  if (idx < 0)
    return -1;

  if (offset != 0)
    {
      int offset_idx = -1;
      relative = address + offset;
      if (relative >= mod->low_addr && relative <= mod->high_addr)
	{
	  offset_idx = INTUSE(dwfl_module_relocate_address) (mod, &relative);
	  if (offset_idx < 0)
	    return -1;
	}
      if (offset_idx != idx)
	{
	  __libdwfl_seterrno (DWFL_E_ADDR_OUTOFRANGE);
	  return -1;
	}
    }

  return 0;
}
