/* Combine stripped files with separate symbols and debug information.
   Copyright (C) 2007 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Roland McGrath <roland@redhat.com>, 2007.

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

/* TODO:

  * SHX_XINDEX

  * prelink vs .debug_* linked addresses

  * merge many inputs? -> ET_EXEC with union phdrs + new phdrs for ET_REL mods
  ** with applied relocs to ET_REL mods, use data modified by dwfl
  *** still must apply relocs to SHF_ALLOC
  ** useless unless merge all symtabs and dwarf sections

 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <argp.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <libintl.h>
#include <locale.h>
#include <mcheck.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gelf.h>
#include <libebl.h>
#include <libdwfl.h>
#include "system.h"

#ifndef _
# define _(str) gettext (str)
#endif

/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *)
  = print_version;

/* Bug report address.  */
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  /* Group 2 will follow group 1 from dwfl_standard_argp.  */
  { "match-file-names", 'f', NULL, 0,
    N_("Match MODULE against file names, not module names"), 2 },
  { "ignore-missing", 'i', NULL, 0, N_("Silently skip unfindable files"), 0 },

  { NULL, 0, NULL, 0, N_("Output options:"), 0 },
  { "output", 'o', "FILE", 0, N_("Place output into FILE"), 0 },
  { "output-directory", 'd', "DIRECTORY",
    0, N_("Create multiple output files under DIRECTORY"), 0 },
  { "module-names", 'm', NULL, 0, N_("Use module rather than file names"), 0 },
  { "all", 'a', NULL, 0,
    N_("Create output for modules that have no separate debug information"),
    0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

struct arg_info
{
  const char *output_file;
  const char *output_dir;
  Dwfl *dwfl;
  char **args;
  bool all;
  bool ignore;
  bool modnames;
  bool match_files;
};

/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arg_info *info = state->input;

  switch (key)
    {
    case ARGP_KEY_INIT:
      state->child_inputs[0] = &info->dwfl;
      break;

    case 'o':
      if (info->output_file != NULL)
	{
	  argp_error (state, _("-o option specified twice"));
	  return EINVAL;
	}
      info->output_file = arg;
      break;

    case 'd':
      if (info->output_dir != NULL)
	{
	  argp_error (state, _("-d option specified twice"));
	  return EINVAL;
	}
      info->output_dir = arg;
      break;

    case 'm':
      info->modnames = true;
      break;
    case 'f':
      info->match_files = true;
      break;
    case 'a':
      info->all = true;
      break;
    case 'i':
      info->ignore = true;
      break;

    case ARGP_KEY_ARGS:
    case ARGP_KEY_NO_ARGS:
      /* We "consume" all the arguments here.  */
      info->args = &state->argv[state->next];

      if (info->output_file != NULL && info->output_dir != NULL)
	{
	  argp_error (state, _("only one of -o or -d allowed"));
	  return EINVAL;
	}

      if (info->output_dir != NULL)
	{
	  struct stat64 st;
	  error_t fail = 0;
	  if (stat64 (info->output_dir, &st) < 0)
	    fail = errno;
	  else if (!S_ISDIR (st.st_mode))
	    fail = ENOTDIR;
	  if (fail)
	    {
	      argp_failure (state, EXIT_FAILURE, fail,
			    _("output directory '%s'"), info->output_dir);
	      return fail;
	    }
	}

      if (info->dwfl == NULL)
	{
	  if (state->next + 2 != state->argc)
	    {
	      argp_error (state, _("exactly two file arguments are required"));
	      return EINVAL;
	    }

	  if (info->ignore || info->all || info->modnames)
	    {
	      argp_error (state, _("\
-m, -a, and -i options not allowed with explicit files"));
	      return EINVAL;
	    }

	  /* Bail out immediately to prevent dwfl_standard_argp's parser
	     from defaulting to "-e a.out".  */
	  return ENOSYS;
	}
      else if (info->output_file == NULL && info->output_dir == NULL)
	{
	  argp_error (state,
		      _("-o or -d is required when using implicit files"));
	  return EINVAL;
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
  fprintf (stream, "unstrip (%s) %s\n", PACKAGE_NAME, VERSION);
  fprintf (stream, _("\
Copyright (C) %s Red Hat, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2007");
  fprintf (stream, gettext ("Written by %s.\n"), "Roland McGrath");
}

#define ELF_CHECK(call, msg)						      \
  do									      \
    {									      \
      if (!(call)) 							      \
	error (EXIT_FAILURE, 0, msg, elf_errmsg (-1));			      \
    } while (0)

/* Copy INELF to newly-created OUTELF, exit via error for any problems.  */
static void
copy_elf (Elf *outelf, Elf *inelf)
{
  ELF_CHECK (gelf_newehdr (outelf, gelf_getclass (inelf)),
	     _("cannot create ELF header: %s"));

  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (inelf, &ehdr_mem);
  ELF_CHECK (gelf_update_ehdr (outelf, ehdr),
	     _("cannot copy ELF header: %s"));

  if (ehdr->e_phnum > 0)
    {
      ELF_CHECK (gelf_newphdr (outelf, ehdr->e_phnum),
		 _("cannot create program headers: %s"));

      GElf_Phdr phdr_mem;
      for (uint_fast16_t i = 0; i < ehdr->e_phnum; ++i)
	ELF_CHECK (gelf_update_phdr (outelf, i,
				     gelf_getphdr (inelf, i, &phdr_mem)),
		   _("cannot copy program header: %s"));
    }

  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (inelf, scn)) != NULL)
    {
      Elf_Scn *newscn = elf_newscn (outelf);

      GElf_Shdr shdr_mem;
      ELF_CHECK (gelf_update_shdr (newscn, gelf_getshdr (scn, &shdr_mem)),
		 _("cannot copy section header: %s"));

      Elf_Data *data = elf_getdata (scn, NULL);
      ELF_CHECK (data != NULL, _("cannot get section data: %s"));
      Elf_Data *newdata = elf_newdata (newscn);
      ELF_CHECK (newdata != NULL, _("cannot copy section data: %s"));
      *newdata = *data;
      elf_flagdata (newdata, ELF_C_SET, ELF_F_DIRTY);
    }
}


/* The binutils linker leaves gratuitous section symbols in .symtab
   that strip has to remove.  Older linkers likewise include a
   symbol for every section, even unallocated ones, in .dynsym.
   Because of this, the related sections can shrink in the stripped
   file from their original size.  Older versions of strip do not
   adjust the sh_size field in the debuginfo file's SHT_NOBITS
   version of the section header, so it can appear larger.  */
static bool
section_can_shrink (const GElf_Shdr *shdr)
{
  switch (shdr->sh_type)
    {
    case SHT_SYMTAB:
    case SHT_DYNSYM:
    case SHT_HASH:
    case SHT_GNU_versym:
      return true;
    }
  return false;
}

/* See if this symbol table has a leading section symbol for every single
   section, in order.  The binutils linker produces this.  */
static size_t
symtab_count_leading_section_symbols (Elf_Scn *scn, size_t shnum)
{
  Elf_Data *data = elf_getdata (scn, NULL);
  Elf_Data *shndxdata = NULL;	/* XXX */

  for (size_t i = 1; i < shnum; ++i)
    {
      GElf_Sym sym_mem;
      GElf_Word shndx = SHN_UNDEF;
      GElf_Sym *sym = gelf_getsymshndx (data, shndxdata, i, &sym_mem, &shndx);
      ELF_CHECK (sym != NULL, _("cannot get symbol table entry: %s"));
      if (sym->st_shndx != SHN_XINDEX)
	shndx = sym->st_shndx;

      if (shndx != i || GELF_ST_TYPE (sym->st_info) != STT_SECTION)
	return i;
    }

  return shnum;
}

/* We expanded the output section, so update its header.  */
static void
update_sh_size (Elf_Scn *outscn, const Elf_Data *data)
{
  GElf_Shdr shdr_mem;
  GElf_Shdr *newshdr = gelf_getshdr (outscn, &shdr_mem);
  ELF_CHECK (newshdr != NULL, _("cannot get section header: %s"));

  newshdr->sh_size = data->d_size;

  ELF_CHECK (gelf_update_shdr (outscn, newshdr),
	     _("cannot update section header: %s"));
}

/* Update relocation sections using the symbol table.  */
static void
adjust_relocs (Elf_Scn *outscn, Elf_Scn *inscn, const GElf_Shdr *shdr,
	       size_t map[], const GElf_Shdr *symshdr)
{
  Elf_Data *data = elf_getdata (outscn, NULL);

  inline void adjust_reloc (GElf_Xword *info)
    {
      size_t ndx = GELF_R_SYM (*info);
      if (ndx != STN_UNDEF)
	*info = GELF_R_INFO (map[ndx - 1], GELF_R_TYPE (*info));
    }

  switch (shdr->sh_type)
    {
    case SHT_REL:
      for (size_t i = 0; i < shdr->sh_size / shdr->sh_entsize; ++i)
	{
	  GElf_Rel rel_mem;
	  GElf_Rel *rel = gelf_getrel (data, i, &rel_mem);
	  adjust_reloc (&rel->r_info);
	  ELF_CHECK (gelf_update_rel (data, i, rel),
		     _("cannot update relocation: %s"));
	}
      break;

    case SHT_RELA:
      for (size_t i = 0; i < shdr->sh_size / shdr->sh_entsize; ++i)
	{
	  GElf_Rela rela_mem;
	  GElf_Rela *rela = gelf_getrela (data, i, &rela_mem);
	  adjust_reloc (&rela->r_info);
	  ELF_CHECK (gelf_update_rela (data, i, rela),
		     _("cannot update relocation: %s"));
	}
      break;

    case SHT_GROUP:
      {
	GElf_Shdr shdr_mem;
	GElf_Shdr *newshdr = gelf_getshdr (outscn, &shdr_mem);
	ELF_CHECK (newshdr != NULL, _("cannot get section header: %s"));
	if (newshdr->sh_info != STN_UNDEF)
	  {
	    newshdr->sh_info = map[newshdr->sh_info - 1];
	    ELF_CHECK (gelf_update_shdr (outscn, newshdr),
		       _("cannot update section header: %s"));
	  }
	break;
      }

    case SHT_HASH:
      /* We must expand the table and rejigger its contents.  */
      {
	const size_t nsym = symshdr->sh_size / symshdr->sh_entsize;
	const size_t onent = shdr->sh_size / shdr->sh_entsize;
	assert (data->d_size == shdr->sh_size);

#define CONVERT_HASH(Hash_Word)						      \
	{								      \
	  const Hash_Word *const old_hash = data->d_buf;		      \
	  const size_t nbucket = old_hash[0];				      \
	  const size_t nchain = old_hash[1];				      \
	  const Hash_Word *const old_bucket = &old_hash[2];		      \
	  const Hash_Word *const old_chain = &old_bucket[nbucket];	      \
	  assert (onent == 2 + nbucket + nchain);			      \
									      \
	  const size_t nent = 2 + nbucket + nsym;			      \
	  Hash_Word *const new_hash = xcalloc (nent, sizeof new_hash[0]);     \
	  Hash_Word *const new_bucket = &new_hash[2];			      \
	  Hash_Word *const new_chain = &new_bucket[nbucket];		      \
									      \
	  new_hash[0] = nbucket;					      \
	  new_hash[1] = nsym;						      \
	  for (size_t i = 0; i < nbucket; ++i)				      \
	    if (old_bucket[i] != STN_UNDEF)				      \
	      new_bucket[i] = map[old_bucket[i] - 1];			      \
									      \
	  for (size_t i = 1; i < nchain; ++i)				      \
	    if (old_chain[i] != STN_UNDEF)				      \
	      new_chain[map[i - 1]] = map[old_chain[i] - 1];		      \
									      \
	  data->d_buf = new_hash;					      \
	  data->d_size = nent * sizeof new_hash[0];			      \
	}

	switch (shdr->sh_entsize)
	  {
	  case 4:
	    CONVERT_HASH (Elf32_Word);
	    break;
	  case 8:
	    CONVERT_HASH (Elf64_Xword);
	    break;
	  default:
	    abort ();
	  }

	elf_flagdata (data, ELF_C_SET, ELF_F_DIRTY);
	update_sh_size (outscn, data);

#undef	CONVERT_HASH
      }
      break;

    case SHT_GNU_versym:
      /* We must expand the table and move its elements around.  */
      {
	const size_t nent = symshdr->sh_size / symshdr->sh_entsize;
	const size_t onent = shdr->sh_size / shdr->sh_entsize;
	assert (nent >= onent);

	/* We don't bother using gelf_update_versym because there is
	   really no conversion to be done.  */
	assert (sizeof (Elf32_Versym) == sizeof (GElf_Versym));
	assert (sizeof (Elf64_Versym) == sizeof (GElf_Versym));
	GElf_Versym *versym = xcalloc (nent, sizeof versym[0]);

	for (size_t i = 1; i < onent; ++i)
	  {
	    GElf_Versym *v = gelf_getversym (data, i, &versym[map[i - 1]]);
	    ELF_CHECK (v != NULL, _("cannot get symbol version: %s"));
	  }

	data->d_buf = versym;
	data->d_size = nent * shdr->sh_entsize;
	elf_flagdata (data, ELF_C_SET, ELF_F_DIRTY);
	update_sh_size (outscn, data);
      }
      break;

    default:
      error (EXIT_FAILURE, 0,
	     _("unexpected section type in [%Zu] with sh_link to symtab"),
	     elf_ndxscn (inscn));
    }
}

/* Adjust all the relocation sections in the file.  */
static void
adjust_all_relocs (Elf *elf, Elf_Scn *symtab, const GElf_Shdr *symshdr,
		   size_t map[])
{
  size_t new_sh_link = elf_ndxscn (symtab);
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (elf, scn)) != NULL)
    if (scn != symtab)
      {
	GElf_Shdr shdr_mem;
	GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));
	if (shdr->sh_type != SHT_NOBITS && shdr->sh_link == new_sh_link)
	  adjust_relocs (scn, scn, shdr, map, symshdr);
      }
}

