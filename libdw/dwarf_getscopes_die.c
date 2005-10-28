/* Return scope DIEs containing given DIE.
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
#include "libdwP.h"
#include <assert.h>
#include <stdlib.h>

static int
scope_visitor (unsigned int depth, struct Dwarf_Die_Chain *die, void *arg)
{
  if (die->die.addr != *(void **) arg)
    return 0;

  Dwarf_Die *scopes = malloc (depth * sizeof scopes[0]);
  if (scopes == NULL)
    {
      __libdw_seterrno (DWARF_E_NOMEM);
      return -1;
    }

  unsigned int i = 0;
  do
    {
      scopes[i++] = die->die;
      die = die->parent;
    }
  while (die != NULL);
  assert (i == depth);

  *(void **) arg = scopes;
  return depth;
}

int
dwarf_getscopes_die (Dwarf_Die *die, Dwarf_Die **scopes)
{
  if (die == NULL)
    return -1;

  struct Dwarf_Die_Chain cu = { .die = CUDIE (die->cu), .parent = NULL };
  void *info = die->addr;
  int result = __libdw_visit_scopes (1, &cu, &scope_visitor, NULL, &info);
  if (result > 0)
    *scopes = info;
  return result;
}
