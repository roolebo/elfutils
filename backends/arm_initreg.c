/* Fetch live process registers from TID.
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if defined __arm__
# include <sys/types.h>
# include <sys/user.h>
# include <sys/ptrace.h>
#endif

#define BACKEND arm_
#include "libebl_CPU.h"

bool
arm_set_initial_registers_tid (pid_t tid __attribute__ ((unused)),
			  ebl_tid_registers_t *setfunc __attribute__ ((unused)),
			       void *arg __attribute__ ((unused)))
{
#if ! defined __arm__
  return false;
#else
  struct user_regs user_regs;
  if (ptrace (PTRACE_GETREGS, tid, NULL, &user_regs) != 0)
    return false;
  Dwarf_Word dwarf_regs[16];
  for (int i = 0; i < 16; i++)
    dwarf_regs[i] = user_regs.uregs[i];
  return setfunc (0, 16, dwarf_regs, arg);
#endif
}
