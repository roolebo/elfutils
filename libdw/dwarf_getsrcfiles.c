/* Return source file information of CU.
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

#include <assert.h>
#include <dwarf.h>
#include "libdwP.h"


int
dwarf_getsrcfiles (Dwarf_Die *cudie, Dwarf_Files **files, size_t *nfiles)
{
  if (unlikely (cudie == NULL
		|| INTUSE(dwarf_tag) (cudie) != DW_TAG_compile_unit))
    return -1;

  int res = -1;

  /* Get the information if it is not already known.  */
  struct Dwarf_CU *const cu = cudie->cu;
  if (cu->lines == NULL)
    {
      Dwarf_Lines *lines;
      size_t nlines;

      /* Let the more generic function do the work.  It'll create more
	 data but that will be needed in an real program anyway.  */
      res = INTUSE(dwarf_getsrclines) (cudie, &lines, &nlines);
    }
  else if (cu->files != (void *) -1l)
    /* We already have the information.  */
    res = 0;

  if (likely (res == 0))
    {
      assert (cu->files != NULL && cu->files != (void *) -1l);
      *files = cu->files;
      if (nfiles != NULL)
	*nfiles = cu->files->nfiles;
    }

  // XXX Eventually: unlocking here.

  return res;
}
INTDEF (dwarf_getsrcfiles)
