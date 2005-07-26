/* Return one of the sources lines of a CU.
   Copyright (C) 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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


Dwarf_Line *
dwarf_onesrcline (Dwarf_Lines *lines, size_t idx)
{
  if (lines == NULL)
    return NULL;

  if (idx >= lines->nlines)
    {
      __libdw_seterrno (DWARF_E_INVALID_LINE_IDX);
      return NULL;
    }

  return &lines->info[idx];
}
