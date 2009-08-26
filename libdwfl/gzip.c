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

#include "libdwflP.h"

#include <unistd.h>

#ifdef BZLIB
# define inflate_groks_header	true
# include <bzlib.h>
# define unzip		__libdw_bunzip2
# define DWFL_E_ZLIB	DWFL_E_BZLIB
# define MAGIC		"BZh"
# define Z(what)	BZ_##what
# define z_stream	bz_stream
# define inflateInit(z)	BZ2_bzDecompressInit (z, 0, 0)
# define inflate(z, f)	BZ2_bzDecompress (z)
# define inflateEnd(z)	BZ2_bzDecompressEnd (z)
# define gzFile		BZFILE *
# define gzdopen	BZ2_bzdopen
# define gzread		BZ2_bzread
# define gzclose	BZ2_bzclose
# define gzerror	BZ2_bzerror
#else
# define inflate_groks_header	false
# include <zlib.h>
# define unzip		__libdw_gunzip
# define MAGIC		"\037\213"
# define Z(what)	Z_##what
#endif

/* We can also handle Linux kernel zImage format in a very hackish way.
   If it looks like one, we actually just scan the image for the right
   magic bytes to figure out where the compressed image starts.  */

#define LINUX_MAGIC_OFFSET	514
#define LINUX_MAGIC		"HdrS"
#define LINUX_MAX_SCAN		32768

static bool
mapped_zImage (off64_t *start_offset, void **mapped, size_t *mapped_size)
{
  const size_t pos = LINUX_MAGIC_OFFSET + sizeof LINUX_MAGIC;
  if (*mapped_size > pos
      && !memcmp (*mapped + LINUX_MAGIC_OFFSET,
		  LINUX_MAGIC, sizeof LINUX_MAGIC - 1))
    {
      size_t scan = *mapped_size - pos;
      if (scan > LINUX_MAX_SCAN)
	scan = LINUX_MAX_SCAN;
      void *p = memmem (*mapped + pos, scan, MAGIC, sizeof MAGIC - 1);
      if (p != NULL)
	{
	  *start_offset += p - *mapped;
	  *mapped_size = *mapped + *mapped_size - p,
	  *mapped = p;
	  return true;
	}
    }
  return false;
}

/* If this is not a compressed image, return DWFL_E_BADELF.
   If we uncompressed it into *WHOLE, *WHOLE_SIZE, return DWFL_E_NOERROR.
   Otherwise return an error for bad compressed data or I/O failure.  */

