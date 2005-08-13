/* Check special symbol's st_value.
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

#include <inttypes.h>
#include <libeblP.h>


bool
ebl_check_special_symbol (ebl, ehdr, sym, name, destshdr)
     Ebl *ebl;
     GElf_Ehdr *ehdr;
     const GElf_Sym *sym;
     const char *name;
     const GElf_Shdr *destshdr;
{
  if (ebl == NULL)
    return false;

  return ebl->check_special_symbol (ebl->elf, ehdr, sym, name, destshdr);
}
