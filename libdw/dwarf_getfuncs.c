/* Get function information.
   Copyright (C) 2005 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

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
