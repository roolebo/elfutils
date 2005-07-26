/* Get abbreviation at given offset.
   Copyright (C) 2004, 2005 Red Hat, Inc.
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


int
dwarf_offabbrev (Dwarf *dbg, Dwarf_Off offset, size_t *lengthp,
		 Dwarf_Abbrev *abbrevp)
{
  if (dbg == NULL)
    return -1;

  Dwarf_Abbrev *abbrev = __libdw_getabbrev (dbg, NULL, offset, lengthp,
					    abbrevp);

  if (abbrev == NULL)
    return -1;

  return abbrev == DWARF_END_ABBREV ? 1 : 0;
}
