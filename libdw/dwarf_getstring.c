/* Get string.
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

#include <string.h>
#include "libdwP.h"


const char *
dwarf_getstring (dbg, offset, lenp)
     Dwarf *dbg;
     Dwarf_Off offset;
     size_t *lenp;
{
  if (dbg == NULL)
    return NULL;

  if (dbg->sectiondata[IDX_debug_str] == NULL
      || offset >= dbg->sectiondata[IDX_debug_str]->d_size)
    {
    no_string:
      __libdw_seterrno (DWARF_E_NO_STRING);
      return NULL;
    }

  const char *result = ((const char *) dbg->sectiondata[IDX_debug_str]->d_buf
			+ offset);
  const char *endp = memchr (result, '\0',
			     dbg->sectiondata[IDX_debug_str]->d_size - offset);
  if (endp == NULL)
    goto no_string;

  if (lenp != NULL)
    *lenp = endp - result;

  return result;
}
