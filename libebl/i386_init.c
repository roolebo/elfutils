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

#include <libebl_i386.h>


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
  eh->reloc_type_name = i386_reloc_type_name;
  eh->reloc_type_check = i386_reloc_type_check;
  eh->reloc_valid_use = i386_reloc_valid_use;
  eh->reloc_simple_type = i386_reloc_simple_type;
  eh->gotpc_reloc_check = i386_gotpc_reloc_check;
  eh->core_note = i386_core_note;
  generic_debugscn_p = eh->debugscn_p;
  eh->debugscn_p = i386_debugscn_p;
  eh->copy_reloc_p = i386_copy_reloc_p;
  eh->destr = i386_destr;

  return MODVERSION;
}
