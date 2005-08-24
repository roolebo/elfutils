/* Standard argp argument parsers for tools using libdwfl.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include "libdwflP.h"
#include <argp.h>
#include <stdlib.h>
#include <assert.h>
#include <libintl.h>

/* gettext helper macros.  */
#define _(Str) dgettext ("elfutils", Str)


#define OPT_DEBUGINFO 0x100

static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Input Selection:"), 0 },
  { "executable", 'e', "FILE", 0, N_("Find addresses in FILE"), 0 },
  { "pid", 'p', "PID", 0,
    N_("Find addresses in files mapped into process PID"), 0 },
  { "kernel", 'k', NULL, 0, N_("Find addresses in the running kernel"), 0 },
  { "offline-kernel", 'K', "RELEASE", OPTION_ARG_OPTIONAL,
    N_("Kernel with all modules"), 0 },
  { "debuginfo-path", OPT_DEBUGINFO, "PATH", 0,
    N_("Search path for separate debuginfo files"), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static char *debuginfo_path;

static const Dwfl_Callbacks offline_callbacks =
  {
    .find_debuginfo = INTUSE(dwfl_standard_find_debuginfo),
    .debuginfo_path = &debuginfo_path,

    .section_address = INTUSE(dwfl_offline_section_address),
  };

static const Dwfl_Callbacks proc_callbacks =
  {
    .find_debuginfo = INTUSE(dwfl_standard_find_debuginfo),
    .debuginfo_path = &debuginfo_path,

    .find_elf = INTUSE(dwfl_linux_proc_find_elf),
  };

static const Dwfl_Callbacks kernel_callbacks =
  {
    .find_debuginfo = INTUSE(dwfl_standard_find_debuginfo),
    .debuginfo_path = &debuginfo_path,

    .find_elf = INTUSE(dwfl_linux_kernel_find_elf),
    .section_address = INTUSE(dwfl_linux_kernel_module_section_address),
  };

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  inline void failure (int errnum, const char *msg)
    {
      if (errnum == -1)
	argp_failure (state, EXIT_FAILURE, 0, "%s: %s",
		      msg, INTUSE(dwfl_errmsg) (-1));
      else
	argp_failure (state, EXIT_FAILURE, errnum, "%s", msg);
    }
  inline error_t fail (int errnum, const char *msg)
    {
      failure (errnum, msg);
      return errnum == -1 ? EIO : errnum;
    }

  switch (key)
    {
    case OPT_DEBUGINFO:
      debuginfo_path = arg;
      break;

    case 'e':
      {
	Dwfl *dwfl = state->hook;
	if (dwfl == NULL)
	  {
	    dwfl = INTUSE(dwfl_begin) (&offline_callbacks);
	    if (dwfl == NULL)
	      return fail (-1, arg);
	    state->hook = dwfl;
	  }
	if (dwfl->callbacks == &offline_callbacks)
	  {
	    if (INTUSE(dwfl_report_offline) (dwfl, "", arg, -1) == NULL)
	      return fail (-1, arg);
	    state->hook = dwfl;
	  }
	else
	  {
	  toomany:
	    argp_error (state,
			"%s", _("only one of -e, -p, -k, or -K allowed"));
	    return EINVAL;
	  }
      }
      break;

    case 'p':
      if (state->hook == NULL)
	{
	  Dwfl *dwfl = INTUSE(dwfl_begin) (&proc_callbacks);
	  int result = INTUSE(dwfl_linux_proc_report) (dwfl, atoi (arg));
	  if (result != 0)
	    return fail (result, arg);
	  state->hook = dwfl;
	}
      else
	goto toomany;
      break;

    case 'k':
      if (state->hook == NULL)
	{
	  Dwfl *dwfl = INTUSE(dwfl_begin) (&kernel_callbacks);
	  int result = INTUSE(dwfl_linux_kernel_report_kernel) (dwfl);
	  if (result != 0)
	    return fail (result, _("cannot load kernel symbols"));
	  result = INTUSE(dwfl_linux_kernel_report_modules) (dwfl);
	  if (result != 0)
	    /* Non-fatal to have no modules since we do have the kernel.  */
	    failure (result, _("cannot find kernel modules"));
	  state->hook = dwfl;
	}
      else
	goto toomany;
      break;

    case 'K':
      if (state->hook == NULL)
	{
	  Dwfl *dwfl = INTUSE(dwfl_begin) (&offline_callbacks);
	  int result = INTUSE(dwfl_linux_kernel_report_offline) (dwfl, arg,
								 NULL);
	  if (result != 0)
	    return fail (result, _("cannot find kernel or modules"));
	  state->hook = dwfl;
	}
      else
	goto toomany;
      break;

    case ARGP_KEY_SUCCESS:
      {
	Dwfl *dwfl = state->hook;

	if (dwfl == NULL)
	  {
	    /* Default if no -e, -p, or -k, is "-e a.out".  */
	    arg = "a.out";
	    dwfl = INTUSE(dwfl_begin) (&offline_callbacks);
	    if (INTUSE(dwfl_report_offline) (dwfl, "", arg, -1) == NULL)
	      return fail (-1, arg);
	    state->hook = dwfl;
	  }

	/* One of the three flavors has done dwfl_begin and some reporting
	   if we got here.  Tie up the Dwfl and return it to the caller of
	   argp_parse.  */

	int result = INTUSE(dwfl_report_end) (dwfl, NULL, NULL);
	assert (result == 0);

	*(Dwfl **) state->input = dwfl;
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static const struct argp libdwfl_argp =
  { .options = options, .parser = parse_opt };

const struct argp *
dwfl_standard_argp (void)
{
  return &libdwfl_argp;
}

#ifdef _MUDFLAP
/* In the absence of a mudflap wrapper for argp_parse, or a libc compiled
   with -fmudflap, we'll see spurious errors for using the struct argp_state
   on argp_parse's stack.  */

void __attribute__ ((constructor))
__libdwfl_argp_mudflap_options (void)
{
  __mf_set_options ("-heur-stack-bound");
}
#endif
