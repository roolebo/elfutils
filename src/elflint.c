/* Pedantic checking of ELF files compliance with gABI/psABI spec.
   Copyright (C) 2001, 2002, 2003, 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

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
#include <byteswap.h>
#include <endian.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <inttypes.h>
#include <libintl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>

#include <elf-knowledge.h>
#include <system.h>
#include "../libebl/libeblP.h"


/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/* Bug report address.  */
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

#define ARGP_strict	300
#define ARGP_gnuld	301

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{

  { "strict", ARGP_strict, NULL, 0,
    N_("Be extremely strict, flag level 2 features."), 0 },
  { "quiet", 'q', NULL, 0, N_("Do not print anything if successful"), 0 },
  { "debuginfo", 'd', NULL, 0, N_("Binary is a separate debuginfo file"), 0 },
  { "gnu-ld", ARGP_gnuld, NULL, 0,
    N_("Binary has been created with GNU ld and is therefore known to be \
broken in certain ways"), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("\
Pedantic checking of ELF files compliance with gABI/psABI spec.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("FILE...");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, NULL, NULL
};


/* Declarations of local functions.  */
static void process_file (int fd, Elf *elf, const char *prefix,
			  const char *suffix, const char *fname, size_t size,
			  bool only_one);
static void process_elf_file (Elf *elf, const char *prefix, const char *suffix,
			      const char *fname, size_t size, bool only_one);

/* Report an error.  */
#define ERROR(str, args...) \
  do {									      \
    printf (str, ##args);						      \
    ++error_count;							      \
  } while (0)
static unsigned int error_count;

/* True if we should perform very strict testing.  */
static bool be_strict;

/* True if no message is to be printed if the run is succesful.  */
static bool be_quiet;

/* True if binary is from strip -f, not a normal ELF file.  */
static bool is_debuginfo;

/* True if binary is assumed to be generated with GNU ld.  */
static bool gnuld;

/* Index of section header string table.  */
static uint32_t shstrndx;

/* Array to count references in section groups.  */
static int *scnref;


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
	ERROR (gettext ("cannot generate Elf descriptor: %s\n"),
	       elf_errmsg (-1));
      else
	{
	  unsigned int prev_error_count = error_count;
	  struct stat64 st;

	  if (fstat64 (fd, &st) != 0)
	    {
	      printf ("cannot stat '%s': %m\n", argv[remaining]);
	      close (fd);
	      continue;
	    }

	  process_file (fd, elf, NULL, NULL, argv[remaining], st.st_size,
			only_one);

	  /* Now we can close the descriptor.  */
	  if (elf_end (elf) != 0)
	    ERROR (gettext ("error while closing Elf descriptor: %s\n"),
		   elf_errmsg (-1));

	  if (prev_error_count == error_count && !be_quiet)
	    puts (gettext ("No errors"));
	}

      close (fd);
    }
  while (++remaining < argc);

  return error_count != 0;
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
	   struct argp_state *state __attribute__ ((unused)))
{
  switch (key)
    {
    case ARGP_strict:
      be_strict = true;
      break;

    case 'q':
      be_quiet = true;
      break;

    case 'd':
      is_debuginfo = true;

    case ARGP_gnuld:
      gnuld = true;
      break;

    case ARGP_KEY_NO_ARGS:
      fputs (gettext ("Missing file name.\n"), stderr);
      argp_help (&argp, stderr, ARGP_HELP_SEE | ARGP_HELP_EXIT_ERR,
		 program_invocation_short_name);
      exit (1);

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "elflint (%s) %s\n", PACKAGE_NAME, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Red Hat, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2005");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* Process one file.  */
static void
process_file (int fd, Elf *elf, const char *prefix, const char *suffix,
	      const char *fname, size_t size, bool only_one)
{
  /* We can handle two types of files: ELF files and archives.  */
  Elf_Kind kind = elf_kind (elf);

  switch (kind)
    {
    case ELF_K_ELF:
      /* Yes!  It's an ELF file.  */
      process_elf_file (elf, prefix, suffix, fname, size, only_one);
      break;

    case ELF_K_AR:
      {
	Elf *subelf;
	Elf_Cmd cmd = ELF_C_READ_MMAP;
	size_t prefix_len = prefix == NULL ? 0 : strlen (prefix);
	size_t fname_len = strlen (fname) + 1;
	char new_prefix[prefix_len + 1 + fname_len];
	char new_suffix[(suffix == NULL ? 0 : strlen (suffix)) + 2];
	char *cp = new_prefix;

	/* Create the full name of the file.  */
	if (prefix != NULL)
	  {
	    cp = mempcpy (cp, prefix, prefix_len);
	    *cp++ = '(';
	    strcpy (stpcpy (new_suffix, suffix), ")");
	  }
	else
	  new_suffix[0] = '\0';
	memcpy (cp, fname, fname_len);

	/* It's an archive.  We process each file in it.  */
	while ((subelf = elf_begin (fd, cmd, elf)) != NULL)
	  {
	    kind = elf_kind (subelf);

	    /* Call this function recursively.  */
	    if (kind == ELF_K_ELF || kind == ELF_K_AR)
	      {
		Elf_Arhdr *arhdr = elf_getarhdr (subelf);
		assert (arhdr != NULL);

		process_file (fd, subelf, new_prefix, new_suffix,
			      arhdr->ar_name, arhdr->ar_size, false);
	      }

	    /* Get next archive element.  */
	    cmd = elf_next (subelf);
	    if (elf_end (subelf) != 0)
	      ERROR (gettext (" error while freeing sub-ELF descriptor: %s\n"),
		     elf_errmsg (-1));
	  }
      }
      break;

    default:
      /* We cannot do anything.  */
      ERROR (gettext ("\
Not an ELF file - it has the wrong magic bytes at the start"));
      break;
    }
}


static const char *
section_name (Ebl *ebl, int idx)
{
  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr;

  shdr = gelf_getshdr (elf_getscn (ebl->elf, idx), &shdr_mem);

  return elf_strptr (ebl->elf, shstrndx, shdr->sh_name);
}


static const int valid_e_machine[] =
  {
    EM_M32, EM_SPARC, EM_386, EM_68K, EM_88K, EM_860, EM_MIPS, EM_S370,
    EM_MIPS_RS3_LE, EM_PARISC, EM_VPP500, EM_SPARC32PLUS, EM_960, EM_PPC,
    EM_PPC64, EM_S390, EM_V800, EM_FR20, EM_RH32, EM_RCE, EM_ARM,
    EM_FAKE_ALPHA, EM_SH, EM_SPARCV9, EM_TRICORE, EM_ARC, EM_H8_300,
    EM_H8_300H, EM_H8S, EM_H8_500, EM_IA_64, EM_MIPS_X, EM_COLDFIRE,
    EM_68HC12, EM_MMA, EM_PCP, EM_NCPU, EM_NDR1, EM_STARCORE, EM_ME16,
    EM_ST100, EM_TINYJ, EM_X86_64, EM_PDSP, EM_FX66, EM_ST9PLUS, EM_ST7,
    EM_68HC16, EM_68HC11, EM_68HC08, EM_68HC05, EM_SVX, EM_ST19, EM_VAX,
    EM_CRIS, EM_JAVELIN, EM_FIREPATH, EM_ZSP, EM_MMIX, EM_HUANY, EM_PRISM,
    EM_AVR, EM_FR30, EM_D10V, EM_D30V, EM_V850, EM_M32R, EM_MN10300,
    EM_MN10200, EM_PJ, EM_OPENRISC, EM_ARC_A5, EM_XTENSA
  };
#define nvalid_e_machine \
  (sizeof (valid_e_machine) / sizeof (valid_e_machine[0]))


/* Number of sections.  */
static unsigned int shnum;


static void
check_elf_header (Ebl *ebl, GElf_Ehdr *ehdr, size_t size)
{
  char buf[512];
  size_t cnt;

  /* Check e_ident field.  */
  if (ehdr->e_ident[EI_MAG0] != ELFMAG0)
    ERROR ("e_ident[%d] != '%c'\n", EI_MAG0, ELFMAG0);
  if (ehdr->e_ident[EI_MAG1] != ELFMAG1)
    ERROR ("e_ident[%d] != '%c'\n", EI_MAG1, ELFMAG1);
  if (ehdr->e_ident[EI_MAG2] != ELFMAG2)
    ERROR ("e_ident[%d] != '%c'\n", EI_MAG2, ELFMAG2);
  if (ehdr->e_ident[EI_MAG3] != ELFMAG3)
    ERROR ("e_ident[%d] != '%c'\n", EI_MAG3, ELFMAG3);

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32
      && ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    ERROR (gettext ("e_ident[%d] == %d is no known class\n"),
	   EI_CLASS, ehdr->e_ident[EI_CLASS]);

  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB
      && ehdr->e_ident[EI_DATA] != ELFDATA2MSB)
    ERROR (gettext ("e_ident[%d] == %d is no known data encoding\n"),
	   EI_DATA, ehdr->e_ident[EI_DATA]);

  if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    ERROR (gettext ("unknown ELF header version number e_ident[%d] == %d\n"),
	   EI_VERSION, ehdr->e_ident[EI_VERSION]);

  /* We currently don't handle any OS ABIs.  */
  if (ehdr->e_ident[EI_OSABI] != ELFOSABI_NONE)
    ERROR (gettext ("unsupported OS ABI e_ident[%d] == \"%s\"\n"),
	   EI_OSABI,
	   ebl_osabi_name (ebl, ehdr->e_ident[EI_OSABI], buf, sizeof (buf)));

  /* No ABI versions other than zero supported either.  */
  if (ehdr->e_ident[EI_ABIVERSION] != 0)
    ERROR (gettext ("unsupport ABI version e_ident[%d] == %d\n"),
	   EI_ABIVERSION, ehdr->e_ident[EI_ABIVERSION]);

  for (cnt = EI_PAD; cnt < EI_NIDENT; ++cnt)
    if (ehdr->e_ident[cnt] != 0)
      ERROR (gettext ("e_ident[%zu] is not zero\n"), cnt);

  /* Check the e_type field.  */
  if (ehdr->e_type != ET_REL && ehdr->e_type != ET_EXEC
      && ehdr->e_type != ET_DYN && ehdr->e_type != ET_CORE)
    ERROR (gettext ("unknown object file type %d\n"), ehdr->e_type);

  /* Check the e_machine field.  */
  for (cnt = 0; cnt < nvalid_e_machine; ++cnt)
    if (valid_e_machine[cnt] == ehdr->e_machine)
      break;
  if (cnt == nvalid_e_machine)
    ERROR (gettext ("unknown machine type %d\n"), ehdr->e_machine);

  /* Check the e_version field.  */
  if (ehdr->e_version != EV_CURRENT)
    ERROR (gettext ("unknown object file version\n"));

  /* Check the e_phoff and e_phnum fields.  */
  if (ehdr->e_phoff == 0)
    {
      if (ehdr->e_phnum != 0)
	ERROR (gettext ("invalid program header offset\n"));
      else if (ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN)
	ERROR (gettext ("\
executables and DSOs cannot have zero program header offset\n"));
    }
  else if (ehdr->e_phnum == 0)
    ERROR (gettext ("invalid number of program header entries\n"));

  /* Check the e_shoff field.  */
  shnum = ehdr->e_shnum;
  shstrndx = ehdr->e_shstrndx;
  if (ehdr->e_shoff == 0)
    {
      if (ehdr->e_shnum != 0)
	ERROR (gettext ("invalid section header table offset\n"));
      else if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN
	       && ehdr->e_type != ET_CORE)
	ERROR (gettext ("section header table must be present\n"));
    }
  else
    {
      if (ehdr->e_shnum == 0)
	{
	  /* Get the header of the zeroth section.  The sh_size field
	     might contain the section number.  */
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr;

	  shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
	  if (shdr != NULL)
	    {
	      /* The error will be reported later.  */
	      if (shdr->sh_size == 0)
		ERROR (gettext ("\
invalid number of section header table entries\n"));
	      else
		shnum = shdr->sh_size;
	    }
	}

      if (ehdr->e_shstrndx == SHN_XINDEX)
	{
	  /* Get the header of the zeroth section.  The sh_size field
	     might contain the section number.  */
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr;

	  shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
	  if (shdr != NULL)
	    {
	      /* The error will be reported later.  */
	      if (shdr->sh_link >= shnum)
		ERROR (gettext ("invalid section header index\n"));
	      else
		shstrndx = shdr->sh_link;
	    }
	}
      else if (shstrndx >= shnum)
	ERROR (gettext ("invalid section header index\n"));
    }

  /* Check the e_flags field.  */
  if (!ebl_machine_flag_check (ebl, ehdr->e_flags))
    ERROR (gettext ("invalid machine flags: %s\n"),
	   ebl_machine_flag_name (ebl, ehdr->e_flags, buf, sizeof (buf)));

  /* Check e_ehsize, e_phentsize, and e_shentsize fields.  */
  if (gelf_getclass (ebl->elf) == ELFCLASS32)
    {
      if (ehdr->e_ehsize != 0 && ehdr->e_ehsize != sizeof (Elf32_Ehdr))
	ERROR (gettext ("invalid ELF header size: %hd\n"), ehdr->e_ehsize);

      if (ehdr->e_phentsize != 0 && ehdr->e_phentsize != sizeof (Elf32_Phdr))
	ERROR (gettext ("invalid program header size: %hd\n"),
	       ehdr->e_phentsize);
      else if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > size)
	ERROR (gettext ("invalid program header position or size\n"));

      if (ehdr->e_shentsize != 0 && ehdr->e_shentsize != sizeof (Elf32_Shdr))
	ERROR (gettext ("invalid section header size: %hd\n"),
	       ehdr->e_shentsize);
      else if (ehdr->e_shoff + ehdr->e_shnum * ehdr->e_shentsize > size)
	ERROR (gettext ("invalid section header position or size\n"));
    }
  else if (gelf_getclass (ebl->elf) == ELFCLASS64)
    {
      if (ehdr->e_ehsize != 0 && ehdr->e_ehsize != sizeof (Elf64_Ehdr))
	ERROR (gettext ("invalid ELF header size: %hd\n"), ehdr->e_ehsize);

      if (ehdr->e_phentsize != 0 && ehdr->e_phentsize != sizeof (Elf64_Phdr))
	ERROR (gettext ("invalid program header size: %hd\n"),
	       ehdr->e_phentsize);
      else if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > size)
	ERROR (gettext ("invalid program header position or size\n"));

      if (ehdr->e_shentsize != 0 && ehdr->e_shentsize != sizeof (Elf64_Shdr))
	ERROR (gettext ("invalid section header size: %hd\n"),
	       ehdr->e_shentsize);
      else if (ehdr->e_shoff + ehdr->e_shnum * ehdr->e_shentsize > size)
	ERROR (gettext ("invalid section header position or size\n"));
    }
}


