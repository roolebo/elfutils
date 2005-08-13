/* PPC64 specific symbolic name handling.
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
#include <string.h>

#include <libebl_ppc64.h>


/* Return of the backend.  */
const char *
ppc64_backend_name (void)
{
  return "ppc64";
}


/* Relocation mapping table.  */
static struct
{
  const char *name;
  enum { both = 0, rel = 1, exec = 2 } appear;
} reloc_map_table[] =
  {
    // XXX Check all the appear values.
    [R_PPC64_NONE] = { "R_PPC64_NONE", both },
    [R_PPC64_ADDR32] = { "R_PPC64_ADDR32", both },
    [R_PPC64_ADDR24] = { "R_PPC64_ADDR24", both },
    [R_PPC64_ADDR16] = { "R_PPC64_ADDR16", both },
    [R_PPC64_ADDR16_LO] = { "R_PPC64_ADDR16_LO", both },
    [R_PPC64_ADDR16_HI] = { "R_PPC64_ADDR16_HI", both },
    [R_PPC64_ADDR16_HA] = { "R_PPC64_ADDR16_HA", both },
    [R_PPC64_ADDR14] = { "R_PPC64_ADDR14", both },
    [R_PPC64_ADDR14_BRTAKEN] = { "R_PPC64_ADDR14_BRTAKEN", exec },
    [R_PPC64_ADDR14_BRNTAKEN] = { "R_PPC64_ADDR14_BRNTAKEN", exec },
    [R_PPC64_REL24] = { "R_PPC64_REL24", both },
    [R_PPC64_REL14] = { "R_PPC64_REL14", both },
    [R_PPC64_REL14_BRTAKEN] = { "R_PPC64_REL14_BRTAKEN", exec },
    [R_PPC64_REL14_BRNTAKEN] = { "R_PPC64_REL14_BRNTAKEN", exec },
    [R_PPC64_GOT16] = { "R_PPC64_GOT16", rel },
    [R_PPC64_GOT16_LO] = { "R_PPC64_GOT16_LO", rel },
    [R_PPC64_GOT16_HI] = { "R_PPC64_GOT16_HI", rel },
    [R_PPC64_GOT16_HA] = { "R_PPC64_GOT16_HA", rel },
    [R_PPC64_COPY] = { "R_PPC64_COPY", exec },
    [R_PPC64_GLOB_DAT] = { "R_PPC64_GLOB_DAT", exec },
    [R_PPC64_JMP_SLOT] = { "R_PPC64_JMP_SLOT", exec },
    [R_PPC64_RELATIVE] = { "R_PPC64_RELATIVE", exec },
    [R_PPC64_UADDR32] = { "R_PPC64_UADDR32", exec },
    [R_PPC64_UADDR16] = { "R_PPC64_UADDR16", exec },
    [R_PPC64_REL32] = { "R_PPC64_REL32", exec },
    [R_PPC64_PLT32] = { "R_PPC64_PLT32", exec },
    [R_PPC64_PLTREL32] = { "R_PPC64_PLTREL32", both },
    [R_PPC64_PLT16_LO] = { "R_PPC64_PLT16_LO", both },
    [R_PPC64_PLT16_HI] = { "R_PPC64_PLT16_HI", both },
    [R_PPC64_PLT16_HA] = { "R_PPC64_PLT16_HA", both },
    [R_PPC64_SECTOFF] = { "R_PPC64_SECTOFF", both },
    [R_PPC64_SECTOFF_LO] = { "R_PPC64_SECTOFF_LO", both },
    [R_PPC64_SECTOFF_HI] = { "R_PPC64_SECTOFF_HI", both },
    [R_PPC64_SECTOFF_HA] = { "R_PPC64_SECTOFF_HA", both },
    [R_PPC64_ADDR30] = { "R_PPC64_ADDR30", both },
    [R_PPC64_ADDR64] = { "R_PPC64_ADDR64", both },
    [R_PPC64_ADDR16_HIGHER] = { "R_PPC64_ADDR16_HIGHER", both },
    [R_PPC64_ADDR16_HIGHERA] = { "R_PPC64_ADDR16_HIGHERA", both },
    [R_PPC64_ADDR16_HIGHEST] = { "R_PPC64_ADDR16_HIGHEST", both },
    [R_PPC64_ADDR16_HIGHESTA] = { "R_PPC64_ADDR16_HIGHESTA", both },
    [R_PPC64_UADDR64] = { "R_PPC64_UADDR64", both },
    [R_PPC64_REL64] = { "R_PPC64_REL64", both },
    [R_PPC64_PLT64] = { "R_PPC64_PLT64", both },
    [R_PPC64_PLTREL64] = { "R_PPC64_PLTREL64", both },
    [R_PPC64_TOC16] = { "R_PPC64_TOC16", both },
    [R_PPC64_TOC16_LO] = { "R_PPC64_TOC16_LO", both },
    [R_PPC64_TOC16_HI] = { "R_PPC64_TOC16_HI", both },
    [R_PPC64_TOC16_HA] = { "R_PPC64_TOC16_HA", both },
    [R_PPC64_TOC] = { "R_PPC64_TOC", both },
    [R_PPC64_PLTGOT16] = { "R_PPC64_PLTGOT16", both },
    [R_PPC64_PLTGOT16_LO] = { "R_PPC64_PLTGOT16_LO", both },
    [R_PPC64_PLTGOT16_HI] = { "R_PPC64_PLTGOT16_HI", both },
    [R_PPC64_PLTGOT16_HA] = { "R_PPC64_PLTGOT16_HA", both },
    [R_PPC64_ADDR16_DS] = { "R_PPC64_ADDR16_DS", both },
    [R_PPC64_ADDR16_LO_DS] = { "R_PPC64_ADDR16_LO_DS", both },
    [R_PPC64_GOT16_DS] = { "R_PPC64_GOT16_DS", both },
    [R_PPC64_GOT16_LO_DS] = { "R_PPC64_GOT16_LO_DS", both },
    [R_PPC64_PLT16_LO_DS] = { "R_PPC64_PLT16_LO_DS", both },
    [R_PPC64_SECTOFF_DS] = { "R_PPC64_SECTOFF_DS", both },
    [R_PPC64_SECTOFF_LO_DS] = { "R_PPC64_SECTOFF_LO_DS", both },
    [R_PPC64_TOC16_DS] = { "R_PPC64_TOC16_DS", both },
    [R_PPC64_TOC16_LO_DS] = { "R_PPC64_TOC16_LO_DS", both },
    [R_PPC64_PLTGOT16_DS] = { "R_PPC64_PLTGOT16_DS", both },
    [R_PPC64_PLTGOT16_LO_DS] = { "R_PPC64_PLTGOT16_LO_DS", both },
    [R_PPC64_TLS] = { "R_PPC64_TLS", both },
    [R_PPC64_DTPMOD64] = { "R_PPC64_DTPMOD64", both },
    [R_PPC64_TPREL16] = { "R_PPC64_TPREL16", both },
    [R_PPC64_TPREL16_LO] = { "R_PPC64_TPREL16_LO", both },
    [R_PPC64_TPREL16_HI] = { "R_PPC64_TPREL16_HI", both },
    [R_PPC64_TPREL16_HA] = { "R_PPC64_TPREL16_HA", both },
    [R_PPC64_TPREL64] = { "R_PPC64_TPREL64", both },
    [R_PPC64_DTPREL16] = { "R_PPC64_DTPREL16", both },
    [R_PPC64_DTPREL16_LO] = { "R_PPC64_DTPREL16_LO", both },
    [R_PPC64_DTPREL16_HI] = { "R_PPC64_DTPREL16_HI", both },
    [R_PPC64_DTPREL16_HA] = { "R_PPC64_DTPREL16_HA", both },
    [R_PPC64_DTPREL64] = { "R_PPC64_DTPREL64", both },
    [R_PPC64_GOT_TLSGD16] = { "R_PPC64_GOT_TLSGD16", both },
    [R_PPC64_GOT_TLSGD16_LO] = { "R_PPC64_GOT_TLSGD16_LO", both },
    [R_PPC64_GOT_TLSGD16_HI] = { "R_PPC64_GOT_TLSGD16_HI", both },
    [R_PPC64_GOT_TLSGD16_HA] = { "R_PPC64_GOT_TLSGD16_HA", both },
    [R_PPC64_GOT_TLSLD16] = { "R_PPC64_GOT_TLSLD16", both },
    [R_PPC64_GOT_TLSLD16_LO] = { "R_PPC64_GOT_TLSLD16_LO", both },
    [R_PPC64_GOT_TLSLD16_HI] = { "R_PPC64_GOT_TLSLD16_HI", both },
    [R_PPC64_GOT_TLSLD16_HA] = { "R_PPC64_GOT_TLSLD16_HA", both },
    [R_PPC64_GOT_TPREL16_DS] = { "R_PPC64_GOT_TPREL16_DS", both },
    [R_PPC64_GOT_TPREL16_LO_DS] = { "R_PPC64_GOT_TPREL16_LO_DS", both },
    [R_PPC64_GOT_TPREL16_HI] = { "R_PPC64_GOT_TPREL16_HI", both },
    [R_PPC64_GOT_TPREL16_HA] = { "R_PPC64_GOT_TPREL16_HA", both },
    [R_PPC64_GOT_DTPREL16_DS] = { "R_PPC64_GOT_DTPREL16_DS", both },
    [R_PPC64_GOT_DTPREL16_LO_DS] = { "R_PPC64_GOT_DTPREL16_LO_DS", both },
    [R_PPC64_GOT_DTPREL16_HI] = { "R_PPC64_GOT_DTPREL16_HI", both },
    [R_PPC64_GOT_DTPREL16_HA] = { "R_PPC64_GOT_DTPREL16_HA", both },
    [R_PPC64_TPREL16_DS] = { "R_PPC64_TPREL16_DS", both },
    [R_PPC64_TPREL16_LO_DS] = { "R_PPC64_TPREL16_LO_DS", both },
    [R_PPC64_TPREL16_HIGHER] = { "R_PPC64_TPREL16_HIGHER", both },
    [R_PPC64_TPREL16_HIGHERA] = { "R_PPC64_TPREL16_HIGHERA", both },
    [R_PPC64_TPREL16_HIGHEST] = { "R_PPC64_TPREL16_HIGHEST", both },
    [R_PPC64_TPREL16_HIGHESTA] = { "R_PPC64_TPREL16_HIGHESTA", both },
    [R_PPC64_DTPREL16_DS] = { "R_PPC64_DTPREL16_DS", both },
    [R_PPC64_DTPREL16_LO_DS] = { "R_PPC64_DTPREL16_LO_DS", both },
    [R_PPC64_DTPREL16_HIGHER] = { "R_PPC64_DTPREL16_HIGHER", both },
    [R_PPC64_DTPREL16_HIGHERA] = { "R_PPC64_DTPREL16_HIGHERA", both },
    [R_PPC64_DTPREL16_HIGHEST] = { "R_PPC64_DTPREL16_HIGHEST", both },
    [R_PPC64_DTPREL16_HIGHESTA] = { "R_PPC64_DTPREL16_HIGHESTA", both }
 };


