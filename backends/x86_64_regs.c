/* Register names and numbers for x86-64 DWARF.
   Copyright (C) 2005, 2006 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <dwarf.h>

#define BACKEND x86_64_
#include "libebl_CPU.h"

ssize_t
x86_64_register_info (Ebl *ebl __attribute__ ((unused)),
		      int regno, char *name, size_t namelen,
		      const char **prefix, const char **setname,
		      int *bits, int *type)
{
  if (name == NULL)
    return 49;

  if (regno < 0 || regno > 48 || namelen < 6)
    return -1;

  *prefix = "%";
  *bits = 64;
  *type = DW_ATE_unsigned;
  if (regno < 17)
    {
      *setname = "integer";
      if (regno == 16 || regno == 6 || regno == 7)
	*type = DW_ATE_address;
      else
	*type = DW_ATE_signed;
    }
  else if (regno < 33)
    {
      *setname = "SSE";
      *bits = 128;
    }
  else if (regno < 41)
    {
      *setname = "x87";
      *type = DW_ATE_float;
      *bits = 80;
    }
  else
    {
      *setname = "MMX";
      *bits = 64;
    }

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
