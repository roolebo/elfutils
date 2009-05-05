/* ARM specific core note handling.
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

#include <elf.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>

#define BACKEND arm_
#include "libebl_CPU.h"


static const Ebl_Register_Location prstatus_regs[] =
  {
    { .offset = 0, .regno = 0, .count = 16, .bits = 32 },	/* r0..r15 */
    { .offset = 16 * 4, .regno = 128, .count = 1, .bits = 32 }, /* cpsr */
  };
#define PRSTATUS_REGS_SIZE	(18 * 4)

#define PRSTATUS_REGSET_ITEMS						      \
  {									      \
    .name = "orig_r0", .type = ELF_T_SWORD, .format = 'd',		      \
    .offset = offsetof (struct EBLHOOK(prstatus), pr_reg) + (4 * 17),	      \
    .group = "register"	       			  	       	 	      \
  }

static const Ebl_Register_Location fpregset_regs[] =
  {
    { .offset = 0, .regno = 96, .count = 8, .bits = 96 }, /* f0..f7 */
  };
#define FPREGSET_SIZE	140

#define	ULONG			uint32_t
#define PID_T			int32_t
#define	UID_T			uint16_t
#define	GID_T			uint16_t
#define ALIGN_ULONG		4
#define ALIGN_PID_T		4
#define ALIGN_UID_T		2
#define ALIGN_GID_T		2
#define TYPE_ULONG		ELF_T_WORD
#define TYPE_PID_T		ELF_T_SWORD
#define TYPE_UID_T		ELF_T_HALF
#define TYPE_GID_T		ELF_T_HALF

#include "linux-core-note.c"
