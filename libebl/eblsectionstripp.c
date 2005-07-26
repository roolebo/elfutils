/* Check whether section can be stripped.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include "libeblP.h"


bool
ebl_section_strip_p (Ebl *ebl, const GElf_Ehdr *ehdr, const GElf_Shdr *shdr,
		     const char *name, bool remove_comment,
		     bool only_remove_debug)
{
  /* If only debug information should be removed check the name.  There
     is unfortunately no other way.  */
  if (unlikely (only_remove_debug))
    {
      if (ebl_debugscn_p (ebl, name))
	return true;

      if (shdr->sh_type == SHT_RELA || shdr->sh_type == SHT_REL)
	{
	  Elf_Scn *scn_l = elf_getscn (ebl->elf, (shdr)->sh_info);
	  GElf_Shdr shdr_mem_l;
	  GElf_Shdr *shdr_l = gelf_getshdr (scn_l, &shdr_mem_l);
	  if (shdr_l == NULL)
	    {
	      const char *s_l = elf_strptr (ebl->elf, ehdr->e_shstrndx,
					    shdr_l->sh_name);
	      if (s_l != NULL && ebl_debugscn_p (ebl, s_l))
		return true;
	    }
	}

      return false;
    }

  return SECTION_STRIP_P (shdr, name, remove_comment);
}
