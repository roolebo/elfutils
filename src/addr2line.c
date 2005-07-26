/* Locate source files and line information for given addresses
   Copyright (C) 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#include <argp.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <inttypes.h>
#include <libdw.h>
#include <libintl.h>
#include <locale.h>
#include <mcheck.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/* Bug report address.  */
const char *argp_program_bug_address = PACKAGE_BUGREPORT;


/* Values for the parameters which have no short form.  */
#define OPT_DEMANGLER 0x100

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Input Selection:"), 0 },
  { "exe", 'e', "FILE", 0, N_("Find addresses in FILE"), 0 },

  { NULL, 0, NULL, 0, N_("Output Selection:"), 0 },
  { "basenames", 's', NULL, 0, N_("Show only base names of source files"), 0 },
  { "functions", 'f', NULL, 0, N_("Additional show function names"), 0 },

  { NULL, 0, NULL, 0, N_("Miscellaneous:"), 0 },
  /* Unsupported options.  */
  { "target", 'b', "ARG", OPTION_HIDDEN, NULL, 0 },
  { "demangle", 'C', "ARG", OPTION_HIDDEN | OPTION_ARG_OPTIONAL, NULL, 0 },
  { "demangler", OPT_DEMANGLER, "ARG", OPTION_HIDDEN, NULL, 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("\
Locate source files and line information for ADDRs (in a.out by default).");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("[ADDR...]");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, NULL, NULL
};


/* Handle ADDR.  */
static void handle_address (GElf_Addr addr, Elf *elf, Dwarf *dw);


/* Name of the executable.  */
static const char *executable = "a.out";

/* True if only base names of files should be shown.  */
static bool only_basenames;

/* True if function names should be shown.  */
static bool show_functions;


int
main (int argc, char *argv[])
{
  int remaining;
  int result = 0;

  /* Make memory leak detection possible.  */
  mtrace ();

  /* We use no threads here which can interfere with handling a stream.  */
  (void) __fsetlocking (stdout, FSETLOCKING_BYCALLER);

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  /* Make sure the message catalog can be found.  */
  (void) bindtextdomain (PACKAGE, LOCALEDIR);

  /* Initialize the message catalog.  */
  (void) textdomain (PACKAGE);

  /* Parse and process arguments.  */
  (void) argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  /* Tell the library which version we are expecting.  */
  elf_version (EV_CURRENT);

  /* Open the file.  */
  int fd = open64 (executable, O_RDONLY);
  if (fd == -1)
    error (1, errno, gettext ("cannot open '%s'"), executable);

  /* Create the ELF descriptor.  */
  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
  if (elf == NULL)
    {
      close (fd);
      error (1, 0, gettext ("cannot create ELF descriptor: %s"),
	     elf_errmsg (-1));
    }

  /* Try to get a DWARF descriptor.  If it fails, we try to locate the
     debuginfo file.  */
  Dwarf *dw = dwarf_begin_elf (elf, DWARF_C_READ, NULL);
  int fd2 = -1;
  Elf *elf2 = NULL;
  if (dw == NULL)
    {
      char *canon = canonicalize_file_name (executable);
      GElf_Ehdr ehdr_mem;
      GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);

      if (canon != NULL && ehdr != NULL)
	{
	  const char *debuginfo_dir;
	  if (ehdr->e_ident[EI_CLASS] == ELFCLASS32
	      || ehdr->e_machine == EM_IA_64 || ehdr->e_machine == EM_ALPHA)
	    debuginfo_dir = "/usr/lib/debug";
	  else
	    debuginfo_dir = "/usr/lib64/debug";

	  char *difname = alloca (strlen (debuginfo_dir) + strlen (canon) + 1);
	  strcpy (stpcpy (difname, debuginfo_dir), canon);
	  fd2 = open64 (difname, O_RDONLY);
	  if (fd2 != -1)
	    dw = dwarf_begin_elf (elf2, DWARF_C_READ, NULL);
	}

      free (canon);
    }

  /* Now handle the addresses.  In case none are given on the command
     line, read from stdin.  */
  if (remaining == argc)
    {
      /* We use no threads here which can interfere with handling a stream.  */
      (void) __fsetlocking (stdin, FSETLOCKING_BYCALLER);

      char *buf = NULL;
      size_t len = 0;
      while (!feof_unlocked (stdin))
	{
	  if (getline (&buf, &len, stdin) < 0)
	    break;

	  char *endp;
	  uintmax_t addr = strtoumax (buf, &endp, 0);
	  if (endp != buf)
	    handle_address (addr, elf2 ?: elf, dw);
	  else
	    result = 1;
	}

      free (buf);
    }
  else
    {
      do
	{
	  char *endp;
	  uintmax_t addr = strtoumax (argv[remaining], &endp, 0);
	  if (endp != argv[remaining])
	    handle_address (addr, elf2 ?: elf, dw);
	  else
	    result = 1;
	}
      while (++remaining < argc);
    }

  return result;
}


