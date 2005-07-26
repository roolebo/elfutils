/* Get attributes of the DIE.
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

#include "libdwP.h"


ptrdiff_t
dwarf_getattrs (Dwarf_Die *die, int (*callback) (Dwarf_Attribute *, void *),
		void *arg, ptrdiff_t offset)
{
  if (die == NULL)
    return -1l;

  const unsigned char *die_addr = die->addr;

  /* Get the abbreviation code.  */
  unsigned int u128;
  get_uleb128 (u128, die_addr);

  if (die->abbrev == NULL)
    /* Find the abbreviation.  */
    die->abbrev = __libdw_findabbrev (die->cu, u128);

  if (die->abbrev == (Dwarf_Abbrev *) -1l)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1l;
    }

  /* This is where the attributes start.  */
  const unsigned char *attrp = die->abbrev->attrp + offset;

  /* Go over the list of attributes.  */
  Dwarf *dbg = die->cu->dbg;
  while (1)
    {
      /* Are we still in bounds?  */
      if (unlikely (attrp
		    >= ((unsigned char *) dbg->sectiondata[IDX_debug_abbrev]->d_buf
			+ dbg->sectiondata[IDX_debug_abbrev]->d_size)))
	{
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return -1;
	}

      /* Get attribute name and form.  */
      Dwarf_Attribute attr;
      // XXX Fix bound checks
      get_uleb128 (attr.code, attrp);
      get_uleb128 (attr.form, attrp);

      /* We can stop if we found the attribute with value zero.  */
      if (attr.code == 0 && attr.form == 0)
        return 0;

      /* Fill in the rest.  */
      attr.valp = (unsigned char *) die_addr;
      attr.cu = die->cu;

      /* Now call the callback function.  */
      if (callback (&attr, arg) != DWARF_CB_OK)
	return attrp - die->abbrev->attrp;

      /* Skip over the rest of this attribute (if there is any).  */
      if (attr.form != 0)
	{
	  size_t len = __libdw_form_val_len (dbg, die->cu, attr.form,
					     die_addr);

	  if (unlikely (len == (size_t) -1l))
	    /* Something wrong with the file.  */
	    return -1l;

	  // XXX We need better boundary checks.
	  die_addr += len;
	}
    }
  /* NOTREACHED */
}
