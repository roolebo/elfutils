/* Fetch source line information for CU.
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
dwfl_getsrclines (Dwarf_Die *cudie, size_t *nlines)
{
  struct dwfl_cu *cu = (struct dwfl_cu *) cudie;

  if (cu->lines == NULL)
    {
      Dwfl_Error error = __libdwfl_cu_getsrclines (cu);
      if (error != DWFL_E_NOERROR)
	{
	  __libdwfl_seterrno (error);
	  return -1;
	}
    }

  *nlines = cu->die.cu->lines->nlines;
  return -1;
}