/* Check that there is a section group section with index < IDX which
   contains section IDX and that there is exactly one.  */
static void
check_scn_group (Ebl *ebl, int idx)
{
  if (scnref[idx] == 0)
    {
      /* No reference so far.  Search following sections, maybe the
	 order is wrong.  */
      size_t cnt;

      for (cnt = idx + 1; cnt < shnum; ++cnt)
	{
	  Elf_Scn *scn;
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr;
	  Elf_Data *data;
	  Elf32_Word *grpdata;
	  size_t inner;

	  scn = elf_getscn (ebl->elf, cnt);
	  shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr == NULL)
	    /* We cannot get the section header so we cannot check it.
	       The error to get the section header will be shown
	       somewhere else.  */
	    continue;

	  if (shdr->sh_type != SHT_GROUP)
	    continue;

	  data = elf_getdata (scn, NULL);
	  if (data == NULL || data->d_size < sizeof (Elf32_Word))
	    /* Cannot check the section.  */
	    continue;

	  grpdata = (Elf32_Word *) data->d_buf;
	  for (inner = 1; inner < data->d_size / sizeof (Elf32_Word); ++inner)
	    if (grpdata[inner] == (Elf32_Word) idx)
	      goto out;
	}

    out:
      if (cnt == shnum)
	ERROR (gettext ("\
section [%2d] '%s': section with SHF_GROUP flag set not part of a section group\n"),
	       idx, section_name (ebl, idx));
      else
	ERROR (gettext ("\
section [%2d] '%s': section group [%2zu] '%s' does not preceed group member\n"),
	       idx, section_name (ebl, idx),
	       cnt, section_name (ebl, cnt));
    }
}


