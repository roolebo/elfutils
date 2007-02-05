/* Find debugging and symbol information for a module in libdwfl.
   Copyright (C) 2005, 2006, 2007 Red Hat, Inc.
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

const char *
dwfl_module_addrname (Dwfl_Module *mod, GElf_Addr addr)
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

      /* Ignore section and other special symbols.  */
      switch (GELF_ST_TYPE (sym->st_info))
	{
	case STT_SECTION:
	case STT_FILE:
	case STT_TLS:
	  return false;
	}

      /* Figure out what section ADDR lies in.  */
      if (addr_shndx == SHN_UNDEF)
	{
	  GElf_Addr mod_addr = addr - mod->symfile->bias;
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

  /* Look through the symbol table for a matching symbol.  */
  const char *closest = NULL;
  GElf_Addr closest_value = 0;
  for (int i = 1; i < syments; ++i)
    {
      GElf_Sym sym;
      GElf_Word shndx;
      const char *name = INTUSE(dwfl_module_getsym) (mod, i, &sym, &shndx);
      if (name != NULL && sym.st_value <= addr)
	{
	  if (addr < sym.st_value + sym.st_size)
	    return name;

	  /* Handwritten assembly symbols sometimes have no st_size.
	     If no symbol with proper size includes the address, we'll
	     use the closest one that is in the same section as ADDR.   */
	  if (sym.st_size == 0 && sym.st_value >= closest_value
	      && same_section (&sym, shndx))
	    {
	      closest_value = sym.st_value;
	      closest = name;
	    }
	}
    }

  return closest;
}
