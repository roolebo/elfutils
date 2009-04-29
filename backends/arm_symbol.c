/* Arm specific symbolic name handling.
   Copyright (C) 2002-2009 Red Hat, Inc.
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

#include <elf.h>
#include <stddef.h>

#define BACKEND		arm_
#include "libebl_CPU.h"


const char *
arm_segment_type_name (int segment, char *buf __attribute__ ((unused)),
		       size_t len __attribute__ ((unused)))
{
  switch (segment)
    {
    case PT_ARM_EXIDX:
      return "ARM_EXIDX";
    }
  return NULL;
}

/* Return symbolic representation of section type.  */
const char *
arm_section_type_name (int type,
		       char *buf __attribute__ ((unused)),
		       size_t len __attribute__ ((unused)))
{
  switch (type)
    {
    case SHT_ARM_EXIDX:
      return "ARM_EXIDX";
    case SHT_ARM_PREEMPTMAP:
      return "ARM_PREEMPTMAP";
    case SHT_ARM_ATTRIBUTES:
      return "ARM_ATTRIBUTES";
    }

  return NULL;
}

/* Check whether machine flags are valid.  */
bool
arm_machine_flag_check (GElf_Word flags)
{
  switch (flags & EF_ARM_EABIMASK)
    {
    case EF_ARM_EABI_UNKNOWN:
    case EF_ARM_EABI_VER1:
    case EF_ARM_EABI_VER2:
    case EF_ARM_EABI_VER3:
    case EF_ARM_EABI_VER4:
    case EF_ARM_EABI_VER5:
      break;
    default:
      return false;
    }

  return ((flags &~ (EF_ARM_EABIMASK
		     | EF_ARM_RELEXEC
		     | EF_ARM_HASENTRY
		     | EF_ARM_INTERWORK
		     | EF_ARM_APCS_26
		     | EF_ARM_APCS_FLOAT
		     | EF_ARM_PIC
		     | EF_ARM_ALIGN8
		     | EF_ARM_NEW_ABI
		     | EF_ARM_OLD_ABI
		     | EF_ARM_SOFT_FLOAT
		     | EF_ARM_VFP_FLOAT
		     | EF_ARM_MAVERICK_FLOAT
		     | EF_ARM_SYMSARESORTED
		     | EF_ARM_DYNSYMSUSESEGIDX
		     | EF_ARM_MAPSYMSFIRST
		     | EF_ARM_EABIMASK
		     | EF_ARM_BE8
		     | EF_ARM_LE8)) == 0);
}

/* Check for the simple reloc types.  */
Elf_Type
arm_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type)
{
  switch (type)
    {
    case R_ARM_ABS32:
      return ELF_T_WORD;
    case R_ARM_ABS16:
      return ELF_T_HALF;
    case R_ARM_ABS8:
      return ELF_T_BYTE;
    default:
      return ELF_T_NUM;
    }
}
