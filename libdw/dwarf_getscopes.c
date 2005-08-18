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

#include <stdlib.h>
#include "libdwP.h"
#include <dwarf.h>


enum die_class { ignore, match, match_inline, walk, imported };

static enum die_class
classify_die (Dwarf_Die *die)
{
  switch (INTUSE(dwarf_tag) (die))
    {
      /* DIEs with addresses we can try to match.  */
    case DW_TAG_compile_unit:
    case DW_TAG_module:
    case DW_TAG_lexical_block:
    case DW_TAG_with_stmt:
    case DW_TAG_catch_block:
    case DW_TAG_try_block:
    case DW_TAG_entry_point:
      return match;
    case DW_TAG_inlined_subroutine:
      return match_inline;
    case DW_TAG_subprogram:
      /* This might be a concrete out-of-line instance of an inline, in
	 which case it is not guaranteed to be owned by the right scope and
	 we will search for its origin as for DW_TAG_inlined_subroutine.  */
      return (INTUSE(dwarf_hasattr) (die, DW_AT_abstract_origin)
	      ? match_inline : match);

      /* DIEs without addresses that can own DIEs with addresses.  */
    case DW_TAG_namespace:
      return walk;

      /* Special indirection required.  */
    case DW_TAG_imported_unit:
      return imported;

      /* Other DIEs we have no reason to descend.  */
    default:
      break;
    }
  return ignore;
}

/* DIE contains PC.  Find its child that contains PC.  Returns -1 for
   errors, 0 for no matches.  On success, *SCOPES gets the malloc'd array
   of containing scopes.  A positive return value is the number of those
   scopes.  A return value < -1 is -1 - number of those scopes, when the
   outermost scope is a concrete instance of an inline subroutine.  */
static int
find_pc (unsigned int depth, Dwarf_Die *die, Dwarf_Addr pc, Dwarf_Die **scopes)
{
  Dwarf_Die child;
  if (INTUSE(dwarf_child) (die, &child) != 0)
    return -1;

  /* Recurse on this DIE to search within its children.
     Return nonzero if this gets an error or a final result.  */
  inline int search_child (void)
    {
      int n = find_pc (depth + 1, &child, pc, scopes);
      if (n > 0)
	/* That stored the N innermost scopes.  Now store ours.  */
	(*scopes)[n++] = child;
      return n;
    }

  /* Check each of our child DIEs.  */
  enum die_class got = ignore;
  do
    {
      enum die_class child_class = classify_die (&child);
      switch (child_class)
	{
	case match:
	case match_inline:
	  if (INTUSE(dwarf_haspc) (&child, pc) > 0)
	    break;
	  continue;

	case walk:
	  if (INTUSE(dwarf_haschildren) (&child))
	    got = walk;
	  continue;

	case imported:
	  got = walk;
	  continue;

	default:
	case ignore:
	  continue;
	}

      /* We get here only when the PC has matched.  */
      got = child_class;
      break;
    }
  while (INTUSE(dwarf_siblingof) (&child, &child) == 0);

  switch (got)
    {
    case match:
    case match_inline:
      /* We have a DIE that matched the PC.  */
      if (INTUSE(dwarf_haschildren) (&child))
	{
	  /* Recurse on this DIE to narrow within its children.
	     Return now if this gets an error or a final result.  */
	  int result = search_child ();
	  if (result < 0 || (got == match && result > 0))
	    return result;
	  if (result > 0)	/* got == match_inline */
	    /* We have a winner, but CHILD is a concrete inline instance
	       so DIE and its containing scopes do not actually apply.
	       DIE is the scope that inlined the function.  Our root
	       caller must find the abstract scope that defines us.  */
	    return -1 - result;
	}

      /* This DIE has no children containing the PC, so this is it.  */
      *scopes = malloc (depth * sizeof (*scopes)[0]);
      if (*scopes == NULL)
	{
	  __libdw_seterrno (DWARF_E_NOMEM);
	  return -1;
	}
      (*scopes)[0] = child;
      return got == match ? 1 : -2;

    case walk:
      /* We don't have anything matching the PC, but we have some things
	 we might descend to find one.  Recurse on each of those.  */
      if (INTUSE(dwarf_child) (die, &child) != 0)
	return -1;
      do
	switch (classify_die (&child))
	  {
	  case walk:
	    if (INTUSE(dwarf_haschildren) (&child))
	      {
		/* Recurse on this DIE to look for the PC within its children.
		   Return now if this gets an error or a final result.  */
		int result = search_child ();
		if (result != 0)
		  return result;
	      }
	    break;

	  case imported:
	    {
	      /* This imports another compilation unit to appear
		 as part of this one, inside the current scope.
		 Recurse to search the referenced unit, but without
		 recording it as an inner scoping level.  */

	      Dwarf_Attribute attr_mem;
	      Dwarf_Attribute *attr = INTUSE(dwarf_attr) (&child, DW_AT_import,
							  &attr_mem);
	      if (INTUSE(dwarf_formref_die) (attr, &child) != NULL)
		{
		  int result = find_pc (depth, &child, pc, scopes);
		  if (result != 0)
		    return result;
		}
	    }
	    break;

	  default:
	    break;
	  }
      while (INTUSE(dwarf_siblingof) (&child, &child) == 0);
      break;

    default:
    case ignore:
      /* Nothing to see here.  */
      break;
    }

  /* No matches.  */
  return 0;
}


