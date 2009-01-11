/* Error handling in libasm.
   Copyright (C) 2002, 2004, 2005, 2009 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <libintl.h>
#include <stdbool.h>
#include <stdlib.h>

#include "libasmP.h"


/* This is the key for the thread specific memory.  */
static __thread int global_error;


int
asm_errno (void)
{
  int result = global_error;
  global_error = ASM_E_NOERROR;
  return result;
}


void
__libasm_seterrno (value)
     int value;
{
  global_error = value;
}


/* Return the appropriate message for the error.  */
static const char *msgs[ASM_E_NUM] =
{
  [ASM_E_NOERROR] = N_("no error"),
  [ASM_E_NOMEM] = N_("out of memory"),
  [ASM_E_CANNOT_CREATE] = N_("cannot create output file"),
  [ASM_E_INVALID] = N_("invalid parameter"),
  [ASM_E_CANNOT_CHMOD] = N_("cannot change mode of output file"),
  [ASM_E_CANNOT_RENAME] = N_("cannot rename output file"),
  [ASM_E_DUPLSYM] = N_("duplicate symbol"),
  [ASM_E_TYPE] = N_("invalid section type for operation"),
  [ASM_E_IOERROR] = N_("error during output of data"),
  [ASM_E_ENOSUP] = N_("no backend support available"),
};

const char *
asm_errmsg (error)
     int error;
{
  int last_error = global_error;

  if (error < -1)
    return _("unknown error");
  if (error == 0 && last_error == 0)
    /* No error.  */
    return NULL;

  if (error != -1)
    last_error = error;

  if (last_error == ASM_E_LIBELF)
    return elf_errmsg (-1);

  return _(msgs[last_error]);
}
