#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <libasm.h>

struct instr_enc
{
  /* The mnemonic.  Especially encoded for the optimized table.  */
  unsigned int mnemonic : MNEMONIC_BITS;

  /* The rep/repe prefixes.  */
  unsigned int rep : 1;
  unsigned int repe : 1;

  /* Mnemonic suffix.  */
  unsigned int suffix : SUFFIX_BITS;

  /* Nonzero if the instruction uses modr/m.  */
  unsigned int modrm : 1;

  /* 1st parameter.  */
  unsigned int fct1 : FCT1_BITS;
#ifdef STR1_BITS
  unsigned int str1 : STR1_BITS;
#endif
  unsigned int off1_1 : OFF1_1_BITS;
  unsigned int off1_2 : OFF1_2_BITS;
  unsigned int off1_3 : OFF1_3_BITS;

  /* 2nd parameter.  */
  unsigned int fct2 : FCT2_BITS;
#ifdef STR2_BITS
  unsigned int str2 : STR2_BITS;
#endif
  unsigned int off2_1 : OFF2_1_BITS;
  unsigned int off2_2 : OFF2_2_BITS;
  unsigned int off2_3 : OFF2_3_BITS;

  /* 3rd parameter.  */
  unsigned int fct3 : FCT3_BITS;
#ifdef STR3_BITS
  unsigned int str3 : STR3_BITS;
#endif
  unsigned int off3_1 : OFF3_1_BITS;
#ifdef OFF3_2_BITS
  unsigned int off3_2 : OFF3_2_BITS;
#endif
#ifdef OFF3_3_BITS
  unsigned int off3_3 : OFF3_3_BITS;
#endif
};


typedef int (*opfct_t) (GElf_Addr, int *, const char *, size_t, size_t, size_t,
			char *, size_t *, size_t, const uint8_t *data,
			const uint8_t **param_start, const uint8_t *end,
			DisasmGetSymCB_t, void *);


static int
data_prefix (int *prefixes, char *bufp, size_t *bufcntp, size_t bufsize)
{
  char ch = '\0';
  if (*prefixes & has_cs)
    {
      ch = 'c';
      *prefixes &= ~has_cs;
    }
  else if (*prefixes & has_ds)
    {
      ch = 'd';
      *prefixes &= ~has_ds;
    }
  else if (*prefixes & has_es)
    {
      ch = 'e';
      *prefixes &= has_es;
    }
  else if (*prefixes & has_fs)
    {
      ch = 'f';
      *prefixes &= ~has_fs;
    }
  else if (*prefixes & has_gs)
    {
      ch = 'g';
      *prefixes &= ~has_gs;
    }
  else if (*prefixes & has_ss)
    {
      ch = 's';
      *prefixes &= ~has_ss;
    }
  else
    return 0;

  if (*bufcntp + 4 > bufsize)
    return *bufcntp + 4 - bufsize;

  bufp[(*bufcntp)++] = '%';
  bufp[(*bufcntp)++] = ch;
  bufp[(*bufcntp)++] = 's';
  bufp[(*bufcntp)++] = ':';

  return 0;
}

static const char regs[8][4] =
  {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
  };

static int
general_mod$r_m (GElf_Addr addr __attribute__ ((unused)),
		 int *prefixes __attribute__ ((unused)),
		 const char *op1str __attribute__ ((unused)),
		 size_t opoff1 __attribute__ ((unused)),
		 size_t opoff2 __attribute__ ((unused)),
		 size_t opoff3 __attribute__ ((unused)),
		 char *bufp __attribute__ ((unused)),
		 size_t *bufcntp __attribute__ ((unused)),
		 size_t bufsize __attribute__ ((unused)),
		 const uint8_t *data __attribute__ ((unused)),
		 const uint8_t **param_start __attribute__ ((unused)),
		 const uint8_t *end __attribute__ ((unused)),
		 DisasmGetSymCB_t symcb __attribute__ ((unused)),
		 void *symcbarg __attribute__ ((unused)))
{
  int r = data_prefix (prefixes, bufp, bufcntp, bufsize);
  if (r != 0)
    return r;

  uint_fast8_t modrm = data[opoff1 / 8];
  if ((*prefixes & has_addr16) == 0)
    {
      if ((modrm & 7) != 4)
	{
	  int32_t disp = 0;
	  bool nodisp = false;

	  if ((modrm & 0xc7) == 5 || (modrm & 0xc0) == 0x80)
	    /* 32 bit displacement.  */
	    disp = read_4sbyte_unaligned (&data[opoff1 / 8 + 1]);
	  else if ((modrm & 0xc0) == 0x40)
	    /* 8 bit displacement.  */
	    disp = *(const int8_t *) &data[opoff1 / 8 + 1];
	  else if ((modrm & 0xc0) == 0)
	    nodisp = true;

	  char tmpbuf[sizeof ("-0x12345678(%rrr)")];
	  int n;
	  if (nodisp)
	    n = snprintf (tmpbuf, sizeof (tmpbuf), "(%%%s)", regs[modrm & 7]);
	  else if ((modrm & 0xc7) != 5)
	    n = snprintf (tmpbuf, sizeof (tmpbuf), "%s0x%" PRIx32 "(%%%s)",
			  disp < 0 ? "-" : "", disp < 0 ? -disp : disp,
			  regs[modrm & 7]);
	  else
	    n = snprintf (tmpbuf, sizeof (tmpbuf), "0x%" PRIx32, disp);

	  if (*bufcntp + n + 1 > bufsize)
	    return *bufcntp + n + 1 - bufsize;

	  memcpy (&bufp[*bufcntp], tmpbuf, n + 1);
	  *bufcntp += n;
	}
      else
	{
	  /* SIB */
	  uint_fast8_t sib = data[opoff1 / 8 + 1];
	  int32_t disp = 0;
	  bool nodisp = false;

	  if ((modrm & 0xc7) == 5 || (modrm & 0xc0) == 0x80
	      || ((modrm & 0xc7) == 0x4 && (sib & 0x7) == 0x5))
	    /* 32 bit displacement.  */
	    disp = read_4sbyte_unaligned (&data[opoff1 / 8 + 2]);
	  else if ((modrm & 0xc0) == 0x40)
	    /* 8 bit displacement.  */
	    disp = *(const int8_t *) &data[opoff1 / 8 + 2];
	  else
	    nodisp = true;

	  char tmpbuf[sizeof ("-0x12345678(%rrr,%rrr,N)")];
	  char *cp = tmpbuf;
	  int n;
	  if ((modrm & 0xc0) != 0 || (sib & 0x3f) != 0x25)
	    {
	      if (!nodisp)
		{
		  n = snprintf (cp, sizeof (tmpbuf), "%s0x%" PRIx32,
				disp < 0 ? "-" : "", disp < 0 ? -disp : disp);
		  cp += n;
		}

	      *cp++ = '(';

	      if ((modrm & 0xc7) != 0x4 || (sib & 0x7) != 0x5)
		{
		  *cp++ = '%';
		  cp = mempcpy (cp, regs[sib & 7], 3);
		}

	      if ((sib & 0x38) != 0x20)
		{
		  *cp++ = ',';
		  *cp++ = '%';
		  cp = mempcpy (cp, regs[(sib >> 3) & 7], 3);

		  *cp++ = ',';
		  *cp++ = '0' + (1 << (sib >> 6));
		}

	      *cp++ = ')';
	    }
	  else
	    {
	      assert (! nodisp);
	      n = snprintf (cp, sizeof (tmpbuf), "0x%" PRIx32, disp);
	      cp += n;
	    }

	  if (*bufcntp + (cp - tmpbuf) > bufsize)
	    return *bufcntp + (cp - tmpbuf) - bufsize;

	  memcpy (&bufp[*bufcntp], tmpbuf, cp - tmpbuf);
	  *bufcntp += cp - tmpbuf;
	}
    }
  return 0;
}