static void
check_symtab (Ebl *ebl, GElf_Ehdr *ehdr, int idx)
{
  bool no_xndx_warned = false;
  int no_pt_tls = 0;

  Elf_Scn *scn = elf_getscn (ebl->elf, idx);
  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
  GElf_Shdr strshdr_mem;
  GElf_Shdr *strshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				     &strshdr_mem);
  if (shdr == NULL || strshdr == NULL)
    return;
  Elf_Data *data = elf_getdata (scn, NULL);
  if (data == NULL)
    {
      ERROR (gettext ("section [%2d] '%s': cannot get section data\n"),
	     idx, section_name (ebl, idx));
      return;
    }

  if (strshdr->sh_type != SHT_STRTAB)
    ERROR (gettext ("section [%2d] '%s': referenced as string table for section [%2d] '%s' but type is not SHT_STRTAB\n"),
	   shdr->sh_link, section_name (ebl, shdr->sh_link),
	   idx, section_name (ebl, idx));

  /* Search for an extended section index table section.  */
  size_t cnt;
  GElf_Shdr xndxshdr_mem;
  GElf_Shdr *xndxshdr = NULL;
  Elf_Data *xndxdata = NULL;
  Elf32_Word xndxscnidx = 0;
  for (cnt = 1; cnt < shnum; ++cnt)
    if (cnt != (size_t) idx)
      {
	Elf_Scn *xndxscn = elf_getscn (ebl->elf, cnt);
	xndxshdr = gelf_getshdr (xndxscn, &xndxshdr_mem);
	xndxdata = elf_getdata (xndxscn, NULL);
	xndxscnidx = elf_ndxscn (xndxscn);

	if (xndxshdr == NULL || xndxdata == NULL)
	  continue;

	if (xndxshdr->sh_type == SHT_SYMTAB_SHNDX
	    && xndxshdr->sh_link == (GElf_Word) idx)
	  break;
      }
  if (cnt == shnum)
    {
      xndxshdr = NULL;
      xndxdata = NULL;
    }

  if (shdr->sh_entsize != gelf_fsize (ebl->elf, ELF_T_SYM, 1, EV_CURRENT))
    ERROR (gettext ("\
section [%2zu] '%s': entry size is does not match ElfXX_Sym\n"),
	   cnt, section_name (ebl, cnt));

  /* Test the zeroth entry.  */
  GElf_Sym sym_mem;
  Elf32_Word xndx;
  GElf_Sym *sym = gelf_getsymshndx (data, xndxdata, 0, &sym_mem, &xndx);
  if (sym == NULL)
      ERROR (gettext ("section [%2d] '%s': cannot get symbol %d: %s\n"),
	     idx, section_name (ebl, idx), 0, elf_errmsg (-1));
  else
    {
      if (sym->st_name != 0)
	ERROR (gettext ("section [%2d] '%s': '%s' in zeroth entry not zero\n"),
	       idx, section_name (ebl, idx), "st_name");
      if (sym->st_value != 0)
	ERROR (gettext ("section [%2d] '%s': '%s' in zeroth entry not zero\n"),
	       idx, section_name (ebl, idx), "st_value");
      if (sym->st_size != 0)
	ERROR (gettext ("section [%2d] '%s': '%s' in zeroth entry not zero\n"),
	       idx, section_name (ebl, idx), "st_size");
      if (sym->st_info != 0)
	ERROR (gettext ("section [%2d] '%s': '%s' in zeroth entry not zero\n"),
	       idx, section_name (ebl, idx), "st_info");
      if (sym->st_other != 0)
	ERROR (gettext ("section [%2d] '%s': '%s' in zeroth entry not zero\n"),
	       idx, section_name (ebl, idx), "st_other");
      if (sym->st_shndx != 0)
	ERROR (gettext ("section [%2d] '%s': '%s' in zeroth entry not zero\n"),
	       idx, section_name (ebl, idx), "st_shndx");
      if (xndxdata != NULL && xndx != 0)
	ERROR (gettext ("\
section [%2d] '%s': XINDEX for zeroth entry not zero\n"),
	       xndxscnidx, section_name (ebl, xndxscnidx));
    }

  for (cnt = 1; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      sym = gelf_getsymshndx (data, xndxdata, cnt, &sym_mem, &xndx);
      if (sym == NULL)
	{
	  ERROR (gettext ("section [%2d] '%s': cannot get symbol %zu: %s\n"),
		 idx, section_name (ebl, idx), cnt, elf_errmsg (-1));
	  continue;
	}

      const char *name = NULL;
      if (sym->st_name >= strshdr->sh_size)
	ERROR (gettext ("\
section [%2d] '%s': symbol %zu: invalid name value\n"),
	       idx, section_name (ebl, idx), cnt);
      else
	{
	  name = elf_strptr (ebl->elf, shdr->sh_link, sym->st_name);
	  assert (name != NULL);
	}

      if (sym->st_shndx == SHN_XINDEX)
	{
	  if (xndxdata == NULL)
	    {
	      ERROR (gettext ("\
section [%2d] '%s': symbol %zu: too large section index but no extended section index section\n"),
		     idx, section_name (ebl, idx), cnt);
	      no_xndx_warned = true;
	    }
	  else if (xndx < SHN_LORESERVE)
	    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: XINDEX used for index which would fit in st_shndx (%" PRIu32 ")\n"),
		   xndxscnidx, section_name (ebl, xndxscnidx), cnt,
		   xndx);
	}
      else if ((sym->st_shndx >= SHN_LORESERVE
		// && sym->st_shndx <= SHN_HIRESERVE    always true
		&& sym->st_shndx != SHN_ABS
		&& sym->st_shndx != SHN_COMMON)
	       || (sym->st_shndx >= shnum
		   && (sym->st_shndx < SHN_LORESERVE
		       /* || sym->st_shndx > SHN_HIRESERVE  always false */)))
	ERROR (gettext ("\
section [%2d] '%s': symbol %zu: invalid section index\n"),
	       idx, section_name (ebl, idx), cnt);
      else
	xndx = sym->st_shndx;

      if (GELF_ST_TYPE (sym->st_info) >= STT_NUM)
	ERROR (gettext ("section [%2d] '%s': symbol %zu: unknown type\n"),
	       idx, section_name (ebl, idx), cnt);

      if (GELF_ST_BIND (sym->st_info) >= STB_NUM)
	ERROR (gettext ("\
section [%2d] '%s': symbol %zu: unknown symbol binding\n"),
	       idx, section_name (ebl, idx), cnt);

      if (xndx == SHN_COMMON)
	{
	  /* Common symbols can only appear in relocatable files.  */
	  if (ehdr->e_type != ET_REL)
	    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: COMMON only allowed in relocatable files\n"),
		   idx, section_name (ebl, idx), cnt);
	  if (cnt < shdr->sh_info)
	    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: local COMMON symbols are nonsense\n"),
		   idx, section_name (ebl, idx), cnt);
	  if (GELF_R_TYPE (sym->st_info) == STT_FUNC)
	    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: function in COMMON section is nonsense\n"),
		   idx, section_name (ebl, idx), cnt);
	}
      else if (xndx > 0 && xndx < shnum)
	{
	  GElf_Shdr destshdr_mem;
	  GElf_Shdr *destshdr;

	  destshdr = gelf_getshdr (elf_getscn (ebl->elf, xndx), &destshdr_mem);
	  if (destshdr != NULL)
	    {
	      if (GELF_ST_TYPE (sym->st_info) != STT_TLS)
		{
		  if ((sym->st_value - destshdr->sh_addr) > destshdr->sh_size)
		    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: st_value out of bounds\n"),
			   idx, section_name (ebl, idx), cnt);
		  else if ((sym->st_value - destshdr->sh_addr + sym->st_size)
			   > destshdr->sh_size)
		    ERROR (gettext ("\
section [%2d] '%s': symbol %zu does not fit completely in referenced section [%2d] '%s'\n"),
			   idx, section_name (ebl, idx), cnt,
			   (int) xndx, section_name (ebl, xndx));
		}
	      else
		{
		  if ((destshdr->sh_flags & SHF_TLS) == 0)
		    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: referenced section [%2d] '%s' does not have SHF_TLS flag set\n"),
			   idx, section_name (ebl, idx), cnt,
			   (int) xndx, section_name (ebl, xndx));

		  if (ehdr->e_type == ET_REL)
		    {
		      /* For object files the symbol value must fall
                         into the section.  */
		      if (sym->st_value > destshdr->sh_size)
			ERROR (gettext ("\
section [%2d] '%s': symbol %zu: st_value out of bounds of referenced section [%2d] '%s'\n"),
			       idx, section_name (ebl, idx), cnt,
			       (int) xndx, section_name (ebl, xndx));
		      else if (sym->st_value + sym->st_size
			       > destshdr->sh_size)
			ERROR (gettext ("\
section [%2d] '%s': symbol %zu does not fit completely in referenced section [%2d] '%s'\n"),
			       idx, section_name (ebl, idx), cnt,
			       (int) xndx, section_name (ebl, xndx));
		    }
		  else
		    {
		      GElf_Phdr phdr_mem;
		      GElf_Phdr *phdr = NULL;
		      int pcnt;

		      for (pcnt = 0; pcnt < ehdr->e_phnum; ++pcnt)
			{
			  phdr = gelf_getphdr (ebl->elf, pcnt, &phdr_mem);
			  if (phdr != NULL && phdr->p_type == PT_TLS)
			    break;
			}

		      if (pcnt == ehdr->e_phnum)
			{
			  if (no_pt_tls++ == 0)
			    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: TLS symbol but no TLS program header entry\n"),
				   idx, section_name (ebl, idx), cnt);
			}
		      else
			{
			  if (sym->st_value
			      < destshdr->sh_offset - phdr->p_offset)
			    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: st_value short of referenced section [%2d] '%s'\n"),
				   idx, section_name (ebl, idx), cnt,
				   (int) xndx, section_name (ebl, xndx));
			  else if (sym->st_value
				   > (destshdr->sh_offset - phdr->p_offset
				      + destshdr->sh_size))
			    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: st_value out of bounds of referenced section [%2d] '%s'\n"),
				   idx, section_name (ebl, idx), cnt,
				   (int) xndx, section_name (ebl, xndx));
			  else if (sym->st_value + sym->st_size
				   > (destshdr->sh_offset - phdr->p_offset
				      + destshdr->sh_size))
			    ERROR (gettext ("\
section [%2d] '%s': symbol %zu does not fit completely in referenced section [%2d] '%s'\n"),
				   idx, section_name (ebl, idx), cnt,
				   (int) xndx, section_name (ebl, xndx));
			}
		    }
		}
	    }
	}

      if (GELF_ST_BIND (sym->st_info) == STB_LOCAL)
	{
	  if (cnt >= shdr->sh_info)
	    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: local symbol outside range described in sh_info\n"),
		   idx, section_name (ebl, idx), cnt);
	}
      else
	{
	  if (cnt < shdr->sh_info)
	    ERROR (gettext ("\
section [%2d] '%s': symbol %zu: non-local symbol outside range described in sh_info\n"),
		   idx, section_name (ebl, idx), cnt);
	}

      if (GELF_ST_TYPE (sym->st_info) == STT_SECTION
	  && GELF_ST_BIND (sym->st_info) != STB_LOCAL)
	ERROR (gettext ("\
section [%2d] '%s': symbol %zu: non-local section symbol\n"),
	       idx, section_name (ebl, idx), cnt);

      if (name != NULL)
	{
	  if (strcmp (name, "_GLOBAL_OFFSET_TABLE_") == 0)
	    {
	      /* Check that address and size match the global offset
		 table.  We have to locate the GOT by searching for a
		 section named ".got".  */
	      Elf_Scn *gscn = NULL;
	      GElf_Addr addr = 0;
	      GElf_Xword size = 0;
	      bool found = false;

	      while ((gscn = elf_nextscn (ebl->elf, gscn)) != NULL)
		{
		  GElf_Shdr gshdr_mem;
		  GElf_Shdr *gshdr = gelf_getshdr (gscn, &gshdr_mem);
		  assert (gshdr != NULL);

		  const char *sname = elf_strptr (ebl->elf, ehdr->e_shstrndx,
						  gshdr->sh_name);
		  if (sname != NULL)
		    {
		      if (strcmp (sname, ".got.plt") == 0)
			{
			  addr = gshdr->sh_addr;
			  size = gshdr->sh_size;
			  found = true;
			  break;
			}
		      if (strcmp (sname, ".got") == 0)
			{
			  addr = gshdr->sh_addr;
			  size = gshdr->sh_size;
			  found = true;
			  /* Do not stop looking.  There might be a
			     .got.plt section.  */
			}
		    }
		}

	      if (found)
		{
		  /* Found it.  */
		  if (sym->st_value != addr)
		    /* This test is more strict than the psABIs which
		       usually allow the symbol to be in the middle of
		       the .got section, allowing negative offsets.  */
		    ERROR (gettext ("\
section [%2d] '%s': _GLOBAL_OFFSET_TABLE_ symbol value %#" PRIx64 " does not match .got section address %#" PRIx64 "\n"),
			   idx, section_name (ebl, idx),
			   (uint64_t) sym->st_value, (uint64_t) addr);

		  if (!gnuld && sym->st_size != size)
		    ERROR (gettext ("\
section [%2d] '%s': _GLOBAL_OFFSET_TABLE_ symbol size %" PRIu64 " does not match .got section size %" PRIu64 "\n"),
			   idx, section_name (ebl, idx),
			   (uint64_t) sym->st_size, (uint64_t) size);
		}
	      else
		ERROR (gettext ("\
section [%2d] '%s': _GLOBAL_OFFSET_TABLE_ symbol present, but no .got section\n"),
		       idx, section_name (ebl, idx));
	    }
	  else if (strcmp (name, "_DYNAMIC") == 0)
	    {
	      /* Check that address and size match the dynamic
		 section.  We locate the dynamic section via the
		 program header entry.  */
	      int pcnt;

	      for (pcnt = 0; pcnt < ehdr->e_phnum; ++pcnt)
		{
		  GElf_Phdr phdr_mem;
		  GElf_Phdr *phdr = gelf_getphdr (ebl->elf, pcnt, &phdr_mem);

		  if (phdr != NULL && phdr->p_type == PT_DYNAMIC)
		    {
		      if (sym->st_value != phdr->p_vaddr)
			ERROR (gettext ("\
section [%2d] '%s': _DYNAMIC_ symbol value %#" PRIx64 " does not match dynamic segment address %#" PRIx64 "\n"),
			       idx, section_name (ebl, idx),
			       (uint64_t) sym->st_value,
			       (uint64_t) phdr->p_vaddr);

		      if (!gnuld && sym->st_size != phdr->p_memsz)
			ERROR (gettext ("\
section [%2d] '%s': _DYNAMIC symbol size %" PRIu64 " does not match dynamic segment size %" PRIu64 "\n"),
			       idx, section_name (ebl, idx),
			       (uint64_t) sym->st_size,
			       (uint64_t) phdr->p_memsz);

		      break;
		    }
		}
	    }
	}
    }
}


static bool
is_rel_dyn (Ebl *ebl, const GElf_Ehdr *ehdr, int idx, const GElf_Shdr *shdr,
	    bool rela)
{
  /* If this is no executable or DSO it cannot be a .rel.dyn section.  */
  if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
    return false;

  /* Check the section name.  Unfortunately necessary.  */
  if (strcmp (section_name (ebl, idx), rela ? ".rela.dyn" : ".rel.dyn"))
    return false;

  /* When a .rel.dyn section is used a DT_RELCOUNT dynamic section
     entry can be present as well.  */
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (ebl->elf, scn)) != NULL)
    {
      GElf_Shdr rcshdr_mem;
      const GElf_Shdr *rcshdr = gelf_getshdr (scn, &rcshdr_mem);
      assert (rcshdr != NULL);

      if (rcshdr->sh_type == SHT_DYNAMIC)
	{
	  /* Found the dynamic section.  Look through it.  */
	  Elf_Data *d = elf_getdata (scn, NULL);
	  size_t cnt;

	  for (cnt = 1; cnt < rcshdr->sh_size / rcshdr->sh_entsize; ++cnt)
	    {
	      GElf_Dyn dyn_mem;
	      GElf_Dyn *dyn = gelf_getdyn (d, cnt, &dyn_mem);
	      assert (dyn != NULL);

	      if (dyn->d_tag == DT_RELCOUNT)
		{
		  /* Found it.  One last check: does the number
		     specified number of relative relocations exceed
		     the total number of relocations?  */
		  if (dyn->d_un.d_val > shdr->sh_size / shdr->sh_entsize)
		    ERROR (gettext ("\
section [%2d] '%s': DT_RELCOUNT value %d too high for this section\n"),
			   idx, section_name (ebl, idx),
			   (int) dyn->d_un.d_val);
		}
	    }

	  break;
	}
    }

  return true;
}


