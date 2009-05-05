/* ARM-specific auxv handling.
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

#define BACKEND arm_
#include "libebl_CPU.h"

int
EBLHOOK(auxv_info) (GElf_Xword a_type, const char **name, const char **format)
{
  if (a_type != AT_HWCAP)
    return 0;

  *name = "HWCAP";
  *format = "b"
    "swp\0" "half\0" "thumb\0" "26bit\0"
    "fast-mult\0" "fpa\0" "vfp\0" "edsp\0"
    "java\0" "iwmmxt\0"
    "\0";
  return 1;
}
