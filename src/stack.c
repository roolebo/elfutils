/* Unwinding of frames like gstack/pstack.
   Copyright (C) 2013 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include <assert.h>
#include <argp.h>
#include <error.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <locale.h>
#include <fcntl.h>
#include ELFUTILS_HEADER(dwfl)

#include <system.h>

/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
ARGP_PROGRAM_VERSION_HOOK_DEF = print_version;

/* Bug report address.  */
ARGP_PROGRAM_BUG_ADDRESS_DEF = PACKAGE_BUGREPORT;

static bool show_activation = false;
static bool show_module = false;
static bool show_source = false;
static bool show_one_tid = false;

static unsigned maxframes = 64;

struct frame
{
  Dwarf_Addr pc;
  bool isactivation;
};

struct frames
{
  unsigned frames;
  struct frame frame[];
};

static Dwfl *dwfl = NULL;

static int
frame_callback (Dwfl_Frame *state, void *arg)
{
  struct frames *frames = (struct frames *) arg;
  unsigned nr = frames->frames;
  if (! dwfl_frame_pc (state, &frames->frame[nr].pc,
		       &frames->frame[nr].isactivation))
    {
      error (0, 0, "%s", dwfl_errmsg (-1));
      return DWARF_CB_ABORT;
    }

  frames->frames++;
  if (frames->frames == maxframes)
    return DWARF_CB_ABORT;

  return DWARF_CB_OK;
}

static void
print_frames (struct frames *frames)
{
  for (unsigned nr = 0; nr < frames->frames; nr++)
    {
      Dwarf_Addr pc = frames->frame[nr].pc;
      bool isactivation = frames->frame[nr].isactivation;
      Dwarf_Addr pc_adjusted = pc - (isactivation ? 0 : 1);

      /* Get PC->SYMNAME.  */
      Dwfl_Module *mod = dwfl_addrmodule (dwfl, pc_adjusted);
      const char *symname = NULL;
      if (mod)
	symname = dwfl_module_addrname (mod, pc_adjusted);

      // Try to find the address wide if possible.
      static int width = 0;
      if (width == 0 && mod)
	{
	  Dwarf_Addr bias;
	  Elf *elf = dwfl_module_getelf (mod, &bias);
	  if (elf)
	    {
	      GElf_Ehdr ehdr_mem;
	      GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
	      if (ehdr)
		width = ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 8 : 16;
	    }
	}
      if (width == 0)
	width = 16;

      printf ("#%-2u 0x%0*" PRIx64, nr, width, (uint64_t) pc);

      if (show_activation)
	printf ("%4s", ! isactivation ? "- 1" : "");

      if (symname != NULL)
	printf (" %s", symname);

      if (show_module)
	{
	  const char* fname;
	  fname = dwfl_module_info(mod,
				   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	  if (fname != NULL)
	    printf (" - %s", fname);
	}

      if (show_source)
	{
	  Dwfl_Line *lineobj = dwfl_module_getsrc(mod, pc_adjusted);
	  if (lineobj)
	    {
	      int line, col;
	      const char* sname;
	      line = col = -1;
	      sname = dwfl_lineinfo (lineobj, NULL, &line, &col, NULL, NULL);
	      if (sname != NULL)
		{
		  printf ("\n    %s", sname);
		  if (line > 0)
		    {
		      printf (":%d", line);
		      if (col > 0)
			printf (":%d", col);
		    }
		}
	    }
	}
      printf ("\n");
    }
}

static int
thread_callback (Dwfl_Thread *thread, void *thread_arg)
{
  printf ("TID %ld:\n", (long) dwfl_thread_tid (thread));
  struct frames *frames = (struct frames *) thread_arg;
  frames->frames = 0;
  switch (dwfl_thread_getframes (thread, frame_callback, thread_arg))
    {
    case DWARF_CB_OK:
    case DWARF_CB_ABORT:
      break;
    case -1:
      error (0, 0, "dwfl_thread_getframes: %s", dwfl_errmsg (-1));
      break;
    default:
      abort ();
    }
  print_frames (frames);
  return DWARF_CB_OK;
}

static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "stack (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
}

static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
	   struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_INIT:
      state->child_inputs[0] = state->input;
      break;

    case 'm':
      show_module = true;
      break;

    case 's':
      show_source = true;
      break;

    case 'a':
      show_activation = true;
      break;

    case 'v':
      show_activation = show_source = show_module = true;
      break;

    case '1':
      show_one_tid = true;
      break;

    case 'n':
      maxframes = atoi (arg);
      if (maxframes == 0)
	{
	  argp_error (state, N_("-n MAXFRAMES should be 1 or higher."));
	  return EINVAL;
	}
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
  __fsetlocking (stdin, FSETLOCKING_BYCALLER);
  __fsetlocking (stdout, FSETLOCKING_BYCALLER);
  __fsetlocking (stderr, FSETLOCKING_BYCALLER);

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  const struct argp_option options[] =
    {
      { "activation",  'a', NULL, 0,
	N_("Additionally show frame activation"), 0 },
      { "module",  'm', NULL, 0,
	N_("Additionally show module file information"), 0 },
      { "source",  's', NULL, 0,
	N_("Additionally show source file information"), 0 },
      { "verbose", 'v', NULL, 0,
	N_("Show all additional information (activation, module and source)"), 0 },
      { NULL, '1', NULL, 0,
	N_("Show the backtrace of only one thread"), 0 },
      { NULL, 'n', "MAXFRAMES", 0,
	N_("Show at most MAXFRAMES per thread (defaults to 64)"), 0 },
      { NULL, 0, NULL, 0, NULL, 0 }
    };

  const struct argp_child children[] =
    {
      { .argp = dwfl_standard_argp () },
      { .argp = NULL },
    };

  const struct argp argp =
    {
      .options = options,
      .parser = parse_opt,
      .doc = N_("\
Print a stack for each thread in a process or core file.\n\
Only real user processes are supported, no kernel or process maps."),
      .children = children
    };

  int remaining;
  argp_parse (&argp, argc, argv, 0, &remaining, &dwfl);
  assert (dwfl != NULL);
  if (remaining != argc)
    error (2, 0, "eu-stack [-a] [-m] [-s] [-v] [-1] [-n MAXFRAMES]"
	   " [--debuginfo-path=<path>]"
	   " {-p <process id>|--core=<file> [--executable=<file>]|--help}");

  /* dwfl_linux_proc_report has been already called from dwfl_standard_argp's
     parse_opt function.  */
  if (dwfl_report_end (dwfl, NULL, NULL) != 0)
    error (2, 0, "dwfl_report_end: %s", dwfl_errmsg (-1));

  struct frames *frames = malloc (sizeof (struct frames)
				  + sizeof (struct frame) * maxframes);
  frames->frames = 0;

  if (show_one_tid)
    {
      pid_t tid = dwfl_pid (dwfl);
      switch (dwfl_getthread_frames (dwfl, tid, frame_callback, frames))
	{
	case DWARF_CB_OK:
	  break;
	case -1:
	  error (0, 0, "dwfl_getthread_frames (%d): %s", tid,
		 dwfl_errmsg (-1));
	  break;
	default:
	  abort ();
	}
      print_frames (frames);
    }
  else
    {
      switch (dwfl_getthreads (dwfl, thread_callback, frames))
	{
	case DWARF_CB_OK:
	  break;
	case -1:
	  error (0, 0, "dwfl_getthreads: %s", dwfl_errmsg (-1));
	  break;
	default:
	  abort ();
	}
    }
  free (frames);
  dwfl_end (dwfl);

  return 0;
}
