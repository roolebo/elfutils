/* Standard find_debuginfo callback for libdwfl.
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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "system.h"


#define DEFAULT_DEBUGINFO_PATH ":.debug:/usr/lib/debug"


/* Try to open64 [DIR/][SUBDIR/]DEBUGLINK, return file descriptor or -1.
   On success, *DEBUGINFO_FILE_NAME has the malloc'd name of the open file.  */
static int
try_open (const char *dir, const char *subdir, const char *debuglink,
	  char **debuginfo_file_name)
{
  char *fname = NULL;
  if (dir == NULL && subdir == NULL)
    fname = strdup (debuglink);
  else if (subdir == NULL)
    asprintf (&fname, "%s/%s", dir, debuglink);
  else if (dir == NULL)
    asprintf (&fname, "%s/%s", subdir, debuglink);
  else
    asprintf (&fname, "%s/%s/%s", dir, subdir, debuglink);

  if (fname == NULL)
    return -1;

  int fd = open64 (fname, O_RDONLY);
  if (fd < 0)
    free (fname);
  else
    *debuginfo_file_name = fname;

  return fd;
}

/* Return true iff the FD's contents CRC matches DEBUGLINK_CRC.  */
static inline bool
check_crc (int fd, GElf_Word debuglink_crc)
{
  uint32_t file_crc;
  return crc32_file (fd, &file_crc) == 0 && file_crc == debuglink_crc;
}

int
dwfl_standard_find_debuginfo (Dwfl_Module *mod __attribute__ ((unused)),
			      void **userdata __attribute__ ((unused)),
			      const char *modname __attribute__ ((unused)),
			      GElf_Addr base __attribute__ ((unused)),
			      const char *file_name,
			      const char *debuglink_file,
			      GElf_Word debuglink_crc,
			      char **debuginfo_file_name)
{
  bool cancheck = true;

  const char *file_basename = file_name == NULL ? NULL : basename (file_name);
  if (debuglink_file == NULL)
    {
      if (file_basename == NULL)
	{
	  errno = 0;
	  return -1;
	}

      size_t len = strlen (file_basename);
      char *localname = alloca (len + sizeof ".debug");
      memcpy (localname, file_basename, len);
      memcpy (&localname[len], ".debug", sizeof ".debug");
      debuglink_file = localname;
      cancheck = false;
    }

  /* Look for a file named DEBUGLINK_FILE in the directories
     indicated by the debug directory path setting.  */

  const Dwfl_Callbacks *const cb = mod->dwfl->callbacks;
  char *path = strdupa ((cb->debuginfo_path ? *cb->debuginfo_path : NULL)
			?: DEFAULT_DEBUGINFO_PATH);

  /* A leading - or + in the whole path sets whether to check file CRCs.  */
  bool defcheck = true;
  if (path[0] == '-' || path[0] == '+')
    {
      defcheck = path[0] == '+';
      ++path;
    }

  char *file_dirname = (file_basename == file_name ? NULL
			: strndupa (file_name, file_basename - 1 - file_name));
  char *p;
  while ((p = strsep (&path, ":")) != NULL)
    {
      /* A leading - or + says whether to check file CRCs for this element.  */
      bool check = defcheck;
      if (*p == '+' || *p == '-')
	check = *p++ == '+';
      check = check && cancheck;

      const char *dir, *subdir;
      switch (p[0])
	{
	case '\0':
	  /* An empty entry says to try the main file's directory.  */
	  dir = file_dirname;
	  subdir = NULL;
	  break;
	case '/':
	  /* An absolute path says to look there for a subdirectory
	     named by the main file's absolute directory.
	     This cannot be applied to a relative file name.  */
	  if (file_dirname == NULL || file_dirname[0] != '/')
	    continue;
	  dir = p;
	  subdir = file_dirname + 1;
	  break;
	default:
	  /* A relative path says to try a subdirectory of that name
	     in the main file's directory.  */
	  dir = file_dirname;
	  subdir = p;
	  break;
	}

      char *fname;
      int fd = try_open (dir, subdir, debuglink_file, &fname);
      if (fd < 0)
	continue;
      if (!check || check_crc (fd, debuglink_crc))
	{
	  *debuginfo_file_name = fname;
	  return fd;
	}
      free (fname);
      close (fd);
    }

  /* No dice.  */
  errno = 0;
  return -1;
}
INTDEF (dwfl_standard_find_debuginfo)
