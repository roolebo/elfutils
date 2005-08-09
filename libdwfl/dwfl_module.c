/* Maintenance of module list in libdwfl.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include "libdwflP.h"
#include <search.h>

static void
free_cu (struct dwfl_cu *cu)
{
  if (cu->lines != NULL)
    free (cu->lines);
  free (cu);
}

static void
nofree (void *arg __attribute__ ((unused)))
{
}

void
internal_function_def
__libdwfl_module_free (Dwfl_Module *mod)
{
  if (mod->lazy_cu_root != NULL)
    tdestroy (mod->lazy_cu_root, nofree);

  if (mod->aranges != NULL)
    free (mod->aranges);

  if (mod->cu != NULL)
    {
      for (size_t i = 0; i < mod->ncu; ++i)
	free_cu (mod->cu[i]);
      free (mod->cu);
    }

  if (mod->dw != NULL)
    INTUSE(dwarf_end) (mod->dw);

  if (mod->ebl != NULL)
    ebl_closebackend (mod->ebl);

  if (mod->debug.elf != mod->main.elf && mod->debug.elf != NULL)
    elf_end (mod->debug.elf);
  if (mod->main.elf != NULL)
    elf_end (mod->main.elf);

  free (mod->name);
}

void
dwfl_report_begin (Dwfl *dwfl)
{
  for (Dwfl_Module *m = dwfl->modulelist; m != NULL; m = m->next)
    m->gc = true;

  if (dwfl->modules != NULL)
    free (dwfl->modules);
  dwfl->modules = NULL;
  dwfl->nmodules = 0;
}
INTDEF (dwfl_report_begin)

/* Report that a module called NAME pans addresses [START, END).
   Returns the module handle, either existing or newly allocated,
   or returns a null pointer for an allocation error.  */
Dwfl_Module *
dwfl_report_module (Dwfl *dwfl, const char *name,
		    GElf_Addr start, GElf_Addr end)
{
  Dwfl_Module **tailp = &dwfl->modulelist, **prevp = tailp;
  for (Dwfl_Module *m = *prevp; m != NULL; m = *(prevp = &m->next))
    {
      if (m->low_addr == start && m->high_addr == end
	  && !strcmp (m->name, name))
	{
	  /* This module is still here.  Move it to the place in the list
	     after the last module already reported.  */

	  *prevp = m->next;
	  m->next = *tailp;
	  m->gc = false;
	  *tailp = m;
	  return m;
	}

      if (! m->gc)
	tailp = &m->next;
    }

  Dwfl_Module *mod = calloc (1, sizeof *mod);
  if (mod == NULL)
    goto nomem;

  mod->name = strdup (name);
  if (mod->name == NULL)
    {
      free (mod);
    nomem:
      __libdwfl_seterrno (DWFL_E_NOMEM);
      return NULL;
    }

  mod->low_addr = start;
  mod->high_addr = end;
  mod->dwfl = dwfl;

  mod->next = *tailp;
  *tailp = mod;
  ++dwfl->nmodules;

  return mod;
}
INTDEF (dwfl_report_module)

static int
compare_modules (const void *a, const void *b)
{
  Dwfl_Module *const *p1 = a, *const *p2 = b;
  const Dwfl_Module *m1 = *p1, *m2 = *p2;
  if (m1 == NULL)
    return -1;
  if (m2 == NULL)
    return 1;
  return (GElf_Sxword) (m1->low_addr - m2->low_addr);
}


/* Finish reporting the current set of modules to the library.
   If REMOVED is not null, it's called for each module that
   existed before but was not included in the current report.
   Returns a nonzero return value from the callback.
   DWFL cannot be used until this function has returned zero.  */
int dwfl_report_end (Dwfl *dwfl,
		     int (*removed) (Dwfl_Module *, void *,
				     const char *, Dwarf_Addr,
				     void *arg),
		     void *arg)
{
  assert (dwfl->modules == NULL);

  Dwfl_Module **tailp = &dwfl->modulelist;
  while (*tailp != NULL)
    {
      Dwfl_Module *m = *tailp;
      if (m->gc && removed != NULL)
	{
	  int result = (*removed) (MODCB_ARGS (m), arg);
	  if (result != 0)
	    return result;
	}
      if (m->gc)
	{
	  *tailp = m->next;
	  __libdwfl_module_free (m);
	}
      else
	tailp = &m->next;
    }

  dwfl->modules = malloc (dwfl->nmodules * sizeof dwfl->modules[0]);
  if (dwfl->modules == NULL && dwfl->nmodules != 0)
    return -1;

  size_t i = 0;
  for (Dwfl_Module *m = dwfl->modulelist; m != NULL; m = m->next)
    {
      assert (! m->gc);
      dwfl->modules[i++] = m;
    }
  assert (i == dwfl->nmodules);

  qsort (dwfl->modules, dwfl->nmodules, sizeof dwfl->modules[0],
	 &compare_modules);

  return 0;
}
INTDEF (dwfl_report_end)