static int
FCT_MOD$R_M (GElf_Addr addr __attribute__ ((unused)),
	     int *prefixes __attribute__ ((unused)),
	     const char *op1str __attribute__ ((unused)),
	     size_t opoff1 __attribute__ ((unused)),
	     size_t opoff2 __attribute__ ((unused)),
	     size_t opoff3 __attribute__ ((unused)),
	     char *bufp __attribute__ ((unused)),
	     size_t *bufcntp __attribute__ ((unused)),
	     size_t bufsize __attribute__ ((unused)),
	     const uint8_t *data __attribute__ ((unused)),
	     const uint8_t **param_start __attribute__ ((unused)),
	     const uint8_t *end __attribute__ ((unused)),
	     DisasmGetSymCB_t symcb __attribute__ ((unused)),
	     void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 % 8 == 0);
  uint_fast8_t modrm = data[opoff1 / 8];
  if ((modrm & 0xc0) == 0xc0)
    {
      uint_fast8_t byte = data[opoff2 / 8] & 7;
      assert (opoff2 % 8 == 5);
      size_t avail = bufsize - *bufcntp;
      int needed;
      if (*prefixes & (has_rep | has_repne))
	needed = snprintf (&bufp[*bufcntp], avail, "%%%s", regs[byte]);
      else
	needed = snprintf (&bufp[*bufcntp], avail, "%%mm%" PRIxFAST8, byte);
      if ((size_t) needed > avail)
	return needed - avail;
      *bufcntp += needed;
      return 0;
    }

  return general_mod$r_m (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
			  bufcntp, bufsize, data, param_start, end,
			  symcb, symcbarg);
}


static int
FCT_Mod$R_m (GElf_Addr addr __attribute__ ((unused)),
	     int *prefixes __attribute__ ((unused)),
	     const char *op1str __attribute__ ((unused)),
	     size_t opoff1 __attribute__ ((unused)),
	     size_t opoff2 __attribute__ ((unused)),
	     size_t opoff3 __attribute__ ((unused)),
	     char *bufp __attribute__ ((unused)),
	     size_t *bufcntp __attribute__ ((unused)),
	     size_t bufsize __attribute__ ((unused)),
	     const uint8_t *data __attribute__ ((unused)),
	     const uint8_t **param_start __attribute__ ((unused)),
	     const uint8_t *end __attribute__ ((unused)),
	     DisasmGetSymCB_t symcb __attribute__ ((unused)),
	     void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 % 8 == 0);
  uint_fast8_t modrm = data[opoff1 / 8];
  if ((modrm & 0xc0) == 0xc0)
    {
      uint_fast8_t byte = data[opoff2 / 8] & 7;
      assert (opoff2 % 8 == 5);
      size_t avail = bufsize - *bufcntp;
      int needed = snprintf (&bufp[*bufcntp], avail, "%%xmm%" PRIxFAST8, byte);
      if ((size_t) needed > avail)
	return needed - avail;
      *bufcntp += needed;
      return 0;
    }

  return general_mod$r_m (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
			  bufcntp, bufsize, data, param_start, end,
			  symcb, symcbarg);
}

