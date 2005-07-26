/* Check whether DIE has specific attribute, integrating DW_AT_abstract_origin.
   Copyright (C) 2005 Red Hat, Inc.

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
dwarf_hasattr_integrate (Dwarf_Die *die, unsigned int search_name)
{
  Dwarf_Die die_mem;

  do
    {
      if (INTUSE(dwarf_hasattr) (die, search_name))
	return 1;

      Dwarf_Attribute attr_mem;
      Dwarf_Attribute *attr = INTUSE(dwarf_attr) (die, DW_AT_abstract_origin,
						  &attr_mem);
      if (attr == NULL)
	break;

      die = INTUSE(dwarf_formref_die) (attr, &die_mem);
    }
  while (die != NULL);

  return 0;
}
