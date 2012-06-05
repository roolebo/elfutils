/* Out of line functions for memory-access.h macros.
   Copyright (C) 2005, 2006 Red Hat, Inc.
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
#include "libdwP.h"
#include "memory-access.h"

uint64_t
internal_function
__libdw_get_uleb128 (uint64_t acc, unsigned int i, const unsigned char **addrp)
{
  unsigned char __b;
  get_uleb128_rest_return (acc, i, addrp);
}

int64_t
internal_function
__libdw_get_sleb128 (int64_t acc, unsigned int i, const unsigned char **addrp)
{
  unsigned char __b;
  int64_t _v = acc;
  get_sleb128_rest_return (acc, i, addrp);
}