/* The original file probably had section symbols for all of its
   sections, even the unallocated ones.  To match it as closely as
   possible, to add in section symbols for the added sections.  */
static Elf_Data *
add_new_section_symbols (Elf_Scn *old_symscn, size_t old_shnum,
			 Elf *elf, Elf_Scn *symscn, size_t shnum)
{
  const size_t added = shnum - old_shnum;

  GElf_Shdr shdr_mem;
  GElf_Shdr *shdr = gelf_getshdr (symscn, &shdr_mem);
  ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));

  const size_t nsym = shdr->sh_size / shdr->sh_entsize;
  size_t symndx_map[nsym - 1];

  shdr->sh_info += added;
  shdr->sh_size += added * shdr->sh_entsize;

  ELF_CHECK (gelf_update_shdr (symscn, shdr),
	     _("cannot update section header: %s"));

  Elf_Data *symdata = elf_getdata (symscn, NULL);
  Elf_Data *shndxdata = NULL;	/* XXX */

  symdata->d_size = shdr->sh_size;
  symdata->d_buf = xmalloc (symdata->d_size);

  /* Copy the existing section symbols.  */
  Elf_Data *old_symdata = elf_getdata (old_symscn, NULL);
  for (size_t i = 0; i < old_shnum; ++i)
    {
      GElf_Sym sym_mem;
      GElf_Word shndx = SHN_UNDEF;
      GElf_Sym *sym = gelf_getsymshndx (old_symdata, shndxdata,
					i, &sym_mem, &shndx);
      ELF_CHECK (gelf_update_symshndx (symdata, shndxdata, i,
				       sym, shndx),
		 _("cannot update symbol table: %s"));

      if (i > 0)
	symndx_map[i - 1] = i;
    }

  /* Add in the new section symbols.  */
  for (size_t i = old_shnum; i < shnum; ++i)
    {
      GElf_Sym sym =
	{
	  .st_info = GELF_ST_INFO (STB_LOCAL, STT_SECTION),
	  .st_shndx = i < SHN_LORESERVE ? i : SHN_XINDEX
	};
      GElf_Word shndx = i < SHN_LORESERVE ? SHN_UNDEF : i;
      ELF_CHECK (gelf_update_symshndx (symdata, shndxdata, i,
				       &sym, shndx),
		 _("cannot update symbol table: %s"));
    }

  /* Now copy the rest of the existing symbols.  */
  for (size_t i = old_shnum; i < nsym; ++i)
    {
      GElf_Sym sym_mem;
      GElf_Word shndx = SHN_UNDEF;
      GElf_Sym *sym = gelf_getsymshndx (old_symdata, shndxdata,
					i, &sym_mem, &shndx);
      ELF_CHECK (gelf_update_symshndx (symdata, shndxdata,
				       i + added, sym, shndx),
		 _("cannot update symbol table: %s"));

      symndx_map[i - 1] = i + added;
    }

  /* Adjust any relocations referring to the old symbol table.  */
  adjust_all_relocs (elf, symscn, shdr, symndx_map);

  return symdata;
}

