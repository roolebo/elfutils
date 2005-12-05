/* Register names and numbers for x86-64 DWARF.
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

#include <assert.h>
#include <dwarf.h>

#define BACKEND x86_64_
#include "libebl_CPU.h"

ssize_t
x86_64_register_name (Ebl *ebl __attribute__ ((unused)),
		      int regno, char *name, size_t namelen,
		      const char **prefix, const char **setname)
{
  if (name == NULL)
    return 49;

  if (regno < 0 || regno > 48 || namelen < 6)
    return -1;

  *prefix = "%";
  if (regno < 17)
    *setname = "integer";
  else if (regno < 33)
    *setname = "SSE";
  else if (regno < 41)
    *setname = "x87";
  else
    *setname = "MMX";

  switch (regno)
    {
      static const char baseregs[][2] =
	{
	  "ax", "dx", "cx", "bx", "si", "di", "bp", "sp"
	};

    case 0 ... 7:
      name[0] = 'r';
      name[1] = baseregs[regno][0];
      name[2] = baseregs[regno][1];
      namelen = 3;
      break;

    case 8 ... 9:
      name[0] = 'r';
      name[1] = regno - 8 + '8';
      namelen = 2;
      break;

    case 10 ... 15:
      name[0] = 'r';
      name[1] = '1';
      name[2] = regno - 10 + '0';
      namelen = 3;
      break;

    case 16:
      name[0] = 'r';
      name[1] = 'i';
      name[2] = 'p';
      namelen = 3;
      break;

    case 17 ... 26:
      name[0] = 'x';
      name[1] = 'm';
      name[2] = 'm';
      name[3] = regno - 17 + '0';
      namelen = 4;
      break;

    case 27 ... 32:
      name[0] = 'x';
      name[1] = 'm';
      name[2] = 'm';
      name[3] = '1';
      name[4] = regno - 27 + '0';
      namelen = 5;
      break;

    case 33 ... 40:
      name[0] = 's';
      name[1] = 't';
      name[2] = regno - 33 + '0';
      namelen = 3;
      break;

    case 41 ... 48:
      name[0] = 'm';
      name[1] = 'm';
      name[2] = regno - 41 + '0';
      namelen = 3;
      break;
    }

  name[namelen++] = '\0';
  return namelen;
}
