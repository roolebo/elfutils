/* Print information from ELF file in human-readable form.
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 1999.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <argp.h>
#include <assert.h>
#include <dwarf.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <inttypes.h>
#include <langinfo.h>
#include <libdw.h>
#include <libintl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>

#include <system.h>
#include "../libebl/libeblP.h"
#include "../libdw/libdwP.h"
#include "../libdw/memory-access.h"


/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/* Bug report address.  */
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Output selection:"), 0 },
  { "all", 'a', NULL, 0, N_("Equivalent to: -h -l"), 0 },
  { "dynamic", 'd', NULL, 0, N_("Display the dynamic segment"), 0 },
  { "file-header", 'h', NULL, 0, N_("Display the ELF file header"), 0 },
  { "histogram", 'I', NULL, 0,
    N_("Display histogram of bucket list lengths"), 0 },
  { "program-headers", 'l', NULL, 0, N_("Display the program headers"), 0 },
  { "relocs", 'r', NULL, 0, N_("Display relocations"), 0 },
  { "section-headers", 'S', NULL, 0, N_("Display the sections' header"), 0 },
  { "symbols", 's', NULL, 0, N_("Display the symbol table"), 0 },
  { "version-info", 'V', NULL, 0, N_("Display versioning information"), 0 },
  { "debug-dump", 'w', "SECTION", OPTION_ARG_OPTIONAL,
    N_("Display DWARF section content.  SECTION can be one of abbrev, "
       "aranges, frame, info, loc, line, ranges, pubnames, str, or macinfo."),
    0 },
  { "notes", 'n', NULL, 0, N_("Display the core notes"), 0 },
  { "arch-specific", 'A', NULL, 0,
    N_("Display architecture specific information (if any)"), 0 },

  { NULL, 0, NULL, 0, N_("Output control:"), 0 },

  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("\
Print information from ELF file in human-readable form.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("FILE...");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, NULL, NULL
};


/* Flags set by the option controlling the output.  */

/* True if dynamic segment should be printed.  */
static bool print_dynamic_table;

/* True if the file header should be printed.  */
static bool print_file_header;

/* True if the program headers should be printed.  */
static bool print_program_header;

/* True if relocations should be printed.  */
static bool print_relocations;

/* True if the section headers should be printed.  */
static bool print_section_header;

/* True if the symbol table should be printed.  */
static bool print_symbol_table;

/* True if the version information should be printed.  */
static bool print_version_info;

/* True if section groups should be printed.  */
static bool print_section_groups;

/* True if bucket list length histogram should be printed.  */
static bool print_histogram;

/* True if the architecture specific data should be printed.  */
static bool print_arch;

/* True if note section content should be printed.  */
static bool print_notes;

/* Select printing of debugging sections.  */
static enum section_e
{
  section_abbrev = 1,	/* .debug_abbrev  */
  section_aranges = 2,	/* .debug_aranges  */
  section_frame = 4,	/* .debug_frame or .eh_frame  */
  section_info = 8,	/* .debug_info  */
  section_line = 16,	/* .debug_line  */
  section_loc = 32,	/* .debug_loc  */
  section_pubnames = 64,/* .debug_pubnames  */
  section_str = 128,	/* .debug_str  */
  section_macinfo = 256,/* .debug_macinfo  */
  section_ranges = 512, /* .debug_ranges  */
  section_all = (section_abbrev | section_aranges | section_frame
		 | section_info | section_line | section_loc
		 | section_pubnames | section_str | section_macinfo
		 | section_ranges)
} print_debug_sections;

/* Number of sections in the file.  */
static size_t shnum;


/* Declarations of local functions.  */
static void process_file (int fd, Elf *elf, const char *prefix,
			  const char *fname, bool only_one);
static void process_elf_file (Elf *elf, const char *prefix, const char *fname,
			      bool only_one);
