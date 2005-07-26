/* Return file name containing definition of the given function.
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

#include <assert.h>
#include <dwarf.h>
#include "libdwP.h"


const char *
dwarf_func_file (Dwarf_Func *func)
{
  Dwarf_Attribute attr_mem;
  Dwarf_Sword idx = 0;
  Dwarf_Die *die = func->die;

  if (INTUSE(dwarf_formsdata) (INTUSE(dwarf_attr) (die, DW_AT_decl_file,
						   &attr_mem), &idx) != 0)
    return NULL;

  /* Zero means no source file information available.  */
  if (idx == 0)
    {
      __libdw_seterrno (DWARF_E_NO_ENTRY);
      return NULL;
    }

  /* Get the array of source files for the CU.  */
  struct Dwarf_CU  *cu = die->cu;
  if (cu->lines == NULL)
    {
      Dwarf_Lines *lines;
      size_t nlines;

      /* Let the more generic function do the work.  It'll create more
	 data but that will be needed in an real program anyway.  */
      (void) INTUSE(dwarf_getsrclines) (func->cudie, &lines, &nlines);
      assert (cu->lines != NULL);
    }

  if (cu->lines == (void *) -1l)
    {
      /* If the file index is not zero, there must be file information
	 available.  */
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  assert (cu->files != NULL && cu->files != (void *) -1l);

  if (idx >= cu->files->nfiles)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  return cu->files->info[idx].name;
}
