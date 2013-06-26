/* Helper functions to descend DWARF scope trees.
   Copyright (C) 2005,2006,2007 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
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

int
__libdw_visit_scopes (depth, root, previsit, postvisit, arg)
     unsigned int depth;
     struct Dwarf_Die_Chain *root;
     int (*previsit) (unsigned int depth, struct Dwarf_Die_Chain *, void *);
     int (*postvisit) (unsigned int depth, struct Dwarf_Die_Chain *, void *);
     void *arg;
{
  struct Dwarf_Die_Chain child;
  int ret;

  child.parent = root;
  if ((ret = INTUSE(dwarf_child) (&root->die, &child.die)) != 0)
    return ret < 0 ? -1 : 0; // Having zero children is legal.

  inline int recurse (void)
    {
      return __libdw_visit_scopes (depth + 1, &child,
				   previsit, postvisit, arg);
    }

  inline int walk_children ()
  {
    do
      {
	/* For an imported unit, it is logically as if the children of
	   that unit are siblings of the other children.  So don't do
	   a full recursion into the imported unit, but just walk the
	   children in place before moving to the next real child.  */
	while (classify_die (&child.die) == imported)
	  {
	    Dwarf_Die orig_child_die = child.die;
	    Dwarf_Attribute attr_mem;
	    Dwarf_Attribute *attr = INTUSE(dwarf_attr) (&child.die,
							DW_AT_import,
							&attr_mem);
	    if (INTUSE(dwarf_formref_die) (attr, &child.die) != NULL
                && INTUSE(dwarf_child) (&child.die, &child.die) == 0)
	      {
		int result = walk_children ();
		if (result != DWARF_CB_OK)
		  return result;
	      }

	    /* Any "real" children left?  */
	    if ((ret = INTUSE(dwarf_siblingof) (&orig_child_die,
						&child.die)) != 0)
	      return ret < 0 ? -1 : 0;
	  };

	child.prune = false;

	if (previsit != NULL)
	  {
	    int result = (*previsit) (depth + 1, &child, arg);
	    if (result != DWARF_CB_OK)
	      return result;
	  }

	if (!child.prune)
	  switch (classify_die (&child.die))
	    {
	    case match:
	    case match_inline:
	    case walk:
	      if (INTUSE(dwarf_haschildren) (&child.die))
		{
		  int result = recurse ();
		  if (result != DWARF_CB_OK)
		    return result;
		}
	      break;

	    default:
	      break;
	    }

	if (postvisit != NULL)
	  {
	    int result = (*postvisit) (depth + 1, &child, arg);
	    if (result != DWARF_CB_OK)
	      return result;
	  }
      }
    while ((ret = INTUSE(dwarf_siblingof) (&child.die, &child.die)) == 0);

    return ret < 0 ? -1 : 0;
  }

  return walk_children ();
}
