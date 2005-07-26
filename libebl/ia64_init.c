/* Initialization of IA-64 specific backend library.
   Copyright (C) 2002, 2003, 2005 Red Hat, Inc.
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

#include <libebl_ia64.h>


const char *
ia64_init (elf, machine, eh, ehlen)
     Elf *elf __attribute__ ((unused));
     GElf_Half machine __attribute__ ((unused));
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return NULL;

  /* We handle it.  */
  eh->name = "Intel IA-64";
  eh->reloc_type_name = ia64_reloc_type_name;
  eh->reloc_type_check = ia64_reloc_type_check;
  eh->segment_type_name = ia64_segment_type_name;
  eh->dynamic_tag_name = ia64_dynamic_tag_name;
  eh->copy_reloc_p = ia64_copy_reloc_p;
  eh->destr = ia64_destr;

  return MODVERSION;
}
