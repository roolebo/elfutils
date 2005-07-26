/* Return sibling of given DIE.
   Copyright (C) 2003, 2004, 2005 Red Hat, Inc.
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

#include "libdwP.h"
#include <dwarf.h>
#include <string.h>


int
dwarf_siblingof (die, result)
     Dwarf_Die *die;
     Dwarf_Die *result;
{
  /* Ignore previous errors.  */
  if (die == NULL)
    return -1;

  unsigned int level = 0;

  /* Copy of the current DIE.  */
  Dwarf_Die this_die = *die;
  /* Temporary attributes we create.  */
  Dwarf_Attribute sibattr;
  /* Copy of the CU in the request.  */
  sibattr.cu = this_die.cu;
  /* That's the address we start looking.  */
  unsigned char *addr = this_die.addr;
  /* End of the buffer.  */
  unsigned char *endp
    = ((unsigned char *) sibattr.cu->dbg->sectiondata[IDX_debug_info]->d_buf
       + sibattr.cu->end);

  /* Search for the beginning of the next die on this level.  We
     must not return the dies for children of the given die.  */
  do
    {
      /* Find the end of the DIE or the sibling attribute.  */
      addr = __libdw_find_attr (&this_die, DW_AT_sibling, &sibattr.code,
				&sibattr.form);
      if (sibattr.code == DW_AT_sibling)
	{
	  Dwarf_Off offset;
	  sibattr.valp = addr;
	  if (INTUSE(dwarf_formref) (&sibattr, &offset) != 0)
	    /* Something went wrong.  */
	    return -1;

	  /* Compute the next address.  */
	  addr = ((unsigned char *)
		  sibattr.cu->dbg->sectiondata[IDX_debug_info]->d_buf
		  + sibattr.cu->start + offset);
	}
      else if (unlikely (addr == NULL)
	       || unlikely (this_die.abbrev == (Dwarf_Abbrev *) -1l))
	return -1;
      else if (this_die.abbrev->has_children)
	/* This abbreviation has children.  */
	++level;


      while (1)
	{
	  /* Make sure we are still in range.  Some producers might skip
	     the trailing NUL bytes.  */
	  if (addr >= endp)
	    return 1;

	  if (*addr != '\0')
	    break;

	  if (level-- == 0)
	    /* No more sibling at all.  */
	    return 1;

	  ++addr;
	}

      /* Initialize the 'current DIE'.  */
      this_die.addr = addr;
      this_die.abbrev = NULL;
    }
  while (level > 0);

  /* Maybe we reached the end of the CU.  */
  if (addr >= endp)
    return 1;

  /* Clear the entire DIE structure.  This signals we have not yet
     determined any of the information.  */
  memset (result, '\0', sizeof (Dwarf_Die));

  /* We have the address.  */
  result->addr = addr;

  /* Same CU as the parent.  */
  result->cu = sibattr.cu;

  return 0;
}
INTDEF(dwarf_siblingof)
