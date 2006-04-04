/* Report a module to libdwfl based on ELF program headers.
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
#include <fcntl.h>
#include <unistd.h>


Dwfl_Module *
dwfl_report_elf (Dwfl *dwfl, const char *name,
		 const char *file_name, int fd, GElf_Addr base)
{
  bool closefd = false;

  if (fd < 0)
    {
      fd = open64 (file_name, O_RDONLY);
      if (fd < 0)
	{
	  __libdwfl_seterrno (DWFL_E_ERRNO);
	  return NULL;
	}
      closefd = true;
    }

  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP_PRIVATE, NULL);

  GElf_Ehdr ehdr_mem, *ehdr = gelf_getehdr (elf, &ehdr_mem);
  if (ehdr == NULL)
    {
    elf_error:
      __libdwfl_seterrno (DWFL_E_LIBELF);
      if (closefd)
	close (fd);
      return NULL;
    }

  GElf_Addr start = 0, end = 0, bias = 0;
  switch (ehdr->e_type)
    {
    case ET_REL:
      /* For a relocatable object, we do an arbitrary section layout.
	 By updating the section header in place, we leave the layout
	 information to be found by relocation.  */

      start = end = base;

      Elf_Scn *scn = NULL;
      while ((scn = elf_nextscn (elf, scn)) != NULL)
	{
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr == NULL)
	    goto elf_error;

	  if (shdr->sh_flags & SHF_ALLOC)
	    {
	      const GElf_Xword align = shdr->sh_addralign ?: 1;
	      shdr->sh_addr = (end + align - 1) & -align;
	      if (end == base)
		/* This is the first section assigned a location.
		   Use its aligned address as the module's base.  */
		start = shdr->sh_addr;
	      end = shdr->sh_addr + shdr->sh_size;
	      if (! gelf_update_shdr (scn, shdr))
		goto elf_error;
	    }
	}

      if (end == start)
	{
	  __libdwfl_seterrno (DWFL_E_BADELF);
	  if (closefd)
	    close (fd);
	  return NULL;
	}
      break;

      /* Everything else has to have program headers.  */

    case ET_EXEC:
    case ET_CORE:
      /* An assigned base address is meaningless for these.  */
      base = 0;

    case ET_DYN:
    default:
      for (uint_fast16_t i = 0; i < ehdr->e_phnum; ++i)
	{
	  GElf_Phdr phdr_mem, *ph = gelf_getphdr (elf, i, &phdr_mem);
	  if (ph == NULL)
	    goto elf_error;
	  if (ph->p_type == PT_LOAD)
	    {
	      if ((base & (ph->p_align - 1)) != 0)
		base = (base + ph->p_align - 1) & -ph->p_align;
	      start = base + (ph->p_vaddr & -ph->p_align);
	      break;
	    }
	}
      bias = base;

      for (uint_fast16_t i = ehdr->e_phnum; i-- > 0;)
	{
	  GElf_Phdr phdr_mem, *ph = gelf_getphdr (elf, i, &phdr_mem);
	  if (ph == NULL)
	    goto elf_error;
	  if (ph->p_type == PT_LOAD)
	    {
	      end = base + (ph->p_vaddr + ph->p_memsz);
	      break;
	    }
	}

      if (end == 0)
	{
	  __libdwfl_seterrno (DWFL_E_NO_PHDR);
	  if (closefd)
	    close (fd);
	  return NULL;
	}
      break;
    }

  Dwfl_Module *m = INTUSE(dwfl_report_module) (dwfl, name, start, end);
  if (m != NULL)
    {
      if (m->main.name == NULL)
	{
	  m->main.name = strdup (file_name);
	  m->main.fd = fd;
	}
      else if ((fd >= 0 && m->main.fd != fd)
	       || strcmp (m->main.name, file_name))
	{
	  elf_end (elf);
	overlap:
	  if (closefd)
	    close (fd);
	  m->gc = true;
	  __libdwfl_seterrno (DWFL_E_OVERLAP);
	  m = NULL;
	}

      /* Preinstall the open ELF handle for the module.  */
      if (m->main.elf == NULL)
	{
	  m->main.elf = elf;
	  m->main.bias = bias;
	  m->e_type = ehdr->e_type;
	}
      else
	{
	  elf_end (elf);
	  if (m->main.bias != base)
	    goto overlap;
	}
    }
  return m;
}
INTDEF (dwfl_report_elf)
