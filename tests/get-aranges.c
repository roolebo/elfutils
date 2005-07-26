/* Copyright (C) 2002, 2004 Red Hat, Inc.
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


static const Dwarf_Addr testaddr[] =
{
  0x804842b, 0x804842c, 0x804843c, 0x8048459, 0x804845a,
  0x804845b, 0x804845c, 0x8048460, 0x8048465, 0x8048466,
  0x8048467, 0x8048468, 0x8048470, 0x8048471, 0x8048472
};
#define ntestaddr (sizeof (testaddr) / sizeof (testaddr[0]))


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
	  close (fd);
	  continue;
	}

      Dwarf_Aranges *aranges;
      size_t naranges;
      if (dwarf_getaranges (dbg, &aranges, &naranges) != 0)
	printf ("%s: cannot get aranges\n", argv[cnt]);
      else
	{
	  for (size_t i = 0; i < ntestaddr; ++i)
	    {
	      Dwarf_Arange *found;

	      found = dwarf_getarange_addr (aranges, testaddr[i]);
	      if (found != NULL)
		{
		  Dwarf_Off cu_offset;

		  if (dwarf_getarangeinfo (found, NULL, NULL, &cu_offset) != 0)
		    {
		      puts ("failed to get CU die offset");
		      result = 1;
		    }
		  else
		    {
		      const char *cuname;
		      Dwarf_Die cu_die;

		      if (dwarf_offdie (dbg, cu_offset, &cu_die) == NULL
			  || (cuname = dwarf_diename (&cu_die)) == NULL)
			{
			  puts ("failed to get CU die");
			  result = 1;
			}
		      else
			printf ("CU name: \"%s\"\n", cuname);
		    }
		}
	      else
		printf ("%#llx: not in range\n",
			(unsigned long long int) testaddr[i]);
	    }

	  for (size_t i = 0; i < naranges; ++i)
	    {
	      Dwarf_Arange *arange = dwarf_onearange (aranges, i);
	      if (arange == NULL)
		{
		  printf ("cannot get arange %zu: %s\n", i, dwarf_errmsg (-1));
		  break;
		}

	      Dwarf_Addr start;
	      Dwarf_Word length;
	      Dwarf_Off cu_offset;

	      if (dwarf_getarangeinfo (arange, &start, &length, &cu_offset)
		  != 0)
		{
		  printf ("cannot get info from aranges[%zu]\n", i);
		  result = 1;
		}
	      else
		{
		  printf (" [%2zu] start: %#llx, length: %llu, cu: %llu\n",
			  i, (unsigned long long int) start,
			  (unsigned long long int) length,
			  (unsigned long long int) cu_offset);

		  const char *cuname;
		  Dwarf_Die cu_die;
		  if (dwarf_offdie (dbg, cu_offset, &cu_die) == NULL
		      || (cuname = dwarf_diename (&cu_die)) == NULL)
		    {
		      puts ("failed to get CU die");
		      result = 1;
		    }
		  else
		    printf ("CU name: \"%s\"\n", cuname);
		}
	    }
	}

      dwarf_end (dbg);
      close (fd);
    }

  return result;
}
