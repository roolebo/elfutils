/* Retrieve ELF descriptor used for DWARF access.
   Copyright (C) 2002, 2003, 2004, 2005 Red Hat, Inc.
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
#include <stddef.h>

#include "libdwP.h"


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
dwarf_errno (void)
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

      setspecific (key, (void *) (intptr_t) DWARF_E_NOERROR);
      return result;
    }
#endif	/* TLS */

  result = global_error;
  global_error = DWARF_E_NOERROR;
  return result;
}
INTDEF(dwarf_errno)


/* XXX For now we use string pointers.  Once the table stablelizes
   make it more DSO-friendly.  */
static const char *errmsgs[] =
  {
    [DWARF_E_NOERROR] = N_("no error"),
    [DWARF_E_UNKNOWN_ERROR] = N_("unknown error"),
    [DWARF_E_INVALID_ACCESS] = N_("invalid access"),
    [DWARF_E_NO_REGFILE] = N_("no regular file"),
    [DWARF_E_IO_ERROR] = N_("I/O error"),
    [DWARF_E_INVALID_ELF] = N_("invalid ELF file"),
    [DWARF_E_NO_DWARF] = N_("no DWARF information"),
    [DWARF_E_NOELF] = N_("no ELF file"),
    [DWARF_E_GETEHDR_ERROR] = N_("cannot get ELF header"),
    [DWARF_E_NOMEM] = N_("out of memory"),
    [DWARF_E_UNIMPL] = N_("not implemented"),
    [DWARF_E_INVALID_CMD] = N_("invalid command"),
    [DWARF_E_INVALID_VERSION] = N_("invalid version"),
    [DWARF_E_INVALID_FILE] = N_("invalid file"),
    [DWARF_E_NO_ENTRY] = N_("no entries found"),
    [DWARF_E_INVALID_DWARF] = N_("invalid DWARF"),
    [DWARF_E_NO_STRING] = N_("no string data"),
    [DWARF_E_NO_ADDR] = N_("no address value"),
    [DWARF_E_NO_CONSTANT] = N_("no constant value"),
    [DWARF_E_NO_REFERENCE] = N_("no reference value"),
    [DWARF_E_INVALID_REFERENCE] = N_("invalid reference value"),
    [DWARF_E_NO_DEBUG_LINE] = N_(".debug_line section missing"),
    [DWARF_E_INVALID_DEBUG_LINE] = N_("invalid .debug_line section"),
    [DWARF_E_TOO_BIG] = N_("debug information too big"),
    [DWARF_E_VERSION] = N_("invalid DWARF version"),
    [DWARF_E_INVALID_DIR_IDX] = N_("invalid directory index"),
    [DWARF_E_ADDR_OUTOFRANGE] = N_("address out of range"),
    [DWARF_E_NO_LOCLIST] = N_("no location list value"),
    [DWARF_E_NO_BLOCK] = N_("no block data"),
    [DWARF_E_INVALID_LINE_IDX] = N_("invalid line index"),
    [DWARF_E_INVALID_ARANGE_IDX] = N_("invalid address range index"),
    [DWARF_E_NO_MATCH] = N_("no matching address range"),
    [DWARF_E_NO_FLAG] = N_("no flag value"),
    [DWARF_E_INVALID_OFFSET] = N_("invalid offset"),
    [DWARF_E_NO_DEBUG_RANGES] = N_(".debug_ranges section missing"),
  };
#define nerrmsgs (sizeof (errmsgs) / sizeof (errmsgs[0]))


void
__libdw_seterrno (value)
     int value;
{
#ifndef USE_TLS
  /* If we have not yet initialized the buffer do it now.  */
  once_execute (once, init);

  if (threaded)
    /* We do not allocate memory for the data.  It is only a word.
       We can store it in place of the pointer.  */
    setspecific (key, (void *) (intptr_t) value);
#endif	/* TLS */

  global_error = (value >= 0 && value < (int) nerrmsgs
		  ? value : DWARF_E_UNKNOWN_ERROR);
}


const char *
dwarf_errmsg (error)
     int error;
{
  int last_error;

#ifndef USE_TLS
  /* If we have not yet initialized the buffer do it now.  */
  once_execute (once, init);

  if ((error == 0 || error == -1) && threaded)
    /* We do not allocate memory for the data.  It is only a word.
       We can store it in place of the pointer.  */
    last_error = (intptr_t) getspecific (key);
  else
#endif	/* TLS */
    last_error = global_error;

  if (error == 0)
    return last_error != 0 ? _(errmsgs[last_error]) : NULL;
  else if (error < -1 || error >= (int) nerrmsgs)
    return _(errmsgs[DWARF_E_UNKNOWN_ERROR]);

  return _(errmsgs[error == -1 ? last_error : error]);
}


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
