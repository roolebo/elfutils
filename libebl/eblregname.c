/* Return register name information.
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


ssize_t
ebl_register_name (ebl, regno, name, namelen, prefix, setname)
     Ebl *ebl;
     int regno;
     char *name;
     size_t namelen;
     const char **prefix;
     const char **setname;
{
  return ebl == NULL ? -1 : ebl->register_name (ebl, regno, name, namelen,
						prefix, setname);
}
