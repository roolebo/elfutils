/* Linux kernel image support for libdwfl.
   Copyright (C) 2009-2011 Red Hat, Inc.
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

#include "libdwflP.h"
#include "system.h"

#include <unistd.h>
#include <endian.h>

#if BYTE_ORDER == LITTLE_ENDIAN
# define LE16(x)	(x)
#else
# define LE16(x)	bswap_16 (x)
#endif

/* See Documentation/x86/boot.txt in Linux kernel sources
   for an explanation of these format details.  */

#define MAGIC1			0xaa55
#define MAGIC2			0x53726448 /* "HdrS" little-endian */
#define MIN_VERSION		0x0208

#define H_START			(H_SETUP_SECTS & -4)
#define H_SETUP_SECTS		0x1f1
#define H_MAGIC1		0x1fe
#define H_MAGIC2		0x202
#define H_VERSION		0x206
#define H_PAYLOAD_OFFSET	0x248
#define H_PAYLOAD_LENGTH	0x24c
#define H_END			0x250
#define H_READ_SIZE		(H_END - H_START)

Dwfl_Error
internal_function
__libdw_image_header (int fd, off64_t *start_offset,
		      void *mapped, size_t mapped_size)
{
  if (likely (mapped_size > H_END))
    {
      const void *header = mapped;
      char header_buffer[H_READ_SIZE];
      if (header == NULL)
	{
	  ssize_t n = pread_retry (fd, header_buffer, H_READ_SIZE,
				   *start_offset + H_START);
	  if (n < 0)
	    return DWFL_E_ERRNO;
	  if (n < H_READ_SIZE)
	    return DWFL_E_BADELF;

	  header = header_buffer - H_START;
	}

      if (*(uint16_t *) (header + H_MAGIC1) == LE16 (MAGIC1)
	  && *(uint32_t *) (header + H_MAGIC2) == LE32 (MAGIC2)
	  && LE16 (*(uint16_t *) (header + H_VERSION)) >= MIN_VERSION)
	{
	  /* The magic numbers match and the version field is sufficient.
	     Extract the payload bounds.  */

	  uint32_t offset = LE32 (*(uint32_t *) (header + H_PAYLOAD_OFFSET));
	  uint32_t length = LE32 (*(uint32_t *) (header + H_PAYLOAD_LENGTH));

	  offset += ((*(uint8_t *) (header + H_SETUP_SECTS) ?: 4) + 1) * 512;

	  if (offset > H_END && offset < mapped_size
	      && mapped_size - offset >= length)
	    {
	      /* It looks kosher.  Use it!  */
	      *start_offset += offset;
	      return DWFL_E_NOERROR;
	    }
	}
    }
  return DWFL_E_BADELF;
}
