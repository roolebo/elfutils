/* Recover relocatibility for addresses computed from debug information.
   Copyright (C) 2005 Red Hat, Inc.
   This file is part of Red Hat elfutils.

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

   In addition, as a special exception, Red Hat, Inc. gives You the
   additional right to link the code of Red Hat elfutils with code licensed
   under any Open Source Initiative certified open source license
   (http://www.opensource.org/licenses/index.php) which requires the
   distribution of source code with any binary distribution and to
   distribute linked combinations of the two.  Non-GPL Code permitted under
   this exception must only link to the code of Red Hat elfutils through
   those well defined interfaces identified in the file named EXCEPTION
   found in the source code files (the "Approved Interfaces").  The files
   of Non-GPL Code may instantiate templates or use macros or inline
   functions from the Approved Interfaces without causing the resulting
   work to be covered by the GNU General Public License.  Only Red Hat,
   Inc. may make changes or additions to the list of Approved Interfaces.
   Red Hat's grant of this exception is conditioned upon your not adding
   any new exceptions.  If you wish to add a new Approved Interface or
   exception, please contact Red Hat.  You must obey the GNU General Public
   License in all respects for all of the Red Hat elfutils code and other
   code used in conjunction with Red Hat elfutils except the Non-GPL Code
   covered by this exception.  If you modify this file, you may extend this
   exception to your version of the file, but you are not obligated to do
   so.  If you do not wish to provide this exception without modification,
   you must delete this exception statement from your version and license
   this file solely under the GPL without exception.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#include "libdwflP.h"

struct dwfl_relocation
{
  size_t count;
  struct
  {
    Elf_Scn *scn;
    const char *name;
    GElf_Addr start, end;
  } refs[0];
};


struct secref
{
  struct secref *next;
  Elf_Scn *scn;
  const char *name;
  GElf_Addr start, end;
};

static int
compare_secrefs (const void *a, const void *b)
{
  struct secref *const *p1 = a;
  struct secref *const *p2 = b;

  return (*p1)->start - (*p2)->start;
}

static int
cache_sections (Dwfl_Module *mod)
{
  size_t symshstrndx;
  if (elf_getshstrndx (mod->symfile->elf, &symshstrndx) < 0)
    {
      __libdwfl_seterrno (DWFL_E_LIBELF);
      return -1;
    }

  struct secref *refs = NULL;
  size_t nrefs = 0;

  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (mod->symfile->elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	return -1;

      if ((shdr->sh_flags & SHF_ALLOC) && shdr->sh_addr != 0)
	{
	  const char *name = elf_strptr (mod->symfile->elf, symshstrndx,
					 shdr->sh_name);
	  if (name == NULL)
	    return -1;

	  struct secref *newref = alloca (sizeof *newref);
	  newref->scn = scn;
	  newref->name = name;
	  newref->start = shdr->sh_addr;
	  newref->end = shdr->sh_addr + shdr->sh_size;
	  newref->next = refs;
	  refs = newref;
	  ++nrefs;
	}
    }

  mod->reloc_info = malloc (offsetof (struct dwfl_relocation, refs[nrefs]));
  if (mod->reloc_info == NULL)
    {
      __libdwfl_seterrno (DWFL_E_NOMEM);
      return -1;
    }

  struct secref *sortrefs[nrefs];
  for (size_t i = nrefs; i-- > 0; refs = refs->next)
    sortrefs[i] = refs;
  assert (refs == NULL);

  qsort (sortrefs, nrefs, sizeof sortrefs[0], &compare_secrefs);

  mod->reloc_info->count = nrefs;
  for (size_t i = 0; i < nrefs; ++i)
    {
      mod->reloc_info->refs[i].name = sortrefs[i]->name;
      mod->reloc_info->refs[i].scn = sortrefs[i]->scn;
      mod->reloc_info->refs[i].start = sortrefs[i]->start;
      mod->reloc_info->refs[i].end = sortrefs[i]->end;
    }

  return nrefs;
}


int
dwfl_module_relocations (Dwfl_Module *mod)
{
  if (mod == NULL)
    return -1;

  if (mod->reloc_info != NULL)
    return mod->reloc_info->count;

  if (mod->dw == NULL)
    {
      Dwarf_Addr bias;
      if (INTUSE(dwfl_module_getdwarf) (mod, &bias) == NULL)
	return -1;
    }

  switch (mod->e_type)
    {
    case ET_REL:
      return cache_sections (mod);

    case ET_DYN:
      return 1;

    case ET_EXEC:
      assert (mod->debug.bias == 0);
      break;
    }

  return 0;
}

const char *
dwfl_module_relocation_info (Dwfl_Module *mod, unsigned int idx,
			     Elf32_Word *shndxp)
{
  if (mod == NULL)
    return NULL;

  switch (mod->e_type)
    {
    case ET_REL:
      break;

    case ET_DYN:
      if (idx != 0)
	return NULL;
      if (shndxp)
	*shndxp = SHN_ABS;
      return "";

    default:
      return NULL;
    }

  if (unlikely (mod->reloc_info == NULL) && cache_sections (mod) < 0)
    return NULL;

  struct dwfl_relocation *sections = mod->reloc_info;

  if (idx >= sections->count)
    return NULL;

  if (shndxp)
    *shndxp = elf_ndxscn (sections->refs[idx].scn);

  return sections->refs[idx].name;
}

int
dwfl_module_relocate_address (Dwfl_Module *mod, Dwarf_Addr *addr)
{
  if (mod == NULL)
    return -1;

  if (mod->dw == NULL)
    {
      Dwarf_Addr bias;
      if (INTUSE(dwfl_module_getdwarf) (mod, &bias) == NULL)
	return -1;
    }

  if (mod->e_type != ET_REL)
    {
      *addr -= mod->debug.bias;
      return 0;
    }

  if (unlikely (mod->reloc_info == NULL) && cache_sections (mod) < 0)
    return -1;

  struct dwfl_relocation *sections = mod->reloc_info;

  /* The sections are sorted by address, so we can use binary search.  */
  size_t l = 0, u = sections->count;
  while (l < u)
    {
      size_t idx = (l + u) / 2;
      if (*addr < sections->refs[idx].start)
	u = idx;
      else if (*addr > sections->refs[idx].end)
	l = idx + 1;
      else
	{
	  /* Consider the limit of a section to be inside it, unless it's
	     inside the next one.  A section limit address can appear in
	     line records.  */
	  if (*addr == sections->refs[idx].end
	      && idx < sections->count
	      && *addr == sections->refs[idx + 1].start)
	    ++idx;

	  *addr -= sections->refs[idx].start;
	  return idx;
	}
    }

  __libdw_seterrno (DWARF_E_NO_MATCH);
  return -1;
}
INTDEF (dwfl_module_relocate_address)
