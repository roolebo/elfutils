/* Print contents of core note.
   Copyright (C) 2002, 2004, 2005 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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

   In addition, as a special exception, Red Hat, Inc. gives You the
   additional right to link the code of Red Hat elfutils with code licensed
   under any Open Source Initiative certified open source license
   (http://www.opensource.org/licenses/index.php) which requires the
   distribution of source code with any binary distribution and to
   distribute linked combinations of the two.  Non-GPL Code permitted under
   this exception must only link to the code of Red Hat elfutils through
   those well defined interfaces identified in the file named EXCEPTION
   found in the source code files (the "Approved Interfaces").  The files
   of Non-GPL Code may instantiate templates or use macros or inline
   functions from the Approved Interfaces without causing the resulting
   work to be covered by the GNU General Public License.  Only Red Hat,
   Inc. may make changes or additions to the list of Approved Interfaces.
   Red Hat's grant of this exception is conditioned upon your not adding
   any new exceptions.  If you wish to add a new Approved Interface or
   exception, please contact Red Hat.  You must obey the GNU General Public
   License in all respects for all of the Red Hat elfutils code and other
   code used in conjunction with Red Hat elfutils except the Non-GPL Code
   covered by this exception.  If you modify this file, you may extend this
   exception to your version of the file, but you are not obligated to do
   so.  If you do not wish to provide this exception without modification,
   you must delete this exception statement from your version and license
   this file solely under the GPL without exception.

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