static int
generic_abs (int *prefixes __attribute__ ((unused)),
	     size_t opoff1 __attribute__ ((unused)),
	     char *bufp __attribute__ ((unused)),
	     size_t *bufcntp __attribute__ ((unused)),
	     size_t bufsize __attribute__ ((unused)),
	     const uint8_t *data __attribute__ ((unused)),
	     const uint8_t **param_start __attribute__ ((unused)),
	     const uint8_t *end __attribute__ ((unused)),
	     DisasmGetSymCB_t symcb __attribute__ ((unused)),
	     void *symcbarg __attribute__ ((unused)),
	     const char *absstring)
{
  int r = data_prefix (prefixes, bufp, bufcntp, bufsize);
  if (r != 0)
    return r;

  assert (opoff1 % 8 == 0);
  assert (opoff1 / 8 == 1);
  if (*param_start + 4 > end)
    return -1;
  *param_start += 4;
  uint32_t absval = read_4ubyte_unaligned (&data[1]);
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "%s0x%" PRIx32,
			 absstring, absval);
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_absval (GElf_Addr addr __attribute__ ((unused)),
	    int *prefixes __attribute__ ((unused)),
	    const char *op1str __attribute__ ((unused)),
	    size_t opoff1 __attribute__ ((unused)),
	    size_t opoff2 __attribute__ ((unused)),
	    size_t opoff3 __attribute__ ((unused)),
	    char *bufp __attribute__ ((unused)),
	    size_t *bufcntp __attribute__ ((unused)),
	    size_t bufsize __attribute__ ((unused)),
	    const uint8_t *data __attribute__ ((unused)),
	    const uint8_t **param_start __attribute__ ((unused)),
	    const uint8_t *end __attribute__ ((unused)),
	    DisasmGetSymCB_t symcb __attribute__ ((unused)),
	    void *symcbarg __attribute__ ((unused)))
{
  return generic_abs (prefixes, opoff1, bufp,
		      bufcntp, bufsize, data, param_start, end, symcb,
		      symcbarg, "$");
}

static int
FCT_abs (GElf_Addr addr __attribute__ ((unused)),
	 int *prefixes __attribute__ ((unused)),
	 const char *op1str __attribute__ ((unused)),
	 size_t opoff1 __attribute__ ((unused)),
	 size_t opoff2 __attribute__ ((unused)),
	 size_t opoff3 __attribute__ ((unused)),
	 char *bufp __attribute__ ((unused)),
	 size_t *bufcntp __attribute__ ((unused)),
	 size_t bufsize __attribute__ ((unused)),
	 const uint8_t *data __attribute__ ((unused)),
	 const uint8_t **param_start __attribute__ ((unused)),
	 const uint8_t *end __attribute__ ((unused)),
	 DisasmGetSymCB_t symcb __attribute__ ((unused)),
	 void *symcbarg __attribute__ ((unused)))
{
  return generic_abs (prefixes, opoff1, bufp,
		      bufcntp, bufsize, data, param_start, end, symcb,
		      symcbarg, "");
}

static int
FCT_ax (GElf_Addr addr __attribute__ ((unused)),
	int *prefixes __attribute__ ((unused)),
	const char *op1str __attribute__ ((unused)),
	size_t opoff1 __attribute__ ((unused)),
	size_t opoff2 __attribute__ ((unused)),
	size_t opoff3 __attribute__ ((unused)),
	char *bufp __attribute__ ((unused)),
	size_t *bufcntp __attribute__ ((unused)),
	size_t bufsize __attribute__ ((unused)),
	const uint8_t *data __attribute__ ((unused)),
	const uint8_t **param_start __attribute__ ((unused)),
	const uint8_t *end __attribute__ ((unused)),
	DisasmGetSymCB_t symcb __attribute__ ((unused)),
	void *symcbarg __attribute__ ((unused)))
{
  int is_16bit = (*prefixes & has_data16) != 0;

  if (*bufcntp + 4 - is_16bit > bufsize)
    return *bufcntp + 4 - is_16bit - bufsize;

  bufp[(*bufcntp)++] = '%';
  if (! is_16bit)
    bufp[(*bufcntp)++] = 'e';
  bufp[(*bufcntp)++] = 'a';
  bufp[(*bufcntp)++] = 'x';

  return 0;
}

static int
FCT_ax$w (GElf_Addr addr __attribute__ ((unused)),
	  int *prefixes __attribute__ ((unused)),
	  const char *op1str __attribute__ ((unused)),
	  size_t opoff1 __attribute__ ((unused)),
	  size_t opoff2 __attribute__ ((unused)),
	  size_t opoff3 __attribute__ ((unused)),
	  char *bufp __attribute__ ((unused)),
	  size_t *bufcntp __attribute__ ((unused)),
	  size_t bufsize __attribute__ ((unused)),
	  const uint8_t *data __attribute__ ((unused)),
	  const uint8_t **param_start __attribute__ ((unused)),
	  const uint8_t *end __attribute__ ((unused)),
	  DisasmGetSymCB_t symcb __attribute__ ((unused)),
	  void *symcbarg __attribute__ ((unused)))
{
  if ((data[opoff2 / 8] & (1 << (7 - (opoff2 & 7)))) != 0)
    return FCT_ax (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
		   bufcntp, bufsize, data, param_start, end, symcb, symcbarg);

  if (*bufcntp + 3 > bufsize)
    return *bufcntp + 3 - bufsize;

  bufp[(*bufcntp)++] = '%';
  bufp[(*bufcntp)++] = 'a';
  bufp[(*bufcntp)++] = 'l';

  return 0;
}

