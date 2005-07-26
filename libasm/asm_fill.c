/* Determine fill pattern for a section.
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
#include <string.h>

#include <libasmP.h>
#include <system.h>


int
asm_fill (asmscn, bytes, len)
     AsmScn_t *asmscn;
     void *bytes;
     size_t len;
{
  struct FillPattern *pattern;
  struct FillPattern *old_pattern;

  if (asmscn == NULL)
    /* Some earlier error.  */
    return -1;

  if (bytes == NULL)
    /* Use the default pattern.  */
    pattern = (struct FillPattern *) __libasm_default_pattern;
  else
    {
      /* Allocate appropriate memory.  */
      pattern = (struct FillPattern *) malloc (sizeof (struct FillPattern)
					       + len);
      if (pattern == NULL)
	return -1;

      pattern->len = len;
      memcpy (pattern->bytes, bytes, len);
    }

  old_pattern = asmscn->pattern;
  asmscn->pattern = pattern;

  /* Free the old data structure if we have allocated it.  */
  if (old_pattern != __libasm_default_pattern)
    free (old_pattern);

  return 0;
}
