/* Copyright (C) 2000, 2002 Free Software Foundation, Inc.

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

#include <string.h>
#include "system.h"


/* Return a newly allocated copy of STRING.  */
char *
xstrndup (string, n)
     const char *string;
     size_t n;
{
  char *res;
  size_t len = strnlen (string, n);
  *((char *) mempcpy ((res = xmalloc (len + 1)), string, len)) = '\0';
  return res;
}
