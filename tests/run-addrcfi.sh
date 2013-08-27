#! /bin/sh
# Copyright (C) 2013
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

# Note some testfiles are also used in run-allregs.sh.

# Shows return address, CFA location expression and register rules
# from ABI's default CFI program as setup by arch ebl backend hook
# abi_cfi unless overridden by CIE and FDE program at given address.

# EM_386 (function main 0x080489b8)
testfiles testfile11
testrun_compare ${abs_builddir}/addrcfi -e testfile11 0x080489b8 <<\EOF
.eh_frame has 0x80489b8 => [0x80489b8, 0x80489b9):
	return address in reg8
	CFA location expression: bregx(4,4)
	integer reg0 (%eax): undefined
	integer reg1 (%ecx): undefined
	integer reg2 (%edx): undefined
	integer reg3 (%ebx): same_value
	integer reg4 (%esp): location expression: call_frame_cfa stack_value
	integer reg5 (%ebp): same_value
	integer reg6 (%esi): same_value
	integer reg7 (%edi): same_value
	integer reg8 (%eip): location expression: call_frame_cfa plus_uconst(-4)
	integer reg9 (%eflags): undefined
	integer reg10 (%trapno): undefined
	x87 reg11 (%st0): undefined
	x87 reg12 (%st1): undefined
	x87 reg13 (%st2): undefined
	x87 reg14 (%st3): undefined
	x87 reg15 (%st4): undefined
	x87 reg16 (%st5): undefined
	x87 reg17 (%st6): undefined
	x87 reg18 (%st7): undefined
	SSE reg21 (%xmm0): undefined
	SSE reg22 (%xmm1): undefined
	SSE reg23 (%xmm2): undefined
	SSE reg24 (%xmm3): undefined
	SSE reg25 (%xmm4): undefined
	SSE reg26 (%xmm5): undefined
	SSE reg27 (%xmm6): undefined
	SSE reg28 (%xmm7): undefined
	MMX reg29 (%mm0): undefined
	MMX reg30 (%mm1): undefined
	MMX reg31 (%mm2): undefined
	MMX reg32 (%mm3): undefined
	MMX reg33 (%mm4): undefined
	MMX reg34 (%mm5): undefined
	MMX reg35 (%mm6): undefined
	MMX reg36 (%mm7): undefined
	FPU-control reg37 (%fctrl): undefined
	FPU-control reg38 (%fstat): undefined
	FPU-control reg39 (%mxcsr): undefined
	segment reg40 (%es): same_value
	segment reg41 (%cs): same_value
	segment reg42 (%ss): same_value
	segment reg43 (%ds): same_value
	segment reg44 (%fs): same_value
	segment reg45 (%gs): same_value
.debug_frame has 0x80489b8 => [0x80489b8, 0x80489b9):
	return address in reg8
	CFA location expression: bregx(4,4)
	integer reg0 (%eax): undefined
	integer reg1 (%ecx): undefined
	integer reg2 (%edx): undefined
	integer reg3 (%ebx): same_value
	integer reg4 (%esp): location expression: call_frame_cfa stack_value
	integer reg5 (%ebp): same_value
	integer reg6 (%esi): same_value
	integer reg7 (%edi): same_value
	integer reg8 (%eip): location expression: call_frame_cfa plus_uconst(-4)
	integer reg9 (%eflags): undefined
	integer reg10 (%trapno): undefined
	x87 reg11 (%st0): undefined
	x87 reg12 (%st1): undefined
	x87 reg13 (%st2): undefined
	x87 reg14 (%st3): undefined
	x87 reg15 (%st4): undefined
	x87 reg16 (%st5): undefined
	x87 reg17 (%st6): undefined
	x87 reg18 (%st7): undefined
	SSE reg21 (%xmm0): undefined
	SSE reg22 (%xmm1): undefined
	SSE reg23 (%xmm2): undefined
	SSE reg24 (%xmm3): undefined
	SSE reg25 (%xmm4): undefined
	SSE reg26 (%xmm5): undefined
	SSE reg27 (%xmm6): undefined
	SSE reg28 (%xmm7): undefined
	MMX reg29 (%mm0): undefined
	MMX reg30 (%mm1): undefined
	MMX reg31 (%mm2): undefined
	MMX reg32 (%mm3): undefined
	MMX reg33 (%mm4): undefined
	MMX reg34 (%mm5): undefined
	MMX reg35 (%mm6): undefined
	MMX reg36 (%mm7): undefined
	FPU-control reg37 (%fctrl): undefined
	FPU-control reg38 (%fstat): undefined
	FPU-control reg39 (%mxcsr): undefined
	segment reg40 (%es): same_value
	segment reg41 (%cs): same_value
	segment reg42 (%ss): same_value
	segment reg43 (%ds): same_value
	segment reg44 (%fs): same_value
	segment reg45 (%gs): same_value
EOF

