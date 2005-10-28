/* Enumerate the PC ranges covered by a DIE.
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

#include "libdwP.h"
#include <dwarf.h>
#include <assert.h>


ptrdiff_t
dwarf_ranges (Dwarf_Die *die, ptrdiff_t offset, Dwarf_Addr *basep,
	      Dwarf_Addr *startp, Dwarf_Addr *endp)
{
  if (die == NULL)
    return -1;

  if (offset == 0
      /* Usually there is a single contiguous range.  */
      && INTUSE(dwarf_highpc) (die, endp) == 0
      && INTUSE(dwarf_lowpc) (die, startp) == 0)
    /* A offset into .debug_ranges will never be 1, it must be at least a
       multiple of 4.  So we can return 1 as a special case value to mark
       there are no ranges to look for on the next call.  */
    return 1;

  if (offset == 1)
    return 0;

  /* We have to look for a noncontiguous range.  */

  const Elf_Data *d = die->cu->dbg->sectiondata[IDX_debug_ranges];
  if (d == NULL)
    {
      __libdw_seterrno (DWARF_E_NO_DEBUG_RANGES);
      return -1;
    }

  if (offset == 0)
    {
      Dwarf_Attribute attr_mem;
      Dwarf_Attribute *attr = INTUSE(dwarf_attr) (die, DW_AT_ranges,
						  &attr_mem);
      if (attr == NULL)
	return -1;

      /* Must have the form data4 or data8 which act as an offset.  */
      Dwarf_Word start_offset;
      if (INTUSE(dwarf_formudata) (attr, &start_offset) != 0)
	return -1;

      offset = start_offset;
      assert ((Dwarf_Word) offset == start_offset);

      /* Fetch the CU's base address.  */
      if (INTUSE(dwarf_lowpc) (&CUDIE (attr->cu), basep) != 0)
	return -1;
    }
  else if (offset < 0 || (size_t) offset >= d->d_size)
    {
      __libdw_seterrno (DWARF_E_INVALID_OFFSET);
      return -1l;
    }

  unsigned char *readp = d->d_buf + offset;

 next:
  if ((unsigned char *) d->d_buf + d->d_size - readp
      < die->cu->address_size * 2)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

  Dwarf_Addr begin;
  Dwarf_Addr end;
  if (die->cu->address_size == 8)
    {
      begin = read_8ubyte_unaligned_inc (die->cu->dbg, readp);
      end = read_8ubyte_unaligned_inc (die->cu->dbg, readp);
    }
  else
    {
      begin = (Dwarf_Sword) read_4sbyte_unaligned_inc (die->cu->dbg,
						       readp);
      end = read_4ubyte_unaligned_inc (die->cu->dbg, readp);
    }

  if (begin == (Dwarf_Addr) -1l) /* Base address entry.  */
    {
      *basep = end;
      goto next;
    }

  if (begin == 0 && end == 0) /* End of list entry.  */
    return 0;

  /* We have an address range entry.  */
  *startp = *basep + begin;
  *endp = *basep + end;
  return readp - (unsigned char *) d->d_buf;
}
INTDEF (dwarf_ranges)
