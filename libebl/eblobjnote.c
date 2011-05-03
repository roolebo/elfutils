/* Print contents of object file note.
   Copyright (C) 2002, 2007, 2009, 2011 Red Hat, Inc.
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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <libeblP.h>


void
ebl_object_note (ebl, name, type, descsz, desc)
     Ebl *ebl;
     const char *name;
     uint32_t type;
     uint32_t descsz;
     const char *desc;
{
  if (! ebl->object_note (name, type, descsz, desc))
    /* The machine specific function did not know this type.  */

    if (strcmp ("stapsdt", name) == 0)
      {
	if (type != 3)
	  {
	    printf (gettext ("unknown SDT version %u\n"), type);
	    return;
	  }

	/* Descriptor starts with three addresses, pc, base ref and
	   semaphore.  Then three zero terminated strings provider,
	   name and arguments.  */

	union
	{
	  Elf64_Addr a64[3];
	  Elf32_Addr a32[3];
	} addrs;

	size_t addrs_size = gelf_fsize (ebl->elf, ELF_T_ADDR, 3, EV_CURRENT);
	if (descsz < addrs_size + 3)
	  {
	  invalid_sdt:
	    printf (gettext ("invalid SDT probe descriptor\n"));
	    return;
	  }

	Elf_Data src =
	  {
	    .d_type = ELF_T_ADDR, .d_version = EV_CURRENT,
	    .d_buf = (void *) desc, .d_size = addrs_size
	  };

	Elf_Data dst =
	  {
	    .d_type = ELF_T_ADDR, .d_version = EV_CURRENT,
	    .d_buf = &addrs, .d_size = addrs_size
	  };

	if (gelf_xlatetom (ebl->elf, &dst, &src,
			   elf_getident (ebl->elf, NULL)[EI_DATA]) == NULL)
	  {
	    printf ("%s\n", elf_errmsg (-1));
	    return;
	  }

	const char *provider = desc + addrs_size;
	const char *pname = memchr (provider, '\0', desc + descsz - provider);
	if (pname == NULL)
	  goto invalid_sdt;

	++pname;
	const char *args = memchr (pname, '\0', desc + descsz - pname);
	if (args == NULL ||
	    memchr (++args, '\0', desc + descsz - pname) != desc + descsz - 1)
	  goto invalid_sdt;

	GElf_Addr pc;
	GElf_Addr base;
	GElf_Addr sem;
	if (gelf_getclass (ebl->elf) == ELFCLASS32)
	  {
	    pc = addrs.a32[0];
	    base = addrs.a32[1];
	    sem = addrs.a32[2];
	  }
	else
	  {
	    pc = addrs.a64[0];
	    base = addrs.a64[1];
	    sem = addrs.a64[2];
	  }

	printf (gettext ("    PC: "));
	printf ("%#" PRIx64 ",", pc);
	printf (gettext (" Base: "));
	printf ("%#" PRIx64 ",", base);
	printf (gettext (" Semaphore: "));
	printf ("%#" PRIx64 "\n", sem);
	printf (gettext ("    Provider: "));
	printf ("%s,", provider);
	printf (gettext (" Name: "));
	printf ("%s,", pname);
	printf (gettext (" Args: "));
	printf ("'%s'\n", args);
	return;
      }

    switch (type)
      {
      case NT_GNU_BUILD_ID:
	if (strcmp (name, "GNU") == 0 && descsz > 0)
	  {
	    printf (gettext ("    Build ID: "));
	    uint_fast32_t i;
	    for (i = 0; i < descsz - 1; ++i)
	      printf ("%02" PRIx8, (uint8_t) desc[i]);
	    printf ("%02" PRIx8 "\n", (uint8_t) desc[i]);
	  }
	break;

      case NT_GNU_GOLD_VERSION:
	if (strcmp (name, "GNU") == 0 && descsz > 0)
	  /* A non-null terminated version string.  */
	  printf (gettext ("    Linker version: %.*s\n"),
		  (int) descsz, desc);
	break;

      case NT_GNU_ABI_TAG:
	if (strcmp (name, "GNU") == 0 && descsz >= 8 && descsz % 4 == 0)
	  {
	    Elf_Data in =
	      {
		.d_version = EV_CURRENT,
		.d_type = ELF_T_WORD,
		.d_size = descsz,
		.d_buf = (void *) desc
	      };
	    uint32_t buf[descsz / 4];
	    Elf_Data out =
	      {
		.d_version = EV_CURRENT,
		.d_type = ELF_T_WORD,
		.d_size = descsz,
		.d_buf = buf
	      };

	    if (elf32_xlatetom (&out, &in, ebl->data) != NULL)
	      {
		const char *os;
		switch (buf[0])
		  {
		  case ELF_NOTE_OS_LINUX:
		    os = "Linux";
		    break;

		  case ELF_NOTE_OS_GNU:
		    os = "GNU";
		    break;

		  case ELF_NOTE_OS_SOLARIS2:
		    os = "Solaris";
		    break;

		  case ELF_NOTE_OS_FREEBSD:
		    os = "FreeBSD";
		    break;

		  default:
		    os = "???";
		    break;
		  }

		printf (gettext ("    OS: %s, ABI: "), os);
		for (size_t cnt = 1; cnt < descsz / 4; ++cnt)
		  {
		    if (cnt > 1)
		      putchar_unlocked ('.');
		    printf ("%" PRIu32, buf[cnt]);
		  }
		putchar_unlocked ('\n');
	      }
	    break;
	  }
	/* FALLTHROUGH */

      default:
	/* Unknown type.  */
	break;
      }
}