static void print_ehdr (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_shdr (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_phdr (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_scngrp (Ebl *ebl);
static void print_dynamic (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_relocs (Ebl *ebl);
static void handle_relocs_rel (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void handle_relocs_rela (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void print_symtab (Ebl *ebl, int type);
static void handle_symtab (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void print_verinfo (Ebl *ebl);
static void handle_verneed (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void handle_verdef (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr);
static void handle_versym (Ebl *ebl, Elf_Scn *scn,
			   GElf_Shdr *shdr);
static void print_debug (Ebl *ebl, GElf_Ehdr *ehdr);
static void handle_hash (Ebl *ebl);
static void handle_notes (Ebl *ebl, GElf_Ehdr *ehdr);
static void print_liblist (Ebl *ebl);


int
main (int argc, char *argv[])
{
  /* Set locale.  */
  setlocale (LC_ALL, "");

  /* Initialize the message catalog.  */
  textdomain (PACKAGE);

  /* Parse and process arguments.  */
  int remaining;
  argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  /* Before we start tell the ELF library which version we are using.  */
  elf_version (EV_CURRENT);

  /* Now process all the files given at the command line.  */
  bool only_one = remaining + 1 == argc;
  do
    {
      /* Open the file.  */
      int fd = open (argv[remaining], O_RDONLY);
      if (fd == -1)
	{
	  error (0, errno, gettext ("cannot open input file"));
	  continue;
	}

      /* Create an `Elf' descriptor.  */
      Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
      if (elf == NULL)
	error (0, 0, gettext ("cannot generate Elf descriptor: %s\n"),
	       elf_errmsg (-1));
      else
	{
	  process_file (fd, elf, NULL, argv[remaining], only_one);

	  /* Now we can close the descriptor.  */
	  if (elf_end (elf) != 0)
	    error (0, 0, gettext ("error while closing Elf descriptor: %s"),
		   elf_errmsg (-1));
	}

      close (fd);
    }
  while (++remaining < argc);

  return error_message_count != 0;
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg,
	   struct argp_state *state __attribute__ ((unused)))
{
  /* True if any of the control options is set.  */
  static bool any_control_option;

  switch (key)
    {
    case 'a':
      print_file_header = true;
      print_program_header = true;
      print_relocations = true;
      print_section_header = true;
      print_symbol_table = true;
      print_version_info = true;
      print_dynamic_table = true;
      print_section_groups = true;
      print_histogram = true;
      print_arch = true;
      print_notes = true;
      any_control_option = true;
      break;
    case 'A':
      print_arch = true;
      any_control_option = true;
      break;
    case 'd':
      print_dynamic_table = true;
      any_control_option = true;
      break;
    case 'g':
      print_section_groups = true;
      any_control_option = true;
      break;
    case 'h':
      print_file_header = true;
      any_control_option = true;
      break;
    case 'I':
      print_histogram = true;
      any_control_option = true;
      break;
    case 'l':
      print_program_header = true;
      any_control_option = true;
      break;
    case 'n':
      print_notes = true;
      any_control_option = true;
      break;
    case 'r':
      print_relocations = true;
      any_control_option = true;
     break;
    case 'S':
      print_section_header = true;
      any_control_option = true;
      break;
    case 's':
      print_symbol_table = true;
      any_control_option = true;
      break;
    case 'V':
      print_version_info = true;
      any_control_option = true;
      break;
    case 'w':
      if (arg == NULL)
	print_debug_sections = section_all;
      else if (strcmp (arg, "abbrev") == 0)
	print_debug_sections |= section_abbrev;
      else if (strcmp (arg, "aranges") == 0)
	print_debug_sections |= section_aranges;
      else if (strcmp (arg, "ranges") == 0)
	print_debug_sections |= section_ranges;
      else if (strcmp (arg, "frame") == 0)
	print_debug_sections |= section_frame;
      else if (strcmp (arg, "info") == 0)
	print_debug_sections |= section_info;
      else if (strcmp (arg, "loc") == 0)
	print_debug_sections |= section_loc;
      else if (strcmp (arg, "line") == 0)
	print_debug_sections |= section_line;
      else if (strcmp (arg, "pubnames") == 0)
	print_debug_sections |= section_pubnames;
      else if (strcmp (arg, "str") == 0)
	print_debug_sections |= section_str;
      else if (strcmp (arg, "macinfo") == 0)
	print_debug_sections |= section_macinfo;
      else
	{
	  fprintf (stderr, gettext ("Unknown DWARF debug section `%s'.\n"),
		   arg);
	  argp_help (&argp, stderr, ARGP_HELP_SEE,
		     program_invocation_short_name);
	  exit (1);
	}
      any_control_option = true;
      break;
    case ARGP_KEY_NO_ARGS:
      fputs (gettext ("Missing file name.\n"), stderr);
      goto do_argp_help;
    case ARGP_KEY_FINI:
      if (! any_control_option)
	{
	  fputs (gettext ("No operation specified.\n"), stderr);
	do_argp_help:
	  argp_help (&argp, stderr, ARGP_HELP_SEE | ARGP_HELP_EXIT_ERR,
		     program_invocation_short_name);
	  exit (1);
	}
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "readelf (%s) %s\n", PACKAGE_NAME, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Red Hat, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2006");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* Process one file.  */
static void
process_file (int fd, Elf *elf, const char *prefix, const char *fname,
	      bool only_one)
{
  /* We can handle two types of files: ELF files and archives.  */
  Elf_Kind kind = elf_kind (elf);
  struct stat64 st;

  switch (kind)
    {
    case ELF_K_ELF:
      /* Yes!  It's an ELF file.  */
      process_elf_file (elf, prefix, fname, only_one);
      break;

    case ELF_K_AR:
      {
	size_t prefix_len = prefix == NULL ? 0 : strlen (prefix);
	size_t fname_len = strlen (fname) + 1;
	char new_prefix[prefix_len + 1 + fname_len];
	char *cp = new_prefix;

	/* Create the full name of the file.  */
	if (prefix != NULL)
	  {
	    cp = mempcpy (cp, prefix, prefix_len);
	    *cp++ = ':';
	  }
	memcpy (cp, fname, fname_len);

	/* It's an archive.  We process each file in it.  */
	Elf *subelf;
	Elf_Cmd cmd = ELF_C_READ_MMAP;
	while ((subelf = elf_begin (fd, cmd, elf)) != NULL)
	  {
	    kind = elf_kind (subelf);

	    /* Call this function recursively.  */
	    if (kind == ELF_K_ELF || kind == ELF_K_AR)
	      {
		Elf_Arhdr *arhdr = elf_getarhdr (subelf);
		assert (arhdr != NULL);

		process_file (fd, subelf, new_prefix, arhdr->ar_name, false);
	      }

	    /* Get next archive element.  */
	    cmd = elf_next (subelf);
	    if (elf_end (subelf) != 0)
	      error (0, 0,
		     gettext (" error while freeing sub-ELF descriptor: %s\n"),
		     elf_errmsg (-1));
	  }
      }
      break;

    default:
      if (fstat64 (fd, &st) != 0)
	error (0, errno, gettext ("cannot stat input file"));
      else if (st.st_size == 0)
	error (0, 0, gettext ("input file is empty"));
      else
	/* We cannot do anything.  */
	error (0, 0, gettext ("\
Not an ELF file - it has the wrong magic bytes at the start"));
      break;
    }
}


/* Process one file.  */
static void
process_elf_file (Elf *elf, const char *prefix, const char *fname,
		  bool only_one)
{
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);

  /* Print the file name.  */
  if (!only_one)
    {
      if (prefix != NULL)
	printf ("\n%s(%s):\n\n", prefix, fname);
      else
	printf ("\n%s:\n\n", fname);
    }

  if (ehdr == NULL)
    {
      error (0, 0, gettext ("cannot read ELF header: %s"), elf_errmsg (-1));
      return;
    }

  Ebl *ebl = ebl_openbackend (elf);
  if (ebl == NULL)
    {
      error (0, errno, gettext ("cannot create EBL handle"));
      return;
    }

  /* Determine the number of sections.  */
  if (elf_getshnum (ebl->elf, &shnum) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot determine number of sections: %s"),
	   elf_errmsg (-1));

  if (print_file_header)
    print_ehdr (ebl, ehdr);
  if (print_section_header)
    print_shdr (ebl, ehdr);
  if (print_program_header)
    print_phdr (ebl, ehdr);
  if (print_section_groups)
    print_scngrp (ebl);
  if (print_dynamic_table)
    print_dynamic (ebl, ehdr);
  if (print_relocations)
    print_relocs (ebl);
  if (print_histogram)
    handle_hash (ebl);
  if (print_symbol_table)
    print_symtab (ebl, SHT_DYNSYM);
  if (print_version_info)
    print_verinfo (ebl);
  if (print_symbol_table)
    print_symtab (ebl, SHT_SYMTAB);
  if (print_arch)
    print_liblist (ebl);
  if (print_debug_sections != 0)
    print_debug (ebl, ehdr);
  if (print_notes)
    handle_notes (ebl, ehdr);

  ebl_closebackend (ebl);
}


/* Print file type.  */
static void
print_file_type (unsigned short int e_type)
{
  if (e_type <= ET_CORE)
    {
      static const char *knowntypes[] =
      {
	N_("NONE (None)"),
	N_("REL (Relocatable file)"),
	N_("EXEC (Executable file)"),
	N_("DYN (Shared object file)"),
	N_("CORE (Core file)")
      };
      puts (gettext (knowntypes[e_type]));
    }
  else if (e_type >= ET_LOOS && e_type <= ET_HIOS)
    printf (gettext ("OS Specific: (%x)\n"),  e_type);
  else if (e_type >= ET_LOPROC /* && e_type <= ET_HIPROC always true */)
    printf (gettext ("Processor Specific: (%x)\n"),  e_type);
  else
    puts ("???");
}


/* Print ELF header.  */
static void
print_ehdr (Ebl *ebl, GElf_Ehdr *ehdr)
{
  fputs_unlocked (gettext ("ELF Header:\n  Magic:  "), stdout);
  for (size_t cnt = 0; cnt < EI_NIDENT; ++cnt)
    printf (" %02hhx", ehdr->e_ident[cnt]);

  printf (gettext ("\n  Class:                             %s\n"),
	  ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32"
	  : ehdr->e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64"
	  : "\?\?\?");

  printf (gettext ("  Data:                              %s\n"),
	  ehdr->e_ident[EI_DATA] == ELFDATA2LSB
	  ? "2's complement, little endian"
	  : ehdr->e_ident[EI_DATA] == ELFDATA2MSB
	  ? "2's complement, big endian" : "\?\?\?");

  printf (gettext ("  Ident Version:                     %hhd %s\n"),
	  ehdr->e_ident[EI_VERSION],
	  ehdr->e_ident[EI_VERSION] == EV_CURRENT ? gettext ("(current)")
	  : "(\?\?\?)");

  char buf[512];
  printf (gettext ("  OS/ABI:                            %s\n"),
	  ebl_osabi_name (ebl, ehdr->e_ident[EI_OSABI], buf, sizeof (buf)));

  printf (gettext ("  ABI Version:                       %hhd\n"),
	  ehdr->e_ident[EI_ABIVERSION]);

  fputs_unlocked (gettext ("  Type:                              "), stdout);
  print_file_type (ehdr->e_type);

  printf (gettext ("  Machine:                           %s\n"), ebl->name);

  printf (gettext ("  Version:                           %d %s\n"),
	  ehdr->e_version,
	  ehdr->e_version  == EV_CURRENT ? gettext ("(current)") : "(\?\?\?)");

  printf (gettext ("  Entry point address:               %#" PRIx64 "\n"),
	  ehdr->e_entry);

  printf (gettext ("  Start of program headers:          %" PRId64 " %s\n"),
	  ehdr->e_phoff, gettext ("(bytes into file)"));

  printf (gettext ("  Start of section headers:          %" PRId64 " %s\n"),
	  ehdr->e_shoff, gettext ("(bytes into file)"));

  printf (gettext ("  Flags:                             %s\n"),
	  ebl_machine_flag_name (ebl, ehdr->e_flags, buf, sizeof (buf)));

  printf (gettext ("  Size of this header:               %" PRId16 " %s\n"),
	  ehdr->e_ehsize, gettext ("(bytes)"));

  printf (gettext ("  Size of program header entries:    %" PRId16 " %s\n"),
	  ehdr->e_phentsize, gettext ("(bytes)"));

  printf (gettext ("  Number of program headers entries: %" PRId16 "\n"),
	  ehdr->e_phnum);

  printf (gettext ("  Size of section header entries:    %" PRId16 " %s\n"),
	  ehdr->e_shentsize, gettext ("(bytes)"));

  printf (gettext ("  Number of section headers entries: %" PRId16),
	  ehdr->e_shnum);
  if (ehdr->e_shnum == 0)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
      if (shdr != NULL)
	printf (gettext (" (%" PRIu32 " in [0].sh_size)"),
		(uint32_t) shdr->sh_size);
      else
	fputs_unlocked (gettext (" ([0] not available)"), stdout);
    }
  fputc_unlocked ('\n', stdout);

  if (ehdr->e_shstrndx == SHN_XINDEX)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
      if (shdr != NULL)
	/* We managed to get the zeroth section.  */
	snprintf (buf, sizeof (buf), gettext (" (%" PRIu32 " in [0].sh_link)"),
		  (uint32_t) shdr->sh_link);
      else
	{
	  strncpy (buf, gettext (" ([0] not available)"), sizeof (buf));
	  buf[sizeof (buf) - 1] = '\0';
	}

      printf (gettext ("  Section header string table index: XINDEX%s\n\n"),
	      buf);
    }
  else
    printf (gettext ("  Section header string table index: %" PRId16 "\n\n"),
	    ehdr->e_shstrndx);
}


static const char *
get_visibility_type (int value)
{
  switch (value)
    {
    case STV_DEFAULT:
      return "DEFAULT";
    case STV_INTERNAL:
      return "INTERNAL";
    case STV_HIDDEN:
      return "HIDDEN";
    case STV_PROTECTED:
      return "PROTECTED";
    default:
      return "???";
    }
}


/* Print the section headers.  */
static void
print_shdr (Ebl *ebl, GElf_Ehdr *ehdr)
{
  size_t cnt;
  size_t shstrndx;

  if (! print_file_header)
    printf (gettext ("\
There are %d section headers, starting at offset %#" PRIx64 ":\n\
\n"),
	    ehdr->e_shnum, ehdr->e_shoff);

  /* Get the section header string table index.  */
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  puts (gettext ("Section Headers:"));

  if (ehdr->e_ident[EI_CLASS] == ELFCLASS32)
    puts (gettext ("[Nr] Name                 Type         Addr     Off    Size   ES Flags Lk Inf Al"));
  else
    puts (gettext ("[Nr] Name                 Type         Addr             Off      Size     ES Flags Lk Inf Al"));

  for (cnt = 0; cnt < shnum; ++cnt)
    {
      Elf_Scn *scn = elf_getscn (ebl->elf, cnt);

      if (scn == NULL)
	error (EXIT_FAILURE, 0, gettext ("cannot get section: %s"),
	       elf_errmsg (-1));

      /* Get the section header.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	error (EXIT_FAILURE, 0, gettext ("cannot get section header: %s"),
	       elf_errmsg (-1));

      char flagbuf[20];
      char *cp = flagbuf;
      if (shdr->sh_flags & SHF_WRITE)
	*cp++ = 'W';
      if (shdr->sh_flags & SHF_ALLOC)
	*cp++ = 'A';
      if (shdr->sh_flags & SHF_EXECINSTR)
	*cp++ = 'X';
      if (shdr->sh_flags & SHF_MERGE)
	*cp++ = 'M';
      if (shdr->sh_flags & SHF_STRINGS)
	*cp++ = 'S';
      if (shdr->sh_flags & SHF_INFO_LINK)
	*cp++ = 'I';
      if (shdr->sh_flags & SHF_LINK_ORDER)
	*cp++ = 'L';
      if (shdr->sh_flags & SHF_OS_NONCONFORMING)
	*cp++ = 'N';
      if (shdr->sh_flags & SHF_GROUP)
	*cp++ = 'G';
      if (shdr->sh_flags & SHF_TLS)
	*cp++ = 'T';
      if (shdr->sh_flags & SHF_ORDERED)
	*cp++ = 'O';
      if (shdr->sh_flags & SHF_EXCLUDE)
	*cp++ = 'E';
      *cp = '\0';

      char buf[128];
      printf ("[%2zu] %-20s %-12s %0*" PRIx64 " %0*" PRIx64 " %0*" PRIx64
	      " %2" PRId64 " %-5s %2" PRId32 " %3" PRId32
	      " %2" PRId64 "\n",
	      cnt,
	      elf_strptr (ebl->elf, shstrndx, shdr->sh_name)
	      ?: "<corrupt>",
	      ebl_section_type_name (ebl, shdr->sh_type, buf, sizeof (buf)),
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 8 : 16, shdr->sh_addr,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 6 : 8, shdr->sh_offset,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 6 : 8, shdr->sh_size,
	      shdr->sh_entsize, flagbuf, shdr->sh_link, shdr->sh_info,
	      shdr->sh_addralign);
    }

  fputc_unlocked ('\n', stdout);
}


/* Print the program header.  */
static void
print_phdr (Ebl *ebl, GElf_Ehdr *ehdr)
{
  if (ehdr->e_phnum == 0)
    /* No program header, this is OK in relocatable objects.  */
    return;

  puts (gettext ("Program Headers:"));
  if (ehdr->e_ident[EI_CLASS] == ELFCLASS32)
    puts (gettext ("\
  Type           Offset   VirtAddr   PhysAddr   FileSiz  MemSiz   Flg Align"));
  else
    puts (gettext ("\
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align"));

  /* Process all program headers.  */
  bool has_relro = false;
  GElf_Addr relro_from = 0;
  GElf_Addr relro_to = 0;
  for (size_t cnt = 0; cnt < ehdr->e_phnum; ++cnt)
    {
      char buf[128];
      GElf_Phdr mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, cnt, &mem);

      /* If for some reason the header cannot be returned show this.  */
      if (phdr == NULL)
	{
	  puts ("  ???");
	  continue;
	}

      printf ("  %-14s 0x%06" PRIx64 " 0x%0*" PRIx64 " 0x%0*" PRIx64
	      " 0x%06" PRIx64 " 0x%06" PRIx64 " %c%c%c 0x%" PRIx64 "\n",
	      ebl_segment_type_name (ebl, phdr->p_type, buf, sizeof (buf)),
	      phdr->p_offset,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 8 : 16, phdr->p_vaddr,
	      ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 8 : 16, phdr->p_paddr,
	      phdr->p_filesz,
	      phdr->p_memsz,
	      phdr->p_flags & PF_R ? 'R' : ' ',
	      phdr->p_flags & PF_W ? 'W' : ' ',
	      phdr->p_flags & PF_X ? 'E' : ' ',
	      phdr->p_align);

      if (phdr->p_type == PT_INTERP)
	{
	  /* We can show the user the name of the interpreter.  */
	  size_t maxsize;
	  char *filedata = elf_rawfile (ebl->elf, &maxsize);

	  if (filedata != NULL && phdr->p_offset < maxsize)
	    printf (gettext ("\t[Requesting program interpreter: %s]\n"),
		    filedata + phdr->p_offset);
	}
      else if (phdr->p_type == PT_GNU_RELRO)
	{
	  has_relro = true;
	  relro_from = phdr->p_vaddr;
	  relro_to = relro_from + phdr->p_memsz;
	}
    }

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  puts (gettext ("\n Section to Segment mapping:\n  Segment Sections..."));

  for (size_t cnt = 0; cnt < ehdr->e_phnum; ++cnt)
    {
      /* Print the segment number.  */
      printf ("   %2.2zu     ", cnt);

      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, cnt, &phdr_mem);
      /* This must not happen.  */
      if (phdr == NULL)
	error (EXIT_FAILURE, 0, gettext ("cannot get program header: %s"),
	       elf_errmsg (-1));

      /* Iterate over the sections.  */
      bool in_relro = false;
      bool in_ro = false;
      for (size_t inner = 1; inner < shnum; ++inner)
	{
	  Elf_Scn *scn = elf_getscn (ebl->elf, inner);
	  /* This should not happen.  */
	  if (scn == NULL)
	    error (EXIT_FAILURE, 0, gettext ("cannot get section: %s"),
		   elf_errmsg (-1));

	  /* Get the section header.  */
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr == NULL)
	    error (EXIT_FAILURE, 0,
		   gettext ("cannot get section header: %s"),
		   elf_errmsg (-1));

	  if (shdr->sh_size > 0
	      /* Compare allocated sections by VMA, unallocated
		 sections by file offset.  */
	      && (shdr->sh_flags & SHF_ALLOC
		  ? (shdr->sh_addr >= phdr->p_vaddr
		     && (shdr->sh_addr + shdr->sh_size
			 <= phdr->p_vaddr + phdr->p_memsz))
		  : (shdr->sh_offset >= phdr->p_offset
		     && (shdr->sh_offset + shdr->sh_size
			 <= phdr->p_offset + phdr->p_filesz))))
	    {
	      if (has_relro && !in_relro
		  && shdr->sh_addr >= relro_from
		  && shdr->sh_addr + shdr->sh_size <= relro_to)
		{
		  fputs_unlocked (" [RELRO:", stdout);
		  in_relro = true;
		}
	      else if (has_relro && in_relro && shdr->sh_addr >= relro_to)
		{
		  fputs_unlocked ("]", stdout);
		  in_relro =  false;
		}
	      else if (has_relro && in_relro
		       && shdr->sh_addr + shdr->sh_size > relro_to)
		fputs_unlocked ("] <RELRO:", stdout);
	      else if (phdr->p_type == PT_LOAD && (phdr->p_flags & PF_W) == 0)
		{
		  if (!in_ro)
		    {
		      fputs_unlocked (" [RO:", stdout);
		      in_ro = true;
		    }
		}
	      else
		{
		  /* Determine the segment this section is part of.  */
		  size_t cnt2;
		  GElf_Phdr *phdr2 = NULL;
		  for (cnt2 = 0; cnt2 < ehdr->e_phnum; ++cnt2)
		    {
		      GElf_Phdr phdr2_mem;
		      phdr2 = gelf_getphdr (ebl->elf, cnt2, &phdr2_mem);

		      if (phdr2 != NULL && phdr2->p_type == PT_LOAD
			  && shdr->sh_addr >= phdr2->p_vaddr
			  && (shdr->sh_addr + shdr->sh_size
			      <= phdr2->p_vaddr + phdr2->p_memsz))
			break;
		    }

		  if (cnt2 < ehdr->e_phnum)
		    {
		      if ((phdr2->p_flags & PF_W) == 0 && !in_ro)
			{
			  fputs_unlocked (" [RO:", stdout);
			  in_ro = true;
			}
		      else if ((phdr2->p_flags & PF_W) != 0 && in_ro)
			{
			  fputs_unlocked ("]", stdout);
			  in_ro = false;
			}
		    }
		}

	      printf (" %s",
		      elf_strptr (ebl->elf, shstrndx, shdr->sh_name));

	      /* Signal that this sectin is only partially covered.  */
	      if (has_relro && in_relro
		       && shdr->sh_addr + shdr->sh_size > relro_to)
		{
		  fputs_unlocked (">", stdout);
		  in_relro =  false;
		}
	    }
	}
      if (in_relro || in_ro)
	fputs_unlocked ("]", stdout);

      /* Finish the line.  */
      fputc_unlocked ('\n', stdout);
    }
}


static void
handle_scngrp (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);

  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  if (data == NULL || data->d_size < sizeof (Elf32_Word) || symshdr == NULL
      || symdata == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  Elf32_Word *grpref = (Elf32_Word *) data->d_buf;

  GElf_Sym sym_mem;
  printf ((grpref[0] & GRP_COMDAT)
	  ? ngettext ("\
\nCOMDAT section group [%2zu] '%s' with signature '%s' contains %zu entry:\n",
		      "\
\nCOMDAT section group [%2zu] '%s' with signature '%s' contains %zu entries:\n",
		      data->d_size / sizeof (Elf32_Word) - 1)
	  : ngettext ("\
\nSection group [%2zu] '%s' with signature '%s' contains %zu entry:\n", "\
\nSection group [%2zu] '%s' with signature '%s' contains %zu entries:\n",
		      data->d_size / sizeof (Elf32_Word) - 1),
	  elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  elf_strptr (ebl->elf, symshdr->sh_link,
		      gelf_getsym (symdata, shdr->sh_info, &sym_mem)->st_name)
	  ?: gettext ("<INVALID SYMBOL>"),
	  data->d_size / sizeof (Elf32_Word) - 1);

  for (size_t cnt = 1; cnt < data->d_size / sizeof (Elf32_Word); ++cnt)
    {
      GElf_Shdr grpshdr_mem;
      GElf_Shdr *grpshdr = gelf_getshdr (elf_getscn (ebl->elf, grpref[cnt]),
					 &grpshdr_mem);

      const char *str;
      printf ("  [%2u] %s\n",
	      grpref[cnt],
	      grpshdr != NULL
	      && (str = elf_strptr (ebl->elf, shstrndx, grpshdr->sh_name))
	      ? str : gettext ("<INVALID SECTION>"));
    }
}


static void
print_scngrp (Ebl *ebl)
{
  /* Find all relocation sections and handle them.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
       /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL && shdr->sh_type == SHT_GROUP)
	handle_scngrp (ebl, scn, shdr);
    }
}


static const struct flags
{
  int mask;
  const char *str;
} dt_flags[] =
  {
    { DF_ORIGIN, "ORIGIN" },
    { DF_SYMBOLIC, "SYMBOLIC" },
    { DF_TEXTREL, "TEXTREL" },
    { DF_BIND_NOW, "BIND_NOW" },
    { DF_STATIC_TLS, "STATIC_TLS" }
  };
static const int ndt_flags = sizeof (dt_flags) / sizeof (dt_flags[0]);

static const struct flags dt_flags_1[] =
  {
    { DF_1_NOW, "NOW" },
    { DF_1_GLOBAL, "GLOBAL" },
    { DF_1_GROUP, "GROUP" },
    { DF_1_NODELETE, "NODELETE" },
    { DF_1_LOADFLTR, "LOADFLTR" },
    { DF_1_INITFIRST, "INITFIRST" },
    { DF_1_NOOPEN, "NOOPEN" },
    { DF_1_ORIGIN, "ORIGIN" },
    { DF_1_DIRECT, "DIRECT" },
    { DF_1_TRANS, "TRANS" },
    { DF_1_INTERPOSE, "INTERPOSE" },
    { DF_1_NODEFLIB, "NODEFLIB" },
    { DF_1_NODUMP, "NODUMP" },
    { DF_1_CONFALT, "CONFALT" },
    { DF_1_ENDFILTEE, "ENDFILTEE" },
    { DF_1_DISPRELDNE, "DISPRELDNE" },
    { DF_1_DISPRELPND, "DISPRELPND" },
  };
static const int ndt_flags_1 = sizeof (dt_flags_1) / sizeof (dt_flags_1[0]);

static const struct flags dt_feature_1[] =
  {
    { DTF_1_PARINIT, "PARINIT" },
    { DTF_1_CONFEXP, "CONFEXP" }
  };
static const int ndt_feature_1 = (sizeof (dt_feature_1)
				  / sizeof (dt_feature_1[0]));

static const struct flags dt_posflag_1[] =
  {
    { DF_P1_LAZYLOAD, "LAZYLOAD" },
    { DF_P1_GROUPPERM, "GROUPPERM" }
  };
static const int ndt_posflag_1 = (sizeof (dt_posflag_1)
				  / sizeof (dt_posflag_1[0]));


static void
print_flags (int class, GElf_Xword d_val, const struct flags *flags,
		int nflags)
{
  bool first = true;
  int cnt;

  for (cnt = 0; cnt < nflags; ++cnt)
    if (d_val & flags[cnt].mask)
      {
	if (!first)
	  putchar_unlocked (' ');
	fputs_unlocked (flags[cnt].str, stdout);
	d_val &= ~flags[cnt].mask;
	first = false;
      }

  if (d_val != 0)
    {
      if (!first)
	putchar_unlocked (' ');
      printf ("%#0*" PRIx64, class == ELFCLASS32 ? 10 : 18, d_val);
    }

  putchar_unlocked ('\n');
}


static void
print_dt_flags (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_flags, ndt_flags);
}


static void
print_dt_flags_1 (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_flags_1, ndt_flags_1);
}


static void
print_dt_feature_1 (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_feature_1, ndt_feature_1);
}


static void
print_dt_posflag_1 (int class, GElf_Xword d_val)
{
  print_flags (class, d_val, dt_posflag_1, ndt_posflag_1);
}


static void
handle_dynamic (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  GElf_Shdr glink;
  Elf_Data *data;
  size_t cnt;
  size_t shstrndx;

  /* Get the data of the section.  */
  data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  printf (ngettext ("\
\nDynamic segment contains %lu entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nDynamic segment contains %lu entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    shdr->sh_size / shdr->sh_entsize),
	  (unsigned long int) (shdr->sh_size / shdr->sh_entsize),
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));
  fputs_unlocked (gettext ("  Type              Value\n"), stdout);

  for (cnt = 0; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      GElf_Dyn dynmem;
      GElf_Dyn *dyn = gelf_getdyn (data, cnt, &dynmem);
      if (dyn == NULL)
	break;

      char buf[64];
      printf ("  %-17s ",
	      ebl_dynamic_tag_name (ebl, dyn->d_tag, buf, sizeof (buf)));

      switch (dyn->d_tag)
	{
	case DT_NULL:
	case DT_DEBUG:
	case DT_BIND_NOW:
	case DT_TEXTREL:
	  /* No further output.  */
	  fputc_unlocked ('\n', stdout);
	  break;

	case DT_NEEDED:
	  printf (gettext ("Shared library: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_SONAME:
	  printf (gettext ("Library soname: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_RPATH:
	  printf (gettext ("Library rpath: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_RUNPATH:
	  printf (gettext ("Library runpath: [%s]\n"),
		  elf_strptr (ebl->elf, shdr->sh_link, dyn->d_un.d_val));
	  break;

	case DT_PLTRELSZ:
	case DT_RELASZ:
	case DT_STRSZ:
	case DT_RELSZ:
	case DT_RELAENT:
	case DT_SYMENT:
	case DT_RELENT:
	case DT_PLTPADSZ:
	case DT_MOVEENT:
	case DT_MOVESZ:
	case DT_INIT_ARRAYSZ:
	case DT_FINI_ARRAYSZ:
	case DT_SYMINSZ:
	case DT_SYMINENT:
	case DT_GNU_CONFLICTSZ:
	case DT_GNU_LIBLISTSZ:
	  printf (gettext ("%" PRId64 " (bytes)\n"), dyn->d_un.d_val);
	  break;

	case DT_VERDEFNUM:
	case DT_VERNEEDNUM:
	case DT_RELACOUNT:
	case DT_RELCOUNT:
	  printf ("%" PRId64 "\n", dyn->d_un.d_val);
	  break;

	case DT_PLTREL:
	  puts (ebl_dynamic_tag_name (ebl, dyn->d_un.d_val, NULL, 0));
	  break;

	case DT_FLAGS:
	  print_dt_flags (class, dyn->d_un.d_val);
	  break;

	case DT_FLAGS_1:
	  print_dt_flags_1 (class, dyn->d_un.d_val);
	  break;

	case DT_FEATURE_1:
	  print_dt_feature_1 (class, dyn->d_un.d_val);
	  break;

	case DT_POSFLAG_1:
	  print_dt_posflag_1 (class, dyn->d_un.d_val);
	  break;

	default:
	  printf ("%#0*" PRIx64 "\n",
		  class == ELFCLASS32 ? 10 : 18, dyn->d_un.d_val);
	  break;
	}
    }
}


/* Print the dynamic segment.  */
static void
print_dynamic (Ebl *ebl, GElf_Ehdr *ehdr)
{
  for (int i = 0; i < ehdr->e_phnum; ++i)
    {
      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, i, &phdr_mem);

      if (phdr != NULL && phdr->p_type == PT_DYNAMIC)
	{
	  Elf_Scn *scn = gelf_offscn (ebl->elf, phdr->p_offset);
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr != NULL && shdr->sh_type == SHT_DYNAMIC)
	    handle_dynamic (ebl, scn, shdr);
	  break;
	}
    }
}


/* Print relocations.  */
static void
print_relocs (Ebl *ebl)
{
  /* Find all relocation sections and handle them.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
       /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL)
	{
	  if (shdr->sh_type == SHT_REL)
	    handle_relocs_rel (ebl, scn, shdr);
	  else if (shdr->sh_type == SHT_RELA)
	    handle_relocs_rela (ebl, scn, shdr);
	}
    }
}


/* Handle a relocation section.  */
static void
handle_relocs_rel (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  int nentries = shdr->sh_size / shdr->sh_entsize;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the symbol table information.  */
  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  /* Get the section header of the section the relocations are for.  */
  GElf_Shdr destshdr_mem;
  GElf_Shdr *destshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_info),
				      &destshdr_mem);

  if (symshdr == NULL || symdata == NULL || destshdr == NULL)
    {
      printf (gettext ("\nInvalid symbol table at offset %#0" PRIx64 "\n"),
	      shdr->sh_offset);
      return;
    }

  /* Search for the optional extended section index table.  */
  Elf_Scn *xndxscn = NULL;
  Elf_Data *xndxdata = NULL;
  while ((xndxscn = elf_nextscn (ebl->elf, xndxscn)) != NULL)
    {
      GElf_Shdr xndxshdr_mem;
      GElf_Shdr *xndxshdr = gelf_getshdr (xndxscn, &xndxshdr_mem);
      if (xndxshdr != NULL && xndxshdr->sh_type == SHT_SYMTAB_SHNDX
	  && xndxshdr->sh_link == elf_ndxscn (symscn))
	{
	  /* Found it.  */
	  xndxdata = elf_getdata (xndxscn, NULL);
	  break;
	}
    }

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  if (shdr->sh_info != 0)
    printf (ngettext ("\
\nRelocation section [%2u] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
		    "\
\nRelocation section [%2u] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
		      nentries),
	    (unsigned int) elf_ndxscn (scn),
	    elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	    (unsigned int) shdr->sh_info,
	    elf_strptr (ebl->elf, shstrndx, destshdr->sh_name),
	    shdr->sh_offset,
	    nentries);
  else
    /* The .rel.dyn section does not refer to a specific section but
       instead of section index zero.  Do not try to print a section
       name.  */
    printf (ngettext ("\
\nRelocation section [%2u] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
		    "\
\nRelocation section [%2u] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
		      nentries),
	    (unsigned int) elf_ndxscn (scn),
	    elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	    shdr->sh_offset,
	    nentries);
  fputs_unlocked (class == ELFCLASS32
		  ? gettext ("\
  Offset      Type                 Value       Name\n")
		  : gettext ("\
  Offset              Type                 Value               Name\n"),
	 stdout);

  for (int cnt = 0; cnt < nentries; ++cnt)
    {
      GElf_Rel relmem;
      GElf_Rel *rel = gelf_getrel (data, cnt, &relmem);
      if (rel != NULL)
	{
	  char buf[128];
	  GElf_Sym symmem;
	  Elf32_Word xndx;
	  GElf_Sym *sym = gelf_getsymshndx (symdata, xndxdata,
					    GELF_R_SYM (rel->r_info),
					    &symmem, &xndx);
	  if (sym == NULL)
	    printf ("  %#0*" PRIx64 "  %-20s <%s %ld>\n",
		    class == ELFCLASS32 ? 10 : 18, rel->r_offset,
		    ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
		    /* Avoid the leading R_ which isn't carrying any
		       information.  */
		    ? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					   buf, sizeof (buf)) + 2
		    : gettext ("<INVALID RELOC>"),
		    gettext ("INVALID SYMBOL"),
		    (long int) GELF_R_SYM (rel->r_info));
	  else if (GELF_ST_TYPE (sym->st_info) != STT_SECTION)
	    printf ("  %#0*" PRIx64 "  %-20s %#0*" PRIx64 "  %s\n",
		    class == ELFCLASS32 ? 10 : 18, rel->r_offset,
		    ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
		    /* Avoid the leading R_ which isn't carrying any
		       information.  */
		    ? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					   buf, sizeof (buf)) + 2
		    : gettext ("<INVALID RELOC>"),
		    class == ELFCLASS32 ? 10 : 18, sym->st_value,
		    elf_strptr (ebl->elf, symshdr->sh_link, sym->st_name));
	  else
	    {
	      destshdr = gelf_getshdr (elf_getscn (ebl->elf,
						   sym->st_shndx == SHN_XINDEX
						   ? xndx : sym->st_shndx),
				       &destshdr_mem);

	      if (shdr == NULL)
		printf ("  %#0*" PRIx64 "  %-20s <%s %ld>\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			gettext ("INVALID SECTION"),
			(long int) (sym->st_shndx == SHN_XINDEX
				    ? xndx : sym->st_shndx));
	      else
		printf ("  %#0*" PRIx64 "  %-20s %#0*" PRIx64 "  %s\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			class == ELFCLASS32 ? 10 : 18, sym->st_value,
			elf_strptr (ebl->elf, shstrndx, destshdr->sh_name));
	    }
	}
    }
}


/* Handle a relocation section.  */
static void
handle_relocs_rela (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  int nentries = shdr->sh_size / shdr->sh_entsize;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the symbol table information.  */
  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  /* Get the section header of the section the relocations are for.  */
  GElf_Shdr destshdr_mem;
  GElf_Shdr *destshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_info),
				      &destshdr_mem);

  if (symshdr == NULL || symdata == NULL || destshdr == NULL)
    {
      printf (gettext ("\nInvalid symbol table at offset %#0" PRIx64 "\n"),
	      shdr->sh_offset);
      return;
    }

  /* Search for the optional extended section index table.  */
  Elf_Data *xndxdata = NULL;
  Elf_Scn *xndxscn = NULL;
  while ((xndxscn = elf_nextscn (ebl->elf, xndxscn)) != NULL)
    {
      GElf_Shdr xndxshdr_mem;
      GElf_Shdr *xndxshdr = gelf_getshdr (xndxscn, &xndxshdr_mem);
      if (xndxshdr != NULL && xndxshdr->sh_type == SHT_SYMTAB_SHNDX
	  && xndxshdr->sh_link == elf_ndxscn (symscn))
	{
	  /* Found it.  */
	  xndxdata = elf_getdata (xndxscn, NULL);
	  break;
	}
    }

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  printf (ngettext ("\
\nRelocation section [%2zu] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
		    "\
\nRelocation section [%2zu] '%s' for section [%2u] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
		    nentries),
	  elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  (unsigned int) shdr->sh_info,
	  elf_strptr (ebl->elf, shstrndx, destshdr->sh_name),
	  shdr->sh_offset,
	  nentries);
  fputs_unlocked (class == ELFCLASS32
		  ? gettext ("\
  Offset      Type            Value       Addend Name\n")
		  : gettext ("\
  Offset              Type            Value               Addend Name\n"),
		  stdout);

  for (int cnt = 0; cnt < nentries; ++cnt)
    {
      GElf_Rela relmem;
      GElf_Rela *rel = gelf_getrela (data, cnt, &relmem);
      if (rel != NULL)
	{
	  char buf[64];
	  GElf_Sym symmem;
	  Elf32_Word xndx;
	  GElf_Sym *sym = gelf_getsymshndx (symdata, xndxdata,
					    GELF_R_SYM (rel->r_info),
					    &symmem, &xndx);

	  if (sym == NULL)
	    printf ("  %#0*" PRIx64 "  %-15s <%s %ld>\n",
		    class == ELFCLASS32 ? 10 : 18, rel->r_offset,
		    ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
		    /* Avoid the leading R_ which isn't carrying any
		       information.  */
		    ? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					   buf, sizeof (buf)) + 2
		    : gettext ("<INVALID RELOC>"),
		    gettext ("INVALID SYMBOL"),
		    (long int) GELF_R_SYM (rel->r_info));
	  else if (GELF_ST_TYPE (sym->st_info) != STT_SECTION)
	    printf ("\
  %#0*" PRIx64 "  %-15s %#0*" PRIx64 "  %+6" PRId64 " %s\n",
		    class == ELFCLASS32 ? 10 : 18, rel->r_offset,
		    ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
		    /* Avoid the leading R_ which isn't carrying any
		       information.  */
		    ? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					   buf, sizeof (buf)) + 2
		    : gettext ("<INVALID RELOC>"),
		    class == ELFCLASS32 ? 10 : 18, sym->st_value,
		    rel->r_addend,
		    elf_strptr (ebl->elf, symshdr->sh_link, sym->st_name));
	  else
	    {
	      destshdr = gelf_getshdr (elf_getscn (ebl->elf,
						   sym->st_shndx == SHN_XINDEX
						   ? xndx : sym->st_shndx),
				       &destshdr_mem);

	      if (shdr == NULL)
		printf ("  %#0*" PRIx64 "  %-15s <%s %ld>\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			gettext ("INVALID SECTION"),
			(long int) (sym->st_shndx == SHN_XINDEX
				    ? xndx : sym->st_shndx));
	      else
		printf ("\
  %#0*" PRIx64 "  %-15s %#0*" PRIx64 "  %+6" PRId64 " %s\n",
			class == ELFCLASS32 ? 10 : 18, rel->r_offset,
			ebl_reloc_type_check (ebl, GELF_R_TYPE (rel->r_info))
			/* Avoid the leading R_ which isn't carrying any
			   information.  */
			? ebl_reloc_type_name (ebl, GELF_R_TYPE (rel->r_info),
					       buf, sizeof (buf)) + 2
			: gettext ("<INVALID RELOC>"),
			class == ELFCLASS32 ? 10 : 18, sym->st_value,
			rel->r_addend,
			elf_strptr (ebl->elf, shstrndx, destshdr->sh_name));
	    }
	}
    }
}


/* Print the program header.  */
static void
print_symtab (Ebl *ebl, int type)
{
  /* Find the symbol table(s).  For this we have to search through the
     section table.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL && shdr->sh_type == (GElf_Word) type)
	handle_symtab (ebl, scn, shdr);
    }
}


static void
handle_symtab (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  Elf_Data *versym_data = NULL;
  Elf_Data *verneed_data = NULL;
  Elf_Data *verdef_data = NULL;
  Elf_Data *xndx_data = NULL;
  int class = gelf_getclass (ebl->elf);
  Elf32_Word verneed_stridx = 0;
  Elf32_Word verdef_stridx = 0;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Find out whether we have other sections we might need.  */
  Elf_Scn *runscn = NULL;
  while ((runscn = elf_nextscn (ebl->elf, runscn)) != NULL)
    {
      GElf_Shdr runshdr_mem;
      GElf_Shdr *runshdr = gelf_getshdr (runscn, &runshdr_mem);

      if (runshdr != NULL)
	{
	  if (runshdr->sh_type == SHT_GNU_versym
	      && runshdr->sh_link == elf_ndxscn (scn))
	    /* Bingo, found the version information.  Now get the data.  */
	    versym_data = elf_getdata (runscn, NULL);
	  else if (runshdr->sh_type == SHT_GNU_verneed)
	    {
	      /* This is the information about the needed versions.  */
	      verneed_data = elf_getdata (runscn, NULL);
	      verneed_stridx = runshdr->sh_link;
	    }
	  else if (runshdr->sh_type == SHT_GNU_verdef)
	    {
	      /* This is the information about the defined versions.  */
	      verdef_data = elf_getdata (runscn, NULL);
	      verdef_stridx = runshdr->sh_link;
	    }
	  else if (runshdr->sh_type == SHT_SYMTAB_SHNDX
	      && runshdr->sh_link == elf_ndxscn (scn))
	    /* Extended section index.  */
	    xndx_data = elf_getdata (runscn, NULL);
	}
    }

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  /* Now we can compute the number of entries in the section.  */
  unsigned int nsyms = data->d_size / (class == ELFCLASS32
				       ? sizeof (Elf32_Sym)
				       : sizeof (Elf64_Sym));

  printf (ngettext ("\nSymbol table [%2u] '%s' contains %u entry:\n",
		    "\nSymbol table [%2u] '%s' contains %u entries:\n",
		    nsyms),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name), nsyms);
  GElf_Shdr glink;
  printf (ngettext (" %lu local symbol  String table: [%2u] '%s'\n",
		    " %lu local symbols  String table: [%2u] '%s'\n",
		    shdr->sh_info),
	  (unsigned long int) shdr->sh_info,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  fputs_unlocked (class == ELFCLASS32
		  ? gettext ("\
  Num:    Value   Size Type    Bind   Vis          Ndx Name\n")
		  : gettext ("\
  Num:            Value   Size Type    Bind   Vis          Ndx Name\n"),
		  stdout);

  for (unsigned int cnt = 0; cnt < nsyms; ++cnt)
    {
      char typebuf[64];
      char bindbuf[64];
      char scnbuf[64];
      Elf32_Word xndx;
      GElf_Sym sym_mem;
      GElf_Sym *sym = gelf_getsymshndx (data, xndx_data, cnt, &sym_mem, &xndx);

      if (sym == NULL)
	continue;

      /* Determine the real section index.  */
      if (sym->st_shndx != SHN_XINDEX)
	xndx = sym->st_shndx;

      printf (gettext ("\
%5u: %0*" PRIx64 " %6" PRId64 " %-7s %-6s %-9s %6s %s"),
	      cnt,
	      class == ELFCLASS32 ? 8 : 16,
	      sym->st_value,
	      sym->st_size,
	      ebl_symbol_type_name (ebl, GELF_ST_TYPE (sym->st_info),
				    typebuf, sizeof (typebuf)),
	      ebl_symbol_binding_name (ebl, GELF_ST_BIND (sym->st_info),
				       bindbuf, sizeof (bindbuf)),
	      get_visibility_type (GELF_ST_VISIBILITY (sym->st_other)),
	      ebl_section_name (ebl, sym->st_shndx, xndx, scnbuf,
				sizeof (scnbuf), NULL, shnum),
	      elf_strptr (ebl->elf, shdr->sh_link, sym->st_name));

      if (versym_data != NULL)
	{
	  /* Get the version information.  */
	  GElf_Versym versym_mem;
	  GElf_Versym *versym = gelf_getversym (versym_data, cnt, &versym_mem);

	  if (versym != NULL && ((*versym & 0x8000) != 0 || *versym > 1))
	    {
	      bool is_nobits = false;
	      bool check_def = xndx != SHN_UNDEF;

	      if (xndx < SHN_LORESERVE || sym->st_shndx == SHN_XINDEX)
		{
		  GElf_Shdr symshdr_mem;
		  GElf_Shdr *symshdr =
		    gelf_getshdr (elf_getscn (ebl->elf, xndx), &symshdr_mem);

		  is_nobits = (symshdr != NULL
			       && symshdr->sh_type == SHT_NOBITS);
		}

	      if (is_nobits || ! check_def)
		{
		  /* We must test both.  */
		  GElf_Vernaux vernaux_mem;
		  GElf_Vernaux *vernaux = NULL;
		  size_t vn_offset = 0;

		  GElf_Verneed verneed_mem;
		  GElf_Verneed *verneed = gelf_getverneed (verneed_data, 0,
							   &verneed_mem);
		  while (verneed != NULL)
		    {
		      size_t vna_offset = vn_offset;

		      vernaux = gelf_getvernaux (verneed_data,
						 vna_offset += verneed->vn_aux,
						 &vernaux_mem);
		      while (vernaux != NULL
			     && vernaux->vna_other != *versym
			     && vernaux->vna_next != 0)
			{
			  /* Update the offset.  */
			  vna_offset += vernaux->vna_next;

			  vernaux = (vernaux->vna_next == 0
				     ? NULL
				     : gelf_getvernaux (verneed_data,
							vna_offset,
							&vernaux_mem));
			}

		      /* Check whether we found the version.  */
		      if (vernaux != NULL && vernaux->vna_other == *versym)
			/* Found it.  */
			break;

		      vn_offset += verneed->vn_next;
		      verneed = (verneed->vn_next == 0
				 ? NULL
				 : gelf_getverneed (verneed_data, vn_offset,
						    &verneed_mem));
		    }

		  if (vernaux != NULL && vernaux->vna_other == *versym)
		    {
		      printf ("@%s (%u)",
			      elf_strptr (ebl->elf, verneed_stridx,
					  vernaux->vna_name),
			      (unsigned int) vernaux->vna_other);
		      check_def = 0;
		    }
		  else if (! is_nobits)
		    error (0, 0, gettext ("bad dynamic symbol"));
		  else
		    check_def = 1;
		}

	      if (check_def && *versym != 0x8001)
		{
		  /* We must test both.  */
		  size_t vd_offset = 0;

		  GElf_Verdef verdef_mem;
		  GElf_Verdef *verdef = gelf_getverdef (verdef_data, 0,
							&verdef_mem);
		  while (verdef != NULL)
		    {
		      if (verdef->vd_ndx == (*versym & 0x7fff))
			/* Found the definition.  */
			break;

		      vd_offset += verdef->vd_next;
		      verdef = (verdef->vd_next == 0
				? NULL
				: gelf_getverdef (verdef_data, vd_offset,
						  &verdef_mem));
		    }

		  if (verdef != NULL)
		    {
		      GElf_Verdaux verdaux_mem;
		      GElf_Verdaux *verdaux
			= gelf_getverdaux (verdef_data,
					   vd_offset + verdef->vd_aux,
					   &verdaux_mem);

		      if (verdaux != NULL)
			printf ((*versym & 0x8000) ? "@%s" : "@@%s",
				elf_strptr (ebl->elf, verdef_stridx,
					    verdaux->vda_name));
		    }
		}
	    }
	}

      putchar_unlocked ('\n');
    }
}


/* Print version information.  */
static void
print_verinfo (Ebl *ebl)
{
  /* Find the version information sections.  For this we have to
     search through the section table.  */
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      /* Handle the section if it is part of the versioning handling.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL)
	{
	  if (shdr->sh_type == SHT_GNU_verneed)
	    handle_verneed (ebl, scn, shdr);
	  else if (shdr->sh_type == SHT_GNU_verdef)
	    handle_verdef (ebl, scn, shdr);
	  else if (shdr->sh_type == SHT_GNU_versym)
	    handle_versym (ebl, scn, shdr);
	}
    }
}


static const char *
get_ver_flags (unsigned int flags)
{
  static char buf[32];
  char *endp;

  if (flags == 0)
    return gettext ("none");

  if (flags & VER_FLG_BASE)
    endp = stpcpy (buf, "BASE ");
  else
    endp = buf;

  if (flags & VER_FLG_WEAK)
    {
      if (endp != buf)
        endp = stpcpy (endp, "| ");

      endp = stpcpy (endp, "WEAK ");
    }

  if (flags & ~(VER_FLG_BASE | VER_FLG_WEAK))
    {
      strncpy (endp, gettext ("| <unknown>"), buf + sizeof (buf) - endp);
      buf[sizeof (buf) - 1] = '\0';
    }

  return buf;
}


static void
handle_verneed (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  GElf_Shdr glink;
  printf (ngettext ("\
\nVersion needs section [%2u] '%s' contains %d entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nVersion needs section [%2u] '%s' contains %d entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    shdr->sh_info),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name), shdr->sh_info,
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  unsigned int offset = 0;
  for (int cnt = shdr->sh_info; --cnt >= 0; )
    {
      /* Get the data at the next offset.  */
      GElf_Verneed needmem;
      GElf_Verneed *need = gelf_getverneed (data, offset, &needmem);
      if (need == NULL)
	break;

      printf (gettext ("  %#06x: Version: %hu  File: %s  Cnt: %hu\n"),
	      offset, (unsigned short int) need->vn_version,
	      elf_strptr (ebl->elf, shdr->sh_link, need->vn_file),
	      (unsigned short int) need->vn_cnt);

      unsigned int auxoffset = offset + need->vn_aux;
      for (int cnt2 = need->vn_cnt; --cnt2 >= 0; )
	{
	  GElf_Vernaux auxmem;
	  GElf_Vernaux *aux = gelf_getvernaux (data, auxoffset, &auxmem);
	  if (aux == NULL)
	    break;

	  printf (gettext ("  %#06x: Name: %s  Flags: %s  Version: %hu\n"),
		  auxoffset,
		  elf_strptr (ebl->elf, shdr->sh_link, aux->vna_name),
		  get_ver_flags (aux->vna_flags),
		  (unsigned short int) aux->vna_other);

	  auxoffset += aux->vna_next;
	}

      /* Find the next offset.  */
      offset += need->vn_next;
    }
}


static void
handle_verdef (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  int class = gelf_getclass (ebl->elf);
  GElf_Shdr glink;
  printf (ngettext ("\
\nVersion definition section [%2u] '%s' contains %d entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nVersion definition section [%2u] '%s' contains %d entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    shdr->sh_info),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  shdr->sh_info,
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  unsigned int offset = 0;
  for (int cnt = shdr->sh_info; --cnt >= 0; )
    {
      /* Get the data at the next offset.  */
      GElf_Verdef defmem;
      GElf_Verdef *def = gelf_getverdef (data, offset, &defmem);
      if (def == NULL)
	break;

      unsigned int auxoffset = offset + def->vd_aux;
      GElf_Verdaux auxmem;
      GElf_Verdaux *aux = gelf_getverdaux (data, auxoffset, &auxmem);
      if (aux == NULL)
	break;

      printf (gettext ("\
  %#06x: Version: %hd  Flags: %s  Index: %hd  Cnt: %hd  Name: %s\n"),
	      offset, def->vd_version,
	      get_ver_flags (def->vd_flags),
	      def->vd_ndx,
	      def->vd_cnt,
	      elf_strptr (ebl->elf, shdr->sh_link, aux->vda_name));

      auxoffset += aux->vda_next;
      for (int cnt2 = 1; cnt2 < def->vd_cnt; ++cnt2)
	{
	  aux = gelf_getverdaux (data, auxoffset, &auxmem);
	  if (aux == NULL)
	    break;

	  printf (gettext ("  %#06x: Parent %d: %s\n"),
		  auxoffset, cnt2,
		  elf_strptr (ebl->elf, shdr->sh_link, aux->vda_name));

	  auxoffset += aux->vda_next;
	}

      /* Find the next offset.  */
      offset += def->vd_next;
    }
}


static void
handle_versym (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr)
{
  int class = gelf_getclass (ebl->elf);
  const char **vername;
  const char **filename;

  /* Get the data of the section.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    return;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  /* We have to find the version definition section and extract the
     version names.  */
  Elf_Scn *defscn = NULL;
  Elf_Scn *needscn = NULL;

  Elf_Scn *verscn = NULL;
  while ((verscn = elf_nextscn (ebl->elf, verscn)) != NULL)
    {
      GElf_Shdr vershdr_mem;
      GElf_Shdr *vershdr = gelf_getshdr (verscn, &vershdr_mem);

      if (vershdr != NULL)
	{
	  if (vershdr->sh_type == SHT_GNU_verdef)
	    defscn = verscn;
	  else if (vershdr->sh_type == SHT_GNU_verneed)
	    needscn = verscn;
	}
    }

  size_t nvername;
  if (defscn != NULL || needscn != NULL)
    {
      /* We have a version information (better should have).  Now get
	 the version names.  First find the maximum version number.  */
      nvername = 0;
      if (defscn != NULL)
	{
	  /* Run through the version definitions and find the highest
	     index.  */
	  unsigned int offset = 0;
	  Elf_Data *defdata;
	  GElf_Shdr defshdrmem;
	  GElf_Shdr *defshdr;

	  defdata = elf_getdata (defscn, NULL);
	  if (defdata == NULL)
	    return;

	  defshdr = gelf_getshdr (defscn, &defshdrmem);
	  if (defshdr == NULL)
	    return;

	  for (unsigned int cnt = 0; cnt < defshdr->sh_info; ++cnt)
	    {
	      GElf_Verdef defmem;
	      GElf_Verdef *def;

	      /* Get the data at the next offset.  */
	      def = gelf_getverdef (defdata, offset, &defmem);
	      if (def == NULL)
		break;

	      nvername = MAX (nvername, (size_t) (def->vd_ndx & 0x7fff));

	      offset += def->vd_next;
	    }
	}
      if (needscn != NULL)
	{
	  unsigned int offset = 0;
	  Elf_Data *needdata;
	  GElf_Shdr needshdrmem;
	  GElf_Shdr *needshdr;

	  needdata = elf_getdata (needscn, NULL);
	  if (needdata == NULL)
	    return;

	  needshdr = gelf_getshdr (needscn, &needshdrmem);
	  if (needshdr == NULL)
	    return;

	  for (unsigned int cnt = 0; cnt < needshdr->sh_info; ++cnt)
	    {
	      GElf_Verneed needmem;
	      GElf_Verneed *need;
	      unsigned int auxoffset;
	      int cnt2;

	      /* Get the data at the next offset.  */
	      need = gelf_getverneed (needdata, offset, &needmem);
	      if (need == NULL)
		break;

	      /* Run through the auxiliary entries.  */
	      auxoffset = offset + need->vn_aux;
	      for (cnt2 = need->vn_cnt; --cnt2 >= 0; )
		{
		  GElf_Vernaux auxmem;
		  GElf_Vernaux *aux;

		  aux = gelf_getvernaux (needdata, auxoffset, &auxmem);
		  if (aux == NULL)
		    break;

		  nvername = MAX (nvername,
				  (size_t) (aux->vna_other & 0x7fff));

		  auxoffset += aux->vna_next;
		}

	      offset += need->vn_next;
	    }
	}

      /* This is the number of versions we know about.  */
      ++nvername;

      /* Allocate the array.  */
      vername = (const char **) alloca (nvername * sizeof (const char *));
      filename = (const char **) alloca (nvername * sizeof (const char *));

      /* Run through the data structures again and collect the strings.  */
      if (defscn != NULL)
	{
	  /* Run through the version definitions and find the highest
	     index.  */
	  unsigned int offset = 0;
	  Elf_Data *defdata;
	  GElf_Shdr defshdrmem;
	  GElf_Shdr *defshdr;

	  defdata = elf_getdata (defscn, NULL);
	  if (defdata == NULL)
	    return;

	  defshdr = gelf_getshdr (defscn, &defshdrmem);
	  if (defshdr == NULL)
	    return;

	  for (unsigned int cnt = 0; cnt < defshdr->sh_info; ++cnt)
	    {

	      /* Get the data at the next offset.  */
	      GElf_Verdef defmem;
	      GElf_Verdef *def = gelf_getverdef (defdata, offset, &defmem);
	      GElf_Verdaux auxmem;
	      GElf_Verdaux *aux = gelf_getverdaux (defdata,
						   offset + def->vd_aux,
						   &auxmem);
	      if (def == NULL || aux == NULL)
		break;

	      vername[def->vd_ndx & 0x7fff]
		= elf_strptr (ebl->elf, defshdr->sh_link, aux->vda_name);
	      filename[def->vd_ndx & 0x7fff] = NULL;

	      offset += def->vd_next;
	    }
	}
      if (needscn != NULL)
	{
	  unsigned int offset = 0;

	  Elf_Data *needdata = elf_getdata (needscn, NULL);
	  GElf_Shdr needshdrmem;
	  GElf_Shdr *needshdr = gelf_getshdr (needscn, &needshdrmem);
	  if (needdata == NULL || needshdr == NULL)
	    return;

	  for (unsigned int cnt = 0; cnt < needshdr->sh_info; ++cnt)
	    {
	      /* Get the data at the next offset.  */
	      GElf_Verneed needmem;
	      GElf_Verneed *need = gelf_getverneed (needdata, offset,
						    &needmem);
	      if (need == NULL)
		break;

	      /* Run through the auxiliary entries.  */
	      unsigned int auxoffset = offset + need->vn_aux;
	      for (int cnt2 = need->vn_cnt; --cnt2 >= 0; )
		{
		  GElf_Vernaux auxmem;
		  GElf_Vernaux *aux = gelf_getvernaux (needdata, auxoffset,
						       &auxmem);
		  if (aux == NULL)
		    break;

		  vername[aux->vna_other & 0x7fff]
		    = elf_strptr (ebl->elf, needshdr->sh_link, aux->vna_name);
		  filename[aux->vna_other & 0x7fff]
		    = elf_strptr (ebl->elf, needshdr->sh_link, need->vn_file);

		  auxoffset += aux->vna_next;
		}

	      offset += need->vn_next;
	    }
	}
    }
  else
    {
      vername = NULL;
      nvername = 1;
      filename = NULL;
    }

  /* Print the header.  */
  GElf_Shdr glink;
  printf (ngettext ("\
\nVersion symbols section [%2u] '%s' contains %d entry:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'",
		    "\
\nVersion symbols section [%2u] '%s' contains %d entries:\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'",
		    shdr->sh_size / shdr->sh_entsize),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  (int) (shdr->sh_size / shdr->sh_entsize),
	  class == ELFCLASS32 ? 10 : 18, shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  /* Now we can finally look at the actual contents of this section.  */
  for (unsigned int cnt = 0; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      if (cnt % 2 == 0)
	printf ("\n %4d:", cnt);

      GElf_Versym symmem;
      GElf_Versym *sym = gelf_getversym (data, cnt, &symmem);
      if (sym == NULL)
	break;

      switch (*sym)
	{
	  ssize_t n;
	case 0:
	  fputs_unlocked (gettext ("   0 *local*                     "),
			  stdout);
	  break;

	case 1:
	  fputs_unlocked (gettext ("   1 *global*                    "),
			  stdout);
	  break;

	default:
	  n = printf ("%4d%c%s",
		      *sym & 0x7fff, *sym & 0x8000 ? 'h' : ' ',
		      (unsigned int) (*sym & 0x7fff) < nvername
		      ? vername[*sym & 0x7fff] : "???");
	  if ((unsigned int) (*sym & 0x7fff) < nvername
	      && filename[*sym & 0x7fff] != NULL)
	    n += printf ("(%s)", filename[*sym & 0x7fff]);
	  printf ("%*s", MAX (0, 33 - (int) n), " ");
	  break;
	}
    }
  putchar_unlocked ('\n');
}


static void
print_hash_info (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx,
		 uint_fast32_t maxlength, Elf32_Word nbucket,
		 uint_fast32_t nsyms, uint32_t *lengths, const char *extrastr)
{
  uint32_t *counts = (uint32_t *) xcalloc (maxlength + 1, sizeof (uint32_t));

  for (Elf32_Word cnt = 0; cnt < nbucket; ++cnt)
    ++counts[lengths[cnt]];

  GElf_Shdr glink;
  printf (ngettext ("\
\nHistogram for bucket list length in section [%2u] '%s' (total of %d bucket):\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    "\
\nHistogram for bucket list length in section [%2u] '%s' (total of %d buckets):\n Addr: %#0*" PRIx64 "  Offset: %#08" PRIx64 "  Link to section: [%2u] '%s'\n",
		    nbucket),
	  (unsigned int) elf_ndxscn (scn),
	  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
	  (int) nbucket,
	  gelf_getclass (ebl->elf) == ELFCLASS32 ? 10 : 18,
	  shdr->sh_addr,
	  shdr->sh_offset,
	  (unsigned int) shdr->sh_link,
	  elf_strptr (ebl->elf, shstrndx,
		      gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				    &glink)->sh_name));

  if (extrastr != NULL)
    fputs (extrastr, stdout);

  if (nbucket > 0)
    {
      uint64_t success = 0;

      fputs_unlocked (gettext ("\
 Length  Number  % of total  Coverage\n"), stdout);
      printf (gettext ("      0  %6" PRIu32 "      %5.1f%%\n"),
	      counts[0], (counts[0] * 100.0) / nbucket);

      uint64_t nzero_counts = 0;
      for (Elf32_Word cnt = 1; cnt <= maxlength; ++cnt)
	{
	  nzero_counts += counts[cnt] * cnt;
	  printf (gettext ("\
%7d  %6" PRIu32 "      %5.1f%%    %5.1f%%\n"),
		  (int) cnt, counts[cnt], (counts[cnt] * 100.0) / nbucket,
		  (nzero_counts * 100.0) / nsyms);
	}

      Elf32_Word acc = 0;
      for (Elf32_Word cnt = 1; cnt <= maxlength; ++cnt)
	{
	  acc += cnt;
	  success += counts[cnt] * acc;
	}

      printf (gettext ("\
 Average number of tests:   successful lookup: %f\n\
                          unsuccessful lookup: %f\n"),
	      (double) success / (double) nzero_counts,
	      (double) nzero_counts / (double) nbucket);
    }

  free (counts);
}


/* This function handles the traditional System V-style hash table format.  */
static void
handle_sysv_hash (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx)
{
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    {
      error (0, 0, gettext ("cannot get data for section %d: %s"),
	     (int) elf_ndxscn (scn), elf_errmsg (-1));
      return;
    }

  Elf32_Word nbucket = ((Elf32_Word *) data->d_buf)[0];
  Elf32_Word nchain = ((Elf32_Word *) data->d_buf)[1];
  Elf32_Word *bucket = &((Elf32_Word *) data->d_buf)[2];
  Elf32_Word *chain = &((Elf32_Word *) data->d_buf)[2 + nbucket];

  uint32_t *lengths = (uint32_t *) xcalloc (nbucket, sizeof (uint32_t));

  uint_fast32_t maxlength = 0;
  uint_fast32_t nsyms = 0;
  for (Elf32_Word cnt = 0; cnt < nbucket; ++cnt)
    {
      Elf32_Word inner = bucket[cnt];
      while (inner > 0 && inner < nchain)
	{
	  ++nsyms;
	  if (maxlength < ++lengths[cnt])
	    ++maxlength;

	  inner = chain[inner];
	}
    }

  print_hash_info (ebl, scn, shdr, shstrndx, maxlength, nbucket, nsyms,
		   lengths, NULL);

  free (lengths);
}


/* This function handles the incorrect, System V-style hash table
   format some 64-bit architectures use.  */
static void
handle_sysv_hash64 (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx)
{
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    {
      error (0, 0, gettext ("cannot get data for section %d: %s"),
	     (int) elf_ndxscn (scn), elf_errmsg (-1));
      return;
    }

  Elf64_Xword nbucket = ((Elf64_Xword *) data->d_buf)[0];
  Elf64_Xword nchain = ((Elf64_Xword *) data->d_buf)[1];
  Elf64_Xword *bucket = &((Elf64_Xword *) data->d_buf)[2];
  Elf64_Xword *chain = &((Elf64_Xword *) data->d_buf)[2 + nbucket];

  uint32_t *lengths = (uint32_t *) xcalloc (nbucket, sizeof (uint32_t));

  uint_fast32_t maxlength = 0;
  uint_fast32_t nsyms = 0;
  for (Elf64_Xword cnt = 0; cnt < nbucket; ++cnt)
    {
      Elf64_Xword inner = bucket[cnt];
      while (inner > 0 && inner < nchain)
	{
	  ++nsyms;
	  if (maxlength < ++lengths[cnt])
	    ++maxlength;

	  inner = chain[inner];
	}
    }

  print_hash_info (ebl, scn, shdr, shstrndx, maxlength, nbucket, nsyms,
		   lengths, NULL);

  free (lengths);
}


/* This function handles the GNU-style hash table format.  */
static void
handle_gnu_hash (Ebl *ebl, Elf_Scn *scn, GElf_Shdr *shdr, size_t shstrndx)
{
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    {
      error (0, 0, gettext ("cannot get data for section %d: %s"),
	     (int) elf_ndxscn (scn), elf_errmsg (-1));
      return;
    }

  Elf32_Word nbucket = ((Elf32_Word *) data->d_buf)[0];
  Elf32_Word symbias = ((Elf32_Word *) data->d_buf)[1];

  /* Next comes the size of the bitmap.  It's measured in words for
     the architecture.  It's 32 bits for 32 bit archs, and 64 bits for
     64 bit archs.  */
  Elf32_Word bitmask_words = ((Elf32_Word *) data->d_buf)[2];
  if (gelf_getclass (ebl->elf) == ELFCLASS64)
    bitmask_words *= 2;

  Elf32_Word shift = ((Elf32_Word *) data->d_buf)[3];

  uint32_t *lengths = (uint32_t *) xcalloc (nbucket, sizeof (uint32_t));

  Elf32_Word *bitmask = &((Elf32_Word *) data->d_buf)[4];
  Elf32_Word *bucket = &((Elf32_Word *) data->d_buf)[4 + bitmask_words];
  Elf32_Word *chain = &((Elf32_Word *) data->d_buf)[4 + bitmask_words
						    + nbucket];

  /* Compute distribution of chain lengths.  */
  uint_fast32_t maxlength = 0;
  uint_fast32_t nsyms = 0;
  for (Elf32_Word cnt = 0; cnt < nbucket; ++cnt)
    if (bucket[cnt] != 0)
      {
	Elf32_Word inner = bucket[cnt] - symbias;
	do
	  {
	    ++nsyms;
	    if (maxlength < ++lengths[cnt])
	      ++maxlength;
	  }
	while ((chain[inner++] & 1) == 0);
      }

  /* Count bits in bitmask.  */
  uint_fast32_t nbits = 0;
  for (Elf32_Word cnt = 0; cnt < bitmask_words; ++cnt)
    {
      uint_fast32_t word = bitmask[cnt];

      word = (word & 0x55555555) + ((word >> 1) & 0x55555555);
      word = (word & 0x33333333) + ((word >> 2) & 0x33333333);
      word = (word & 0x0f0f0f0f) + ((word >> 4) & 0x0f0f0f0f);
      word = (word & 0x00ff00ff) + ((word >> 8) & 0x00ff00ff);
      nbits += (word & 0x0000ffff) + ((word >> 16) & 0x0000ffff);
    }

  char *str;
  if (asprintf (&str, gettext ("\
 Symbol Bias: %u\n\
 Bitmask Size: %zu bytes  %" PRIuFAST32 "%% bits set  2nd hash shift: %u\n"),
		symbias, bitmask_words * sizeof (Elf32_Word),
		(nbits * 100 + 50) / (bitmask_words * sizeof (Elf32_Word) * 8),
		shift) == -1)
    error (EXIT_FAILURE, 0, gettext ("memory exhausted"));

  print_hash_info (ebl, scn, shdr, shstrndx, maxlength, nbucket, nsyms,
		   lengths, str);

  free (str);
  free (lengths);
}


/* Find the symbol table(s).  For this we have to search through the
   section table.  */
static void
handle_hash (Ebl *ebl)
{
  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      /* Handle the section if it is a symbol table.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL)
	{
	  if (shdr->sh_type == SHT_HASH)
	    {
	      if (ebl_sysvhash_entrysize (ebl) == sizeof (Elf64_Xword))
		handle_sysv_hash64 (ebl, scn, shdr, shstrndx);
	      else
		handle_sysv_hash (ebl, scn, shdr, shstrndx);
	    }
	  else if (shdr->sh_type == SHT_GNU_HASH)
	    handle_gnu_hash (ebl, scn, shdr, shstrndx);
	}
    }
}


static void
print_liblist (Ebl *ebl)
{
  /* Find the library list sections.  For this we have to search
     through the section table.  */
  Elf_Scn *scn = NULL;

  /* Get the section header string table index.  */
  size_t shstrndx;
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL && shdr->sh_type == SHT_GNU_LIBLIST)
	{
	  int nentries = shdr->sh_size / shdr->sh_entsize;
	  printf (ngettext ("\
\nLibrary list section [%2zu] '%s' at offset %#0" PRIx64 " contains %d entry:\n",
			    "\
\nLibrary list section [%2zu] '%s' at offset %#0" PRIx64 " contains %d entries:\n",
			    nentries),
		  elf_ndxscn (scn),
		  elf_strptr (ebl->elf, shstrndx, shdr->sh_name),
		  shdr->sh_offset,
		  nentries);

	  Elf_Data *data = elf_getdata (scn, NULL);
	  if (data == NULL)
	    return;

	  puts (gettext ("\
       Library                       Time Stamp          Checksum Version Flags"));

	  for (int cnt = 0; cnt < nentries; ++cnt)
	    {
	      GElf_Lib lib_mem;
	      GElf_Lib *lib = gelf_getlib (data, cnt, &lib_mem);
	      if (lib == NULL)
		continue;

	      time_t t = (time_t) lib->l_time_stamp;
	      struct tm *tm = gmtime (&t);
	      if (tm == NULL)
		continue;

	      printf ("  [%2d] %-29s %04u-%02u-%02uT%02u:%02u:%02u %08x %-7u %u\n",
		      cnt, elf_strptr (ebl->elf, shdr->sh_link, lib->l_name),
		      tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		      tm->tm_hour, tm->tm_min, tm->tm_sec,
		      (unsigned int) lib->l_checksum,
		      (unsigned int) lib->l_version,
		      (unsigned int) lib->l_flags);
	    }
	}
    }
}


static const char *
dwarf_tag_string (unsigned int tag)
{
  static const char *known_tags[]  =
    {
      [DW_TAG_array_type] = "array_type",
      [DW_TAG_class_type] = "class_type",
      [DW_TAG_entry_point] = "entry_point",
      [DW_TAG_enumeration_type] = "enumeration_type",
      [DW_TAG_formal_parameter] = "formal_parameter",
      [DW_TAG_imported_declaration] = "imported_declaration",
      [DW_TAG_label] = "label",
      [DW_TAG_lexical_block] = "lexical_block",
      [DW_TAG_member] = "member",
      [DW_TAG_pointer_type] = "pointer_type",
      [DW_TAG_reference_type] = "reference_type",
      [DW_TAG_compile_unit] = "compile_unit",
      [DW_TAG_string_type] = "string_type",
      [DW_TAG_structure_type] = "structure_type",
      [DW_TAG_subroutine_type] = "subroutine_type",
      [DW_TAG_typedef] = "typedef",
      [DW_TAG_union_type] = "union_type",
      [DW_TAG_unspecified_parameters] = "unspecified_parameters",
      [DW_TAG_variant] = "variant",
      [DW_TAG_common_block] = "common_block",
      [DW_TAG_common_inclusion] = "common_inclusion",
      [DW_TAG_inheritance] = "inheritance",
      [DW_TAG_inlined_subroutine] = "inlined_subroutine",
      [DW_TAG_module] = "module",
      [DW_TAG_ptr_to_member_type] = "ptr_to_member_type",
      [DW_TAG_set_type] = "set_type",
      [DW_TAG_subrange_type] = "subrange_type",
      [DW_TAG_with_stmt] = "with_stmt",
      [DW_TAG_access_declaration] = "access_declaration",
      [DW_TAG_base_type] = "base_type",
      [DW_TAG_catch_block] = "catch_block",
      [DW_TAG_const_type] = "const_type",
      [DW_TAG_constant] = "constant",
      [DW_TAG_enumerator] = "enumerator",
      [DW_TAG_file_type] = "file_type",
      [DW_TAG_friend] = "friend",
      [DW_TAG_namelist] = "namelist",
      [DW_TAG_namelist_item] = "namelist_item",
      [DW_TAG_packed_type] = "packed_type",
      [DW_TAG_subprogram] = "subprogram",
      [DW_TAG_template_type_parameter] = "template_type_parameter",
      [DW_TAG_template_value_parameter] = "template_value_parameter",
      [DW_TAG_thrown_type] = "thrown_type",
      [DW_TAG_try_block] = "try_block",
      [DW_TAG_variant_part] = "variant_part",
      [DW_TAG_variable] = "variable",
      [DW_TAG_volatile_type] = "volatile_type",
      [DW_TAG_dwarf_procedure] = "dwarf_procedure",
      [DW_TAG_restrict_type] = "restrict_type",
      [DW_TAG_interface_type] = "interface_type",
      [DW_TAG_namespace] = "namespace",
      [DW_TAG_imported_module] = "imported_module",
      [DW_TAG_unspecified_type] = "unspecified_type",
      [DW_TAG_partial_unit] = "partial_unit",
      [DW_TAG_imported_unit] = "imported_unit",
      [DW_TAG_mutable_type] = "mutable_type",
      [DW_TAG_condition] = "condition",
      [DW_TAG_shared_type] = "shared_type",
    };
  const unsigned int nknown_tags = (sizeof (known_tags)
				    / sizeof (known_tags[0]));
  static char buf[40];
  const char *result = NULL;

  if (tag < nknown_tags)
    result = known_tags[tag];

  if (result == NULL)
    /* There are a few known extensions.  */
    switch (tag)
      {
      case DW_TAG_MIPS_loop:
	result = "MIPS_loop";
	break;

      case DW_TAG_format_label:
	result = "format_label";
	break;

      case DW_TAG_function_template:
	result = "function_template";
	break;

      case DW_TAG_class_template:
	result = "class_template";
	break;

      default:
	if (tag < DW_TAG_lo_user)
	  snprintf (buf, sizeof buf, gettext ("unknown tag %hx"), tag);
	else
	  snprintf (buf, sizeof buf, gettext ("unknown user tag %hx"), tag);
	result = buf;
	break;
      }

  return result;
}


static const char *
dwarf_attr_string (unsigned int attrnum)
{
  static const char *known_attrs[] =
    {
      [DW_AT_sibling] = "sibling",
      [DW_AT_location] = "location",
      [DW_AT_name] = "name",
      [DW_AT_ordering] = "ordering",
      [DW_AT_subscr_data] = "subscr_data",
      [DW_AT_byte_size] = "byte_size",
      [DW_AT_bit_offset] = "bit_offset",
      [DW_AT_bit_size] = "bit_size",
      [DW_AT_element_list] = "element_list",
      [DW_AT_stmt_list] = "stmt_list",
      [DW_AT_low_pc] = "low_pc",
      [DW_AT_high_pc] = "high_pc",
      [DW_AT_language] = "language",
      [DW_AT_member] = "member",
      [DW_AT_discr] = "discr",
      [DW_AT_discr_value] = "discr_value",
      [DW_AT_visibility] = "visibility",
      [DW_AT_import] = "import",
      [DW_AT_string_length] = "string_length",
      [DW_AT_common_reference] = "common_reference",
      [DW_AT_comp_dir] = "comp_dir",
      [DW_AT_const_value] = "const_value",
      [DW_AT_containing_type] = "containing_type",
      [DW_AT_default_value] = "default_value",
      [DW_AT_inline] = "inline",
      [DW_AT_is_optional] = "is_optional",
      [DW_AT_lower_bound] = "lower_bound",
      [DW_AT_producer] = "producer",
      [DW_AT_prototyped] = "prototyped",
      [DW_AT_return_addr] = "return_addr",
      [DW_AT_start_scope] = "start_scope",
      [DW_AT_bit_stride] = "bit_stride",
      [DW_AT_upper_bound] = "upper_bound",
      [DW_AT_abstract_origin] = "abstract_origin",
      [DW_AT_accessibility] = "accessibility",
      [DW_AT_address_class] = "address_class",
      [DW_AT_artificial] = "artificial",
      [DW_AT_base_types] = "base_types",
      [DW_AT_calling_convention] = "calling_convention",
      [DW_AT_count] = "count",
      [DW_AT_data_member_location] = "data_member_location",
      [DW_AT_decl_column] = "decl_column",
      [DW_AT_decl_file] = "decl_file",
      [DW_AT_decl_line] = "decl_line",
      [DW_AT_declaration] = "declaration",
      [DW_AT_discr_list] = "discr_list",
      [DW_AT_encoding] = "encoding",
      [DW_AT_external] = "external",
      [DW_AT_frame_base] = "frame_base",
      [DW_AT_friend] = "friend",
      [DW_AT_identifier_case] = "identifier_case",
      [DW_AT_macro_info] = "macro_info",
      [DW_AT_namelist_item] = "namelist_item",
      [DW_AT_priority] = "priority",
      [DW_AT_segment] = "segment",
      [DW_AT_specification] = "specification",
      [DW_AT_static_link] = "static_link",
      [DW_AT_type] = "type",
      [DW_AT_use_location] = "use_location",
      [DW_AT_variable_parameter] = "variable_parameter",
      [DW_AT_virtuality] = "virtuality",
      [DW_AT_vtable_elem_location] = "vtable_elem_location",
      [DW_AT_allocated] = "allocated",
      [DW_AT_associated] = "associated",
      [DW_AT_data_location] = "data_location",
      [DW_AT_byte_stride] = "byte_stride",
      [DW_AT_entry_pc] = "entry_pc",
      [DW_AT_use_UTF8] = "use_UTF8",
      [DW_AT_extension] = "extension",
      [DW_AT_ranges] = "ranges",
      [DW_AT_trampoline] = "trampoline",
      [DW_AT_call_column] = "call_column",
      [DW_AT_call_file] = "call_file",
      [DW_AT_call_line] = "call_line",
      [DW_AT_description] = "description",
      [DW_AT_binary_scale] = "binary_scale",
      [DW_AT_decimal_scale] = "decimal_scale",
      [DW_AT_small] = "small",
      [DW_AT_decimal_sign] = "decimal_sign",
      [DW_AT_digit_count] = "digit_count",
      [DW_AT_picture_string] = "picture_string",
      [DW_AT_mutable] = "mutable",
      [DW_AT_threads_scaled] = "threads_scaled",
      [DW_AT_explicit] = "explicit",
      [DW_AT_object_pointer] = "object_pointer",
      [DW_AT_endianity] = "endianity",
      [DW_AT_elemental] = "elemental",
      [DW_AT_pure] = "pure",
      [DW_AT_recursive] = "recursive",
    };
  const unsigned int nknown_attrs = (sizeof (known_attrs)
				     / sizeof (known_attrs[0]));
  static char buf[40];
  const char *result = NULL;

  if (attrnum < nknown_attrs)
    result = known_attrs[attrnum];

  if (result == NULL)
    /* There are a few known extensions.  */
    switch (attrnum)
      {
      case DW_AT_MIPS_fde:
	result = "MIPS_fde";
	break;

      case DW_AT_MIPS_loop_begin:
	result = "MIPS_loop_begin";
	break;

      case DW_AT_MIPS_tail_loop_begin:
	result = "MIPS_tail_loop_begin";
	break;

      case DW_AT_MIPS_epilog_begin:
	result = "MIPS_epilog_begin";
	break;

      case DW_AT_MIPS_loop_unroll_factor:
	result = "MIPS_loop_unroll_factor";
	break;

      case DW_AT_MIPS_software_pipeline_depth:
	result = "MIPS_software_pipeline_depth";
	break;

      case DW_AT_MIPS_linkage_name:
	result = "MIPS_linkage_name";
	break;

      case DW_AT_MIPS_stride:
	result = "MIPS_stride";
	break;

      case DW_AT_MIPS_abstract_name:
	result = "MIPS_abstract_name";
	break;

      case DW_AT_MIPS_clone_origin:
	result = "MIPS_clone_origin";
	break;

      case DW_AT_MIPS_has_inlines:
	result = "MIPS_has_inlines";
	break;

      case DW_AT_MIPS_stride_byte:
	result = "MIPS_stride_byte";
	break;

      case DW_AT_MIPS_stride_elem:
	result = "MIPS_stride_elem";
	break;

      case DW_AT_MIPS_ptr_dopetype:
	result = "MIPS_ptr_dopetype";
	break;

      case DW_AT_MIPS_allocatable_dopetype:
	result = "MIPS_allocatable_dopetype";
	break;

      case DW_AT_MIPS_assumed_shape_dopetype:
	result = "MIPS_assumed_shape_dopetype";
	break;

      case DW_AT_MIPS_assumed_size:
	result = "MIPS_assumed_size";
	break;

      case DW_AT_sf_names:
	result = "sf_names";
	break;

      case DW_AT_src_info:
	result = "src_info";
	break;

      case DW_AT_mac_info:
	result = "mac_info";
	break;

      case DW_AT_src_coords:
	result = "src_coords";
	break;

      case DW_AT_body_begin:
	result = "body_begin";
	break;

      case DW_AT_body_end:
	result = "body_end";
	break;

      default:
	if (attrnum < DW_AT_lo_user)
	  snprintf (buf, sizeof buf, gettext ("unknown attribute %hx"),
		    attrnum);
	else
	  snprintf (buf, sizeof buf, gettext ("unknown user attribute %hx"),
		    attrnum);
	result = buf;
	break;
      }

  return result;
}


static const char *
dwarf_form_string (unsigned int form)
{
  static const char *known_forms[] =
    {
      [DW_FORM_addr] = "addr",
      [DW_FORM_block2] = "block2",
      [DW_FORM_block4] = "block4",
      [DW_FORM_data2] = "data2",
      [DW_FORM_data4] = "data4",
      [DW_FORM_data8] = "data8",
      [DW_FORM_string] = "string",
      [DW_FORM_block] = "block",
      [DW_FORM_block1] = "block1",
      [DW_FORM_data1] = "data1",
      [DW_FORM_flag] = "flag",
      [DW_FORM_sdata] = "sdata",
      [DW_FORM_strp] = "strp",
      [DW_FORM_udata] = "udata",
      [DW_FORM_ref_addr] = "ref_addr",
      [DW_FORM_ref1] = "ref1",
      [DW_FORM_ref2] = "ref2",
      [DW_FORM_ref4] = "ref4",
      [DW_FORM_ref8] = "ref8",
      [DW_FORM_ref_udata] = "ref_udata",
      [DW_FORM_indirect] = "indirect"
    };
  const unsigned int nknown_forms = (sizeof (known_forms)
				     / sizeof (known_forms[0]));
  static char buf[40];
  const char *result = NULL;

  if (form < nknown_forms)
    result = known_forms[form];

  if (result == NULL)
    snprintf (buf, sizeof buf, gettext ("unknown form %" PRIx64),
	      (uint64_t) form);

  return result;
}


static const char *
dwarf_lang_string (unsigned int lang)
{
  static const char *known[] =
    {
      [DW_LANG_C89] = "ISO C89",
      [DW_LANG_C] = "C",
      [DW_LANG_Ada83] = "Ada83",
      [DW_LANG_C_plus_plus] = "C++",
      [DW_LANG_Cobol74] = "Cobol74",
      [DW_LANG_Cobol85] = "Cobol85",
      [DW_LANG_Fortran77] = "Fortran77",
      [DW_LANG_Fortran90] = "Fortran90",
      [DW_LANG_Pascal83] = "Pascal83",
      [DW_LANG_Modula2] = "Modula2",
      [DW_LANG_Java] = "Java",
      [DW_LANG_C99] = "ISO C99",
      [DW_LANG_Ada95] = "Ada95",
      [DW_LANG_Fortran95] = "Fortran95",
      [DW_LANG_PL1] = "PL1",
      [DW_LANG_Objc] = "Objective C",
      [DW_LANG_ObjC_plus_plus] = "Objective C++",
      [DW_LANG_UPC] = "UPC",
      [DW_LANG_D] = "D",
    };

  if (lang < sizeof (known) / sizeof (known[0]))
    return known[lang];
  else if (lang == DW_LANG_Mips_Assembler)
    /* This language tag is used for assembler in general.  */
    return "Assembler";

  if (lang >= DW_LANG_lo_user && lang <= DW_LANG_hi_user)
    {
      static char buf[30];
      snprintf (buf, sizeof (buf), "lo_user+%u", lang - DW_LANG_lo_user);
      return buf;
    }

  return "???";
}


static const char *
dwarf_inline_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_INL_not_inlined] = "not_inlined",
      [DW_INL_inlined] = "inlined",
      [DW_INL_declared_not_inlined] = "declared_not_inlined",
      [DW_INL_declared_inlined] = "declared_inlined"
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  return "???";
}


