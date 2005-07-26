/* Interface for libebl_i386 module.
   Copyright (C) 2000, 2001, 2002, 2003, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBL_I386_H
#define _LIBEBL_I386_H 1

#include <libeblP.h>


/* Constructor.  */
extern const char *i386_init (Elf *elf, GElf_Half machine, Ebl *eh,
			      size_t ehlen);

/* Destructor.  */
extern void i386_destr (Ebl *bh);


/* Function to get relocation type name.  */
extern const char *i386_reloc_type_name (int type, char *buf, size_t len);

/* Check relocation type.  */
extern bool i386_reloc_type_check (int type);

/* Check relocation type use.  */
extern bool i386_reloc_valid_use (Elf *elf, int type);

/* Check for the simple reloc types.  */
extern Elf_Type i386_reloc_simple_type (Elf *elf, int type);

/* Check relocation type use.  */
extern bool i386_gotpc_reloc_check (Elf *elf, int type);

/* Code note handling.  */
extern bool i386_core_note (const char *name, uint32_t type, uint32_t descsz,
			    const char *desc);

/* Check section name for being that of a debug informatino section.  */
extern bool i386_debugscn_p (const char *name);
extern bool (*generic_debugscn_p) (const char *);

/* Check whether given relocation is a copy relocation.  */
extern bool i386_copy_reloc_p (int reloc);

#endif	/* libebl_i386.h */