static Elf_Data *
check_symtab_section_symbols (Elf *elf, Elf_Scn *scn,
			      size_t shnum, size_t shstrndx,
			      Elf_Scn *oscn, size_t oshnum, size_t oshstrndx,
			      size_t debuglink)
{
  size_t n = symtab_count_leading_section_symbols (oscn, oshnum);

  if (n == oshnum)
    return add_new_section_symbols (oscn, n, elf, scn, shnum);

  if (n == oshstrndx || (n == debuglink && n == oshstrndx - 1))
    return add_new_section_symbols (oscn, n, elf, scn, shstrndx);

  return NULL;
}

struct section
{
  Elf_Scn *scn;
  const char *name;
  Elf_Scn *outscn;
  struct Ebl_Strent *strent;
  GElf_Shdr shdr;
};

static int
compare_alloc_sections (const struct section *s1, const struct section *s2)
{
  /* Sort by address.  */
  if (s1->shdr.sh_addr < s2->shdr.sh_addr)
    return -1;
  if (s1->shdr.sh_addr > s2->shdr.sh_addr)
    return 1;

  return 0;
}

static int
compare_unalloc_sections (const GElf_Shdr *shdr1, const GElf_Shdr *shdr2,
			  const char *name1, const char *name2)
{
  /* Sort by sh_flags as an arbitrary ordering.  */
  if (shdr1->sh_flags < shdr2->sh_flags)
    return -1;
  if (shdr1->sh_flags > shdr2->sh_flags)
    return 1;

  /* Sort by name as last resort.  */
  return strcmp (name1, name2);
}

static int
compare_sections (const void *a, const void *b)
{
  const struct section *s1 = a;
  const struct section *s2 = b;

  /* Sort all non-allocated sections last.  */
  if ((s1->shdr.sh_flags ^ s2->shdr.sh_flags) & SHF_ALLOC)
    return (s1->shdr.sh_flags & SHF_ALLOC) ? -1 : 1;

  return ((s1->shdr.sh_flags & SHF_ALLOC)
	  ? compare_alloc_sections (s1, s2)
	  : compare_unalloc_sections (&s1->shdr, &s2->shdr,
				      s1->name, s2->name));
}


struct symbol
{
  size_t *map;

  union
  {
    const char *name;
    struct Ebl_Strent *strent;
  };
  GElf_Addr value;
  GElf_Xword size;
  GElf_Word shndx;
  union
  {
    struct
    {
      uint8_t info;
      uint8_t other;
    } info;
    int16_t compare;
  };
};

/* Collect input symbols into our internal form.  */
static void
collect_symbols (Elf_Scn *symscn, Elf_Scn *strscn,
		 const size_t nent, const GElf_Addr bias,
		 const size_t scnmap[], struct symbol *table, size_t *map)
{
  Elf_Data *symdata = elf_getdata (symscn, NULL);
  Elf_Data *strdata = elf_getdata (strscn, NULL);
  Elf_Data *shndxdata = NULL;	/* XXX */

  for (size_t i = 1; i < nent; ++i)
    {
      GElf_Sym sym_mem;
      GElf_Word shndx = SHN_UNDEF;
      GElf_Sym *sym = gelf_getsymshndx (symdata, shndxdata, i,
					&sym_mem, &shndx);
      ELF_CHECK (sym != NULL, _("cannot get symbol table entry: %s"));
      if (sym->st_shndx != SHN_XINDEX)
	shndx = sym->st_shndx;

      if (scnmap != NULL && shndx != SHN_UNDEF && shndx < SHN_LORESERVE)
	shndx = scnmap[shndx - 1];

      if (sym->st_name >= strdata->d_size)
	error (EXIT_FAILURE, 0,
	       _("invalid string offset in symbol [%Zu]"), i);

      struct symbol *s = &table[i - 1];
      s->map = &map[i - 1];
      s->name = strdata->d_buf + sym->st_name;
      s->value = sym->st_value + bias;
      s->size = sym->st_size;
      s->shndx = shndx;
      s->info.info = sym->st_info;
      s->info.other = sym->st_other;
    }
}


#define CMP(value)							      \
  if (s1->value < s2->value)						      \
    return -1;								      \
  if (s1->value > s2->value)						      \
    return 1

/* Compare symbols with a consistent ordering,
   but one only meaningful for equality.  */
static int
compare_symbols (const void *a, const void *b)
{
  const struct symbol *s1 = a;
  const struct symbol *s2 = b;

  CMP (value);
  CMP (size);
  CMP (shndx);

  return (s1->compare - s2->compare) ?: strcmp (s1->name, s2->name);
}

/* Compare symbols for output order after slots have been assigned.  */
static int
compare_symbols_output (const void *a, const void *b)
{
  const struct symbol *s1 = a;
  const struct symbol *s2 = b;
  int cmp;

  /* Sort discarded symbols last.  */
  cmp = (*s1->map == 0) - (*s2->map == 0);

  if (cmp == 0)
    /* Local symbols must come first.  */
    cmp = ((GELF_ST_BIND (s2->info.info) == STB_LOCAL)
	   - (GELF_ST_BIND (s1->info.info) == STB_LOCAL));

  if (cmp == 0)
    /* binutils always puts section symbols first.  */
    cmp = ((GELF_ST_TYPE (s2->info.info) == STT_SECTION)
	   - (GELF_ST_TYPE (s1->info.info) == STT_SECTION));

  if (cmp == 0)
    {
      if (GELF_ST_TYPE (s1->info.info) == STT_SECTION)
	{
	  /* binutils always puts section symbols in section index order.  */
	  CMP (shndx);
	  else
	    assert (s1 == s2);
	}

      /* Nothing really matters, so preserve the original order.  */
      CMP (map);
      else
	assert (s1 == s2);
    }

  return cmp;
}