/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "addr2line (%s) %s\n", PACKAGE_NAME, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Red Hat, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2005");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg,
	   struct argp_state *state __attribute__ ((unused)))
{
  switch (key)
    {
    case 'b':
    case 'C':
    case OPT_DEMANGLER:
      /* Ignored for compatibility.  */
      break;

    case 'e':
      executable = arg;
      break;

    case 's':
      only_basenames = true;
      break;

    case 'f':
      show_functions = true;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


struct func_arg
{
  GElf_Addr addr;
  const char *name;
};


static int
match_func (Dwarf_Func *func, void *arg)
{
  struct func_arg *func_arg = (struct func_arg *) arg;
  Dwarf_Addr addr;

  if (dwarf_func_lowpc (func, &addr) == 0 && addr <= func_arg->addr
      && dwarf_func_highpc (func, &addr) == 0 && func_arg->addr < addr)
    {
      func_arg->name = dwarf_func_name (func);
      return DWARF_CB_ABORT;
    }

  return DWARF_CB_OK;
}


static const char *
elf_getname (GElf_Addr addr, Elf *elf)
{
  /* The DWARF information is not available.  Use the ELF
     symbol table.  */
  Elf_Scn *scn = NULL;
  Elf_Scn *dynscn = NULL;

  while ((scn = elf_nextscn (elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr != NULL)
	{
	  if (shdr->sh_type == SHT_SYMTAB)
	    break;
	  if (shdr->sh_type == SHT_DYNSYM)
	    dynscn = scn;
	}
    }
  if (scn == NULL)
    scn = dynscn;

  if (scn != NULL)
    {
      /* Look through the symbol table for a matching symbol.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      assert (shdr != NULL);

      Elf_Data *data = elf_getdata (scn, NULL);
      if (data != NULL)
	for (int cnt = 1; cnt < (int) (shdr->sh_size / shdr->sh_entsize);
	     ++cnt)
	  {
	    GElf_Sym sym_mem;
	    GElf_Sym *sym = gelf_getsym (data, cnt, &sym_mem);
	    if (sym != NULL
		&& sym->st_value <= addr
		&& addr < sym->st_value + sym->st_size)
	      return elf_strptr (elf, shdr->sh_link, sym->st_name);
	  }
    }

  return NULL;
}


static void
handle_address (GElf_Addr addr, Elf *elf, Dwarf *dw)
{
  Dwarf_Die die_mem;
  Dwarf_Die *die = dwarf_addrdie (dw, addr, &die_mem);

  if (show_functions)
    {
      /* First determine the function name.  Use the DWARF information if
	 possible.  */
      struct func_arg arg;
      arg.addr = addr;
      arg.name = NULL;

      if (dwarf_getfuncs (die, match_func, &arg, 0) <= 0)
	arg.name = elf_getname (addr, elf);

      puts (arg.name ?: "??");
    }


  Dwarf_Line *line;
  const char *src;
  if ((line = dwarf_getsrc_die (die, addr)) != NULL
    && (src = dwarf_linesrc (line, NULL, NULL)) != NULL)
    {
      if (only_basenames)
	src = basename (src);

      int lineno;
      if (dwarf_lineno (line, &lineno) != -1)
	{
	  int linecol;
	  if (dwarf_linecol (line, &linecol) != -1 && linecol != 0)
	    printf ("%s:%d:%d\n", src, lineno, linecol);
	  else
	    printf ("%s:%d\n", src, lineno);
	}
      else
	printf ("%s:0\n", src);
    }
  else
    puts ("??:0");
}