static int
FCT_ccc (GElf_Addr addr __attribute__ ((unused)),
	 int *prefixes __attribute__ ((unused)),
	 const char *op1str __attribute__ ((unused)),
	 size_t opoff1 __attribute__ ((unused)),
	 size_t opoff2 __attribute__ ((unused)),
	 size_t opoff3 __attribute__ ((unused)),
	 char *bufp __attribute__ ((unused)),
	 size_t *bufcntp __attribute__ ((unused)),
	 size_t bufsize __attribute__ ((unused)),
	 const uint8_t *data __attribute__ ((unused)),
	 const uint8_t **param_start __attribute__ ((unused)),
	 const uint8_t *end __attribute__ ((unused)),
	 DisasmGetSymCB_t symcb __attribute__ ((unused)),
	 void *symcbarg __attribute__ ((unused)))
{
  if (*prefixes & has_data16)
    return -1;

  assert (opoff1 % 8 == 2);
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "%%cr%" PRIx32,
			 (uint32_t) (data[opoff1 / 8] >> 3) & 7);
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_ddd (GElf_Addr addr __attribute__ ((unused)),
	 int *prefixes __attribute__ ((unused)),
	 const char *op1str __attribute__ ((unused)),
	 size_t opoff1 __attribute__ ((unused)),
	 size_t opoff2 __attribute__ ((unused)),
	 size_t opoff3 __attribute__ ((unused)),
	 char *bufp __attribute__ ((unused)),
	 size_t *bufcntp __attribute__ ((unused)),
	 size_t bufsize __attribute__ ((unused)),
	 const uint8_t *data __attribute__ ((unused)),
	 const uint8_t **param_start __attribute__ ((unused)),
	 const uint8_t *end __attribute__ ((unused)),
	 DisasmGetSymCB_t symcb __attribute__ ((unused)),
	 void *symcbarg __attribute__ ((unused)))
{
  if (*prefixes & has_data16)
    return -1;

  assert (opoff1 % 8 == 2);
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "%%db%" PRIx32,
			 (uint32_t) (data[opoff1 / 8] >> 3) & 7);
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_disp8 (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 % 8 == 0);
  int32_t offset = *(const int8_t *) (*param_start)++;
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "0x%" PRIx32,
			 (uint32_t) (addr + (*param_start - data) + offset));
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
__attribute__ ((noinline))
FCT_ds_xx (const char *reg,
	   int *prefixes __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)))
{
  int prefix = *prefixes & SEGMENT_PREFIXES;

  if (prefix == 0)
    prefix = has_ds;
  /* Make sure only one bit is set.  */
  else if ((prefix - 1) & prefix)
    return -1;
  else
    *prefixes ^= prefix;

  int r = data_prefix (&prefix, bufp, bufcntp, bufsize);
  if (r != 0)
    return r;

  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "(%%%s%s)",
			 *prefixes & idx_addr16 ? "" : "e", reg);
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;

  return 0;
}

static int
FCT_ds_bx (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  return FCT_ds_xx ("bx", prefixes, bufp, bufcntp, bufsize);
}

static int
FCT_ds_si (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  return FCT_ds_xx ("si", prefixes, bufp, bufcntp, bufsize);
}

static int
FCT_dx (GElf_Addr addr __attribute__ ((unused)),
	int *prefixes __attribute__ ((unused)),
	const char *op1str __attribute__ ((unused)),
	size_t opoff1 __attribute__ ((unused)),
	size_t opoff2 __attribute__ ((unused)),
	size_t opoff3 __attribute__ ((unused)),
	char *bufp __attribute__ ((unused)),
	size_t *bufcntp __attribute__ ((unused)),
	size_t bufsize __attribute__ ((unused)),
	const uint8_t *data __attribute__ ((unused)),
	const uint8_t **param_start __attribute__ ((unused)),
	const uint8_t *end __attribute__ ((unused)),
	DisasmGetSymCB_t symcb __attribute__ ((unused)),
	void *symcbarg __attribute__ ((unused)))
{
  if (*bufcntp + 7 > bufsize)
    return *bufcntp + 7 - bufsize;

  memcpy (&bufp[*bufcntp], "(%dx)", 5);
  *bufcntp += 5;

  return 0;
}

static int
FCT_es_di (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "%%es:(%%%sdi)",
			 *prefixes & idx_addr16 ? "" : "e");
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;

  return 0;
}

static int
FCT_imm (GElf_Addr addr __attribute__ ((unused)),
	 int *prefixes __attribute__ ((unused)),
	 const char *op1str __attribute__ ((unused)),
	 size_t opoff1 __attribute__ ((unused)),
	 size_t opoff2 __attribute__ ((unused)),
	 size_t opoff3 __attribute__ ((unused)),
	 char *bufp __attribute__ ((unused)),
	 size_t *bufcntp __attribute__ ((unused)),
	 size_t bufsize __attribute__ ((unused)),
	 const uint8_t *data __attribute__ ((unused)),
	 const uint8_t **param_start __attribute__ ((unused)),
	 const uint8_t *end __attribute__ ((unused)),
	 DisasmGetSymCB_t symcb __attribute__ ((unused)),
	 void *symcbarg __attribute__ ((unused)))
{
  size_t avail = bufsize - *bufcntp;
  int needed;
  if (*prefixes & has_data16)
    {
      if (*param_start + 2 > end)
	return -1;
      uint16_t word = read_2ubyte_unaligned_inc (*param_start);
      needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx16, word);
    }
  else
    {
      if (*param_start + 4 > end)
	return -1;
      uint32_t word = read_4ubyte_unaligned_inc (*param_start);
      needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx32, word);
    }
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_imm$w (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  if ((data[opoff2 / 8] & (1 << (7 - (opoff2 & 7)))) != 0)
    return FCT_imm (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
		    bufcntp, bufsize, data, param_start, end, symcb, symcbarg);

  size_t avail = bufsize - *bufcntp;
  uint_fast8_t word = *(*param_start)++;
  int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIxFAST8, word);
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_imms (GElf_Addr addr __attribute__ ((unused)),
	  int *prefixes __attribute__ ((unused)),
	  const char *op1str __attribute__ ((unused)),
	  size_t opoff1 __attribute__ ((unused)),
	  size_t opoff2 __attribute__ ((unused)),
	  size_t opoff3 __attribute__ ((unused)),
	  char *bufp __attribute__ ((unused)),
	  size_t *bufcntp __attribute__ ((unused)),
	  size_t bufsize __attribute__ ((unused)),
	  const uint8_t *data __attribute__ ((unused)),
	  const uint8_t **param_start __attribute__ ((unused)),
	  const uint8_t *end __attribute__ ((unused)),
	  DisasmGetSymCB_t symcb __attribute__ ((unused)),
	  void *symcbarg __attribute__ ((unused)))
{
  size_t avail = bufsize - *bufcntp;
  int_fast8_t byte = *(*param_start)++;
  int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx32,
			 (int32_t) byte);
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_imm$s (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  uint_fast8_t opcode = data[opoff2 / 8];
  size_t avail = bufsize - *bufcntp;
  if ((opcode & 2) != 0)
    return FCT_imms (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
		     bufcntp, bufsize, data, param_start, end, symcb,
		     symcbarg);

  if ((*prefixes & has_data16) == 0)
    {
      if (*param_start + 4 > end)
	return -1;
      uint32_t word = read_4ubyte_unaligned_inc (*param_start);
      int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx32, word);
      if ((size_t) needed > avail)
	return (size_t) needed - avail;
      *bufcntp += needed;
    }
  else
    {
      if (*param_start + 2 > end)
	return -1;
      uint16_t word = read_2ubyte_unaligned_inc (*param_start);
      int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx16, word);
      if ((size_t) needed > avail)
	return (size_t) needed - avail;
      *bufcntp += needed;
    }
  return 0;
}

