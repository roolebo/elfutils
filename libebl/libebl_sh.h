/* Interface for libebl_sh module.
   Copyright (C) 2000, 2001, 2002, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBL_SH_H
#define _LIBEBL_SH_H 1

#include <libeblP.h>


/* Constructor.  */
extern const char *sh_init (Elf *elf, GElf_Half machine, Ebl *eh,
			    size_t ehlen);

/* Destructor.  */
extern void sh_destr (Ebl *bh);


/* Function to get relocation type name.  */
extern const char *sh_reloc_type_name (int type, char *buf, size_t len);

/* Check whether given relocation is a copy relocation.  */
extern bool sh_copy_reloc_p (int reloc);

#endif	/* libebl_sh.h */
