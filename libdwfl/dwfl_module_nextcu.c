/* Iterate through DWARF compilation units in a module.
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

Dwarf_Die *
dwfl_module_nextcu (Dwfl_Module *mod, Dwarf_Die *lastcu, Dwarf_Addr *bias)
{
  if (INTUSE(dwfl_module_getdwarf) (mod, bias) == NULL)
    return NULL;

  struct dwfl_cu *cu;
  Dwfl_Error error = __libdwfl_nextcu (mod, (struct dwfl_cu *) lastcu, &cu);
  if (likely (error == DWFL_E_NOERROR))
    return &cu->die;		/* Same as a cast, so ok for null too.  */

  __libdwfl_seterrno (error);
  return NULL;
}
