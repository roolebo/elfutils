/* Return block represented by attribute.
   Copyright (C) 2004, 2005 Red Hat, Inc.
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
dwarf_formblock (attr, return_block)
     Dwarf_Attribute *attr;
     Dwarf_Block *return_block;
{
  if (attr == NULL)
    return -1;

  const unsigned char *datap;

  switch (attr->form)
    {
    case DW_FORM_block1:
      return_block->length = *(uint8_t *) attr->valp;
      return_block->data = attr->valp + 1;
      break;

    case DW_FORM_block2:
      return_block->length = read_2ubyte_unaligned (attr->cu->dbg, attr->valp);
      return_block->data = attr->valp + 2;
      break;

    case DW_FORM_block4:
      return_block->length = read_4ubyte_unaligned (attr->cu->dbg, attr->valp);
      return_block->data = attr->valp + 4;
      break;

    case DW_FORM_block:
      datap = attr->valp;
      get_uleb128 (return_block->length, datap);
      return_block->data = (unsigned char *) datap;
      break;

    default:
      __libdw_seterrno (DWARF_E_NO_BLOCK);
      return -1;
    }

  if (return_block->data + return_block->length
      > ((unsigned char *) attr->cu->dbg->sectiondata[IDX_debug_info]->d_buf
	 + attr->cu->dbg->sectiondata[IDX_debug_info]->d_size))
    {
      /* Block does not fit.  */
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  return 0;
}
INTDEF(dwarf_formblock)
