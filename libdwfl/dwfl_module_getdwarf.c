/* Find debugging and symbol information for a module in libdwfl.
   Copyright (C) 2005, 2006 Red Hat, Inc.
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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "../libdw/libdwP.h"	/* DWARF_E_* values are here.  */


/* Open libelf FILE->fd and compute the load base of ELF as loaded in MOD.
   When we return success, FILE->elf and FILE->bias are set up.  */
static inline Dwfl_Error
open_elf (Dwfl_Module *mod, struct dwfl_file *file)
{
  if (file->elf == NULL)
    {
      if (file->fd < 0)
	return CBFAIL;

      file->elf = elf_begin (file->fd, ELF_C_READ_MMAP_PRIVATE, NULL);
    }

  GElf_Ehdr ehdr_mem, *ehdr = gelf_getehdr (file->elf, &ehdr_mem);
  if (ehdr == NULL)
    return DWFL_E (LIBELF, elf_errno ());

  mod->e_type = ehdr->e_type;

  file->bias = 0;
  for (uint_fast16_t i = 0; i < ehdr->e_phnum; ++i)
    {
      GElf_Phdr ph_mem;
      GElf_Phdr *ph = gelf_getphdr (file->elf, i, &ph_mem);
      if (ph == NULL)
	return DWFL_E_LIBELF;
      if (ph->p_type == PT_LOAD)
	{
	  file->bias = ((mod->low_addr & -ph->p_align)
			- (ph->p_vaddr & -ph->p_align));
	  break;
	}
    }

  return DWFL_E_NOERROR;
}

/* Find the main ELF file for this module and open libelf on it.
   When we return success, MOD->main.elf and MOD->main.bias are set up.  */
static void
find_file (Dwfl_Module *mod)
{
  if (mod->main.elf != NULL	/* Already done.  */
      || mod->elferr != DWFL_E_NOERROR)	/* Cached failure.  */
    return;

  mod->main.fd = (*mod->dwfl->callbacks->find_elf) (MODCB_ARGS (mod),
						    &mod->main.name,
						    &mod->main.elf);
  mod->elferr = open_elf (mod, &mod->main);
}

/* Find the separate debuginfo file for this module and open libelf on it.
   When we return success, MOD->debug is set up.  */
static Dwfl_Error
find_debuginfo (Dwfl_Module *mod)
{
  if (mod->debug.elf != NULL)
    return DWFL_E_NOERROR;

  size_t shstrndx;
  if (elf_getshstrndx (mod->main.elf, &shstrndx) < 0)
    return DWFL_E_LIBELF;

  Elf_Scn *scn = elf_getscn (mod->main.elf, 0);
  if (scn == NULL)
    return DWFL_E_LIBELF;
  do
    {
      GElf_Shdr shdr_mem, *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	return DWFL_E_LIBELF;

      const char *name = elf_strptr (mod->main.elf, shstrndx, shdr->sh_name);
      if (name == NULL)
	return DWFL_E_LIBELF;

      if (!strcmp (name, ".gnu_debuglink"))
	break;

      scn = elf_nextscn (mod->main.elf, scn);
    } while (scn != NULL);

  const char *debuglink_file = NULL;
  GElf_Word debuglink_crc = 0;
  if (scn != NULL)
    {
      /* Found the .gnu_debuglink section.  Extract its contents.  */
      Elf_Data *rawdata = elf_rawdata (scn, NULL);
      if (rawdata == NULL)
	return DWFL_E_LIBELF;

      Elf_Data crcdata =
	{
	  .d_type = ELF_T_WORD,
	  .d_buf = &debuglink_crc,
	  .d_size = sizeof debuglink_crc,
	  .d_version = EV_CURRENT,
	};
      Elf_Data conv =
	{
	  .d_type = ELF_T_WORD,
	  .d_buf = rawdata->d_buf + rawdata->d_size - sizeof debuglink_crc,
	  .d_size = sizeof debuglink_crc,
	  .d_version = EV_CURRENT,
	};

      GElf_Ehdr ehdr_mem;
      GElf_Ehdr *ehdr = gelf_getehdr (mod->main.elf, &ehdr_mem);
      if (ehdr == NULL)
	return DWFL_E_LIBELF;

      Elf_Data *d = gelf_xlatetom (mod->main.elf, &crcdata, &conv,
				   ehdr->e_ident[EI_DATA]);
      if (d == NULL)
	return DWFL_E_LIBELF;
      assert (d == &crcdata);

      debuglink_file = rawdata->d_buf;
    }

  mod->debug.fd = (*mod->dwfl->callbacks->find_debuginfo) (MODCB_ARGS (mod),
							   mod->main.name,
							   debuglink_file,
							   debuglink_crc,
							   &mod->debug.name);
  return open_elf (mod, &mod->debug);
}


