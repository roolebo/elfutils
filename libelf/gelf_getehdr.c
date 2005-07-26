/* Get ELF header.
   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 1998.

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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "libelfP.h"


GElf_Ehdr *
gelf_getehdr (elf, dest)
     Elf *elf;
     GElf_Ehdr *dest;
{
  GElf_Ehdr *result = NULL;

  if (elf == NULL)
    return NULL;

  if (unlikely (elf->kind != ELF_K_ELF))
    {
      __libelf_seterrno (ELF_E_INVALID_HANDLE);
      return NULL;
    }

  rwlock_rdlock (elf->lock);

  /* The following is an optimization: the ehdr element is at the same
     position in both the elf32 and elf64 structure.  */
  if (offsetof (struct Elf, state.elf32.ehdr)
      != offsetof (struct Elf, state.elf64.ehdr))
    abort ();
  /* Just pick one of the values.  */
 if (unlikely (elf->state.elf64.ehdr == NULL))
    /* Maybe no ELF header was created yet.  */
    __libelf_seterrno (ELF_E_WRONG_ORDER_EHDR);
  else if (elf->class == ELFCLASS32)
    {
      Elf32_Ehdr *ehdr = elf->state.elf32.ehdr;

      /* Convert the 32-bit struct to an 64-bit one.  */
      memcpy (dest->e_ident, ehdr->e_ident, EI_NIDENT);
#define COPY(name) \
      dest->name = ehdr->name
      COPY (e_type);
      COPY (e_machine);
      COPY (e_version);
      COPY (e_entry);
      COPY (e_phoff);
      COPY (e_shoff);
      COPY (e_flags);
      COPY (e_ehsize);
      COPY (e_phentsize);
      COPY (e_phnum);
      COPY (e_shentsize);
      COPY (e_shnum);
      COPY (e_shstrndx);

      result = dest;
    }
  else
    result = memcpy (dest, elf->state.elf64.ehdr, sizeof (*dest));

  rwlock_unlock (elf->lock);

  return result;
}
INTDEF(gelf_getehdr)
