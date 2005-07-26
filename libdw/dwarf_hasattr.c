/* Check whether given DIE has specific attribute.
   Copyright (C) 2003, 2005 Red Hat, Inc.
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

#include <dwarf.h>
#include "libdwP.h"


int
dwarf_hasattr (die, search_name)
     Dwarf_Die *die;
     unsigned int search_name;
{
  if (die == NULL)
    return 0;

  /* Search for the attribute with the given name.  */
  unsigned int code;
  (void) __libdw_find_attr (die, search_name, &code, NULL);

  return code == search_name;
}
INTDEF (dwarf_hasattr)
