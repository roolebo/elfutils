/* Look up the DIE in a reference-form attribute.
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

#include "libdwP.h"

Dwarf_Die *
dwarf_formref_die (Dwarf_Attribute *attr, Dwarf_Die *die_mem)
{
  Dwarf_Off offset;
  return (unlikely (INTUSE(dwarf_formref) (attr, &offset) != 0) ? NULL
	  : INTUSE(dwarf_offdie) (attr->cu->dbg, attr->cu->start + offset,
				  die_mem));
}
INTDEF (dwarf_formref_die)
