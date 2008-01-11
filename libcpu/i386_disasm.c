/* Disassembler for x86.
   Copyright (C) 2007, 2008 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2007.

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

#include <assert.h>
#include <config.h>
#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <gelf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "../libebl/libeblP.h"

#define MACHINE_ENCODING __LITTLE_ENDIAN
#include "memory-access.h"


#ifndef MNEFILE
# define MNEFILE "i386.mnemonics"
#endif

#define MNESTRFIELD(line) MNESTRFIELD1 (line)
#define MNESTRFIELD1(line) str##line
static const union mnestr_t
{
  struct
  {
#define MNE(name) char MNESTRFIELD (__LINE__)[sizeof (#name)];
#include MNEFILE
#undef MNE
  };
  char str[0];
} mnestr =
  {
    {
#define MNE(name) #name,
#include MNEFILE
#undef MNE
    }
  };

/* The index can be stored in the instrtab.  */
enum
  {
#define MNE(name) MNE_##name,
#include MNEFILE
#undef MNE
    MNE_INVALID
  };

static const unsigned short int mneidx[] =
  {
#define MNE(name) \
  [MNE_##name] = offsetof (union mnestr_t, MNESTRFIELD (__LINE__)),
#include MNEFILE
#undef MNE
  };


enum
  {
    idx_rex_b = 0,
    idx_rex_x,
    idx_rex_r,
    idx_rex_w,
    idx_rex,
    idx_cs,
    idx_ds,
    idx_es,
    idx_fs,
    idx_gs,
    idx_ss,
    idx_data16,
    idx_addr16,
    idx_rep,
    idx_repne,
    idx_lock
  };

enum
  {
#define prefbit(pref) has_##pref = 1 << idx_##pref
    prefbit (rex_b),
    prefbit (rex_x),
    prefbit (rex_r),
    prefbit (rex_w),
    prefbit (rex),
    prefbit (cs),
    prefbit (ds),
    prefbit (es),
    prefbit (fs),
    prefbit (gs),
    prefbit (ss),
    prefbit (data16),
    prefbit (addr16),
    prefbit (rep),
    prefbit (repne),
    prefbit (lock)
#undef prefbit
  };
#define SEGMENT_PREFIXES \
  (has_cs | has_ds | has_es | has_fs | has_gs | has_ss)

#define prefix_cs	0x2e
#define prefix_ds	0x3e
#define prefix_es	0x26
#define prefix_fs	0x64
#define prefix_gs	0x65
#define prefix_ss	0x36
#define prefix_data16	0x66
#define prefix_addr16	0x67
#define prefix_rep	0xf3
#define prefix_repne	0xf2
#define prefix_lock	0xf0


static const uint8_t known_prefixes[] =
  {
#define newpref(pref) [idx_##pref] = prefix_##pref
    newpref (cs),
    newpref (ds),
    newpref (es),
    newpref (fs),
    newpref (gs),
    newpref (ss),
    newpref (data16),
    newpref (addr16),
    newpref (rep),
    newpref (repne),
    newpref (lock)
#undef newpref
  };
#define nknown_prefixes (sizeof (known_prefixes) / sizeof (known_prefixes[0]))


#if 0
static const char *prefix_str[] =
  {
#define newpref(pref) [idx_##pref] = #pref
    newpref (cs),
    newpref (ds),
    newpref (es),
    newpref (fs),
    newpref (gs),
    newpref (ss),
    newpref (data16),
    newpref (addr16),
    newpref (rep),
    newpref (repne),
    newpref (lock)
#undef newpref
  };
#endif


struct output_data
{
  GElf_Addr addr;
  int *prefixes;
  const char *op1str;
  size_t opoff1;
  size_t opoff2;
  size_t opoff3;
  char *bufp;
  size_t *bufcntp;
  size_t bufsize;
  const uint8_t *data;
  const uint8_t **param_start;
  const uint8_t *end;
  DisasmGetSymCB_t symcb;
  void *symcbarg;
};


#ifndef DISFILE
# define DISFILE "i386_dis.h"
#endif
#include DISFILE


#define ADD_CHAR(ch) \
  do {									      \
    if (unlikely (bufcnt == bufsize))					      \
      goto enomem;							      \
    buf[bufcnt++] = (ch);						      \
  } while (0)

#define ADD_STRING(str) \
  do {									      \
    const char *_str = (str);						      \
    size_t _len = strlen (_str);					      \
    if (unlikely (bufcnt + _len > bufsize))				      \
      goto enomem;							      \
    memcpy (buf + bufcnt, str, _len);					      \
    bufcnt += _len;							      \
  } while (0)


#include <stdio.h>
int
i386_disasm (const uint8_t **startp, const uint8_t *end, GElf_Addr addr,
	     const char *fmt, DisasmOutputCB_t outcb, DisasmGetSymCB_t symcb,
	     void *outcbarg, void *symcbarg)
{
  const char *save_fmt = fmt;

  while (1)
    {
#define BUFSIZE 512
      const size_t bufsize = BUFSIZE;
      char initbuf[BUFSIZE];
      char *buf = initbuf;
      size_t bufcnt = 0;

      int prefixes = 0;
      int last_prefix_bit = 0;

      const uint8_t *data = *startp;
      const uint8_t *begin = data;

      fmt = save_fmt;

      /* Recognize all prefixes.  */
      while (data < end)
	{
	  unsigned int i;
	  for (i = idx_cs; i < nknown_prefixes; ++i)
	    if (known_prefixes[i] == *data)
	      break;
	  if (i == nknown_prefixes)
	    break;

	  prefixes |= last_prefix_bit = 1 << i;

	  ++data;
	}

#ifdef X86_64
      if (data < end && (*data & 0xf0) == 0x40)
	prefixes |= ((*data++) & 0xf) | has_rex;
#endif

      assert (data <= end);
      if (data == end)
	{
	  if (prefixes != 0)
	    goto print_prefix;

	  return -1;
	}

      const uint8_t *curr = match_data;
      const uint8_t *const match_end = match_data + sizeof (match_data);

    enomem:
      ;

      size_t cnt = 0;
      while (curr < match_end)
	{
	  const uint8_t *start = curr;

	  uint_fast8_t len = *curr++;

	  assert (len > 0);
	  assert (curr + 2 * len + 2 <= match_end);

	  const uint8_t *codep = data;
	  size_t avail = len;
	  int correct_prefix = 0;
	  int opoff = 0;

	  if (data > begin && codep[-1] == curr[1] && curr[0] == 0xff)
	    {
	      /* We match a prefix byte.  This is exactly one byte and
		 is matched exactly, without a mask.  */
	      --avail;

	      --len;
	      start += 2;
	      opoff = 8;

	      curr += 2;
	      assert (avail > 0);

	      assert (last_prefix_bit != 0);
	      correct_prefix = last_prefix_bit;
	    }

	  do
	    {
	      uint_fast8_t masked = *codep++ & *curr++;
	      if (masked != *curr++)
		break;

	      --avail;
	      if (codep == end && avail > 0)
		return 0;
	    }
	  while (avail > 0);

	  if (avail != 0)
	    {
	    not:
	      curr = start + 1 + 2 * len + 2;
	      ++cnt;
	      continue;
	    }

	  if (len > end - data)
	    /* There is not enough data for the entire instruction.  The
	       caller can figure this out by looking at the pointer into
	       the input data.  */
	    return 0;

	  assert (correct_prefix == 0
		  || (prefixes & correct_prefix) != 0);
	  prefixes ^= correct_prefix;

	  size_t prefix_size = 0;

	  // XXXonly print as prefix if valid?
	  if ((prefixes & has_lock) != 0)
	    {
	      ADD_STRING ("lock ");
	      prefix_size += 5;
	    }

	  if (instrtab[cnt].rep)
	    {
	      if ((prefixes & has_rep) !=  0)
		{
		  ADD_STRING ("rep ");
		  prefix_size += 4;
		}
	    }
	  else if (instrtab[cnt].repe
		   && (prefixes & (has_rep | has_repne)) != 0)
	    {
	      if ((prefixes & has_repne) != 0)
		{
		  ADD_STRING ("repne ");
		  prefix_size += 6;
		}
	      else if ((prefixes & has_rep) != 0)
		{
		  ADD_STRING ("repe ");
		  prefix_size += 5;
		}
	    }
	  else if ((prefixes & (has_rep | has_repne)) != 0)
	    {
	      uint_fast8_t byte;
	    print_prefix:
	      bufcnt = 0;
	      byte = *begin;
	      /* This is a prefix byte.  Print it.  */
	      switch (byte)
		{
		case prefix_rep:
		  ADD_STRING ("rep");
		  break;
		case prefix_repne:
		  ADD_STRING ("repne");
		  break;
		case prefix_cs:
		  ADD_STRING ("cs");
		  break;
		case prefix_ds:
		  ADD_STRING ("ds");
		  break;
		case prefix_es:
		  ADD_STRING ("es");
		  break;
		case prefix_fs:
		  ADD_STRING ("fs");
		  break;
		case prefix_gs:
		  ADD_STRING ("gs");
		  break;
		case prefix_ss:
		  ADD_STRING ("ss");
		  break;
		case prefix_data16:
		  ADD_STRING ("data16");
		  break;
		case prefix_addr16:
		  ADD_STRING ("addr16");
		  break;
		case prefix_lock:
		  ADD_STRING ("lock");
		  break;
		default:
		  /* Cannot happen.  */
		  puts ("unknown prefix");
		  abort ();
		}
	      data = begin + 1;
	      ++addr;

	      /* The string definitely fits.  */
	      buf[bufcnt++] = '\0';

	      goto out;
	    }

	  /* We have a match.  First determine how many bytes are
	     needed for the adressing mode.  */
	  const uint8_t *param_start = codep;
	  if (instrtab[cnt].modrm)
	    {
	      uint_fast8_t modrm = codep[-1];

#ifndef X86_64
	      if (likely ((prefixes & has_addr16) != 0))
		{
		  /* Account for displacement.  */
		  if ((modrm & 0xc7) == 6 || (modrm & 0xc0) == 0x80)
		    param_start += 2;
		  else if ((modrm & 0xc0) == 0x40)
		    param_start += 1;
		}
	      else
#endif
		{
		  /* Account for SIB.  */
		  if ((modrm & 0xc0) != 0xc0 && (modrm & 0x7) == 0x4)
		    param_start += 1;

		  /* Account for displacement.  */
		  if ((modrm & 0xc7) == 5 || (modrm & 0xc0) == 0x80
		      || ((modrm & 0xc7) == 0x4 && (codep[0] & 0x7) == 0x5))
		    param_start += 4;
		  else if ((modrm & 0xc0) == 0x40)
		    param_start += 1;
		}

	      if (unlikely (param_start > end))
		goto not;
	    }

	  struct output_data output_data =
	    {
	      .addr = addr + (data - begin),
	      .prefixes = &prefixes,
	      .bufp = buf,
	      .bufcntp = &bufcnt,
	      .bufsize = bufsize,
	      .data = data,
	      .param_start = &param_start,
	      .end = end,
	      .symcb = symcb,
	      .symcbarg = symcbarg
	    };

	  unsigned long string_end_idx = 0;
	  while (*fmt != '\0')
	    {
	      if (*fmt != '%')
		{
		  char ch = *fmt++;
		  if (ch == '\\')
		    {
		      switch ((ch = *fmt++))
			{
			case '0' ... '7':
			  {
			    int val = ch - '0';
			    ch = *fmt;
			    if (ch >= '0' && ch <= '7')
			      {
				val *= 8;
				val += ch - '0';
				ch = *++fmt;
				if (ch >= '0' && ch <= '7' && val < 32)
				  {
				    val *= 8;
				    val += ch - '0';
				    ++fmt;
				  }
			      }
			    ch = val;
			  }
			  break;

			case 'n':
			  ch = '\n';
			  break;

			case 't':
			  ch = '\t';
			  break;

			default:
			  return EINVAL;
			}
		    }
		  ADD_CHAR (ch);
		  continue;
		}
	      ++fmt;

	      int width = 0;
	      while (isdigit (*fmt))
		width = width * 10 + (*fmt++ - '0');

	      int prec = 0;
	      if (*fmt == '.')
		while (isdigit (*++fmt))
		  prec = prec * 10 + (*fmt - '0');

	      size_t start_idx = bufcnt;
	      switch (*fmt++)
		{
		  char mnebuf[16];
		  const char *str;

		case 'm':
		  /* Mnemonic.  */

		  if (unlikely (instrtab[cnt].mnemonic == MNE_INVALID))
		    {
		      switch (*data)
			{
			case 0x98:
			  if (prefixes & ~has_data16)
			    goto print_prefix;
			  str = prefixes & has_data16 ? "cbtw" : "cwtl";
			  break;

			case 0x99:
			  if (prefixes & ~has_data16)
			    goto print_prefix;
			  str = prefixes & has_data16 ? "cwtd" : "cltd";
			  break;

			case 0xe3:
			  if (prefixes & ~has_addr16)
			    goto print_prefix;
#ifdef X86_64
			  str = prefixes & has_addr16 ? "jecxz" : "jrcxz";
#else
			  str = prefixes & has_addr16 ? "jcxz" : "jecxz";
#endif
			  break;

			case 0x0f:
#ifdef X86_64
			  if (data[1] == 0xc7)
			    {
			      str = ((prefixes & has_rex_w)
				     ? "cmpxchg16b" : "cmpxchg8b");
			      break;
			    }
#endif
			  if (data[1] == 0xc2)
			    {
			      if (param_start >= end)
				goto not;
			      if (*param_start > 7)
				goto not;
			      static const char cmpops[][9] =
				{
				  [0] = "cmpeq",
				  [1] = "cmplt",
				  [2] = "cmple",
				  [3] = "cmpunord",
				  [4] = "cmpneq",
				  [5] = "cmpnlt",
				  [6] = "cmpnle",
				  [7] = "cmpord"
				};
			      char *cp = stpcpy (mnebuf, cmpops[*param_start]);
			      if (correct_prefix & (has_rep | has_repne))
				*cp++ = 's';
			      else
				*cp++ = 'p';
			      if (correct_prefix & (has_data16 | has_repne))
				*cp++ = 'd';
			      else
				*cp++ = 's';
			      *cp = '\0';
			      str = mnebuf;
			      /* Eat the immediate byte indicating the
				 operation.  */
			      ++param_start;
			      break;
			    }

			default:
			  assert (! "INVALID not handled");
			}
		    }
		  else
		    str = mnestr.str + mneidx[instrtab[cnt].mnemonic];

		  ADD_STRING (str);

		  switch (instrtab[cnt].suffix)
		    {
		    case suffix_none:
		      break;
		    case suffix_w:
		      if ((codep[-1] & 0xc0) != 0xc0)
			{
			  char ch;

			  if (data[0] & 1)
			    {
			      if (prefixes & has_data16)
				ch = 'w';
#ifdef X86_64
			      else if (prefixes & has_rex_w)
				ch = 'q';
#endif
			      else
				ch = 'l';
			    }
			  else
			    ch = 'b';

			  ADD_CHAR (ch);
			}
		      break;
		    case suffix_w0:
		      if ((codep[-1] & 0xc0) != 0xc0)
			ADD_CHAR ('l');
		      break;
		    case suffix_w1:
		      if ((data[0] & 0x4) == 0)
			ADD_CHAR ('l');
		      break;
		    case suffix_W:
		      if (prefixes & has_data16)
			{
			  ADD_CHAR ('w');
			  prefixes &= ~has_data16;
			}
#ifdef X86_64
		      else
			ADD_CHAR ('q');
#endif
		      break;
		    case suffix_W1:
		      if (prefixes & has_data16)
			{
			  ADD_CHAR ('w');
			  prefixes &= ~has_data16;
			}
#ifdef X86_64
		      else if (prefixes & has_rex_w)
			ADD_CHAR ('q');
#endif
		      break;

		    case suffix_tttn:;
		      static const char tttn[16][3] =
			{
			  "o", "no", "b", "ae", "e", "ne", "be", "a",
			  "s", "ns", "p", "np", "l", "ge", "le", "g"
			};
		      ADD_STRING (tttn[codep[-1 - instrtab[cnt].modrm] & 0x0f]);
		      break;
		    case suffix_D:
		      if ((codep[-1] & 0xc0) != 0xc0)
			ADD_CHAR ((data[0] & 0x04) == 0 ? 's' : 'l');
		      break;
		    default:
		      printf("unknown suffix %d\n", instrtab[cnt].suffix);
		      abort ();
		    }

		  string_end_idx = bufcnt;
		  break;

		case 'o':
		  if (prec == 1 && instrtab[cnt].fct1 != 0)
		    {
		      /* First parameter.  */
		      if (instrtab[cnt].str1 != 0)
			ADD_STRING (op1_str[instrtab[cnt].str1]);

		      output_data.op1str = op1_str[instrtab[cnt].str1];
		      output_data.opoff1 = (instrtab[cnt].off1_1
					    + OFF1_1_BIAS - opoff);
		      output_data.opoff2 = (instrtab[cnt].off1_2
					    + OFF1_2_BIAS - opoff);
		      output_data.opoff3 = (instrtab[cnt].off1_3
					    + OFF1_3_BIAS - opoff);
		      int r = op1_fct[instrtab[cnt].fct1] (&output_data);
		      if (r < 0)
			goto not;
		      if (r > 0)
			goto enomem;

		      string_end_idx = ~0ul;
		    }
		  else if (prec == 2 && instrtab[cnt].fct2 != 0)
		    {
		      /* Second parameter.  */
		      if (instrtab[cnt].str2 != 0)
			ADD_STRING (op2_str[instrtab[cnt].str2]);

		      output_data.op1str = op2_str[instrtab[cnt].str2];
		      output_data.opoff1 = (instrtab[cnt].off2_1
					    + OFF2_1_BIAS - opoff);
		      output_data.opoff2 = (instrtab[cnt].off2_2
					    + OFF2_2_BIAS - opoff);
		      output_data.opoff3 = (instrtab[cnt].off2_3
					    + OFF2_3_BIAS - opoff);
		      int r = op2_fct[instrtab[cnt].fct2] (&output_data);
		      if (r < 0)
			goto not;
		      if (r > 0)
			goto enomem;

		      string_end_idx = ~0ul;
		    }
		  else if (prec == 3 && instrtab[cnt].fct3 != 0)
		    {
		      /* Third parameter.  */
		      if (instrtab[cnt].str3 != 0)
			ADD_STRING (op3_str[instrtab[cnt].str3]);

		      output_data.op1str = op3_str[instrtab[cnt].str3];
		      output_data.opoff1 = (instrtab[cnt].off3_1
					    + OFF3_1_BIAS - opoff);
		      output_data.opoff2 = (instrtab[cnt].off3_2
					    + OFF3_2_BIAS - opoff);
#ifdef OFF3_3_BITS
		      output_data.opoff3 = (instrtab[cnt].off3_3
					    + OFF3_3_BIAS - opoff);
#else
		      output_data.opoff3 = 0;
#endif
		      int r = op3_fct[instrtab[cnt].fct3] (&output_data);
		      if (r < 0)
			goto not;
		      if (r > 0)
			goto enomem;

		      string_end_idx = ~0ul;
		    }
		  break;

		case 'e':
		  /* String end marker.  */
		  if (string_end_idx == ~0ul)
		    string_end_idx = bufcnt;
		  /* No padding.  */
		  width = 0;
		  break;
		}

	      /* Pad according to the specified width.  */
	      while (bufcnt + prefix_size < start_idx + width)
		ADD_CHAR (' ');
	      prefix_size = 0;
	    }

	  if ((prefixes & SEGMENT_PREFIXES) != 0)
	    goto print_prefix;

	  if (string_end_idx != ~0ul)
	    buf[string_end_idx] = '\0';

	  addr += param_start - begin;
	  data = param_start;

	  goto out;
	}

      /* Invalid (or at least unhandled) opcode.  */
      if (prefixes != 0)
	goto print_prefix;
      assert (*startp == data);
      ++data;
      ADD_STRING ("(bad)");
      addr += data - begin;

      buf[bufcnt++] = '\0';

    out:
      *startp = data;
      int res = outcb (buf, strlen (buf), outcbarg);
      if (res != 0)
	return res;
    }

  return 0;
}
