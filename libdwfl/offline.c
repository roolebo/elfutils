/* Recover relocatibility for addresses computed from debug information.
   Copyright (C) 2005, 2006 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include "libdwflP.h"
#include <unistd.h>

/* Since dwfl_report_elf lays out the sections already, this will only be
   called when the section headers of the debuginfo file are being
   consulted instead.  With binutils strip-to-debug, the symbol table
   is in the debuginfo file and relocation looks there.  */
int
dwfl_offline_section_address (Dwfl_Module *mod,
			      void **userdata __attribute__ ((unused)),
			      const char *modname __attribute__ ((unused)),
			      Dwarf_Addr base __attribute__ ((unused)),
			      const char *secname __attribute__ ((unused)),
			      Elf32_Word shndx,
			      const GElf_Shdr *shdr __attribute__ ((unused)),
			      Dwarf_Addr *addr)
{
  assert (mod->symfile != &mod->main);

  GElf_Shdr shdr_mem;
  GElf_Shdr *main_shdr = gelf_getshdr (elf_getscn (mod->main.elf, shndx),
				       &shdr_mem);
  if (unlikely (main_shdr == NULL))
    return -1;

  assert (shdr->sh_addr == 0);
  assert (shdr->sh_flags & SHF_ALLOC);
  assert (main_shdr->sh_addr != 0);
  assert (main_shdr->sh_flags == shdr->sh_flags);

  *addr = main_shdr->sh_addr;
  return 0;
}
INTDEF (dwfl_offline_section_address)

Dwfl_Module *
dwfl_report_offline (Dwfl *dwfl, const char *name,
		     const char *file_name, int fd)
{
  if (dwfl == NULL)
    return NULL;

  Dwfl_Module *mod = INTUSE(dwfl_report_elf) (dwfl, name, file_name, fd,
					      dwfl->offline_next_address);
  if (mod != NULL)
    {
      /* If this is an ET_EXEC file with fixed addresses, the address range
	 it consumed may or may not intersect with the arbitrary range we
	 will use for relocatable modules.  Make sure we always use a free
	 range for the offline allocations.  If this module did use
	 offline_next_address, it may have rounded it up for the module's
	 alignment requirements.  */
      if ((dwfl->offline_next_address >= mod->low_addr
	   || mod->low_addr - dwfl->offline_next_address < OFFLINE_REDZONE)
	  && dwfl->offline_next_address < mod->high_addr + OFFLINE_REDZONE)
	dwfl->offline_next_address = mod->high_addr + OFFLINE_REDZONE;

      /* Don't keep the file descriptor around.  */
      if (mod->main.fd != -1 && elf_cntl (mod->main.elf, ELF_C_FDREAD) == 0)
	{
	  close (mod->main.fd);
	  mod->main.fd = -1;
	}
    }

  return mod;
}
INTDEF (dwfl_report_offline)
