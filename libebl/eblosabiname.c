/* Return OS ABI name
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
ebl_osabi_name (ebl, osabi, buf, len)
     Ebl *ebl;
     int osabi;
     char *buf;
     size_t len;
{
  const char *res = ebl != NULL ? ebl->osabi_name (osabi, buf, len) : NULL;

  if (res == NULL)
    {
      if (osabi == ELFOSABI_NONE)
	res = "UNIX - System V";
      else if (osabi == ELFOSABI_HPUX)
	res = "HP/UX";
      else if (osabi == ELFOSABI_NETBSD)
	res = "NetBSD";
      else if (osabi == ELFOSABI_LINUX)
	res = "Linux";
      else if (osabi == ELFOSABI_SOLARIS)
	res = "Solaris";
      else if (osabi == ELFOSABI_AIX)
	res = "AIX";
      else if (osabi == ELFOSABI_IRIX)
	res = "Irix";
      else if (osabi == ELFOSABI_FREEBSD)
	res = "FreeBSD";
      else if (osabi == ELFOSABI_TRU64)
	res = "TRU64";
      else if (osabi == ELFOSABI_MODESTO)
	res = "Modesto";
      else if (osabi == ELFOSABI_OPENBSD)
	res = "OpenBSD";
      else if (osabi == ELFOSABI_ARM)
	res = "Arm";
      else if (osabi == ELFOSABI_STANDALONE)
	res = gettext ("Stand alone");
      else
	{
	  snprintf (buf, len, "%s: %d", gettext ("<unknown>"), osabi);

	  res = buf;
	}
    }

  return res;
}