#undef CMP

/* Fill in any SHT_NOBITS sections in UNSTRIPPED by
   copying their contents and sh_type from STRIPPED.  */
static void
copy_elided_sections (Elf *unstripped, Elf *stripped, uint_fast16_t e_type,
		      uint_fast16_t phnum, GElf_Addr bias)
{
  size_t unstripped_shstrndx;
  ELF_CHECK (elf_getshstrndx (unstripped, &unstripped_shstrndx) == 0,
	     _("cannot get section header string table section index: %s"));

  size_t stripped_shstrndx;
  ELF_CHECK (elf_getshstrndx (stripped, &stripped_shstrndx) == 0,
	     _("cannot get section header string table section index: %s"));

  size_t unstripped_shnum;
  ELF_CHECK (elf_getshnum (unstripped, &unstripped_shnum) == 0,
	     _("cannot get section count: %s"));

  size_t stripped_shnum;
  ELF_CHECK (elf_getshnum (stripped, &stripped_shnum) == 0,
	     _("cannot get section count: %s"));

  /* Cache the stripped file's section details.  */
  struct section sections[stripped_shnum - 1];
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (stripped, scn)) != NULL)
    {
      size_t i = elf_ndxscn (scn) - 1;
      GElf_Shdr *shdr = gelf_getshdr (scn, &sections[i].shdr);
      ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));
      sections[i].name = elf_strptr (stripped, stripped_shstrndx,
				     shdr->sh_name);
      if (sections[i].name == NULL)
	error (EXIT_FAILURE, 0, _("cannot read section [%Zu] name: %s"),
	       elf_ndxscn (scn), elf_errmsg (-1));
      sections[i].scn = scn;
      sections[i].outscn = NULL;
      sections[i].strent = NULL;
    }

  const struct section *stripped_symtab = NULL;

  /* Sort the sections, allocated by address and others after.  */
  qsort (sections, stripped_shnum - 1, sizeof sections[0], compare_sections);
  size_t nalloc = stripped_shnum - 1;
  while (nalloc > 0 && !(sections[nalloc - 1].shdr.sh_flags & SHF_ALLOC))
    {
      --nalloc;
      if (sections[nalloc].shdr.sh_type == SHT_SYMTAB)
	stripped_symtab = &sections[nalloc];
    }

  /* Locate a matching allocated section in SECTIONS.  */
  inline struct section *find_alloc_section (const GElf_Shdr *shdr,
					     const char *name)
    {
      const GElf_Addr addr = shdr->sh_addr + bias;
      size_t l = 0, u = nalloc;
      while (l < u)
	{
	  size_t i = (l + u) / 2;
	  if (addr < sections[i].shdr.sh_addr)
	    u = i;
	  else if (addr > sections[i].shdr.sh_addr)
	    l = i + 1;
	  else
	    {
	      /* We've found allocated sections with this address.
		 Find one with matching size, flags, and name.  */
	      while (i > 0 && sections[i - 1].shdr.sh_addr == addr)
		--i;
	      for (; i < nalloc && sections[i].shdr.sh_addr == addr;
		   ++i)
		if (sections[i].shdr.sh_flags == shdr->sh_flags
		    && (sections[i].shdr.sh_size == shdr->sh_size
			|| (sections[i].shdr.sh_size < shdr->sh_size
			    && section_can_shrink (&sections[i].shdr)))
		    && !strcmp (sections[i].name, name))
		  return &sections[i];
	      break;
	    }
	}
      return NULL;
    }

  /* Locate a matching unallocated section in SECTIONS.  */
  inline struct section *find_unalloc_section (const GElf_Shdr *shdr,
					       const char *name)
    {
      size_t l = nalloc, u = stripped_shnum - 1;
      while (l < u)
	{
	  size_t i = (l + u) / 2;
	  struct section *sec = &sections[i];
	  int cmp = compare_unalloc_sections (shdr, &sec->shdr,
					      name, sec->name);
	  if (cmp < 0)
	    u = i;
	  else if (cmp > 0)
	    l = i + 1;
	  else
	    return sec;
	}
      return NULL;
    }

  Elf_Data *shstrtab = elf_getdata (elf_getscn (unstripped,
						unstripped_shstrndx), NULL);
  ELF_CHECK (shstrtab != NULL,
	     _("cannot read section header string table: %s"));
  inline const char *unstripped_section_name (Elf_Scn *sec,
					      const GElf_Shdr *shdr)
    {
      if (shdr->sh_name >= shstrtab->d_size)
	error (EXIT_FAILURE, 0, _("cannot read section [%Zu] name: %s"),
	       elf_ndxscn (sec), elf_errmsg (-1));
      return shstrtab->d_buf + shdr->sh_name;
    }

  /* Match each debuginfo section with its corresponding stripped section.  */
  Elf_Scn *unstripped_symtab = NULL;
  size_t unstripped_strtab_ndx = SHN_UNDEF;
  scn = NULL;
  while ((scn = elf_nextscn (unstripped, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));

      /* Anything not already SHT_NOBITS is fine as it stands.  */
      if (shdr->sh_type != SHT_NOBITS)
	{
	  if (shdr->sh_type == SHT_SYMTAB)
	    {
	      unstripped_symtab = scn;
	      unstripped_strtab_ndx = shdr->sh_link;
	    }
	  continue;
	}

      const char *name = unstripped_section_name (scn, shdr);

      /* Look for the section that matches.  */
      struct section *sec = ((shdr->sh_flags & SHF_ALLOC)
			     ? find_alloc_section (shdr, name)
			     : find_unalloc_section (shdr, name));
      if (sec == NULL)
	error (EXIT_FAILURE, 0,
	       _("cannot find matching section for [%Zu] '%s'"),
	       elf_ndxscn (scn), name);

      sec->outscn = scn;
    }

  /* Make sure each main file section has a place to go.  */
  const struct section *stripped_dynsym = NULL;
  size_t debuglink = SHN_UNDEF;
  size_t ndx_section[stripped_shnum - 1];
  struct Ebl_Strtab *strtab = NULL;
  for (struct section *sec = sections;
       sec < &sections[stripped_shnum - 1];
       ++sec)
    {
      size_t secndx = elf_ndxscn (sec->scn);

      if (sec->outscn == NULL)
	{
	  /* We didn't find any corresponding section for this.  */

	  if (secndx == stripped_shstrndx)
	    {
	      /* We only need one .shstrtab.  */
	      ndx_section[secndx - 1] = unstripped_shstrndx;
	      continue;
	    }

	  if (unstripped_symtab != NULL && sec == stripped_symtab)
	    {
	      /* We don't need a second symbol table.  */
	      ndx_section[secndx - 1] = elf_ndxscn (unstripped_symtab);
	      continue;
	    }

	  if (unstripped_symtab != NULL && stripped_symtab != NULL
	      && secndx == stripped_symtab->shdr.sh_link)
	    {
	      /* ... nor its string table.  */
	      GElf_Shdr shdr_mem;
	      GElf_Shdr *shdr = gelf_getshdr (unstripped_symtab, &shdr_mem);
	      ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));
	      ndx_section[secndx - 1] = shdr->sh_link;
	      continue;
	    }

	  if (!(sec->shdr.sh_flags & SHF_ALLOC)
	      && !strcmp (sec->name, ".gnu_debuglink"))
	    {
	      /* This was created by stripping.  We don't want it.  */
	      debuglink = secndx;
	      continue;
	    }

	  sec->outscn = elf_newscn (unstripped);
	  Elf_Data *newdata = elf_newdata (sec->outscn);
	  ELF_CHECK (newdata != NULL && gelf_update_shdr (sec->outscn,
							  &sec->shdr),
		     _("cannot add new section: %s"));

	  if (strtab == NULL)
	    strtab = ebl_strtabinit (true);
	  sec->strent = ebl_strtabadd (strtab, sec->name, 0);
	  ELF_CHECK (sec->strent != NULL,
		     _("cannot add section name to string table: %s"));
	}

      /* Cache the mapping of original section indices to output sections.  */
      ndx_section[secndx - 1] = elf_ndxscn (sec->outscn);
    }

  Elf_Data *strtab_data = NULL;
  if (strtab != NULL)
    {
      /* We added some sections, so we need a new shstrtab.  */

      struct Ebl_Strent *unstripped_strent[unstripped_shnum - 1];
      memset (unstripped_strent, 0, sizeof unstripped_strent);
      for (struct section *sec = sections;
	   sec < &sections[stripped_shnum - 1];
	   ++sec)
	if (sec->outscn != NULL)
	  {
	    if (sec->strent == NULL)
	      {
		sec->strent = ebl_strtabadd (strtab, sec->name, 0);
		ELF_CHECK (sec->strent != NULL,
			   _("cannot add section name to string table: %s"));
	      }
	    unstripped_strent[elf_ndxscn (sec->outscn) - 1] = sec->strent;
	  }

      /* Add names of sections we aren't touching.  */
      for (size_t i = 0; i < unstripped_shnum - 1; ++i)
	if (unstripped_strent[i] == NULL)
	  {
	    scn = elf_getscn (unstripped, i + 1);
	    GElf_Shdr shdr_mem;
	    GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	    const char *name = unstripped_section_name (scn, shdr);
	    unstripped_strent[i] = ebl_strtabadd (strtab, name, 0);
	    ELF_CHECK (unstripped_strent[i] != NULL,
		       _("cannot add section name to string table: %s"));
	  }
	else
	  unstripped_strent[i] = NULL;

      /* Now finalize the string table so we can get offsets.  */
      strtab_data = elf_getdata (elf_getscn (unstripped, unstripped_shstrndx),
				 NULL);
      ELF_CHECK (elf_flagdata (strtab_data, ELF_C_SET, ELF_F_DIRTY),
		 _("cannot update section header string table data: %s"));
      ebl_strtabfinalize (strtab, strtab_data);

      /* Update the sh_name fields of sections we aren't modifying later.  */
      for (size_t i = 0; i < unstripped_shnum - 1; ++i)
	if (unstripped_strent[i] != NULL)
	  {
	    scn = elf_getscn (unstripped, i + 1);
	    GElf_Shdr shdr_mem;
	    GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	    shdr->sh_name = ebl_strtaboffset (unstripped_strent[i]);
	    if (i + 1 == unstripped_shstrndx)
	      shdr->sh_size = strtab_data->d_size;
	    ELF_CHECK (gelf_update_shdr (scn, shdr),
		       _("cannot update section header: %s"));
	  }
    }

  /* Get the updated section count.  */
  ELF_CHECK (elf_getshnum (unstripped, &unstripped_shnum) == 0,
	     _("cannot get section count: %s"));

  bool placed[unstripped_shnum - 1];
  memset (placed, 0, sizeof placed);

  /* Now update the output sections and copy in their data.  */
  GElf_Off offset = 0;
  for (const struct section *sec = sections;
       sec < &sections[stripped_shnum - 1];
       ++sec)
    if (sec->outscn != NULL)
      {
	GElf_Shdr shdr_mem;
	GElf_Shdr *shdr = gelf_getshdr (sec->outscn, &shdr_mem);
	ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));

	shdr_mem.sh_addr = sec->shdr.sh_addr;
	shdr_mem.sh_type = sec->shdr.sh_type;
	shdr_mem.sh_size = sec->shdr.sh_size;
	shdr_mem.sh_info = sec->shdr.sh_info;
	shdr_mem.sh_link = sec->shdr.sh_link;
	if (sec->shdr.sh_link != SHN_UNDEF)
	  shdr_mem.sh_link = ndx_section[sec->shdr.sh_link - 1];
	if (shdr_mem.sh_flags & SHF_INFO_LINK)
	  shdr_mem.sh_info = ndx_section[sec->shdr.sh_info - 1];

	if (strtab != NULL)
	  shdr_mem.sh_name = ebl_strtaboffset (sec->strent);

	Elf_Data *indata = elf_getdata (sec->scn, NULL);
	ELF_CHECK (indata != NULL, _("cannot get section data: %s"));
	Elf_Data *outdata = elf_getdata (sec->outscn, NULL);
	ELF_CHECK (outdata != NULL, _("cannot copy section data: %s"));
	*outdata = *indata;
	elf_flagdata (outdata, ELF_C_SET, ELF_F_DIRTY);

	/* Preserve the file layout of the allocated sections.  */
	if (e_type != ET_REL && (shdr_mem.sh_flags & SHF_ALLOC))
	  {
	    shdr_mem.sh_offset = sec->shdr.sh_offset;
	    placed[elf_ndxscn (sec->outscn) - 1] = true;

	    const GElf_Off end_offset = (shdr_mem.sh_offset
					 + (shdr_mem.sh_type == SHT_NOBITS
					    ? 0 : shdr_mem.sh_size));
	    if (end_offset > offset)
	      offset = end_offset;
	  }

	ELF_CHECK (gelf_update_shdr (sec->outscn, &shdr_mem),
		   _("cannot update section header: %s"));

	if (shdr_mem.sh_type == SHT_SYMTAB || shdr_mem.sh_type == SHT_DYNSYM)
	  {
	    /* We must adjust all the section indices in the symbol table.  */

	    Elf_Data *shndxdata = NULL;	/* XXX */

	    for (size_t i = 1; i < shdr_mem.sh_size / shdr_mem.sh_entsize; ++i)
	      {
		GElf_Sym sym_mem;
		GElf_Word shndx = SHN_UNDEF;
		GElf_Sym *sym = gelf_getsymshndx (outdata, shndxdata,
						  i, &sym_mem, &shndx);
		ELF_CHECK (sym != NULL,
			   _("cannot get symbol table entry: %s"));
		if (sym->st_shndx != SHN_XINDEX)
		  shndx = sym->st_shndx;

		if (shndx != SHN_UNDEF && shndx < SHN_LORESERVE)
		  {
		    if (shndx >= stripped_shnum)
		      error (EXIT_FAILURE, 0,
			     _("symbol [%Zu] has invalid section index"), i);

		    shndx = ndx_section[shndx - 1];
		    if (shndx < SHN_LORESERVE)
		      {
			sym->st_shndx = shndx;
			shndx = SHN_UNDEF;
		      }
		    else
		      sym->st_shndx = SHN_XINDEX;

		    ELF_CHECK (gelf_update_symshndx (outdata, shndxdata,
						     i, sym, shndx),
			       _("cannot update symbol table: %s"));
		  }
	      }

	    if (shdr_mem.sh_type == SHT_SYMTAB)
	      stripped_symtab = sec;
	    if (shdr_mem.sh_type == SHT_DYNSYM)
	      stripped_dynsym = sec;
	  }
      }

  /* We may need to update the symbol table.  */
  Elf_Data *symdata = NULL;
  struct Ebl_Strtab *symstrtab = NULL;
  Elf_Data *symstrdata = NULL;
  if (unstripped_symtab != NULL && (stripped_symtab != NULL
				    || (e_type != ET_REL && bias != 0)))
    {
      /* Merge the stripped file's symbol table into the unstripped one.  */
      const size_t stripped_nsym = (stripped_symtab == NULL ? 1
				    : (stripped_symtab->shdr.sh_size
				       / stripped_symtab->shdr.sh_entsize));

      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (unstripped_symtab, &shdr_mem);
      ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));
      const size_t unstripped_nsym = shdr->sh_size / shdr->sh_entsize;

      /* First collect all the symbols from both tables.  */

      const size_t total_syms = stripped_nsym - 1 + unstripped_nsym - 1;
      struct symbol symbols[total_syms];
      size_t symndx_map[total_syms];

      if (stripped_symtab != NULL)
	collect_symbols (stripped_symtab->scn,
			 elf_getscn (stripped, stripped_symtab->shdr.sh_link),
			 stripped_nsym, 0, ndx_section,
			 symbols, symndx_map);

      Elf_Scn *unstripped_strtab = elf_getscn (unstripped, shdr->sh_link);
      collect_symbols (unstripped_symtab, unstripped_strtab,
		       unstripped_nsym, e_type == ET_REL ? 0 : bias, NULL,
		       &symbols[stripped_nsym - 1],
		       &symndx_map[stripped_nsym - 1]);

      /* Next, sort our array of all symbols.  */
      qsort (symbols, total_syms, sizeof symbols[0], compare_symbols);

      /* Now we can weed out the duplicates.  Assign remaining symbols
	 new slots, collecting a map from old indices to new.  */
      size_t nsym = *symbols[0].map = 1;
      for (size_t i = 1; i < total_syms; ++i)
	*symbols[i].map = (!compare_symbols (&symbols[i - 1], &symbols[i])
			   ? 0 /* This is a duplicate.  */
			   : ++nsym); /* Allocate the next slot.  */

      /* Now we sort again, to determine the order in the output.  */
      qsort (symbols, total_syms, sizeof symbols[0], compare_symbols_output);

      if (nsym < total_syms)
	/* The discarded symbols are now at the end of the table.  */
	assert (*symbols[nsym].map == 0);

      /* Now a final pass updates the map with the final order,
	 and builds up the new string table.  */
      symstrtab = ebl_strtabinit (true);
      for (size_t i = 0; i < nsym; ++i)
	{
	  assert (*symbols[i].map != 0);
	  *symbols[i].map = i;
	  symbols[i].strent = ebl_strtabadd (symstrtab, symbols[i].name, 0);
	}

      /* Now we are ready to write the new symbol table.  */
      symdata = elf_getdata (unstripped_symtab, NULL);
      symstrdata = elf_getdata (unstripped_strtab, NULL);
      Elf_Data *shndxdata = NULL;	/* XXX */

      ebl_strtabfinalize (symstrtab, symstrdata);
      elf_flagdata (symstrdata, ELF_C_SET, ELF_F_DIRTY);

      shdr->sh_size = symdata->d_size = (1 + nsym) * shdr->sh_entsize;
      symdata->d_buf = xmalloc (symdata->d_size);

      GElf_Sym sym;
      memset (&sym, 0, sizeof sym);
      ELF_CHECK (gelf_update_symshndx (symdata, shndxdata, 0, &sym, SHN_UNDEF),
		 _("cannot update symbol table: %s"));

      shdr->sh_info = 1;
      for (size_t i = 0; i < nsym; ++i)
	{
	  struct symbol *s = &symbols[i];

	  /* Fill in the symbol details.  */
	  sym.st_name = ebl_strtaboffset (s->strent);
	  sym.st_value = s->value; /* Already biased to output address.  */
	  sym.st_size = s->size;
	  sym.st_shndx = s->shndx; /* Already mapped to output index.  */
	  sym.st_info = s->info.info;
	  sym.st_other = s->info.other;

	  /* Keep track of the number of leading local symbols.  */
	  if (GELF_ST_BIND (sym.st_info) == STB_LOCAL)
	    {
	      assert (shdr->sh_info == 1 + i);
	      shdr->sh_info = 1 + i + 1;
	    }

	  ELF_CHECK (gelf_update_symshndx (symdata, shndxdata, 1 + i,
					   &sym, SHN_UNDEF),
		     _("cannot update symbol table: %s"));

	}
      elf_flagdata (symdata, ELF_C_SET, ELF_F_DIRTY);
      ELF_CHECK (gelf_update_shdr (unstripped_symtab, shdr),
		 _("cannot update section header: %s"));

      /* Adjust any relocations referring to the old symbol table.  */
      const size_t old_sh_link = elf_ndxscn (stripped_symtab->scn);
      for (const struct section *sec = sections;
	   sec < &sections[stripped_shnum - 1];
	   ++sec)
	if (sec->outscn != NULL && sec->shdr.sh_link == old_sh_link)
	  adjust_relocs (sec->outscn, sec->scn, &sec->shdr, symndx_map, shdr);

      /* Also adjust references to the other old symbol table.  */
      adjust_all_relocs (unstripped, unstripped_symtab, shdr,
			 &symndx_map[stripped_nsym - 1]);
    }
  else if (stripped_symtab != NULL && stripped_shnum != unstripped_shnum)
    check_symtab_section_symbols (unstripped, stripped_symtab->scn,
				  unstripped_shnum, unstripped_shstrndx,
				  stripped_symtab->outscn,
				  stripped_shnum, stripped_shstrndx,
				  debuglink);

  if (stripped_dynsym != NULL)
    (void) check_symtab_section_symbols (unstripped, stripped_dynsym->outscn,
					 unstripped_shnum,
					 unstripped_shstrndx,
					 stripped_dynsym->scn, stripped_shnum,
					 stripped_shstrndx, debuglink);

  /* We need to preserve the layout of the stripped file so the
     phdrs will match up.  This requires us to do our own layout of
     the added sections.  We do manual layout even for ET_REL just
     so we can try to match what the original probably had.  */

  elf_flagelf (unstripped, ELF_C_SET, ELF_F_LAYOUT);

  if (offset == 0)
    /* For ET_REL we are starting the layout from scratch.  */
    offset = gelf_fsize (unstripped, ELF_T_EHDR, 1, EV_CURRENT);

  bool skip_reloc = false;
  do
    {
      skip_reloc = !skip_reloc;
      for (size_t i = 0; i < unstripped_shnum - 1; ++i)
	if (!placed[i])
	  {
	    scn = elf_getscn (unstripped, 1 + i);

	    GElf_Shdr shdr_mem;
	    GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	    ELF_CHECK (shdr != NULL, _("cannot get section header: %s"));

	    if (skip_reloc
		&& (shdr->sh_type == SHT_REL || shdr->sh_type == SHT_RELA))
	      continue;

	    GElf_Off align = shdr->sh_addralign ?: 1;
	    offset = (offset + align - 1) & -align;
	    shdr->sh_offset = offset;
	    if (shdr->sh_type != SHT_NOBITS)
	      offset += shdr->sh_size;

	    ELF_CHECK (gelf_update_shdr (scn, shdr),
		       _("cannot update section header: %s"));

	    if (unstripped_shstrndx == 1 + i)
	      {
		/* Place the section headers immediately after
		   .shstrtab, and update the ELF header.  */

		GElf_Ehdr ehdr_mem;
		GElf_Ehdr *ehdr = gelf_getehdr (unstripped, &ehdr_mem);
		ELF_CHECK (ehdr != NULL, _("cannot get ELF header: %s"));

		GElf_Off sh_align = gelf_getclass (unstripped) * 4;
		offset = (offset + sh_align - 1) & -sh_align;
		ehdr->e_shnum = unstripped_shnum;
		ehdr->e_shoff = offset;
		offset += unstripped_shnum * ehdr->e_shentsize;
		ELF_CHECK (gelf_update_ehdr (unstripped, ehdr),
			   _("cannot update ELF header: %s"));
	      }

	    placed[i] = true;
	  }
    } while (skip_reloc);

  /* Copy each program header from the stripped file.  */
  for (uint_fast16_t i = 0; i < phnum; ++i)
    {
      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr = gelf_getphdr (stripped, i, &phdr_mem);
      ELF_CHECK (phdr != NULL, _("cannot get program header: %s"));

      ELF_CHECK (gelf_update_phdr (unstripped, i, phdr),
		 _("cannot update program header: %s"));
    }

  /* Finally, write out the file.  */
  ELF_CHECK (elf_update (unstripped, ELF_C_WRITE) > 0,
	     _("cannot write output file: %s"));

  if (strtab != NULL)
    {
      ebl_strtabfree (strtab);
      free (strtab_data->d_buf);
    }

  if (symdata != NULL)
    free (symdata->d_buf);
  if (symstrtab != NULL)
    {
      ebl_strtabfree (symstrtab);
      free (symstrdata->d_buf);
    }
}

