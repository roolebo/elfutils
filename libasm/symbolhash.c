/* Symbol hash table implementation.
   Copyright (C) 2001, 2002 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

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

#include <string.h>

#include <libasmP.h>
#include <libebl.h>

/* Definitions for the symbol hash table.  */
#define TYPE AsmSym_t *
#define NAME asm_symbol_tab
#define ITERATE 1
#define REVERSE 1
#define COMPARE(a, b) \
  strcmp (ebl_string ((a)->strent), ebl_string ((b)->strent))

#define next_prime __libasm_next_prime
extern size_t next_prime (size_t) attribute_hidden;

#include "../lib/dynamicsizehash.c"

#undef next_prime
#define next_prime attribute_hidden __libasm_next_prime
#include "../lib/next_prime.c"
