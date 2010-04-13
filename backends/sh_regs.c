/* Register names and numbers for SH DWARF.
   Copyright (C) 2010 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Contributed by Matt Fleming <matt@console-pimps.org>.

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
#include <string.h>

#define BACKEND sh_
#include "libebl_CPU.h"

ssize_t
sh_register_info (Ebl *ebl __attribute__ ((unused)),
		  int regno, char *name, size_t namelen,
		  const char **prefix, const char **setname,
		  int *bits, int *type)
{
  if (name == NULL)
    return 104;

  if (regno < 0 || regno > 103 || namelen < 6)
    return -1;

  *prefix = NULL;
  *bits = 32;
  *type = DW_ATE_signed;

  switch (regno)
    {
    case 0 ... 9:
      *setname = "integer";
      name[0] = 'r';
      name[1] = regno + '0';
      namelen = 2;
      break;

    case 10 ... 15:
      *setname = "integer";
      name[0] = 'r';
      name[1] = '1';
      name[2] = regno - 10 + '0';
      namelen = 3;
      break;

    case 16:
      *setname = "system";
      *type = DW_ATE_address;
      name[0] = 'p';
      name[1] = 'c';
      namelen = 2;
      break;

    case 17:
      *setname = "system";
      *type = DW_ATE_address;
      name[0] = 'p';
      name[1] = 'r';
      namelen = 2;
      break;

    case 18:
      *setname = "control";
      *type = DW_ATE_unsigned;
      name[0] = 's';
      name[1] = 'r';
      namelen = 2;
      break;

    case 19:
      *setname = "control";
      *type = DW_ATE_unsigned;
      name[0] = 'g';
      name[1] = 'b';
      name[2] = 'r';
      namelen = 3;
      break;

    case 20:
      *setname = "system";
      name[0] = 'm';
      name[1] = 'a';
      name[2] = 'c';
      name[3] = 'h';
      namelen = 4;
      break;

    case 21:
      *setname = "system";
      name[0] = 'm';
      name[1] = 'a';
      name[2] = 'c';
      name[3] = 'l';
      namelen = 4;

      break;

    case 23:
      *setname = "system";
      *type = DW_ATE_unsigned;
      name[0] = 'f';
      name[1] = 'p';
      name[2] = 'u';
      name[3] = 'l';
      namelen = 4;
      break;

    case 24:
      *setname = "system";
      *type = DW_ATE_unsigned;
      name[0] = 'f';
      name[1] = 'p';
      name[2] = 's';
      name[3] = 'c';
      name[4] = 'r';
      namelen = 5;
      break;

    case 25 ... 34:
      *setname = "fpu";
      *type = DW_ATE_float;
      name[0] = 'f';
      name[1] = 'r';
      name[2] = regno - 25 + '0';
      namelen = 3;
      break;

    case 35 ... 40:
      *setname = "fpu";
      *type = DW_ATE_float;
      name[0] = 'f';
      name[1] = 'r';
      name[2] = '1';
      name[3] = regno - 35 + '0';
      namelen = 4;
      break;

    case 87 ... 96:
      *type = DW_ATE_float;
      *setname = "fpu";
      name[0] = 'x';
      name[1] = 'f';
      name[2] = regno - 87 + '0';
      namelen = 3;
      break;

    case 97 ... 103:
      *type = DW_ATE_float;
      *setname = "fpu";
      name[0] = 'x';
      name[1] = 'f';
      name[2] = '1';
      name[3] = regno - 97 + '0';
      namelen = 4;
      break;

    default:
      return 0;
    }

  name[namelen++] = '\0';
  return namelen;
}
