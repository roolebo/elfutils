/* Return offset in archive for current file ELF.
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
#include <libelf.h>
#include <stddef.h>

#include "libelfP.h"


off_t
elf_getaroff (elf)
     Elf *elf;
{
  /* Be gratious, the specs demand it.  */
  if (elf == NULL || elf->parent == NULL)
    return ELF_C_NULL;

  /* We can be sure the parent is an archive.  */
  Elf *parent = elf->parent;
  assert (parent->kind == ELF_K_AR);

  return parent->state.ar.offset;
}