static int
FCT_imm16 (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  if (*param_start + 2 > end)
    return -1;
  uint16_t word = read_2ubyte_unaligned_inc (*param_start);
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx16, word);
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_imms8 (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  size_t avail = bufsize - *bufcntp;
  int_fast8_t byte = *(*param_start)++;
  int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx32,
			 (int32_t) byte);
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_imm8 (GElf_Addr addr __attribute__ ((unused)),
	  int *prefixes __attribute__ ((unused)),
	  const char *op1str __attribute__ ((unused)),
	  size_t opoff1 __attribute__ ((unused)),
	  size_t opoff2 __attribute__ ((unused)),
	  size_t opoff3 __attribute__ ((unused)),
	  char *bufp __attribute__ ((unused)),
	  size_t *bufcntp __attribute__ ((unused)),
	  size_t bufsize __attribute__ ((unused)),
	  const uint8_t *data __attribute__ ((unused)),
	  const uint8_t **param_start __attribute__ ((unused)),
	  const uint8_t *end __attribute__ ((unused)),
	  DisasmGetSymCB_t symcb __attribute__ ((unused)),
	  void *symcbarg __attribute__ ((unused)))
{
  size_t avail = bufsize - *bufcntp;
  uint_fast8_t byte = *(*param_start)++;
  int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx32,
			 (uint32_t) byte);
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}

#ifndef X86_64
static int
FCT_rel (GElf_Addr addr __attribute__ ((unused)),
	 int *prefixes __attribute__ ((unused)),
	 const char *op1str __attribute__ ((unused)),
	 size_t opoff1 __attribute__ ((unused)),
	 size_t opoff2 __attribute__ ((unused)),
	 size_t opoff3 __attribute__ ((unused)),
	 char *bufp __attribute__ ((unused)),
	 size_t *bufcntp __attribute__ ((unused)),
	 size_t bufsize __attribute__ ((unused)),
	 const uint8_t *data __attribute__ ((unused)),
	 const uint8_t **param_start __attribute__ ((unused)),
	 const uint8_t *end __attribute__ ((unused)),
	 DisasmGetSymCB_t symcb __attribute__ ((unused)),
	 void *symcbarg __attribute__ ((unused)))
{
  size_t avail = bufsize - *bufcntp;
  if (*param_start + 4 > end)
    return -1;
  uint32_t rel = read_4ubyte_unaligned_inc (*param_start);
  int needed = snprintf (&bufp[*bufcntp], avail, "0x%" PRIx32,
			 (uint32_t) (addr + rel + (*param_start - data)));
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}
#endif

static int
FCT_mmxreg (GElf_Addr addr __attribute__ ((unused)),
	    int *prefixes __attribute__ ((unused)),
	    const char *op1str __attribute__ ((unused)),
	    size_t opoff1 __attribute__ ((unused)),
	    size_t opoff2 __attribute__ ((unused)),
	    size_t opoff3 __attribute__ ((unused)),
	    char *bufp __attribute__ ((unused)),
	    size_t *bufcntp __attribute__ ((unused)),
	    size_t bufsize __attribute__ ((unused)),
	    const uint8_t *data __attribute__ ((unused)),
	    const uint8_t **param_start __attribute__ ((unused)),
	    const uint8_t *end __attribute__ ((unused)),
	    DisasmGetSymCB_t symcb __attribute__ ((unused)),
	    void *symcbarg __attribute__ ((unused)))
{
  uint_fast8_t byte = data[opoff1 / 8];
  assert (opoff1 % 8 == 2 || opoff1 % 8 == 5);
  byte = (byte >> (5 - opoff1 % 8)) & 7;
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "%%mm%" PRIxFAST8, byte);
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_mmxreg2 (GElf_Addr addr __attribute__ ((unused)),
	     int *prefixes __attribute__ ((unused)),
	     const char *op1str __attribute__ ((unused)),
	     size_t opoff1 __attribute__ ((unused)),
	     size_t opoff2 __attribute__ ((unused)),
	     size_t opoff3 __attribute__ ((unused)),
	     char *bufp __attribute__ ((unused)),
	     size_t *bufcntp __attribute__ ((unused)),
	     size_t bufsize __attribute__ ((unused)),
	     const uint8_t *data __attribute__ ((unused)),
	     const uint8_t **param_start __attribute__ ((unused)),
	     const uint8_t *end __attribute__ ((unused)),
	     DisasmGetSymCB_t symcb __attribute__ ((unused)),
	     void *symcbarg __attribute__ ((unused)))
{
  uint_fast8_t byte = data[opoff1 / 8];
  assert (opoff1 % 8 == 2 || opoff1 % 8 == 5);
  byte = (byte >> (5 - opoff1 % 8)) & 7;
  size_t avail = bufsize - *bufcntp;
  int needed;
  if (*prefixes & (has_rep | has_repne))
    needed = snprintf (&bufp[*bufcntp], avail, "%%%s", regs[byte]);
  else
    needed = snprintf (&bufp[*bufcntp], avail, "%%mm%" PRIxFAST8, byte);
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}