static const char *
dwarf_encoding_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_ATE_void] = "void",
      [DW_ATE_address] = "address",
      [DW_ATE_boolean] = "boolean",
      [DW_ATE_complex_float] = "complex_float",
      [DW_ATE_float] = "float",
      [DW_ATE_signed] = "signed",
      [DW_ATE_signed_char] = "signed_char",
      [DW_ATE_unsigned] = "unsigned",
      [DW_ATE_unsigned_char] = "unsigned_char",
      [DW_ATE_imaginary_float] = "imaginary_float",
      [DW_ATE_packed_decimal] = "packed_decimal",
      [DW_ATE_numeric_string] = "numeric_string",
      [DW_ATE_edited] = "edited",
      [DW_ATE_signed_fixed] = "signed_fixed",
      [DW_ATE_unsigned_fixed] = "unsigned_fixed",
      [DW_ATE_decimal_float] = "decimal_float",
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  if (code >= DW_ATE_lo_user && code <= DW_ATE_hi_user)
    {
      static char buf[30];
      snprintf (buf, sizeof (buf), "lo_user+%u", code - DW_ATE_lo_user);
      return buf;
    }

  return "???";
}


static const char *
dwarf_access_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_ACCESS_public] = "public",
      [DW_ACCESS_protected] = "protected",
      [DW_ACCESS_private] = "private"
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  return "???";
}


