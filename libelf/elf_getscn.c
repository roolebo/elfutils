/* Get section at specific index.
   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004 Red Hat, Inc.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 1998.

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


Elf_Scn *
elf_getscn (elf, idx)
     Elf *elf;
     size_t idx;
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
  Elf_ScnList *runp = (elf->class == ELFCLASS32
		       || (offsetof (struct Elf, state.elf32.scns)
			   == offsetof (struct Elf, state.elf64.scns))
		       ? &elf->state.elf32.scns : &elf->state.elf64.scns);
  while (1)
    {
      if (idx < runp->max)
	{
	  if (idx < runp->cnt)
	    result = &runp->data[idx];
	  else
	    __libelf_seterrno (ELF_E_INVALID_INDEX);
	  break;
	}

      idx -= runp->max;

      runp = runp->next;
      if (runp == NULL)
	{
	  __libelf_seterrno (ELF_E_INVALID_INDEX);
	  break;
	}
    }

  rwlock_unlock (elf->lock);

  return result;
}
INTDEF(elf_getscn)
