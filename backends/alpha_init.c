/* Initialization of Alpha specific backend library.
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

#define BACKEND		alpha_
#define RELOC_PREFIX	R_ALPHA_
#include "libebl_CPU.h"

/* This defines the common reloc hooks based on alpha_reloc.def.  */
#include "common-reloc.c"


const char *
alpha_init (elf, machine, eh, ehlen)
     Elf *elf __attribute__ ((unused));
     GElf_Half machine __attribute__ ((unused));
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return NULL;

  /* We handle it.  */
  eh->name = "Alpha";
  alpha_init_reloc (eh);
  HOOK (eh, dynamic_tag_name);
  HOOK (eh, dynamic_tag_check);
  HOOK (eh, reloc_simple_type);
  HOOK (eh, return_value_location);

  return MODVERSION;
}
