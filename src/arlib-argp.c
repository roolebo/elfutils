/* Options common to ar and ranlib.
   Copyright (C) 2012 Red Hat, Inc.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <argp.h>
#include <libintl.h>

#include "arlib.h"

bool arlib_deterministic_output = DEFAULT_AR_DETERMINISTIC;

static const struct argp_option options[] =
  {
    { NULL, 'D', NULL, 0,
      N_("Use zero for uid, gid, and date in archive members."), 0 },
    { NULL, 'U', NULL, 0,
      N_("Use actual uid, gid, and date in archive members."), 0 },

    { NULL, 0, NULL, 0, NULL, 0 }
  };

static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
           struct argp_state *state __attribute__ ((unused)))
{
  switch (key)
    {
    case 'D':
      arlib_deterministic_output = true;
      break;

    case 'U':
      arlib_deterministic_output = false;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  inline char *text_for_default (void)
  {
    char *new_text;
    if (unlikely (asprintf (&new_text, gettext ("%s (default)"), text) < 0))
      return (char *) text;
    return new_text;
  }

  switch (key)
    {
    case 'D':
      if (DEFAULT_AR_DETERMINISTIC)
        return text_for_default ();
      break;
    case 'U':
      if (! DEFAULT_AR_DETERMINISTIC)
        return text_for_default ();
      break;
    }

  return (char *) text;
}

static const struct argp argp =
  {
    options, parse_opt, NULL, NULL, NULL, help_filter, NULL
  };

const struct argp_child arlib_argp_children[] =
  {
    { &argp, 0, "", 2 },
    { NULL, 0, NULL, 0 }
  };
