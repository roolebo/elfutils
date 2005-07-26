/* Add string to a section.
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <libasmP.h>


/* Add zero terminated string STR of size LEN to (sub)section ASMSCN.  */
int
asm_addstrz (asmscn, str, len)
     AsmScn_t *asmscn;
     const char *str;
     size_t len;
{
  if (asmscn == NULL)
    return -1;

  if (unlikely (asmscn->type == SHT_NOBITS))
    {
      if (len == 0)
	{
	  if (str[0] != '\0')
	    {
	      __libasm_seterrno (ASM_E_TYPE);
	      return -1;
	    }
	}
      else
	{
	  size_t cnt;

	  for (cnt = 0; cnt < len; ++cnt)
	    if (str[cnt] != '\0')
	      {
		__libasm_seterrno (ASM_E_TYPE);
		return -1;
	      }
	}
    }

  if (len == 0)
    len = strlen (str) + 1;

  if (unlikely (asmscn->ctx->textp))
    {
      bool nextline = true;

      do
	{
	  if (nextline)
	    {
	      fputs ("\t.string\t\"", asmscn->ctx->out.file);
	      nextline = false;
	    }

	  if (*str == '\0')
	    fputs ("\\000", asmscn->ctx->out.file);
	  else if (! isascii (*str))
	    fprintf (asmscn->ctx->out.file, "\\%03o",
		     (unsigned int) *((unsigned char *)str));
	  else if (*str == '\\')
	    fputs ("\\\\", asmscn->ctx->out.file);
	  else if (*str == '\n')
	    {
	      fputs ("\\n\"", asmscn->ctx->out.file);
	      nextline = true;
	    }
	  else
	    fputc (*str, asmscn->ctx->out.file);

	  ++str;
	}
      while (--len > 0 && (len > 1 || *str != '\0'));

      if (! nextline)
	fputs ("\"\n", asmscn->ctx->out.file);
    }
  else
    {
      /* Make sure there is enough room.  */
      if (__libasm_ensure_section_space (asmscn, len) != 0)
	return -1;

      /* Copy the string.  */
      memcpy (&asmscn->content->data[asmscn->content->len], str, len);

      /* Adjust the pointer in the data buffer.  */
      asmscn->content->len += len;

      /* Increment the offset in the (sub)section.  */
      asmscn->offset += len;
    }

  return 0;
}
