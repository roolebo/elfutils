/* Disassembler for BPF.
   Copyright (C) 2016 Red Hat, Inc.
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <gelf.h>
#include <inttypes.h>
#include <linux/bpf.h>

#include "../libelf/common.h"
#include "../libebl/libeblP.h"


static const char class_string[8][8] = {
  [BPF_LD]    = "ld",
  [BPF_LDX]   = "ldx",
  [BPF_ST]    = "st",
  [BPF_STX]   = "stx",
  [BPF_ALU]   = "alu",
  [BPF_JMP]   = "jmp",
  [BPF_RET]   = "6",		/* completely unused in ebpf */
  [BPF_ALU64] = "alu64",
};

/* Dest = 1$, Src = 2$, Imm = 3$, Off = 4$, Branch = 5$.  */

#define DST		"r%1$d"
#define DSTU		"(u32)" DST
#define DSTS		"(s64)" DST

#define SRC		"r%2$d"
#define SRCU		"(u32)" SRC
#define SRCS		"(s64)" SRC

#define IMMS		"%3$d"
#define IMMX		"%3$#x"
#define OFF		"%4$+d"
#define JMP		"%5$#x"

#define A32(O, S)	DST " = " DSTU " " #O " " S
#define A64(O, S)	DST " " #O "= " S
#define J64(D, O, S)	"if " D " " #O " " S " goto " JMP
#define LOAD(T)		DST " = *(" #T " *)(" SRC OFF ")"
#define STORE(T, S)	"*(" #T " *)(" DST OFF ") = " S
#define XADD(T, S)	"lock *(" #T " *)(" DST OFF ") += " S
#define LDSKB(T, S)	"r0 = *(" #T " *)skb[" S "]"

