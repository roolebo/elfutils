/* x86_64 specific symbolic name handling.
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

#include <assert.h>
#include <elf.h>
#include <stddef.h>

#define BACKEND		x86_64_
#include "libebl_CPU.h"

/* Check for the simple reloc types.  */
Elf_Type
x86_64_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_X86_64_64:
      return ELF_T_XWORD;
    case R_X86_64_32:
      return ELF_T_WORD;
    case R_X86_64_32S:
      return ELF_T_SWORD;
    case R_X86_64_16:
      return ELF_T_HALF;
    case R_X86_64_8:
      return ELF_T_BYTE;
    default:
      return ELF_T_NUM;
    }
}
