/* Return scope DIEs containing PC address.
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

#include <assert.h>
#include <stdlib.h>
#include "libdwP.h"
#include <dwarf.h>


struct args
{
  Dwarf_Addr pc;
  Dwarf_Die *scopes;
  unsigned int inlined, nscopes;
  Dwarf_Die inlined_origin;
};

/* Preorder visitor: prune the traversal if this DIE does not contain PC.  */
static int
pc_match (unsigned int depth, struct Dwarf_Die_Chain *die, void *arg)
{
  struct args *a = arg;

  if (a->scopes != NULL || INTUSE(dwarf_haspc) (&die->die, a->pc) <= 0)
    die->prune = true;
  else if (INTUSE (dwarf_tag) (&die->die) == DW_TAG_inlined_subroutine)
    a->inlined = depth;

  return 0;
}

/* Preorder visitor for second partial traversal after finding a
   concrete inlined instance.  */
static int
origin_match (unsigned int depth, struct Dwarf_Die_Chain *die, void *arg)
{
  struct args *a = arg;

  if (die->die.addr != a->inlined_origin.addr)
    return 0;

  /* We have a winner!  This is the abstract definition of the inline
     function of which A->scopes[A->nscopes - 1] is a concrete instance.
  */

  unsigned int nscopes = a->nscopes + depth;
  Dwarf_Die *scopes = realloc (a->scopes, nscopes * sizeof scopes[0]);
  if (scopes == NULL)
    {
      free (a->scopes);
      __libdw_seterrno (DWARF_E_NOMEM);
      return -1;
    }

  a->scopes = scopes;
  do
    {
      die = die->parent;
      scopes[a->nscopes++] = die->die;
    }
  while (a->nscopes < nscopes);
  assert (die->parent == NULL);
  return a->nscopes;
}

/* Postorder visitor: first (innermost) call wins.  */
static int
pc_record (unsigned int depth, struct Dwarf_Die_Chain *die, void *arg)
{
  struct args *a = arg;

  if (a->scopes == NULL)
    {
      if (die->prune)
	return 0;

      /* We have hit the innermost DIE that contains the target PC.  */

      a->nscopes = depth + 1 - a->inlined;
      a->scopes = malloc (a->nscopes * sizeof a->scopes[0]);
      if (a->scopes == NULL)
	{
	  __libdw_seterrno (DWARF_E_NOMEM);
	  return -1;
	}

      for (unsigned int i = 0; i < a->nscopes; ++i)
	{
	  a->scopes[i] = die->die;
	  die = die->parent;
	}

      if (a->inlined == 0)
	{
	  assert (die == NULL);
	  return a->nscopes;
	}

      /* This is the concrete inlined instance itself.
	 Record its abstract_origin pointer.  */
      Dwarf_Die *const inlinedie = &a->scopes[depth - a->inlined];

      assert (INTUSE (dwarf_tag) (inlinedie) == DW_TAG_inlined_subroutine);
      Dwarf_Attribute attr_mem;
      Dwarf_Attribute *attr = INTUSE (dwarf_attr) (inlinedie,
						   DW_AT_abstract_origin,
						   &attr_mem);
      if (INTUSE (dwarf_formref_die) (attr, &a->inlined_origin) == NULL)
	return -1;
      return 0;
    }


  /* We've recorded the scopes back to one that is a concrete inlined
     instance.  Now return out of the traversal back to the scope
     containing that instance.  */

  assert (a->inlined);
  if (depth >= a->inlined)
    /* Not there yet.  */
    return 0;

  /* Now we are in a scope that contains the concrete inlined instance.
     Search it for the inline function's abstract definition.
     If we don't find it, return to search the containing scope.
     If we do find it, the nonzero return value will bail us out
     of the postorder traversal.  */
  return __libdw_visit_scopes (depth, die, &origin_match, NULL, &a);
}


int
dwarf_getscopes (Dwarf_Die *cudie, Dwarf_Addr pc, Dwarf_Die **scopes)
{
  if (cudie == NULL)
    return -1;

  struct Dwarf_Die_Chain cu = { .parent = NULL, .die = *cudie };
  struct args a = { .pc = pc };

  int result = __libdw_visit_scopes (0, &cu, &pc_match, &pc_record, &a);

  if (result == 0 && a.scopes != NULL)
    result = __libdw_visit_scopes (0, &cu, &origin_match, NULL, &a);

  if (result > 0)
    *scopes = a.scopes;

  return result;
}
