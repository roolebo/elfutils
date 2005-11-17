/* Copyright (C) 2005 Red Hat, Inc.

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

#include <fcntl.h>
#include ELFUTILS_HEADER(dw)
#include <stdio.h>
#include <unistd.h>


static int
cb (Dwarf_Die *func, void *arg __attribute__ ((unused)))
{
  const char *file = dwarf_decl_file (func);
  int line = -1;
  dwarf_decl_line (func, &line);
  const char *fct = dwarf_diename (func);

  printf ("%s:%d:%s\n", file, line, fct);

  return DWARF_CB_OK;
}


int
main (int argc, char *argv[])
{
  for (int i = 1; i < argc; ++i)
    {
      int fd = open (argv[i], O_RDONLY);

      Dwarf *dbg = dwarf_begin (fd, DWARF_C_READ);
      if (dbg != NULL)
	{
	  Dwarf_Off off = 0;
	  size_t cuhl;
	  Dwarf_Off noff;

	  while (dwarf_nextcu (dbg, off, &noff, &cuhl, NULL, NULL, NULL) == 0)
	    {
	      Dwarf_Die die_mem;
	      Dwarf_Die *die = dwarf_offdie (dbg, off + cuhl, &die_mem);

	      (void) dwarf_getfuncs (die, cb, NULL, 0);

	      off = noff;
	    }

	  dwarf_end (dbg);
	}

      close (fd);
    }
}
