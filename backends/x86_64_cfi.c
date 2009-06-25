/* x86-64 ABI-specified defaults for DWARF CFI.
   Copyright (C) 2009 Red Hat, Inc.
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

#include <dwarf.h>

#define BACKEND x86_64_
#include "libebl_CPU.h"

int
x86_64_abi_cfi (Ebl *ebl __attribute__ ((unused)), Dwarf_CIE *abi_info)
{
  static const uint8_t abi_cfi[] =
    {
      /* Call-saved regs.  */
      DW_CFA_same_value, ULEB128_7 (0), /* %rbx */
      DW_CFA_same_value, ULEB128_7 (6), /* %rbp */
      DW_CFA_same_value, ULEB128_7 (12), /* %r12 */
      DW_CFA_same_value, ULEB128_7 (13), /* %r13 */
      DW_CFA_same_value, ULEB128_7 (14), /* %r14 */
      DW_CFA_same_value, ULEB128_7 (15), /* %r15 */
      DW_CFA_same_value, ULEB128_7 (16), /* %r16 */

      /* The CFA is the SP.  */
      DW_CFA_val_offset, ULEB128_7 (7), ULEB128_7 (0),
    };

  abi_info->initial_instructions = abi_cfi;
  abi_info->initial_instructions_end = &abi_cfi[sizeof abi_cfi];
  abi_info->data_alignment_factor = 8;

  abi_info->return_address_register = 16; /* %rip */

  return 0;
}
