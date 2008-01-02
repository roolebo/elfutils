/* Copyright (C) 2005 Red Hat, Inc.

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

#include <string.h>

#include "libasmP.h"


struct buffer
{
  char *buf;
  size_t len;
};


static int
buffer_cb (char *str, size_t len, void *arg)
{
  struct buffer *buffer = (struct buffer *) arg;

  if (len > buffer->len)
    /* Return additional needed space.  */
    return len - buffer->len;

  buffer->buf = mempcpy (buffer->buf, str, len);
  buffer->len = len;

  return 0;
}


int
disasm_str (DisasmCtx_t *ctx, const uint8_t **startp, const uint8_t *end,
	    GElf_Addr addr, const char *fmt, char **bufp, size_t len,
	    void *symcbarg)
{
  struct buffer buffer = { .buf = *bufp, .len = len };

  int res = INTUSE(disasm_cb) (ctx, startp, end, addr, fmt, buffer_cb, &buffer,
			       symcbarg);
  *bufp = buffer.buf;
  return res;
}
