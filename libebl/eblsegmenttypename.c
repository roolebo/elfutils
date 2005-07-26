/* Return segment type name.
   Copyright (C) 2001, 2002, 2004 Red Hat, Inc.
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
ebl_segment_type_name (ebl, segment, buf, len)
     Ebl *ebl;
     int segment;
     char *buf;
     size_t len;
{
  const char *res;

  res = ebl != NULL ? ebl->segment_type_name (segment, buf, len) : NULL;
  if (res == NULL)
    {
      static const char *ptypes[PT_NUM] =
	{
#define PTYPE(name) [PT_##name] = #name
	  PTYPE (NULL),
	  PTYPE (LOAD),
	  PTYPE (DYNAMIC),
	  PTYPE (INTERP),
	  PTYPE (NOTE),
	  PTYPE (SHLIB),
	  PTYPE (PHDR),
	  PTYPE (TLS)
	};

      /* Is it one of the standard segment types?  */
      if (segment >= PT_NULL && segment < PT_NUM)
	res = ptypes[segment];
      else if (segment == PT_GNU_EH_FRAME)
	res = "GNU_EH_FRAME";
      else if (segment == PT_GNU_STACK)
	res = "GNU_STACK";
      else if (segment == PT_GNU_RELRO)
	res = "GNU_RELRO";
      else if (segment == PT_SUNWBSS)
	res = "SUNWBSS";
      else if (segment == PT_SUNWSTACK)
	res = "SUNWSTACK";
      else
	{
	  if (segment >= PT_LOOS && segment <= PT_HIOS)
	    snprintf (buf, len, "LOOS+%d", segment - PT_LOOS);
	  else if (segment >= PT_LOPROC && segment <= PT_HIPROC)
	    snprintf (buf, len, "LOPROC+%d", segment - PT_LOPROC);
	  else
	    snprintf (buf, len, "%s: %d", gettext ("<unknown>"), segment);

	  res = buf;
	}
    }

  return res;
}
