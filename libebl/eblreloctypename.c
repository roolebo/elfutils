/* Return relocation type name.
   Copyright (C) 2001, 2002 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

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

#include <stdio.h>
#include <libeblP.h>


const char *
ebl_reloc_type_name (ebl, reloc, buf, len)
     Ebl *ebl;
     int reloc;
     char *buf;
     size_t len;
{
  const char *res;

  res = ebl != NULL ? ebl->reloc_type_name (reloc, buf, len) : NULL;
  if (res == NULL)
    /* There are no generic relocation type names.  */
    res = "???";

  return res;
}
