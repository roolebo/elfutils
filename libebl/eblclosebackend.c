/* Free ELF backend handle.
   Copyright (C) 2000, 2001, 2002 Red Hat, Inc.

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

#include <dlfcn.h>
#include <stdlib.h>

#include <libeblP.h>


void
ebl_closebackend (Ebl *ebl)
{
  if (ebl != NULL)
    {
      /* Run the destructor.  */
      ebl->destr (ebl);

      /* Close the dynamically loaded object.  */
      if (ebl->dlhandle != NULL)
	(void) dlclose (ebl->dlhandle);

      /* Free the resources.  */
      free (ebl);
    }
}
