/* Handle error.
   Copyright (C) 2000, 2002 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

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

#include <assert.h>
#include <stdlib.h>

#include <libdwarfP.h>
#include <system.h>


void
internal_function
__libdwarf_error (Dwarf_Debug dbg, Dwarf_Error *error, int errval)
{
  /* Allocate memory for the error structure given to the user.  */
  Dwarf_Error errmem = (Dwarf_Error) malloc (sizeof (*error));
  /* We cannot report an error if we cannot allocate memory.  */
  if (errmem == NULL)
    return;

  errmem->de_error = errval;

  /* DBG must never be NULL.  */
  assert (dbg != NULL);

  if (error != NULL)
    /* If the user provides an ERROR parameter we have to use it.  */
    *error = errmem;
  else if (likely (dbg->dbg_errhand != NULL))
    /* Use the handler the user provided if possible.  */
    dbg->dbg_errhand (error, dbg->dbg_errarg);
  else
    {
      assert (! "error and dbg->dbg_errhand == NULL");
      abort ();
    }
}
