/* Find CU for given offset.
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

#include <assert.h>
#include <search.h>
#include "libdwP.h"


static int
findcu_cb (const void *arg1, const void *arg2)
{
  struct Dwarf_CU *cu1 = (struct Dwarf_CU *) arg1;
  struct Dwarf_CU *cu2 = (struct Dwarf_CU *) arg2;

  /* Find out which of the two arguments is the search value.  It has
     end offset 0.  */
  if (cu1->end == 0)
    {
      if (cu1->start < cu2->start)
	return -1;
      if (cu1->start >= cu2->end)
	return 1;
    }
  else
    {
      if (cu2->start < cu1->start)
	return 1;
      if (cu2->start >= cu1->end)
	return -1;
    }

  return 0;
}


struct Dwarf_CU *
__libdw_findcu (dbg, start)
     Dwarf *dbg;
     Dwarf_Off start;
{
  /* Maybe we already know that CU.  */
  struct Dwarf_CU fake = { .start = start, .end = 0 };
  struct Dwarf_CU **found = tfind (&fake, &dbg->cu_tree, findcu_cb);
  if (found != NULL)
    return *found;

  if (start < dbg->next_cu_offset)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  /* No.  Then read more CUs.  */
  while (1)
    {
      Dwarf_Off oldoff = dbg->next_cu_offset;
      uint8_t address_size;
      uint8_t offset_size;
      Dwarf_Off abbrev_offset;

      if (INTUSE(dwarf_nextcu) (dbg, oldoff, &dbg->next_cu_offset, NULL,
				&abbrev_offset, &address_size, &offset_size)
	  != 0)
	/* No more entries.  */
	return NULL;

      /* Create an entry for this CU.  */
      struct Dwarf_CU *newp = libdw_typed_alloc (dbg, struct Dwarf_CU);

      newp->dbg = dbg;
      newp->start = oldoff;
      newp->end = dbg->next_cu_offset;
      newp->address_size = address_size;
      newp->offset_size = offset_size;
      Dwarf_Abbrev_Hash_init (&newp->abbrev_hash, 41);
      newp->orig_abbrev_offset = newp->last_abbrev_offset = abbrev_offset;
      newp->lines = NULL;
      newp->locs = NULL;

      /* Add the new entry to the search tree.  */
      if (tsearch (newp, &dbg->cu_tree, findcu_cb) == NULL)
	{
	  /* Something went wrong.  Unfo the operation.  */
	  dbg->next_cu_offset = oldoff;
	  __libdw_seterrno (DWARF_E_NOMEM);
	  return NULL;
	}

      /* Is this the one we are looking for?  */
      if (start < dbg->next_cu_offset)
	// XXX Match exact offset.
	return newp;
    }
  /* NOTREACHED */
}
