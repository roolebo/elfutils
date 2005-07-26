/* Get macro information.
   Copyright (C) 2002, 2003, 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <dwarf.h>
#include <string.h>

#include <libdwP.h>


ptrdiff_t
dwarf_getmacros (die, callback, arg, offset)
     Dwarf_Die *die;
     int (*callback) (Dwarf_Macro *, void *);
     void *arg;
     ptrdiff_t offset;
{
  /* Get the appropriate attribute.  */
  Dwarf_Attribute attr;
  if (INTUSE(dwarf_attr) (die, DW_AT_macro_info, &attr) == NULL)
    return -1;

  /* Offset into the .debug_macinfo section.  */
  Dwarf_Word macoff;
  if (INTUSE(dwarf_formudata) (&attr, &macoff) != 0)
    return -1;

  const unsigned char *readp
    = die->cu->dbg->sectiondata[IDX_debug_macinfo]->d_buf + offset;
  const unsigned char *readendp
    = readp + die->cu->dbg->sectiondata[IDX_debug_macinfo]->d_size;

  if (readp == readendp)
    return 0;

  if (*readp != DW_MACINFO_start_file)
    goto invalid;

  while (readp < readendp)
    {
      unsigned int opcode = *readp++;
      unsigned int u128;
      unsigned int u128_2 = 0;
      const char *str = NULL;
      const unsigned char *endp;

      switch (opcode)
	{
	case DW_MACINFO_define:
	case DW_MACINFO_undef:
	case DW_MACINFO_vendor_ext:
	  /*  For the first two opcodes the parameters are
	        line, string
	      For the latter
	        number, string.
	      We can treat these cases together.  */
	  get_uleb128 (u128, readp);

	  endp = memchr (readp, '\0', readendp - readp);
	  if (endp == NULL)
	    goto invalid;

	  str = (char *) readp;
	  readp = endp + 1;
	  break;

	case DW_MACINFO_start_file:
	  /* The two parameters are line and file index.  */
	  get_uleb128 (u128, readp);
	  get_uleb128 (u128_2, readp);
	  break;

	case DW_MACINFO_end_file:
	  /* No parameters for this one.  */
	  u128 = 0;
	  break;

	case 0:
	  /* Nothing more to do.  */
	  return 0;

	default:
	  goto invalid;
	}

      Dwarf_Macro mac;
      mac.opcode = opcode;
      mac.param1 = u128;
      if (str == NULL)
	mac.param2.u = u128_2;
      else
	mac.param2.s = str;

      if (callback (&mac, arg) != DWARF_CB_OK)
	return (readp
		- ((unsigned char *) die->cu->dbg->sectiondata[IDX_debug_macinfo]->d_buf
		   + offset));
    }

  /* If we come here the termination of the data for the CU is not
     present.  */
 invalid:
  __libdw_seterrno (DWARF_E_INVALID_DWARF);
  return -1;
}
