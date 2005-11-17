/* IA-64 specific symbolic name handling.
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

#include <elf.h>
#include <stddef.h>
#include <assert.h>

#define BACKEND		ia64_
#include "libebl_CPU.h"


const char *
ia64_segment_type_name (int segment, char *buf __attribute__ ((unused)),
			size_t len __attribute__ ((unused)))
{
  switch (segment)
    {
    case PT_IA_64_ARCHEXT:
      return "IA_64_ARCHEXT";
    case PT_IA_64_UNWIND:
      return "IA_64_UNWIND";
    case PT_IA_64_HP_OPT_ANOT:
      return "IA_64_HP_OPT_ANOT";
    case PT_IA_64_HP_HSL_ANOT:
      return "IA_64_HP_HSL_ANOT";
    case PT_IA_64_HP_STACK:
      return "IA_64_HP_STACK";
    default:
      break;
    }
  return NULL;
}

const char *
ia64_dynamic_tag_name (int64_t tag, char *buf __attribute__ ((unused)),
		       size_t len __attribute__ ((unused)))
{
  switch (tag)
    {
    case DT_IA_64_PLT_RESERVE:
      return "IA_64_PLT_RESERVE";
    default:
      break;
    }
  return NULL;
}

/* Check dynamic tag.  */
bool
ia64_dynamic_tag_check (int64_t tag)
{
  return tag == DT_IA_64_PLT_RESERVE;
}

/* Check whether machine flags are valid.  */
bool
ia64_machine_flag_check (GElf_Word flags)
{
  return ((flags &~ EF_IA_64_ABI64) == 0);
}

/* Return symbolic representation of section type.  */
const char *
ia64_section_type_name (int type,
			char *buf __attribute__ ((unused)),
			size_t len __attribute__ ((unused)))
{
  switch (type)
    {
    case SHT_IA_64_EXT:
      return "SHT_IA_64_EXT";
    case SHT_IA_64_UNWIND:
      return "HT_IA_64_UNWIND";
    }

  return NULL;
}

/* Check for the simple reloc types.  */
Elf_Type
ia64_reloc_simple_type (Ebl *ebl, int type)
{
  switch (type)
    {
    case R_IA64_DIR32MSB:
      if (ebl->data == ELFDATA2MSB)
	return ELF_T_WORD;
      break;
    case R_IA64_DIR32LSB:
      if (ebl->data == ELFDATA2LSB)
	return ELF_T_WORD;
      break;
    case R_IA64_DIR64MSB:
      if (ebl->data == ELFDATA2MSB)
	return ELF_T_XWORD;
      break;
    case R_IA64_DIR64LSB:
      if (ebl->data == ELFDATA2LSB)
	return ELF_T_XWORD;
      break;
    }

  return ELF_T_NUM;
}
