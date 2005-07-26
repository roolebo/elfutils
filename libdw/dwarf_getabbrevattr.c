/* Get specific attribute of abbreviation.
   Copyright (C) 2003, 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include "libdwP.h"


int
dwarf_getabbrevattr (abbrev, idx, namep, formp, offsetp)
     Dwarf_Abbrev *abbrev;
     size_t idx;
     unsigned int *namep;
     unsigned int *formp;
     Dwarf_Off *offsetp;
{
  if (abbrev == NULL)
    return -1;

  size_t cnt = 0;
  const unsigned char *attrp = abbrev->attrp;
  const unsigned char *start_attrp;
  unsigned int name;
  unsigned int form;

  do
    {
      start_attrp = attrp;

      /* Attribute code and form are encoded as ULEB128 values.  */
      get_uleb128 (name, attrp);
      get_uleb128 (form, attrp);

      /* If both values are zero the index is out of range.  */
      if (name == 0 && form == 0)
	return -1;
    }
  while (cnt++ < idx);

  /* Store the result if requested.  */
  if (namep != NULL)
    *namep = name;
  if (formp != NULL)
    *formp = form;
  if (offsetp != NULL)
    *offsetp = (start_attrp - abbrev->attrp) + abbrev->offset;

  return 0;
}
