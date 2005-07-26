/* Memory handling for libdw.
   Copyright (C) 2003, 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2003.

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

#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/param.h>
#include "libdwP.h"


void *
__libdw_allocate (Dwarf *dbg, size_t minsize)
{
  size_t size = MAX (dbg->mem_default_size,
		     2 * minsize + offsetof (struct libdw_memblock, mem));
  struct libdw_memblock *newp = malloc (size);
  if (newp == NULL)
    dbg->oom_handler ();

  newp->size = newp->remaining = size - offsetof (struct libdw_memblock, mem);

  newp->prev = dbg->mem_tail;
  dbg->mem_tail = newp;

  return newp->mem;
}


Dwarf_OOM
dwarf_new_oom_handler (Dwarf *dbg, Dwarf_OOM handler)
{
  Dwarf_OOM old = dbg->oom_handler;
  dbg->oom_handler = handler;
  return old;
}


void
__attribute ((noreturn, visibility ("hidden")))
__libdw_oom (void)
{
  while (1)
    error (EXIT_FAILURE, ENOMEM, "libdw");
}
