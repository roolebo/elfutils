/* Copyright (C) 2001, 2002 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef SYMBOLHASH_H
#define SYMBOLHASH_H	1

/* Definitions for the symbol hash table.  */
#define TYPE struct symbol *
#define NAME ld_symbol_tab
#define ITERATE 1
#define COMPARE(a, b) strcmp ((a)->name, (b)->name)
#include <dynamicsizehash.h>

#endif	/* symbolhash.h */
