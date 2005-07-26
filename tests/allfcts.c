#include <fcntl.h>
#include <libdw.h>
#include <stdio.h>
#include <unistd.h>


static int
cb (Dwarf_Func *func, void *arg __attribute__ ((unused)))
{
  const char *file = dwarf_func_file (func);
  int line = -1;
  dwarf_func_line (func, &line);
  const char *fct = dwarf_func_name (func);

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