static const char *
dwarf_visibility_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_VIS_local] = "local",
      [DW_VIS_exported] = "exported",
      [DW_VIS_qualified] = "qualified"
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  return "???";
}


static const char *
dwarf_virtuality_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_VIRTUALITY_none] = "none",
      [DW_VIRTUALITY_virtual] = "virtual",
      [DW_VIRTUALITY_pure_virtual] = "pure_virtual"
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  return "???";
}


static const char *
dwarf_identifier_case_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_ID_case_sensitive] = "sensitive",
      [DW_ID_up_case] = "up_case",
      [DW_ID_down_case] = "down_case",
      [DW_ID_case_insensitive] = "insensitive"
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  return "???";
}


static const char *
dwarf_calling_convention_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_CC_normal] = "normal",
      [DW_CC_program] = "program",
      [DW_CC_nocall] = "nocall",
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  if (code >= DW_CC_lo_user && code <= DW_CC_hi_user)
    {
      static char buf[30];
      snprintf (buf, sizeof (buf), "lo_user+%u", code - DW_CC_lo_user);
      return buf;
    }

  return "???";
}


static const char *
dwarf_ordering_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_ORD_row_major] = "row_major",
      [DW_ORD_col_major] = "col_major"
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  return "???";
}


static const char *
dwarf_discr_list_string (unsigned int code)
{
  static const char *known[] =
    {
      [DW_DSC_label] = "label",
      [DW_DSC_range] = "range"
    };

  if (code < sizeof (known) / sizeof (known[0]))
    return known[code];

  return "???";
}


