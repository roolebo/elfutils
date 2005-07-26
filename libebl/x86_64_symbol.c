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

#include <libebl_x86_64.h>


/* Return of the backend.  */
const char *
x86_64_backend_name (void)
{
  return "x86-64";
}


/* Relocation mapping table.  */
static struct
{
  const char *name;
  enum { both = 0, rel = 1, exec = 2 } appear;
} reloc_map_table[] =
  {
    [R_X86_64_NONE] = { "R_X86_64_NONE", both },
    [R_X86_64_64] = { "R_X86_64_64", both },
    [R_X86_64_PC32] = { "R_X86_64_PC32", rel },
    [R_X86_64_GOT32] = { "R_X86_64_GOT32", rel },
    [R_X86_64_PLT32] = { "R_X86_64_PLT32", rel },
    [R_X86_64_COPY] = { "R_X86_64_COPY", exec },
    [R_X86_64_GLOB_DAT] = { "R_X86_64_GLOB_DAT", exec },
    [R_X86_64_JUMP_SLOT] = { "R_X86_64_JUMP_SLOT", exec },
    [R_X86_64_RELATIVE] = { "R_X86_64_RELATIVE", exec },
    [R_X86_64_GOTPCREL] = { "R_X86_64_GOTPCREL", exec },
    [R_X86_64_32] = { "R_X86_64_32", both },
    [R_X86_64_32S] = { "R_X86_64_32S", rel },
    [R_X86_64_16] = { "R_X86_64_16", rel },
    [R_X86_64_PC16] = { "R_X86_64_PC16", rel },
    [R_X86_64_8] = { "R_X86_64_8", rel },
    [R_X86_64_PC8] = { "R_X86_64_PC8", rel },
    [R_X86_64_DTPMOD64] = { "R_X86_64_DTPMOD64", rel },
    [R_X86_64_DTPOFF64] = { "R_X86_64_DTPOFF64", rel },
    [R_X86_64_TPOFF64] = { "R_X86_64_TPOFF64", rel },
    [R_X86_64_TLSGD] = { "R_X86_64_TLSGD", rel },
    [R_X86_64_TLSLD] = { "R_X86_64_TLSLD", rel },
    [R_X86_64_DTPOFF32] = { "R_X86_64_DTPOFF32", rel },
    [R_X86_64_GOTTPOFF] = { "R_X86_64_GOTTPOFF", rel },
    [R_X86_64_TPOFF32] = { "R_X86_64_TPOFF32", rel }
  };


/* Determine relocation type string for x86-64.  */
const char *
x86_64_reloc_type_name (int type, char *buf __attribute__ ((unused)),
			size_t len __attribute__ ((unused)))
{
  if (type < 0 || type >= R_X86_64_NUM)
    return NULL;

  return reloc_map_table[type].name;
}


/* Check for correct relocation type.  */
bool
x86_64_reloc_type_check (int type)
{
  return (type >= R_X86_64_NONE && type < R_X86_64_NUM
	  && reloc_map_table[type].name != NULL) ? true : false;
}


/* Check for correct relocation type use.  */
bool
x86_64_reloc_valid_use (Elf *elf, int type)
{
  if (type < R_X86_64_NONE || type >= R_X86_64_NUM
      || reloc_map_table[type].name == NULL)
    return false;

  Elf64_Ehdr *ehdr = elf64_getehdr (elf);
  assert (ehdr != NULL);

  if (reloc_map_table[type].appear == rel)
    return ehdr->e_type == ET_REL;

  if (reloc_map_table[type].appear == exec)
    return ehdr->e_type != ET_REL;

  assert (reloc_map_table[type].appear == both);
  return true;
}

/* Check for the simple reloc types.  */
Elf_Type
x86_64_reloc_simple_type (Elf *elf __attribute__ ((unused)), int type)
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

/* Check whether given relocation is a copy relocation.  */
bool
x86_64_copy_reloc_p (int reloc)
{
  return reloc == R_X86_64_COPY;
}
