/* Report a module to libdwfl based on ELF program headers.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

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

  GElf_Addr start = 0, end = 0;
  for (uint_fast16_t i = 0; i < ehdr->e_phnum; ++i)
    {
      GElf_Phdr phdr_mem, *ph = gelf_getphdr (elf, i, &phdr_mem);
      if (ph == NULL)
	goto elf_error;
      if (ph->p_type == PT_LOAD)
	{
	  start = base + (ph->p_vaddr & -ph->p_align);
	  break;
	}
    }

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

  Dwfl_Module *m = INTUSE(dwfl_report_module) (dwfl, name,
					       base + start, base + end);
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
	  m->main.bias = base;
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
