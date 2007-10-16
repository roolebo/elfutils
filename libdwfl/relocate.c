/* Relocate debug information.
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

typedef uint8_t GElf_Byte;

/* Adjust *VALUE to add the load address of the SHNDX section.
   We update the section header in place to cache the result.  */

Dwfl_Error
internal_function
__libdwfl_relocate_value (Dwfl_Module *mod, Elf *elf, size_t *shstrndx,
			  Elf32_Word shndx, GElf_Addr *value)
{
  Elf_Scn *refscn = elf_getscn (elf, shndx);
  GElf_Shdr refshdr_mem, *refshdr = gelf_getshdr (refscn, &refshdr_mem);
  if (refshdr == NULL)
    return DWFL_E_LIBELF;

  if (refshdr->sh_addr == 0
      && (refshdr->sh_flags & SHF_ALLOC)
      && refshdr->sh_offset != 0)
    {
      /* This is a loaded section.  Find its actual
	 address and update the section header.  */

      if (*shstrndx == SHN_UNDEF
	  && unlikely (elf_getshstrndx (elf, shstrndx) < 0))
	return DWFL_E_LIBELF;

      const char *name = elf_strptr (elf, *shstrndx, refshdr->sh_name);
      if (unlikely (name == NULL))
	return DWFL_E_LIBELF;

      if ((*mod->dwfl->callbacks->section_address) (MODCB_ARGS (mod),
						    name, shndx, refshdr,
						    &refshdr->sh_addr))
	return CBFAIL;

      if (refshdr->sh_addr == (Dwarf_Addr) -1l)
	/* The callback indicated this section wasn't really loaded but we
	   don't really care.  */
	refshdr->sh_addr = 0;	/* Make no adjustment below.  */

      /* Mark it so we don't check it again for the next relocation.  */
      refshdr->sh_offset = 0;

      /* Update the in-core file's section header to show the final
	 load address (or unloadedness).  This serves as a cache,
	 so we won't get here again for the same section.  */
      if (unlikely (! gelf_update_shdr (refscn, refshdr)))
	return DWFL_E_LIBELF;
    }

  /* Apply the adjustment.  */
  *value += refshdr->sh_addr;
  return DWFL_E_NOERROR;
}

/* This is just doing dwfl_module_getsym, except that we must always use
   the symbol table in RELOCATED itself when it has one, not MOD->symfile.  */
static Dwfl_Error
relocate_getsym (Elf **symelf, Elf_Data **symdata, Elf_Data **symxndxdata,
		 size_t *symshstrndx,
		 Dwfl_Module *mod, Elf *relocated,
		 int symndx, GElf_Sym *sym, GElf_Word *shndx)
{
  if (*symdata == NULL)
    {
      if (mod->symfile->elf != relocated)
	{
	  /* We have to look up the symbol table in the file we are
	     relocating, if it has its own.  These reloc sections refer to
	     the symbol table in this file, and a symbol table in the main
	     file might not match.  However, some tools did produce ET_REL
	     .debug files with relocs but no symtab of their own.  */
	  Elf_Scn *scn = NULL;
	  while ((scn = elf_nextscn (relocated, scn)) != NULL)
	    {
	      GElf_Shdr shdr_mem, *shdr = gelf_getshdr (scn, &shdr_mem);
	      if (shdr != NULL)
		switch (shdr->sh_type)
		  {
		  default:
		    continue;
		  case SHT_SYMTAB:
		    *symelf = relocated;
		    *symdata = elf_getdata (scn, NULL);
		    if (unlikely (*symdata == NULL))
		      return DWFL_E_LIBELF;
		    break;
		  case SHT_SYMTAB_SHNDX:
		    *symxndxdata = elf_getdata (scn, NULL);
		    if (unlikely (*symxndxdata == NULL))
		      return DWFL_E_LIBELF;
		    break;
		  }
	      if (*symdata != NULL && *symxndxdata != NULL)
		break;
	    }
	}
      if (*symdata == NULL)
	{
	  /* The symbol table we have already cached is the one from
	     the file being relocated, so it's what we need.  Or else
	     this is an ET_REL .debug file with no .symtab of its own;
	     the symbols refer to the section indices in the main file.  */
	  *symelf = mod->symfile->elf;
	  *symdata = mod->symdata;
	  *symxndxdata = mod->symxndxdata;
	}
    }

  if (unlikely (gelf_getsymshndx (*symdata, *symxndxdata,
				  symndx, sym, shndx) == NULL))
    return DWFL_E_LIBELF;

  if (sym->st_shndx != SHN_XINDEX)
    *shndx = sym->st_shndx;

  switch (*shndx)
    {
    case SHN_ABS:
    case SHN_UNDEF:
    case SHN_COMMON:
      return DWFL_E_NOERROR;
    }

  return __libdwfl_relocate_value (mod, *symelf, symshstrndx,
				   *shndx, &sym->st_value);
}

