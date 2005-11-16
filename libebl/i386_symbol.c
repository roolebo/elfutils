/* i386 specific symbolic name handling.
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

#include <assert.h>
#include <elf.h>
#include <stddef.h>
#include <string.h>

#define BACKEND i386_
#include "libebl_CPU.h"


/* Return true if the symbol type is that referencing the GOT.  */
bool
i386_gotpc_reloc_check (Elf *elf __attribute__ ((unused)), int type)
{
  return type == R_386_GOTPC;
}

/* Check for the simple reloc types.  */
Elf_Type
i386_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_386_32:
      return ELF_T_SWORD;
    case R_386_16:
      return ELF_T_HALF;
    case R_386_8:
      return ELF_T_BYTE;
    default:
      return ELF_T_NUM;
    }
}

/* Check section name for being that of a debug information section.  */
bool (*generic_debugscn_p) (const char *);
bool
i386_debugscn_p (const char *name)
{
  return (generic_debugscn_p (name)
	  || strcmp (name, ".stab") == 0
	  || strcmp (name, ".stabstr") == 0);
}
