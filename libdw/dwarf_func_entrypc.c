/* Get entry address of function.
   Copyright (C) 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#include <dwarf.h>
#include "libdwP.h"


int
dwarf_func_entrypc (Dwarf_Func *func, Dwarf_Addr *return_addr)
{
  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = INTUSE(dwarf_attr) (func->die, DW_AT_entry_pc,
					      &attr_mem);
  if (attr != NULL)
    return INTUSE(dwarf_formaddr) (attr, return_addr);

  return INTUSE(dwarf_lowpc) (func->die, return_addr);
}