/* Determine relocation type string for PPC.  */
const char *
ppc64_reloc_type_name (int type, char *buf __attribute__ ((unused)),
		       size_t len __attribute__ ((unused)))
{
  if (type < R_PPC64_NONE || type >= R_PPC64_NUM)
    return NULL;

  return reloc_map_table[type].name;
}


/* Check for correct relocation type.  */
bool
ppc64_reloc_type_check (int type)
{
  return (type >= R_PPC64_NONE && type < R_PPC64_NUM
	  && reloc_map_table[type].name != NULL) ? true : false;
}


/* Check for correct relocation type use.  */
bool
ppc64_reloc_valid_use (Elf *elf, int type)
{
  if (type < R_PPC64_NONE || type >= R_PPC64_NUM
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
ppc64_reloc_simple_type (Elf *elf __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_PPC64_ADDR64:
    case R_PPC64_UADDR64:
      return ELF_T_XWORD;
    case R_PPC64_ADDR32:
    case R_PPC64_UADDR32:
      return ELF_T_WORD;
    case R_PPC64_UADDR16:
      return ELF_T_HALF;
    default:
      return ELF_T_NUM;
    }
}


const char *
ppc64_dynamic_tag_name (int64_t tag, char *buf __attribute__ ((unused)),
			size_t len __attribute__ ((unused)))
{
  switch (tag)
    {
    case DT_PPC64_GLINK:
      return "PPC64_GLINK";
    case DT_PPC64_OPD:
      return "PPC64_OPD";
    case DT_PPC64_OPDSZ:
      return "PPC64_OPDSZ";
    default:
      break;
    }
  return NULL;
}

bool
ppc64_dynamic_tag_check (int64_t tag)
{
  return (tag == DT_PPC64_GLINK
	  || tag == DT_PPC64_OPD
	  || tag == DT_PPC64_OPDSZ);
}


/* Check whether given relocation is a copy relocation.  */
bool
ppc64_copy_reloc_p (int reloc)
{
  return reloc == R_PPC64_COPY;
}


/* Check whether given symbol's st_size is ok despite normal check failing.  */
bool
ppc64_check_special_symbol (Elf *elf,
			    const GElf_Sym *sym __attribute__ ((unused)),
			    const char *name __attribute__ ((unused)),
			    const GElf_Shdr *destshdr)
{
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
  if (ehdr == NULL)
    return false;
  const char *sname = elf_strptr (elf, ehdr->e_shstrndx, destshdr->sh_name);
  if (sname == NULL)
    return false;
  return !strcmp (sname, ".opd");
}

/* Check if backend uses a bss PLT in this file.  */
bool
ppc64_bss_plt_p (Elf *elf __attribute__ ((unused)))
{
  return true;
}
