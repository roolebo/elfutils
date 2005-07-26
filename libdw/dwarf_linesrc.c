/* Find line information for address.
   Copyright (C) 2004 Red Hat, Inc.
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


const char *
dwarf_linesrc (Dwarf_Line *line, Dwarf_Word *mtime, Dwarf_Word *length)
{
  if (line == NULL)
    return NULL;

  if (line->file >= line->files->nfiles)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  if (mtime != NULL)
    *mtime = line->files->info[line->file].mtime;

  if (length != NULL)
    *length = line->files->info[line->file].length;

  return line->files->info[line->file].name;
}