static bool
check_reloc_shdr (Ebl *ebl, const GElf_Ehdr *ehdr, const GElf_Shdr *shdr,
		  int idx, int reltype, GElf_Shdr **destshdrp,
		  GElf_Shdr *destshdr_memp)
{
  bool reldyn = false;

  /* Check whether the link to the section we relocate is reasonable.  */
  if (shdr->sh_info >= shnum)
    ERROR (gettext ("section [%2d] '%s': invalid destination section index\n"),
	   idx, section_name (ebl, idx));
  else if (shdr->sh_info != 0)
    {
      *destshdrp = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_info),
				 destshdr_memp);
      if (*destshdrp != NULL)
	{
	  if((*destshdrp)->sh_type != SHT_PROGBITS
	     && (*destshdrp)->sh_type != SHT_NOBITS)
	    {
	      reldyn = is_rel_dyn (ebl, ehdr, idx, shdr, true);
	      if (!reldyn)
		ERROR (gettext ("\
section [%2d] '%s': invalid destination section type\n"),
		       idx, section_name (ebl, idx));
	      else
		{
		  /* There is no standard, but we require that .rel{,a}.dyn
		     sections have a sh_info value of zero.  */
		  if (shdr->sh_info != 0)
		    ERROR (gettext ("\
section [%2d] '%s': sh_info should be zero\n"),
			   idx, section_name (ebl, idx));
		}
	    }

	  if (((*destshdrp)->sh_flags & (SHF_MERGE | SHF_STRINGS)) != 0)
	    ERROR (gettext ("\
section [%2d] '%s': no relocations for merge-able sections possible\n"),
		   idx, section_name (ebl, idx));
	}
    }

  if (shdr->sh_entsize != gelf_fsize (ebl->elf, reltype, 1, EV_CURRENT))
    ERROR (gettext (reltype == ELF_T_RELA ? "\
section [%2d] '%s': section entry size does not match ElfXX_Rela\n" : "\
section [%2d] '%s': section entry size does not match ElfXX_Rel\n"),
	   idx, section_name (ebl, idx));

  return reldyn;
}


static void
check_one_reloc (Ebl *ebl, int idx, size_t cnt, const GElf_Shdr *symshdr,
		 Elf_Data *symdata, GElf_Addr r_offset, GElf_Xword r_info,
		 const GElf_Shdr *destshdr, bool reldyn)
{
  bool known_broken = gnuld;

  if (!ebl_reloc_type_check (ebl, GELF_R_TYPE (r_info)))
    ERROR (gettext ("section [%2d] '%s': relocation %zu: invalid type\n"),
	   idx, section_name (ebl, idx), cnt);
  else if (!ebl_reloc_valid_use (ebl, GELF_R_TYPE (r_info)))
    ERROR (gettext ("\
section [%2d] '%s': relocation %zu: relocation type invalid for the file type\n"),
	   idx, section_name (ebl, idx), cnt);

  if (symshdr != NULL
      && ((GELF_R_SYM (r_info) + 1)
	  * gelf_fsize (ebl->elf, ELF_T_SYM, 1, EV_CURRENT)
	  > symshdr->sh_size))
    ERROR (gettext ("\
section [%2d] '%s': relocation %zu: invalid symbol index\n"),
	   idx, section_name (ebl, idx), cnt);

  if (ebl_gotpc_reloc_check (ebl, GELF_R_TYPE (r_info)))
    {
      const char *name;
      char buf[64];
      GElf_Sym sym_mem;
      GElf_Sym *sym = gelf_getsym (symdata, GELF_R_SYM (r_info), &sym_mem);
      if (sym != NULL
	  /* Get the name for the symbol.  */
	  && (name = elf_strptr (ebl->elf, symshdr->sh_link, sym->st_name))
	  && strcmp (name, "_GLOBAL_OFFSET_TABLE_") !=0 )
	ERROR (gettext ("\
section [%2d] '%s': relocation %zu: only symbol '_GLOBAL_OFFSET_TABLE_' can be used with %s\n"),
	       idx, section_name (ebl, idx), cnt,
	       ebl_reloc_type_name (ebl, GELF_R_SYM (r_info),
				    buf, sizeof (buf)));
    }

  if (reldyn)
    {
      // XXX TODO Check .rel.dyn section addresses.
    }
  else if (!known_broken)
    {
      if (destshdr != NULL
	  && GELF_R_TYPE (r_info) != 0
	  && (r_offset - destshdr->sh_addr) >= destshdr->sh_size)
	ERROR (gettext ("\
section [%2d] '%s': relocation %zu: offset out of bounds\n"),
	       idx, section_name (ebl, idx), cnt);
    }

  if (symdata != NULL && ebl_copy_reloc_p (ebl, GELF_R_TYPE (r_info)))
    {
      /* Make sure the referenced symbol is an object or unspecified.  */
      GElf_Sym sym_mem;
      GElf_Sym *sym = gelf_getsym (symdata, GELF_R_SYM (r_info), &sym_mem);
      if (sym != NULL
	  && GELF_ST_TYPE (sym->st_info) != STT_NOTYPE
	  && GELF_ST_TYPE (sym->st_info) != STT_OBJECT)
	{
	  char buf[64];
	  ERROR (gettext ("section [%2d] '%s': relocation %zu: copy relocation against symbol of type %s\n"),
		 idx, section_name (ebl, idx), cnt,
		 ebl_symbol_type_name (ebl, GELF_ST_TYPE (sym->st_info),
				       buf, sizeof (buf)));
	}
    }
}


static void
check_rela (Ebl *ebl, GElf_Ehdr *ehdr, GElf_Shdr *shdr, int idx)
{
  Elf_Data *data = elf_getdata (elf_getscn (ebl->elf, idx), NULL);
  if (data == NULL)
    {
      ERROR (gettext ("section [%2d] '%s': cannot get section data\n"),
	     idx, section_name (ebl, idx));
      return;
    }

  /* Check the fields of the section header.  */
  GElf_Shdr destshdr_mem;
  GElf_Shdr *destshdr = NULL;
  bool reldyn = check_reloc_shdr (ebl, ehdr, shdr, idx, ELF_T_RELA, &destshdr,
				  &destshdr_mem);

  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  for (size_t cnt = 0; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      GElf_Rela rela_mem;
      GElf_Rela *rela = gelf_getrela (data, cnt, &rela_mem);
      if (rela == NULL)
	{
	  ERROR (gettext ("\
section [%2d] '%s': cannot get relocation %zu: %s\n"),
		 idx, section_name (ebl, idx), cnt, elf_errmsg (-1));
	  continue;
	}

      check_one_reloc (ebl, idx, cnt, symshdr, symdata, rela->r_offset,
		       rela->r_info, destshdr, reldyn);
    }
}


static void
check_rel (Ebl *ebl, GElf_Ehdr *ehdr, GElf_Shdr *shdr, int idx)
{
  Elf_Data *data = elf_getdata (elf_getscn (ebl->elf, idx), NULL);
  if (data == NULL)
    {
      ERROR (gettext ("section [%2d] '%s': cannot get section data\n"),
	     idx, section_name (ebl, idx));
      return;
    }

  /* Check the fields of the section header.  */
  GElf_Shdr destshdr_mem;
  GElf_Shdr *destshdr = NULL;
  bool reldyn = check_reloc_shdr (ebl, ehdr, shdr, idx, ELF_T_REL, &destshdr,
				  &destshdr_mem);

  Elf_Scn *symscn = elf_getscn (ebl->elf, shdr->sh_link);
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (symscn, &symshdr_mem);
  Elf_Data *symdata = elf_getdata (symscn, NULL);

  for (size_t cnt = 0; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      GElf_Rel rel_mem;
      GElf_Rel *rel = gelf_getrel (data, cnt, &rel_mem);
      if (rel == NULL)
	{
	  ERROR (gettext ("\
section [%2d] '%s': cannot get relocation %zu: %s\n"),
		 idx, section_name (ebl, idx), cnt, elf_errmsg (-1));
	  continue;
	}

      check_one_reloc (ebl, idx, cnt, symshdr, symdata, rel->r_offset,
		       rel->r_info, destshdr, reldyn);
    }
}


/* Number of dynamic sections.  */
static int ndynamic;


