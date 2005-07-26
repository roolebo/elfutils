/* Out of line functions for memory-access.h macros.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "libdwP.h"
#include "memory-access.h"

uint64_t
internal_function_def
__libdw_get_uleb128 (uint64_t acc, unsigned int i, const unsigned char **addrp)
{
  unsigned char __b;
  get_uleb128_rest_return (acc, i, addrp);
}

int64_t
internal_function_def
__libdw_get_sleb128 (int64_t acc, unsigned int i, const unsigned char **addrp)
{
  unsigned char __b;
  int64_t _v = acc;
  get_sleb128_rest_return (acc, i, addrp);
}
