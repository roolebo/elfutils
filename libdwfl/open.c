/* Decompression support for libdwfl: zlib (gzip) and/or bzlib (bzip2).
   Copyright (C) 2009 Red Hat, Inc.
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

#include "../libelf/libelfP.h"
#undef	_
#include "libdwflP.h"

#include <unistd.h>

#if !USE_ZLIB
# define __libdw_gunzip(...)	false
#endif

#if !USE_BZLIB
# define __libdw_bunzip2(...)	false
#endif

/* Always consumes *ELF, never consumes FD.
   Replaces *ELF on success.  */
static Dwfl_Error
decompress (int fd __attribute__ ((unused)), Elf **elf)
{
  Dwfl_Error error = DWFL_E_BADELF;
  void *buffer = NULL;
  size_t size = 0;

#if USE_ZLIB || USE_BZLIB
  const off64_t offset = (*elf)->start_offset;
  void *const mapped = ((*elf)->map_address == NULL ? NULL
			: (*elf)->map_address + (*elf)->start_offset);
  const size_t mapped_size = (*elf)->maximum_size;

  error = __libdw_gunzip (fd, offset, mapped, mapped_size, &buffer, &size);
  if (error == DWFL_E_BADELF)
    error = __libdw_bunzip2 (fd, offset, mapped, mapped_size, &buffer, &size);
#endif

  elf_end (*elf);
  *elf = NULL;

  if (error == DWFL_E_NOERROR)
    {
      if (unlikely (size == 0))
	{
	  error = DWFL_E_BADELF;
	  free (buffer);
	}
      else
	{
	  *elf = elf_memory (buffer, size);
	  if (*elf == NULL)
	    {
	      error = DWFL_E_LIBELF;
	      free (buffer);
	    }
	  else
	    (*elf)->flags |= ELF_F_MALLOCED;
	}
    }

  return error;
}

Dwfl_Error internal_function
__libdw_open_file (int *fdp, Elf **elfp, bool close_on_fail, bool archive_ok)
{
  bool close_fd = false;
  Dwfl_Error error = DWFL_E_NOERROR;

  Elf *elf = elf_begin (*fdp, ELF_C_READ_MMAP_PRIVATE, NULL);
  Elf_Kind kind = elf_kind (elf);
  if (unlikely (kind == ELF_K_NONE))
    {
      if (unlikely (elf == NULL))
	error = DWFL_E_LIBELF;
      else
	{
	  error = decompress (*fdp, &elf);
	  if (error == DWFL_E_NOERROR)
	    {
	      close_fd = true;
	      kind = elf_kind (elf);
	    }
	}
    }

  if (error == DWFL_E_NOERROR
      && kind != ELF_K_ELF
      && !(archive_ok && kind == ELF_K_AR))
    {
      elf_end (elf);
      elf = NULL;
      error = DWFL_E_BADELF;
    }

  if (error == DWFL_E_NOERROR ? close_fd : close_on_fail)
    {
      close (*fdp);
      *fdp = -1;
    }

  *elfp = elf;
  return error;
}
