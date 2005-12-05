/* Register names and numbers for PowerPC DWARF.
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

#define BACKEND ppc_
#include "libebl_CPU.h"

ssize_t
ppc_register_name (Ebl *ebl __attribute__ ((unused)),
		   int regno, char *name, size_t namelen,
		   const char **prefix, const char **setname)
{
  if (name == NULL)
    return 1156;

  if (regno < 0 || regno > 1155 || namelen < 7)
    return -1;

  *prefix = NULL;

  if (regno < 32 || regno == 64 || regno == 66)
    *setname = "integer";
  else if (regno < 64 || regno == 65)
    *setname = "FPU";
  else if (regno < 1124)
    *setname = "privileged";
  else
    *setname = "vector";

  switch (regno)
    {
    case 0 ... 9:
      name[0] = 'r';
      name[1] = regno + '0';
      namelen = 2;
      break;

    case 10 ... 31:
      name[0] = 'r';
      name[1] = regno / 10 + '0';
      name[2] = regno % 10 + '0';
      namelen = 3;
      break;

    case 32 + 0 ... 32 + 9:
      name[0] = 'f';
      name[1] = (regno - 32) + '0';
      namelen = 2;
      break;

    case 32 + 10 ... 32 + 31:
      name[0] = 'f';
      name[1] = (regno - 32) / 10 + '0';
      name[2] = (regno - 32) % 10 + '0';
      namelen = 3;
      break;

    case 64:
      return stpcpy (name, "cr") - name;
    case 65:
      return stpcpy (name, "fpscr") - name;
    case 66:
      return stpcpy (name, "msr") - name;

    case 70 + 0 ... 70 + 9:
      name[0] = 's';
      name[1] = 'r';
      name[2] = (regno - 70) + '0';
      namelen = 3;
      break;

    case 70 + 10 ... 70 + 15:
      name[0] = 's';
      name[1] = 'r';
      name[2] = (regno - 70) / 10 + '0';
      name[3] = (regno - 70) % 10 + '0';
      namelen = 4;
      break;

    case 100 ... 109:
      name[0] = 's';
      name[1] = 'p';
      name[2] = 'r';
      name[3] = (regno - 100) + '0';
      namelen = 4;
      break;

    case 110 ... 199:
      name[0] = 's';
      name[1] = 'p';
      name[2] = 'r';
      name[3] = (regno - 100) / 10 + '0';
      name[4] = (regno - 100) % 10 + '0';
      namelen = 5;
      break;

    case 200 ... 999:
      name[0] = 's';
      name[1] = 'p';
      name[2] = 'r';
      name[3] = (regno - 100) / 100 + '0';
      name[4] = ((regno - 100) % 100 / 10) + '0';
      name[5] = (regno - 100) % 10 + '0';
      namelen = 6;
      break;

    case 1124 + 0 ... 1124 + 9:
      name[0] = 'v';
      name[1] = 'r';
      name[2] = (regno - 1124) + '0';
      namelen = 3;
      break;

    case 1124 + 10 ... 1124 + 31:
      name[0] = 'v';
      name[1] = 'r';
      name[2] = (regno - 1124) / 10 + '0';
      name[3] = (regno - 1124) % 10 + '0';
      namelen = 4;
      break;

    default:
      *setname = NULL;
      return 0;
    }

  name[namelen++] = '\0';
  return namelen;
}

__typeof (ppc_register_name)
     ppc64_register_name __attribute__ ((alias ("ppc_register_name")));
