/* Declarations for common convenience functions.
   Copyright (C) 2006-2011 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifndef LIB_SYSTEM_H
#define LIB_SYSTEM_H	1

#include <argp.h>
#include <stddef.h>
#include <stdint.h>
#include <endian.h>
#include <byteswap.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define LE32(n)	(n)
# define BE32(n)	bswap_32 (n)
#elif __BYTE_ORDER == __BIG_ENDIAN
# define BE32(n)	(n)
# define LE32(n)	bswap_32 (n)
#else
# error "Unknown byte order"
#endif

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


/* We need define two variables, argp_program_version_hook and
   argp_program_bug_address, in all programs.  argp.h declares these
   variables as non-const (which is correct in general).  But we can
   do better, it is not going to change.  So we want to move them into
   the .rodata section.  Define macros to do the trick.  */
#define ARGP_PROGRAM_VERSION_HOOK_DEF \
  void (*const apvh) (FILE *, struct argp_state *) \
   __asm ("argp_program_version_hook")
#define ARGP_PROGRAM_BUG_ADDRESS_DEF \
  const char *const apba__ __asm ("argp_program_bug_address")


/* The demangler from libstdc++.  */
extern char *__cxa_demangle (const char *mangled_name, char *output_buffer,
			     size_t *length, int *status);



/* Color handling.  */

/* Command line parser.  */
extern const struct argp color_argp;

/* Coloring mode.  */
enum color_enum
  {
    color_never = 0,
    color_always,
    color_auto
  } __attribute__ ((packed));
extern enum color_enum color_mode;

/* Colors to use for the various components.  */
extern char *color_address;
extern char *color_bytes;
extern char *color_mnemonic;
extern char *color_operand1;
extern char *color_operand2;
extern char *color_operand3;
extern char *color_label;
extern char *color_undef;
extern char *color_undef_tls;
extern char *color_undef_weak;
extern char *color_symbol;
extern char *color_tls;
extern char *color_weak;

extern const char color_off[];

#endif /* system.h */
