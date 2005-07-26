/* Set up a session using libdwfl.
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

Dwfl *
dwfl_begin (const Dwfl_Callbacks *callbacks)
{
  if (elf_version (EV_CURRENT) == EV_NONE)
    {
      __libdwfl_seterrno (DWFL_E_LIBELF);
      return NULL;
    }

  Dwfl *dwfl = calloc (1, sizeof *dwfl);
  if (dwfl == NULL)
    __libdwfl_seterrno (DWFL_E_NOMEM);
  else
    dwfl->callbacks = callbacks;

  return dwfl;
}
INTDEF (dwfl_begin)
