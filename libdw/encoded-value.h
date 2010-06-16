/* DW_EH_PE_* support for libdw unwinder.
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

#ifndef _ENCODED_VALUE_H
#define _ENCODED_VALUE_H 1

#include <dwarf.h>
#include <stdlib.h>
#include "libdwP.h"


static size_t __attribute__ ((unused))
encoded_value_size (const Elf_Data *data, const unsigned char e_ident[],
		    uint8_t encoding, const uint8_t *p)
{
  if (encoding == DW_EH_PE_omit)
    return 0;

  switch (encoding & 0x07)
    {
    case DW_EH_PE_udata2:
      return 2;
    case DW_EH_PE_udata4:
      return 4;
    case DW_EH_PE_udata8:
      return 8;

    case DW_EH_PE_absptr:
      return e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;

    case DW_EH_PE_uleb128:
      if (p != NULL)
	{
	  const uint8_t *end = p;
	  while (end < (uint8_t *) data->d_buf + data->d_size)
	    if (*end++ & 0x80u)
	      return end - p;
	}

    default:
      abort ();
      return 0;
    }
}

static inline int __attribute__ ((unused))
__libdw_cfi_read_address_inc (const Dwarf_CFI *cache,
			      const unsigned char **addrp,
			      int width, Dwarf_Addr *ret)
{
  width = width ?: cache->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8;

  if (cache->dbg != NULL)
    return __libdw_read_address_inc (cache->dbg, IDX_debug_frame,
				     addrp, width, ret);

  /* Only .debug_frame might have relocation to consider.
     Read plain values from .eh_frame data.  */

  if (width == 4)
    *ret = read_4ubyte_unaligned_inc (cache, *addrp);
  else
    *ret = read_8ubyte_unaligned_inc (cache, *addrp);
  return 0;
}

static bool __attribute__ ((unused))
read_encoded_value (const Dwarf_CFI *cache, uint8_t encoding, const uint8_t **p,
		    Dwarf_Addr *result)
{
  *result = 0;
  switch (encoding & 0x70)
    {
    case DW_EH_PE_absptr:
      break;
    case DW_EH_PE_pcrel:
      *result = (cache->frame_vaddr
		 + (*p - (const uint8_t *) cache->data->d.d_buf));
      break;
    case DW_EH_PE_textrel:
      // ia64: segrel
      *result = cache->textrel;
      break;
    case DW_EH_PE_datarel:
      // i386: GOTOFF
      // ia64: gprel
      *result = cache->datarel;
      break;
    case DW_EH_PE_funcrel:	/* XXX */
      break;
    case DW_EH_PE_aligned:
      {
	const size_t size = encoded_value_size (&cache->data->d, cache->e_ident,
						encoding, *p);
	size_t align = ((cache->frame_vaddr
			 + (*p - (const uint8_t *) cache->data->d.d_buf))
			& (size - 1));
	if (align != 0)
	  *p += size - align;
	break;
      }

    default:
      abort ();
    }

  Dwarf_Addr value;
  switch (encoding & 0x0f)
    {
    case DW_EH_PE_udata2:
      value = read_2ubyte_unaligned_inc (cache, *p);
      break;

    case DW_EH_PE_sdata2:
      value = read_2sbyte_unaligned_inc (cache, *p);
      break;

    case DW_EH_PE_udata4:
      if (__libdw_cfi_read_address_inc (cache, p, 4, &value))
	return true;
      break;

    case DW_EH_PE_sdata4:
      if (__libdw_cfi_read_address_inc (cache, p, 4, &value))
	return true;
      value = (Dwarf_Sword) (Elf32_Sword) value; /* Sign-extend.  */
      break;

    case DW_EH_PE_udata8:
    case DW_EH_PE_sdata8:
      if (__libdw_cfi_read_address_inc (cache, p, 8, &value))
	return true;
      break;

    case DW_EH_PE_absptr:
      if (__libdw_cfi_read_address_inc (cache, p, 0, &value))
	return true;
      break;

    case DW_EH_PE_uleb128:
      get_uleb128 (value, *p);
      break;

    case DW_EH_PE_sleb128:
      get_sleb128 (value, *p);
      break;

    default:
      abort ();
    }

  *result += value;

  if (encoding & DW_EH_PE_indirect)
    {
      if (unlikely (*result < cache->frame_vaddr))
	return true;
      *result -= cache->frame_vaddr;
      if (unlikely (*result > (cache->data->d.d_size
			       - encoded_value_size (NULL, cache->e_ident,
						     DW_EH_PE_absptr, NULL))))
	return true;
      const uint8_t *ptr = cache->data->d.d_buf + *result;
      return __libdw_cfi_read_address_inc (cache, &ptr, 0, result);
    }

  return false;
}

#endif	/* encoded-value.h */
