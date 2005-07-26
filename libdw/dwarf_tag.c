/* Return tag of given DIE.
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


Dwarf_Abbrev *
internal_function_def
__libdw_findabbrev (struct Dwarf_CU *cu, unsigned int code)
{
  Dwarf_Abbrev *abb;

  /* See whether the entry is already in the hash table.  */
  abb = Dwarf_Abbrev_Hash_find (&cu->abbrev_hash, code, NULL);
  if (abb == NULL)
    while (cu->last_abbrev_offset != (size_t) -1l)
      {
	size_t length;

	/* Find the next entry.  It gets automatically added to the
	   hash table.  */
	abb = __libdw_getabbrev (cu->dbg, cu, cu->last_abbrev_offset, &length,
				 NULL);
	if (abb == NULL || abb == DWARF_END_ABBREV)
	  {
	    /* Make sure we do not try to search for it again.  */
	    cu->last_abbrev_offset = (size_t) -1l;
	    abb = (void *) -1l;
	    break;
	  }

	cu->last_abbrev_offset += length;

	/* Is this the code we are looking for?  */
	if (abb->code == code)
	  break;
      }

  return abb;
}


int
dwarf_tag (die)
     Dwarf_Die *die;
{
  /* Do we already know the abbreviation?  */
  if (die->abbrev == NULL)
    {
      /* Get the abbreviation code.  */
      unsigned int u128;
      const unsigned char *addr = die->addr;
      get_uleb128 (u128, addr);

      /* Find the abbreviation.  */
      die->abbrev = __libdw_findabbrev (die->cu, u128);
    }

  if (die->abbrev == (Dwarf_Abbrev *) -1l)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return DW_TAG_invalid;
    }

  return die->abbrev->tag;
}
INTDEF(dwarf_tag)
