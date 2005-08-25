/* Get information from a source line record returned by libdwfl.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include "libdwflP.h"
#include "../libdw/libdwP.h"

extern const char *
dwfl_lineinfo (Dwfl_Line *line, Dwarf_Addr *addr, int *linep, int *colp,
	       Dwarf_Word *mtime, Dwarf_Word *length)
{
  if (line == NULL)
    return NULL;

  struct dwfl_cu *cu = dwfl_linecu (line);
  const Dwarf_Line *info = &cu->die.cu->lines->info[line->idx];

  if (addr != NULL)
    *addr = info->addr + cu->mod->debug.bias;
  if (linep != NULL)
    *linep = info->line;
  if (colp != NULL)
    *colp = info->column;

  struct Dwarf_Fileinfo_s *file = &info->files->info[info->file];
  if (mtime != NULL)
    *mtime = file->mtime;
  if (length != NULL)
    *length = file->length;
  return file->name;
}
