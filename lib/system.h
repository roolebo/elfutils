/* Copyright (C) 2000, 2002, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef LIB_SYSTEM_H
#define LIB_SYSTEM_H	1

#include <stddef.h>
#include <stdint.h>

extern void *xmalloc (size_t) __attribute__ ((__malloc__));
extern void *xcalloc (size_t, size_t) __attribute__ ((__malloc__));
extern void *xrealloc (void *, size_t) __attribute__ ((__malloc__));

extern char *xstrdup (const char *) __attribute__ ((__malloc__));
extern char *xstrndup (const char *, size_t) __attribute__ ((__malloc__));


extern uint32_t crc32 (uint32_t crc, unsigned char *buf, size_t len);
extern int crc32_file (int fd, uint32_t *resp);

/* A special gettext function we use if the strings are too short.  */
#define sgettext(Str) \
  ({ const char *__res = strrchr (gettext (Str), '|');			      \
     __res ? __res + 1 : Str; })

#define gettext_noop(Str) Str


#define pwrite_retry(fd, buf,  len, off) \
  TEMP_FAILURE_RETRY (pwrite (fd, buf, len, off))
#define write_retry(fd, buf, n) \
     TEMP_FAILURE_RETRY (write (fd, buf, n))
#define pread_retry(fd, buf,  len, off) \
  TEMP_FAILURE_RETRY (pread (fd, buf, len, off))

#endif /* system.h */
