/* Finish a session using libdwfl.
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

void
dwfl_end (Dwfl *dwfl)
{
  if (dwfl != NULL)
    {
      for (size_t i = 0; i < dwfl->nmodules; ++i)
	if (dwfl->modules[i] != NULL)
	  __libdwfl_module_free (dwfl->modules[i]);
      free (dwfl->modules);
    }
}
