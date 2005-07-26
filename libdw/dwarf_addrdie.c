/* Return CU DIE containing given address.
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


Dwarf_Die *
dwarf_addrdie (dbg, addr, result)
     Dwarf *dbg;
     Dwarf_Addr addr;
     Dwarf_Die *result;
{
  Dwarf_Aranges *aranges;
  size_t naranges;
  Dwarf_Off off;

  if (INTUSE(dwarf_getaranges) (dbg, &aranges, &naranges) != 0
      || INTUSE(dwarf_getarangeinfo) (INTUSE(dwarf_getarange_addr) (aranges,
								    addr),
				      NULL, NULL, &off) != 0)
    return NULL;

  return INTUSE(dwarf_offdie) (dbg, off, result);
}
