/* Error handling in libdwfl.
   Copyright (C) 2005 Red Hat, Inc.

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
#include <libintl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "libdwflP.h"


#ifdef USE_TLS
/* The error number.  */
static __thread int global_error;
#else
/* This is the key for the thread specific memory.  */
static tls_key_t key;

/* The error number.  Used in non-threaded programs.  */
static int global_error;
static bool threaded;
/* We need to initialize the thread-specific data.  */
once_define (static, once);

/* The initialization and destruction functions.  */
static void init (void);
static void free_key_mem (void *mem);
#endif	/* TLS */


int
dwfl_errno (void)
{
  int result;

#ifndef USE_TLS
  /* If we have not yet initialized the buffer do it now.  */
  once_execute (once, init);

  if (threaded)
    {
      /* We do not allocate memory for the data.  It is only a word.
	 We can store it in place of the pointer.  */
      result = (intptr_t) getspecific (key);

      setspecific (key, (void *) (intptr_t) DWFL_E_NOERROR);
      return result;
    }
#endif	/* TLS */

  result = global_error;
  global_error = DWFL_E_NOERROR;
  return result;
}


static const struct msgtable
{
#define DWFL_ERROR(name, text) char msg_##name[sizeof text];
  DWFL_ERRORS
#undef	DWFL_ERROR
} msgtable =
  {
#define DWFL_ERROR(name, text) text,
  DWFL_ERRORS
#undef	DWFL_ERROR
  };
#define msgstr (&msgtable.msg_NOERROR[0])

static const uint_fast16_t msgidx[] =
{
#define DWFL_ERROR(name, text) \
  [DWFL_E_##name] = offsetof (struct msgtable, msg_##name),
  DWFL_ERRORS
#undef	DWFL_ERROR
};
#define nmsgidx (sizeof msgidx / sizeof msgidx[0])


static inline int
canonicalize (Dwfl_Error error)
{
  unsigned int value;

  switch (error)
    {
    default:
      value = error;
      if ((value &~ 0xffff) != 0)
	break;
      assert (value < nmsgidx);
      break;
    case DWFL_E_ERRNO:
      value = DWFL_E (ERRNO, errno);
      break;
    case DWFL_E_LIBELF:
      value = DWFL_E (LIBELF, elf_errno ());
      break;
    case DWFL_E_LIBDW:
      value = DWFL_E (LIBDW, INTUSE(dwarf_errno) ());
      break;
#if 0
    DWFL_E_LIBEBL:
      value = DWFL_E (LIBEBL, ebl_errno ());
      break;
#endif
    }

  return value;
}

int
internal_function_def
__libdwfl_canon_error (Dwfl_Error error)
{
  return canonicalize (error);
}

void
internal_function_def
__libdwfl_seterrno (Dwfl_Error error)
{
  int value = canonicalize (error);

#ifndef USE_TLS
  /* If we have not yet initialized the buffer do it now.  */
  once_execute (once, init);

  if (threaded)
    /* We do not allocate memory for the data.  It is only a word.
       We can store it in place of the pointer.  */
    setspecific (key, (void *) (intptr_t) value);
#endif	/* TLS */

  global_error = value;
}


const char *
dwfl_errmsg (error)
     int error;
{
  if (error == 0 || error == -1)
    {
      int last_error;

#ifndef USE_TLS
      /* If we have not yet initialized the buffer do it now.  */
      once_execute (once, init);

      if (threaded)
	/* We do not allocate memory for the data.  It is only a word.
	   We can store it in place of the pointer.  */
	last_error = (intptr_t) getspecific (key);
      else
#endif	/* TLS */
	last_error = global_error;

      if (error == 0 && last_error == 0)
	return NULL;

      error = last_error;
      global_error = DWFL_E_NOERROR;
    }

  switch (error &~ 0xffff)
    {
    case OTHER_ERROR (ERRNO):
      return strerror_r (error & 0xffff, "bad", 0);
    case OTHER_ERROR (LIBELF):
      return elf_errmsg (error & 0xffff);
    case OTHER_ERROR (LIBDW):
      return INTUSE(dwarf_errmsg) (error & 0xffff);
#if 0
    case OTHER_ERROR (LIBEBL):
      return ebl_errmsg (error & 0xffff);
#endif
    }

  return _(&msgstr[msgidx[(unsigned int) error < nmsgidx
			  ? error : DWFL_E_UNKNOWN_ERROR]]);
}
INTDEF (dwfl_errmsg)


#ifndef USE_TLS
/* Free the thread specific data, this is done if a thread terminates.  */
static void
free_key_mem (void *mem __attribute__ ((unused)))
{
  setspecific (key, NULL);
}


/* Initialize the key for the global variable.  */
static void
init (void)
{
  // XXX Screw you, gcc4, the unused function attribute does not work.
  __asm ("" :: "r" (free_key_mem));

  if (key_create (&key, free_key_mem) == 0)
    /* Creating the key succeeded.  */
    threaded = true;
}
#endif	/* TLS */
