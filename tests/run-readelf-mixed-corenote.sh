#! /bin/sh
# Copyright (C) 2012 Red Hat, Inc.
# This file is part of elfutils.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. $srcdir/test-subr.sh

testfiles testfile63

testrun_compare ../src/readelf -n testfile63 <<\EOF

Note segment of 892 bytes at offset 0x274:
  Owner          Data size  Type
  CORE                 148  PRSTATUS
    info.si_signo: 11, info.si_code: 0, info.si_errno: 0, cursig: 11
    sigpend: <>
    sighold: <>
    pid: 11087, ppid: 11063, pgrp: 11087, sid: 11063
    utime: 0.000000, stime: 0.010000, cutime: 0.000000, cstime: 0.000000
    orig_r0: -1, fpvalid: 1
    r0:             1  r1:   -1091672508  r2:   -1091672500
    r3:             0  r4:             0  r5:             0
    r6:         33728  r7:             0  r8:             0
    r9:             0  r10:  -1225703496  r11:  -1091672844
    r12:            0  sp:    0xbeee64f4  lr:    0xb6dc3f48
    pc:    0x00008500  spsr:  0x60000010
  CORE                 124  PRPSINFO
    state: 0, sname: R, zomb: 0, nice: 0, flag: 0x00400500
    uid: 0, gid: 0, pid: 11087, ppid: 11063, pgrp: 11087, sid: 11063
    fname: a.out, psargs: ./a.out 
  CORE                 144  AUXV
    HWCAP: 0xe8d7  <swp half thumb fast-mult vfp edsp>
    PAGESZ: 4096
    CLKTCK: 100
    PHDR: 0x8034
    PHENT: 32
    PHNUM: 8
    BASE: 0xb6eee000
    FLAGS: 0
    ENTRY: 0x83c0
    UID: 0
    EUID: 0
    GID: 0
    EGID: 0
    SECURE: 0
    RANDOM: 0xbeee674e
    EXECFN: 0xbeee6ff4
    PLATFORM: 0xbeee675e
    NULL
  CORE                 116  FPREGSET
    f0: 0x000000000000000000000000  f1: 0x000000000000000000000000
    f2: 0x000000000000000000000000  f3: 0x000000000000000000000000
    f4: 0x000000000000000000000000  f5: 0x000000000000000000000000
    f6: 0x000000000000000000000000  f7: 0x000000000000000000000000
  LINUX                260  ARM_VFP
    fpscr: 0x00000000
    d0:  0x0000000000000000  d1:  0x0000000000000000
    d2:  0x0000000000000000  d3:  0x0000000000000000
    d4:  0x0000000000000000  d5:  0x0000000000000000
    d6:  0x0000000000000000  d7:  0x0000000000000000
    d8:  0x0000000000000000  d9:  0x0000000000000000
    d10: 0x0000000000000000  d11: 0x0000000000000000
    d12: 0x0000000000000000  d13: 0x0000000000000000
    d14: 0x0000000000000000  d15: 0x0000000000000000
    d16: 0x0000000000000000  d17: 0x0000000000000000
    d18: 0x0000000000000000  d19: 0x0000000000000000
    d20: 0x0000000000000000  d21: 0x0000000000000000
    d22: 0x0000000000000000  d23: 0x0000000000000000
    d24: 0x0000000000000000  d25: 0x0000000000000000
    d26: 0x0000000000000000  d27: 0x0000000000000000
    d28: 0x0000000000000000  d29: 0x0000000000000000
    d30: 0x0000000000000000  d31: 0x0000000000000000
EOF

exit 0
