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
    idx_cs = 0,
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

      const uint8_t *data = *startp;
      const uint8_t *begin = data;

      fmt = save_fmt;

      /* Recognize all prefixes.  */
      while (data < end)
	{
	  unsigned int i;
	  for (i = 0; i < nknown_prefixes; ++i)
	    if (known_prefixes[i] == *data)
	      break;
	  if (i == nknown_prefixes)
	    break;

	  prefixes |= 1 << i;

	  ++data;
	}

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
	  const uint8_t *const start = curr;

	  uint_fast8_t len = *curr++;

	  assert (len > 0);
	  assert (curr + 2 * len + 2 <= match_end);

	  const uint8_t *codep = data;
	  size_t avail = len;

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

	      if ((prefixes & has_addr16) == 0)
		{
		  /* Account for SIB.  */
		  if ((modrm & 0xc0) != 0xc0 && (modrm & 0x7) == 0x4)
		    param_start += 1;
		}

	      /* Account for displacement.  */
	      if ((modrm & 0xc7) == 5 || (modrm & 0xc0) == 0x80
		  || ((modrm & 0xc7) == 0x4 && (codep[0] & 0x7) == 0x5))
		param_start += 4;
	      else if ((modrm & 0xc0) == 0x40)
		param_start += 1;
	    }

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
		  const char *str;

		case 'm':
		  /* Mnemonic.  */

		  if (unlikely (instrtab[cnt].mnemonic == MNE_INVALID))
		    {
		      switch (*data)
			{
			case 0x90:
			  if (prefixes & ~has_rep)
			    goto print_prefix;
			  /* Discard the 'rep' prefix string possibly
			     already in the buffer.  */
			  bufcnt = 0;
			  str = prefixes & has_rep ? "pause" : "nop";
			  break;

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
			  str = prefixes & has_addr16 ? "jcxz" : "jecxz";
			  break;

			case 0x0f:
			  if (data[1] == 0x10 || data[1] == 0x11)
			    {
			      bufcnt = 0;
			      int mod = prefixes & (has_data16 | has_rep
						    | has_repne);
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "movupd"
				     : mod & has_rep
				     ? "movss"
				     : mod & has_repne
				     ? "movsd"
				     : "movups");
			      break;
			    }
			  if (data[1] == 0x12)
			    {
			      bufcnt = 0;
			      int mod = prefixes & (has_data16 | has_rep
						    | has_repne);
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "movlpd"
				     : mod & has_rep
				     ? "movsldup"
				     : mod & has_repne
				     ? "movddup"
				     : (data[2] & 0xc0) == 0xc0
				     ? "movhlps"
				     : "movlps");
			      break;
			    }
			  if (data[1] == 0x13)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? ((data[2] & 0xc0) == 0xc0
					? "movhlpd"
					: "movlpd")
				     : (data[2] & 0xc0) == 0xc0
				     ? "movhlps"
				     : "movlps");
			      break;
			    }
			  if (data[1] == 0x14)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "unpcklpd" : "unpcklps");
			      break;
			    }
			  if (data[1] == 0x15)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "unpckhpd" : "unpckhps");
			      break;
			    }
			  if (data[1] == 0x16)
			    {
			      bufcnt = 0;
			      int mod = prefixes & (has_data16 | has_rep);
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "movhpd"
				     : mod & has_rep
				     ? "movshdup"
				     : (data[2] & 0xc0) == 0xc0
				     ? "movlhps"
				     : "movhps");
			      break;
			    }
			  if (data[1] == 0x17)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? ((data[2] & 0xc0) == 0xc0
					? "movlhpd" : "movhpd")
				     : (data[2] & 0xc0) == 0xc0
				     ? "movlhps"
				     : "movhps");
			      break;
			    }
			  if (data[1] == 0x28 || data[1] == 0x29)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16) ? "movapd" : "movaps";
			      break;
			    }
			  if (data[1] == 0x2a)
			    {
			      bufcnt = 0;
			      int mod = prefixes & (has_data16 | has_rep
						    | has_repne);
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "cvtpi2pd"
				     : mod & has_rep
				     ? "cvtsi2ss"
				     : mod & has_repne
				     ? "cvtsi2sd"
				     : "cvtpi2ps");
			      break;
			    }
			  if (data[1] == 0x2b)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      prefixes ^= mod;
			      str = (mod & has_data16) ? "movntpd" : "movntps";
			      break;
			    }
			  if (data[1] == 0x2c)
			    {
			      bufcnt = 0;
			      int mod = prefixes & (has_data16 | has_rep
						    | has_repne);
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "cvttpd2pi"
				     : mod & has_rep
				     ? "cvttss2si"
				     : mod & has_repne
				     ? "cvttsd2si"
				     : "cvttps2pi");
			      break;
			    }
			  if (data[1] == 0x2d)
			    {
			      bufcnt = 0;
			      int mod = prefixes & (has_data16 | has_rep
						    | has_repne);
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "cvtpd2pi"
				     : mod & has_rep
				     ? "cvtss2si"
				     : mod & has_repne
				     ? "cvtsd2si"
				     : "cvtps2pi");
			      break;
			    }
			  if (data[1] == 0x2e)
			    {
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "ucomisd" : "ucomiss");
			      break;
			    }
			  if (data[1] == 0x2f)
			    {
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "comisd" : "comiss");
			      break;
			    }
			  if (data[1] == 0x50)
			    {
			      int mod = prefixes & has_data16;
			      if (mod & (mod - 1))
				return -1;
			      prefixes ^= mod;
			      str = (mod & has_data16
				     ? "movmskpd" : "movmskps");
			      break;
			    }
			  if (data[1] == 0x51)
			    {
			      bufcnt = 0;
			      int mod = prefixes & (has_data16 | has_rep
						    | has_repne);
			      if (mod & (mod - 1))
				return -1;
			      str = (mod & has_data16
				     ? "sqrtpd"
				     : mod & has_rep
				     ? "sqrtss"
				     : mod & has_repne
				     ? "sqrtsd"
				     : "sqrtps");
			      break;
			    }
			  if (data[1] == 0x52)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_rep;
			      if (mod & (mod - 1))
				return -1;
			      str = mod & has_rep ? "rsqrtss" : "rsqrtps";
			      break;
			    }
			  if (data[1] == 0x53)
			    {
			      bufcnt = 0;
			      int mod = prefixes & has_rep;
			      if (mod & (mod - 1))
				return -1;
			      str = mod & has_rep ? "rcpss" : "rcpps";
			      break;
			    }
			  /* FALLTHROUGH */

			default:
			  abort ();
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
#ifdef x86_64
		      else
			abort ();
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

		      int r = op1_fct[instrtab[cnt].fct1] (addr
							   + (data - begin),
							   &prefixes,
#ifdef STR1_BITS
							   op1_str[instrtab[cnt].str1],
#else
							   NULL,
#endif
							   instrtab[cnt].off1_1 + OFF1_1_BIAS,
							   instrtab[cnt].off1_2 + OFF1_2_BIAS,
							   instrtab[cnt].off1_3 + OFF1_3_BIAS,
							   buf, &bufcnt, bufsize,
							   data, &param_start,
							   end,
							   symcb, symcbarg);
		      if (r < 0)
			goto not;
		      if (r > 0)
			goto enomem;

		      string_end_idx = ~0ul;
		    }
		  else if (prec == 2 && instrtab[cnt].fct2 != 0)
		    {
		      /* Second parameter.  */
#ifdef STR2_BITS
		      // XXX Probably not needed once the instruction
		      // XXX tables are complete
		      if (instrtab[cnt].str2 != 0)
			ADD_STRING (op2_str[instrtab[cnt].str2]);
#endif

		      int r = op2_fct[instrtab[cnt].fct2] (addr
							   + (data - begin),
							   &prefixes,
#ifdef STR2_BITS
							   op2_str[instrtab[cnt].str2],
#else
							   NULL,
#endif
							   instrtab[cnt].off2_1 + OFF2_1_BIAS,
							   instrtab[cnt].off2_2 + OFF2_2_BIAS,
							   instrtab[cnt].off2_3 + OFF2_3_BIAS,
							   buf, &bufcnt, bufsize,
							   data, &param_start,
							   end,
							   symcb, symcbarg);
		      if (r < 0)
			goto not;
		      if (r > 0)
			goto enomem;

		      string_end_idx = ~0ul;
		    }
		  else if (prec == 3 && instrtab[cnt].fct3 != 0)
		    {
		      /* Third parameter.  */
#ifdef STR3_BITS
		      // XXX Probably not needed once the instruction
		      // XXX tables are complete
		      if (instrtab[cnt].str3 != 0)
			ADD_STRING (op3_str[instrtab[cnt].str3]);
#endif

		      int r = op3_fct[instrtab[cnt].fct3] (addr
							   + (data - begin),
							   &prefixes,
#ifdef STR3_BITS
							   op3_str[instrtab[cnt].str3],
#else
							   NULL,
#endif
							   instrtab[cnt].off3_1 + OFF3_1_BIAS,
#ifdef OFF3_2_BITS
							   instrtab[cnt].off3_2 + OFF3_2_BIAS,
#else
							   0,
#endif
#ifdef OFF3_3_BITS
							   instrtab[cnt].off3_3 + OFF3_3_BIAS,
#else
							   0,
#endif
							   buf, &bufcnt, bufsize,
							   data, &param_start,
							   end,
							   symcb, symcbarg);
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
