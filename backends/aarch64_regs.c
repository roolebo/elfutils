/* Register names and numbers for AArch64 DWARF.
   Copyright (C) 2013, 2014 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <dwarf.h>

#define BACKEND aarch64_
#include "libebl_CPU.h"

ssize_t
aarch64_register_info (Ebl *ebl __attribute__ ((unused)),
		       int regno, char *name, size_t namelen,
		       const char **prefix, const char **setnamep,
		       int *bits, int *typep)
{
  if (name == NULL)
    return 128;

  ssize_t
  regtype (const char *setname, int type, const char *rname, bool nr, int arg)
  {
    *setnamep = setname;
    *typep = type;
    int s;
    if (nr)
      s = snprintf (name, namelen, "%s%d", rname, arg);
    else
      s = snprintf (name, namelen, "%s", rname);
    if (s < 0 || (unsigned) s >= namelen)
      return -1;
    return s + 1;
  }

  ssize_t
  regtyper (const char *setname, int type, const char *rname)
  {
    return regtype (setname, type, rname, false, 0);
  }

  ssize_t
  regtypen (const char *setname, int type, const char *rname, int arg)
  {
    return regtype (setname, type, rname, true, arg);
  }

  *prefix = "";
  *bits = 64;

  switch (regno)
    {
    case 0 ... 30:
      return regtypen ("integer", DW_ATE_signed, "x", regno);

    case 31:
      return regtyper ("integer", DW_ATE_address, "sp");

    case 32:
      return 0;

    case 33:
      return regtyper ("integer", DW_ATE_address, "elr");

    case 34 ... 63:
      return 0;

    case 64 ... 95:
      /* FP/SIMD register file supports a variety of data types--it
	 can be thought of as a register holding a single integer or
	 floating-point value, or a vector of 8-, 16-, 32- or 64-bit
	 integers.  128-bit quad-word is the only singular value that
	 covers the whole register, so mark the register thus.  */
      *bits = 128;
      return regtypen ("FP/SIMD", DW_ATE_unsigned, "v", regno - 64);

    case 96 ... 127:
      return 0;

    default:
      return -1;
    }
}