static void
check_dynamic (Ebl *ebl, int idx)
{
  Elf_Scn *scn;
  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr;
  Elf_Data *data;
  GElf_Shdr strshdr_mem;
  GElf_Shdr *strshdr;
  size_t cnt;
  static const bool dependencies[DT_NUM][DT_NUM] =
    {
      [DT_NEEDED] = { [DT_STRTAB] = true },
      [DT_PLTRELSZ] = { [DT_JMPREL] = true },
      [DT_HASH] = { [DT_SYMTAB] = true },
      [DT_STRTAB] = { [DT_STRSZ] = true },
      [DT_SYMTAB] = { [DT_STRTAB] = true, [DT_HASH] = true,
		      [DT_SYMENT] = true },
      [DT_RELA] = { [DT_RELASZ] = true, [DT_RELAENT] = true },
      [DT_RELASZ] = { [DT_RELA] = true },
      [DT_RELAENT] = { [DT_RELA] = true },
      [DT_STRSZ] = { [DT_STRTAB] = true },
      [DT_SYMENT] = { [DT_SYMTAB] = true },
      [DT_SONAME] = { [DT_STRTAB] = true },
      [DT_RPATH] = { [DT_STRTAB] = true },
      [DT_REL] = { [DT_RELSZ] = true, [DT_RELENT] = true },
      [DT_RELSZ] = { [DT_REL] = true },
      [DT_RELENT] = { [DT_REL] = true },
      [DT_JMPREL] = { [DT_PLTRELSZ] = true, [DT_PLTREL] = true },
      [DT_RUNPATH] = { [DT_STRTAB] = true },
      [DT_PLTREL] = { [DT_JMPREL] = true },
      [DT_PLTRELSZ] = { [DT_JMPREL] = true }
    };
  bool has_dt[DT_NUM];
  static const bool level2[DT_NUM] =
    {
      [DT_RPATH] = true,
      [DT_SYMBOLIC] = true,
      [DT_TEXTREL] = true,
      [DT_BIND_NOW] = true
    };
  static const bool mandatory[DT_NUM] =
    {
      [DT_NULL] = true,
      [DT_HASH] = true,
      [DT_STRTAB] = true,
      [DT_SYMTAB] = true,
      [DT_STRSZ] = true,
      [DT_SYMENT] = true
    };
  GElf_Addr reladdr = 0;
  GElf_Word relsz = 0;
  GElf_Addr pltreladdr = 0;
  GElf_Word pltrelsz = 0;

  memset (has_dt, '\0', sizeof (has_dt));

  if (++ndynamic == 2)
    ERROR (gettext ("more than one dynamic section present\n"));

  scn = elf_getscn (ebl->elf, idx);
  shdr = gelf_getshdr (scn, &shdr_mem);
  if (shdr == NULL)
    return;
  data = elf_getdata (scn, NULL);
  if (data == NULL)
    {
      ERROR (gettext ("section [%2d] '%s': cannot get section data\n"),
	     idx, section_name (ebl, idx));
      return;
    }

  strshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link), &strshdr_mem);
  if (strshdr != NULL && strshdr->sh_type != SHT_STRTAB)
    ERROR (gettext ("\
section [%2d] '%s': referenced as string table for section [%2d] '%s' but type is not SHT_STRTAB\n"),
	   shdr->sh_link, section_name (ebl, shdr->sh_link),
	   idx, section_name (ebl, idx));

  if (shdr->sh_entsize != gelf_fsize (ebl->elf, ELF_T_DYN, 1, EV_CURRENT))
    ERROR (gettext ("\
section [%2d] '%s': section entry size does not match ElfXX_Dyn\n"),
	   idx, section_name (ebl, idx));

  if (shdr->sh_info != 0)
    ERROR (gettext ("section [%2d] '%s': sh_info not zero\n"),
	   idx, section_name (ebl, idx));

  bool non_null_warned = false;
  for (cnt = 0; cnt < shdr->sh_size / shdr->sh_entsize; ++cnt)
    {
      GElf_Dyn dyn_mem;
      GElf_Dyn *dyn;

      dyn = gelf_getdyn (data, cnt, &dyn_mem);
      if (dyn == NULL)
	{
	  ERROR (gettext ("\
section [%2d] '%s': cannot get dynamic section entry %zu: %s\n"),
		 idx, section_name (ebl, idx), cnt, elf_errmsg (-1));
	  continue;
	}

      if (has_dt[DT_NULL] && dyn->d_tag != DT_NULL && ! non_null_warned)
	{
	  ERROR (gettext ("\
section [%2d] '%s': non-DT_NULL entries follow DT_NULL entry\n"),
		 idx, section_name (ebl, idx));
	  non_null_warned = true;
	}

      if (!ebl_dynamic_tag_check (ebl, dyn->d_tag))
	ERROR (gettext ("section [%2d] '%s': entry %zu: unknown tag\n"),
	       idx, section_name (ebl, idx), cnt);

      if (dyn->d_tag >= 0 && dyn->d_tag < DT_NUM)
	{
	  if (has_dt[dyn->d_tag]
	      && dyn->d_tag != DT_NEEDED
	      && dyn->d_tag != DT_NULL
	      && dyn->d_tag != DT_POSFLAG_1)
	    {
	      char buf[50];
	      ERROR (gettext ("\
section [%2d] '%s': entry %zu: more than one entry with tag %s\n"),
		     idx, section_name (ebl, idx), cnt,
		     ebl_dynamic_tag_name (ebl, dyn->d_tag,
					   buf, sizeof (buf)));
	    }

	  if (be_strict && level2[dyn->d_tag])
	    {
	      char buf[50];
	      ERROR (gettext ("\
section [%2d] '%s': entry %zu: level 2 tag %s used\n"),
		     idx, section_name (ebl, idx), cnt,
		     ebl_dynamic_tag_name (ebl, dyn->d_tag,
					   buf, sizeof (buf)));
	    }

	  has_dt[dyn->d_tag] = true;
	}

      if (dyn->d_tag == DT_PLTREL && dyn->d_un.d_val != DT_REL
	  && dyn->d_un.d_val != DT_RELA)
	ERROR (gettext ("\
section [%2d] '%s': entry %zu: DT_PLTREL value must be DT_REL or DT_RELA\n"),
	       idx, section_name (ebl, idx), cnt);

      if (dyn->d_tag == DT_REL)
	reladdr = dyn->d_un.d_ptr;
      if (dyn->d_tag == DT_RELSZ)
	relsz = dyn->d_un.d_val;
      if (dyn->d_tag == DT_JMPREL)
	pltreladdr = dyn->d_un.d_ptr;
      if (dyn->d_tag == DT_PLTRELSZ)
	pltrelsz = dyn->d_un.d_val;
    }

  for (cnt = 1; cnt < DT_NUM; ++cnt)
    if (has_dt[cnt])
      {
	int inner;

	for (inner = 0; inner < DT_NUM; ++inner)
	  if (dependencies[cnt][inner] && ! has_dt[inner])
	    {
	      char buf1[50];
	      char buf2[50];

	      ERROR (gettext ("\
section [%2d] '%s': contains %s entry but not %s\n"),
		     idx, section_name (ebl, idx),
		     ebl_dynamic_tag_name (ebl, cnt, buf1, sizeof (buf1)),
		     ebl_dynamic_tag_name (ebl, inner, buf2, sizeof (buf2)));
	    }
      }
    else
      {
	if (mandatory[cnt])
	  {
	    char buf[50];
	    ERROR (gettext ("\
section [%2d] '%s': mandatory tag %s not present\n"),
		   idx, section_name (ebl, idx),
		   ebl_dynamic_tag_name (ebl, cnt, buf, sizeof (buf)));
	  }
      }

  /* Check the rel/rela tags.  At least one group must be available.  */
  if ((has_dt[DT_RELA] || has_dt[DT_RELASZ] || has_dt[DT_RELAENT])
      && (!has_dt[DT_RELA] || !has_dt[DT_RELASZ] || !has_dt[DT_RELAENT]))
    ERROR (gettext ("\
section [%2d] '%s': not all of %s, %s, and %s are present\n"),
	   idx, section_name (ebl, idx),
	   "DT_RELA", "DT_RELASZ", "DT_RELAENT");

  if ((has_dt[DT_REL] || has_dt[DT_RELSZ] || has_dt[DT_RELENT])
      && (!has_dt[DT_REL] || !has_dt[DT_RELSZ] || !has_dt[DT_RELENT]))
    ERROR (gettext ("\
section [%2d] '%s': not all of %s, %s, and %s are present\n"),
	   idx, section_name (ebl, idx),
	   "DT_REL", "DT_RELSZ", "DT_RELENT");
}


static void
check_symtab_shndx (Ebl *ebl, int idx)
{
  Elf_Scn *scn;
  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr;
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr;
  Elf_Scn *symscn;
  size_t cnt;
  Elf_Data *data;
  Elf_Data *symdata;

  scn = elf_getscn (ebl->elf, idx);
  shdr = gelf_getshdr (scn, &shdr_mem);
  if (shdr == NULL)
    return;

  symscn = elf_getscn (ebl->elf, shdr->sh_link);
  symshdr = gelf_getshdr (symscn, &symshdr_mem);
  if (symshdr != NULL && symshdr->sh_type != SHT_SYMTAB)
    ERROR (gettext ("\
section [%2d] '%s': extended section index section not for symbol table\n"),
	   idx, section_name (ebl, idx));
  symdata = elf_getdata (symscn, NULL);
  if (symdata == NULL)
    ERROR (gettext ("cannot get data for symbol section\n"));

  if (shdr->sh_entsize != sizeof (Elf32_Word))
    ERROR (gettext ("\
section [%2d] '%s': entry size does not match Elf32_Word\n"),
	   idx, section_name (ebl, idx));

  if (symshdr != NULL
      && (shdr->sh_size / shdr->sh_entsize
	  < symshdr->sh_size / symshdr->sh_entsize))
    ERROR (gettext ("\
section [%2d] '%s': extended index table too small for symbol table\n"),
	   idx, section_name (ebl, idx));

  if (shdr->sh_info != 0)
    ERROR (gettext ("section [%2d] '%s': sh_info not zero\n"),
	   idx, section_name (ebl, idx));

  for (cnt = idx + 1; cnt < shnum; ++cnt)
    {
      GElf_Shdr rshdr_mem;
      GElf_Shdr *rshdr;

      rshdr = gelf_getshdr (elf_getscn (ebl->elf, cnt), &rshdr_mem);
      if (rshdr != NULL && rshdr->sh_type == SHT_SYMTAB_SHNDX
	  && rshdr->sh_link == shdr->sh_link)
	{
	  ERROR (gettext ("\
section [%2d] '%s': extended section index in section [%2zu] '%s' refers to same symbol table\n"),
		 idx, section_name (ebl, idx),
		 cnt, section_name (ebl, cnt));
	  break;
	}
    }

  data = elf_getdata (scn, NULL);

  if (*((Elf32_Word *) data->d_buf) != 0)
    ERROR (gettext ("symbol 0 should have zero extended section index\n"));

  for (cnt = 1; cnt < data->d_size / sizeof (Elf32_Word); ++cnt)
    {
      Elf32_Word xndx = ((Elf32_Word *) data->d_buf)[cnt];

      if (xndx != 0)
	{
	  GElf_Sym sym_data;
	  GElf_Sym *sym = gelf_getsym (symdata, cnt, &sym_data);
	  if (sym == NULL)
	    {
	      ERROR (gettext ("cannot get data for symbol %zu\n"), cnt);
	      continue;
	    }

	  if (sym->st_shndx != SHN_XINDEX)
	    ERROR (gettext ("\
extended section index is %" PRIu32 " but symbol index is not XINDEX\n"),
		   (uint32_t) xndx);
	}
    }
}


static void
check_hash (Ebl *ebl, int idx)
{
  Elf_Scn *scn;
  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr;
  Elf_Data *data;
  Elf32_Word nbucket;
  Elf32_Word nchain;
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr;

  scn = elf_getscn (ebl->elf, idx);
  shdr = gelf_getshdr (scn, &shdr_mem);
  if (shdr == NULL)
    return;
  data = elf_getdata (scn, NULL);
  if (data == NULL)
    {
      ERROR (gettext ("section [%2d] '%s': cannot get section data\n"),
	     idx, section_name (ebl, idx));
      return;
    }

  symshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link), &symshdr_mem);
  if (symshdr != NULL && symshdr->sh_type != SHT_DYNSYM)
    ERROR (gettext ("\
section [%2d] '%s': hash table not for dynamic symbol table\n"),
	   idx, section_name (ebl, idx));

  if (shdr->sh_entsize != sizeof (Elf32_Word))
    ERROR (gettext ("\
section [%2d] '%s': entry size does not match Elf32_Word\n"),
	   idx, section_name (ebl, idx));

  if ((shdr->sh_flags & SHF_ALLOC) == 0)
    ERROR (gettext ("section [%2d] '%s': not marked to be allocated\n"),
	   idx, section_name (ebl, idx));

  if (shdr->sh_size < 2 * shdr->sh_entsize)
    {
      ERROR (gettext ("\
section [%2d] '%s': hash table has not even room for nbucket and nchain\n"),
	     idx, section_name (ebl, idx));
      return;
    }

  nbucket = ((Elf32_Word *) data->d_buf)[0];
  nchain = ((Elf32_Word *) data->d_buf)[1];

  if (shdr->sh_size < (2 + nbucket + nchain) * shdr->sh_entsize)
    ERROR (gettext ("\
section [%2d] '%s': hash table section is too small (is %ld, expected %ld)\n"),
	   idx, section_name (ebl, idx), (long int) shdr->sh_size,
	   (long int) ((2 + nbucket + nchain) * shdr->sh_entsize));

  if (symshdr != NULL)
    {
      size_t symsize = symshdr->sh_size / symshdr->sh_entsize;
      size_t cnt;

      if (nchain < symshdr->sh_size / symshdr->sh_entsize)
	ERROR (gettext ("section [%2d] '%s': chain array not large enough\n"),
	       idx, section_name (ebl, idx));

      for (cnt = 2; cnt < 2 + nbucket; ++cnt)
	if (((Elf32_Word *) data->d_buf)[cnt] >= symsize)
	  ERROR (gettext ("\
section [%2d] '%s': hash bucket reference %zu out of bounds\n"),
		 idx, section_name (ebl, idx), cnt - 2);

      for (; cnt < 2 + nbucket + nchain; ++cnt)
	if (((Elf32_Word *) data->d_buf)[cnt] >= symsize)
	  ERROR (gettext ("\
section [%2d] '%s': hash chain reference %zu out of bounds\n"),
		 idx, section_name (ebl, idx), cnt - 2 - nbucket);
    }
}


