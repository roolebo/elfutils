/* Enumerate DWARF register numbers and their names.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include "libdwflP.h"


int
dwfl_module_register_names (mod, func, arg)
     Dwfl_Module *mod;
     int (*func) (void *, int regno, const char *setname,
		  const char *prefix, const char *regname);
     void *arg;
{
  if (unlikely (mod == NULL))
    return -1;

  if (unlikely (mod->ebl == NULL))
    {
      Dwfl_Error error = __libdwfl_module_getebl (mod);
      if (error != DWFL_E_NOERROR)
	{
	  __libdwfl_seterrno (error);
	  return -1;
	}
    }

  int nregs = ebl_register_name (mod->ebl, -1, NULL, 0, NULL, NULL);
  int result = 0;
  for (int regno = 0; regno < nregs && likely (result == 0); ++regno)
    {
      char name[32];
      const char *setname = NULL;
      const char *prefix = NULL;
      ssize_t len = ebl_register_name (mod->ebl, regno, name, sizeof name,
				       &prefix, &setname);
      if (unlikely (len < 0))
	{
	  __libdwfl_seterrno (DWFL_E_LIBEBL);
	  result = -1;
	  break;
	}
      if (likely (len > 0))
	{
	  assert (len > 1);	/* Backend should never yield "".  */
	  result = (*func) (arg, regno, setname, prefix, name);
	}
    }

  return result;
}
