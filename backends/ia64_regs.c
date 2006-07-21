/* Register names and numbers for IA64 DWARF.
   Copyright (C) 2006 Red Hat, Inc.
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
ia64_register_name (Ebl *ebl __attribute__ ((unused)),
		    int regno, char *name, size_t namelen,
		    const char **prefix, const char **setname)
{
  if (name == NULL)
    return 687 + 64;

  if (regno < 0 || regno > 687 + 63 || namelen < 12)
    return -1;

  *prefix = "";

  *setname = "application";
  switch (regno)
    {
    case 0 ... 9:
      name[0] = 'r';
      name[1] = (regno - 0) + '0';
      namelen = 2;
      *setname = "integer";
      break;

    case 10 ... 99:
      name[0] = 'r';
      name[1] = (regno - 0) / 10 + '0';
      name[2] = (regno - 0) % 10 + '0';
      namelen = 3;
      *setname = "integer";
      break;

    case 100 ... 127:
      name[0] = 'r';
      name[1] = '1';
      name[2] = (regno - 100) / 10 + '0';
      name[3] = (regno - 0) % 10 + '0';
      namelen = 4;
      *setname = "integer";
      break;

    case 128 + 0 ... 128 + 9:
      name[0] = 'f';
      name[1] = (regno - 128) + '0';
      namelen = 2;
      *setname = "FPU";
      break;

    case 128 + 10 ... 128 + 99:
      name[0] = 'f';
      name[1] = (regno - 128) / 10 + '0';
      name[2] = (regno - 128) % 10 + '0';
      namelen = 3;
      *setname = "FPU";
      break;

    case 128 + 100 ... 128 + 127:
      name[0] = 'f';
      name[1] = '1';
      name[2] = (regno - 128 - 100) / 10 + '0';
      name[3] = (regno - 128) % 10 + '0';
      namelen = 4;
      *setname = "FPU";
      break;

    case 320 + 0 ... 320 + 7:
      name[0] = 'b';
      name[1] = (regno - 320) + '0';
      namelen = 2;
      *setname = "branch";
      break;

    case 328:
      return stpcpy (name, "vfp") + 1 - name;
    case 329:
      return stpcpy (name, "vrap") + 1 - name;
    case 330:
      return stpcpy (name, "pr") + 1 - name;
    case 331:
      return stpcpy (name, "ip") + 1 - name;
    case 332:
      return stpcpy (name, "psr") + 1 - name;
    case 333:
      return stpcpy (name, "cfm") + 1 - name;

    case 334 + 0 ... 334 + 7:
      name[0] = 'k';
      name[1] = 'r';
      name[2] = (regno - 334) + '0';
      namelen = 3;
      break;

    case 350:
      return stpcpy (name, "rsc") + 1 - name;
    case 351:
      return stpcpy (name, "bsp") + 1 - name;
    case 352:
      return stpcpy (name, "bspstore") + 1 - name;
    case 353:
      return stpcpy (name, "rnat") + 1 - name;
    case 355:
      return stpcpy (name, "fcr") + 1 - name;
    case 358:
      return stpcpy (name, "eflag") + 1 - name;
    case 359:
      return stpcpy (name, "csd") + 1 - name;
    case 360:
      return stpcpy (name, "ssd") + 1 - name;
    case 361:
      return stpcpy (name, "cflg") + 1 - name;
    case 362:
      return stpcpy (name, "fsr") + 1 - name;
    case 363:
      return stpcpy (name, "fir") + 1 - name;
    case 364:
      return stpcpy (name, "fdr") + 1 - name;
    case 366:
      return stpcpy (name, "ccv") + 1 - name;
    case 370:
      return stpcpy (name, "unat") + 1 - name;
    case 374:
      return stpcpy (name, "fpsr") + 1 - name;
    case 378:
      return stpcpy (name, "itc") + 1 - name;
    case 398:
      return stpcpy (name, "pfs") + 1 - name;
    case 399:
      return stpcpy (name, "lc") + 1 - name;
    case 400:
      return stpcpy (name, "ec") + 1 - name;

    case 462 + 0 ... 462 + 9:
      name[0] = 'n';
      name[1] = 'a';
      name[2] = 't';
      name[3] = (regno - 462) + '0';
      namelen = 4;
      *setname = "NAT";
      break;

    case 462 + 10 ... 462 + 99:
      name[0] = 'n';
      name[1] = 'a';
      name[2] = 't';
      name[3] = (regno - 462) / 10 + '0';
      name[4] = (regno - 462) % 10 + '0';
      namelen = 5;
      *setname = "NAT";
      break;

    case 462 + 100 ... 462 + 127:
      name[0] = 'n';
      name[1] = 'a';
      name[2] = 't';
      name[3] = (regno - 462 - 100) / 10 + '0';
      name[4] = (regno - 462) % 10 + '0';
      namelen = 5;
      *setname = "NAT";
      break;

    case 590:
      return stpcpy (name, "bof") + 1 - name;

    case 687 + 0 ... 687 + 9:
      name[0] = 'p';
      name[1] = (regno - 687) + '0';
      namelen = 2;
      *setname = "predicate";
      break;

    case 687 + 10 ... 687 + 63:
      name[0] = 'p';
      name[1] = (regno - 687) / 10 + '0';
      name[2] = (regno - 687) % 10 + '0';
      namelen = 3;
      *setname = "predicate";
      break;

    default:
      *setname = NULL;
      return 0;
    }

  name[namelen++] = '\0';
  return namelen;
}
