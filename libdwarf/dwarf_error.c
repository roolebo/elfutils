/* Handle error.
   Copyright (C) 2000, 2002 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

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
