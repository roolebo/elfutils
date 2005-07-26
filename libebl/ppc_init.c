/* Initialization of PPC specific backend library.
   Copyright (C) 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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

#include <libebl_ppc.h>


const char *
ppc_init (elf, machine, eh, ehlen)
     Elf *elf __attribute__ ((unused));
     GElf_Half machine __attribute__ ((unused));
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return NULL;

  /* We handle it.  */
  eh->name = "PowerPC";
  eh->reloc_type_name = ppc_reloc_type_name;
  eh->reloc_type_check = ppc_reloc_type_check;
  eh->reloc_valid_use = ppc_reloc_valid_use;
  eh->reloc_simple_type = ppc_reloc_simple_type;
  eh->copy_reloc_p = ppc_copy_reloc_p;
  eh->destr = ppc_destr;

  return MODVERSION;
}