static void
check_null (Ebl *ebl, GElf_Shdr *shdr, int idx)
{
#define TEST(name, extra) \
  if (extra && shdr->sh_##name != 0)					      \
    ERROR (gettext ("section [%2d] '%s': nonzero sh_%s for NULL section\n"),  \
	   idx, section_name (ebl, idx), #name)

  TEST (name, 1);
  TEST (flags, 1);
  TEST (addr, 1);
  TEST (offset, 1);
  TEST (size, idx != 0);
  TEST (link, idx != 0);
  TEST (info, 1);
  TEST (addralign, 1);
  TEST (entsize, 1);
}


static void
check_group (Ebl *ebl, GElf_Ehdr *ehdr, GElf_Shdr *shdr, int idx)
{
  if (ehdr->e_type != ET_REL)
    {
      ERROR (gettext ("\
section [%2d] '%s': section groups only allowed in relocatable object files\n"),
	     idx, section_name (ebl, idx));
      return;
    }

  /* Check that sh_link is an index of a symbol table.  */
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				     &symshdr_mem);
  if (symshdr == NULL)
    ERROR (gettext ("section [%2d] '%s': cannot get symbol table: %s\n"),
	   idx, section_name (ebl, idx), elf_errmsg (-1));
  else
    {
      if (symshdr->sh_type != SHT_SYMTAB)
	ERROR (gettext ("\
section [%2d] '%s': section reference in sh_link is no symbol table\n"),
	       idx, section_name (ebl, idx));

      if (shdr->sh_info >= symshdr->sh_size / gelf_fsize (ebl->elf, ELF_T_SYM,
							  1, EV_CURRENT))
	ERROR (gettext ("\
section [%2d] '%s': invalid symbol index in sh_info\n"),
	       idx, section_name (ebl, idx));

      if (shdr->sh_flags != 0)
	ERROR (gettext ("section [%2d] '%s': sh_flags not zero\n"),
	       idx, section_name (ebl, idx));

      if (be_strict
	  && shdr->sh_entsize != elf32_fsize (ELF_T_WORD, 1, EV_CURRENT))
	ERROR (gettext ("section [%2d] '%s': sh_flags not set correctly\n"),
	       idx, section_name (ebl, idx));
    }

  Elf_Data *data = elf_getdata (elf_getscn (ebl->elf, idx), NULL);
  if (data == NULL)
    ERROR (gettext ("section [%2d] '%s': cannot get data: %s\n"),
	   idx, section_name (ebl, idx), elf_errmsg (-1));
  else
    {
      size_t elsize = elf32_fsize (ELF_T_WORD, 1, EV_CURRENT);
      size_t cnt;
      Elf32_Word val;

      if (data->d_size % elsize != 0)
	ERROR (gettext ("\
section [%2d] '%s': section size not multiple of sizeof(Elf32_Word)\n"),
	       idx, section_name (ebl, idx));

      if (data->d_size < elsize)
	ERROR (gettext ("\
section [%2d] '%s': section group without flags word\n"),
	       idx, section_name (ebl, idx));
      else if (be_strict)
	{
	  if (data->d_size < 2 * elsize)
	    ERROR (gettext ("\
section [%2d] '%s': section group without member\n"),
		   idx, section_name (ebl, idx));
	  else if (data->d_size < 3 * elsize)
	    ERROR (gettext ("\
section [%2d] '%s': section group with only one member\n"),
		   idx, section_name (ebl, idx));
	}

#if ALLOW_UNALIGNED
      val = *((Elf32_Word *) data->d_buf);
#else
      memcpy (&val, data->d_buf, elsize);
#endif
      if ((val & ~GRP_COMDAT) != 0)
	ERROR (gettext ("section [%2d] '%s': unknown section group flags\n"),
	       idx, section_name (ebl, idx));

      for (cnt = elsize; cnt < data->d_size; cnt += elsize)
	{
#if ALLOW_UNALIGNED
	  val = *((Elf32_Word *) ((char *) data->d_buf + cnt));
#else
	  memcpy (&val, (char *) data->d_buf + cnt, elsize);
#endif

	  if (val > shnum)
	    ERROR (gettext ("\
section [%2d] '%s': section index %Zu out of range\n"),
		   idx, section_name (ebl, idx), cnt / elsize);
	  else
	    {
	      GElf_Shdr refshdr_mem;
	      GElf_Shdr *refshdr;

	      refshdr = gelf_getshdr (elf_getscn (ebl->elf, val),
				      &refshdr_mem);
	      if (refshdr == NULL)
		ERROR (gettext ("\
section [%2d] '%s': cannot get section header for element %zu: %s\n"),
		       idx, section_name (ebl, idx), cnt / elsize,
		       elf_errmsg (-1));
	      else
		{
		  if (refshdr->sh_type == SHT_GROUP)
		    ERROR (gettext ("\
section [%2d] '%s': section group contains another group [%2d] '%s'\n"),
			   idx, section_name (ebl, idx),
			   val, section_name (ebl, val));

		  if ((refshdr->sh_flags & SHF_GROUP) == 0)
		    ERROR (gettext ("\
section [%2d] '%s': element %Zu references section [%2d] '%s' without SHF_GROUP flag set\n"),
			   idx, section_name (ebl, idx), cnt / elsize,
			   val, section_name (ebl, val));
		}

	      if (++scnref[val] == 2)
		ERROR (gettext ("\
section [%2d] '%s' is contained in more than one section group\n"),
		       val, section_name (ebl, val));
	    }
	}
    }
}


static bool has_loadable_segment;
static bool has_interp_segment;

static const struct
{
  const char *name;
  size_t namelen;
  GElf_Word type;
  enum { unused, exact, atleast } attrflag;
  GElf_Word attr;
  GElf_Word attr2;
} special_sections[] =
  {
    /* See figure 4-14 in the gABI.  */
    { ".bss", 5, SHT_NOBITS, exact, SHF_ALLOC | SHF_WRITE, 0 },
    { ".comment", 8, SHT_PROGBITS, exact, 0, 0 },
    { ".data", 6, SHT_PROGBITS, exact, SHF_ALLOC | SHF_WRITE, 0 },
    { ".data1", 7, SHT_PROGBITS, exact, SHF_ALLOC | SHF_WRITE, 0 },
    { ".debug", 7, SHT_PROGBITS, exact, 0, 0 },
    { ".dynamic", 9, SHT_DYNAMIC, atleast, SHF_ALLOC, SHF_WRITE },
    { ".dynstr", 8, SHT_STRTAB, exact, SHF_ALLOC, 0 },
    { ".dynsym", 8, SHT_DYNSYM, exact, SHF_ALLOC, 0 },
    { ".fini", 6, SHT_PROGBITS, exact, SHF_ALLOC | SHF_EXECINSTR, 0 },
    { ".fini_array", 12, SHT_FINI_ARRAY, exact, SHF_ALLOC | SHF_WRITE, 0 },
    { ".got", 5, SHT_PROGBITS, unused, 0, 0 }, // XXX more info?
    { ".hash", 6, SHT_HASH, exact, SHF_ALLOC, 0 },
    { ".init", 6, SHT_PROGBITS, exact, SHF_ALLOC | SHF_EXECINSTR, 0 },
    { ".init_array", 12, SHT_INIT_ARRAY, exact, SHF_ALLOC | SHF_WRITE, 0 },
    { ".interp", 8, SHT_PROGBITS, atleast, 0, SHF_ALLOC }, // XXX more tests?
    { ".line", 6, SHT_PROGBITS, exact, 0, 0 },
    { ".note", 6, SHT_NOTE, exact, 0, 0 },
    { ".plt", 5, SHT_PROGBITS, unused, 0, 0 }, // XXX more tests
    { ".preinit_array", 15, SHT_PREINIT_ARRAY, exact, SHF_ALLOC | SHF_WRITE, 0 },
    { ".rela", 5, SHT_RELA, atleast, 0, SHF_ALLOC }, // XXX more tests
    { ".rel", 4, SHT_REL, atleast, 0, SHF_ALLOC }, // XXX more tests
    { ".rodata", 8, SHT_PROGBITS, exact, SHF_ALLOC, 0 },
    { ".rodata1", 9, SHT_PROGBITS, exact, SHF_ALLOC, 0 },
    { ".shstrtab", 10, SHT_STRTAB, exact, 0, 0 },
    { ".strtab", 8, SHT_STRTAB, atleast, 0, SHF_ALLOC }, // XXX more tests
    { ".symtab", 8, SHT_SYMTAB, atleast, 0, SHF_ALLOC }, // XXX more tests
    { ".symtab_shndx", 14, SHT_SYMTAB_SHNDX, atleast, 0, SHF_ALLOC }, // XXX more tests
    { ".tbss", 6, SHT_NOBITS, exact, SHF_ALLOC | SHF_WRITE | SHF_TLS, 0 },
    { ".tdata", 7, SHT_PROGBITS, exact, SHF_ALLOC | SHF_WRITE | SHF_TLS, 0 },
    { ".tdata1", 8, SHT_PROGBITS, exact, SHF_ALLOC | SHF_WRITE | SHF_TLS, 0 },
    { ".text", 6, SHT_PROGBITS, exact, SHF_ALLOC | SHF_EXECINSTR, 0 }
  };
#define nspecial_sections \
  (sizeof (special_sections) / sizeof (special_sections[0]))


static const char *
section_flags_string (GElf_Word flags, char *buf, size_t len)
{
  static const struct
  {
    GElf_Word flag;
    const char *name;
  } known_flags[] =
    {
#define NEWFLAG(name) { SHF_##name, #name }
      NEWFLAG (WRITE),
      NEWFLAG (ALLOC),
      NEWFLAG (EXECINSTR),
      NEWFLAG (MERGE),
      NEWFLAG (STRINGS),
      NEWFLAG (INFO_LINK),
      NEWFLAG (LINK_ORDER),
      NEWFLAG (OS_NONCONFORMING),
      NEWFLAG (GROUP),
      NEWFLAG (TLS)
    };
#undef NEWFLAG
  const size_t nknown_flags = sizeof (known_flags) / sizeof (known_flags[0]);

  char *cp = buf;
  size_t cnt;

  for (cnt = 0; cnt < nknown_flags; ++cnt)
    if (flags & known_flags[cnt].flag)
      {
	if (cp != buf && len > 1)
	  {
	    *cp++ = '|';
	    --len;
	  }

	size_t ncopy = MIN (len - 1, strlen (known_flags[cnt].name));
	cp = mempcpy (cp, known_flags[cnt].name, ncopy);
	len -= ncopy;

	flags ^= known_flags[cnt].flag;
      }

  if (flags != 0 || cp == buf)
    snprintf (cp, len - 1, "%" PRIx64, (uint64_t) flags);

  *cp = '\0';

  return buf;
}


static void
check_versym (Ebl *ebl, GElf_Shdr *shdr, int idx)
{
  /* The number of elements in the version symbol table must be the
     same as the number of symbols.  */
  GElf_Shdr symshdr_mem;
  GElf_Shdr *symshdr = gelf_getshdr (elf_getscn (ebl->elf, shdr->sh_link),
				     &symshdr_mem);
  if (symshdr == NULL)
    /* The error has already been reported.  */
    return;

  if (symshdr->sh_type != SHT_DYNSYM)
    {
      ERROR (gettext ("\
section [%2d] '%s' refers in sh_link to section [%2d] '%s' which is no dynamic symbol table\n"),
	     idx, section_name (ebl, idx),
	     shdr->sh_link, section_name (ebl, shdr->sh_link));
      return;
    }

  if (shdr->sh_size / shdr->sh_entsize
      != symshdr->sh_size / symshdr->sh_entsize)
    ERROR (gettext ("\
section [%2d] '%s' has different number of entries than symbol table [%2d] '%s'\n"),
	   idx, section_name (ebl, idx),
	   shdr->sh_link, section_name (ebl, shdr->sh_link));

  // XXX TODO A lot more tests
  // check value of the fields.  local symbols must have zero entries.
  // nonlocal symbols refer to valid version.  Check that version index
  // in bound.
}


static void
check_sections (Ebl *ebl, GElf_Ehdr *ehdr)
{
  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr;
  size_t cnt;
  bool dot_interp_section = false;

  if (ehdr->e_shoff == 0)
    /* No section header.  */
    return;

  /* Allocate array to count references in section groups.  */
  scnref = (int *) xcalloc (shnum, sizeof (int));

  /* Check the zeroth section first.  It must not have any contents
     and the section header must contain nonzero value at most in the
     sh_size and sh_link fields.  */
  shdr = gelf_getshdr (elf_getscn (ebl->elf, 0), &shdr_mem);
  if (shdr == NULL)
    ERROR (gettext ("cannot get section header of zeroth section\n"));
  else
    {
      if (shdr->sh_name != 0)
	ERROR (gettext ("zeroth section has nonzero name\n"));
      if (shdr->sh_type != 0)
	ERROR (gettext ("zeroth section has nonzero type\n"));
      if (shdr->sh_flags != 0)
	ERROR (gettext ("zeroth section has nonzero flags\n"));
      if (shdr->sh_addr != 0)
	ERROR (gettext ("zeroth section has nonzero address\n"));
      if (shdr->sh_offset != 0)
	ERROR (gettext ("zeroth section has nonzero offset\n"));
      if (shdr->sh_info != 0)
	ERROR (gettext ("zeroth section has nonzero info field\n"));
      if (shdr->sh_addralign != 0)
	ERROR (gettext ("zeroth section has nonzero align value\n"));
      if (shdr->sh_entsize != 0)
	ERROR (gettext ("zeroth section has nonzero entry size value\n"));

      if (shdr->sh_size != 0 && ehdr->e_shnum != 0)
	ERROR (gettext ("\
zeroth section has nonzero size value while ELF header has nonzero shnum value\n"));

      if (shdr->sh_link != 0 && ehdr->e_shstrndx != SHN_XINDEX)
	ERROR (gettext ("\
zeroth section has nonzero link value while ELF header does not signal overflow in shstrndx\n"));
    }

  for (cnt = 1; cnt < shnum; ++cnt)
    {
      Elf_Scn *scn;

      scn = elf_getscn (ebl->elf, cnt);
      shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	{
	  ERROR (gettext ("\
cannot get section header for section [%2zu] '%s': %s\n"),
		 cnt, section_name (ebl, cnt), elf_errmsg (-1));
	  continue;
	}

      const char *scnname = elf_strptr (ebl->elf, shstrndx, shdr->sh_name);

      if (scnname == NULL)
	ERROR (gettext ("section [%2zu]: invalid name\n"), cnt);
      else
	{
	  /* Check whether it is one of the special sections defined in
	     the gABI.  */
	  size_t s;
	  for (s = 0; s < nspecial_sections; ++s)
	    if (strncmp (scnname, special_sections[s].name,
			 special_sections[s].namelen) == 0)
	      {
		char stbuf1[100];
		char stbuf2[100];
		char stbuf3[100];

		if (shdr->sh_type != special_sections[s].type
		    && !(is_debuginfo && shdr->sh_type == SHT_NOBITS))
		  ERROR (gettext ("\
section [%2d] '%s' has wrong type: expected %s, is %s\n"),
			 (int) cnt, scnname,
			 ebl_section_type_name (ebl, special_sections[s].type,
						stbuf1, sizeof (stbuf1)),
			 ebl_section_type_name (ebl, shdr->sh_type,
						stbuf2, sizeof (stbuf2)));

		if (special_sections[s].attrflag == exact)
		  {
		    /* Except for the link order and group bit all the
		       other bits should match exactly.  */
		    if ((shdr->sh_flags & ~(SHF_LINK_ORDER | SHF_GROUP))
			!= special_sections[s].attr)
		      ERROR (gettext ("\
section [%2zu] '%s' has wrong flags: expected %s, is %s\n"),
			     cnt, scnname,
			     section_flags_string (special_sections[s].attr,
						   stbuf1, sizeof (stbuf1)),
			     section_flags_string (shdr->sh_flags
						   & ~SHF_LINK_ORDER,
						   stbuf2, sizeof (stbuf2)));
		  }
		else if (special_sections[s].attrflag == atleast)
		  {
		    if ((shdr->sh_flags & special_sections[s].attr)
			!= special_sections[s].attr
			|| ((shdr->sh_flags & ~(SHF_LINK_ORDER | SHF_GROUP
						| special_sections[s].attr
						| special_sections[s].attr2))
			    != 0))
		      ERROR (gettext ("\
section [%2zu] '%s' has wrong flags: expected %s and possibly %s, is %s\n"),
			     cnt, scnname,
			     section_flags_string (special_sections[s].attr,
						   stbuf1, sizeof (stbuf1)),
			     section_flags_string (special_sections[s].attr2,
						   stbuf2, sizeof (stbuf2)),
			     section_flags_string (shdr->sh_flags
						   & ~(SHF_LINK_ORDER
						       | SHF_GROUP),
						   stbuf3, sizeof (stbuf3)));
		  }

		if (strcmp (scnname, ".interp") == 0)
		  {
		    dot_interp_section = true;

		    if (ehdr->e_type == ET_REL)
		      ERROR (gettext ("\
section [%2zu] '%s' present in object file\n"),
			     cnt, scnname);

		    if ((shdr->sh_flags & SHF_ALLOC) != 0
			&& !has_loadable_segment)
		      ERROR (gettext ("\
section [%2zu] '%s' has SHF_ALLOC flag set but there is no loadable segment\n"),
			     cnt, scnname);
		    else if ((shdr->sh_flags & SHF_ALLOC) == 0
			     && has_loadable_segment)
		      ERROR (gettext ("\
section [%2zu] '%s' has SHF_ALLOC flag not set but there are loadable segments\n"),
			     cnt, scnname);
		  }
		else
		  {
		    if (strcmp (scnname, ".symtab_shndx") == 0
			&& ehdr->e_type != ET_REL)
		      ERROR (gettext ("\
section [%2zu] '%s' is extension section index table in non-object file\n"),
			     cnt, scnname);

		    /* These sections must have the SHF_ALLOC flag set iff
		       a loadable segment is available.

		       .relxxx
		       .strtab
		       .symtab
		       .symtab_shndx

		       Check that if there is a reference from the
		       loaded section these sections also have the
		       ALLOC flag set.  */
#if 0
		    // XXX TODO
		    if ((shdr->sh_flags & SHF_ALLOC) != 0
			&& !has_loadable_segment)
		      ERROR (gettext ("\
section [%2zu] '%s' has SHF_ALLOC flag set but there is no loadable segment\n"),
			     cnt, scnname);
		    else if ((shdr->sh_flags & SHF_ALLOC) == 0
			     && has_loadable_segment)
		      ERROR (gettext ("\
section [%2zu] '%s' has SHF_ALLOC flag not set but there are loadable segments\n"),
			     cnt, scnname);
#endif
		  }

		break;
	      }
	}

      if (shdr->sh_entsize != 0 && shdr->sh_size % shdr->sh_entsize)
	ERROR (gettext ("\
section [%2zu] '%s': size not multiple of entry size\n"),
	       cnt, section_name (ebl, cnt));

      if (elf_strptr (ebl->elf, shstrndx, shdr->sh_name) == NULL)
	ERROR (gettext ("cannot get section header\n"));

      if (shdr->sh_type >= SHT_NUM
	  && shdr->sh_type != SHT_GNU_LIBLIST
	  && shdr->sh_type != SHT_CHECKSUM
	  && shdr->sh_type != SHT_GNU_verdef
	  && shdr->sh_type != SHT_GNU_verneed
	  && shdr->sh_type != SHT_GNU_versym)
	ERROR (gettext ("unsupported section type %d\n"), (int) shdr->sh_type);

#define ALL_SH_FLAGS (SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR | SHF_MERGE \
		      | SHF_STRINGS | SHF_INFO_LINK | SHF_LINK_ORDER \
		      | SHF_OS_NONCONFORMING | SHF_GROUP | SHF_TLS)
      if (shdr->sh_flags & ~ALL_SH_FLAGS)
	ERROR (gettext ("section [%2zu] '%s' contain unknown flag(s) %d\n"),
	       cnt, section_name (ebl, cnt),
	       (int) shdr->sh_flags & ~ALL_SH_FLAGS);
      else if (shdr->sh_flags & SHF_TLS)
	{
	  // XXX Correct?
	  if (shdr->sh_addr != 0 && !gnuld)
	    ERROR (gettext ("\
section [%2zu] '%s': thread-local data sections address not zero\n"),
		   cnt, section_name (ebl, cnt));

	  // XXX TODO more tests!?
	}

      if (shdr->sh_link >= shnum)
	ERROR (gettext ("\
section [%2zu] '%s': invalid section reference in link value\n"),
	       cnt, section_name (ebl, cnt));

      if (SH_INFO_LINK_P (shdr) && shdr->sh_info >= shnum)
	ERROR (gettext ("\
section [%2zu] '%s': invalid section reference in info value\n"),
	       cnt, section_name (ebl, cnt));

      if ((shdr->sh_flags & SHF_MERGE) == 0
	  && (shdr->sh_flags & SHF_STRINGS) != 0
	  && be_strict)
	ERROR (gettext ("\
section [%2zu] '%s': strings flag set without merge flag\n"),
	       cnt, section_name (ebl, cnt));

      if ((shdr->sh_flags & SHF_MERGE) != 0 && shdr->sh_entsize == 0)
	ERROR (gettext ("\
section [%2zu] '%s': merge flag set but entry size is zero\n"),
	       cnt, section_name (ebl, cnt));

      if (shdr->sh_flags & SHF_GROUP)
	check_scn_group (ebl, cnt);

      if (ehdr->e_type != ET_REL && (shdr->sh_flags & SHF_ALLOC) != 0)
	{
	  /* Make sure the section is contained in a loaded segment
	     and that the initialization part matches NOBITS sections.  */
	  int pcnt;
	  GElf_Phdr phdr_mem;
	  GElf_Phdr *phdr;

	  for (pcnt = 0; pcnt < ehdr->e_phnum; ++pcnt)
	    if ((phdr = gelf_getphdr (ebl->elf, pcnt, &phdr_mem)) != NULL
		&& ((phdr->p_type == PT_LOAD
		     && (shdr->sh_flags & SHF_TLS) == 0)
		    || (phdr->p_type == PT_TLS
			&& (shdr->sh_flags & SHF_TLS) != 0))
		&& phdr->p_offset <= shdr->sh_offset
		&& phdr->p_offset + phdr->p_memsz > shdr->sh_offset)
	      {
		/* Found the segment.  */
		if (phdr->p_offset + phdr->p_memsz
		    < shdr->sh_offset + shdr->sh_size)
		  ERROR (gettext ("\
section [%2zu] '%s' not fully contained in segment of program header entry %d\n"),
			 cnt, section_name (ebl, cnt), pcnt);

		if (shdr->sh_type == SHT_NOBITS)
		  {
		    if (shdr->sh_offset < phdr->p_offset + phdr->p_filesz
			&& !is_debuginfo)
		      ERROR (gettext ("\
section [%2zu] '%s' has type NOBITS but is read from the file in segment of program header entry %d\n"),
			 cnt, section_name (ebl, cnt), pcnt);
		  }
		else
		  {
		    if (shdr->sh_offset >= phdr->p_offset + phdr->p_filesz)
		      ERROR (gettext ("\
section [%2zu] '%s' has not type NOBITS but is not read from the file in segment of program header entry %d\n"),
			 cnt, section_name (ebl, cnt), pcnt);
		  }

		break;
	      }

	  if (pcnt == ehdr->e_phnum)
	    ERROR (gettext ("\
section [%2zu] '%s': alloc flag set but section not in any loaded segment\n"),
		   cnt, section_name (ebl, cnt));
	}

      if (cnt == shstrndx && shdr->sh_type != SHT_STRTAB)
	ERROR (gettext ("\
section [%2zu] '%s': ELF header says this is the section header string table but type is not SHT_TYPE\n"),
	       cnt, section_name (ebl, cnt));

      switch (shdr->sh_type)
	{
	case SHT_SYMTAB:
	case SHT_DYNSYM:
	  check_symtab (ebl, ehdr, cnt);
	  break;

	case SHT_RELA:
	  check_rela (ebl, ehdr, shdr, cnt);
	  break;

	case SHT_REL:
	  check_rel (ebl, ehdr, shdr, cnt);
	  break;

	case SHT_DYNAMIC:
	  check_dynamic (ebl, cnt);
	  break;

	case SHT_SYMTAB_SHNDX:
	  check_symtab_shndx (ebl, cnt);
	  break;

	case SHT_HASH:
	  check_hash (ebl, cnt);
	  break;

	case SHT_NULL:
	  check_null (ebl, shdr, cnt);
	  break;

	case SHT_GROUP:
	  check_group (ebl, ehdr, shdr, cnt);
	  break;

	case SHT_GNU_versym:
	  check_versym (ebl, shdr, cnt);
	  break;

	default:
	  /* Nothing.  */
	  break;
	}
    }

  if (has_interp_segment && !dot_interp_section)
    ERROR (gettext ("INTERP program header entry but no .interp section\n"));

  free (scnref);
}


