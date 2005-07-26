/* Check dynamic tag.
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

#include <inttypes.h>
#include <libeblP.h>


bool
ebl_dynamic_tag_check (ebl, tag)
     Ebl *ebl;
     int64_t tag;
{
  bool res = ebl != NULL ? ebl->dynamic_tag_check (tag) : false;

  if (!res
      && ((tag >= 0 && tag < DT_NUM)
	  || (tag >= DT_GNU_PRELINKED && tag <= DT_SYMINENT)
	  || (tag >= DT_GNU_CONFLICT && tag <= DT_SYMINFO)
	  || tag == DT_VERSYM
	  || (tag >= DT_RELACOUNT && tag <= DT_VERNEEDNUM)
	  || tag == DT_AUXILIARY
	  || tag == DT_FILTER))
    res = true;

  return res;
}