/* Process one pair of files, already opened.  */
static void
handle_file (const char *output_file,
	     Elf *stripped, const GElf_Ehdr *stripped_ehdr,
	     Elf *unstripped)
{
  /* Determine the address bias between the debuginfo file and the main
     file, which may have been modified by prelinking.  */
  GElf_Addr bias = 0;
  if (unstripped != NULL)
    for (uint_fast16_t i = 0; i < stripped_ehdr->e_phnum; ++i)
      {
	GElf_Phdr phdr_mem;
	GElf_Phdr *phdr = gelf_getphdr (stripped, i, &phdr_mem);
	ELF_CHECK (phdr != NULL, _("cannot get program header: %s"));
	if (phdr->p_type == PT_LOAD)
	  {
	    GElf_Phdr unstripped_phdr_mem;
	    GElf_Phdr *unstripped_phdr = gelf_getphdr (unstripped, i,
						       &unstripped_phdr_mem);
	    ELF_CHECK (unstripped_phdr != NULL,
		       _("cannot get program header: %s"));
	    bias = phdr->p_vaddr - unstripped_phdr->p_vaddr;
	    break;
	  }
      }

  /* One day we could adjust all the DWARF data (like prelink itself does).  */
  if (bias != 0)
    error (0, 0,
	   _("DWARF output not adjusted for prelinking bias; use prelink -u"));

  if (output_file == NULL)
    /* Modify the unstripped file in place.  */
    copy_elided_sections (unstripped, stripped, stripped_ehdr->e_type,
			  stripped_ehdr->e_phnum, bias);
  else
    {
      /* Copy the unstripped file and then modify it.  */
      int outfd = open64 (output_file, O_RDWR | O_CREAT,
			  stripped_ehdr->e_type == ET_REL ? 0666 : 0777);
      if (outfd < 0)
	error (EXIT_FAILURE, errno, _("cannot open '%s'"), output_file);
      Elf *outelf = elf_begin (outfd, ELF_C_WRITE, NULL);
      ELF_CHECK (outelf != NULL, _("cannot create ELF descriptor: %s"));

      if (unstripped == NULL)
	{
	  /* Actually, we are just copying out the main file as it is.  */
	  copy_elf (outelf, stripped);
	  if (stripped_ehdr->e_type != ET_REL)
	    elf_flagelf (outelf, ELF_C_SET, ELF_F_LAYOUT);
	  ELF_CHECK (elf_update (outelf, ELF_C_WRITE) > 0,
		     _("cannot write output file: %s"));
	}
      else
	{
	  copy_elf (outelf, unstripped);
	  copy_elided_sections (outelf, stripped, stripped_ehdr->e_type,
				stripped_ehdr->e_phnum, bias);
	}

      elf_end (outelf);
      close (outfd);
    }
}