static int
FCT_mod$r_m (GElf_Addr addr __attribute__ ((unused)),
	     int *prefixes __attribute__ ((unused)),
	     const char *op1str __attribute__ ((unused)),
	     size_t opoff1 __attribute__ ((unused)),
	     size_t opoff2 __attribute__ ((unused)),
	     size_t opoff3 __attribute__ ((unused)),
	     char *bufp __attribute__ ((unused)),
	     size_t *bufcntp __attribute__ ((unused)),
	     size_t bufsize __attribute__ ((unused)),
	     const uint8_t *data __attribute__ ((unused)),
	     const uint8_t **param_start __attribute__ ((unused)),
	     const uint8_t *end __attribute__ ((unused)),
	     DisasmGetSymCB_t symcb __attribute__ ((unused)),
	     void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 % 8 == 0);
  uint_fast8_t modrm = data[opoff1 / 8];
  if ((modrm & 0xc0) == 0xc0)
    {
      int is_16bit = (*prefixes & has_data16) != 0;

      if (*bufcntp + 5 - is_16bit > bufsize)
	return *bufcntp + 5 - is_16bit - bufsize;
      bufp[(*bufcntp)++] = '%';
      memcpy (&bufp[*bufcntp], regs[modrm & 7] + is_16bit,
	      sizeof (regs[0]) - is_16bit);
      *bufcntp += 3 - is_16bit;
      return 0;
    }

  return general_mod$r_m (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
			  bufcntp, bufsize, data, param_start, end,
			  symcb, symcbarg);
}


#ifndef X86_64
static int
FCT_moda$r_m (GElf_Addr addr __attribute__ ((unused)),
	      int *prefixes __attribute__ ((unused)),
	      const char *op1str __attribute__ ((unused)),
	      size_t opoff1 __attribute__ ((unused)),
	      size_t opoff2 __attribute__ ((unused)),
	      size_t opoff3 __attribute__ ((unused)),
	      char *bufp __attribute__ ((unused)),
	      size_t *bufcntp __attribute__ ((unused)),
	      size_t bufsize __attribute__ ((unused)),
	      const uint8_t *data __attribute__ ((unused)),
	      const uint8_t **param_start __attribute__ ((unused)),
	      const uint8_t *end __attribute__ ((unused)),
	      DisasmGetSymCB_t symcb __attribute__ ((unused)),
	      void *symcbarg __attribute__ ((unused)))
{
  int r = data_prefix (prefixes, bufp, bufcntp, bufsize);
  if (r != 0)
    return r;

  assert (opoff1 % 8 == 0);
  uint_fast8_t modrm = data[opoff1 / 8];
  if ((modrm & 0xc0) == 0xc0)
    {
      if (*bufcntp + 3 > bufsize)
	return *bufcntp + 3 - bufsize;

      memcpy (&bufp[*bufcntp], "???", 3);
      *bufcntp += 3;

      return 0;
    }

  return general_mod$r_m (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
			  bufcntp, bufsize, data, param_start, end,
			  symcb, symcbarg);
}
#endif


static int
FCT_mod$r_m$w (GElf_Addr addr __attribute__ ((unused)),
	       int *prefixes __attribute__ ((unused)),
	       const char *op1str __attribute__ ((unused)),
	       size_t opoff1 __attribute__ ((unused)),
	       size_t opoff2 __attribute__ ((unused)),
	       size_t opoff3 __attribute__ ((unused)),
	       char *bufp __attribute__ ((unused)),
	       size_t *bufcntp __attribute__ ((unused)),
	       size_t bufsize __attribute__ ((unused)),
	       const uint8_t *data __attribute__ ((unused)),
	       const uint8_t **param_start __attribute__ ((unused)),
	       const uint8_t *end __attribute__ ((unused)),
	       DisasmGetSymCB_t symcb __attribute__ ((unused)),
	       void *symcbarg __attribute__ ((unused)))
{
  int r = data_prefix (prefixes, bufp, bufcntp, bufsize);
  if (r != 0)
    return r;

  assert (opoff1 % 8 == 0);
  uint_fast8_t modrm = data[opoff1 / 8];
  if ((modrm & 0xc0) == 0xc0)
    {
      if ((data[opoff3 / 8] & (1 << (7 - (opoff3 & 7)))) == 0)
	{
	  if (*bufcntp + 3 > bufsize)
	    return *bufcntp + 3 - bufsize;
	  bufp[(*bufcntp)++] = '%';
	  bufp[(*bufcntp)++] = "acdb"[modrm & 3];
	  bufp[(*bufcntp)++] = "lh"[(modrm & 4) >> 2];
	}
      else
	{
	  int is_16bit = (*prefixes & has_data16) != 0;

	  if (*bufcntp + 5 - is_16bit > bufsize)
	    return *bufcntp + 5 - is_16bit - bufsize;
	  bufp[(*bufcntp)++] = '%';
	  memcpy (&bufp[*bufcntp], regs[modrm & 7] + is_16bit,
		  sizeof (regs[0]) - is_16bit);
	  *bufcntp += 3 - is_16bit;
	}
      return 0;
    }

  return general_mod$r_m (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
			  bufcntp, bufsize, data, param_start, end,
			  symcb, symcbarg);
}


