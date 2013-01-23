/* Find debugging and symbol information for a module in libdwfl.
   Copyright (C) 2006-2013 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "libdwflP.h"

const char *
dwfl_module_getsym (Dwfl_Module *mod, int ndx,
		    GElf_Sym *sym, GElf_Word *shndxp)
{
  if (unlikely (mod == NULL))
    return NULL;

  if (unlikely (mod->symdata == NULL))
    {
      int result = INTUSE(dwfl_module_getsymtab) (mod);
      if (result < 0)
	return NULL;
    }

  /* All local symbols should come before all global symbols.  If we
     have an auxiliary table make sure all the main locals come first,
     then all aux locals, then all main globals and finally all aux globals.
     And skip the auxiliary table zero undefined entry.  */
  GElf_Word shndx;
  int tndx = ndx;
  int skip_aux_zero = (mod->syments > 0 && mod->aux_syments > 0) ? 1 : 0;
  struct dwfl_file *file;
  Elf_Data *symdata;
  Elf_Data *symxndxdata;
  Elf_Data *symstrdata;
  if (mod->aux_symdata == NULL
      || ndx < mod->first_global)
    {
      /* main symbol table (locals).  */
      tndx = ndx;
      file = mod->symfile;
      symdata = mod->symdata;
      symxndxdata = mod->symxndxdata;
      symstrdata = mod->symstrdata;
    }
  else if (ndx < mod->first_global + mod->aux_first_global - skip_aux_zero)
    {
      /* aux symbol table (locals).  */
      tndx = ndx - mod->first_global + skip_aux_zero;
      file = &mod->aux_sym;
      symdata = mod->aux_symdata;
      symxndxdata = mod->aux_symxndxdata;
      symstrdata = mod->aux_symstrdata;
    }
  else if ((size_t) ndx < mod->syments + mod->aux_first_global - skip_aux_zero)
    {
      /* main symbol table (globals).  */
      tndx = ndx - mod->aux_first_global + skip_aux_zero;
      file = mod->symfile;
      symdata = mod->symdata;
      symxndxdata = mod->symxndxdata;
      symstrdata = mod->symstrdata;
    }
  else
    {
      /* aux symbol table (globals).  */
      tndx = ndx - mod->syments + skip_aux_zero;
      file = &mod->aux_sym;
      symdata = mod->aux_symdata;
      symxndxdata = mod->aux_symxndxdata;
      symstrdata = mod->aux_symstrdata;
    }
  sym = gelf_getsymshndx (symdata, symxndxdata, tndx, sym, &shndx);

  if (unlikely (sym == NULL))
    {
      __libdwfl_seterrno (DWFL_E_LIBELF);
      return NULL;
    }

  if (sym->st_shndx != SHN_XINDEX)
    shndx = sym->st_shndx;

  /* Figure out whether this symbol points into an SHF_ALLOC section.  */
  bool alloc = true;
  if ((shndxp != NULL || mod->e_type != ET_REL)
      && (sym->st_shndx == SHN_XINDEX
	  || (sym->st_shndx < SHN_LORESERVE && sym->st_shndx != SHN_UNDEF)))
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (elf_getscn (file->elf, shndx),
				      &shdr_mem);
      alloc = unlikely (shdr == NULL) || (shdr->sh_flags & SHF_ALLOC);
    }

  if (shndxp != NULL)
    /* Yield -1 in case of a non-SHF_ALLOC section.  */
    *shndxp = alloc ? shndx : (GElf_Word) -1;

  switch (sym->st_shndx)
    {
    case SHN_ABS:		/* XXX sometimes should use bias?? */
    case SHN_UNDEF:
    case SHN_COMMON:
      break;

    default:
      if (mod->e_type == ET_REL)
	{
	  /* In an ET_REL file, the symbol table values are relative
	     to the section, not to the module's load base.  */
	  size_t symshstrndx = SHN_UNDEF;
	  Dwfl_Error result = __libdwfl_relocate_value (mod, file->elf,
							&symshstrndx,
							shndx, &sym->st_value);
	  if (unlikely (result != DWFL_E_NOERROR))
	    {
	      __libdwfl_seterrno (result);
	      return NULL;
	    }
	}
      else if (alloc)
	/* Apply the bias to the symbol value.  */
	sym->st_value = dwfl_adjusted_st_value (mod, file, sym->st_value);
      break;
    }

  if (unlikely (sym->st_name >= symstrdata->d_size))
    {
      __libdwfl_seterrno (DWFL_E_BADSTROFF);
      return NULL;
    }
  return (const char *) symstrdata->d_buf + sym->st_name;
}
INTDEF (dwfl_module_getsym)
