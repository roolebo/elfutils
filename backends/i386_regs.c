/* Register names and numbers for i386 DWARF.
   Copyright (C) 2005 Red Hat, Inc.
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

#define BACKEND i386_
#include "libebl_CPU.h"

ssize_t
i386_register_name (Ebl *ebl __attribute__ ((unused)),
		    int regno, char *name, size_t namelen,
		    const char **prefix, const char **setname)
{
  if (name == NULL)
    return 46;

  if (regno < 0 || regno > 45 || namelen < 6)
    return -1;

  *prefix = "%";
  if (regno < 11)
    *setname = "integer";
  else if (regno < 19)
    *setname = "x87";
  else if (regno < 29)
    *setname = "SSE";
  else if (regno < 37)
    *setname = "MMX";
  else if (regno < 40)
    *setname = "FPU-control";
  else
    *setname = "segment";

  switch (regno)
    {
      static const char baseregs[][2] =
	{
	  "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "ip"
	};

    case 0 ... 8:
      name[0] = 'e';
      name[1] = baseregs[regno][0];
      name[2] = baseregs[regno][1];
      namelen = 3;
      break;

    case 9:
      return stpcpy (name, "eflags") - name;
    case 10:
      return stpcpy (name, "trapno") - name;

    case 11 ... 18:
      name[0] = 's';
      name[1] = 't';
      name[2] = regno - 11 + '0';
      namelen = 3;
      break;

    case 21 ... 28:
      name[0] = 'x';
      name[1] = 'm';
      name[2] = 'm';
      name[3] = regno - 21 + '0';
      namelen = 4;
      break;

    case 29 ... 36:
      name[0] = 'm';
      name[1] = 'm';
      name[2] = regno - 29 + '0';
      namelen = 3;
      break;

    case 37:
      return stpcpy (name, "fctrl") - name;
    case 38:
      return stpcpy (name, "fstat") - name;
    case 39:
      return stpcpy (name, "mxcsr") - name;

    case 40 ... 45:
      name[0] = "ecsdfg"[regno - 40];
      name[1] = 's';
      namelen = 2;
      break;

    default:
      *setname = NULL;
      return 0;
    }

  name[namelen++] = '\0';
  return namelen;
}
