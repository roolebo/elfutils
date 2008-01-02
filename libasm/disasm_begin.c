/* Create context descriptor for disassembler.
   Copyright (C) 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#include <stdlib.h>

#include "libasmP.h"
#include "../libebl/libeblP.h"


DisasmCtx_t *
disasm_begin (Ebl *ebl, Elf *elf, DisasmGetSymCB_t symcb)
{
  if (ebl == NULL)
    return NULL;

  if (ebl->disasm == NULL)
    {
      __libasm_seterrno (ASM_E_ENOSUP);
      return NULL;
    }

  DisasmCtx_t *ctx = (DisasmCtx_t *) malloc (sizeof (DisasmCtx_t));
  if (ctx == NULL)
    {
      __libasm_seterrno (ASM_E_NOMEM);
      return NULL;
    }

  ctx->ebl = ebl;
  ctx->elf = elf;
  ctx->symcb = symcb;

  return ctx;
}
