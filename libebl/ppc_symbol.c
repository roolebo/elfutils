/* PPC specific symbolic name handling.
   Copyright (C) 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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

#include <libebl_ppc.h>


/* Return of the backend.  */
const char *
ppc_backend_name (void)
{
  return "ppc";
}


/* Relocation mapping table.  */
static struct
{
  const char *name;
  enum { both = 0, rel = 1, exec = 2 } appear;
} reloc_map_table[] =
  {
    // XXX Check all the appear values.
    [R_PPC_NONE] = { "R_PPC_NONE", both },
    [R_PPC_ADDR32] = { "R_PPC_ADDR32", both },
    [R_PPC_ADDR24] = { "R_PPC_ADDR24", both },
    [R_PPC_ADDR16] = { "R_PPC_ADDR16", both },
    [R_PPC_ADDR16_LO] = { "R_PPC_ADDR16_LO", both },
    [R_PPC_ADDR16_HI] = { "R_PPC_ADDR16_HI", both },
    [R_PPC_ADDR16_HA] = { "R_PPC_ADDR16_HA", both },
    [R_PPC_ADDR14] = { "R_PPC_ADDR14", exec },
    [R_PPC_ADDR14_BRTAKEN] = { "R_PPC_ADDR14_BRTAKEN", exec },
    [R_PPC_ADDR14_BRNTAKEN] = { "R_PPC_ADDR14_BRNTAKEN", exec },
    [R_PPC_REL24] = { "R_PPC_REL24", both },
    [R_PPC_REL14] = { "R_PPC_REL14", both },
    [R_PPC_REL14_BRTAKEN] = { "R_PPC_REL14_BRTAKEN", exec },
    [R_PPC_REL14_BRNTAKEN] = { "R_PPC_REL14_BRNTAKEN", exec },
    [R_PPC_GOT16] = { "R_PPC_GOT16", rel },
    [R_PPC_GOT16_LO] = { "R_PPC_GOT16_LO", rel },
    [R_PPC_GOT16_HI] = { "R_PPC_GOT16_HI", rel },
    [R_PPC_GOT16_HA] = { "R_PPC_GOT16_HA", rel },
    [R_PPC_PLTREL24] = { "R_PPC_PLTREL24", rel },
    [R_PPC_COPY] = { "R_PPC_COPY", exec },
    [R_PPC_GLOB_DAT] = { "R_PPC_GLOB_DAT", exec },
    [R_PPC_JMP_SLOT] = { "R_PPC_JMP_SLOT", exec },
    [R_PPC_RELATIVE] = { "R_PPC_RELATIVE", exec },
    [R_PPC_LOCAL24PC] = { "R_PPC_LOCAL24PC", rel },
    [R_PPC_UADDR32] = { "R_PPC_UADDR32", exec },
    [R_PPC_UADDR16] = { "R_PPC_UADDR16", exec },
    [R_PPC_REL32] = { "R_PPC_REL32", exec },
    [R_PPC_PLT32] = { "R_PPC_PLT32", exec },
    [R_PPC_PLTREL32] = { "R_PPC_PLTREL32", both },
    [R_PPC_PLT16_LO] = { "R_PPC_PLT16_LO", both },
    [R_PPC_PLT16_HI] = { "R_PPC_PLT16_HI", both },
    [R_PPC_PLT16_HA] = { "R_PPC_PLT16_HA", both },
    [R_PPC_SDAREL16] = { "R_PPC_SDAREL16", both },
    [R_PPC_SECTOFF] = { "R_PPC_SECTOFF", both },
    [R_PPC_SECTOFF_LO] = { "R_PPC_SECTOFF_LO", both },
    [R_PPC_SECTOFF_HI] = { "R_PPC_SECTOFF_HI", both },
    [R_PPC_SECTOFF_HA] = { "R_PPC_SECTOFF_HA", both },
    [R_PPC_TLS] = { "R_PPC_TLS", both },
    [R_PPC_DTPMOD32] = { "R_PPC_DTPMOD32", exec },
    [R_PPC_TPREL16] = { "R_PPC_TPREL16", rel },
    [R_PPC_TPREL16_LO] = { "R_PPC_TPREL16_LO", rel },
    [R_PPC_TPREL16_HI] = { "R_PPC_TPREL16_HI", rel },
    [R_PPC_TPREL16_HA] = { "R_PPC_TPREL16_HA", rel },
    [R_PPC_TPREL32] = { "R_PPC_TPREL32", exec },
    [R_PPC_DTPREL16] = { "R_PPC_DTPREL16", rel },
    [R_PPC_DTPREL16_LO] = { "R_PPC_DTPREL16_LO", rel },
    [R_PPC_DTPREL16_HI] = { "R_PPC_DTPREL16_HI", rel },
    [R_PPC_DTPREL16_HA] = { "R_PPC_DTPREL16_HA", rel },
    [R_PPC_DTPREL32] = { "R_PPC_DTPREL32", exec },
    [R_PPC_GOT_TLSGD16] = { "R_PPC_GOT_TLSGD16", exec },
    [R_PPC_GOT_TLSGD16_LO] = { "R_PPC_GOT_TLSGD16_LO", exec },
    [R_PPC_GOT_TLSGD16_HI] = { "R_PPC_GOT_TLSGD16_HI", exec },
    [R_PPC_GOT_TLSGD16_HA] = { "R_PPC_GOT_TLSGD16_HA", exec },
    [R_PPC_GOT_TLSLD16] = { "R_PPC_GOT_TLSLD16", exec },
    [R_PPC_GOT_TLSLD16_LO] = { "R_PPC_GOT_TLSLD16_LO", exec },
    [R_PPC_GOT_TLSLD16_HI] = { "R_PPC_GOT_TLSLD16_HI", exec },
    [R_PPC_GOT_TLSLD16_HA] = { "R_PPC_GOT_TLSLD16_HA", exec },
    [R_PPC_GOT_TPREL16] = { "R_PPC_GOT_TPREL16", exec },
    [R_PPC_GOT_TPREL16_LO] = { "R_PPC_GOT_TPREL16_LO", exec },
    [R_PPC_GOT_TPREL16_HI] = { "R_PPC_GOT_TPREL16_HI", exec },
    [R_PPC_GOT_TPREL16_HA] = { "R_PPC_GOT_TPREL16_HA", exec },
    [R_PPC_GOT_DTPREL16] = { "R_PPC_GOT_DTPREL16", exec },
    [R_PPC_GOT_DTPREL16_LO] = { "R_PPC_GOT_DTPREL16_LO", exec },
    [R_PPC_GOT_DTPREL16_HI] = { "R_PPC_GOT_DTPREL16_HI", exec },
    [R_PPC_GOT_DTPREL16_HA] = { "R_PPC_GOT_DTPREL16_HA", exec }
  };


/* Determine relocation type string for PPC.  */
const char *
ppc_reloc_type_name (int type, char *buf __attribute__ ((unused)),
		     size_t len __attribute__ ((unused)))
{
  if (type < 0 || type >= R_PPC_NUM)
    return NULL;

  return reloc_map_table[type].name;
}


/* Check for correct relocation type.  */
bool
ppc_reloc_type_check (int type)
{
  return (type >= R_PPC_NONE && type < R_PPC_NUM
	  && reloc_map_table[type].name != NULL) ? true : false;
}


/* Check for correct relocation type use.  */
bool
ppc_reloc_valid_use (Elf *elf, int type)
{
  if (type < R_PPC_NONE || type >= R_PPC_NUM
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


/* Check for the simple reloc types.  */
Elf_Type
ppc_reloc_simple_type (Elf *elf __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_PPC_ADDR32:
    case R_PPC_UADDR32:
      return ELF_T_WORD;
    case R_PPC_UADDR16:
      return ELF_T_HALF;
    default:
      return ELF_T_NUM;
    }
}

/* Check whether given relocation is a copy relocation.  */
bool
ppc_copy_reloc_p (int reloc)
{
  return reloc == R_PPC_COPY;
}
