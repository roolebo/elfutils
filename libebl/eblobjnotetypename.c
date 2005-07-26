/* Return note type name.
   Copyright (C) 2002 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <inttypes.h>
#include <stdio.h>
#include <libeblP.h>


const char *
ebl_object_note_type_name (ebl, type, buf, len)
     Ebl *ebl;
     uint32_t type;
     char *buf;
     size_t len;
{
  const char *res = ebl->object_note_type_name (type, buf, len);

  if (res == NULL)
    {
      static const char *knowntypes[] =
	{
#define KNOWNSTYPE(name) [NT_##name] = #name
	  KNOWNSTYPE (VERSION),
	};

      /* Handle standard names.  */
      if (type < sizeof (knowntypes) / sizeof (knowntypes[0])
	  && knowntypes[type] != NULL)
	res = knowntypes[type];
      else
	{
	  snprintf (buf, len, "%s: %" PRIu32, gettext ("<unknown>"), type);

	  res = buf;
	}
    }

  return res;
}
