/* Create new ELF header.
   Copyright (C) 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gelf.h>
#include <stdlib.h>

#include "libelfP.h"


Elf_Scn *
gelf_offscn (elf, offset)
     Elf *elf;
     GElf_Off offset;
{
  if (elf->class == ELFCLASS32)
    {
      if ((Elf32_Off) offset != offset)
	{
	  __libelf_seterrno (ELF_E_INVALID_OFFSET);
	  return NULL;
	}

      return INTUSE(elf32_offscn) (elf, (Elf32_Off) offset);
    }

  return INTUSE(elf64_offscn) (elf, offset);
}
