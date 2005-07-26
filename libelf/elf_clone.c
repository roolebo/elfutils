/* Create clone of a given descriptor.
   Copyright (C) 2003, 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include "libelfP.h"
#include "common.h"


Elf *
elf_clone (Elf *elf, Elf_Cmd cmd)
{
  Elf *retval = NULL;

  if (elf == NULL)
    /* Some earlier mistake.  */
    return NULL;

  /* Make sure the descriptor is not suddenly going away.  */
  rwlock_rdlock (elf->lock);

  if (cmd != ELF_C_EMPTY)
    // XXX TODO handle ELF_C_READ/WRITE etc
    goto out;

  retval = allocate_elf (elf->fildes, elf->map_address, elf->start_offset,
			 elf->maximum_size, elf->cmd, elf->parent, elf->kind,
			 elf->state.elf32.scns.max * sizeof (Elf_Scn));
  if (retval != NULL)
    {
      /* We have to write to the file in any case.  */
      retval->flags = ELF_F_DIRTY;

      /* Some more or less arbitrary value.  */
      retval->state.elf.scnincr = 10;

      /* We have allocated room for some sections.  */
      assert (offsetof (struct Elf, state.elf32.scns)
	      == offsetof (struct Elf, state.elf64.scns));
      retval->state.elf.scns_last = &retval->state.elf32.scns;
      retval->state.elf32.scns.max = elf->state.elf32.scns.max;

      retval->class = elf->class;
    }

  /* Release the lock.  */
 out:
  rwlock_unlock (elf->lock);

  return retval;
}
