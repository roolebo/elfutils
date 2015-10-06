/* Object attribute tags for SPARC.
   Copyright (C) 2015 Oracle, Inc.
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

#include <string.h>
#include <dwarf.h>

#define BACKEND sparc_
#include "libebl_CPU.h"

bool
sparc_check_object_attribute (Ebl *ebl __attribute__ ((unused)),
			      const char *vendor, int tag, uint64_t value,
			      const char **tag_name, const char **value_name)
{
  if (!strcmp (vendor, "gnu"))
    switch (tag)
      {
      case 4:
	*tag_name = "GNU_Sparc_HWCAPS";
	static const char *hwcaps[30] =
	  {
	    "mul32", "div32", "fsmuld", "v8plus", "popc", "vis", "vis2",
	    "asi_blk_init", "fmaf", NULL, "vis3", "hpc", "random", "trans", "fjfmau",
	    "ima", "asi_cache_sparing", "aes", "des", "kasumi", "camellia",
	    "md5", "sha1", "sha256", "sha512", "mpmul", "mont", "pause",
	    "cbcond", "crc32c"
	  };
	if (value < 30 && hwcaps[value] != NULL)
	  *value_name = hwcaps[value];
	return true;

      case 8:
	*tag_name = "GNU_Sparc_HWCAPS2";
	static const char *hwcaps2[11] =
	  {
	    "fjathplus", "vis3b", "adp", "sparc5", "mwait", "xmpmul",
	    "xmont", "nsec", "fjathhpc", "fjdes", "fjaes"
	  };
	if (value < 11)
	  *value_name = hwcaps2[value];
	return true;
      }

  return false;
}