/* 8 character field between opcode and arguments.  */
static const char * const code_fmts[256] = {
  [BPF_ALU | BPF_ADD  | BPF_K]    = A32(+, IMMS),
  [BPF_ALU | BPF_SUB  | BPF_K]    = A32(-, IMMS),
  [BPF_ALU | BPF_MUL  | BPF_K]    = A32(*, IMMS),
  [BPF_ALU | BPF_DIV  | BPF_K]    = A32(/, IMMS),
  [BPF_ALU | BPF_OR   | BPF_K]    = A32(|, IMMX),
  [BPF_ALU | BPF_AND  | BPF_K]    = A32(&, IMMX),
  [BPF_ALU | BPF_LSH  | BPF_K]    = A32(<<, IMMS),
  [BPF_ALU | BPF_RSH  | BPF_K]    = A32(>>, IMMS),
  [BPF_ALU | BPF_MOD  | BPF_K]    = A32(%, IMMS),
  [BPF_ALU | BPF_XOR  | BPF_K]    = A32(^, IMMX),
  [BPF_ALU | BPF_MOV  | BPF_K]    = DST " = " IMMX,
  [BPF_ALU | BPF_ARSH | BPF_K]    = DST " = (u32)((s32)" DST " >> " IMMS ")",

  [BPF_ALU | BPF_ADD  | BPF_X]    = A32(+, SRCU),
  [BPF_ALU | BPF_SUB  | BPF_X]    = A32(-, SRCU),
  [BPF_ALU | BPF_MUL  | BPF_X]    = A32(*, SRCU),
  [BPF_ALU | BPF_DIV  | BPF_X]    = A32(/, SRCU),
  [BPF_ALU | BPF_OR   | BPF_X]    = A32(|, SRCU),
  [BPF_ALU | BPF_AND  | BPF_X]    = A32(&, SRCU),
  [BPF_ALU | BPF_LSH  | BPF_X]    = A32(<<, SRCU),
  [BPF_ALU | BPF_RSH  | BPF_X]    = A32(>>, SRCU),
  [BPF_ALU | BPF_MOD  | BPF_X]    = A32(%, SRCU),
  [BPF_ALU | BPF_XOR  | BPF_X]    = A32(^, SRCU),
  [BPF_ALU | BPF_MOV  | BPF_X]    = DST " = " SRCU,
  [BPF_ALU | BPF_ARSH | BPF_X]    = DST " = (u32)((s32)" DST " >> " SRC ")",

  [BPF_ALU64 | BPF_ADD  | BPF_K]  = A64(+, IMMS),
  [BPF_ALU64 | BPF_SUB  | BPF_K]  = A64(-, IMMS),
  [BPF_ALU64 | BPF_MUL  | BPF_K]  = A64(*, IMMS),
  [BPF_ALU64 | BPF_DIV  | BPF_K]  = A64(/, IMMS),
  [BPF_ALU64 | BPF_OR   | BPF_K]  = A64(|, IMMS),
  [BPF_ALU64 | BPF_AND  | BPF_K]  = A64(&, IMMS),
  [BPF_ALU64 | BPF_LSH  | BPF_K]  = A64(<<, IMMS),
  [BPF_ALU64 | BPF_RSH  | BPF_K]  = A64(>>, IMMS),
  [BPF_ALU64 | BPF_MOD  | BPF_K]  = A64(%, IMMS),
  [BPF_ALU64 | BPF_XOR  | BPF_K]  = A64(^, IMMS),
  [BPF_ALU64 | BPF_MOV  | BPF_K]  = DST " = " IMMS,
  [BPF_ALU64 | BPF_ARSH | BPF_K]  = DST " = (s64)" DST " >> " IMMS,

  [BPF_ALU64 | BPF_ADD  | BPF_X]  = A64(+, SRC),
  [BPF_ALU64 | BPF_SUB  | BPF_X]  = A64(-, SRC),
  [BPF_ALU64 | BPF_MUL  | BPF_X]  = A64(*, SRC),
  [BPF_ALU64 | BPF_DIV  | BPF_X]  = A64(/, SRC),
  [BPF_ALU64 | BPF_OR   | BPF_X]  = A64(|, SRC),
  [BPF_ALU64 | BPF_AND  | BPF_X]  = A64(&, SRC),
  [BPF_ALU64 | BPF_LSH  | BPF_X]  = A64(<<, SRC),
  [BPF_ALU64 | BPF_RSH  | BPF_X]  = A64(>>, SRC),
  [BPF_ALU64 | BPF_MOD  | BPF_X]  = A64(%, SRC),
  [BPF_ALU64 | BPF_XOR  | BPF_X]  = A64(^, SRC),
  [BPF_ALU64 | BPF_MOV  | BPF_X]  = DST " = " SRC,
  [BPF_ALU64 | BPF_ARSH | BPF_X]  = DST " = (s64)" DST " >> " SRC,

  [BPF_ALU | BPF_NEG]		  = DST " = (u32)-" DST,
  [BPF_ALU64 | BPF_NEG]		  = DST " = -" DST,

  /* The imm field contains {16,32,64}.  */
  [BPF_ALU | BPF_END | BPF_TO_LE] = DST " = le%3$-6d(" DST ")",
  [BPF_ALU | BPF_END | BPF_TO_BE] = DST " = be%3$-6d(" DST ")",

  [BPF_JMP | BPF_JEQ  | BPF_K]    = J64(DST, ==, IMMS),
  [BPF_JMP | BPF_JGT  | BPF_K]    = J64(DST, >, IMMS),
  [BPF_JMP | BPF_JGE  | BPF_K]    = J64(DST, >=, IMMS),
  [BPF_JMP | BPF_JSET | BPF_K]    = J64(DST, &, IMMS),
  [BPF_JMP | BPF_JNE  | BPF_K]    = J64(DST, !=, IMMS),
  [BPF_JMP | BPF_JSGT | BPF_K]    = J64(DSTS, >, IMMS),
  [BPF_JMP | BPF_JSGE | BPF_K]    = J64(DSTS, >=, IMMS),

  [BPF_JMP | BPF_JEQ  | BPF_X]    = J64(DST, ==, SRC),
  [BPF_JMP | BPF_JGT  | BPF_X]    = J64(DST, >, SRC),
  [BPF_JMP | BPF_JGE  | BPF_X]    = J64(DST, >=, SRC),
  [BPF_JMP | BPF_JSET | BPF_X]    = J64(DST, &, SRC),
  [BPF_JMP | BPF_JNE  | BPF_X]    = J64(DST, !=, SRC),
  [BPF_JMP | BPF_JSGT | BPF_X]    = J64(DSTS, >, SRCS),
  [BPF_JMP | BPF_JSGE | BPF_X]    = J64(DSTS, >=, SRCS),

  [BPF_JMP | BPF_JA]              = "goto " JMP,
  [BPF_JMP | BPF_CALL]            = "call " IMMS,
  [BPF_JMP | BPF_EXIT]            = "exit",

  [BPF_LDX | BPF_MEM | BPF_B]     = LOAD(u8),
  [BPF_LDX | BPF_MEM | BPF_H]     = LOAD(u16),
  [BPF_LDX | BPF_MEM | BPF_W]     = LOAD(u32),
  [BPF_LDX | BPF_MEM | BPF_DW]    = LOAD(u64),

  [BPF_STX | BPF_MEM | BPF_B]     = STORE(u8, SRC),
  [BPF_STX | BPF_MEM | BPF_H]     = STORE(u16, SRC),
  [BPF_STX | BPF_MEM | BPF_W]     = STORE(u32, SRC),
  [BPF_STX | BPF_MEM | BPF_DW]    = STORE(u64, SRC),

  [BPF_STX | BPF_XADD | BPF_W]    = XADD(u32, SRC),
  [BPF_STX | BPF_XADD | BPF_DW]   = XADD(u64, SRC),

  [BPF_ST | BPF_MEM | BPF_B]      = STORE(u8, IMMS),
  [BPF_ST | BPF_MEM | BPF_H]      = STORE(u16, IMMS),
  [BPF_ST | BPF_MEM | BPF_W]      = STORE(u32, IMMS),
  [BPF_ST | BPF_MEM | BPF_DW]     = STORE(u64, IMMS),

  [BPF_LD | BPF_ABS | BPF_B]      = LDSKB(u8, IMMS),
  [BPF_LD | BPF_ABS | BPF_H]      = LDSKB(u16, IMMS),
  [BPF_LD | BPF_ABS | BPF_W]      = LDSKB(u32, IMMS),

  [BPF_LD | BPF_IND | BPF_B]      = LDSKB(u8, SRC "+" IMMS),
  [BPF_LD | BPF_IND | BPF_H]      = LDSKB(u16, SRC "+" IMMS),
  [BPF_LD | BPF_IND | BPF_W]      = LDSKB(u32, SRC "+" IMMS),
};

