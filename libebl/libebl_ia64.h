/* Interface for libebl_ia64 module.
   Copyright (C) 2002, 2003, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBL_IA64_H
#define _LIBEBL_IA64_H 1

#include <libeblP.h>


/* Constructor.  */
extern const char *ia64_init (Elf *elf, GElf_Half machine, Ebl *eh,
			      size_t ehlen);

/* Destructor.  */
extern void ia64_destr (Ebl *bh);


/* Function to get relocation type name.  */
extern const char *ia64_reloc_type_name (int type, char *buf, size_t len);

/* Check relocation type.  */
extern bool ia64_reloc_type_check (int type);

/* Name of segment type.  */
extern const char *ia64_segment_type_name (int segment, char *buf, size_t len);

/* Name of dynamic tag.  */
extern const char *ia64_dynamic_tag_name (int64_t tag, char *buf, size_t len);

/* Check whether given relocation is a copy relocation.  */
extern bool ia64_copy_reloc_p (int reloc);

/* Check whether machine flags are valid.  */
extern bool ia64_machine_flag_check (GElf_Word flags);


#endif	/* libebl_ia64.h */
