/* SH specific relocation handling.
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

#include <elf.h>
#include <stddef.h>

#define BACKEND sh_
#include "libebl_CPU.h"


/* Return true if the symbol type is that referencing the GOT.  */
bool
sh_gotpc_reloc_check (Elf *elf __attribute__ ((unused)), int type)
{
  return type == R_SH_GOTPC;
}

/* Check for the simple reloc types.  */
Elf_Type
sh_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_SH_DIR32:
      return ELF_T_WORD;
    default:
      return ELF_T_NUM;
    }
}
