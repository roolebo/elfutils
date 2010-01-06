/* Compute size of an aggregate type from DWARF.
   Copyright (C) 2010 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   In addition, as a special exception, Red Hat, Inc. gives You the
   additional right to link the code of Red Hat elfutils with code licensed
   under any Open Source Initiative certified open source license
   (http://www.opensource.org/licenses/index.php) which requires the
   distribution of source code with any binary distribution and to
   distribute linked combinations of the two.  Non-GPL Code permitted under
   this exception must only link to the code of Red Hat elfutils through
   those well defined interfaces identified in the file named EXCEPTION
   found in the source code files (the "Approved Interfaces").  The files
   of Non-GPL Code may instantiate templates or use macros or inline
   functions from the Approved Interfaces without causing the resulting
   work to be covered by the GNU General Public License.  Only Red Hat,
   Inc. may make changes or additions to the list of Approved Interfaces.
   Red Hat's grant of this exception is conditioned upon your not adding
   any new exceptions.  If you wish to add a new Approved Interface or
   exception, please contact Red Hat.  You must obey the GNU General Public
   License in all respects for all of the Red Hat elfutils code and other
   code used in conjunction with Red Hat elfutils except the Non-GPL Code
   covered by this exception.  If you modify this file, you may extend this
   exception to your version of the file, but you are not obligated to do
   so.  If you do not wish to provide this exception without modification,
   you must delete this exception statement from your version and license
   this file solely under the GPL without exception.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <dwarf.h>
#include "libdwP.h"


static Dwarf_Die *
get_type (Dwarf_Die *die, Dwarf_Attribute *attr_mem, Dwarf_Die *type_mem)
{
  return INTUSE(dwarf_formref_die)
    (INTUSE(dwarf_attr_integrate) (die, DW_AT_type, attr_mem), type_mem);
}

static int
array_size (Dwarf_Die *die, Dwarf_Word *size,
	    Dwarf_Attribute *attr_mem, Dwarf_Die *type_mem)
{
  Dwarf_Word eltsize;
  if (INTUSE(dwarf_aggregate_size) (get_type (die, attr_mem, type_mem),
				    &eltsize) != 0)
      return -1;

  /* An array can have DW_TAG_subrange_type or DW_TAG_enumeration_type
     children instead that give the size of each dimension.  */

  Dwarf_Die child;
  if (INTUSE(dwarf_child) (die, &child) != 0)
    return -1;

  bool any = false;
  Dwarf_Word total = 0;
  do
    {
      Dwarf_Word count;
      switch (INTUSE(dwarf_tag) (&child))
	{
	case DW_TAG_subrange_type:
	  /* This has either DW_AT_count or DW_AT_upper_bound.  */
	  if (INTUSE(dwarf_attr_integrate) (&child, DW_AT_count,
					    attr_mem) != NULL)
	    {
	      if (INTUSE(dwarf_formudata) (attr_mem, &count) != 0)
		return -1;
	    }
	  else
	    {
	      Dwarf_Sword upper;
	      Dwarf_Sword lower;
	      if (INTUSE(dwarf_formsdata) (INTUSE(dwarf_attr_integrate)
					   (&child, DW_AT_upper_bound,
					    attr_mem), &upper) != 0)
		return -1;

	      /* Having DW_AT_lower_bound is optional.  */
	      if (INTUSE(dwarf_attr_integrate) (&child, DW_AT_lower_bound,
						attr_mem) != NULL)
		{
		  if (INTUSE(dwarf_formsdata) (attr_mem, &lower) != 0)
		    return -1;
		}
	      else
		{
		  /* Determine default lower bound from language,
		     as per "4.12 Subrange Type Entries".  */
		  Dwarf_Die cu = CUDIE (die->cu);
		  switch (INTUSE(dwarf_srclang) (&cu))
		    {
		    case DW_LANG_C:
		    case DW_LANG_C89:
		    case DW_LANG_C99:
		    case DW_LANG_C_plus_plus:
		    case DW_LANG_Objc:
		    case DW_LANG_ObjC_plus_plus:
		    case DW_LANG_Java:
		    case DW_LANG_D:
		    case DW_LANG_UPC:
		      lower = 0;
		      break;

		    case DW_LANG_Ada83:
		    case DW_LANG_Ada95:
		    case DW_LANG_Cobol74:
		    case DW_LANG_Cobol85:
		    case DW_LANG_Fortran77:
		    case DW_LANG_Fortran90:
		    case DW_LANG_Fortran95:
		    case DW_LANG_Pascal83:
		    case DW_LANG_Modula2:
		    case DW_LANG_PL1:
		      lower = 1;
		      break;

		    default:
		      return -1;
		    }
		}
	      if (unlikely (lower > upper))
		return -1;
	      count = upper - lower + 1;
	    }
	  break;

	case DW_TAG_enumeration_type:
	  /* We have to find the DW_TAG_enumerator child with the
	     highest value to know the array's element count.  */
	  count = 0;
	  Dwarf_Die enum_child;
	  int has_children = INTUSE(dwarf_child) (die, &enum_child);
	  if (has_children < 0)
	    return -1;
	  if (has_children > 0)
	    do
	      if (INTUSE(dwarf_tag) (&enum_child) == DW_TAG_enumerator)
		{
		  Dwarf_Word value;
		  if (INTUSE(dwarf_formudata) (INTUSE(dwarf_attr_integrate)
					       (&enum_child, DW_AT_const_value,
						attr_mem), &value) != 0)
		    return -1;
		  if (value >= count)
		    count = value + 1;
		}
	    while (INTUSE(dwarf_siblingof) (&enum_child, &enum_child) > 0);
	  break;

	default:
	  continue;
	}

      /* This is a subrange_type or enumeration_type and we've set COUNT.
	 Now determine the stride for this array dimension.  */
      Dwarf_Word stride = eltsize;
      if (INTUSE(dwarf_attr_integrate) (&child, DW_AT_byte_stride,
					attr_mem) != NULL)
	{
	  if (INTUSE(dwarf_formudata) (attr_mem, &stride) != 0)
	    return -1;
	}
      else if (INTUSE(dwarf_attr_integrate) (&child, DW_AT_bit_stride,
					     attr_mem) != NULL)
	{
	  if (INTUSE(dwarf_formudata) (attr_mem, &stride) != 0)
	    return -1;
	  if (stride % 8) 	/* XXX maybe compute in bits? */
	    return -1;
	  stride /= 8;
	}

      any = true;
      total += stride * count;
    }
  while (INTUSE(dwarf_siblingof) (&child, &child) == 0);

  if (!any)
    return -1;

  *size = total;
  return 0;
}

static int
aggregate_size (Dwarf_Die *die, Dwarf_Word *size, Dwarf_Die *type_mem)
{
  Dwarf_Attribute attr_mem;

  if (INTUSE(dwarf_attr_integrate) (die, DW_AT_byte_size, &attr_mem) != NULL)
    return INTUSE(dwarf_formudata) (&attr_mem, size);

  switch (INTUSE(dwarf_tag) (die))
    {
    case DW_TAG_typedef:
    case DW_TAG_subrange_type:
      return aggregate_size (get_type (die, &attr_mem, type_mem),
			     size, type_mem); /* Tail call.  */

    case DW_TAG_array_type:
      return array_size (die, size, &attr_mem, type_mem);
    }

  /* Most types must give their size directly.  */
  return -1;
}

int
dwarf_aggregate_size (die, size)
     Dwarf_Die *die;
     Dwarf_Word *size;
{
  Dwarf_Die type_mem;
  return aggregate_size (die, size, &type_mem);
}
INTDEF (dwarf_aggregate_size)
