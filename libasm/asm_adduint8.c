/* Add unsigned integer to a section.
   Copyright (C) 2002 Red Hat, Inc.
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

#include <libasmP.h>

#ifndef SIZE
# define SIZE 8
#endif

#define UFCT(size) _UFCT(size)
#define _UFCT(size) asm_adduint##size
#define FCT(size) _FCT(size)
#define _FCT(size) asm_addint##size
#define UTYPE(size) _UTYPE(size)
#define _UTYPE(size) uint##size##_t
#define TYPE(size) _TYPE(size)
#define _TYPE(size) int##size##_t


int
UFCT(SIZE) (asmscn, num)
     AsmScn_t *asmscn;
     UTYPE(SIZE) num;
{
  return INTUSE(FCT(SIZE)) (asmscn, (TYPE(SIZE)) num);
}