/* Try to find a symbol table in FILE.  */
static Dwfl_Error
load_symtab (struct dwfl_file *file, struct dwfl_file **symfile,
	     Elf_Scn **symscn, Elf_Scn **xndxscn,
	     size_t *syments, GElf_Word *strshndx)
{
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (file->elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem, *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr != NULL)
	switch (shdr->sh_type)
	  {
	  case SHT_SYMTAB:
	    *symscn = scn;
	    *symfile = file;
	    *strshndx = shdr->sh_link;
	    *syments = shdr->sh_size / shdr->sh_entsize;
	    if (*symscn != NULL && *xndxscn != NULL)
	      return DWFL_E_NOERROR;
	    break;

	  case SHT_DYNSYM:
	    /* Use this if need be, but keep looking for SHT_SYMTAB.  */
	    *symscn = scn;
	    *symfile = file;
	    *strshndx = shdr->sh_link;
	    *syments = shdr->sh_size / shdr->sh_entsize;
	    break;

	  case SHT_SYMTAB_SHNDX:
	    *xndxscn = scn;
	    break;

	  default:
	    break;
	  }
    }
  return DWFL_E_NO_SYMTAB;
}

/* Try to find a symbol table in either MOD->main.elf or MOD->debug.elf.  */
static void
find_symtab (Dwfl_Module *mod)
{
  if (mod->symdata != NULL	/* Already done.  */
      || mod->symerr != DWFL_E_NOERROR) /* Cached previous failure.  */
    return;

  find_file (mod);
  mod->symerr = mod->elferr;
  if (mod->symerr != DWFL_E_NOERROR)
    return;

  /* First see if the main ELF file has the debugging information.  */
  Elf_Scn *symscn = NULL, *xndxscn = NULL;
  GElf_Word strshndx;
  mod->symerr = load_symtab (&mod->main, &mod->symfile, &symscn,
			     &xndxscn, &mod->syments, &strshndx);
  switch (mod->symerr)
    {
    default:
      return;

    case DWFL_E_NOERROR:
      break;

    case DWFL_E_NO_SYMTAB:
      /* Now we have to look for a separate debuginfo file.  */
      mod->symerr = find_debuginfo (mod);
      switch (mod->symerr)
	{
	default:
	  return;

	case DWFL_E_NOERROR:
	  mod->symerr = load_symtab (&mod->debug, &mod->symfile, &symscn,
				     &xndxscn, &mod->syments, &strshndx);
	  break;

	case DWFL_E_CB:		/* The find_debuginfo hook failed.  */
	  mod->symerr = DWFL_E_NO_SYMTAB;
	  break;
	}

      switch (mod->symerr)
	{
	default:
	  return;

	case DWFL_E_NOERROR:
	  break;

	case DWFL_E_NO_SYMTAB:
	  if (symscn == NULL)
	    return;
	  /* We still have the dynamic symbol table.  */
	  mod->symerr = DWFL_E_NOERROR;
	  break;
	}
      break;
    }

  /* This does some sanity checks on the string table section.  */
  if (elf_strptr (mod->symfile->elf, strshndx, 0) == NULL)
    {
    elferr:
      mod->symerr = DWFL_E (LIBELF, elf_errno ());
      return;
    }

  /* Cache the data; MOD->syments was set above.  */

  mod->symstrdata = elf_getdata (elf_getscn (mod->symfile->elf, strshndx),
				 NULL);
  if (mod->symstrdata == NULL)
    goto elferr;

  if (xndxscn == NULL)
    mod->symxndxdata = NULL;
  else
    {
      mod->symxndxdata = elf_getdata (xndxscn, NULL);
      if (mod->symxndxdata == NULL)
	goto elferr;
    }

  mod->symdata = elf_getdata (symscn, NULL);
  if (mod->symdata == NULL)
    goto elferr;
}


/* Try to open a libebl backend for MOD.  */
Dwfl_Error
internal_function
__libdwfl_module_getebl (Dwfl_Module *mod)
{
  if (mod->ebl == NULL)
    {
      find_file (mod);
      if (mod->elferr != DWFL_E_NOERROR)
	return mod->elferr;

      mod->ebl = ebl_openbackend (mod->main.elf);
      if (mod->ebl == NULL)
	return DWFL_E_LIBEBL;
    }
  return DWFL_E_NOERROR;
}

