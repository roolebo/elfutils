/* Alpha specific symbolic name handling.
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

#define BACKEND		alpha_
#include "libebl_CPU.h"


const char *
alpha_dynamic_tag_name (int64_t tag, char *buf __attribute__ ((unused)),
			size_t len __attribute__ ((unused)))
{
  switch (tag)
    {
    case DT_ALPHA_PLTRO:
      return "ALPHA_PLTRO";
    default:
      break;
    }
  return NULL;
}

bool
alpha_dynamic_tag_check (int64_t tag)
{
  return tag == DT_ALPHA_PLTRO;
}

/* Check for the simple reloc types.  */
Elf_Type
alpha_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_ALPHA_REFLONG:
      return ELF_T_WORD;
    case R_ALPHA_REFQUAD:
      return ELF_T_XWORD;
    default:
      return ELF_T_NUM;
    }
}
