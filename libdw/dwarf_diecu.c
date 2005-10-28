/* Return CU DIE containing given DIE.
   Copyright (C) 2005 Red Hat, Inc.

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

#include <string.h>
#include "libdwP.h"


Dwarf_Die *
dwarf_diecu (die, result, address_sizep, offset_sizep)
     Dwarf_Die *die;
     Dwarf_Die *result;
     uint8_t *address_sizep;
     uint8_t *offset_sizep;
{
  if (die == NULL)
    return NULL;

  /* Clear the entire DIE structure.  This signals we have not yet
     determined any of the information.  */
  memset (result, '\0', sizeof (Dwarf_Die));

  result->addr = ((char *) die->cu->dbg->sectiondata[IDX_debug_info]->d_buf
		  + die->cu->start + 3 * die->cu->offset_size - 4 + 3);
  result->cu = die->cu;

  if (address_sizep != NULL)
    *address_sizep = die->cu->address_size;
  if (offset_sizep != NULL)
    *offset_sizep = die->cu->offset_size;

  return result;
}
