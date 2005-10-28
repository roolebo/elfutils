/* Get function information.
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

#include <dwarf.h>
#include "libdwP.h"


ptrdiff_t
dwarf_getfuncs (Dwarf_Die *cudie, int (*callback) (Dwarf_Die *, void *),
		void *arg, ptrdiff_t offset)
{
  if (unlikely (cudie == NULL
		|| INTUSE(dwarf_tag) (cudie) != DW_TAG_compile_unit))
    return -1;

  Dwarf_Die die_mem;
  Dwarf_Die *die;

  int res;
  if (offset == 0)
    res = INTUSE(dwarf_child) (cudie, &die_mem);
  else
    {
      die = INTUSE(dwarf_offdie) (cudie->cu->dbg, offset, &die_mem);
      res = INTUSE(dwarf_siblingof) (die, &die_mem);
    }
  die = res != 0 ? NULL : &die_mem;

  while (die != NULL)
    {
      if (INTUSE(dwarf_tag) (die) == DW_TAG_subprogram)
	{
	  if (callback (die, arg) != DWARF_CB_OK)
	    return INTUSE(dwarf_dieoffset) (die);
	}

      if (INTUSE(dwarf_siblingof) (die, &die_mem) != 0)
	break;
    }

  /* That's all.  */
  return 0;
}