static int
open_file (const char *file, bool writable)
{
  int fd = open64 (file, writable ? O_RDWR : O_RDONLY);
  if (fd < 0)
    error (EXIT_FAILURE, errno, _("cannot open '%s'"), file);
  return fd;
}

/* Handle a pair of files we need to open by name.  */
static void
handle_explicit_files (const char *output_file,
		       const char *stripped_file, const char *unstripped_file)
{
  int stripped_fd = open_file (stripped_file, false);
  Elf *stripped = elf_begin (stripped_fd, ELF_C_READ, NULL);
  GElf_Ehdr stripped_ehdr;
  ELF_CHECK (gelf_getehdr (stripped, &stripped_ehdr),
	     _("cannot create ELF descriptor: %s"));

  int unstripped_fd = -1;
  Elf *unstripped = NULL;
  if (unstripped_file != NULL)
    {
      unstripped_fd = open_file (unstripped_file, output_file == NULL);
      unstripped = elf_begin (unstripped_fd,
			      (output_file == NULL ? ELF_C_RDWR : ELF_C_READ),
			      NULL);
      GElf_Ehdr unstripped_ehdr;
      ELF_CHECK (gelf_getehdr (unstripped, &unstripped_ehdr),
		 _("cannot create ELF descriptor: %s"));

      if (memcmp (stripped_ehdr.e_ident, unstripped_ehdr.e_ident, EI_NIDENT)
	  || stripped_ehdr.e_type != unstripped_ehdr.e_type
	  || stripped_ehdr.e_machine != unstripped_ehdr.e_machine
	  || stripped_ehdr.e_phnum != unstripped_ehdr.e_phnum)
	error (EXIT_FAILURE, 0, _("'%s' and '%s' do not seem to match"),
	       stripped_file, unstripped_file);
    }

  handle_file (output_file, stripped, &stripped_ehdr, unstripped);

  elf_end (stripped);
  close (stripped_fd);

  elf_end (unstripped);
  close (unstripped_fd);
}