# EM_X86_64 (function foo 0x00000000000009d0)
testfiles testfile12
testrun_compare ${abs_builddir}/addrcfi -e testfile12 0x00000000000009d0 <<\EOF
.eh_frame has 0x9d0 => [0x9d0, 0x9d1):
	return address in reg16
	CFA location expression: bregx(7,8)
	integer reg0 (%rax): same_value
	integer reg1 (%rdx): undefined
	integer reg2 (%rcx): undefined
	integer reg3 (%rbx): undefined
	integer reg4 (%rsi): undefined
	integer reg5 (%rdi): undefined
	integer reg6 (%rbp): same_value
	integer reg7 (%rsp): location expression: call_frame_cfa stack_value
	integer reg8 (%r8): undefined
	integer reg9 (%r9): undefined
	integer reg10 (%r10): undefined
	integer reg11 (%r11): undefined
	integer reg12 (%r12): same_value
	integer reg13 (%r13): same_value
	integer reg14 (%r14): same_value
	integer reg15 (%r15): same_value
	integer reg16 (%rip): location expression: call_frame_cfa plus_uconst(-8)
	SSE reg17 (%xmm0): undefined
	SSE reg18 (%xmm1): undefined
	SSE reg19 (%xmm2): undefined
	SSE reg20 (%xmm3): undefined
	SSE reg21 (%xmm4): undefined
	SSE reg22 (%xmm5): undefined
	SSE reg23 (%xmm6): undefined
	SSE reg24 (%xmm7): undefined
	SSE reg25 (%xmm8): undefined
	SSE reg26 (%xmm9): undefined
	SSE reg27 (%xmm10): undefined
	SSE reg28 (%xmm11): undefined
	SSE reg29 (%xmm12): undefined
	SSE reg30 (%xmm13): undefined
	SSE reg31 (%xmm14): undefined
	SSE reg32 (%xmm15): undefined
	x87 reg33 (%st0): undefined
	x87 reg34 (%st1): undefined
	x87 reg35 (%st2): undefined
	x87 reg36 (%st3): undefined
	x87 reg37 (%st4): undefined
	x87 reg38 (%st5): undefined
	x87 reg39 (%st6): undefined
	x87 reg40 (%st7): undefined
	MMX reg41 (%mm0): undefined
	MMX reg42 (%mm1): undefined
	MMX reg43 (%mm2): undefined
	MMX reg44 (%mm3): undefined
	MMX reg45 (%mm4): undefined
	MMX reg46 (%mm5): undefined
	MMX reg47 (%mm6): undefined
	MMX reg48 (%mm7): undefined
	integer reg49 (%rflags): undefined
	segment reg50 (%es): undefined
	segment reg51 (%cs): undefined
	segment reg52 (%ss): undefined
	segment reg53 (%ds): undefined
	segment reg54 (%fs): undefined
	segment reg55 (%gs): undefined
	segment reg58 (%fs.base): undefined
	segment reg59 (%gs.base): undefined
	control reg62 (%tr): undefined
	control reg63 (%ldtr): undefined
	control reg64 (%mxcsr): undefined
	control reg65 (%fcw): undefined
	control reg66 (%fsw): undefined
.debug_frame has 0x9d0 => [0x9d0, 0x9d1):
	return address in reg16
	CFA location expression: bregx(7,8)
	integer reg0 (%rax): same_value
	integer reg1 (%rdx): undefined
	integer reg2 (%rcx): undefined
	integer reg3 (%rbx): undefined
	integer reg4 (%rsi): undefined
	integer reg5 (%rdi): undefined
	integer reg6 (%rbp): same_value
	integer reg7 (%rsp): location expression: call_frame_cfa stack_value
	integer reg8 (%r8): undefined
	integer reg9 (%r9): undefined
	integer reg10 (%r10): undefined
	integer reg11 (%r11): undefined
	integer reg12 (%r12): same_value
	integer reg13 (%r13): same_value
	integer reg14 (%r14): same_value
	integer reg15 (%r15): same_value
	integer reg16 (%rip): location expression: call_frame_cfa plus_uconst(-8)
	SSE reg17 (%xmm0): undefined
	SSE reg18 (%xmm1): undefined
	SSE reg19 (%xmm2): undefined
	SSE reg20 (%xmm3): undefined
	SSE reg21 (%xmm4): undefined
	SSE reg22 (%xmm5): undefined
	SSE reg23 (%xmm6): undefined
	SSE reg24 (%xmm7): undefined
	SSE reg25 (%xmm8): undefined
	SSE reg26 (%xmm9): undefined
	SSE reg27 (%xmm10): undefined
	SSE reg28 (%xmm11): undefined
	SSE reg29 (%xmm12): undefined
	SSE reg30 (%xmm13): undefined
	SSE reg31 (%xmm14): undefined
	SSE reg32 (%xmm15): undefined
	x87 reg33 (%st0): undefined
	x87 reg34 (%st1): undefined
	x87 reg35 (%st2): undefined
	x87 reg36 (%st3): undefined
	x87 reg37 (%st4): undefined
	x87 reg38 (%st5): undefined
	x87 reg39 (%st6): undefined
	x87 reg40 (%st7): undefined
	MMX reg41 (%mm0): undefined
	MMX reg42 (%mm1): undefined
	MMX reg43 (%mm2): undefined
	MMX reg44 (%mm3): undefined
	MMX reg45 (%mm4): undefined
	MMX reg46 (%mm5): undefined
	MMX reg47 (%mm6): undefined
	MMX reg48 (%mm7): undefined
	integer reg49 (%rflags): undefined
	segment reg50 (%es): undefined
	segment reg51 (%cs): undefined
	segment reg52 (%ss): undefined
	segment reg53 (%ds): undefined
	segment reg54 (%fs): undefined
	segment reg55 (%gs): undefined
	segment reg58 (%fs.base): undefined
	segment reg59 (%gs.base): undefined
	control reg62 (%tr): undefined
	control reg63 (%ldtr): undefined
	control reg64 (%mxcsr): undefined
	control reg65 (%fcw): undefined
	control reg66 (%fsw): undefined
EOF
