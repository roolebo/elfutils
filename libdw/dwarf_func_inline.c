#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "libdwP.h"
#include <dwarf.h>

struct visitor_info
{
  void *die_addr;
  int (*callback) (Dwarf_Die *, void *);
  void *arg;
};

static int
scope_visitor (unsigned int depth __attribute__ ((unused)),
	       Dwarf_Die *die, void *arg)
{
  struct visitor_info *const v = arg;

  if (INTUSE(dwarf_tag) (die) != DW_TAG_inlined_subroutine)
    return DWARF_CB_OK;

  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = INTUSE(dwarf_attr) (die, DW_AT_abstract_origin,
					      &attr_mem);
  if (attr == NULL)
    return DWARF_CB_OK;

  Dwarf_Die origin_mem;
  Dwarf_Die *origin = INTUSE(dwarf_formref_die) (attr, &origin_mem);
  if (origin == NULL)
    return DWARF_CB_ABORT;

  if (origin->addr != v->die_addr)
    return DWARF_CB_OK;

  return (*v->callback) (die, v->arg);
}

int
dwarf_func_inline (Dwarf_Func *func)
{
  Dwarf_Attribute attr_mem;
  Dwarf_Word val;
  if (INTUSE(dwarf_formudata) (INTUSE(dwarf_attr) (func->die, DW_AT_inline,
						   &attr_mem),
			       &val) == 0)
  switch (val)
    {
    case DW_INL_not_inlined:
      return 0;

    case DW_INL_declared_not_inlined:
      return -1;

    case DW_INL_inlined:
    case DW_INL_declared_inlined:
      return 1;
    }

  return 0;
}

int
dwarf_func_inline_instances (Dwarf_Func *func,
			     int (*callback) (Dwarf_Die *, void *),
			     void *arg)
{
  struct visitor_info v = { func->die->addr, callback, arg };
  return __libdw_visit_scopes (0, func->cudie, &scope_visitor, &v);
}
