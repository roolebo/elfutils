/* Return second macro parameter.
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

#include "libdwP.h"


int
dwarf_macro_param2 (Dwarf_Macro *macro, Dwarf_Word *paramp, const char **strp)
{
  if (macro == NULL)
    return -1;

  if (paramp != NULL)
    *paramp = macro->param2.u;
  if (strp != NULL)
    *strp = macro->param2.s;

  return 0;
}