static void
bswap_bpf_insn (struct bpf_insn *p)
{
  /* Note that the dst_reg and src_reg fields are 4-bit bitfields.
     That means these two nibbles are (typically) layed out in the
     opposite order between big- and little-endian hosts.  This is
     not required by any standard, but does happen to be true for
     at least ppc, s390, arm and mips as big-endian hosts.  */
  int t = p->dst_reg;
  p->dst_reg = p->src_reg;
  p->src_reg = t;

  /* The other 2 and 4 byte fields are trivially converted.  */
  CONVERT (p->off);
  CONVERT (p->imm);
}

int
bpf_disasm (Ebl *ebl, const uint8_t **startp, const uint8_t *end,
	    GElf_Addr addr, const char *fmt __attribute__((unused)),
	    DisasmOutputCB_t outcb,
	    DisasmGetSymCB_t symcb __attribute__((unused)),
	    void *outcbarg,
	    void *symcbarg __attribute__((unused)))
{
  const bool need_bswap = MY_ELFDATA != ebl->data;
  const uint8_t *start = *startp;
  char buf[128];
  int len, retval = 0;

  while (start + sizeof(struct bpf_insn) <= end)
    {
      struct bpf_insn i;
      unsigned code, class, jmp;
      const char *code_fmt;

      memcpy(&i, start, sizeof(struct bpf_insn));
      if (need_bswap)
	bswap_bpf_insn (&i);
      start += sizeof(struct bpf_insn);
      addr += sizeof(struct bpf_insn);

      /* ??? We really should pass in CTX, so that we can detect
	 wrong endianness and do some swapping.  */

      code = i.code;
      code_fmt = code_fmts[code];

      if (code == (BPF_LD | BPF_IMM | BPF_DW))
	{
	  struct bpf_insn i2;
	  uint64_t imm64;

	  if (start + sizeof(struct bpf_insn) > end)
	    {
	      start -= sizeof(struct bpf_insn);
	      *startp = start;
	      goto done;
	    }
	  memcpy(&i2, start, sizeof(struct bpf_insn));
	  if (need_bswap)
	    bswap_bpf_insn (&i2);
	  start += sizeof(struct bpf_insn);
	  addr += sizeof(struct bpf_insn);

	  imm64 = (uint32_t)i.imm | ((uint64_t)i2.imm << 32);
	  switch (i.src_reg)
	    {
	    case 0:
	      code_fmt = DST " = %2$#" PRIx64;
	      break;
	    case BPF_PSEUDO_MAP_FD:
	      code_fmt = DST " = map_fd(%2$#" PRIx64 ")";
	      break;
	    default:
	      code_fmt = DST " = ld_pseudo(%3$d, %2$#" PRIx64 ")";
	      break;
	    }
	  len = snprintf(buf, sizeof(buf), code_fmt,
			 i.dst_reg, imm64, i.src_reg);
	}
      else if (code_fmt != NULL)
	{
	  jmp = addr + i.off * sizeof(struct bpf_insn);
	  len = snprintf(buf, sizeof(buf), code_fmt, i.dst_reg, i.src_reg,
			 i.imm, i.off, jmp);
	}
      else
	{
	  class = BPF_CLASS(code);
	  len = snprintf(buf, sizeof(buf), "invalid class %s",
			 class_string[class]);
        }

      *startp = start;
      retval = outcb (buf, len, outcbarg);
      if (retval != 0)
	goto done;
    }

 done:
  return retval;
}
