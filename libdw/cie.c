/* CIE reading.
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
#include "encoded-value.h"
#include <assert.h>
#include <search.h>
#include <stdlib.h>


static int
compare_cie (const void *a, const void *b)
{
  const struct dwarf_cie *cie1 = a;
  const struct dwarf_cie *cie2 = b;
  if (cie1->offset < cie2->offset)
    return -1;
  if (cie1->offset > cie2->offset)
    return 1;
  return 0;
}

/* There is no CIE at OFFSET in the tree.  Add it.  */
static struct dwarf_cie *
intern_new_cie (Dwarf_CFI *cache, Dwarf_Off offset, const Dwarf_CIE *info)
{
  struct dwarf_cie *cie = malloc (sizeof (struct dwarf_cie));
  if (cie == NULL)
    {
      __libdw_seterrno (DWARF_E_NOMEM);
      return NULL;
    }

  cie->offset = offset;
  cie->code_alignment_factor = info->code_alignment_factor;
  cie->data_alignment_factor = info->data_alignment_factor;
  cie->return_address_register = info->return_address_register;

  cie->fde_augmentation_data_size = 0;
  cie->sized_augmentation_data = false;
  cie->signal_frame = false;

  cie->fde_encoding = DW_EH_PE_absptr;
  cie->lsda_encoding = DW_EH_PE_omit;

  /* Grok the augmentation string and its data.  */
  const uint8_t *data = info->augmentation_data;
  for (const char *ap = info->augmentation; *ap != '\0'; ++ap)
    {
      uint8_t encoding;
      switch (*ap)
	{
	case 'z':
	  cie->sized_augmentation_data = true;
	  continue;

	case 'S':
	  cie->signal_frame = true;
	  continue;

	case 'L':		/* LSDA pointer encoding byte.  */
	  cie->lsda_encoding = *data++;
	  if (!cie->sized_augmentation_data)
	    cie->fde_augmentation_data_size
	      += encoded_value_size (&cache->data->d, cache->e_ident,
				     cie->lsda_encoding, NULL);
	  continue;

	case 'R':		/* FDE address encoding byte.  */
	  cie->fde_encoding = *data++;
	  continue;

	case 'P':		/* Skip personality routine.  */
	  encoding = *data++;
	  data += encoded_value_size (&cache->data->d, cache->e_ident,
				      encoding, data);
	  continue;

	default:
	  /* Unknown augmentation string.  If we have 'z' we can ignore it,
	     otherwise we must bail out.  */
	  if (cie->sized_augmentation_data)
	    continue;
	}
      /* We only get here when we need to bail out.  */
      break;
    }

  if ((cie->fde_encoding & 0x0f) == DW_EH_PE_absptr)
    {
      /* Canonicalize encoding to a specific size.  */
      assert (DW_EH_PE_absptr == 0);

      /* XXX should get from dwarf_next_cfi with v4 header.  */
      uint_fast8_t address_size
	= cache->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;
      switch (address_size)
	{
	case 8:
	  cie->fde_encoding |= DW_EH_PE_udata8;
	  break;
	case 4:
	  cie->fde_encoding |= DW_EH_PE_udata4;
	  break;
	default:
	  free (cie);
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return NULL;
	}
    }

  /* Save the initial instructions to be played out into initial state.  */
  cie->initial_instructions = info->initial_instructions;
  cie->initial_instructions_end = info->initial_instructions_end;
  cie->initial_state = NULL;

  /* Add the new entry to the search tree.  */
  if (tsearch (cie, &cache->cie_tree, &compare_cie) == NULL)
    {
      free (cie);
      __libdw_seterrno (DWARF_E_NOMEM);
      return NULL;
    }

  return cie;
}

/* Look up a CIE_pointer for random access.  */
struct dwarf_cie *
internal_function
__libdw_find_cie (Dwarf_CFI *cache, Dwarf_Off offset)
{
  const struct dwarf_cie cie_key = { .offset = offset };
  struct dwarf_cie **found = tfind (&cie_key, &cache->cie_tree, &compare_cie);
  if (found != NULL)
    return *found;

  /* We have not read this CIE yet.  Go find it.  */
  Dwarf_Off next_offset = offset;
  Dwarf_CFI_Entry entry;
  int result = INTUSE(dwarf_next_cfi) (cache->e_ident,
				       &cache->data->d, CFI_IS_EH (cache),
				       offset, &next_offset, &entry);
  if (result != 0 || entry.cie.CIE_id != DW_CIE_ID_64)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  /* If this happened to be what we would have read next, notice it.  */
  if (cache->next_offset == offset)
    cache->next_offset = next_offset;

  return intern_new_cie (cache, offset, &entry.cie);
}

/* Enter a CIE encountered while reading through for FDEs.  */
void
internal_function
__libdw_intern_cie (Dwarf_CFI *cache, Dwarf_Off offset, const Dwarf_CIE *info)
{
  const struct dwarf_cie cie_key = { .offset = offset };
  struct dwarf_cie **found = tfind (&cie_key, &cache->cie_tree, &compare_cie);
  if (found == NULL)
    /* We have not read this CIE yet.  Enter it.  */
    (void) intern_new_cie (cache, offset, info);
}