static void
check_note (Ebl *ebl, GElf_Ehdr *ehdr, GElf_Phdr *phdr, int cnt)
{
  if (ehdr->e_type != ET_CORE && ehdr->e_type != ET_REL
      && ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
    ERROR (gettext ("\
phdr[%d]: no note entries defined for the type of file\n"),
	   cnt);

  if (is_debuginfo)
    /* The p_offset values in a separate debug file are bogus.  */
    return;

  char *notemem = gelf_rawchunk (ebl->elf, phdr->p_offset, phdr->p_filesz);

  /* ELF64 files often use note section entries in the 32-bit format.
     The p_align field is set to 8 in case the 64-bit format is used.
     In case the p_align value is 0 or 4 the 32-bit format is
     used.  */
  GElf_Xword align = phdr->p_align == 0 || phdr->p_align == 4 ? 4 : 8;
#define ALIGNED_LEN(len) (((len) + align - 1) & ~(align - 1))

  GElf_Xword idx = 0;
  while (idx < phdr->p_filesz)
    {
      uint64_t namesz;
      uint64_t descsz;
      uint64_t type;
      uint32_t namesz32;
      uint32_t descsz32;

      if (align == 4)
	{
	  uint32_t *ptr = (uint32_t *) (notemem + idx);

	  if ((__BYTE_ORDER == __LITTLE_ENDIAN
	       && ehdr->e_ident[EI_DATA] == ELFDATA2MSB)
	      || (__BYTE_ORDER == __BIG_ENDIAN
		  && ehdr->e_ident[EI_DATA] == ELFDATA2LSB))
	    {
	      namesz32 = namesz = bswap_32 (*ptr);
	      ++ptr;
	      descsz32 = descsz = bswap_32 (*ptr);
	      ++ptr;
	      type = bswap_32 (*ptr);
	    }
	  else
	    {
	      namesz32 = namesz = *ptr++;
	      descsz32 = descsz = *ptr++;
	      type = *ptr;
	    }
	}
      else
	{
	  uint64_t *ptr = (uint64_t *) (notemem + idx);
	  uint32_t *ptr32 = (uint32_t *) (notemem + idx);

	  if ((__BYTE_ORDER == __LITTLE_ENDIAN
	       && ehdr->e_ident[EI_DATA] == ELFDATA2MSB)
	      || (__BYTE_ORDER == __BIG_ENDIAN
		  && ehdr->e_ident[EI_DATA] == ELFDATA2LSB))
	    {
	      namesz = bswap_64 (*ptr);
	      ++ptr;
	      descsz = bswap_64 (*ptr);
	      ++ptr;
	      type = bswap_64 (*ptr);

	      namesz32 = bswap_32 (*ptr32);
	      ++ptr32;
	      descsz32 = bswap_32 (*ptr32);
	    }
	  else
	    {
	      namesz = *ptr++;
	      descsz = *ptr++;
	      type = *ptr;

	      namesz32 = *ptr32++;
	      descsz32 = *ptr32;
	    }
	}

      if (idx + 3 * align > phdr->p_filesz
	  || (idx + 3 * align + ALIGNED_LEN (namesz) + ALIGNED_LEN (descsz)
	      > phdr->p_filesz))
	{
	  if (ehdr->e_ident[EI_CLASS] == ELFCLASS64
	      && idx + 3 * 4 <= phdr->p_filesz
	      && (idx + 3 * 4 + ALIGNED_LEN (namesz32) + ALIGNED_LEN (descsz32)
		  <= phdr->p_filesz))
	    ERROR (gettext ("\
phdr[%d]: note entries probably in form of a 32-bit ELF file\n"), cnt);
	  else
	    ERROR (gettext ("phdr[%d]: extra %zu bytes after last note\n"),
		   cnt, (size_t) (phdr->p_filesz - idx));
	  break;
	}

      /* Make sure it is one of the note types we know about.  */
      if (ehdr->e_type == ET_CORE)
	{
	  switch (type)
	    {
	    case NT_PRSTATUS:
	    case NT_FPREGSET:
	    case NT_PRPSINFO:
	    case NT_TASKSTRUCT:		/* NT_PRXREG on Solaris.  */
	    case NT_PLATFORM:
	    case NT_AUXV:
	    case NT_GWINDOWS:
	    case NT_ASRS:
	    case NT_PSTATUS:
	    case NT_PSINFO:
	    case NT_PRCRED:
	    case NT_UTSNAME:
	    case NT_LWPSTATUS:
	    case NT_LWPSINFO:
	    case NT_PRFPXREG:
	      /* Known type.  */
	      break;

	    default:
	      ERROR (gettext ("\
phdr[%d]: unknown core file note type %" PRIu64 " at offset %" PRIu64 "\n"),
		     cnt, type, idx);
	    }
	}
      else
	{
	  if (type != NT_VERSION)
	    ERROR (gettext ("\
phdr[%d]: unknown object file note type %" PRIu64 " at offset %" PRIu64 "\n"),
		   cnt, type, idx);
	}

      /* Move to the next entry.  */
      idx += 3 * align + ALIGNED_LEN (namesz) + ALIGNED_LEN (descsz);

    }

  gelf_freechunk (ebl->elf, notemem);
}


static void
check_program_header (Ebl *ebl, GElf_Ehdr *ehdr)
{
  if (ehdr->e_phoff == 0)
    return;

  if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN
      && ehdr->e_type != ET_CORE)
    ERROR (gettext ("\
only executables, shared objects, and core files can have program headers\n"));

  int num_pt_interp = 0;
  int num_pt_tls = 0;
  int num_pt_relro = 0;

  for (int cnt = 0; cnt < ehdr->e_phnum; ++cnt)
    {
      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr;

      phdr = gelf_getphdr (ebl->elf, cnt, &phdr_mem);
      if (phdr == NULL)
	{
	  ERROR (gettext ("cannot get program header entry %d: %s\n"),
		 cnt, elf_errmsg (-1));
	  continue;
	}

      if (phdr->p_type >= PT_NUM && phdr->p_type != PT_GNU_EH_FRAME
	  && phdr->p_type != PT_GNU_STACK && phdr->p_type != PT_GNU_RELRO)
	ERROR (gettext ("\
program header entry %d: unknown program header entry type\n"),
	       cnt);

      if (phdr->p_type == PT_LOAD)
	has_loadable_segment = true;
      else if (phdr->p_type == PT_INTERP)
	{
	  if (++num_pt_interp != 1)
	    {
	      if (num_pt_interp == 2)
		ERROR (gettext ("\
more than one INTERP entry in program header\n"));
	    }
	  has_interp_segment = true;
	}
      else if (phdr->p_type == PT_TLS)
	{
	  if (++num_pt_tls == 2)
	    ERROR (gettext ("more than one TLS entry in program header\n"));
	}
      else if (phdr->p_type == PT_NOTE)
	check_note (ebl, ehdr, phdr, cnt);
      else if (phdr->p_type == PT_DYNAMIC
	       && ehdr->e_type == ET_EXEC && ! has_interp_segment)
	ERROR (gettext ("static executable cannot have dynamic sections\n"));
      else if (phdr->p_type == PT_GNU_RELRO)
	{
	  if (++num_pt_relro == 2)
	    ERROR (gettext ("\
more than one GNU_RELRO entry in program header\n"));
	  else
	    {
	      /* Check that the region is in a writable segment.  */
	      int inner;
	      for (inner = 0; inner < ehdr->e_phnum; ++inner)
		{
		  GElf_Phdr phdr2_mem;
		  GElf_Phdr *phdr2;

		  phdr2 = gelf_getphdr (ebl->elf, inner, &phdr2_mem);
		  if (phdr2 == NULL)
		    continue;

		  if (phdr2->p_type == PT_LOAD
		      && phdr->p_vaddr >= phdr2->p_vaddr
		      && (phdr->p_vaddr + phdr->p_memsz
			  <= phdr2->p_vaddr + phdr2->p_memsz))
		    {
		      if ((phdr2->p_flags & PF_W) == 0)
			ERROR (gettext ("\
loadable segment GNU_RELRO applies to is not writable\n"));
		      if ((phdr2->p_flags & PF_X) != 0)
			ERROR (gettext ("\
loadable segment GNU_RELRO applies to is executable\n"));
		      break;
		    }
		}

	      if (inner >= ehdr->e_phnum)
		ERROR (gettext ("\
GNU_RELRO segment not contained in a loaded segment\n"));
	    }
	}

      if (phdr->p_filesz > phdr->p_memsz)
	ERROR (gettext ("\
program header entry %d: file size greater than memory size\n"),
	       cnt);

      if (phdr->p_align > 1)
	{
	  if (!powerof2 (phdr->p_align))
	    ERROR (gettext ("\
program header entry %d: alignment not a power of 2\n"), cnt);
	  else if ((phdr->p_vaddr - phdr->p_offset) % phdr->p_align != 0)
	    ERROR (gettext ("\
program header entry %d: file offset and virtual address not module of alignment\n"), cnt);
	}
    }
}


/* Process one file.  */
static void
process_elf_file (Elf *elf, const char *prefix, const char *suffix,
		  const char *fname, size_t size, bool only_one)
{
  /* Reset variables.  */
  ndynamic = 0;

  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
  Ebl *ebl;

  /* Print the file name.  */
  if (!only_one)
    {
      if (prefix != NULL)
	printf ("\n%s(%s)%s:\n", prefix, fname, suffix);
      else
	printf ("\n%s:\n", fname);
    }

  if (ehdr == NULL)
    {
      ERROR (gettext ("cannot read ELF header: %s\n"), elf_errmsg (-1));
      return;
    }

  ebl = ebl_openbackend (elf);
  /* If there is no appropriate backend library we cannot test
     architecture and OS specific features.  Any encountered extension
     is an error.  */

  /* Go straight by the gABI, check all the parts in turn.  */
  check_elf_header (ebl, ehdr, size);

  /* Check the program header.  */
  check_program_header (ebl, ehdr);

  /* Next the section headers.  It is OK if there are no section
     headers at all.  */
  check_sections (ebl, ehdr);

  /* Free the resources.  */
  ebl_closebackend (ebl);
}
