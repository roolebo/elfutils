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
__libdw_visit_scopes (depth, root, visit, arg)
     unsigned int depth;
     Dwarf_Die *root;
     int (*visit) (unsigned int depth, Dwarf_Die *die, void *arg);
     void *arg;
{
  Dwarf_Die child;
  if (INTUSE(dwarf_child) (root, &child) != 0)
    return -1;

  do
    {
      int result = (*visit) (depth, &child, arg);
      if (result != DWARF_CB_OK)
	return result;

      switch (classify_die (&child))
	{
	case match:
	case match_inline:
	case walk:
	  if (INTUSE(dwarf_haschildren) (&child))
	    {
	      result = __libdw_visit_scopes (depth + 1, &child, visit, arg);
	      if (result != DWARF_CB_OK)
		return result;
	    }
	  break;

	case imported:
	  {
	    /* This is imports another compilation unit to appear
	       as part of this one, inside the current scope.
	       Recurse to searesulth the referenced unit, but without
	       recording it as an inner scoping level.  */

	    Dwarf_Attribute attr_mem;
	    Dwarf_Attribute *attr = INTUSE(dwarf_attr) (&child, DW_AT_import,
							&attr_mem);
	    if (INTUSE(dwarf_formref_die) (attr, &child) != NULL)
	      {
		result = __libdw_visit_scopes (depth + 1, &child, visit, arg);
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
