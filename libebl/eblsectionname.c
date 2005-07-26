/* Return section name.
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
ebl_section_name (ebl, section, xsection, buf, len, scnnames, shnum)
     Ebl *ebl;
     int section;
     int xsection;
     char *buf;
     size_t len;
     const char *scnnames[];
     size_t shnum;
{
  const char *res = ebl != NULL ? ebl->section_name (section, xsection,
						     buf, len) : NULL;

  if (res == NULL)
    {
      if (section == SHN_UNDEF)
	res = "UNDEF";
      else if (section == SHN_ABS)
	res = "ABS";
      else if (section == SHN_COMMON)
	res = "COMMON";
      else if (section == SHN_BEFORE)
	res = "BEFORE";
      else if (section == SHN_AFTER)
	res = "AFTER";
      else if ((section < SHN_LORESERVE || section == SHN_XINDEX)
	       && (size_t) section < shnum)
	{
	  int idx = section != SHN_XINDEX ? section : xsection;

	  if (scnnames != NULL)
	    res = scnnames[idx];
	  else
	    {
	      snprintf (buf, len, "%d", idx);
	      res = buf;
	    }
	}
      else
	{
	  /* Handle OS-specific section names.  */
	  if (section == SHN_XINDEX)
	    snprintf (buf, len, "%s: %d", "XINDEX", xsection);
	  else if (section >= SHN_LOOS && section <= SHN_HIOS)
	    snprintf (buf, len, "LOOS+%x", section - SHN_LOOS);
	  /* Handle processor-specific section names.  */
	  else if (section >= SHN_LOPROC && section <= SHN_HIPROC)
	    snprintf (buf, len, "LOPROC+%x", section - SHN_LOPROC);
	  else if (section >= SHN_LORESERVE && section <= SHN_HIRESERVE)
	    snprintf (buf, len, "LORESERVE+%x", section - SHN_LORESERVE);
	  else
	    snprintf (buf, len, "%s: %d", gettext ("<unknown>"), section);

	  res = buf;
	}
    }

  return res;
}