static void
print_ops (Dwarf *dbg, int indent, int indentrest,
	   unsigned int addrsize, Dwarf_Word len, const unsigned char *data)
{
  static const char *known[] =
    {
      [DW_OP_addr] = "addr",
      [DW_OP_deref] = "deref",
      [DW_OP_const1u] = "const1u",
      [DW_OP_const1s] = "const1s",
      [DW_OP_const2u] = "const2u",
      [DW_OP_const2s] = "const2s",
      [DW_OP_const4u] = "const4u",
      [DW_OP_const4s] = "const4s",
      [DW_OP_const8u] = "const8u",
      [DW_OP_const8s] = "const8s",
      [DW_OP_constu] = "constu",
      [DW_OP_consts] = "consts",
      [DW_OP_dup] = "dup",
      [DW_OP_drop] = "drop",
      [DW_OP_over] = "over",
      [DW_OP_pick] = "pick",
      [DW_OP_swap] = "swap",
      [DW_OP_rot] = "rot",
      [DW_OP_xderef] = "xderef",
      [DW_OP_abs] = "abs",
      [DW_OP_and] = "and",
      [DW_OP_div] = "div",
      [DW_OP_minus] = "minus",
      [DW_OP_mod] = "mod",
      [DW_OP_mul] = "mul",
      [DW_OP_neg] = "neg",
      [DW_OP_not] = "not",
      [DW_OP_or] = "or",
      [DW_OP_plus] = "plus",
      [DW_OP_plus_uconst] = "plus_uconst",
      [DW_OP_shl] = "shl",
      [DW_OP_shr] = "shr",
      [DW_OP_shra] = "shra",
      [DW_OP_xor] = "xor",
      [DW_OP_bra] = "bra",
      [DW_OP_eq] = "eq",
      [DW_OP_ge] = "ge",
      [DW_OP_gt] = "gt",
      [DW_OP_le] = "le",
      [DW_OP_lt] = "lt",
      [DW_OP_ne] = "ne",
      [DW_OP_skip] = "skip",
      [DW_OP_lit0] = "lit0",
      [DW_OP_lit1] = "lit1",
      [DW_OP_lit2] = "lit2",
      [DW_OP_lit3] = "lit3",
      [DW_OP_lit4] = "lit4",
      [DW_OP_lit5] = "lit5",
      [DW_OP_lit6] = "lit6",
      [DW_OP_lit7] = "lit7",
      [DW_OP_lit8] = "lit8",
      [DW_OP_lit9] = "lit9",
      [DW_OP_lit10] = "lit10",
      [DW_OP_lit11] = "lit11",
      [DW_OP_lit12] = "lit12",
      [DW_OP_lit13] = "lit13",
      [DW_OP_lit14] = "lit14",
      [DW_OP_lit15] = "lit15",
      [DW_OP_lit16] = "lit16",
      [DW_OP_lit17] = "lit17",
      [DW_OP_lit18] = "lit18",
      [DW_OP_lit19] = "lit19",
      [DW_OP_lit20] = "lit20",
      [DW_OP_lit21] = "lit21",
      [DW_OP_lit22] = "lit22",
      [DW_OP_lit23] = "lit23",
      [DW_OP_lit24] = "lit24",
      [DW_OP_lit25] = "lit25",
      [DW_OP_lit26] = "lit26",
      [DW_OP_lit27] = "lit27",
      [DW_OP_lit28] = "lit28",
      [DW_OP_lit29] = "lit29",
      [DW_OP_lit30] = "lit30",
      [DW_OP_lit31] = "lit31",
      [DW_OP_reg0] = "reg0",
      [DW_OP_reg1] = "reg1",
      [DW_OP_reg2] = "reg2",
      [DW_OP_reg3] = "reg3",
      [DW_OP_reg4] = "reg4",
      [DW_OP_reg5] = "reg5",
      [DW_OP_reg6] = "reg6",
      [DW_OP_reg7] = "reg7",
      [DW_OP_reg8] = "reg8",
      [DW_OP_reg9] = "reg9",
      [DW_OP_reg10] = "reg10",
      [DW_OP_reg11] = "reg11",
      [DW_OP_reg12] = "reg12",
      [DW_OP_reg13] = "reg13",
      [DW_OP_reg14] = "reg14",
      [DW_OP_reg15] = "reg15",
      [DW_OP_reg16] = "reg16",
      [DW_OP_reg17] = "reg17",
      [DW_OP_reg18] = "reg18",
      [DW_OP_reg19] = "reg19",
      [DW_OP_reg20] = "reg20",
      [DW_OP_reg21] = "reg21",
      [DW_OP_reg22] = "reg22",
      [DW_OP_reg23] = "reg23",
      [DW_OP_reg24] = "reg24",
      [DW_OP_reg25] = "reg25",
      [DW_OP_reg26] = "reg26",
      [DW_OP_reg27] = "reg27",
      [DW_OP_reg28] = "reg28",
      [DW_OP_reg29] = "reg29",
      [DW_OP_reg30] = "reg30",
      [DW_OP_reg31] = "reg31",
      [DW_OP_breg0] = "breg0",
      [DW_OP_breg1] = "breg1",
      [DW_OP_breg2] = "breg2",
      [DW_OP_breg3] = "breg3",
      [DW_OP_breg4] = "breg4",
      [DW_OP_breg5] = "breg5",
      [DW_OP_breg6] = "breg6",
      [DW_OP_breg7] = "breg7",
      [DW_OP_breg8] = "breg8",
      [DW_OP_breg9] = "breg9",
      [DW_OP_breg10] = "breg10",
      [DW_OP_breg11] = "breg11",
      [DW_OP_breg12] = "breg12",
      [DW_OP_breg13] = "breg13",
      [DW_OP_breg14] = "breg14",
      [DW_OP_breg15] = "breg15",
      [DW_OP_breg16] = "breg16",
      [DW_OP_breg17] = "breg17",
      [DW_OP_breg18] = "breg18",
      [DW_OP_breg19] = "breg19",
      [DW_OP_breg20] = "breg20",
      [DW_OP_breg21] = "breg21",
      [DW_OP_breg22] = "breg22",
      [DW_OP_breg23] = "breg23",
      [DW_OP_breg24] = "breg24",
      [DW_OP_breg25] = "breg25",
      [DW_OP_breg26] = "breg26",
      [DW_OP_breg27] = "breg27",
      [DW_OP_breg28] = "breg28",
      [DW_OP_breg29] = "breg29",
      [DW_OP_breg30] = "breg30",
      [DW_OP_breg31] = "breg31",
      [DW_OP_regx] = "regx",
      [DW_OP_fbreg] = "fbreg",
      [DW_OP_bregx] = "bregx",
      [DW_OP_piece] = "piece",
      [DW_OP_deref_size] = "deref_size",
      [DW_OP_xderef_size] = "xderef_size",
      [DW_OP_nop] = "nop",
      [DW_OP_push_object_address] = "push_object_address",
      [DW_OP_call2] = "call2",
      [DW_OP_call4] = "call4",
      [DW_OP_call_ref] = "call_ref",
      [DW_OP_form_tls_address] = "form_tls_address",
      [DW_OP_call_frame_cfa] = "call_frame_cfa",
      [DW_OP_bit_piece] = "bit_piece",
    };

  Dwarf_Word offset = 0;
  while (len-- > 0)
    {
      size_t op = *data++;

      switch (op)
	{
	case DW_OP_call_ref:
	case DW_OP_addr:;
	  /* Address operand.  */
	  Dwarf_Word addr;
	  if (addrsize == 4)
	    addr = read_4ubyte_unaligned (dbg, data);
	  else
	    {
	      assert (addrsize == 8);
	      addr = read_8ubyte_unaligned (dbg, data);
	    }
	  data += addrsize;
	  len -= addrsize;

	  printf ("%*s[%4" PRIuMAX "] %s %" PRIuMAX "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", (uintmax_t) addr);
	  offset += 1 + addrsize;
	  break;

	case DW_OP_deref_size:
	case DW_OP_xderef_size:
	case DW_OP_pick:
	case DW_OP_const1u:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu8 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", *((uint8_t *) data));
	  ++data;
	  --len;
	  offset += 2;
	  break;

	case DW_OP_const2u:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu16 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", read_2ubyte_unaligned (dbg, data));
	  len -= 2;
	  data += 2;
	  offset += 3;
	  break;

	case DW_OP_const4u:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu32 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", read_4ubyte_unaligned (dbg, data));
	  len -= 4;
	  data += 4;
	  offset += 5;
	  break;

	case DW_OP_const8u:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIu64 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", read_8ubyte_unaligned (dbg, data));
	  len -= 8;
	  data += 8;
	  offset += 9;
	  break;

	case DW_OP_const1s:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId8 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", *((int8_t *) data));
	  ++data;
	  --len;
	  offset += 2;
	  break;

	case DW_OP_const2s:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId16 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", read_2sbyte_unaligned (dbg, data));
	  len -= 2;
	  data += 2;
	  offset += 3;
	  break;

	case DW_OP_const4s:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId32 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", read_4sbyte_unaligned (dbg, data));
	  len -= 4;
	  data += 4;
	  offset += 5;
	  break;

	case DW_OP_const8s:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRId64 "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", read_8sbyte_unaligned (dbg, data));
	  len -= 8;
	  data += 8;
	  offset += 9;
	  break;

	case DW_OP_piece:
	case DW_OP_regx:
	case DW_OP_plus_uconst:
	case DW_OP_constu:;
	  const unsigned char *start = data;
	  unsigned int uleb;
	  get_uleb128 (uleb, data);
	  printf ("%*s[%4" PRIuMAX "] %s %u\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", uleb);
	  len -= data - start;
	  offset += 1 + (data - start);
	  break;

	case DW_OP_bit_piece:
	  start = data;
	  unsigned int uleb2;
	  get_uleb128 (uleb, data);
	  get_uleb128 (uleb2, data);
	  printf ("%*s[%4" PRIuMAX "] %s %u, %u\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", uleb, uleb2);
	  len -= data - start;
	  offset += 1 + (data - start);
	  break;

	case DW_OP_fbreg:
	case DW_OP_breg0 ... DW_OP_breg31:
	case DW_OP_consts:
	  start = data;
	  unsigned int sleb;
	  get_sleb128 (sleb, data);
	  printf ("%*s[%4" PRIuMAX "] %s %d\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", sleb);
	  len -= data - start;
	  offset += 1 + (data - start);
	  break;

	case DW_OP_bregx:
	  start = data;
	  get_uleb128 (uleb, data);
	  get_sleb128 (sleb, data);
	  printf ("%*s[%4" PRIuMAX "] %s %u %d\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???", uleb, sleb);
	  len -= data - start;
	  offset += 1 + (data - start);
	  break;

	case DW_OP_call2:
	case DW_OP_call4:
	case DW_OP_skip:
	case DW_OP_bra:
	  printf ("%*s[%4" PRIuMAX "] %s %" PRIuMAX "\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???",
		  (uintmax_t) (offset + read_2sbyte_unaligned (dbg, data)));
	  len -= 2;
	  data += 2;
	  offset += 3;
	  break;

	default:
	  /* No Operand.  */
	  printf ("%*s[%4" PRIuMAX "] %s\n",
		  indent, "", (uintmax_t) offset,
		  known[op] ?: "???");
	  ++offset;
	  break;
	}

      indent = indentrest;
    }
}