Dwfl_Error
internal_function
__libdwfl_relocate (Dwfl_Module *mod, Elf *debugfile)
{
  assert (mod->e_type == ET_REL);

  GElf_Ehdr ehdr_mem;
  const GElf_Ehdr *ehdr = gelf_getehdr (debugfile, &ehdr_mem);
  if (ehdr == NULL)
    return DWFL_E_LIBELF;

  /* Cache used by relocate_getsym.  */
  Elf *reloc_symelf = NULL;
  Elf_Data *reloc_symdata = NULL;
  Elf_Data *reloc_symxndxdata = NULL;
  size_t reloc_symshstrndx = SHN_UNDEF;

  size_t d_shstrndx;
  if (elf_getshstrndx (debugfile, &d_shstrndx) < 0)
    return DWFL_E_LIBELF;
  if (mod->symfile->elf == debugfile)
    reloc_symshstrndx = d_shstrndx;

  /* Look at each section in the debuginfo file, and process the
     relocation sections for debugging sections.  */
  Dwfl_Error result = DWFL_E_NO_DWARF;
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (debugfile, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if ((shdr->sh_type == SHT_REL || shdr->sh_type == SHT_RELA)
	  && shdr->sh_size != 0)
	{
	  /* It's a relocation section.  First, fetch the name of the
	     section these relocations apply to.  */

	  Elf_Scn *tscn = elf_getscn (debugfile, shdr->sh_info);
	  if (tscn == NULL)
	    return DWFL_E_LIBELF;

	  GElf_Shdr tshdr_mem;
	  GElf_Shdr *tshdr = gelf_getshdr (tscn, &tshdr_mem);
	  const char *tname = elf_strptr (debugfile, d_shstrndx,
					  tshdr->sh_name);
	  if (tname == NULL)
	    return DWFL_E_LIBELF;

	  if (! ebl_debugscn_p (mod->ebl, tname))
	    /* This relocation section is not for a debugging section.
	       Nothing to do here.  */
	    continue;

	  /* Fetch the section data that needs the relocations applied.  */
	  Elf_Data *tdata = elf_rawdata (tscn, NULL);
	  if (tdata == NULL)
	    return DWFL_E_LIBELF;

	  /* Apply one relocation.  Returns true for any invalid data.  */
	  Dwfl_Error relocate (GElf_Addr offset, const GElf_Sxword *addend,
			       int rtype, int symndx)
	    {
	      /* First, resolve the symbol to an absolute value.  */
	      GElf_Addr value;

	      if (symndx == STN_UNDEF)
		/* When strip removes a section symbol referring to a
		   section moved into the debuginfo file, it replaces
		   that symbol index in relocs with STN_UNDEF.  We
		   don't actually need the symbol, because those relocs
		   are always references relative to the nonallocated
		   debugging sections, which start at zero.  */
		value = 0;
	      else
		{
		  GElf_Sym sym;
		  GElf_Word shndx;
		  Dwfl_Error error = relocate_getsym (&reloc_symelf,
						      &reloc_symdata,
						      &reloc_symxndxdata,
						      &reloc_symshstrndx,
						      mod, debugfile,
						      symndx, &sym, &shndx);
		  if (unlikely (error != DWFL_E_NOERROR))
		    return error;

		  if (shndx == SHN_UNDEF || shndx == SHN_COMMON)
		    return DWFL_E_RELUNDEF;

		  value = sym.st_value;
		}

	      /* These are the types we can relocate.  */
#define TYPES		DO_TYPE (BYTE, Byte); DO_TYPE (HALF, Half); \
	      		DO_TYPE (WORD, Word); DO_TYPE (SWORD, Sword); \
			DO_TYPE (XWORD, Xword); DO_TYPE (SXWORD, Sxword)
	      size_t size;
	      Elf_Type type = ebl_reloc_simple_type (mod->ebl, rtype);
	      switch (type)
		{
#define DO_TYPE(NAME, Name)				\
		  case ELF_T_##NAME:			\
		    size = sizeof (GElf_##Name);	\
		  break
		  TYPES;
#undef DO_TYPE
		default:
		  /* This might be because ebl_openbackend failed to find
		     any libebl_CPU.so library.  Diagnose that clearly.  */
		  if (ebl_get_elfmachine (mod->ebl) == EM_NONE)
		    return DWFL_E_UNKNOWN_MACHINE;
		  return DWFL_E_BADRELTYPE;
		}

	      if (offset + size >= tdata->d_size)
		return DWFL_E_BADRELOFF;

#define DO_TYPE(NAME, Name) GElf_##Name Name;
	      union { TYPES; } tmpbuf;
#undef DO_TYPE
	      Elf_Data tmpdata =
		{
		  .d_type = type,
		  .d_buf = &tmpbuf,
		  .d_size = size,
		  .d_version = EV_CURRENT,
		};
	      Elf_Data rdata =
		{
		  .d_type = type,
		  .d_buf = tdata->d_buf + offset,
		  .d_size = size,
		  .d_version = EV_CURRENT,
		};

	      /* XXX check for overflow? */
	      if (addend)
		{
		  /* For the addend form, we have the value already.  */
		  value += *addend;
		  switch (type)
		    {
#define DO_TYPE(NAME, Name)			\
		      case ELF_T_##NAME:	\
			tmpbuf.Name = value;	\
		      break
		      TYPES;
#undef DO_TYPE
		    default:
		      abort ();
		    }
		}
	      else
		{
		  /* Extract the original value and apply the reloc.  */
		  Elf_Data *d = gelf_xlatetom (debugfile, &tmpdata, &rdata,
					       ehdr->e_ident[EI_DATA]);
		  if (d == NULL)
		    return DWFL_E_LIBELF;
		  assert (d == &tmpdata);
		  switch (type)
		    {
#define DO_TYPE(NAME, Name)					\
		      case ELF_T_##NAME:			\
			tmpbuf.Name += (GElf_##Name) value;	\
		      break
		      TYPES;
#undef DO_TYPE
		    default:
		      abort ();
		    }
		}

	      /* Now convert the relocated datum back to the target
		 format.  This will write into rdata.d_buf, which
		 points into the raw section data being relocated.  */
	      Elf_Data *s = gelf_xlatetof (debugfile, &rdata, &tmpdata,
					   ehdr->e_ident[EI_DATA]);
	      if (s == NULL)
		return DWFL_E_LIBELF;
	      assert (s == &rdata);

	      /* We have applied this relocation!  */
	      return DWFL_E_NOERROR;
	    }

	  /* Fetch the relocation section and apply each reloc in it.  */
	  Elf_Data *reldata = elf_getdata (scn, NULL);
	  if (reldata == NULL)
	    return DWFL_E_LIBELF;

	  result = DWFL_E_NOERROR;
	  size_t nrels = shdr->sh_size / shdr->sh_entsize;
	  if (shdr->sh_type == SHT_REL)
	    for (size_t relidx = 0; !result && relidx < nrels; ++relidx)
	      {
		GElf_Rel rel_mem, *r = gelf_getrel (reldata, relidx, &rel_mem);
		if (r == NULL)
		  return DWFL_E_LIBELF;
		result = relocate (r->r_offset, NULL,
				   GELF_R_TYPE (r->r_info),
				   GELF_R_SYM (r->r_info));
	      }
	  else
	    for (size_t relidx = 0; !result && relidx < nrels; ++relidx)
	      {
		GElf_Rela rela_mem, *r = gelf_getrela (reldata, relidx,
						       &rela_mem);
		if (r == NULL)
		  return DWFL_E_LIBELF;
		result = relocate (r->r_offset, &r->r_addend,
				   GELF_R_TYPE (r->r_info),
				   GELF_R_SYM (r->r_info));
	      }
	  if (result != DWFL_E_NOERROR)
	    break;

	  /* Mark this relocation section as being empty now that we have
	     done its work.  This affects unstrip -R, so e.g. it emits an
	     empty .rela.debug_info along with a .debug_info that has
	     already been fully relocated.  */
	  shdr->sh_size = 0;
	  reldata->d_size = 0;
	  gelf_update_shdr (scn, shdr);
	}
    }

  return result;
}
