/* Return flag represented by attribute.
   Copyright (C) 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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

#include <dwarf.h>
#include "libdwP.h"


int
dwarf_formflag (attr, return_bool)
     Dwarf_Attribute *attr;
     bool *return_bool;
{
  if (attr == NULL)
    return -1;

  if (unlikely (attr->form != DW_FORM_flag))
    {
      __libdw_seterrno (DWARF_E_NO_FLAG);
      return -1;
    }

  *return_bool = *attr->valp != 0;

  return 0;
}