#ifndef X86_64
static int
FCT_mod$8r_m (GElf_Addr addr __attribute__ ((unused)),
	      int *prefixes __attribute__ ((unused)),
	      const char *op1str __attribute__ ((unused)),
	      size_t opoff1 __attribute__ ((unused)),
	      size_t opoff2 __attribute__ ((unused)),
	      size_t opoff3 __attribute__ ((unused)),
	      char *bufp __attribute__ ((unused)),
	      size_t *bufcntp __attribute__ ((unused)),
	      size_t bufsize __attribute__ ((unused)),
	      const uint8_t *data __attribute__ ((unused)),
	      const uint8_t **param_start __attribute__ ((unused)),
	      const uint8_t *end __attribute__ ((unused)),
	      DisasmGetSymCB_t symcb __attribute__ ((unused)),
	      void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 % 8 == 0);
  uint_fast8_t modrm = data[opoff1 / 8];
  if ((modrm & 0xc0) == 0xc0)
    {
      if (*bufcntp + 3 > bufsize)
	return *bufcntp + 3 - bufsize;
      bufp[(*bufcntp)++] = '%';
      bufp[(*bufcntp)++] = "acdb"[modrm & 3];
      bufp[(*bufcntp)++] = "lh"[(modrm & 4) >> 2];
      return 0;
    }

  return general_mod$r_m (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
			  bufcntp, bufsize, data, param_start, end,
			  symcb, symcbarg);
}
#endif

static int
FCT_mod$16r_m (GElf_Addr addr __attribute__ ((unused)),
	       int *prefixes __attribute__ ((unused)),
	       const char *op1str __attribute__ ((unused)),
	       size_t opoff1 __attribute__ ((unused)),
	       size_t opoff2 __attribute__ ((unused)),
	       size_t opoff3 __attribute__ ((unused)),
	       char *bufp __attribute__ ((unused)),
	       size_t *bufcntp __attribute__ ((unused)),
	       size_t bufsize __attribute__ ((unused)),
	       const uint8_t *data __attribute__ ((unused)),
	       const uint8_t **param_start __attribute__ ((unused)),
	       const uint8_t *end __attribute__ ((unused)),
	       DisasmGetSymCB_t symcb __attribute__ ((unused)),
	       void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 % 8 == 0);
  uint_fast8_t modrm = data[opoff1 / 8];
  if ((modrm & 0xc0) == 0xc0)
    {
      uint_fast8_t byte = data[opoff1 / 8] & 7;
      if (*bufcntp + 3 > bufsize)
	return *bufcntp + 3 - bufsize;
      bufp[(*bufcntp)++] = '%';
      memcpy (&bufp[*bufcntp], regs[byte] + 1, sizeof (regs[0]) - 1);
      *bufcntp += 2;
      return 0;
    }

  return general_mod$r_m (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
			  bufcntp, bufsize, data, param_start, end,
			  symcb, symcbarg);
}

static int
FCT_reg (GElf_Addr addr __attribute__ ((unused)),
	 int *prefixes __attribute__ ((unused)),
	 const char *op1str __attribute__ ((unused)),
	 size_t opoff1 __attribute__ ((unused)),
	 size_t opoff2 __attribute__ ((unused)),
	 size_t opoff3 __attribute__ ((unused)),
	 char *bufp __attribute__ ((unused)),
	 size_t *bufcntp __attribute__ ((unused)),
	 size_t bufsize __attribute__ ((unused)),
	 const uint8_t *data __attribute__ ((unused)),
	 const uint8_t **param_start __attribute__ ((unused)),
	 const uint8_t *end __attribute__ ((unused)),
	 DisasmGetSymCB_t symcb __attribute__ ((unused)),
	 void *symcbarg __attribute__ ((unused)))
{
  uint_fast8_t byte = data[opoff1 / 8];
  assert (opoff1 % 8 + 3 <= 8);
  byte >>= 8 - (opoff1 % 8 + 3);
  byte &= 7;
  int is_16bit = (*prefixes & has_data16) != 0;
  if (*bufcntp + 4 > bufsize)
    return *bufcntp + 4 - bufsize;
  bufp[(*bufcntp)++] = '%';
  memcpy (&bufp[*bufcntp], regs[byte] + is_16bit, sizeof (regs[0]) - is_16bit);
  *bufcntp += 3 - is_16bit;
  return 0;
}

static int
FCT_reg$w (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  if (data[opoff2 / 8] & (1 << (7 - (opoff2 & 7))))
    return FCT_reg (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
		    bufcntp, bufsize, data, param_start, end, symcb, symcbarg);
  uint_fast8_t byte = data[opoff1 / 8];
  assert (opoff1 % 8 + 3 <= 8);
  byte >>= 8 - (opoff1 % 8 + 3);
  byte &= 7;
  if (*bufcntp + 3 > bufsize)
    return *bufcntp + 3 - bufsize;
  bufp[(*bufcntp)++] = '%';
  bufp[(*bufcntp)++] = "acdb"[byte & 3];
  bufp[(*bufcntp)++] = "lh"[byte >> 2];
  return 0;
}

#ifndef X86_64
static int
FCT_freg (GElf_Addr addr __attribute__ ((unused)),
	  int *prefixes __attribute__ ((unused)),
	  const char *op1str __attribute__ ((unused)),
	  size_t opoff1 __attribute__ ((unused)),
	  size_t opoff2 __attribute__ ((unused)),
	  size_t opoff3 __attribute__ ((unused)),
	  char *bufp __attribute__ ((unused)),
	  size_t *bufcntp __attribute__ ((unused)),
	  size_t bufsize __attribute__ ((unused)),
	  const uint8_t *data __attribute__ ((unused)),
	  const uint8_t **param_start __attribute__ ((unused)),
	  const uint8_t *end __attribute__ ((unused)),
	  DisasmGetSymCB_t symcb __attribute__ ((unused)),
	  void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 / 8 == 1);
  assert (opoff1 % 8 == 5);
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "%%st(%" PRIx32 ")",
			 (uint32_t) (data[1] & 7));
  if ((size_t) needed > avail)
    return (size_t) needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_reg16 (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  if (*prefixes & has_data16)
    return -1;

  *prefixes |= has_data16;
  return FCT_reg (addr, prefixes, op1str, opoff1, opoff2, opoff3, bufp,
		  bufcntp, bufsize, data, param_start, end, symcb, symcbarg);
}
#endif

