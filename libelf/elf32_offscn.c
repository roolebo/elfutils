/* Get section at specific index.
   Copyright (C) 2005 Red Hat, Inc.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "libelfP.h"

#ifndef LIBELFBITS
# define LIBELFBITS 32
#endif


Elf_Scn *
elfw2(LIBELFBITS,offscn) (elf, offset)
     Elf *elf;
     ElfW2(LIBELFBITS,Off) offset;
{
  if (elf == NULL)
    return NULL;

  if (unlikely (elf->kind != ELF_K_ELF))
    {
      __libelf_seterrno (ELF_E_INVALID_HANDLE);
      return NULL;
    }

  rwlock_rdlock (elf->lock);

  Elf_Scn *result = NULL;

  /* Find the section in the list.  */
  Elf_ScnList *runp = &elf->state.ELFW(elf,LIBELFBITS).scns;
  while (1)
    {
      for (unsigned int i = 0; i < runp->cnt; ++i)
	if (runp->data[i].shdr.ELFW(e,LIBELFBITS)->sh_offset == offset)
	  {
	    result = &runp->data[i];
	    goto out;
	  }

      runp = runp->next;
      if (runp == NULL)
	{
	  __libelf_seterrno (ELF_E_INVALID_OFFSET);
	  break;
	}
    }

 out:
  rwlock_unlock (elf->lock);

  return result;
}
INTDEF(elfw2(LIBELFBITS,offscn))
