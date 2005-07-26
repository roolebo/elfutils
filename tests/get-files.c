/* Copyright (C) 2002, 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include <fcntl.h>
#include <libelf.h>
#include <libdw.h>
#include <stdio.h>
#include <unistd.h>


int
main (int argc, char *argv[])
{
  int result = 0;
  int cnt;

  for (cnt = 1; cnt < argc; ++cnt)
    {
      int fd = open (argv[cnt], O_RDONLY);

      Dwarf *dbg = dwarf_begin (fd, DWARF_C_READ);
      if (dbg == NULL)
	{
	  printf ("%s not usable\n", argv[cnt]);
	  result = 1;
	  if (fd != -1)
	    close (fd);
	  continue;
	}

      Dwarf_Off o = 0;
      Dwarf_Off ncu;
      Dwarf_Off ao;
      size_t cuhl;
      uint8_t asz;
      uint8_t osz;
      while (dwarf_nextcu (dbg, o, &ncu, &cuhl, &ao, &asz, &osz) == 0)
	{
	  printf ("cuhl = %zu, o = %llu, asz = %hhu, osz = %hhu, ncu = %llu\n",
		  cuhl, (unsigned long long int) ao,
		  asz, osz, (unsigned long long int) ncu);

	  Dwarf_Die die_mem;
	  Dwarf_Die *die = dwarf_offdie (dbg, o + cuhl, &die_mem);
	  if (die == NULL)
	    {
	      printf ("%s: cannot get CU die\n", argv[cnt]);
	      result = 1;
	      break;
	    }

	  Dwarf_Files *files;
	  size_t nfiles;
	  if (dwarf_getsrcfiles (die, &files, &nfiles) != 0)
	    {
	      printf ("%s: cannot get files\n", argv[cnt]);
	      result = 1;
	      break;
	    }

	  for (size_t i = 0; i < nfiles; ++i)
	    printf (" file[%zu] = \"%s\"\n", i,
		    dwarf_filesrc (files, i, NULL, NULL));

	  o = ncu;
	}

      dwarf_end (dbg);
      close (fd);
    }

  return result;
}
