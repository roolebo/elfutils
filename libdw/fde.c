/* FDE reading.
   Copyright (C) 2009-2010 Red Hat, Inc.
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

#include "cfi.h"
#include <search.h>
#include <stdlib.h>

#include "encoded-value.h"

static int
compare_fde (const void *a, const void *b)
{
  const struct dwarf_fde *fde1 = a;
  const struct dwarf_fde *fde2 = b;

  /* Find out which of the two arguments is the search value.
     It has end offset 0.  */
  if (fde1->end == 0)
    {
      if (fde1->start < fde2->start)
	return -1;
      if (fde1->start >= fde2->end)
	return 1;
    }
  else
    {
      if (fde2->start < fde1->start)
	return 1;
      if (fde2->start >= fde1->end)
	return -1;
    }

  return 0;
}

static struct dwarf_fde *
intern_fde (Dwarf_CFI *cache, const Dwarf_FDE *entry)
{
  /* Look up the new entry's CIE.  */
  struct dwarf_cie *cie = __libdw_find_cie (cache, entry->CIE_pointer);
  if (cie == NULL)
    return (void *) -1l;

  struct dwarf_fde *fde = malloc (sizeof (struct dwarf_fde));
  if (fde == NULL)
    {
      __libdw_seterrno (DWARF_E_NOMEM);
      return NULL;
    }

  fde->instructions = entry->start;
  fde->instructions_end = entry->end;
  if (unlikely (read_encoded_value (cache, cie->fde_encoding,
				    &fde->instructions, &fde->start))
      || unlikely (read_encoded_value (cache, cie->fde_encoding & 0x0f,
				       &fde->instructions, &fde->end)))
    return NULL;
  fde->end += fde->start;

  fde->cie = cie;

  if (cie->sized_augmentation_data)
    {
      /* The CIE augmentation says the FDE has a DW_FORM_block
	 before its actual instruction stream.  */
      Dwarf_Word len;
      get_uleb128 (len, fde->instructions);
      if ((Dwarf_Word) (fde->instructions_end - fde->instructions) < len)
	{
	  free (fde);
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return NULL;
	}
      fde->instructions += len;
    }
  else
    /* We had to understand all of the CIE augmentation string.
       We've recorded the number of data bytes in FDEs.  */
    fde->instructions += cie->fde_augmentation_data_size;

  /* Add the new entry to the search tree.  */
  if (tsearch (fde, &cache->fde_tree, &compare_fde) == NULL)
    {
      free (fde);
      __libdw_seterrno (DWARF_E_NOMEM);
      return NULL;
    }

  return fde;
}

