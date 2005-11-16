/* Internal definitions for interface for libebl.
   Copyright (C) 2000, 2001, 2002, 2004, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBLP_H
#define _LIBEBLP_H 1

#include <gelf.h>
#include <libebl.h>
#include <libintl.h>


/* Backend handle.  */
struct ebl
{
  /* Machine name.  */
  const char *name;

  /* Emulation name.  */
  const char *emulation;

  /* ELF machine, class, and data encoding.  */
  uint_fast16_t machine;
  uint_fast8_t class;
  uint_fast8_t data;

  /* The libelf handle (if known).  */
  Elf *elf;

  /* See ebl-hooks.h for the declarations of the hook functions.  */
# define EBLHOOK(name) (*name)
# include "ebl-hooks.h"
# undef EBLHOOK

  /* Internal data.  */
  void *dlhandle;
};


/* Type of the initialization functions in the backend modules.  */
typedef const char *(*ebl_bhinit_t) (Elf *, GElf_Half, Ebl *, size_t);


/* gettext helper macros.  */
#define _(Str) dgettext ("elfutils", Str)

#endif	/* libeblP.h */