/* Try to start up libdw on DEBUGFILE.  */
static Dwfl_Error
load_dw (Dwfl_Module *mod, struct dwfl_file *debugfile)
{
  if (mod->e_type == ET_REL)
    {
      const Dwfl_Callbacks *const cb = mod->dwfl->callbacks;

      /* The debugging sections have to be relocated.  */
      if (cb->section_address == NULL)
	return DWFL_E_NOREL;

      Dwfl_Error error = __libdwfl_module_getebl (mod);
      if (error != DWFL_E_NOERROR)
	return error;

      find_symtab (mod);
      Dwfl_Error result = mod->symerr;
      if (result == DWFL_E_NOERROR)
	result = __libdwfl_relocate (mod, debugfile->elf);
      if (result != DWFL_E_NOERROR)
	return result;

      /* Don't keep the file descriptors around.  */
      if (mod->main.fd != -1 && elf_cntl (mod->main.elf, ELF_C_FDREAD) == 0)
	{
	  close (mod->main.fd);
	  mod->main.fd = -1;
	}
      if (debugfile->fd != -1 && elf_cntl (debugfile->elf, ELF_C_FDREAD) == 0)
	{
	  close (debugfile->fd);
	  debugfile->fd = -1;
	}
    }

  mod->dw = INTUSE(dwarf_begin_elf) (debugfile->elf, DWARF_C_READ, NULL);
  if (mod->dw == NULL)
    {
      int err = INTUSE(dwarf_errno) ();
      return err == DWARF_E_NO_DWARF ? DWFL_E_NO_DWARF : DWFL_E (LIBDW, err);
    }

  /* Until we have iterated through all CU's, we might do lazy lookups.  */
  mod->lazycu = 1;

  return DWFL_E_NOERROR;
}

/* Try to start up libdw on either the main file or the debuginfo file.  */
static void
find_dw (Dwfl_Module *mod)
{
  if (mod->dw != NULL		/* Already done.  */
      || mod->dwerr != DWFL_E_NOERROR) /* Cached previous failure.  */
    return;

  find_file (mod);
  mod->dwerr = mod->elferr;
  if (mod->dwerr != DWFL_E_NOERROR)
    return;

  /* First see if the main ELF file has the debugging information.  */
  mod->dwerr = load_dw (mod, &mod->main);
  switch (mod->dwerr)
    {
    case DWFL_E_NOERROR:
      mod->debug.elf = mod->main.elf;
      mod->debug.bias = mod->main.bias;
      return;

    case DWFL_E_NO_DWARF:
      break;

    default:
      goto canonicalize;
    }

  /* Now we have to look for a separate debuginfo file.  */
  mod->dwerr = find_debuginfo (mod);
  switch (mod->dwerr)
    {
    case DWFL_E_NOERROR:
      mod->dwerr = load_dw (mod, &mod->debug);
      break;

    case DWFL_E_CB:		/* The find_debuginfo hook failed.  */
      mod->dwerr = DWFL_E_NO_DWARF;
      return;

    default:
      break;
    }

 canonicalize:
  mod->dwerr = __libdwfl_canon_error (mod->dwerr);
}


Elf *
dwfl_module_getelf (Dwfl_Module *mod, GElf_Addr *loadbase)
{
  if (mod == NULL)
    return NULL;

  find_file (mod);
  if (mod->elferr == DWFL_E_NOERROR)
    {
      *loadbase = mod->main.bias;
      return mod->main.elf;
    }

  __libdwfl_seterrno (mod->elferr);
  return NULL;
}
INTDEF (dwfl_module_getelf)


Dwarf *
dwfl_module_getdwarf (Dwfl_Module *mod, Dwarf_Addr *bias)
{
  if (mod == NULL)
    return NULL;

  find_dw (mod);
  if (mod->dwerr == DWFL_E_NOERROR)
    {
      *bias = mod->debug.bias;
      return mod->dw;
    }

  __libdwfl_seterrno (mod->dwerr);
  return NULL;
}
INTDEF (dwfl_module_getdwarf)

int
dwfl_module_getsymtab (Dwfl_Module *mod)
{
  if (mod == NULL)
    return -1;

  find_symtab (mod);
  if (mod->symerr == DWFL_E_NOERROR)
    return mod->syments;

  __libdwfl_seterrno (mod->symerr);
  return -1;
}
INTDEF (dwfl_module_getsymtab)
