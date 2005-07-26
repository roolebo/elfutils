/* Interface for libebl_x86_64 module.
   Copyright (C) 2002, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBL_X86_64_H
#define _LIBEBL_X86_64_H 1

#include <libeblP.h>


/* Constructor.  */
extern const char *x86_64_init (Elf *elf, GElf_Half machine, Ebl *eh,
				size_t ehlen);

/* Destructor.  */
extern void x86_64_destr (Ebl *bh);


/* Function to get relocation type name.  */
extern const char *x86_64_reloc_type_name (int type, char *buf, size_t len);

/* Check relocation type.  */
extern bool x86_64_reloc_type_check (int type);

/* Check relocation type use.  */
extern bool x86_64_reloc_valid_use (Elf *elf, int type);

/* Check for the simple reloc types.  */
extern Elf_Type x86_64_reloc_simple_type (Elf *elf, int type);

/* Code note handling.  */
extern bool x86_64_core_note (const char *name, uint32_t type, uint32_t descsz,
			      const char *desc);

/* Check whether given relocation is a copy relocation.  */
extern bool x86_64_copy_reloc_p (int reloc);

#endif	/* libebl_x86_64.h */
