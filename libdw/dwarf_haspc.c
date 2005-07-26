/* Determine whether a DIE covers a PC address.
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


int
dwarf_haspc (Dwarf_Die *die, Dwarf_Addr pc)
{
  if (die == NULL)
    return -1;

  /* Usually there is a single contiguous range.  */
  Dwarf_Addr lowpc, highpc;
  if (INTUSE(dwarf_highpc) (die, &highpc) == 0
      && INTUSE(dwarf_lowpc) (die, &lowpc) == 0)
    return pc >= lowpc && pc < highpc;

  /* We have to look for a noncontiguous range.  */
  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = INTUSE(dwarf_attr) (die, DW_AT_ranges, &attr_mem);
  if (attr == NULL)
    return -1;

  /* Must have the form data4 or data8 which act as an offset.  */
  Dwarf_Word offset;
  if (INTUSE(dwarf_formudata) (attr, &offset) != 0)
    return -1;

  const Elf_Data *d = attr->cu->dbg->sectiondata[IDX_debug_ranges];
  if (d == NULL)
    {
      __libdw_seterrno (DWARF_E_NO_DEBUG_RANGES);
      return -1;
    }

  /* Fetch the CU's base address.  */
  Dwarf_Addr base;
  Dwarf_Die cudie =
    {
      .cu = attr->cu,
      .addr = ((char *) attr->cu->dbg->sectiondata[IDX_debug_info]->d_buf
	       + attr->cu->start + 3 * attr->cu->offset_size - 4 + 3),
    };
  if (INTUSE(dwarf_lowpc) (&cudie, &base) != 0)
    return -1;

  unsigned char *readp = d->d_buf + offset;
  Dwarf_Addr begin;
  Dwarf_Addr end;
  do
    {
    next:
      if ((unsigned char *) d->d_buf + d->d_size - readp
	  < attr->cu->address_size * 2)
	{
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return -1;
	}

      if (attr->cu->address_size == 8)
	{
	  begin = read_8ubyte_unaligned_inc (attr->cu->dbg, readp);
	  end = read_8ubyte_unaligned_inc (attr->cu->dbg, readp);
	}
      else
	{
	  begin = (Dwarf_Sword) read_4sbyte_unaligned_inc (attr->cu->dbg,
							   readp);
	  end = read_4ubyte_unaligned_inc (attr->cu->dbg, readp);
	}

      if (begin == (Dwarf_Addr) -1l) /* Base address entry.  */
	{
	  base = end;
	  goto next;
	}

      if (begin == 0 && end == 0) /* End of list entry.  */
	/* This is not the droid you are looking for.  */
	return 0;

      /* We have an address range entry.  */
    }
  while (pc < base + begin || pc >= base + end);

  /* This one matches the address.  */
  return 1;
}
INTDEF (dwarf_haspc)
