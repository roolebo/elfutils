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

#include <libebl_i386.h>


/* Return of the backend.  */
const char *
i386_backend_name (void)
{
  return "i386";
}


/* Relocation mapping table.  */
static struct
{
  const char *name;
  enum { both = 0, rel = 1, exec = 2 } appear;
} reloc_map_table[] =
  {
    [R_386_NONE] = { "R_386_NONE", both },
    [R_386_32] = { "R_386_32", both },
    [R_386_PC32] = { "R_386_PC32", rel },
    [R_386_GOT32] = { "R_386_GOT32", rel },
    [R_386_PLT32] = { "R_386_PLT32", rel },
    [R_386_COPY] = { "R_386_COPY", exec },
    [R_386_GLOB_DAT] = { "R_386_GLOB_DAT", exec },
    [R_386_JMP_SLOT] = { "R_386_JMP_SLOT", exec },
    [R_386_RELATIVE] = { "R_386_RELATIVE", exec },
    [R_386_GOTOFF] = { "R_386_GOTOFF", rel },
    [R_386_GOTPC] = { "R_386_GOTPC", rel },
    [R_386_32PLT] = { "R_386_32PLT", rel },
    [R_386_TLS_TPOFF] = { "R_386_TLS_TPOFF", rel },
    [R_386_TLS_IE] = { "R_386_TLS_IE", rel },
    [R_386_TLS_GOTIE] = { "R_386_TLS_GOTIE", rel },
    [R_386_TLS_LE] = { "R_386_TLS_LE", rel },
    [R_386_TLS_GD] = { "R_386_TLS_GD", rel },
    [R_386_TLS_LDM] = { "R_386_TLS_LDM", rel },
    [R_386_16] = { "R_386_16", rel },
    [R_386_PC16] = { "R_386_PC16", rel },
    [R_386_8] = { "R_386_8", rel },
    [R_386_PC8] = { "R_386_PC8", rel },
    [R_386_TLS_GD_32] = { "R_386_TLS_GD_32", rel },
    [R_386_TLS_GD_PUSH] = { "R_386_TLS_GD_PUSH", rel },
    [R_386_TLS_GD_CALL] = { "R_386_TLS_GD_CALL", rel },
    [R_386_TLS_GD_POP] = { "R_386_TLS_GD_POP", rel },
    [R_386_TLS_LDM_32] = { "R_386_TLS_LDM_32", rel },
    [R_386_TLS_LDM_PUSH] = { "R_386_TLS_LDM_PUSH", rel },
    [R_386_TLS_LDM_CALL] = { "R_386_TLS_LDM_CALL", rel },
    [R_386_TLS_LDM_POP] = { "R_386_TLS_LDM_POP", rel },
    [R_386_TLS_LDO_32] = { "R_386_TLS_LDO_32", rel },
    [R_386_TLS_IE_32] = { "R_386_TLS_IE_32", rel },
    [R_386_TLS_LE_32] = { "R_386_TLS_LE_32", rel },
    [R_386_TLS_DTPMOD32] = { "R_386_TLS_DTPMOD32", rel },
    [R_386_TLS_DTPOFF32] = { "R_386_TLS_DTPOFF32", rel },
    [R_386_TLS_TPOFF32] = { "R_386_TLS_TPOFF32", rel }
  };


/* Determine relocation type string for x86.  */
const char *
i386_reloc_type_name (int type, char *buf __attribute__ ((unused)),
		      size_t len __attribute__ ((unused)))
{
  if (type < 0 || type >= R_386_NUM)
    return NULL;

  return reloc_map_table[type].name;
}


/* Check for correct relocation type.  */
bool
i386_reloc_type_check (int type)
{
  return (type >= R_386_NONE && type < R_386_NUM
	  && reloc_map_table[type].name != NULL) ? true : false;
}


/* Check for correct relocation type use.  */
bool
i386_reloc_valid_use (Elf *elf, int type)
{
  if (type < R_386_NONE || type >= R_386_NUM
      || reloc_map_table[type].name == NULL)
    return false;

  Elf32_Ehdr *ehdr = elf32_getehdr (elf);
  assert (ehdr != NULL);

  if (reloc_map_table[type].appear == rel)
    return ehdr->e_type == ET_REL;

  if (reloc_map_table[type].appear == exec)
    return ehdr->e_type != ET_REL;

  assert (reloc_map_table[type].appear == both);
  return true;
}


/* Return true if the symbol type is that referencing the GOT.  */
bool
i386_gotpc_reloc_check (Elf *elf __attribute__ ((unused)), int type)
{
  return type == R_386_GOTPC;
}

/* Check for the simple reloc types.  */
Elf_Type
i386_reloc_simple_type (Elf *elf __attribute__ ((unused)), int type)
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

/* Check section name for being that of a debug informatino section.  */
bool (*generic_debugscn_p) (const char *);
bool
i386_debugscn_p (const char *name)
{
  return (generic_debugscn_p (name)
	  || strcmp (name, ".stab") == 0
	  || strcmp (name, ".stabstr") == 0);
}

/* Check whether given relocation is a copy relocation.  */
bool
i386_copy_reloc_p (int reloc)
{
  return reloc == R_386_COPY;
}