/* Handle a pair of files opened implicitly by libdwfl for one module.  */
static void
handle_dwfl_module (const char *output_file, Dwfl_Module *mod,
		    bool all, bool ignore)
{
  GElf_Addr bias;
  Elf *stripped = dwfl_module_getelf (mod, &bias);
  if (stripped == NULL)
    {
      if (ignore)
	return;

      const char *file;
      const char *modname = dwfl_module_info (mod, NULL, NULL, NULL,
					      NULL, NULL, &file, NULL);
      if (file == NULL)
	error (EXIT_FAILURE, 0,
	       _("cannot find stripped file for module '%s': %s"),
	       modname, dwfl_errmsg (-1));
      else
	error (EXIT_FAILURE, 0,
	       _("cannot open stripped file '%s' for module '%s': %s"),
	       modname, file, dwfl_errmsg (-1));
    }

  Elf *debug = dwarf_getelf (dwfl_module_getdwarf (mod, &bias));
  if (debug == NULL && !all)
    {
      if (ignore)
	return;

      const char *file;
      const char *modname = dwfl_module_info (mod, NULL, NULL, NULL,
					      NULL, NULL, NULL, &file);
      if (file == NULL)
	error (EXIT_FAILURE, 0,
	       _("cannot find debug file for module '%s': %s"),
	       modname, dwfl_errmsg (-1));
      else
	error (EXIT_FAILURE, 0,
	       _("cannot open debug file '%s' for module '%s': %s"),
	       modname, file, dwfl_errmsg (-1));
    }

  if (debug == stripped)
    {
      if (all)
	debug = NULL;
      else
	{
	  const char *file;
	  const char *modname = dwfl_module_info (mod, NULL, NULL, NULL,
						  NULL, NULL, &file, NULL);
	  error (EXIT_FAILURE, 0, _("module '%s' file '%s' is not stripped"),
		 modname, file);
	}
    }

  GElf_Ehdr stripped_ehdr;
  ELF_CHECK (gelf_getehdr (stripped, &stripped_ehdr),
	     _("cannot create ELF descriptor: %s"));

  if (stripped_ehdr.e_type == ET_REL)
    {
      /* We can't use the Elf handles already open,
	 because the DWARF sections have been relocated.  */

      const char *stripped_file = NULL;
      const char *unstripped_file = NULL;
      (void) dwfl_module_info (mod, NULL, NULL, NULL, NULL, NULL,
			       &stripped_file, &unstripped_file);

      handle_explicit_files (output_file, stripped_file, unstripped_file);
    }
  else
    handle_file (output_file, stripped, &stripped_ehdr, debug);
}

