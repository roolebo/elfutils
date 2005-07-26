/* Print contents of core note.
   Copyright (C) 2002, 2004, 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <byteswap.h>
#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <libeblP.h>


void
ebl_core_note (ebl, name, type, descsz, desc)
     Ebl *ebl;
     const char *name;
     uint32_t type;
     uint32_t descsz;
     const char *desc;
{
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (ebl->elf, &ehdr_mem);
  assert (ehdr != NULL);
  int class = ehdr->e_ident[EI_CLASS];
  int endian = ehdr->e_ident[EI_DATA];

  if (! ebl->core_note (name, type, descsz, desc))
    /* The machine specific function did not know this type.  */
    switch (type)
      {
      case NT_PLATFORM:
	printf (gettext ("    Platform: %.*s\n"), (int) descsz, desc);
	break;

      case NT_AUXV:
	;
	size_t elsize = (class == ELFCLASS32
			 ? sizeof (Elf32_auxv_t) : sizeof (Elf64_auxv_t));

	for (size_t cnt = 0; (cnt + 1) * elsize <= descsz; ++cnt)
	  {
	    uintmax_t atype;
	    uintmax_t val;

	    if (class == ELFCLASS32)
	      {
		Elf32_auxv_t *auxv = &((Elf32_auxv_t *) desc)[cnt];

		if ((endian == ELFDATA2LSB && __BYTE_ORDER == __LITTLE_ENDIAN)
		    || (endian == ELFDATA2MSB && __BYTE_ORDER == __BIG_ENDIAN))
		  {
		    atype = auxv->a_type;
		    val = auxv->a_un.a_val;
		  }
		else
		  {
		    atype = bswap_32 (auxv->a_type);
		    val = bswap_32 (auxv->a_un.a_val);
		  }
	      }
	    else
	      {
		Elf64_auxv_t *auxv = &((Elf64_auxv_t *) desc)[cnt];

		if ((endian == ELFDATA2LSB && __BYTE_ORDER == __LITTLE_ENDIAN)
		    || (endian == ELFDATA2MSB && __BYTE_ORDER == __BIG_ENDIAN))
		  {
		    atype = auxv->a_type;
		    val = auxv->a_un.a_val;
		  }
		else
		  {
		    atype = bswap_64 (auxv->a_type);
		    val = bswap_64 (auxv->a_un.a_val);
		  }
	      }

	    /* XXX Do we need the auxiliary vector info anywhere
	       else?  If yes, move code into a separate function.  */
	    const char *at;

	    switch (atype)
	      {
#define NEW_AT(name) case AT_##name: at = #name; break
		NEW_AT (NULL);
		NEW_AT (IGNORE);
		NEW_AT (EXECFD);
		NEW_AT (PHDR);
		NEW_AT (PHENT);
		NEW_AT (PHNUM);
		NEW_AT (PAGESZ);
		NEW_AT (BASE);
		NEW_AT (FLAGS);
		NEW_AT (ENTRY);
		NEW_AT (NOTELF);
		NEW_AT (UID);
		NEW_AT (EUID);
		NEW_AT (GID);
		NEW_AT (EGID);
		NEW_AT (CLKTCK);
		NEW_AT (PLATFORM);
		NEW_AT (HWCAP);
		NEW_AT (FPUCW);
		NEW_AT (DCACHEBSIZE);
		NEW_AT (ICACHEBSIZE);
		NEW_AT (UCACHEBSIZE);
		NEW_AT (IGNOREPPC);
		NEW_AT (SECURE);
		NEW_AT (SYSINFO);
		NEW_AT (SYSINFO_EHDR);
		NEW_AT (L1I_CACHESHAPE);
		NEW_AT (L1D_CACHESHAPE);
		NEW_AT (L2_CACHESHAPE);
		NEW_AT (L3_CACHESHAPE);

	      default:;
		static char buf[30];
		sprintf (buf, "%ju (AT_?""?""?)", atype);
		at = buf;
		break;
	      }

	    switch (atype)
	      {
	      case AT_NULL:
	      case AT_IGNORE:
	      case AT_IGNOREPPC:
	      case AT_NOTELF:
	      default:
		printf ("    %s\n", at);
		break;

	      case AT_EXECFD:
	      case AT_PHENT:
	      case AT_PHNUM:
	      case AT_PAGESZ:
	      case AT_UID:
	      case AT_EUID:
	      case AT_GID:
	      case AT_EGID:
	      case AT_CLKTCK:
	      case AT_FPUCW:
	      case AT_DCACHEBSIZE:
	      case AT_ICACHEBSIZE:
	      case AT_UCACHEBSIZE:
	      case AT_SECURE:
	      case AT_L1I_CACHESHAPE:
	      case AT_L1D_CACHESHAPE:
	      case AT_L2_CACHESHAPE:
	      case AT_L3_CACHESHAPE:
		printf ("    %s: %jd\n", at, val);
		break;

	      case AT_PHDR:
	      case AT_BASE:
	      case AT_FLAGS:	/* XXX Print flags?  */
	      case AT_ENTRY:
	      case AT_PLATFORM:	/* XXX Get string?  */
	      case AT_HWCAP:	/* XXX Print flags?  */
	      case AT_SYSINFO:
	      case AT_SYSINFO_EHDR:
		printf ("    %s: %#jx\n", at, val);
		break;
	      }

	    if (atype == AT_NULL)
	      /* Reached the end.  */
	      break;
	  }
	break;

      default:
	/* Unknown type.  */
	break;
      }
}
