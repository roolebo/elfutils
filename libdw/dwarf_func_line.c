/* Get line number of beginning of given function.
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

#include <assert.h>
#include <dwarf.h>
#include <limits.h>
#include "libdwP.h"


int
dwarf_func_line (Dwarf_Func *func, int *linep)
{
  return __libdw_func_intval (func, linep, DW_AT_decl_line);
}


int internal_function
__libdw_func_intval (Dwarf_Func *func, int *linep, int attval)
{
  Dwarf_Attribute attr_mem;
  Dwarf_Sword line;

  int res = INTUSE(dwarf_formsdata) (INTUSE(dwarf_attr) (func->die, attval,
							 &attr_mem), &line);
  if (res == 0)
    {
      assert (line >= 0 && line <= INT_MAX);
      *linep = line;
    }

  return res;
}
