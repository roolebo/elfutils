/* Register names and numbers for SPARC DWARF.
   Copyright (C) 2005 Red Hat, Inc.

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

#include <string.h>

#define BACKEND sparc_
#include "libebl_CPU.h"

ssize_t
sparc_register_name (Ebl *ebl,
		     int regno, char *name, size_t namelen,
		     const char **prefix, const char **setname)
{
  const int nfp = ebl->machine == EM_SPARCV9 ? 64 : 32;

  if (name == NULL)
    return 32 + nfp;

  if (regno < 0 || regno >= 32 + nfp || namelen < 4)
    return -1;

  *prefix = "%";

  if (regno < 32)
    {
      *setname = "integer";
      name[0] = "goli"[regno >> 3];
      name[1] = (regno & 7) + '0';
      namelen = 2;
    }
  else
    {
      *setname = "FPU";
      name[0] = 'f';
      if (regno < 32 + 10)
	{
	  name[1] = (regno - 32) + '0';
	  namelen = 2;
	}
      else
	{
	  name[1] = (regno - 32) / 10 + '0';
	  name[2] = (regno - 32) % 10 + '0';
	  namelen = 3;
	}
    }

  name[namelen++] = '\0';
  return namelen;
}
