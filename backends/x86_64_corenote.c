/* x86-64 specific core note handling.
   Copyright (C) 2005 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#define BACKEND		x86_64_
#include "libebl_CPU.h"


/* We cannot include <sys/procfs.h> since the definition would be for
   the host platform and not always x86-64 as required here.  */
struct elf_prstatus
  {
    struct
    {
      int32_t si_signo;			/* Signal number.  */
      int32_t si_code;			/* Extra code.  */
      int32_t si_errno;			/* Errno.  */
    } pr_info;				/* Info associated with signal.  */
    int16_t pr_cursig;			/* Current signal.  */
    uint64_t pr_sigpend;		/* Set of pending signals.  */
    uint64_t pr_sighold;		/* Set of held signals.  */
    int32_t pr_pid;
    int32_t pr_ppid;
    int32_t pr_pgrp;
    int32_t pr_sid;
    struct x86_64_timeval
    {
      int64_t tv_sec;
      int32_t tv_usec;
    } pr_utime;				/* User time.  */
    struct x86_64_timeval pr_stime;	/* System time.  */
    struct x86_64_timeval pr_cutime;	/* Cumulative user time.  */
    struct x86_64_timeval pr_cstime;	/* Cumulative system time.  */
    uint64_t pr_reg[27];	/* GP registers.  */
    int32_t pr_fpvalid;			/* True if math copro being used.  */
  };


struct elf_prpsinfo
  {
    char pr_state;			/* Numeric process state.  */
    char pr_sname;			/* Char for pr_state.  */
    char pr_zomb;			/* Zombie.  */
    char pr_nice;			/* Nice val.  */
    uint64_t pr_flag;			/* Flags.  */
    uint32_t pr_uid;
    uint32_t pr_gid;
    int32_t pr_pid;
    int32_t pr_ppid;
    int32_t pr_pgrp;
    int32_t pr_sid;
    /* Lots missing */
    char pr_fname[16];			/* Filename of executable.  */
    char pr_psargs[80];			/* Initial part of arg list.  */
  };


bool
x86_64_core_note (name, type, descsz, desc)
     const char *name __attribute__ ((unused));
     uint32_t type;
     uint32_t descsz;
     const char *desc;
{
  bool result = false;

  switch (type)
    {
    case NT_PRSTATUS:
      if (descsz < sizeof (struct elf_prstatus))
	/* Not enough data.  */
	break;

      struct elf_prstatus *stat = (struct elf_prstatus *) desc;

      printf ("    SIGINFO:  signo: %" PRId32 ", code = %" PRId32
	      ", errno = %" PRId32 "\n"
	      "    signal: %" PRId16 ", pending: %#08" PRIx64 ", holding: %#08"
	      PRIx64 "\n"
	      "    pid: %" PRId32 ", ppid = %" PRId32 ", pgrp = %" PRId32
	      ", sid = %" PRId32 "\n"
	      "     utime: %6" PRId64 ".%06" PRId32
	      "s,  stime: %6" PRId64 ".%06" PRId32 "s\n"
	      "    cutime: %6" PRId64 ".%06" PRId32
	      "s, cstime: %6" PRId64 ".%06" PRId32 "s\n"
	      "    rax: %016" PRIx64 "  rbx: %016" PRIx64 "\n"
	      "    rcx: %016" PRIx64 "  rdx: %016" PRIx64 "\n"
	      "    rsi: %016" PRIx64 "  rdi: %016" PRIx64 "\n"
	      "    rbp: %016" PRIx64 "  rsp: %016" PRIx64 "\n"
	      "    r8:  %016" PRIx64 "  r9:  %016" PRIx64 "\n"
	      "    r10: %016" PRIx64 "  r11: %016" PRIx64 "\n"
	      "    r12: %016" PRIx64 "  r13: %016" PRIx64 "\n"
	      "    r14: %016" PRIx64 "  r15: %016" PRIx64 "\n"
	      "    rip: %016" PRIx64 "  eflags: %08" PRIx64 "\n"
	      "    original rax: %016" PRIx64 "\n"
	      "    cs: %04" PRIx64 "  ds: %04" PRIx64 "  es: %04" PRIx64
	      "  ss: %04" PRIx64 "\n"
	      "    fs: %04" PRIx64 "  fs_base: %016" PRIx64
	      "  gs: %04" PRIx64 "  gs_base: %016" PRIx64 "\n\n",
	      stat->pr_info. si_signo,
	      stat->pr_info. si_code,
	      stat->pr_info. si_errno,
	      stat->pr_cursig,
	      stat->pr_sigpend, stat->pr_sighold,
	      stat->pr_pid, stat->pr_ppid, stat->pr_pgrp, stat->pr_sid,
	      stat->pr_utime.tv_sec, stat->pr_utime.tv_usec,
	      stat->pr_stime.tv_sec, stat->pr_stime.tv_usec,
	      stat->pr_cutime.tv_sec, stat->pr_cutime.tv_usec,
	      stat->pr_cstime.tv_sec, stat->pr_cstime.tv_usec,
	      stat->pr_reg[10], stat->pr_reg[5], stat->pr_reg[11],
	      stat->pr_reg[12], stat->pr_reg[13], stat->pr_reg[14],
	      stat->pr_reg[4], stat->pr_reg[10], stat->pr_reg[9],
	      stat->pr_reg[7], stat->pr_reg[6], stat->pr_reg[5],
	      stat->pr_reg[3], stat->pr_reg[2], stat->pr_reg[1],
	      stat->pr_reg[0], stat->pr_reg[16], stat->pr_reg[18],
	      stat->pr_reg[15], stat->pr_reg[17], stat->pr_reg[23],
	      stat->pr_reg[24], stat->pr_reg[20],
	      stat->pr_reg[25], stat->pr_reg[21],
	      stat->pr_reg[26], stat->pr_reg[22]);

      /* We handled this entry.  */
      result = true;
      break;

    case NT_PRPSINFO:
      if (descsz < sizeof (struct elf_prpsinfo))
	/* Not enough data.  */
	break;

      struct elf_prpsinfo *info = (struct elf_prpsinfo *) desc;

      printf ("    state: %c (%hhd),  zombie: %hhd,  nice: %hhd\n"
	      "    flags: %08" PRIx64 "  uid: %" PRIu32 "  gid: %" PRIu32 "\n"
	      "    pid: %" PRId32 "  ppid: %" PRId32 "  pgrp: %" PRId32
	      "  sid: %" PRId32 "\n"
	      "    fname: %.16s\n"
	      "     args: %.80s\n\n",
	      info->pr_sname, info->pr_state, info->pr_zomb, info->pr_nice,
	      info->pr_flag, info->pr_uid, info->pr_gid,
	      info->pr_pid, info->pr_ppid, info->pr_pgrp, info->pr_sid,
	      info->pr_fname, info->pr_psargs);

      /* We handled this entry.  */
      result = true;
      break;

    default:
      break;
    }

  return result;
}
