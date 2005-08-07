/* Test program for libdwfl basic module tracking, relocation.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include <config.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <locale.h>
#include <argp.h>
#include <libdwfl.h>


static int
print_func (Dwarf_Func *func, void *arg)
{
  const Dwarf_Addr dwbias = *(Dwarf_Addr *) arg;

  const char *file = dwarf_func_file (func);
  int line = -1;
  dwarf_func_line (func, &line);
  const char *fct = dwarf_func_name (func);

  printf ("  %s:%d: %s:", file, line, fct);

  Dwarf_Addr lo = -1, hi = -1, entry = -1;
  if (dwarf_func_lowpc (func, &lo) == 0)
    lo += dwbias;
  else
    printf (" (lowpc => %s)", dwarf_errmsg (-1));
  if (dwarf_func_highpc (func, &hi) == 0)
    hi += dwbias;
  else
    printf (" (highpc => %s)", dwarf_errmsg (-1));
  if (dwarf_func_entrypc (func, &entry) == 0)
    entry += dwbias;
  else
    printf (" (entrypc => %s)", dwarf_errmsg (-1));

  if (lo != (Dwarf_Addr) -1 || hi != (Dwarf_Addr) -1
      || entry != (Dwarf_Addr) -1)
    printf (" %#" PRIx64 "..%#" PRIx64 " => %#" PRIx64 "\n",
	    lo, hi, entry);
  else
    puts ("");

  return DWARF_CB_OK;
}

static int
print_module (Dwfl_Module *mod __attribute__ ((unused)),
	      void **userdata __attribute__ ((unused)),
	      const char *name, Dwarf_Addr base,
	      Dwarf *dw, Dwarf_Addr bias,
	      void *arg)
{
  printf ("module: %30s %08" PRIx64 " %12p %" PRIx64 " (%s)\n",
	  name, base, dw, bias, dwfl_errmsg (-1));

  if (dw != NULL && *(const bool *) arg)
    {
      Dwarf_Off off = 0;
      size_t cuhl;
      Dwarf_Off noff;

      while (dwarf_nextcu (dw, off, &noff, &cuhl, NULL, NULL, NULL) == 0)
	{
	  Dwarf_Die die_mem;
	  Dwarf_Die *die = dwarf_offdie (dw, off + cuhl, &die_mem);

	  (void) dwarf_getfuncs (die, print_func, &bias, 0);

	  off = noff;
	}
    }

  return DWARF_CB_OK;
}

static bool show_functions;

static const struct argp_option options[] =
  {
    { "functions", 'f', NULL, 0, N_("Additional show function names"), 0 },
    { NULL, 0, NULL, 0, NULL, 0 }
  };

static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
	   struct argp_state *state __attribute__ ((unused)))
{
  switch (key)
    {
    case ARGP_KEY_INIT:
      state->child_inputs[0] = state->input;
      break;

    case 'f':
      show_functions = true;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  /* We use no threads here which can interfere with handling a stream.  */
  (void) __fsetlocking (stdout, FSETLOCKING_BYCALLER);

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  Dwfl *dwfl = NULL;
  const struct argp_child argp_children[] =
    {
      { .argp = dwfl_standard_argp () },
      { .argp = NULL }
    };
  const struct argp argp =
    {
      options, parse_opt, NULL, NULL, argp_children, NULL, NULL
    };
  (void) argp_parse (&argp, argc, argv, 0, NULL, &dwfl);
  assert (dwfl != NULL);

  ptrdiff_t p = 0;
  do
    p = dwfl_getdwarf (dwfl, &print_module, &show_functions, p);
  while (p > 0);
  if (p < 0)
    error (2, 0, "dwfl_getdwarf: %s", dwfl_errmsg (-1));

  dwfl_end (dwfl);

  return 0;
}
