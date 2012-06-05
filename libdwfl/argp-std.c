/* Standard argp argument parsers for tools using libdwfl.
   Copyright (C) 2005-2010 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "libdwflP.h"
#include <argp.h>
#include <stdlib.h>
#include <assert.h>
#include <libintl.h>
#include <fcntl.h>
#include <unistd.h>

/* gettext helper macros.  */
#define _(Str) dgettext ("elfutils", Str)


#define OPT_DEBUGINFO	0x100
#define OPT_COREFILE	0x101

static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Input selection options:"), 0 },
  { "executable", 'e', "FILE", 0, N_("Find addresses in FILE"), 0 },
  { "core", OPT_COREFILE, "COREFILE", 0,
    N_("Find addresses from signatures found in COREFILE"), 0 },
  { "pid", 'p', "PID", 0,
    N_("Find addresses in files mapped into process PID"), 0 },
  { "linux-process-map", 'M', "FILE", 0,
    N_("Find addresses in files mapped as read from FILE"
       " in Linux /proc/PID/maps format"), 0 },
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

    /* We use this table for core files too.  */
    .find_elf = INTUSE(dwfl_build_id_find_elf),
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
  inline void failure (Dwfl *dwfl, int errnum, const char *msg)
    {
      if (dwfl != NULL)
	dwfl_end (dwfl);
      if (errnum == -1)
	argp_failure (state, EXIT_FAILURE, 0, "%s: %s",
		      msg, INTUSE(dwfl_errmsg) (-1));
      else
	argp_failure (state, EXIT_FAILURE, errnum, "%s", msg);
    }
  inline error_t fail (Dwfl *dwfl, int errnum, const char *msg)
    {
      failure (dwfl, errnum, msg);
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
	      return fail (dwfl, -1, arg);
	    state->hook = dwfl;

	    /* Start at zero so if there is just one -e foo.so,
	       the DSO is shown without address bias.  */
	    dwfl->offline_next_address = 0;
	  }
	if (dwfl->callbacks == &offline_callbacks)
	  {
	    if (INTUSE(dwfl_report_offline) (dwfl, "", arg, -1) == NULL)
	      return fail (dwfl, -1, arg);
	    state->hook = dwfl;
	  }
	else
	  {
	  toomany:
	    argp_error (state, "%s",
			_("only one of -e, -p, -k, -K, or --core allowed"));
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
	    return fail (dwfl, result, arg);
	  state->hook = dwfl;
	}
      else
	goto toomany;
      break;

    case 'M':
      if (state->hook == NULL)
	{
	  FILE *f = fopen (arg, "r");
	  if (f == NULL)
	  nofile:
	    {
	      int code = errno;
	      argp_failure (state, EXIT_FAILURE, code,
			    "cannot open '%s'", arg);
	      return code;
	    }
	  Dwfl *dwfl = INTUSE(dwfl_begin) (&proc_callbacks);
	  int result = INTUSE(dwfl_linux_proc_maps_report) (dwfl, f);
	  fclose (f);
	  if (result != 0)
	    return fail (dwfl, result, arg);
	  state->hook = dwfl;
	}
      else
	goto toomany;
      break;

    case OPT_COREFILE:
      {
	Dwfl *dwfl = state->hook;
	if (dwfl == NULL)
	  state->hook = dwfl = INTUSE(dwfl_begin) (&offline_callbacks);
	/* Permit -e and --core together.  */
	else if (dwfl->callbacks != &offline_callbacks)
	  goto toomany;

	int fd = open64 (arg, O_RDONLY);
	if (fd < 0)
	  goto nofile;

	Elf *core;
	Dwfl_Error error = __libdw_open_file (&fd, &core, true, false);
	if (error != DWFL_E_NOERROR)
	  {
	    argp_failure (state, EXIT_FAILURE, 0,
			  _("cannot read ELF core file: %s"),
			  INTUSE(dwfl_errmsg) (error));
	    return error == DWFL_E_ERRNO ? errno : EIO;
	  }

	int result = INTUSE(dwfl_core_file_report) (dwfl, core);
	if (result < 0)
	  {
	    elf_end (core);
	    close (fd);
	    return fail (dwfl, result, arg);
	  }

	/* From now we leak FD and CORE.  */

	if (result == 0)
	  {
	    argp_failure (state, EXIT_FAILURE, 0,
			  _("No modules recognized in core file"));
	    return ENOENT;
	  }
      }
      break;

    case 'k':
      if (state->hook == NULL)
	{
	  Dwfl *dwfl = INTUSE(dwfl_begin) (&kernel_callbacks);
	  int result = INTUSE(dwfl_linux_kernel_report_kernel) (dwfl);
	  if (result != 0)
	    return fail (dwfl, result, _("cannot load kernel symbols"));
	  result = INTUSE(dwfl_linux_kernel_report_modules) (dwfl);
	  if (result != 0)
	    /* Non-fatal to have no modules since we do have the kernel.  */
	    failure (dwfl, result, _("cannot find kernel modules"));
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
	    return fail (dwfl, result, _("cannot find kernel or modules"));
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
	      return fail (dwfl, -1, arg);
	    state->hook = dwfl;
	  }

	/* One of the three flavors has done dwfl_begin and some reporting
	   if we got here.  Tie up the Dwfl and return it to the caller of
	   argp_parse.  */

	int result = INTUSE(dwfl_report_end) (dwfl, NULL, NULL);
	assert (result == 0);
      }
      break;

    case ARGP_KEY_ERROR:
      dwfl_end (state->hook);
      state->hook = NULL;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  /* Update the input all along, so a parent parser can see it.  */
  *(Dwfl **) state->input = state->hook;
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
