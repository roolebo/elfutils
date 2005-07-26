/* Return list address ranges.
   Copyright (C) 2000, 2001, 2002, 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

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

#include <libdwP.h>


int
dwarf_getarangeinfo (Dwarf_Arange *arange, Dwarf_Addr *addrp,
		     Dwarf_Word *lengthp, Dwarf_Off *offsetp)
{
  if (arange == NULL)
    return -1;

  if (addrp != NULL)
    *addrp = arange->addr;
  if (lengthp != NULL)
    *lengthp = arange->length;
  if (offsetp != NULL)
    *offsetp = arange->offset;

  return 0;
}
INTDEF(dwarf_getarangeinfo)
