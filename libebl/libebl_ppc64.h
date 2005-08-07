/* Interface for libebl_PPC64 module.
   Copyright (C) 2004, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBL_PPC64_H
#define _LIBEBL_PPC64_H 1

#include <libeblP.h>


/* Constructor.  */
extern const char *ppc64_init (Elf *elf, GElf_Half machine, Ebl *eh,
			       size_t ehlen);

/* Destructor.  */
extern void ppc64_destr (Ebl *bh);


/* Function to get relocation type name.  */
extern const char *ppc64_reloc_type_name (int type, char *buf, size_t len);

/* Check relocation type.  */
extern bool ppc64_reloc_type_check (int type);

/* Check relocation type use.  */
extern bool ppc64_reloc_valid_use (Elf *elf, int type);

/* Check for the simple reloc types.  */
extern Elf_Type ppc64_reloc_simple_type (Elf *elf, int type);

/* Code note handling.  */
extern bool ppc64_core_note (const char *name, uint32_t type, uint32_t descsz,
			      const char *desc);

/* Name of dynamic tag.  */
extern const char *ppc64_dynamic_tag_name (int64_t tag, char *buf, size_t len);

/* Check dynamic tag.  */
extern bool ppc64_dynamic_tag_check (int64_t tag);

/* Check whether given relocation is a copy relocation.  */
extern bool ppc64_copy_reloc_p (int reloc);

#endif	/* libebl_ppc.h */
