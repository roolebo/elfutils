/* Copyright (C) 2002, 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include <fcntl.h>
#include <libasm.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


static const char fname[] = "asm-tst4-out.o";


int
main (void)
{
  AsmCtx_t *ctx;
  int result = 0;
  size_t cnt;

  elf_version (EV_CURRENT);

  ctx = asm_begin (fname, false, EM_386, ELFCLASS32, ELFDATA2LSB);
  if (ctx == NULL)
    {
      printf ("cannot create assembler context: %s\n", asm_errmsg (-1));
      return 1;
    }

  /* Create 66000 sections.  */
  for (cnt = 0; cnt < 66000; ++cnt)
    {
      char buf[20];
      AsmScn_t *scn;

      /* Create a unique name.  */
      snprintf (buf, sizeof (buf), ".data.%Zu", cnt);

      /* Create the section.  */
      scn = asm_newscn (ctx, buf, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
      if (scn == NULL)
	{
	  printf ("cannot create section \"%s\" in output file: %s\n",
		  buf, asm_errmsg (-1));
	  asm_abort (ctx);
	  return 1;
	}

      /* Add some content.  */
      if (asm_adduint32 (scn, cnt) != 0)
	{
	  printf ("cannot create content of section \"%s\": %s\n",
		  buf, asm_errmsg (-1));
	  asm_abort (ctx);
	  return 1;
	}
    }

  /* Create the output file.  */
  if (asm_end (ctx) != 0)
    {
      printf ("cannot create output file: %s\n", asm_errmsg (-1));
      asm_abort (ctx);
      return 1;
    }

  if (result == 0)
    result = WEXITSTATUS (system ("\
env LD_LIBRARY_PATH=../libelf ../src/elflint -q asm-tst4-out.o"));

  /* We don't need the file anymore.  */
  unlink (fname);

  return result;
}
