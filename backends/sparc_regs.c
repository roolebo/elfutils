/* Register names and numbers for SPARC DWARF.
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
