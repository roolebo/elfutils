/* Standard libdwfl callbacks for debugging the running Linux kernel.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include <config.h>
#undef	_FILE_OFFSET_BITS	/* Doesn't jibe with fts.  */

#include "libdwflP.h"
#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <fts.h>


#define MODULEDIRFMT	"/lib/modules/%s"

#define MODULELIST	"/proc/modules"
#define	SECADDRFMT	"/sys/module/%s/sections/%s"


static inline const char *
kernel_release (void)
{
  /* Cache the `uname -r` string we'll use.  */
  static struct utsname utsname;
  if (utsname.release[0] == '\0' && uname (&utsname) != 0)
    return NULL;
  return utsname.release;
}

/* Find the ELF file for the running kernel and dwfl_report_elf it.  */
int
dwfl_linux_kernel_report_kernel (Dwfl *dwfl)
{
  if (dwfl == NULL)
    return -1;

  const char *release = kernel_release ();
  if (release == NULL)
    return errno;

  char *fname = NULL;
  asprintf (&fname, "/boot/vmlinux-%s", release);
  if (fname == NULL)
    return -1;
  int fd = open64 (fname, O_RDONLY);
  if (fd < 0)
    {
      free (fname);
      fname = NULL;
      asprintf (&fname, "/usr/lib/debug" MODULEDIRFMT "/vmlinux", release);
      if (fname == NULL)
	return -1;
      fd = open64 (fname, O_RDONLY);
    }

  int result = 0;
  if (fd < 0)
    result = errno;
  else if (INTUSE(dwfl_report_elf) (dwfl, "kernel", fname, fd, 0) == NULL)
    {
      close (fd);
      result = -1;
    }

  free (fname);

  return result;
}
INTDEF (dwfl_linux_kernel_report_kernel)

/* Dwfl_Callbacks.find_elf for the running Linux kernel and its modules.  */

int
dwfl_linux_kernel_find_elf (Dwfl_Module *mod __attribute__ ((unused)),
			    void **userdata __attribute__ ((unused)),
			    const char *module_name,
			    Dwarf_Addr base __attribute__ ((unused)),
			    char **file_name,
			    Elf **elfp __attribute__ ((unused)))
{
  const char *release = kernel_release ();
  if (release == NULL)
    return -1;

  /* Do "find /lib/modules/`uname -r` -name MODULE_NAME.ko".  */

  char *modulesdir[] = { NULL, NULL };
  asprintf (&modulesdir[0], MODULEDIRFMT "/kernel", release);
  if (modulesdir[0] == NULL)
    return -1;

  FTS *fts = fts_open (modulesdir, FTS_LOGICAL | FTS_NOSTAT, NULL);
  if (fts == NULL)
    {
      free (modulesdir[0]);
      return -1;
    }

  size_t namelen = strlen (module_name);
  FTSENT *f;
  int error = ENOENT;
  while ((f = fts_read (fts)) != NULL)
    {
      error = ENOENT;
      switch (f->fts_info)
	{
	case FTS_F:
	case FTS_NSOK:
	  /* See if this file name is "MODULE_NAME.ko".  */
	  if (f->fts_namelen == namelen + 3
	      && !memcmp (f->fts_name, module_name, namelen)
	      && !memcmp (f->fts_name + namelen, ".ko", 4))
	    {
	      int fd = open64 (f->fts_accpath, O_RDONLY);
	      *file_name = strdup (f->fts_path);
	      fts_close (fts);
	      if (fd < 0)
		free (*file_name);
	      else if (*file_name == NULL)
		{
		  close (fd);
		  fd = -1;
		}
	      return fd;
	    }
	  break;

	case FTS_ERR:
	case FTS_DNR:
	case FTS_NS:
	  error = f->fts_errno;
	  break;

	default:
	  break;
	}
    }

  errno = error;
  return -1;
}
INTDEF (dwfl_linux_kernel_find_elf)

/* Dwfl_Callbacks.section_address for kernel modules in the running Linux.
   We read the information from /sys/module directly.  */

int
dwfl_linux_kernel_module_section_address
(Dwfl_Module *mod __attribute__ ((unused)),
 void **userdata __attribute__ ((unused)),
 const char *modname, Dwarf_Addr base __attribute__ ((unused)),
 const char *secname, Dwarf_Addr *addr)
{
  char *sysfile = NULL;
  asprintf (&sysfile, SECADDRFMT, modname, secname);
  if (sysfile == NULL)
    return ENOMEM;

  FILE *f = fopen (sysfile, "r");
  if (f == NULL)
    {
      if (errno == ENOENT)
	{
	  /* The .modinfo and .data.percpu sections are never kept
	     loaded in the kernel.  If the kernel was compiled without
	     CONFIG_MODULE_UNLOAD, the .exit.* sections are not
	     actually loaded at all.

	     Just relocate these bogusly to zero.  This part of the
	     debug information will not be of any use.  */

	  if (!strcmp (secname, ".modinfo")
	      || !strcmp (secname, ".data.percpu")
	      || !strncmp (secname, ".exit", 5))
	    {
	      *addr = 0;
	      return DWARF_CB_OK;
	    }
	}

      return DWARF_CB_ABORT;
    }

  (void) __fsetlocking (f, FSETLOCKING_BYCALLER);

  int result = (fscanf (f, "%" PRIi64 "\n", addr) == 1 ? 0
		: ferror_unlocked (f) ? errno : ENOEXEC);
  fclose (f);

  if (result == 0)
    return DWARF_CB_OK;

  errno = result;
  return DWARF_CB_ABORT;
}
INTDEF (dwfl_linux_kernel_module_section_address)

int
dwfl_linux_kernel_report_modules (Dwfl *dwfl)
{
  FILE *f = fopen (MODULELIST, "r");
  if (f == NULL)
    return errno;

  (void) __fsetlocking (f, FSETLOCKING_BYCALLER);

  int result = 0;
  Dwarf_Addr modaddr;
  unsigned long int modsz;
  char modname[128];
  while (fscanf (f, "%128s %lu %*s %*s %*s %" PRIi64 "\n",
		 modname, &modsz, &modaddr) == 3)
    if (INTUSE(dwfl_report_module) (dwfl, modname,
				    modaddr, modaddr + modsz) == NULL)
      {
	result = -1;
	break;
      }

  if (result == 0)
    result = ferror_unlocked (f) ? errno : feof_unlocked (f) ? 0 : ENOEXEC;

  fclose (f);

  return result;
}
INTDEF (dwfl_linux_kernel_report_modules)