static void
print_debug_abbrev_section (Ebl *ebl __attribute__ ((unused)),
			    GElf_Ehdr *ehdr __attribute__ ((unused)),
			    Elf_Scn *scn __attribute__ ((unused)),
			    GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\nDWARF section '%s' at offset %#" PRIx64 ":\n"
		   " [ Code]\n"),
	  ".debug_abbrev", (uint64_t) shdr->sh_offset);

  Dwarf_Off offset = 0;
  while (offset < shdr->sh_size)
    {
      printf (gettext ("\nAbbreviation section at offset %" PRIu64 ":\n"),
	      offset);

      while (1)
	{
	  size_t length;
	  Dwarf_Abbrev abbrev;

	  int res = dwarf_offabbrev (dbg, offset, &length, &abbrev);
	  if (res != 0)
	    {
	      if (res < 0)
		{
		  printf (gettext ("\
 *** error while reading abbreviation: %s\n"),
			  dwarf_errmsg (-1));
		  return;
		}

	      /* This is the NUL byte at the end of the section.  */
	      ++offset;
	      break;
	    }

	  /* We know these calls can never fail.  */
	  unsigned int code = dwarf_getabbrevcode (&abbrev);
	  unsigned int tag = dwarf_getabbrevtag (&abbrev);
	  int has_children = dwarf_abbrevhaschildren (&abbrev);

	  printf (gettext (" [%5u] offset: %" PRId64
			   ", children: %s, tag: %s\n"),
		  code, (int64_t) offset,
		  has_children ? gettext ("yes") : gettext ("no"),
		  dwarf_tag_string (tag));

	  size_t cnt = 0;
	  unsigned int name;
	  unsigned int form;
	  Dwarf_Off enoffset;
	  while (dwarf_getabbrevattr (&abbrev, cnt,
				      &name, &form, &enoffset) == 0)
	    {
	      printf ("          attr: %s, form: %s, offset: %#" PRIx64 "\n",
		      dwarf_attr_string (name), dwarf_form_string (form),
		      (uint64_t) enoffset);

	      ++cnt;
	    }

	  offset += length;
	}
    }
}


/* Print content of DWARF .debug_aranges section.  We fortunately do
   not have to know a bit about the structure of the section, libdwarf
   takes care of it.  */
static void
print_debug_aranges_section (Ebl *ebl __attribute__ ((unused)),
			     GElf_Ehdr *ehdr __attribute__ ((unused)),
			     Elf_Scn *scn __attribute__ ((unused)),
			     GElf_Shdr *shdr, Dwarf *dbg)
{
  Dwarf_Aranges *aranges;
  size_t cnt;
  if (dwarf_getaranges (dbg, &aranges, &cnt) != 0)
    {
      error (0, 0, gettext ("cannot get .debug_aranges content: %s"),
	     dwarf_errmsg (-1));
      return;
    }

  printf (ngettext ("\
\nDWARF section '%s' at offset %#" PRIx64 " contains %zu entry:\n",
		    "\
\nDWARF section '%s' at offset %#" PRIx64 " contains %zu entries:\n",
		    cnt),
	  ".debug_aranges", (uint64_t) shdr->sh_offset, cnt);

  /* Compute floor(log16(cnt)).  */
  size_t tmp = cnt;
  int digits = 1;
  while (tmp >= 16)
    {
      ++digits;
      tmp >>= 4;
    }

  for (size_t n = 0; n < cnt; ++n)
    {
      Dwarf_Arange *runp = dwarf_onearange (aranges, n);
      if (runp == NULL)
	{
	  printf ("cannot get arange %zu: %s\n", n, dwarf_errmsg (-1));
	  return;
	}

      Dwarf_Addr start;
      Dwarf_Word length;
      Dwarf_Off offset;

      if (dwarf_getarangeinfo (runp, &start, &length, &offset) != 0)
	printf (gettext (" [%*zu] ???\n"), digits, n);
      else
	printf (gettext (" [%*zu] start: %0#*" PRIx64
			 ", length: %5" PRIu64 ", CU DIE offset: %6"
			 PRId64 "\n"),
		digits, n, ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 10 : 18,
		(uint64_t) start, (uint64_t) length, (int64_t) offset);
    }
}

/* Print content of DWARF .debug_ranges section.  */
static void
print_debug_ranges_section (Ebl *ebl __attribute__ ((unused)),
			    GElf_Ehdr *ehdr, Elf_Scn *scn, GElf_Shdr *shdr,
			    Dwarf *dbg)
{
  Elf_Data *data = elf_rawdata (scn, NULL);

  if (data == NULL)
    {
      error (0, 0, gettext ("cannot get .debug_ranges content: %s"),
	     elf_errmsg (-1));
      return;
    }

  printf (gettext ("\
\nDWARF section '%s' at offset %#" PRIx64 ":\n"),
	  ".debug_ranges", (uint64_t) shdr->sh_offset);

  size_t address_size = ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;

  bool first = true;
  unsigned char *readp = data->d_buf;
  while (readp < (unsigned char *) data->d_buf + data->d_size)
    {
      ptrdiff_t offset = readp - (unsigned char *) data->d_buf;

      if (data->d_size - offset < address_size * 2)
	{
	  printf (" [%6tx]  <INVALID DATA>\n", offset);
	  break;
	}

      Dwarf_Addr begin;
      Dwarf_Addr end;
      if (address_size == 8)
	{
	  begin = read_8ubyte_unaligned_inc (dbg, readp);
	  end = read_8ubyte_unaligned_inc (dbg, readp);
	}
      else
	{
	  begin = read_4ubyte_unaligned_inc (dbg, readp);
	  end = read_4ubyte_unaligned_inc (dbg, readp);
	  if (begin == (Dwarf_Addr) (uint32_t) -1)
	    begin = (Dwarf_Addr) -1l;
	}

      if (begin == (Dwarf_Addr) -1l) /* Base address entry.  */
	printf (" [%6tx]  base address %#0*" PRIxMAX "\n", offset,
		2 + (int) (address_size * 2), (uintmax_t) end);
      else if (begin == 0 && end == 0) /* End of list entry.  */
	first = true;
      else
	{
	  /* We have an address range entry.  */
	  if (first)		/* First address range entry in a list.  */
	    printf (" [%6tx]  %#0*" PRIxMAX "..%#0*" PRIxMAX "\n", offset,
		    2 + (int) (address_size * 2), (uintmax_t) begin,
		    2 + (int) (address_size * 2), (uintmax_t) end);
	  else
	    printf ("           %#0*" PRIxMAX "..%#0*" PRIxMAX "\n",
		    2 + (int) (address_size * 2), (uintmax_t) begin,
		    2 + (int) (address_size * 2), (uintmax_t) end);

	  first = false;
	}
    }
}


static void
print_debug_frame_section (Ebl *ebl __attribute__ ((unused)),
			   GElf_Ehdr *ehdr __attribute__ ((unused)),
			   Elf_Scn *scn __attribute__ ((unused)),
			   GElf_Shdr *shdr __attribute__ ((unused)),
			   Dwarf *dbg __attribute__ ((unused)))
{
}


struct attrcb_args
{
  Dwarf *dbg;
  int level;
  unsigned int addrsize;
  Dwarf_Off cu_offset;
};


static int
attr_callback (Dwarf_Attribute *attrp, void *arg)
{
  struct attrcb_args *cbargs = (struct attrcb_args *) arg;
  const int level = cbargs->level;

  unsigned int attr = dwarf_whatattr (attrp);
  if (unlikely (attr == 0))
    {
      error (0, 0, gettext ("cannot get attribute code: %s"),
	     dwarf_errmsg (-1));
      return DWARF_CB_ABORT;
    }

  unsigned int form = dwarf_whatform (attrp);
  if (unlikely (form == 0))
    {
      error (0, 0, gettext ("cannot get attribute form: %s"),
	     dwarf_errmsg (-1));
      return DWARF_CB_ABORT;
    }

  switch (form)
    {
    case DW_FORM_addr:;
      Dwarf_Addr addr;
      if (unlikely (dwarf_formaddr (attrp, &addr) != 0))
	{
	attrval_out:
	  error (0, 0, gettext ("cannot get attribute value: %s"),
		 dwarf_errmsg (-1));
	  return DWARF_CB_ABORT;
	}
      printf ("           %*s%-20s %#0*" PRIxMAX "\n",
	      (int) (level * 2), "", dwarf_attr_string (attr),
	      2 + (int) (cbargs->addrsize * 2), (uintmax_t) addr);
      break;

    case DW_FORM_indirect:
    case DW_FORM_strp:
    case DW_FORM_string:;
      const char *str = dwarf_formstring (attrp);
      if (unlikely (str == NULL))
	goto attrval_out;
      printf ("           %*s%-20s \"%s\"\n",
	      (int) (level * 2), "", dwarf_attr_string (attr), str);
      break;

    case DW_FORM_ref_addr:
    case DW_FORM_ref_udata:
    case DW_FORM_ref8:
    case DW_FORM_ref4:
    case DW_FORM_ref2:
    case DW_FORM_ref1:;
      Dwarf_Off ref;
      if (unlikely (dwarf_formref (attrp, &ref) != 0))
	goto attrval_out;

      printf ("           %*s%-20s [%6" PRIxMAX "]\n",
	      (int) (level * 2), "", dwarf_attr_string (attr),
	      (uintmax_t) (ref + cbargs->cu_offset));
      break;

    case DW_FORM_udata:
    case DW_FORM_sdata:
    case DW_FORM_data8:
    case DW_FORM_data4:
    case DW_FORM_data2:
    case DW_FORM_data1:;
      Dwarf_Word num;
      if (unlikely (dwarf_formudata (attrp, &num) != 0))
	goto attrval_out;

      const char *valuestr = NULL;
      switch (attr)
	{
	case DW_AT_location:
	case DW_AT_data_member_location:
	case DW_AT_vtable_elem_location:
	case DW_AT_string_length:
	case DW_AT_use_location:
	case DW_AT_frame_base:
	case DW_AT_return_addr:
	case DW_AT_static_link:
	  printf ("           %*s%-20s location list [%6" PRIxMAX "]\n",
		  (int) (level * 2), "", dwarf_attr_string (attr),
		  (uintmax_t) num);
	  return DWARF_CB_OK;

	case DW_AT_ranges:
	  printf ("           %*s%-20s range list [%6" PRIxMAX "]\n",
		  (int) (level * 2), "", dwarf_attr_string (attr),
		  (uintmax_t) num);
	  return DWARF_CB_OK;

	case DW_AT_language:
	  valuestr = dwarf_lang_string (num);
	  break;
	case DW_AT_encoding:
	  valuestr = dwarf_encoding_string (num);
	  break;
	case DW_AT_accessibility:
	  valuestr = dwarf_access_string (num);
	  break;
	case DW_AT_visibility:
	  valuestr = dwarf_visibility_string (num);
	  break;
	case DW_AT_virtuality:
	  valuestr = dwarf_virtuality_string (num);
	  break;
	case DW_AT_identifier_case:
	  valuestr = dwarf_identifier_case_string (num);
	  break;
	case DW_AT_calling_convention:
	  valuestr = dwarf_calling_convention_string (num);
	  break;
	case DW_AT_inline:
	  valuestr = dwarf_inline_string (num);
	  break;
	case DW_AT_ordering:
	  valuestr = dwarf_ordering_string (num);
	  break;
	case DW_AT_discr_list:
	  valuestr = dwarf_discr_list_string (num);
	  break;
	default:
	  /* Nothing.  */
	  break;
	}

      if (valuestr == NULL)
	printf ("           %*s%-20s %" PRIuMAX "\n",
		(int) (level * 2), "", dwarf_attr_string (attr),
		(uintmax_t) num);
      else
	printf ("           %*s%-20s %s (%" PRIuMAX ")\n",
		(int) (level * 2), "", dwarf_attr_string (attr),
		valuestr, (uintmax_t) num);
      break;

    case DW_FORM_flag:;
      bool flag;
      if (unlikely (dwarf_formflag (attrp, &flag) != 0))
	goto attrval_out;

      printf ("           %*s%-20s %s\n",
	      (int) (level * 2), "", dwarf_attr_string (attr),
	      nl_langinfo (flag ? YESSTR : NOSTR));
      break;

    case DW_FORM_block4:
    case DW_FORM_block2:
    case DW_FORM_block1:
    case DW_FORM_block:;
      Dwarf_Block block;
      if (unlikely (dwarf_formblock (attrp, &block) != 0))
	goto attrval_out;

      printf ("           %*s%-20s %" PRIxMAX " byte block\n",
	      (int) (level * 2), "", dwarf_attr_string (attr),
	      (uintmax_t) block.length);

      switch (attr)
	{
	case DW_AT_location:
	case DW_AT_data_member_location:
	case DW_AT_vtable_elem_location:
	case DW_AT_string_length:
	case DW_AT_use_location:
	case DW_AT_frame_base:
	case DW_AT_return_addr:
	case DW_AT_static_link:
	  print_ops (cbargs->dbg, 12 + level * 2, 12 + level * 2,
		     cbargs->addrsize, block.length, block.data);
	  break;
	}
      break;

    default:
      printf ("           %*s%-20s [form: %d] ???\n",
	      (int) (level * 2), "", dwarf_attr_string (attr),
	      (int) form);
      break;
    }

  return DWARF_CB_OK;
}


