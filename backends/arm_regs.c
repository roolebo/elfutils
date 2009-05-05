/* Register names and numbers for ARM DWARF.
   Copyright (C) 2009 Red Hat, Inc.
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

#include <string.h>
#include <dwarf.h>

#define BACKEND arm_
#include "libebl_CPU.h"

ssize_t
arm_register_info (Ebl *ebl __attribute__ ((unused)),
		   int regno, char *name, size_t namelen,
		   const char **prefix, const char **setname,
		   int *bits, int *type)
{
  if (name == NULL)
    return 320;

  if (regno < 0 || regno > 320 || namelen < 5)
    return -1;

  *prefix = NULL;
  *bits = 32;
  *type = DW_ATE_signed;
  *setname = "integer";

  switch (regno)
    {
    case 0 ... 9:
      name[0] = 'r';
      name[1] = regno + '0';
      namelen = 2;
      break;

    case 10 ... 12:
      name[0] = 'r';
      name[1] = '1';
      name[2] = regno % 10 + '0';
      namelen = 3;
      break;

    case 13 ... 15:
      *type = DW_ATE_address;
      name[0] = "slp"[regno - 13];
      name[1] = "prc"[regno - 13];
      namelen = 2;
      break;

    case 16 + 0 ... 16 + 7:
      regno += 96 - 16;
      /* Fall through.  */
    case 96 + 0 ... 96 + 7:
      *setname = "FPA";
      *type = DW_ATE_float;
      *bits = 96;
      name[0] = 'f';
      name[1] = regno - 96 + '0';
      namelen = 2;
      break;

    case 128:
      *type = DW_ATE_unsigned;
      return stpcpy (name, "spsr") + 1 - name;

    case 256 + 0 ... 256 + 9:
      *setname = "VFP";
      *type = DW_ATE_float;
      *bits = 64;
      name[0] = 'd';
      name[1] = regno - 256 + '0';
      namelen = 2;
      break;

    case 256 + 10 ... 256 + 31:
      *setname = "VFP";
      *type = DW_ATE_float;
      *bits = 64;
      name[0] = 'd';
      name[1] = (regno - 256) / 10 + '0';
      name[2] = (regno - 256) % 10 + '0';
      namelen = 3;
      break;

    default:
      *setname = NULL;
      return 0;
    }

  name[namelen++] = '\0';
  return namelen;
}
