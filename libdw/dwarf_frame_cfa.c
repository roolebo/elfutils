/* Get CFA expression for frame.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "cfi.h"
#include <dwarf.h>
#include <stdlib.h>

int
dwarf_frame_cfa (fs, ops, nops)
     Dwarf_Frame *fs;
     Dwarf_Op **ops;
     size_t *nops;
{
  /* Maybe there was a previous error.  */
  if (fs == NULL)
    return -1;

  int result = 0;
  switch (fs->cfa_rule)
    {
    case cfa_undefined:
      *ops = NULL;
      *nops = 0;
      break;

    case cfa_offset:
      /* The Dwarf_Op was already fully initialized by execute_cfi.  */
      *ops = &fs->cfa_data.offset;
      *nops = 1;
      break;

    case cfa_expr:
      /* Parse the expression into internal form.  */
      result = __libdw_intern_expression
	(NULL, fs->cache->other_byte_order,
	 fs->cache->e_ident[EI_CLASS] == ELFCLASS32 ? 4 : 8, 4,
	 &fs->cache->expr_tree, &fs->cfa_data.expr, false, false,
	 ops, nops, IDX_debug_frame);
      break;

    case cfa_invalid:
      __libdw_seterrno (DWARF_E_INVALID_CFI);
      result = -1;
      break;

    default:
      abort ();
    }

  return result;
}
