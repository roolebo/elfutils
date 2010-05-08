/* Get register location expression for frame.
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

int
dwarf_frame_register (fs, regno, ops_mem, ops, nops)
     Dwarf_Frame *fs;
     int regno;
     Dwarf_Op ops_mem[3];
     Dwarf_Op **ops;
     size_t *nops;
{
  /* Maybe there was a previous error.  */
  if (fs == NULL)
    return -1;

  if (unlikely (regno < 0))
    {
      __libdw_seterrno (DWARF_E_INVALID_ACCESS);
      return -1;
    }

  *ops = ops_mem;
  *nops = 0;

  if (unlikely ((size_t) regno >= fs->nregs))
    goto default_rule;

  const struct dwarf_frame_register *reg = &fs->regs[regno];

  switch (reg->rule)
    {
    case reg_unspecified:
    default_rule:
      /* Use the default rule for registers not yet mentioned in CFI.  */
      if (fs->cache->default_same_value)
	goto same_value;
      /*FALLTHROUGH*/
    case reg_undefined:
      /* The value is known to be unavailable.  */
      break;

    case reg_same_value:
    same_value:
      /* The location is not known here, but the caller might know it.  */
      *ops = NULL;
      break;

    case reg_offset:
    case reg_val_offset:
      ops_mem[(*nops)++] = (Dwarf_Op) { .atom = DW_OP_call_frame_cfa };
      if (reg->value != 0)
	ops_mem[(*nops)++] = (Dwarf_Op) { .atom = DW_OP_plus_uconst,
					  .number = reg->value };
      if (reg->rule == reg_val_offset)
	/* A value, not a location.  */
	ops_mem[(*nops)++] = (Dwarf_Op) { .atom = DW_OP_stack_value };
      *ops = ops_mem;
      break;

    case reg_register:
      ops_mem[(*nops)++] = (Dwarf_Op) { .atom = DW_OP_regx,
					.number = reg->value };
      break;

    case reg_val_expression:
    case reg_expression:
      {
	unsigned int address_size = (fs->cache->e_ident[EI_CLASS] == ELFCLASS32
				     ? 4 : 8);

	Dwarf_Block block;
	const uint8_t *p = fs->cache->data->d.d_buf + reg->value;
	get_uleb128 (block.length, p);
	block.data = (void *) p;

	/* Parse the expression into internal form.  */
	if (__libdw_intern_expression (NULL,
				       fs->cache->other_byte_order,
				       address_size, 4,
				       &fs->cache->expr_tree, &block,
				       true, reg->rule == reg_val_expression,
				       ops, nops, IDX_debug_frame) < 0)
	  return -1;
	break;
      }
    }

  return 0;
}