/* OWNER owns OWNED.  Find intermediate scopes.  *SCOPES was allocated by
   find_pc and has SKIP elements.  We realloc it, append more containing
   scopes, and return 1 + the number appended.  Returns -1 on errors,
   or 0 when OWNED was not found within OWNER.  */
static int
find_die (unsigned int depth, Dwarf_Die *owner, Dwarf_Die *owned,
	  Dwarf_Die **scopes, unsigned int skip)
{
  Dwarf_Die child;
  if (INTUSE(dwarf_child) (owner, &child) != 0)
    return -1;

  do
    {
      if (child.addr == owned->addr)
	/* This is the one.  OWNER is the innermost owner.  */
	return 1;

      /* Unfortunately we cannot short-circuit the dead-end paths just by
	 checking the physical layout to see if OWNED falls within CHILD.
	 If it doesn't, there may still be a DW_TAG_imported_unit that
	 refers to its true owner indirectly.  */

      switch (classify_die (&child))
	{
	case match:
	case match_inline:
	case walk:
	  if (INTUSE(dwarf_haschildren) (&child))
	    {
	      /* Recurse on this DIE to look for OWNED within its children.
		 Return now if this gets an error or a final result.  */
	      int n = find_die (depth + 1, &child, owned, scopes, skip);
	      if (n < 0)
		return n;
	      if (n > 1)
		{
		  /* We have a winner.  CHILD owns the owner of OWNED.  */
		  (*scopes)[skip + n - 1] = child;
		  return n + 1;
		}
	      if (n > 0)	/* n == 1 */
		{
		  /* CHILD is the direct owner of OWNED.  */
		  Dwarf_Die *nscopes = realloc (*scopes,
						(skip + depth)
						* sizeof nscopes[0]);
		  if (nscopes == NULL)
		    {
		      free (*scopes);
		      *scopes = NULL;
		      __libdw_seterrno (DWARF_E_NOMEM);
		      return -1;
		    }
		  nscopes[skip] = child;
		  *scopes = nscopes;
		  return 2;
		}
	    }
	  break;

	case imported:
	  {
	    /* This is imports another compilation unit to appear
	       as part of this one, inside the current scope.
	       Recurse to search the referenced unit, but without
	       recording it as an inner scoping level.  */

	    Dwarf_Attribute attr_mem;
	    Dwarf_Attribute *attr = INTUSE(dwarf_attr) (&child, DW_AT_import,
							&attr_mem);
	    if (INTUSE(dwarf_formref_die) (attr, &child) != NULL)
	      {
		int result = find_die (depth, &child, owner, scopes, skip);
		if (result != 0)
		  return result;
	      }
	  }
	  break;

	default:
	  break;
	}
    }
  while (INTUSE(dwarf_siblingof) (&child, &child) == 0);

  return 0;
}


int
dwarf_getscopes (Dwarf_Die *cudie, Dwarf_Addr pc, Dwarf_Die **scopes)
{
  if (cudie == NULL)
    return -1;

  int n = find_pc (2, cudie, pc, scopes);
  if (likely (n >= 0))
    {
      /* We have a final result.  Now store the outermost scope, the CU.  */
      (*scopes)[n++] = *cudie;
      return n;
    }
  if (n == -1)
    return n;

  /* We have the scopes out to one that is a concrete instance of an
     inlined subroutine (usually DW_TAG_inlined_subroutine, but can
     be DW_TAG_subprogram for a concrete out-of-line instance).
     Now we must find the lexical scopes that contain the
     corresponding abstract inline subroutine definition.  */

  n = -n - 1;

  Dwarf_Attribute attr_mem;
  Dwarf_Die die_mem;
  Dwarf_Die *origin = INTUSE(dwarf_formref_die)
    (INTUSE(dwarf_attr) (&(*scopes)[n - 1], DW_AT_abstract_origin, &attr_mem),
     &die_mem);
  if (unlikely (origin == NULL))
    goto invalid;

  int result = find_die (0, cudie, origin, scopes, n);
  if (likely (result > 0))
    {
      n = n + result - 1;
      (*scopes)[n++] = *cudie;
      return n;
    }

  if (result == 0)		/* No match, shouldn't happen.  */
    {
    invalid:
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
    }

  free (*scopes);
  *scopes = NULL;
  return -1;
}
