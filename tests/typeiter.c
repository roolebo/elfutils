/* Copyright (C) 2012 Red Hat, Inc.
   This file is part of Red Hat elfutils.

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

#include <fcntl.h>
#include ELFUTILS_HEADER(dw)
#include <stdio.h>
#include <unistd.h>
#include <dwarf.h>

int
main (int argc, char *argv[])
{
  for (int i = 1; i < argc; ++i)
    {
      int fd = open (argv[i], O_RDONLY);

      Dwarf *dbg = dwarf_begin (fd, DWARF_C_READ);
      if (dbg != NULL)
	{
	  Dwarf_Off off = 0;
	  size_t cuhl;
	  Dwarf_Off noff;

	  while (dwarf_nextcu (dbg, off, &noff, &cuhl, NULL, NULL, NULL) == 0)
	    {
	      Dwarf_Die die_mem;
	      Dwarf_Die *die = dwarf_offdie (dbg, off + cuhl, &die_mem);

	      Dwarf_Die iter_mem;
	      Dwarf_Die *iter = &iter_mem;
	      dwarf_child (die, &iter_mem);

	      while (1)
		{
		  if (dwarf_tag (iter) == DW_TAG_variable)
		    {
		      Dwarf_Attribute attr_mem;
		      Dwarf_Die form_mem;
		      dwarf_formref_die (dwarf_attr (iter, DW_AT_type,
						     &attr_mem),
					 &form_mem);
		    }

		  if (dwarf_siblingof (iter, &iter_mem) != 0)
		    break;
		}

	      off = noff;
	    }

	  off = 0;
	  uint64_t type_sig;

	  while (dwarf_next_unit (dbg, off, &noff, &cuhl, NULL, NULL, NULL,
				  NULL, &type_sig, NULL) == 0)
	    {
	      Dwarf_Die die_mem;
	      Dwarf_Die *die = dwarf_offdie_types (dbg, off + cuhl, &die_mem);

	      if (die == NULL)
		printf ("fail\n");
	      else
		printf ("ok\n");

	      off = noff;
	    }

	  dwarf_end (dbg);
	}

      close (fd);
    }
}
