/* Common interface for libebl modules.
   Copyright (C) 2000, 2001, 2002, 2003, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBL_CPU_H
#define _LIBEBL_CPU_H 1

#include <libeblP.h>

#define EBLHOOK(name)	EBLHOOK_1(BACKEND, name)
#define EBLHOOK_1(a, b)	EBLHOOK_2(a, b)
#define EBLHOOK_2(a, b)	a##b

/* Constructor.  */
extern const char *EBLHOOK(init) (Elf *elf, GElf_Half machine,
				  Ebl *eh, size_t ehlen);

#include "ebl-hooks.h"

#define HOOK(eh, name)	eh->name = EBLHOOK(name)

extern bool (*generic_debugscn_p) (const char *) attribute_hidden;


#endif	/* libebl_CPU.h */