/* Create directories containing PATH.  */
static void
make_directories (const char *path)
{
  const char *lastslash = strrchr (path, '/');
  if (lastslash == NULL)
    return;

  while (lastslash > path && lastslash[-1] == '/')
    --lastslash;
  if (lastslash == path)
    return;

  char *dir = strndupa (path, lastslash - path);
  while (mkdir (dir, 0777) < 0 && errno != EEXIST)
    if (errno == ENOENT)
      make_directories (dir);
    else
      error (EXIT_FAILURE, errno, _("cannot create directory '%s'"), dir);
}

/* Handle one module being written to the output directory.  */
static void
handle_output_dir_module (const char *output_dir, Dwfl_Module *mod,
			  bool all, bool ignore, bool modnames)
{
  if (! modnames)
    {
      /* Make sure we've searched for the ELF file.  */
      GElf_Addr bias;
      (void) dwfl_module_getelf (mod, &bias);
    }

  const char *file;
  const char *name = dwfl_module_info (mod, NULL, NULL, NULL,
				       NULL, NULL, &file, NULL);

  if (file == NULL && ignore)
    return;

  char *output_file;
  if (asprintf (&output_file, "%s/%s", output_dir, modnames ? name : file) < 0)
    error (EXIT_FAILURE, 0, _("memory exhausted"));

  make_directories (output_file);
  handle_dwfl_module (output_file, mod, all, ignore);
}


struct match_module_info
{
  char **patterns;
  Dwfl_Module *found;
  bool match_files;
};

static int
match_module (Dwfl_Module *mod,
	      void **userdata __attribute__ ((unused)),
	      const char *name,
	      Dwarf_Addr start __attribute__ ((unused)),
	      void *arg)
{
  struct match_module_info *info = arg;

  if (info->patterns[0] == NULL) /* Match all.  */
    {
    match:
      info->found = mod;
      return DWARF_CB_ABORT;
    }

  if (info->match_files)
    {
      /* Make sure we've searched for the ELF file.  */
      GElf_Addr bias;
      (void) dwfl_module_getelf (mod, &bias);

      const char *file;
      const char *check = dwfl_module_info (mod, NULL, NULL, NULL,
					    NULL, NULL, &file, NULL);
      assert (check == name);
      if (file == NULL)
	return DWARF_CB_OK;

      name = file;
    }

  for (char **p = info->patterns; *p != NULL; ++p)
    if (fnmatch (*p, name, 0) == 0)
      goto match;

  return DWARF_CB_OK;
}

/* Handle files opened implicitly via libdwfl.  */
static void
handle_implicit_modules (const struct arg_info *info)
{
  struct match_module_info mmi = { info->args, NULL, info->match_files };
  inline ptrdiff_t next (ptrdiff_t offset)
    {
      return dwfl_getmodules (info->dwfl, &match_module, &mmi, offset);
    }
  ptrdiff_t offset = next (0);
  if (offset == 0)
    error (EXIT_FAILURE, 0, _("no matching modules found"));

  if (info->output_dir == NULL)
    {
      if (next (offset) != 0)
	error (EXIT_FAILURE, 0, _("matched more than one module"));
      handle_dwfl_module (info->output_file, mmi.found,
			  info->all, info->ignore);
    }
  else
    do
      handle_output_dir_module (info->output_dir, mmi.found,
				info->all, info->ignore, info->modnames);
    while ((offset = next (offset)) > 0);
}

int
main (int argc, char **argv)
{
  /* Make memory leak detection possible.  */
  mtrace ();

  /* We use no threads here which can interfere with handling a stream.  */
  __fsetlocking (stdin, FSETLOCKING_BYCALLER);
  __fsetlocking (stdout, FSETLOCKING_BYCALLER);
  __fsetlocking (stderr, FSETLOCKING_BYCALLER);

  /* Set locale.  */
  setlocale (LC_ALL, "");

  /* Make sure the message catalog can be found.  */
  bindtextdomain (PACKAGE, LOCALEDIR);

  /* Initialize the message catalog.  */
  textdomain (PACKAGE);

  /* Parse and process arguments.  */
  const struct argp_child argp_children[] =
    {
      {
	.argp = dwfl_standard_argp (),
	.header = N_("Input selection options:"),
	.group = 1,
      },
      { .argp = NULL },
    };
  const struct argp argp =
    {
      .options = options,
      .parser = parse_opt,
      .children = argp_children,
      .args_doc = N_("STRIPPED-FILE DEBUG-FILE\n[MODULE...]"),
      .doc = N_("\
Combine stripped files with separate symbols and debug information.\v\
The first form puts the result in DEBUG-FILE if -o was not given.\n\
\n\
MODULE arguments give file name patterns matching modules to process.\n\
With -f these match the file name of the main (stripped) file \
(slashes are never special), otherwise they match the simple module names.  \
With no arguments, process all modules found.\n\
\n\
Multiple modules are written to files under OUTPUT-DIRECTORY, \
creating subdirectories as needed.  \
With -m these files have simple module names, otherwise they have the \
name of the main file complete with directory underneath OUTPUT-DIRECTORY.")
    };

  int remaining;
  struct arg_info info = { .args = NULL };
  error_t result = argp_parse (&argp, argc, argv, 0, &remaining, &info);
  if (result == ENOSYS)
    assert (info.dwfl == NULL);
  else if (result)
    return EXIT_FAILURE;
  assert (info.args != NULL);

  /* Tell the library which version we are expecting.  */
  elf_version (EV_CURRENT);

  if (info.dwfl == NULL)
    {
      assert (result == ENOSYS);

      if (info.output_dir != NULL)
	{
	  char *file;
	  if (asprintf (&file, "%s/%s", info.output_dir, info.args[0]) < 0)
	    error (EXIT_FAILURE, 0, _("memory exhausted"));
	  make_directories (file);
	  handle_explicit_files (file, info.args[0], info.args[1]);
	  free (file);
	}
      else
	handle_explicit_files (info.output_file, info.args[0], info.args[1]);
    }
  else
    {
      /* parse_opt checked this.  */
      assert (info.output_file != NULL || info.output_dir != NULL);

      handle_implicit_modules (&info);

      dwfl_end (info.dwfl);
    }

  return 0;
}
