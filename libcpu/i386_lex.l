%{
/* Copyright (C) 2004, 2005, 2007 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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
#include <error.h>
#include <libintl.h>

#include <system.h>
#include "i386_parse.h"


static void eat_to_eol (void);
static void invalid_char (int ch);
%}

ID              [a-zA-Z_][a-zA-Z0-9_/]*
ID2             [a-zA-Z0-9_:/]*
NUMBER		[0-9]+
WHITE		[[:space:]]+

%option yylineno
%option never-interactive
%option noyywrap


%x MAIN

%%

"%mask"				{ return kMASK; }

"%prefix"			{ return kPREFIX; }
"%suffix"			{ return kSUFFIX; }

"%synonym"			{ return kSYNONYM; }

{NUMBER}			{ i386_lval.num = strtoul (yytext, NULL, 10);
				  return kNUMBER; }

"%%"				{ BEGIN (MAIN); return kPERCPERC; }


<MAIN>"0"			{ return '0'; }
<MAIN>"1"			{ return '1'; }

<INITIAL,MAIN>"{"{ID2}"}"	{ i386_lval.str = xstrndup (yytext + 1,
							    yyleng - 2);
				  return kBITFIELD; }

<MAIN>"INVALID"			{ i386_lval.str = (void *) -1l;
				  return kID; }

<MAIN>{ID}			{ i386_lval.str = xstrndup (yytext, yyleng);
				  return kID; }

<MAIN>","			{ return ','; }

<MAIN>":"			{ return ':'; }

<INITIAL,MAIN>^"\n"		{ /* IGNORE */ }

<INITIAL,MAIN>"\n"		{ return '\n'; }

<INITIAL,MAIN>^"#"		{ eat_to_eol (); }

{WHITE}				{ /* IGNORE */ }

<MAIN>{WHITE}			{ return kSPACE; }

<MAIN>.				{ i386_lval.ch = *yytext; return kCHAR; }

.				{ invalid_char (*yytext); }


%%

static void
eat_to_eol (void)
{
  while (1)
    {
      int c = input ();

      if (c == EOF || c == '\n')
	break;
    }
}

static void
invalid_char (int ch)
{
  error (0, 0, (isascii (ch)
		? gettext ("invalid character '%c' at line %d; ignored")
		: gettext ("invalid character '\\%o' at line %d; ignored")),
	 ch, yylineno);
}

// Local Variables:
// mode: C
// End:
