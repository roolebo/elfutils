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

#include <error.h>
#include <libintl.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include "system.h"

#ifndef _
# define _(str) gettext (str)
#endif


/* Allocate N bytes of memory dynamically, with error checking.  */
void *
xmalloc (n)
     size_t n;
{
  void *p;

  p = malloc (n);
  if (p == NULL)
    error (EXIT_FAILURE, 0, _("memory exhausted"));
  return p;
}


/* Allocate memory for N elements of S bytes, with error checking.  */
void *
xcalloc (n, s)
     size_t n, s;
{
  void *p;

  p = calloc (n, s);
  if (p == NULL)
    error (EXIT_FAILURE, 0, _("memory exhausted"));
  return p;
}


/* Change the size of an allocated block of memory P to N bytes,
   with error checking.  */
void *
xrealloc (p, n)
     void *p;
     size_t n;
{
  p = realloc (p, n);
  if (p == NULL)
    error (EXIT_FAILURE, 0, _("memory exhausted"));
  return p;
}
