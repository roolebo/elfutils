/* Return reference offset represented by attribute.
   Copyright (C) 2003, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2003.

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
dwarf_formref (attr, return_offset)
     Dwarf_Attribute *attr;
     Dwarf_Off *return_offset;
{
  if (attr == NULL)
    return -1;

  const unsigned char *datap;

  switch (attr->form)
    {
    case DW_FORM_ref1:
      *return_offset = *attr->valp;
      break;

    case DW_FORM_ref2:
      *return_offset = read_2ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_ref4:
      *return_offset = read_4ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_ref8:
      *return_offset = read_8ubyte_unaligned (attr->cu->dbg, attr->valp);
      break;

    case DW_FORM_ref_udata:
      datap = attr->valp;
      get_uleb128 (*return_offset, datap);
      break;

    case DW_FORM_ref_addr:
      __libdw_seterrno (DWARF_E_INVALID_REFERENCE);
      return -1;

    default:
      __libdw_seterrno (DWARF_E_NO_REFERENCE);
      return -1;
    }

  return 0;
}
INTDEF(dwarf_formref)
