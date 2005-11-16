/* Arm specific symbolic name handling.
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

#include <elf.h>
#include <stddef.h>

#define BACKEND		arm_
#include "libebl_CPU.h"

/* Check for the simple reloc types.  */
Elf_Type
arm_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_ARM_ABS32:
      return ELF_T_WORD;
    case R_ARM_ABS16:
      return ELF_T_HALF;
    case R_ARM_ABS8:
      return ELF_T_BYTE;
    default:
      return ELF_T_NUM;
    }
}
