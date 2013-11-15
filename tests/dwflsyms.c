/* Test program for libdwfl symbol resolving
   Copyright (C) 2013 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include <assert.h>
#include <inttypes.h>
#include ELFUTILS_HEADER(dwfl)
#include <elf.h>
#include <dwarf.h>
#include <argp.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>

static const char *
gelf_type (GElf_Sym *sym)
{
  switch (GELF_ST_TYPE (sym->st_info))
    {
    case STT_NOTYPE:
      return "NOTYPE";
    case STT_OBJECT:
      return "OBJECT";
    case STT_FUNC:
      return "FUNC";
    case STT_SECTION:
      return "SECTION";
    case STT_FILE:
      return "FILE";
    case STT_COMMON:
      return "COMMON";
    case STT_TLS:
      return "TLS";
    default:
      return "UNKNOWN";
    }
}

static const char *
gelf_bind (GElf_Sym *sym)
{
  switch (GELF_ST_BIND (sym->st_info))
    {
    case STB_LOCAL:
      return "LOCAL";
    case STB_GLOBAL:
      return "GLOBAL";
    case STB_WEAK:
      return "WEAK";
    default:
      return "UNKNOWN";
    }
}

static int
list_syms (struct Dwfl_Module *mod,
	   void **user __attribute__ ((unused)),
	   const char *mod_name __attribute__ ((unused)),
	   Dwarf_Addr low_addr __attribute__ ((unused)),
	   void *arg __attribute__ ((unused)))
{
  int syms = dwfl_module_getsymtab (mod);
  assert (syms >= 0);

  for (int ndx = 0; ndx < syms; ndx++)
    {
      GElf_Sym sym;
      GElf_Word shndxp;
      const char *name = dwfl_module_getsym (mod, ndx, &sym, &shndxp);
      printf("%4d: %s\t%s\t%s (%" PRIu64 ") %#" PRIx64,
	     ndx, gelf_type (&sym), gelf_bind (&sym), name,
	     sym.st_size, sym.st_value);

      /* And the reverse, which works for function symbols at least.
	 Note this only works because the st.value is adjusted by
	 dwfl_module_getsym ().  */
      if (GELF_ST_TYPE (sym.st_info) == STT_FUNC && shndxp != SHN_UNDEF)
	{
	  GElf_Addr addr = sym.st_value;
	  GElf_Sym asym;
	  GElf_Word ashndxp;
	  const char *aname = dwfl_module_addrsym (mod, addr, &asym, &ashndxp);
	  assert (strcmp (name, aname) == 0);

	  int res = dwfl_module_relocate_address (mod, &addr);
	  assert (res != -1);
	  printf(", rel: %#" PRIx64 "", addr);
	}
      printf ("\n");
    }

  return DWARF_CB_OK;
}

int
main (int argc, char *argv[])
{
  int remaining;
  Dwfl *dwfl;
  error_t res;

  res = argp_parse (dwfl_standard_argp (), argc, argv, 0, &remaining, &dwfl);
  assert (res == 0 && dwfl != NULL);

  ptrdiff_t off = 0;
  do
    off = dwfl_getmodules (dwfl, list_syms, NULL, off);
  while (off > 0);

  dwfl_end (dwfl);

  return off;
}
