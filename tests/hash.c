/* Copyright (C) 2002 Red Hat, Inc.
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

#include <libelf.h>
#include <stdio.h>


static int
check (const char *name, unsigned long int expected)
{
  unsigned long int actual = elf_hash (name);

  return actual != expected;
}


int
main (void)
{
  int status;

  /* Check some names.  We know what the expected result is.  */
  status = check ("_DYNAMIC", 165832675);
  status |= check ("_GLOBAL_OFFSET_TABLE_", 102264335);

  return status;
}
