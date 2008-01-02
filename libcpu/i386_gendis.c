#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern int i386_parse (void);


extern FILE *i386_in;
extern int i386_debug;
char *infname;

FILE *outfile;

int
main (int argc, char *argv[argc])
{
  outfile = stdout;

  if (argc == 1)
    error (EXIT_FAILURE, 0, "usage: %s <MNEDEFFILE>", argv[0]);

  //i386_debug = 1;
  infname = argv[1];
  if (strcmp (infname, "-") == 0)
    i386_in = stdin;
  else
    {
      i386_in = fopen (infname, "r");
      if (i386_in == NULL)
	error (EXIT_FAILURE, errno, "cannot open %s", argv[1]);
    }

  i386_parse ();

  return error_message_count != 0;
}
