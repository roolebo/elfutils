/* Fetch source line info for CU.
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

Dwfl_Error
internal_function_def
__libdwfl_cu_getsrclines (struct dwfl_cu *cu)
{
  if (cu->lines == NULL)
    {
      Dwarf_Lines *lines;
      size_t nlines;
      if (INTUSE(dwarf_getsrclines) (&cu->die, &lines, &nlines) != 0)
	return DWFL_E_LIBDW;

      cu->lines = malloc (offsetof (struct Dwfl_Lines, idx[nlines]));
      if (cu->lines == NULL)
	return DWFL_E_NOMEM;
      cu->lines->cu = cu;
      for (unsigned int i = 0; i < nlines; ++i)
	cu->lines->idx[i].idx = i;
    }

  return DWFL_E_NOERROR;
}