static void
print_debug_info_section (Ebl *ebl __attribute__ ((unused)),
			  GElf_Ehdr *ehdr __attribute__ ((unused)),
			  Elf_Scn *scn __attribute__ ((unused)),
			  GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\
\nDWARF section '%s' at offset %#" PRIx64 ":\n [Offset]\n"),
	  ".debug_info", (uint64_t) shdr->sh_offset);

  /* If the section is empty we don't have to do anything.  */
  if (shdr->sh_size == 0)
    return;

  int maxdies = 20;
  Dwarf_Die *dies = (Dwarf_Die *) xmalloc (maxdies * sizeof (Dwarf_Die));

  Dwarf_Off offset = 0;

  /* New compilation unit.  */
  size_t cuhl;
  //Dwarf_Half version;
  Dwarf_Off abbroffset;
  uint8_t addrsize;
  uint8_t offsize;
  Dwarf_Off nextcu;
 next_cu:
  if (dwarf_nextcu (dbg, offset, &nextcu, &cuhl, &abbroffset, &addrsize,
		    &offsize) != 0)
    goto do_return;

  printf (gettext (" Compilation unit at offset %" PRIu64 ":\n"
		   " Version: %" PRIu16 ", Abbreviation section offset: %"
		   PRIu64 ", Address size: %" PRIu8 ", Offset size: %" PRIu8 "\n"),
	  (uint64_t) offset, /*version*/2, abbroffset, addrsize, offsize);


  struct attrcb_args args;
  args.dbg = dbg;
  args.addrsize = addrsize;
  args.cu_offset = offset;

  offset += cuhl;

  int level = 0;

  if (unlikely (dwarf_offdie (dbg, offset, &dies[level]) == NULL))
    {
      error (0, 0, gettext ("cannot get DIE at offset %" PRIu64
			    " in section '%s': %s"),
	     (uint64_t) offset, ".debug_info", dwarf_errmsg (-1));
      goto do_return;
    }

  do
    {
      offset = dwarf_dieoffset (&dies[level]);
      if (offset == ~0ul)
	{
	  error (0, 0, gettext ("cannot get DIE offset: %s"),
		 dwarf_errmsg (-1));
	  goto do_return;
	}

      int tag = dwarf_tag (&dies[level]);
      if (tag == DW_TAG_invalid)
	{
	  error (0, 0, gettext ("cannot get tag of DIE at offset %" PRIu64
				" in section '%s': %s"),
		 (uint64_t) offset, ".debug_info", dwarf_errmsg (-1));
	  goto do_return;
	}

#if 1
      const char *tagstr = dwarf_tag_string (tag);
#else
      static const char *const lowtags[] =
	{
	  [DW_TAG_array_type] = "array_type",
	  [DW_TAG_class_type] = "class_type",
	  [DW_TAG_entry_point] = "entry_point",
	  [DW_TAG_enumeration_type] = "enumeration_type",
	  [DW_TAG_formal_parameter] = "formal_parameter",
	  [DW_TAG_imported_declaration] = "imported_declaration",
	  [DW_TAG_label] = "label",
	  [DW_TAG_lexical_block] = "lexical_block",
	  [DW_TAG_member] = "member",
	  [DW_TAG_pointer_type] = "pointer_type",
	  [DW_TAG_reference_type] = "reference_type",
	  [DW_TAG_compile_unit] = "compile_unit",
	  [DW_TAG_string_type] = "string_type",
	  [DW_TAG_structure_type] = "structure_type",
	  [DW_TAG_subroutine_type] = "subroutine_type",
	  [DW_TAG_typedef] = "typedef",
	  [DW_TAG_union_type] = "union_type",
	  [DW_TAG_unspecified_parameters] = "unspecified_parameters",
	  [DW_TAG_variant] = "variant",
	  [DW_TAG_common_block] = "common_block",
	  [DW_TAG_common_inclusion] = "common_inclusion",
	  [DW_TAG_inheritance] = "inheritance",
	  [DW_TAG_inlined_subroutine] = "inlined_subroutine",
	  [DW_TAG_module] = "module",
	  [DW_TAG_ptr_to_member_type] = "ptr_to_member_type",
	  [DW_TAG_set_type] = "set_type",
	  [DW_TAG_subrange_type] = "subrange_type",
	  [DW_TAG_with_stmt] = "with_stmt",
	  [DW_TAG_access_declaration] = "access_declaration",
	  [DW_TAG_base_type] = "base_type",
	  [DW_TAG_catch_block] = "catch_block",
	  [DW_TAG_const_type] = "const_type",
	  [DW_TAG_constant] = "constant",
	  [DW_TAG_enumerator] = "enumerator",
	  [DW_TAG_file_type] = "file_type",
	  [DW_TAG_friend] = "friend",
	  [DW_TAG_namelist] = "namelist",
	  [DW_TAG_namelist_item] = "namelist_item",
	  [DW_TAG_packed_type] = "packed_type",
	  [DW_TAG_subprogram] = "subprogram",
	  [DW_TAG_template_type_param] = "template_type_param",
	  [DW_TAG_template_value_param] = "template_value_param",
	  [DW_TAG_thrown_type] = "thrown_type",
	  [DW_TAG_try_block] = "try_block",
	  [DW_TAG_variant_part] = "variant_part",
	  [DW_TAG_variable] = "variable",
	  [DW_TAG_volatile_type] = "volatile_type"
	};

      const char *tagstr;
      switch (tag)
	{
	case DW_TAG_lo_user:
	  tagstr = "lo_user";
	  break;

	case DW_TAG_MIPS_loop:
	  tagstr = "MIPS_loop";
	  break;

	case DW_TAG_format_label:
	  tagstr = "format_label";
	  break;

	case DW_TAG_function_template:
	  tagstr = "function_template";
	  break;

	case DW_TAG_class_template:
	  tagstr = "class_template";
	  break;
	case DW_TAG_hi_user:
	  tagstr = "hi_user";
	  break;

	default:
	  if (tag < (int) (sizeof (lowtags) / sizeof (lowtags[0])))
	    tagstr = lowtags[tag];
	  else
	    tagstr = "???";
	  break;
	}
#endif

      printf (" [%6" PRIx64 "]  %*s%s\n",
	      (uint64_t) offset, (int) (level * 2), "", tagstr);

      /* Print the attribute values.  */
      args.level = level;
      (void) dwarf_getattrs (&dies[level], attr_callback, &args, 0);

      /* Make room for the next level's DIE.  */
      if (level + 1 == maxdies)
	dies = (Dwarf_Die *) xrealloc (dies,
				       (maxdies += 10)
				       * sizeof (Dwarf_Die));

      int res = dwarf_child (&dies[level], &dies[level + 1]);
      if (res > 0)
	{
	  while ((res = dwarf_siblingof (&dies[level], &dies[level])) == 1)
	    if (level-- == 0)
	      break;

	  if (res == -1)
	    {
	      error (0, 0, gettext ("cannot get next DIE: %s\n"),
		     dwarf_errmsg (-1));
	      goto do_return;
	    }
	}
      else if (unlikely (res < 0))
	{
	  error (0, 0, gettext ("cannot get next DIE: %s"),
		 dwarf_errmsg (-1));
	  goto do_return;
	}
      else
	++level;
    }
  while (level >= 0);

  offset = nextcu;
  if (offset != 0)
     goto next_cu;

 do_return:
  free (dies);
}


static void
print_debug_line_section (Ebl *ebl, GElf_Ehdr *ehdr __attribute__ ((unused)),
			  Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\
\nDWARF section '%s' at offset %#" PRIx64 ":\n"),
	  ".debug_line", (uint64_t) shdr->sh_offset);

  if (shdr->sh_size == 0)
    return;

  /* There is no functionality in libdw to read the information in the
     way it is represented here.  Hardcode the decoder.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL || data->d_buf == NULL)
    {
      error (0, 0, gettext ("cannot get line data section data: %s"),
	     elf_errmsg (-1));
      return;
    }

  const unsigned char *linep = (const unsigned char *) data->d_buf;
  const unsigned char *lineendp;

  while (linep
	 < (lineendp = (const unsigned char *) data->d_buf + data->d_size))
    {
      size_t start_offset = linep - (const unsigned char *) data->d_buf;

      printf (gettext ("\nTable at offset %Zu:\n"), start_offset);

      Dwarf_Word unit_length = read_4ubyte_unaligned_inc (dbg, linep);
      unsigned int length = 4;
      if (unlikely (unit_length == 0xffffffff))
	{
	  if (unlikely (linep + 8 > lineendp))
	    {
	    invalid_data:
	      error (0, 0, gettext ("invalid data in section [%zu] '%s'"),
		     elf_ndxscn (scn), ".debug_line");
	      return;
	    }
	  unit_length = read_8ubyte_unaligned_inc (dbg, linep);
	  length = 8;
	}

      /* Check whether we have enough room in the section.  */
      if (unit_length < 2 + length + 5 * 1
	  || unlikely (linep + unit_length > lineendp))
	goto invalid_data;
      lineendp = linep + unit_length;

      /* The next element of the header is the version identifier.  */
      uint_fast16_t version = read_2ubyte_unaligned_inc (dbg, linep);

      /* Next comes the header length.  */
      Dwarf_Word header_length;
      if (length == 4)
	header_length = read_4ubyte_unaligned_inc (dbg, linep);
      else
	header_length = read_8ubyte_unaligned_inc (dbg, linep);
      //const unsigned char *header_start = linep;

      /* Next the minimum instruction length.  */
      uint_fast8_t minimum_instr_len = *linep++;

        /* Then the flag determining the default value of the is_stmt
	   register.  */
      uint_fast8_t default_is_stmt = *linep++;

      /* Now the line base.  */
      int_fast8_t line_base = *((const int_fast8_t *) linep);
      ++linep;

      /* And the line range.  */
      uint_fast8_t line_range = *linep++;

      /* The opcode base.  */
      uint_fast8_t opcode_base = *linep++;

      /* Print what we got so far.  */
      printf (gettext ("\n"
		       " Length:                     %" PRIu64 "\n"
		       " DWARF version:              %" PRIuFAST16 "\n"
		       " Prologue length:            %" PRIu64 "\n"
		       " Minimum instruction length: %" PRIuFAST8 "\n"
		       " Initial value if '%s': %" PRIuFAST8 "\n"
		       " Line base:                  %" PRIdFAST8 "\n"
		       " Line range:                 %" PRIuFAST8 "\n"
		       " Opcode base:                %" PRIuFAST8 "\n"
		       "\n"
		       "Opcodes:\n"),
	      (uint64_t) unit_length, version, (uint64_t) header_length,
	      minimum_instr_len, "is_stmt", default_is_stmt, line_base,
	      line_range, opcode_base);

      if (unlikely (linep + opcode_base - 1 >= lineendp))
	goto invalid_data;
      int opcode_base_l10 = 1;
      unsigned int tmp = opcode_base;
      while (tmp > 10)
	{
	  tmp /= 10;
	  ++opcode_base_l10;
	}
      const uint8_t *standard_opcode_lengths = linep - 1;
      for (uint_fast8_t cnt = 1; cnt < opcode_base; ++cnt)
	printf (ngettext ("  [%*" PRIuFAST8 "]  %hhu argument\n",
			  "  [%*" PRIuFAST8 "]  %hhu arguments\n",
			  (int) linep[cnt - 1]),
		opcode_base_l10, cnt, linep[cnt - 1]);
      linep += opcode_base - 1;
      if (unlikely (linep >= lineendp))
	goto invalid_data;

      puts (gettext ("\nDirectory table:"));
      while (*linep != 0)
	{
	  unsigned char *endp = memchr (linep, '\0', lineendp - linep);
	  if (endp == NULL)
	    goto invalid_data;

	  printf (" %s\n", (char *) linep);

	  linep = endp + 1;
	}
      /* Skip the final NUL byte.  */
      ++linep;

      if (unlikely (linep >= lineendp))
	goto invalid_data;
      puts (gettext ("\nFile name table:\n"
		     " Entry Dir   Time      Size      Name"));
      for (unsigned int cnt = 1; *linep != 0; ++cnt)
	{
	  /* First comes the file name.  */
	  char *fname = (char *) linep;
	  unsigned char *endp = memchr (fname, '\0', lineendp - linep);
	  if (endp == NULL)
	    goto invalid_data;
	  linep = endp + 1;

	  /* Then the index.  */
	  unsigned int diridx;
	  get_uleb128 (diridx, linep);

	  /* Next comes the modification time.  */
	  unsigned int mtime;
	  get_uleb128 (mtime, linep);

	  /* Finally the length of the file.  */
	  unsigned int fsize;
	  get_uleb128 (fsize, linep);

	  printf (" %-5u %-5u %-9u %-9u %s\n",
		  cnt, diridx, mtime, fsize, fname);
	}
      /* Skip the final NUL byte.  */
      ++linep;

      puts (gettext ("\nLine number statements:"));
      Dwarf_Word address = 0;
      size_t line = 1;
      uint_fast8_t is_stmt = default_is_stmt;

      /* Default address value, in case we do not find the CU.  */
      size_t address_size
	= elf_getident (ebl->elf, NULL)[EI_CLASS] == ELFCLASS32 ? 4 : 8;

      /* Determine the CU this block is for.  */
      Dwarf_Off cuoffset;
      Dwarf_Off ncuoffset = 0;
      size_t hsize;
      while (dwarf_nextcu (dbg, cuoffset = ncuoffset, &ncuoffset, &hsize,
			   NULL, NULL, NULL) == 0)
	{
	  Dwarf_Die cudie;
	  if (dwarf_offdie (dbg, cuoffset + hsize, &cudie) == NULL)
	    continue;
	  Dwarf_Attribute stmt_list;
	  if (dwarf_attr (&cudie, DW_AT_stmt_list, &stmt_list) == NULL)
	    continue;
	  Dwarf_Word lineoff;
	  if (dwarf_formudata (&stmt_list, &lineoff) != 0)
	    continue;
	  if (lineoff == start_offset)
	    {
	      /* Found the CU.  */
	      address_size = cudie.cu->address_size;
	      break;
	    }
	}

      while (linep < lineendp)
	{
	  unsigned int u128;
	  int s128;

	  /* Read the opcode.  */
	  unsigned int opcode = *linep++;

	  /* Is this a special opcode?  */
	  if (likely (opcode >= opcode_base))
	    {
	      /* Yes.  Handling this is quite easy since the opcode value
		 is computed with

		 opcode = (desired line increment - line_base)
		           + (line_range * address advance) + opcode_base
	      */
	      int line_increment = (line_base
				    + (opcode - opcode_base) % line_range);
	      unsigned int address_increment = (minimum_instr_len
						* ((opcode - opcode_base)
						   / line_range));

	      /* Perform the increments.  */
	      line += line_increment;
	      address += address_increment;

	      printf (gettext ("\
 special opcode %u: address+%u = %#" PRIx64 ", line%+d = %zu\n"),
		      opcode, address_increment, (uint64_t) address,
		      line_increment, line);
	    }
	  else if (opcode == 0)
	    {
	      /* This an extended opcode.  */
	      if (unlikely (linep + 2 > lineendp))
		goto invalid_data;

	      /* The length.  */
	      unsigned int len = *linep++;

	      if (unlikely (linep + len > lineendp))
		goto invalid_data;

	      /* The sub-opcode.  */
	      opcode = *linep++;

	      printf (gettext (" extended opcode %u: "), opcode);

	      switch (opcode)
		{
		case DW_LNE_end_sequence:
		  puts (gettext ("end of sequence"));

		  /* Reset the registers we care about.  */
		  address = 0;
		  line = 1;
		  is_stmt = default_is_stmt;
		  break;

		case DW_LNE_set_address:
		  if (address_size == 4)
		    address = read_4ubyte_unaligned_inc (dbg, linep);
		  else
		    address = read_8ubyte_unaligned_inc (dbg, linep);
		  printf (gettext ("set address to %#" PRIx64 "\n"),
			  (uint64_t) address);
		  break;

		case DW_LNE_define_file:
		  {
		    char *fname = (char *) linep;
		    unsigned char *endp = memchr (linep, '\0',
						  lineendp - linep);
		    if (endp == NULL)
		      goto invalid_data;
		    linep = endp + 1;

		    unsigned int diridx;
		    get_uleb128 (diridx, linep);
		    Dwarf_Word mtime;
		    get_uleb128 (mtime, linep);
		    Dwarf_Word filelength;
		    get_uleb128 (filelength, linep);

		    printf (gettext ("\
define new file: dir=%u, mtime=%" PRIu64 ", length=%" PRIu64 ", name=%s\n"),
			    diridx, (uint64_t) mtime, (uint64_t) filelength,
			    fname);
		  }
		  break;

		default:
		  /* Unknown, ignore it.  */
		  puts (gettext ("unknown opcode"));
		  linep += len - 1;
		  break;
		}
	    }
	  else if (opcode <= DW_LNS_set_epilogue_begin)
	    {
	      /* This is a known standard opcode.  */
	      switch (opcode)
		{
		case DW_LNS_copy:
		  /* Takes no argument.  */
		  puts (gettext (" copy"));
		  break;

		case DW_LNS_advance_pc:
		  /* Takes one uleb128 parameter which is added to the
		     address.  */
		  get_uleb128 (u128, linep);
		  address += minimum_instr_len * u128;
		  printf (gettext ("\
 advance address by %u to %#" PRIx64 "\n"),
			  u128, (uint64_t) address);
		  break;

		case DW_LNS_advance_line:
		  /* Takes one sleb128 parameter which is added to the
		     line.  */
		  get_sleb128 (s128, linep);
		  line += s128;
		  printf (gettext ("\
 advance line by constant %d to %" PRId64 "\n"),
			  s128, (int64_t) line);
		  break;

		case DW_LNS_set_file:
		  /* Takes one uleb128 parameter which is stored in file.  */
		  get_uleb128 (u128, linep);
		  printf (gettext (" set file to %" PRIu64 "\n"),
			  (uint64_t) u128);
		  break;

		case DW_LNS_set_column:
		  /* Takes one uleb128 parameter which is stored in column.  */
		  if (unlikely (standard_opcode_lengths[opcode] != 1))
		    goto invalid_data;

		  get_uleb128 (u128, linep);
		  printf (gettext (" set column to %" PRIu64 "\n"),
			  (uint64_t) u128);
		  break;

		case DW_LNS_negate_stmt:
		  /* Takes no argument.  */
		  is_stmt = 1 - is_stmt;
		  printf (gettext (" set '%s' to %" PRIuFAST8 "\n"),
			  "is_stmt", is_stmt);
		  break;

		case DW_LNS_set_basic_block:
		  /* Takes no argument.  */
		  puts (gettext (" set basic block flag"));
		  break;

		case DW_LNS_const_add_pc:
		  /* Takes no argument.  */
		  u128 = (minimum_instr_len
			  * ((255 - opcode_base) / line_range));
		  address += u128;
		  printf (gettext ("\
 advance address by constant %u to %#" PRIx64 "\n"),
			  u128, (uint64_t) address);
		  break;

		case DW_LNS_fixed_advance_pc:
		  /* Takes one 16 bit parameter which is added to the
		     address.  */
		  if (unlikely (standard_opcode_lengths[opcode] != 1))
		    goto invalid_data;

		  u128 = read_2ubyte_unaligned_inc (dbg, linep);
		  address += u128;
		  printf (gettext ("\
 advance address by fixed value %u to %#" PRIx64 "\n"),
			  u128, (uint64_t) address);
		  break;

		case DW_LNS_set_prologue_end:
		  /* Takes no argument.  */
		  puts (gettext (" set prologue end flag"));
		  break;

		case DW_LNS_set_epilogue_begin:
		  /* Takes no argument.  */
		  puts (gettext (" set epilogue begin flag"));
		  break;
		}
	    }
	  else
	    {
	      /* This is a new opcode the generator but not we know about.
		 Read the parameters associated with it but then discard
		 everything.  Read all the parameters for this opcode.  */
	      printf (ngettext (" unknown opcode with %" PRIu8 " parameter:",
				" unknown opcode with %" PRIu8 " parameters:",
				standard_opcode_lengths[opcode]),
		      standard_opcode_lengths[opcode]);
	      for (int n = standard_opcode_lengths[opcode]; n > 0; --n)
		{
		  get_uleb128 (u128, linep);
		  if (n != standard_opcode_lengths[opcode])
		    putc_unlocked (',', stdout);
		  printf (" %u", u128);
		}

	      /* Next round, ignore this opcode.  */
	      continue;
	    }
	}
    }

  /* There must only be one data block.  */
  assert (elf_getdata (scn, data) == NULL);
}