struct dwarf_fde *
internal_function
__libdw_fde_by_offset (Dwarf_CFI *cache, Dwarf_Off offset)
{
  Dwarf_CFI_Entry entry;
  Dwarf_Off next_offset;
  int result = INTUSE(dwarf_next_cfi) (cache->e_ident,
				       &cache->data->d, CFI_IS_EH (cache),
				       offset, &next_offset, &entry);
  if (result != 0)
    {
      if (result > 0)
      invalid:
	__libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  if (unlikely (dwarf_cfi_cie_p (&entry)))
    goto invalid;

  /* We have a new FDE to consider.  */
  struct dwarf_fde *fde = intern_fde (cache, &entry.fde);
  if (fde == (void *) -1l || fde == NULL)
    return NULL;

  /* If this happened to be what we would have read next, notice it.  */
  if (cache->next_offset == offset)
    cache->next_offset = next_offset;

  return fde;
}

/* Use a binary search table in .eh_frame_hdr format, yield an FDE offset.  */
static Dwarf_Off
binary_search_fde (Dwarf_CFI *cache, Dwarf_Addr address)
{
  const size_t size = 2 * encoded_value_size (&cache->data->d, cache->e_ident,
					      cache->search_table_encoding,
					      NULL);

  /* Dummy used by read_encoded_value.  */
  Dwarf_CFI dummy_cfi =
    {
      .e_ident = cache->e_ident,
      .datarel = cache->search_table_vaddr,
      .frame_vaddr = cache->search_table_vaddr,
    };

  size_t l = 0, u = cache->search_table_entries;
  while (l < u)
    {
      size_t idx = (l + u) / 2;

      const uint8_t *p = &cache->search_table[idx * size];
      Dwarf_Addr start;
      if (unlikely (read_encoded_value (&dummy_cfi,
					cache->search_table_encoding, &p,
					&start)))
	break;
      if (address < start)
	u = idx;
      else
	{
	  Dwarf_Addr fde;
	  if (unlikely (read_encoded_value (&dummy_cfi,
					    cache->search_table_encoding, &p,
					    &fde)))
	    break;
	  if (address >= start)
	    {
	      l = idx + 1;

	      /* If this is the last entry, its upper bound is assumed to be
		 the end of the module.
		 XXX really should be end of containing PT_LOAD segment */
	      if (l < cache->search_table_entries)
		{
		  /* Look at the start address in the following entry.  */
		  Dwarf_Addr end;
		  if (unlikely (read_encoded_value
				(&dummy_cfi, cache->search_table_encoding, &p,
				 &end)))
		    break;
		  if (address >= end)
		    continue;
		}

	      return fde - cache->frame_vaddr;
	    }
	}
    }

  return (Dwarf_Off) -1l;
}

struct dwarf_fde *
internal_function
__libdw_find_fde (Dwarf_CFI *cache, Dwarf_Addr address)
{
  /* Look for a cached FDE covering this address.  */

  const struct dwarf_fde fde_key = { .start = address, .end = 0 };
  struct dwarf_fde **found = tfind (&fde_key, &cache->fde_tree, &compare_fde);
  if (found != NULL)
    return *found;

  /* Use .eh_frame_hdr binary search table if possible.  */
  if (cache->search_table != NULL)
    {
      Dwarf_Off offset = binary_search_fde (cache, address);
      if (offset == (Dwarf_Off) -1l)
	goto no_match;
      struct dwarf_fde *fde = __libdw_fde_by_offset (cache, offset);
      if (unlikely (fde != NULL)
	  /* Sanity check the address range.  */
	  && unlikely (address < fde->start || address >= fde->end))
	{
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return NULL;
	}
      return fde;
    }

  /* It's not there.  Read more CFI entries until we find it.  */
  while (1)
    {
      Dwarf_Off last_offset = cache->next_offset;
      Dwarf_CFI_Entry entry;
      int result = INTUSE(dwarf_next_cfi) (cache->e_ident,
					   &cache->data->d, CFI_IS_EH (cache),
					   last_offset, &cache->next_offset,
					   &entry);
      if (result > 0)
	break;
      if (result < 0)
	{
	  if (cache->next_offset == last_offset)
	    /* We couldn't progress past the bogus FDE.  */
	    break;
	  /* Skip the loser and look at the next entry.  */
	  continue;
	}

      if (dwarf_cfi_cie_p (&entry))
	{
	  /* This is a CIE, not an FDE.  We eagerly intern these
	     because the next FDE will usually refer to this CIE.  */
	  __libdw_intern_cie (cache, last_offset, &entry.cie);
	  continue;
	}

      /* We have a new FDE to consider.  */
      struct dwarf_fde *fde = intern_fde (cache, &entry.fde);

      if (fde == (void *) -1l)	/* Bad FDE, but we can keep looking.  */
	continue;

      if (fde == NULL)		/* Bad data.  */
	return NULL;

      /* Is this the one we're looking for?  */
      if (fde->start <= address && fde->end > address)
	return fde;
    }

 no_match:
  /* We found no FDE covering this address.  */
  __libdw_seterrno (DWARF_E_NO_MATCH);
  return NULL;
}
