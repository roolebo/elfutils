/* Initialization of i386 specific backend library.
   Copyright (C) 2000, 2001, 2002, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

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

#define BACKEND		i386_
#define RELOC_PREFIX	R_386_
#include "libebl_CPU.h"

/* This defines the common reloc hooks based on i386_reloc.def.  */
#include "common-reloc.c"

const char *
i386_init (elf, machine, eh, ehlen)
     Elf *elf __attribute__ ((unused));
     GElf_Half machine __attribute__ ((unused));
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return NULL;

  /* We handle it.  */
  eh->name = "Intel 80386";
  i386_init_reloc (eh);
  eh->reloc_simple_type = i386_reloc_simple_type;
  eh->gotpc_reloc_check = i386_gotpc_reloc_check;
  eh->core_note = i386_core_note;
  generic_debugscn_p = eh->debugscn_p;
  eh->debugscn_p = i386_debugscn_p;

  return MODVERSION;
}