static void
print_debug_loc_section (Ebl *ebl __attribute__ ((unused)),
			 GElf_Ehdr *ehdr __attribute__ ((unused)),
			 Elf_Scn *scn __attribute__ ((unused)),
			 GElf_Shdr *shdr,
			 Dwarf *dbg __attribute__ ((unused)))
{
  Elf_Data *data = elf_rawdata (scn, NULL);

  if (data == NULL)
    {
      error (0, 0, gettext ("cannot get .debug_loc content: %s"),
	     elf_errmsg (-1));
      return;
    }

  printf (gettext ("\
\nDWARF section '%s' at offset %#" PRIx64 ":\n"),
	  ".debug_loc", (uint64_t) shdr->sh_offset);

  size_t address_size = ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;

  bool first = true;
  unsigned char *readp = data->d_buf;
  while (readp < (unsigned char *) data->d_buf + data->d_size)
    {
      ptrdiff_t offset = readp - (unsigned char *) data->d_buf;

      if (data->d_size - offset < address_size * 2)
	{
	  printf (" [%6tx]  <INVALID DATA>\n", offset);
	  break;
	}

      Dwarf_Addr begin;
      Dwarf_Addr end;
      if (address_size == 8)
	{
	  begin = read_8ubyte_unaligned_inc (dbg, readp);
	  end = read_8ubyte_unaligned_inc (dbg, readp);
	}
      else
	{
	  begin = read_4ubyte_unaligned_inc (dbg, readp);
	  end = read_4ubyte_unaligned_inc (dbg, readp);
	  if (begin == (Dwarf_Addr) (uint32_t) -1)
	    begin = (Dwarf_Addr) -1l;
	}

      if (begin == (Dwarf_Addr) -1l) /* Base address entry.  */
	printf (" [%6tx]  base address %#0*" PRIxMAX "\n", offset,
		2 + (int) (address_size * 2), (uintmax_t) end);
      else if (begin == 0 && end == 0) /* End of list entry.  */
	first = true;
      else
	{
	  /* We have a location expression entry.  */
	  uint_fast16_t len = read_2ubyte_unaligned_inc (dbg, readp);

	  if (first)		/* First entry in a list.  */
	    printf (" [%6tx]  %#0*" PRIxMAX "..%#0*" PRIxMAX,
		    offset,
		    2 + (int) (address_size * 2), (uintmax_t) begin,
		    2 + (int) (address_size * 2), (uintmax_t) end);
	  else
	    printf ("           %#0*" PRIxMAX "..%#0*" PRIxMAX,
		    2 + (int) (address_size * 2), (uintmax_t) begin,
		    2 + (int) (address_size * 2), (uintmax_t) end);

	  print_ops (dbg, 1, 18 + (address_size * 4),
		     address_size, len, readp);

	  first = false;
	  readp += len;
	}
    }
}

struct mac_culist
{
  Dwarf_Die die;
  Dwarf_Off offset;
  Dwarf_Files *files;
  struct mac_culist *next;
};


static int
mac_compare (const void *p1, const void *p2)
{
  struct mac_culist *m1 = (struct mac_culist *) p1;
  struct mac_culist *m2 = (struct mac_culist *) p2;

  if (m1->offset < m2->offset)
    return -1;
  if (m1->offset > m2->offset)
    return 1;
  return 0;
}


static void
print_debug_macinfo_section (Ebl *ebl __attribute__ ((unused)),
			     GElf_Ehdr *ehdr __attribute__ ((unused)),
			     Elf_Scn *scn, GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\
\nDWARF section '%s' at offset %#" PRIx64 ":\n"),
	  ".debug_macinfo", (uint64_t) shdr->sh_offset);
  putc_unlocked ('\n', stdout);

  /* There is no function in libdw to iterate over the raw content of
     the section but it is easy enough to do.  */
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL || data->d_buf == NULL)
    {
      error (0, 0, gettext ("cannot get macro information section data: %s"),
	     elf_errmsg (-1));
      return;
    }

  /* Get the source file information for all CUs.  */
  Dwarf_Off offset;
  Dwarf_Off ncu = 0;
  size_t hsize;
  struct mac_culist *culist = NULL;
  size_t nculist = 0;
  while (dwarf_nextcu (dbg, offset = ncu, &ncu, &hsize, NULL, NULL, NULL) == 0)
    {
      Dwarf_Die cudie;
      if (dwarf_offdie (dbg, offset + hsize, &cudie) == NULL)
	continue;

      Dwarf_Attribute attr;
      if (dwarf_attr (&cudie, DW_AT_macro_info, &attr) == NULL)
	continue;

      Dwarf_Word macoff;
      if (dwarf_formudata (&attr, &macoff) != 0)
	continue;

      struct mac_culist *newp = (struct mac_culist *) alloca (sizeof (*newp));
      newp->die = cudie;
      newp->offset = macoff;
      newp->files = NULL;
      newp->next = culist;
      culist = newp;
      ++nculist;
    }

  /* Convert the list into an array for easier consumption.  */
  struct mac_culist *cus = (struct mac_culist *) alloca ((nculist + 1)
							 * sizeof (*cus));
  /* Add sentinel.  */
  cus[nculist].offset = data->d_size;
  if (nculist > 0)
    {
      for (size_t cnt = nculist - 1; culist != NULL; --cnt)
	{
	  assert (cnt < nculist);
	  cus[cnt] = *culist;
	  culist = culist->next;
	}

      /* Sort the array according to the offset in the .debug_macinfo
	 section.  Note we keep the sentinel at the end.  */
      qsort (cus, nculist, sizeof (*cus), mac_compare);
    }

  const unsigned char *readp = (const unsigned char *) data->d_buf;
  const unsigned char *readendp = readp + data->d_size;
  int level = 1;

  while (readp < readendp)
    {
      unsigned int opcode = *readp++;
      unsigned int u128;
      unsigned int u128_2;
      const unsigned char *endp;

      switch (opcode)
	{
	case DW_MACINFO_define:
	case DW_MACINFO_undef:
	case DW_MACINFO_vendor_ext:
	  /*  For the first two opcodes the parameters are
	        line, string
	      For the latter
	        number, string.
	      We can treat these cases together.  */
	  get_uleb128 (u128, readp);

	  endp = memchr (readp, '\0', readendp - readp);
	  if (endp == NULL)
	    {
	      printf (gettext ("\
%*s*** non-terminated string at end of section"),
		      level, "");
	      return;
	    }

	  if (opcode == DW_MACINFO_define)
	    printf ("%*s#define %s, line %u\n",
		    level, "", (char *) readp, u128);
	  else if (opcode == DW_MACINFO_undef)
	    printf ("%*s#undef %s, line %u\n",
		    level, "", (char *) readp, u128);
	  else
	    printf (" #vendor-ext %s, number %u\n", (char *) readp, u128);

	  readp = endp + 1;
	  break;

	case DW_MACINFO_start_file:
	  /* The two parameters are line and file index, in this order.  */
	  get_uleb128 (u128, readp);
	  get_uleb128 (u128_2, readp);

	  /* Find the CU DIE for this file.  */
	  size_t macoff = readp - (const unsigned char *) data->d_buf;
	  const char *fname = "???";
	  if (macoff >= cus[0].offset)
	    {
	      while (macoff >= cus[1].offset)
		++cus;

	      if (cus[0].files == NULL
		&& dwarf_getsrcfiles (&cus[0].die, &cus[0].files, NULL) != 0)
		cus[0].files = (Dwarf_Files *) -1l;

	      if (cus[0].files != (Dwarf_Files *) -1l)
		fname = (dwarf_filesrc (cus[0].files, u128_2, NULL, NULL)
			 ?: "???");
	    }

	  printf ("%*sstart_file %u, [%u] %s\n",
		  level, "", u128, u128_2, fname);
	  ++level;
	  break;

	case DW_MACINFO_end_file:
	  --level;
	  printf ("%*send_file\n", level, "");
	  /* Nothing more to do.  */
	  break;

	default:
	  // XXX gcc seems to generate files with a trailing zero.
	  if (opcode != 0 || readp != readendp)
	    printf ("%*s*** invalid opcode %u\n", level, "", opcode);
	  break;
	}
    }
}


/* Callback for printing global names.  */
static int
print_pubnames (Dwarf *dbg __attribute__ ((unused)), Dwarf_Global *global,
		void *arg)
{
  int *np = (int *) arg;

  printf (gettext (" [%5d] DIE offset: %6" PRId64
		   ", CU DIE offset: %6" PRId64 ", name: %s\n"),
	  (*np)++, global->die_offset, global->cu_offset, global->name);

  return 0;
}


/* Print the known exported symbols in the DWARF section '.debug_pubnames'.  */
static void
print_debug_pubnames_section (Ebl *ebl __attribute__ ((unused)),
			      GElf_Ehdr *ehdr __attribute__ ((unused)),
			      Elf_Scn *scn __attribute__ ((unused)),
			      GElf_Shdr *shdr, Dwarf *dbg)
{
  printf (gettext ("\nDWARF section '%s' at offset %#" PRIx64 ":\n"),
	  ".debug_pubnames", (uint64_t) shdr->sh_offset);

  int n = 0;
  (void) dwarf_getpubnames (dbg, print_pubnames, &n, 0);
}

/* Print the content of the DWARF string section '.debug_str'.  */
static void
print_debug_str_section (Ebl *ebl __attribute__ ((unused)),
			 GElf_Ehdr *ehdr __attribute__ ((unused)),
			 Elf_Scn *scn __attribute__ ((unused)),
			 GElf_Shdr *shdr, Dwarf *dbg)
{
  /* Compute floor(log16(shdr->sh_size)).  */
  GElf_Addr tmp = shdr->sh_size;
  int digits = 1;
  while (tmp >= 16)
    {
      ++digits;
      tmp >>= 4;
    }
  digits = MAX (4, digits);

  printf (gettext ("\nDWARF section '%s' at offset %#" PRIx64 ":\n"
		   " %*s  String\n"),
	  ".debug_str", (uint64_t) shdr->sh_offset,
	  /* TRANS: the debugstr| prefix makes the string unique.  */
	  digits + 2, sgettext ("debugstr|Offset"));

  Dwarf_Off offset = 0;
  while (offset < shdr->sh_size)
    {
      size_t len;
      const char *str = dwarf_getstring (dbg, offset, &len);
      if (str == NULL)
	{
	  printf (gettext (" *** error while reading strings: %s\n"),
		  dwarf_errmsg (-1));
	  break;
	}

      printf (" [%*" PRIx64 "]  \"%s\"\n", digits, (uint64_t) offset, str);

      offset += len + 1;
    }
}


static void
print_debug (Ebl *ebl, GElf_Ehdr *ehdr)
{
  /* Find the version information sections.  For this we have to
     search through the section table.  */
  Dwarf *dbg;
  Elf_Scn *scn;
  size_t shstrndx;

  /* Before we start the real work get a debug context descriptor.  */
  dbg = dwarf_begin_elf (ebl->elf, DWARF_C_READ, NULL);
  if (dbg == NULL)
    {
      error (0, 0, gettext ("cannot get debug context descriptor: %s"),
	     dwarf_errmsg (-1));
      return;
    }

  /* Get the section header string table index.  */
  if (elf_getshstrndx (ebl->elf, &shstrndx) < 0)
    error (EXIT_FAILURE, 0,
	   gettext ("cannot get section header string table index"));

  scn = NULL;
  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      /* Handle the section if it is part of the versioning handling.  */
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr != NULL || shdr->sh_type != SHT_PROGBITS)
	{
	  static const struct
	  {
	    const char *name;
	    enum section_e bitmask;
	    void (*fp) (Ebl *, GElf_Ehdr *, Elf_Scn *, GElf_Shdr *, Dwarf *);
	  } debug_sections[] =
	    {
#define NEW_SECTION(name) \
	      { ".debug_" #name, section_##name, print_debug_##name##_section }
	      NEW_SECTION (abbrev),
	      NEW_SECTION (aranges),
	      NEW_SECTION (frame),
	      NEW_SECTION (info),
	      NEW_SECTION (line),
	      NEW_SECTION (loc),
	      NEW_SECTION (pubnames),
	      NEW_SECTION (str),
	      NEW_SECTION (macinfo),
	      NEW_SECTION (ranges),
	      { ".eh_frame", section_frame, print_debug_frame_section }
	    };
	  const int ndebug_sections = (sizeof (debug_sections)
				       / sizeof (debug_sections[0]));
	  const char *name = elf_strptr (ebl->elf, shstrndx,
					 shdr->sh_name);
	  int n;

	  for (n = 0; n < ndebug_sections; ++n)
	    if (strcmp (name, debug_sections[n].name) == 0)
	      {
		if (print_debug_sections & debug_sections[n].bitmask)
		  debug_sections[n].fp (ebl, ehdr, scn, shdr, dbg);
		break;
	      }
	}
    }

  /* We are done with the DWARF handling.  */
  dwarf_end (dbg);
}


static void
handle_notes (Ebl *ebl, GElf_Ehdr *ehdr)
{
  int class = gelf_getclass (ebl->elf);
  size_t cnt;

  /* We have to look through the program header to find the note
     sections.  There can be more than one.  */
  for (cnt = 0; cnt < ehdr->e_phnum; ++cnt)
    {
      GElf_Phdr mem;
      GElf_Phdr *phdr = gelf_getphdr (ebl->elf, cnt, &mem);

      if (phdr == NULL || phdr->p_type != PT_NOTE)
	/* Not what we are looking for.  */
	continue;

      printf (gettext ("\
\nNote segment of %" PRId64 " bytes at offset %#0" PRIx64 ":\n"),
	      phdr->p_filesz, phdr->p_offset);

      char *notemem = gelf_rawchunk (ebl->elf, phdr->p_offset, phdr->p_filesz);
      if (notemem == NULL)
	error (EXIT_FAILURE, 0,
	       gettext ("cannot get content of note section: %s"),
	       elf_errmsg (-1));

      fputs_unlocked (gettext ("  Owner          Data size  Type\n"), stdout);


      /* Handle the note section content.  It consists of one or more
	 entries each of which consists of five parts:

	 - a 32-bit name length
	 - a 32-bit descriptor length
	 - a 32-bit type field
	 - the NUL-terminated name, length as specified in the first field
	 - the descriptor, length as specified in the second field

	 The variable sized fields are padded to 32- or 64-bits
	 depending on whether the file is a 32- or 64-bit ELF file.
      */
      // XXX Which 64-bit archs need 8-byte alignment?  x86-64 does not.
      size_t align = class == ELFCLASS32 ? 4 : 4; // XXX 8;
#define ALIGNED_LEN(len) (((len) + align - 1) & ~(align - 1))

      size_t idx = 0;
      while (idx < phdr->p_filesz)
	{
	  /* XXX Handle 64-bit note section entries correctly.  */
	  struct
	  {
	    uint32_t namesz;
	    uint32_t descsz;
	    uint32_t type;
	    char name[0];
	  } *noteentry = (__typeof (noteentry)) (notemem + idx);

	  if (idx + 12 > phdr->p_filesz
	      || (idx + 12 + ALIGNED_LEN (noteentry->namesz)
		  + ALIGNED_LEN (noteentry->descsz) > phdr->p_filesz))
	    /* This entry isn't completely contained in the note
	       section.  Ignore it.  */
	    break;

	  char buf[100];
	  char buf2[100];
	  printf (gettext ("  %-13.*s  %9" PRId32 "  %s\n"),
		  (int) noteentry->namesz, noteentry->name,
		  noteentry->descsz,
		  ehdr->e_type == ET_CORE
		  ? ebl_core_note_type_name (ebl, noteentry->type,
					     buf, sizeof (buf))
		  : ebl_object_note_type_name (ebl, noteentry->type,
					       buf2, sizeof (buf2)));

	  /* Filter out invalid entries.  */
	  if (memchr (noteentry->name, '\0', noteentry->namesz) != NULL
	      /* XXX For now help broken Linux kernels.  */
	      || 1)
	    {
	      if (ehdr->e_type == ET_CORE)
		ebl_core_note (ebl, noteentry->name, noteentry->type,
			       noteentry->descsz,
			       &noteentry->name[ALIGNED_LEN (noteentry->namesz)]);
	      else
		ebl_object_note (ebl, noteentry->name, noteentry->type,
				 noteentry->descsz,
				 &noteentry->name[ALIGNED_LEN (noteentry->namesz)]);
	    }

	  /* Move to the next entry.  */
	  idx += (12 + ALIGNED_LEN (noteentry->namesz)
		  + ALIGNED_LEN (noteentry->descsz));
	}

      gelf_freechunk (ebl->elf, notemem);
    }
}
