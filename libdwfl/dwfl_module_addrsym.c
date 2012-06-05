/* Find debugging and symbol information for a module in libdwfl.
   Copyright (C) 2005-2011 Red Hat, Inc.
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

/* Returns the name of the symbol "closest" to ADDR.
   Never returns symbols at addresses above ADDR.  */

const char *
dwfl_module_addrsym (Dwfl_Module *mod, GElf_Addr addr,
		     GElf_Sym *closest_sym, GElf_Word *shndxp)
{
  int syments = INTUSE(dwfl_module_getsymtab) (mod);
  if (syments < 0)
    return NULL;

  /* Return true iff we consider ADDR to lie in the same section as SYM.  */
  GElf_Word addr_shndx = SHN_UNDEF;
  inline bool same_section (const GElf_Sym *sym, GElf_Word shndx)
    {
      /* For absolute symbols and the like, only match exactly.  */
      if (shndx >= SHN_LORESERVE)
	return sym->st_value == addr;

      /* Figure out what section ADDR lies in.  */
      if (addr_shndx == SHN_UNDEF)
	{
	  GElf_Addr mod_addr = dwfl_deadjust_st_value (mod, addr);
	  Elf_Scn *scn = NULL;
	  addr_shndx = SHN_ABS;
	  while ((scn = elf_nextscn (mod->symfile->elf, scn)) != NULL)
	    {
	      GElf_Shdr shdr_mem;
	      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	      if (likely (shdr != NULL)
		  && mod_addr >= shdr->sh_addr
		  && mod_addr < shdr->sh_addr + shdr->sh_size)
		{
		  addr_shndx = elf_ndxscn (scn);
		  break;
		}
	    }
	}

      return shndx == addr_shndx;
    }

  /* Keep track of the closest symbol we have seen so far.
     Here we store only symbols with nonzero st_size.  */
  const char *closest_name = NULL;
  GElf_Word closest_shndx = SHN_UNDEF;

  /* Keep track of an eligible symbol with st_size == 0 as a fallback.  */
  const char *sizeless_name = NULL;
  GElf_Sym sizeless_sym = { 0, 0, 0, 0, 0, SHN_UNDEF };
  GElf_Word sizeless_shndx = SHN_UNDEF;

  /* Keep track of the lowest address a relevant sizeless symbol could have.  */
  GElf_Addr min_label = 0;

  /* Look through the symbol table for a matching symbol.  */
  inline void search_table (int start, int end)
    {
      for (int i = start; i < end; ++i)
	{
	  GElf_Sym sym;
	  GElf_Word shndx;
	  const char *name = INTUSE(dwfl_module_getsym) (mod, i, &sym, &shndx);
	  if (name != NULL && name[0] != '\0'
	      && sym.st_shndx != SHN_UNDEF
	      && sym.st_value <= addr
	      && GELF_ST_TYPE (sym.st_info) != STT_SECTION
	      && GELF_ST_TYPE (sym.st_info) != STT_FILE
	      && GELF_ST_TYPE (sym.st_info) != STT_TLS)
	    {
	      /* Even if we don't choose this symbol, its existence excludes
		 any sizeless symbol (assembly label) that is below its upper
		 bound.  */
	      if (sym.st_value + sym.st_size > min_label)
		min_label = sym.st_value + sym.st_size;

	      if (sym.st_size == 0 || addr - sym.st_value < sym.st_size)
		{
		  /* This symbol is a better candidate than the current one
		     if it's closer to ADDR or is global when it was local.  */
		  if (closest_name == NULL
		      || closest_sym->st_value < sym.st_value
		      || (GELF_ST_BIND (closest_sym->st_info)
			  < GELF_ST_BIND (sym.st_info)))
		    {
		      if (sym.st_size != 0)
			{
			  *closest_sym = sym;
			  closest_shndx = shndx;
			  closest_name = name;
			}
		      else if (closest_name == NULL
			       && sym.st_value >= min_label
			       && same_section (&sym, shndx))
			{
			  /* Handwritten assembly symbols sometimes have no
			     st_size.  If no symbol with proper size includes
			     the address, we'll use the closest one that is in
			     the same section as ADDR.  */
			  sizeless_sym = sym;
			  sizeless_shndx = shndx;
			  sizeless_name = name;
			}
		    }
		  /* When the beginning of its range is no closer,
		     the end of its range might be.  But do not
		     replace a global symbol with a local!  */
		  else if (sym.st_size != 0
			   && closest_sym->st_value == sym.st_value
			   && closest_sym->st_size > sym.st_size
			   && (GELF_ST_BIND (closest_sym->st_info)
			       <= GELF_ST_BIND (sym.st_info)))
		    {
		      *closest_sym = sym;
		      closest_shndx = shndx;
		      closest_name = name;
		    }
		}
	    }
	}
    }

  /* First go through global symbols.  mod->first_global is setup by
     dwfl_module_getsymtab to the index of the first global symbol in
     the module's symbol table, or -1 when unknown.  All symbols with
     local binding come first in the symbol table, then all globals.  */
  search_table (mod->first_global < 0 ? 1 : mod->first_global, syments);

  /* If we found nothing searching the global symbols, then try the locals.
     Unless we have a global sizeless symbol that matches exactly.  */
  if (closest_name == NULL && mod->first_global > 1
      && (sizeless_name == NULL || sizeless_sym.st_value != addr))
    search_table (1, mod->first_global);

  /* If we found no proper sized symbol to use, fall back to the best
     candidate sizeless symbol we found, if any.  */
  if (closest_name == NULL
      && sizeless_name != NULL && sizeless_sym.st_value >= min_label)
    {
      *closest_sym = sizeless_sym;
      closest_shndx = sizeless_shndx;
      closest_name = sizeless_name;
    }

  if (shndxp != NULL)
    *shndxp = closest_shndx;
  return closest_name;
}
INTDEF (dwfl_module_addrsym)