static int
FCT_sel (GElf_Addr addr __attribute__ ((unused)),
	 int *prefixes __attribute__ ((unused)),
	 const char *op1str __attribute__ ((unused)),
	 size_t opoff1 __attribute__ ((unused)),
	 size_t opoff2 __attribute__ ((unused)),
	 size_t opoff3 __attribute__ ((unused)),
	 char *bufp __attribute__ ((unused)),
	 size_t *bufcntp __attribute__ ((unused)),
	 size_t bufsize __attribute__ ((unused)),
	 const uint8_t *data __attribute__ ((unused)),
	 const uint8_t **param_start __attribute__ ((unused)),
	 const uint8_t *end __attribute__ ((unused)),
	 DisasmGetSymCB_t symcb __attribute__ ((unused)),
	 void *symcbarg __attribute__ ((unused)))
{
  assert (opoff1 % 8 == 0);
  assert (opoff1 / 8 == 5);
  if (*param_start + 2 > end)
    return -1;
  *param_start += 2;
  uint16_t absval = read_2ubyte_unaligned (&data[5]);
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "$0x%" PRIx16, absval);
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}

static int
FCT_sreg2 (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  uint_fast8_t byte = data[opoff1 / 8];
  assert (opoff1 % 8 + 3 <= 8);
  byte >>= 8 - (opoff1 % 8 + 2);

  if (*bufcntp + 3 > bufsize)
    return *bufcntp + 3 - bufsize;

  bufp[(*bufcntp)++] = '%';
  bufp[(*bufcntp)++] = "ecsd"[byte & 3];
  bufp[(*bufcntp)++] = 's';

  return 0;
}

static int
FCT_sreg3 (GElf_Addr addr __attribute__ ((unused)),
	   int *prefixes __attribute__ ((unused)),
	   const char *op1str __attribute__ ((unused)),
	   size_t opoff1 __attribute__ ((unused)),
	   size_t opoff2 __attribute__ ((unused)),
	   size_t opoff3 __attribute__ ((unused)),
	   char *bufp __attribute__ ((unused)),
	   size_t *bufcntp __attribute__ ((unused)),
	   size_t bufsize __attribute__ ((unused)),
	   const uint8_t *data __attribute__ ((unused)),
	   const uint8_t **param_start __attribute__ ((unused)),
	   const uint8_t *end __attribute__ ((unused)),
	   DisasmGetSymCB_t symcb __attribute__ ((unused)),
	   void *symcbarg __attribute__ ((unused)))
{
  uint_fast8_t byte = data[opoff1 / 8];
  assert (opoff1 % 8 + 4 <= 8);
  byte >>= 8 - (opoff1 % 8 + 3);

  if ((byte & 7) >= 6)
    return -1;

  if (*bufcntp + 3 > bufsize)
    return *bufcntp + 3 - bufsize;

  bufp[(*bufcntp)++] = '%';
  bufp[(*bufcntp)++] = "ecsdfg"[byte & 7];
  bufp[(*bufcntp)++] = 's';

  return 0;
}

static int
FCT_string (GElf_Addr addr __attribute__ ((unused)),
	    int *prefixes __attribute__ ((unused)),
	    const char *op1str __attribute__ ((unused)),
	    size_t opoff1 __attribute__ ((unused)),
	    size_t opoff2 __attribute__ ((unused)),
	    size_t opoff3 __attribute__ ((unused)),
	    char *bufp __attribute__ ((unused)),
	    size_t *bufcntp __attribute__ ((unused)),
	    size_t bufsize __attribute__ ((unused)),
	    const uint8_t *data __attribute__ ((unused)),
	    const uint8_t **param_start __attribute__ ((unused)),
	    const uint8_t *end __attribute__ ((unused)),
	    DisasmGetSymCB_t symcb __attribute__ ((unused)),
	    void *symcbarg __attribute__ ((unused)))
{
  return 0;
}

static int
FCT_xmmreg (GElf_Addr addr __attribute__ ((unused)),
	    int *prefixes __attribute__ ((unused)),
	    const char *op1str __attribute__ ((unused)),
	    size_t opoff1 __attribute__ ((unused)),
	    size_t opoff2 __attribute__ ((unused)),
	    size_t opoff3 __attribute__ ((unused)),
	    char *bufp __attribute__ ((unused)),
	    size_t *bufcntp __attribute__ ((unused)),
	    size_t bufsize __attribute__ ((unused)),
	    const uint8_t *data __attribute__ ((unused)),
	    const uint8_t **param_start __attribute__ ((unused)),
	    const uint8_t *end __attribute__ ((unused)),
	    DisasmGetSymCB_t symcb __attribute__ ((unused)),
	    void *symcbarg __attribute__ ((unused)))
{
  uint_fast8_t byte = data[opoff1 / 8];
  assert (opoff1 % 8 == 2 || opoff1 % 8 == 5);
  byte = (byte >> (5 - opoff1 % 8)) & 7;
  size_t avail = bufsize - *bufcntp;
  int needed = snprintf (&bufp[*bufcntp], avail, "%%xmm%" PRIxFAST8, byte);
  if ((size_t) needed > avail)
    return needed - avail;
  *bufcntp += needed;
  return 0;
}
