#include <fcntl.h>
#include <inttypes.h>
#include <libdw.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int
main (int argc, char *argv[])
{
  for (int cnt = 1; cnt < argc; ++cnt)
    {
      char *fname;
      char *file;
      int line;

      switch (sscanf (argv[cnt], "%a[^:]:%a[^:]:%d",
			 &fname, &file, &line))
	{
	default:
	case 0:
	case 1:
	  printf ("ignored %s\n", argv[cnt]);
	  continue;
	case 2:
	  line = 0;
	  break;
	case 3:
	  break;
	}

      int fd = open (fname, O_RDONLY);
      if (fd == -1)
	continue;

      Dwarf *dbg = dwarf_begin (fd, DWARF_C_READ);
      if (dbg != NULL)
	{
	  Dwarf_Line **lines = NULL;
	  size_t nlines = 0;

	  if (dwarf_getsrc_file (dbg, file, line, 0, &lines, &nlines) == 0)
	    {
	      for (size_t inner = 0; inner < nlines; ++inner)
		{
		  Dwarf_Addr addr;
		  if (dwarf_lineaddr (lines[inner], &addr) == 0)
		    printf ("%s -> %#" PRIxMAX "\n",
			    argv[cnt], (uintmax_t) addr);
		}

	      free (lines);
	    }

	  dwarf_end (dbg);
	}

      close (fd);
      free (fname);
      free (file);
    }

  return 0;
}
