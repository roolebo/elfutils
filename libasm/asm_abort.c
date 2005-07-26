/* Abort operations on the assembler context, free all resources.
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

#include <stdlib.h>
#include <unistd.h>

#include <libasmP.h>
#include <libelf.h>


int
asm_abort (ctx)
     AsmCtx_t *ctx;
{
  if (ctx == NULL)
    /* Something went wrong earlier.  */
    return -1;

  if (likely (! ctx->textp))
    /* First free the ELF file.  We don't care about the result.  */
    (void) elf_end (ctx->out.elf);

  /* Now close the temporary file and remove it.  */
  (void) unlink (ctx->tmp_fname);

  /* Free the resources.  */
  __libasm_finictx (ctx);

  return 0;
}
