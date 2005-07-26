/* Initialization of x86-64 specific backend library.
   Copyright (C) 2002, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <libebl_x86_64.h>


const char *
x86_64_init (elf, machine, eh, ehlen)
     Elf *elf __attribute__ ((unused));
     GElf_Half machine __attribute__ ((unused));
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return NULL;

  /* We handle it.  */
  eh->name = "AMD x86-64";
  eh->reloc_type_name = x86_64_reloc_type_name;
  eh->reloc_type_check = x86_64_reloc_type_check;
  eh->reloc_valid_use = x86_64_reloc_valid_use;
  eh->reloc_simple_type = x86_64_reloc_simple_type;
  eh->core_note = x86_64_core_note;
  eh->copy_reloc_p = x86_64_copy_reloc_p;
  eh->destr = x86_64_destr;

  return MODVERSION;
}