Dwfl_Error internal_function
unzip (int fd, off64_t start_offset,
       void *mapped, size_t mapped_size,
       void **whole, size_t *whole_size)
{
  void *buffer = NULL;
  size_t size = 0;
  inline bool bigger_buffer (size_t start)
  {
    size_t more = size ? size * 2 : start;
    char *b = realloc (buffer, more);
    while (unlikely (b == NULL) && more >= size + 1024)
      b = realloc (buffer, more -= 1024);
    if (unlikely (b == NULL))
      return false;
    buffer = b;
    size = more;
    return true;
  }
  inline void smaller_buffer (size_t end)
  {
    buffer = realloc (buffer, end) ?: buffer;
    size = end;
  }

  inline Dwfl_Error zlib_fail (int result)
  {
    Dwfl_Error failure = DWFL_E_ZLIB;
    switch (result)
      {
      case Z (MEM_ERROR):
	failure = DWFL_E_NOMEM;
	break;
      }
    free (buffer);
    *whole = NULL;
    return failure;
  }

  /* If the file is already mapped in, look at the header.  */
  if (mapped != NULL
      && (mapped_size <= sizeof MAGIC
	  || memcmp (mapped, MAGIC, sizeof MAGIC - 1))
      && !mapped_zImage (&start_offset, &mapped, &mapped_size))
    /* Not a compressed file.  */
    return DWFL_E_BADELF;

  if (mapped != NULL && inflate_groks_header)
    {
      /* This style actually only works with bzlib.
	 The stupid zlib interface has nothing to grok the
	 gzip file headers except the slow gzFile interface.  */

      z_stream z = { .next_in = mapped, .avail_in = mapped_size };
      int result = inflateInit (&z);
      if (result != Z (OK))
	return zlib_fail (result);

      do
	{
	  ptrdiff_t pos = (void *) z.next_out - buffer;
	  if (!bigger_buffer (z.avail_in))
	    {
	      result = Z (MEM_ERROR);
	      break;
	    }
	  z.next_out = buffer + pos;
	  z.avail_out = size - pos;
	}
      while ((result = inflate (&z, Z_SYNC_FLUSH)) == Z (OK));

#ifdef BZLIB
      uint64_t total_out = (((uint64_t) z.total_out_hi32 << 32)
			    | z.total_out_lo32);
      smaller_buffer (total_out);
#else
      smaller_buffer (z.total_out);
#endif

      inflateEnd (&z);

      if (result != Z (STREAM_END))
	return zlib_fail (result);
    }
  else
    {
      /* Let the decompression library read the file directly.  */

      gzFile zf;
      Dwfl_Error open_stream (void)
      {
	int d = dup (fd);
	if (unlikely (d < 0))
	  return DWFL_E_BADELF;
	if (start_offset != 0)
	  {
	    off64_t off = lseek (d, start_offset, SEEK_SET);
	    if (off != start_offset)
	      {
		close (d);
		return DWFL_E_BADELF;
	      }
	  }
	zf = gzdopen (d, "r");
	if (unlikely (zf == NULL))
	  {
	    close (d);
	    return zlib_fail (Z (MEM_ERROR));
	  }

	/* From here on, zlib will close D.  */

	return DWFL_E_NOERROR;
      }

      Dwfl_Error result = open_stream ();

#ifndef BZLIB
      if (result == DWFL_E_NOERROR && gzdirect (zf))
	{
	  bool found = false;
	  char buf[sizeof LINUX_MAGIC - 1];
	  gzseek (zf, start_offset + LINUX_MAGIC_OFFSET, SEEK_SET);
	  int n = gzread (zf, buf, sizeof buf);
	  if (n == sizeof buf
	      && !memcmp (buf, LINUX_MAGIC, sizeof LINUX_MAGIC - 1))
	    while (gzread (zf, buf, sizeof MAGIC - 1) == sizeof MAGIC - 1)
	      if (!memcmp (buf, MAGIC, sizeof MAGIC - 1))
		{
		  start_offset = gztell (zf) - 2;
		  found = true;
		  break;
		}
	  gzclose (zf);
	  if (found)
	    {
	      result = open_stream ();
	      if (result == DWFL_E_NOERROR && unlikely (gzdirect (zf)))
		{
		  gzclose (zf);
		  result = DWFL_E_BADELF;
		}
	    }
	  else
	    result = DWFL_E_BADELF;
	}
#endif

      if (result != DWFL_E_NOERROR)
	return result;

      ptrdiff_t pos = 0;
      while (1)
	{
	  if (!bigger_buffer (1024))
	    {
	      gzclose (zf);
	      return zlib_fail (Z (MEM_ERROR));
	    }
	  int n = gzread (zf, buffer + pos, size - pos);
	  if (n < 0)
	    {
	      int code;
	      gzerror (zf, &code);
#ifdef BZLIB
	      if (code == BZ_DATA_ERROR_MAGIC)
		{
		  gzclose (zf);
		  free (buffer);
		  return DWFL_E_BADELF;
		}
#endif
	      gzclose (zf);
	      return zlib_fail (code);
	    }
	  if (n == 0)
	    break;
	  pos += n;
	}

      gzclose (zf);
      smaller_buffer (pos);
    }

  *whole = buffer;
  *whole_size = size;

  return DWFL_E_NOERROR;
}
