/* Create, modify, and extract from archives.
   Copyright (C) 2005-2012, 2016, 2017 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <argp.h>
#include <assert.h>
#include <fcntl.h>
#include <gelf.h>
#include <libintl.h>
#include <limits.h>
#include <locale.h>
#include <search.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE___FSETLOCKING
# include <stdio_ext.h>
#endif
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "dirname.h"
#include <system.h>
#include <printversion.h>

#include "arlib.h"


/* Name and version of program.  */
ARGP_PROGRAM_VERSION_HOOK_DEF = print_version;

/* Prototypes for local functions.  */
static int do_oper_extract (int oper, const char *arfname, char **argv,
			    int argc, long int instance);
static int do_oper_delete (const char *arfname, char **argv, int argc,
			   long int instance);
static int do_oper_insert (int oper, const char *arfname, char **argv,
			   int argc, const char *member);


/* Bug report address.  */
ARGP_PROGRAM_BUG_ADDRESS_DEF = PACKAGE_BUGREPORT;


/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Commands:"), 1 },
  { NULL, 'd', NULL, 0, N_("Delete files from archive."), 0 },
  { NULL, 'm', NULL, 0, N_("Move files in archive."), 0 },
  { NULL, 'p', NULL, 0, N_("Print files in archive."), 0 },
  { NULL, 'q', NULL, 0, N_("Quick append files to archive."), 0 },
  { NULL, 'r', NULL, 0,
    N_("Replace existing or insert new file into archive."), 0 },
  { NULL, 't', NULL, 0, N_("Display content of archive."), 0 },
  { NULL, 'x', NULL, 0, N_("Extract files from archive."), 0 },

  { NULL, 0, NULL, 0, N_("Command Modifiers:"), 2 },
  { NULL, 'o', NULL, 0, N_("Preserve original dates."), 0 },
  { NULL, 'N', NULL, 0, N_("Use instance [COUNT] of name."), 0 },
  { NULL, 'C', NULL, 0,
    N_("Do not replace existing files with extracted files."), 0 },
  { NULL, 'T', NULL, 0, N_("Allow filename to be truncated if necessary."),
    0 },
  { NULL, 'v', NULL, 0, N_("Provide verbose output."), 0 },
  { NULL, 's', NULL, 0, N_("Force regeneration of symbol table."), 0 },
  { NULL, 'a', NULL, 0, N_("Insert file after [MEMBER]."), 0 },
  { NULL, 'b', NULL, 0, N_("Insert file before [MEMBER]."), 0 },
  { NULL, 'i', NULL, 0, N_("Same as -b."), 0 },
  { NULL, 'c', NULL, 0, N_("Suppress message when library has to be created."),
    0 },
  { NULL, 'P', NULL, 0, N_("Use full path for file matching."), 0 },
  { NULL, 'u', NULL, 0, N_("Update only older files in archive."), 0 },

  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("Create, modify, and extract from archives.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("[MEMBER] [COUNT] ARCHIVE [FILE...]");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, arlib_argp_children, NULL, NULL
};


/* What operation to perform.  */
static enum
  {
    oper_none,
    oper_delete,
    oper_move,
    oper_print,
    oper_qappend,
    oper_replace,
    oper_list,
    oper_extract
  } operation;

/* Modifiers.  */
static bool verbose;
static bool preserve_dates;
static bool instance_specifed;
static bool dont_replace_existing;
static bool allow_truncate_fname;
static bool force_symtab;
static bool suppress_create_msg;
static bool full_path;
static bool update_newer;
static enum { ipos_none, ipos_before, ipos_after } ipos;


int
main (int argc, char *argv[])
{
#ifdef HAVE___FSETLOCKING
  /* We use no threads here which can interfere with handling a stream.  */
  (void) __fsetlocking (stdin, FSETLOCKING_BYCALLER);
  (void) __fsetlocking (stdout, FSETLOCKING_BYCALLER);
  (void) __fsetlocking (stderr, FSETLOCKING_BYCALLER);
#endif

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  /* Make sure the message catalog can be found.  */
  (void) bindtextdomain (PACKAGE_TARNAME, LOCALEDIR);

  /* Initialize the message catalog.  */
  (void) textdomain (PACKAGE_TARNAME);

  /* For historical reasons the options in the first parameter need
     not be preceded by a dash.  Add it now if necessary.  */
  if (argc > 1 && argv[1][0] != '-')
    {
      size_t len = strlen (argv[1]) + 1;
      char *newp = alloca (len + 1);
      newp[0] = '-';
      memcpy (&newp[1], argv[1], len);
      argv[1] = newp;
    }

  /* Parse and process arguments.  */
  int remaining;
  (void) argp_parse (&argp, argc, argv, ARGP_IN_ORDER, &remaining, NULL);

  /* Tell the library which version we are expecting.  */
  (void) elf_version (EV_CURRENT);

  /* Handle the [MEMBER] parameter.  */
  const char *member = NULL;
  if (ipos != ipos_none)
    {
      /* Only valid for certain operations.  */
      if (operation != oper_move && operation != oper_replace)
	error (1, 0, gettext ("\
'a', 'b', and 'i' are only allowed with the 'm' and 'r' options"));

      if (remaining == argc)
	{
	  error (0, 0, gettext ("\
MEMBER parameter required for 'a', 'b', and 'i' modifiers"));
	  argp_help (&argp, stderr, ARGP_HELP_USAGE | ARGP_HELP_SEE,
		     program_invocation_short_name);
	  exit (EXIT_FAILURE);
	}

      member = argv[remaining++];
    }

  /* Handle the [COUNT] parameter.  */
  long int instance = -1;
  if (instance_specifed)
    {
      /* Only valid for certain operations.  */
      if (operation != oper_extract && operation != oper_delete)
	error (1, 0, gettext ("\
'N' is only meaningful with the 'x' and 'd' options"));

      if (remaining == argc)
	{
	  error (0, 0, gettext ("COUNT parameter required"));
	  argp_help (&argp, stderr, ARGP_HELP_SEE,
		     program_invocation_short_name);
	  exit (EXIT_FAILURE);
	}

      char *endp;
      errno = 0;
      if (((instance = strtol (argv[remaining], &endp, 10)) == LONG_MAX
	   && errno == ERANGE)
	  || instance <= 0
	  || *endp != '\0')
	error (1, 0, gettext ("invalid COUNT parameter %s"), argv[remaining]);

      ++remaining;
    }

  if ((dont_replace_existing || allow_truncate_fname)
      && unlikely (operation != oper_extract))
    error (1, 0, gettext ("'%c' is only meaningful with the 'x' option"),
	   dont_replace_existing ? 'C' : 'T');

  /* There must at least be one more parameter specifying the archive.   */
  if (remaining == argc)
    {
      error (0, 0, gettext ("archive name required"));
      argp_help (&argp, stderr, ARGP_HELP_SEE, program_invocation_short_name);
      exit (EXIT_FAILURE);
    }

  const char *arfname = argv[remaining++];
  argv += remaining;
  argc -= remaining;

  int status;
  switch (operation)
    {
    case oper_none:
      error (0, 0, gettext ("command option required"));
      argp_help (&argp, stderr, ARGP_HELP_STD_ERR,
		 program_invocation_short_name);
      status = 1;
      break;

    case oper_list:
    case oper_print:
      status = do_oper_extract (operation, arfname, argv, argc, -1);
      break;

    case oper_extract:
      status = do_oper_extract (operation, arfname, argv, argc, instance);
      break;

    case oper_delete:
      status = do_oper_delete (arfname, argv, argc, instance);
      break;

    case oper_move:
    case oper_qappend:
    case oper_replace:
      status = do_oper_insert (operation, arfname, argv, argc, member);
      break;

    default:
      assert (! "should not happen");
      status = 1;
      break;
    }

  return status;
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
	   struct argp_state *state __attribute__ ((unused)))
{
  switch (key)
    {
    case 'd':
    case 'm':
    case 'p':
    case 'q':
    case 'r':
    case 't':
    case 'x':
      if (operation != oper_none)
	{
	  error (0, 0, gettext ("More than one operation specified"));
	  argp_help (&argp, stderr, ARGP_HELP_SEE,
		     program_invocation_short_name);
	  exit (EXIT_FAILURE);
	}

      switch (key)
	{
	case 'd':
	  operation = oper_delete;
	  break;
	case 'm':
	  operation = oper_move;
	  break;
	case 'p':
	  operation = oper_print;
	  break;
	case 'q':
	  operation = oper_qappend;
	  break;
	case 'r':
	  operation = oper_replace;
	  break;
	case 't':
	  operation = oper_list;
	  break;
	case 'x':
	  operation = oper_extract;
	  break;
	}
      break;

    case 'a':
      ipos = ipos_after;
      break;

    case 'b':
    case 'i':
      ipos = ipos_before;
      break;

    case 'c':
      suppress_create_msg = true;
      break;

    case 'C':
      dont_replace_existing = true;
      break;

    case 'N':
      instance_specifed = true;
      break;

    case 'o':
      preserve_dates = true;
      break;

    case 'P':
      full_path = true;
      break;

    case 's':
      force_symtab = true;
      break;

    case 'T':
      allow_truncate_fname = true;
      break;

    case 'u':
      update_newer = true;
      break;

    case 'v':
      verbose = true;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static int
open_archive (const char *arfname, int flags, int mode, Elf **elf,
	      struct stat *st, bool miss_allowed)
{
  int fd = open (arfname, flags, mode);
  if (fd == -1)
    {
      if (miss_allowed)
	return -1;

      error (EXIT_FAILURE, errno, gettext ("cannot open archive '%s'"),
	     arfname);
    }

  if (elf != NULL)
    {
      Elf_Cmd cmd = flags == O_RDONLY ? ELF_C_READ_MMAP : ELF_C_RDWR_MMAP;

      *elf = elf_begin (fd, cmd, NULL);
      if (*elf == NULL)
	error (EXIT_FAILURE, 0, gettext ("cannot open archive '%s': %s"),
	       arfname, elf_errmsg (-1));

      if (flags == O_RDONLY && elf_kind (*elf) != ELF_K_AR)
	error (EXIT_FAILURE, 0, gettext ("%s: not an archive file"), arfname);
    }

  if (st != NULL && fstat (fd, st) != 0)
    error (EXIT_FAILURE, errno, gettext ("cannot stat archive '%s'"),
	   arfname);

  return fd;
}


static void
not_found (int argc, char *argv[argc], bool found[argc])
{
  for (int i = 0; i < argc; ++i)
    if (!found[i])
      printf (gettext ("no entry %s in archive\n"), argv[i]);
}


static int
copy_content (Elf *elf, int newfd, off_t off, size_t n)
{
  size_t len;
  char *rawfile = elf_rawfile (elf, &len);

  assert (off + n <= len);

  /* Tell the kernel we will read all the pages sequentially.  */
  size_t ps = sysconf (_SC_PAGESIZE);
  if (n > 2 * ps)
    posix_madvise (rawfile + (off & ~(ps - 1)), n, POSIX_MADV_SEQUENTIAL);

  return write_retry (newfd, rawfile + off, n) != (ssize_t) n;
}


static int
do_oper_extract (int oper, const char *arfname, char **argv, int argc,
		 long int instance)
{
  bool found[argc > 0 ? argc : 1];
  memset (found, '\0', sizeof (found));

  size_t name_max = 0;
  inline bool should_truncate_fname (void)
  {
    if (errno == ENAMETOOLONG && allow_truncate_fname)
      {
	if (name_max == 0)
	  {
	    long int len = pathconf (".", _PC_NAME_MAX);
	    if (len > 0)
	      name_max = len;
	  }
	return name_max != 0;
      }
    return false;
  }

  off_t index_off = -1;
  size_t index_size = 0;
  off_t cur_off = SARMAG;

  int status = 0;
  Elf *elf;
  int fd = open_archive (arfname, O_RDONLY, 0, &elf, NULL, false);

  if (hcreate (2 * argc) == 0)
    error (EXIT_FAILURE, errno, gettext ("cannot create hash table"));

  for (int cnt = 0; cnt < argc; ++cnt)
    {
      ENTRY entry = { .key = argv[cnt], .data = &argv[cnt] };
      if (hsearch (entry, ENTER) == NULL)
	error (EXIT_FAILURE, errno,
	       gettext ("cannot insert into hash table"));
    }

  struct stat st;
  if (force_symtab)
    {
      if (fstat (fd, &st) != 0)
	{
	  error (0, errno, gettext ("cannot stat '%s'"), arfname);
	  close (fd);
	  return 1;
	}
      arlib_init ();
    }

  Elf_Cmd cmd = ELF_C_READ_MMAP;
  Elf *subelf;
  while ((subelf = elf_begin (fd, cmd, elf)) != NULL)
    {
      Elf_Arhdr *arhdr = elf_getarhdr (subelf);

      if (strcmp (arhdr->ar_name, "/") == 0)
	{
	  index_off = elf_getaroff (subelf);
	  index_size = arhdr->ar_size;
	  goto next;
	}
      if (strcmp (arhdr->ar_name, "//") == 0)
	goto next;

      if (force_symtab)
	{
	  arlib_add_symbols (elf, arfname, arhdr->ar_name, cur_off);
	  cur_off += (((arhdr->ar_size + 1) & ~((off_t) 1))
		      + sizeof (struct ar_hdr));
	}

      bool do_extract = argc <= 0;
      if (!do_extract)
	{
	  ENTRY entry;
	  entry.key = arhdr->ar_name;
	  ENTRY *res = hsearch (entry, FIND);
	  if (res != NULL && (instance < 0 || instance-- == 0)
	      && !found[(char **) res->data - argv])
	    found[(char **) res->data - argv] = do_extract = true;
	}

      if (do_extract)
	{
	  if (verbose)
	    {
	      if (oper == oper_print)
		{
		  printf ("\n<%s>\n\n", arhdr->ar_name);

		  /* We have to flush now because now we use the descriptor
		     directly.  */
		  fflush (stdout);
		}
	      else if (oper == oper_list)
		{
		  char datestr[100];
		  struct tm *tp = localtime (&arhdr->ar_date);
		  if (tp == NULL)
		    {
		      time_t time = 0;
		      tp = localtime (&time);
		    }

		  strftime (datestr, sizeof (datestr), "%b %e %H:%M %Y", tp);

		  printf ("%c%c%c%c%c%c%c%c%c %u/%u %6ju %s %s\n",
			  (arhdr->ar_mode & S_IRUSR) ? 'r' : '-',
			  (arhdr->ar_mode & S_IWUSR) ? 'w' : '-',
			  (arhdr->ar_mode & S_IXUSR)
			  ? ((arhdr->ar_mode & S_ISUID) ? 's' : 'x')
			  : ((arhdr->ar_mode & S_ISUID) ? 'S' : '-'),
			  (arhdr->ar_mode & S_IRGRP) ? 'r' : '-',
			  (arhdr->ar_mode & S_IWGRP) ? 'w' : '-',
			  (arhdr->ar_mode & S_IXGRP)
			  ? ((arhdr->ar_mode & S_ISGID) ? 's' : 'x')
			  : ((arhdr->ar_mode & S_ISGID) ? 'S' : '-'),
			  (arhdr->ar_mode & S_IROTH) ? 'r' : '-',
			  (arhdr->ar_mode & S_IWOTH) ? 'w' : '-',
			  (arhdr->ar_mode & S_IXOTH)
			  ? ((arhdr->ar_mode & S_ISVTX) ? 't' : 'x')
			  : ((arhdr->ar_mode & S_ISVTX) ? 'T' : '-'),
			  arhdr->ar_uid,
			  arhdr->ar_gid,
			  (uintmax_t) arhdr->ar_size,
			  datestr,
			  arhdr->ar_name);
		}
	      else
		printf ("x - %s\n", arhdr->ar_name);
	    }

	  if (oper == oper_list)
	    {
	      if (!verbose)
		puts (arhdr->ar_name);

	      goto next;
	    }

	  size_t nleft;
	  char *data = elf_rawfile (subelf, &nleft);
	  if (data == NULL)
	    {
	      error (0, 0, gettext ("cannot read content of %s: %s"),
		     arhdr->ar_name, elf_errmsg (-1));
	      status = 1;
	      goto next;
	    }

	  int xfd;
	  char tempfname[] = "XXXXXX";
	  bool use_mkstemp = true;

	  if (oper == oper_print)
	    xfd = STDOUT_FILENO;
	  else
	    {
	      xfd = mkstemp (tempfname);
	      if (unlikely (xfd == -1))
		{
		  /* We cannot create a temporary file.  Try to overwrite
		     the file or create it if it does not exist.  */
		  int flags = O_WRONLY | O_CREAT;
		  if (dont_replace_existing)
		    flags |= O_EXCL;
		  else
		    flags |= O_TRUNC;
		  xfd = open (arhdr->ar_name, flags, 0600);
		  if (unlikely (xfd == -1))
		    {
		      int printlen = INT_MAX;

		      if (should_truncate_fname ())
			{
			  /* Try to truncate the name.  First find out by how
			     much.  */
			  printlen = name_max;
			  char truncfname[name_max + 1];
			  *((char *) mempcpy (truncfname, arhdr->ar_name,
					      name_max)) = '\0';

			  xfd = open (truncfname, flags, 0600);
			}

		      if (xfd == -1)
			{
			  error (0, errno, gettext ("cannot open %.*s"),
				 (int) printlen, arhdr->ar_name);
			  status = 1;
			  goto next;
			}
		    }

		  use_mkstemp = false;
		}
	    }

	  ssize_t n;
	  while ((n = TEMP_FAILURE_RETRY (write (xfd, data, nleft))) != -1)
	    {
	      nleft -= n;
	      if (nleft == 0)
		break;
	      data += n;
	    }

	  if (unlikely (n == -1))
	    {
	      error (0, errno, gettext ("failed to write %s"), arhdr->ar_name);
	      status = 1;
	      unlink (tempfname);
	      close (xfd);
	      goto next;
	    }

	  if (oper != oper_print)
	    {
	      /* Fix up the mode.  */
	      if (unlikely (fchmod (xfd, arhdr->ar_mode) != 0))
		{
		  error (0, errno, gettext ("cannot change mode of %s"),
			 arhdr->ar_name);
		  status = 0;
		}

	      if (preserve_dates)
		{
		  struct timespec tv[2];
		  tv[0].tv_sec = arhdr->ar_date;
		  tv[0].tv_nsec = 0;
		  tv[1].tv_sec = arhdr->ar_date;
		  tv[1].tv_nsec = 0;

		  if (unlikely (futimens (xfd, tv) != 0))
		    {
		      error (0, errno,
			     gettext ("cannot change modification time of %s"),
			     arhdr->ar_name);
		      status = 1;
		    }
		}

	      /* If we used a temporary file, move it do the right
		 name now.  */
	      if (use_mkstemp)
		{
		  int r;

		  if (dont_replace_existing)
		    {
		      r = link (tempfname, arhdr->ar_name);
		      if (likely (r == 0))
			unlink (tempfname);
		    }
		  else
		    r = rename (tempfname, arhdr->ar_name);

		  if (unlikely (r) != 0)
		    {
		      int printlen = INT_MAX;

		      if (should_truncate_fname ())
			{
			  /* Try to truncate the name.  First find out by how
			     much.  */
			  printlen = name_max;
			  char truncfname[name_max + 1];
			  *((char *) mempcpy (truncfname, arhdr->ar_name,
					      name_max)) = '\0';

			  if (dont_replace_existing)
			    {
			      r = link (tempfname, truncfname);
			      if (likely (r == 0))
				unlink (tempfname);
			    }
			  else
			    r = rename (tempfname, truncfname);
			}

		      if (r != 0)
			{
			  error (0, errno, gettext ("\
cannot rename temporary file to %.*s"),
				 printlen, arhdr->ar_name);
			  unlink (tempfname);
			  status = 1;
			}
		    }
		}

	      close (xfd);
	    }
	}

    next:
      cmd = elf_next (subelf);
      if (elf_end (subelf) != 0)
	error (1, 0, "%s: %s", arfname, elf_errmsg (-1));
    }

  hdestroy ();

  if (force_symtab)
    {
      arlib_finalize ();

      if (symtab.symsnamelen != 0
	  /* We have to rewrite the file also if it initially had an index
	     but now does not need one anymore.  */
	  || (symtab.symsnamelen == 0 && index_size != 0))
	{
	  char tmpfname[strlen (arfname) + 7];
	  strcpy (stpcpy (tmpfname, arfname), "XXXXXX");
	  int newfd = mkstemp (tmpfname);
	  if (unlikely (newfd == -1))
	    {
	    nonew:
	      error (0, errno, gettext ("cannot create new file"));
	      status = 1;
	    }
	  else
	    {
	      /* Create the header.  */
	      if (unlikely (write_retry (newfd, ARMAG, SARMAG) != SARMAG))
		{
		  // XXX Use /prof/self/fd/%d ???
		nonew_unlink:
		  unlink (tmpfname);
		  if (newfd != -1)
		    close (newfd);
		  goto nonew;
		}

	      /* Create the new file.  There are three parts as far we are
		 concerned: 1. original context before the index, 2. the
		 new index, 3. everything after the new index.  */
	      off_t rest_off;
	      if (index_off != -1)
		rest_off = (index_off + sizeof (struct ar_hdr)
			    + ((index_size + 1) & ~1ul));
	      else
		rest_off = SARMAG;

	      if ((symtab.symsnamelen != 0
		   && ((write_retry (newfd, symtab.symsoff,
				     symtab.symsofflen)
			!= (ssize_t) symtab.symsofflen)
		       || (write_retry (newfd, symtab.symsname,
					symtab.symsnamelen)
			   != (ssize_t) symtab.symsnamelen)))
		  /* Even if the original file had content before the
		     symbol table, we write it in the correct order.  */
		  || (index_off != SARMAG
		      && copy_content (elf, newfd, SARMAG, index_off - SARMAG))
		  || copy_content (elf, newfd, rest_off, st.st_size - rest_off)
		  /* Set the mode of the new file to the same values the
		     original file has.  */
		  || fchmod (newfd, st.st_mode & ALLPERMS) != 0
		  /* Never complain about fchown failing.  */
		  || (({asm ("" :: "r" (fchown (newfd, st.st_uid,
						st.st_gid))); }),
		      close (newfd) != 0)
		  || (newfd = -1, rename (tmpfname, arfname) != 0))
		goto nonew_unlink;
	    }
	}
    }

  elf_end (elf);

  close (fd);

  not_found (argc, argv, found);

  return status;
}


struct armem
{
  off_t off;
  off_t old_off;
  size_t size;
  long int long_name_off;
  struct armem *next;
  void *mem;
  time_t sec;
  uid_t uid;
  gid_t gid;
  mode_t mode;
  const char *name;
  Elf *elf;
};


static int
write_member (struct armem *memb, off_t *startp, off_t *lenp, Elf *elf,
	      off_t end_off, int newfd)
{
  struct ar_hdr arhdr;
  /* The ar_name is not actually zero teminated, but we need that for
     snprintf.  Also if the name is too long, then the string starts
     with '/' plus an index off number (decimal).  */
  char tmpbuf[sizeof (arhdr.ar_name) + 2];

  bool changed_header = memb->long_name_off != -1;
  if (changed_header)
    {
      /* In case of a long file name we assume the archive header
	 changed and we write it here.  */
      memcpy (&arhdr, elf_rawfile (elf, NULL) + *startp, sizeof (arhdr));

      snprintf (tmpbuf, sizeof (tmpbuf), "/%-*ld",
		(int) sizeof (arhdr.ar_name), memb->long_name_off);
      changed_header = memcmp (arhdr.ar_name, tmpbuf,
			       sizeof (arhdr.ar_name)) != 0;
    }

  /* If the files are adjacent in the old file extend the range.  */
  if (*startp != -1 && !changed_header && *startp + *lenp == memb->old_off)
    {
      /* Extend the current range.  */
      *lenp += (memb->next != NULL
		? memb->next->off : end_off) - memb->off;
      return 0;
    }

  /* Write out the old range.  */
  if (*startp != -1 && copy_content (elf, newfd, *startp, *lenp))
    return -1;

  *startp = memb->old_off;
  *lenp = (memb->next != NULL ? memb->next->off : end_off) - memb->off;

  if (changed_header)
    {
      memcpy (arhdr.ar_name, tmpbuf, sizeof (arhdr.ar_name));

      if (unlikely (write_retry (newfd, &arhdr, sizeof (arhdr))
		    != sizeof (arhdr)))
	return -1;

      *startp += sizeof (struct ar_hdr);
      assert ((size_t) *lenp >= sizeof (struct ar_hdr));
      *lenp -= sizeof (struct ar_hdr);
    }

  return 0;
}

/* Store the name in the long name table if necessary.
   Record its offset or -1 if we did not need to use the table.  */
static void
remember_long_name (struct armem *mem, const char *name, size_t namelen)
{
  mem->long_name_off = (namelen > MAX_AR_NAME_LEN
			? arlib_add_long_name (name, namelen)
			: -1l);
}

static int
do_oper_delete (const char *arfname, char **argv, int argc,
		long int instance)
{
  bool *found = alloca (sizeof (bool) * argc);
  memset (found, '\0', sizeof (bool) * argc);

  /* List of the files we keep.  */
  struct armem *to_copy = NULL;

  int status = 0;
  Elf *elf;
  struct stat st;
  int fd = open_archive (arfname, O_RDONLY, 0, &elf, &st, false);

  if (hcreate (2 * argc) == 0)
    error (EXIT_FAILURE, errno, gettext ("cannot create hash table"));

  for (int cnt = 0; cnt < argc; ++cnt)
    {
      ENTRY entry = { .key = argv[cnt], .data = &argv[cnt] };
      if (hsearch (entry, ENTER) == NULL)
	error (EXIT_FAILURE, errno,
	       gettext ("cannot insert into hash table"));
    }

  arlib_init ();

  off_t cur_off = SARMAG;
  Elf_Cmd cmd = ELF_C_READ_MMAP;
  Elf *subelf;
  while ((subelf = elf_begin (fd, cmd, elf)) != NULL)
    {
      Elf_Arhdr *arhdr = elf_getarhdr (subelf);

      /* Ignore the symbol table and the long file name table here.  */
      if (strcmp (arhdr->ar_name, "/") == 0
	  || strcmp (arhdr->ar_name, "//") == 0)
	goto next;

      bool do_delete = argc <= 0;
      if (!do_delete)
	{
	  ENTRY entry;
	  entry.key = arhdr->ar_name;
	  ENTRY *res = hsearch (entry, FIND);
	  if (res != NULL && (instance < 0 || instance-- == 0)
	      && !found[(char **) res->data - argv])
	    found[(char **) res->data - argv] = do_delete = true;
	}

      if (do_delete)
	{
	  if (verbose)
	    printf ("d - %s\n", arhdr->ar_name);
	}
      else
	{
	  struct armem *newp = alloca (sizeof (struct armem));
	  newp->old_off = elf_getaroff (subelf);
	  newp->off = cur_off;

	  cur_off += (((arhdr->ar_size + 1) & ~((off_t) 1))
		      + sizeof (struct ar_hdr));

	  if (to_copy == NULL)
	    to_copy = newp->next = newp;
	  else
	    {
	      newp->next = to_copy->next;
	      to_copy = to_copy->next = newp;
	    }

	  /* If we recreate the symbol table read the file's symbol
	     table now.  */
	  arlib_add_symbols (subelf, arfname, arhdr->ar_name, newp->off);

	  /* Remember long file names.  */
	  remember_long_name (newp, arhdr->ar_name, strlen (arhdr->ar_name));
	}

    next:
      cmd = elf_next (subelf);
      if (elf_end (subelf) != 0)
	error (1, 0, "%s: %s", arfname, elf_errmsg (-1));
    }

  arlib_finalize ();

  hdestroy ();

  /* Create a new, temporary file in the same directory as the
     original file.  */
  char tmpfname[strlen (arfname) + 7];
  strcpy (stpcpy (tmpfname, arfname), "XXXXXX");
  int newfd = mkstemp (tmpfname);
  if (unlikely (newfd == -1))
    goto nonew;

  /* Create the header.  */
  if (unlikely (write_retry (newfd, ARMAG, SARMAG) != SARMAG))
    {
      // XXX Use /prof/self/fd/%d ???
    nonew_unlink:
      unlink (tmpfname);
      if (newfd != -1)
	close (newfd);
    nonew:
      error (0, errno, gettext ("cannot create new file"));
      status = 1;
      goto errout;
    }

  /* If the archive is empty that is all we have to do.  */
  if (likely (to_copy != NULL))
    {
      /* Write the symbol table or the long file name table or both.  */
      if (symtab.symsnamelen != 0
	  && ((write_retry (newfd, symtab.symsoff, symtab.symsofflen)
	       != (ssize_t) symtab.symsofflen)
	      || (write_retry (newfd, symtab.symsname, symtab.symsnamelen)
		  != (ssize_t) symtab.symsnamelen)))
	goto nonew_unlink;

      if (symtab.longnameslen > sizeof (struct ar_hdr)
	  && (write_retry (newfd, symtab.longnames, symtab.longnameslen)
	      != (ssize_t) symtab.longnameslen))
	goto nonew_unlink;

      /* NULL-terminate the list of files to copy.  */
      struct armem *last = to_copy;
      to_copy = to_copy->next;
      last->next = NULL;

      off_t start = -1;
      off_t len = -1;

      do
	if (write_member (to_copy, &start, &len, elf, cur_off, newfd) != 0)
	  goto nonew_unlink;
      while ((to_copy = to_copy->next) != NULL);

      /* Write the last part.  */
      if (copy_content (elf, newfd, start, len))
	goto nonew_unlink;
    }

  /* Set the mode of the new file to the same values the original file
     has.  */
  if (fchmod (newfd, st.st_mode & ALLPERMS) != 0
      /* Never complain about fchown failing.  */
      || (({asm ("" :: "r" (fchown (newfd, st.st_uid, st.st_gid))); }),
	  close (newfd) != 0)
      || (newfd = -1, rename (tmpfname, arfname) != 0))
    goto nonew_unlink;

 errout:
  elf_end (elf);

  arlib_fini ();

  close (fd);

  not_found (argc, argv, found);

  return status;
}


/* Prints the given value in the given buffer without a trailing zero char.
   Returns false if the given value doesn't fit in the given buffer.  */
static bool
no0print (bool ofmt, char *buf, int bufsize, long int val)
{
  char tmpbuf[bufsize + 1];
  int ret = snprintf (tmpbuf, sizeof (tmpbuf), ofmt ? "%-*lo" : "%-*ld",
		      bufsize, val);
  if (ret >= (int) sizeof (tmpbuf))
    return false;
  memcpy (buf, tmpbuf, bufsize);
  return true;
}


static int
do_oper_insert (int oper, const char *arfname, char **argv, int argc,
		const char *member)
{
  int status = 0;
  Elf *elf = NULL;
  struct stat st;
  int fd = open_archive (arfname, O_RDONLY, 0, &elf, &st, oper != oper_move);

  /* List of the files we keep.  */
  struct armem *all = NULL;
  struct armem *after_memberelem = NULL;
  struct armem **found = alloca (sizeof (*found) * argc);
  memset (found, '\0', sizeof (*found) * argc);

  arlib_init ();

  /* Initialize early for no_old case.  */
  off_t cur_off = SARMAG;

  if (fd == -1)
    {
      if (!suppress_create_msg)
	fprintf (stderr, "%s: creating %s\n",
		 program_invocation_short_name, arfname);

      goto no_old;
    }

  /* Store the names of all files from the command line in a hash
     table so that we can match it.  Note that when no file name is
     given we are basically doing nothing except recreating the
     index.  */
  if (oper != oper_qappend)
    {
      if (hcreate (2 * argc) == 0)
	error (EXIT_FAILURE, errno, gettext ("cannot create hash table"));

      for (int cnt = 0; cnt < argc; ++cnt)
	{
	  ENTRY entry;
	  entry.key = full_path ? argv[cnt] : base_name (argv[cnt]);
	  entry.data = &argv[cnt];
	  if (hsearch (entry, ENTER) == NULL)
	    error (EXIT_FAILURE, errno,
		   gettext ("cannot insert into hash table"));
	}
    }

  /* While iterating over the current content of the archive we must
     determine a number of things: which archive members to keep,
     which are replaced, and where to insert the new members.  */
  Elf_Cmd cmd = ELF_C_READ_MMAP;
  Elf *subelf;
  while ((subelf = elf_begin (fd, cmd, elf)) != NULL)
    {
      Elf_Arhdr *arhdr = elf_getarhdr (subelf);

      /* Ignore the symbol table and the long file name table here.  */
      if (strcmp (arhdr->ar_name, "/") == 0
	  || strcmp (arhdr->ar_name, "//") == 0)
	goto next;

      struct armem *newp = alloca (sizeof (struct armem));
      newp->old_off = elf_getaroff (subelf);
      newp->size = arhdr->ar_size;
      newp->sec = arhdr->ar_date;
      newp->mem = NULL;

      /* Remember long file names.  */
      remember_long_name (newp, arhdr->ar_name, strlen (arhdr->ar_name));

      /* Check whether this is a file we are looking for.  */
      if (oper != oper_qappend)
	{
	  /* Check whether this is the member used as the insert point.  */
	  if (member != NULL && strcmp (arhdr->ar_name, member) == 0)
	    {
	      /* Note that all == NULL means insert at the beginning.  */
	      if (ipos == ipos_before)
		after_memberelem = all;
	      else
		after_memberelem = newp;
	      member = NULL;
	    }

	  ENTRY entry;
	  entry.key = arhdr->ar_name;
	  ENTRY *res = hsearch (entry, FIND);
	  if (res != NULL && found[(char **) res->data - argv] == NULL)
	    {
	      found[(char **) res->data - argv] = newp;

	      /* If we insert before or after a certain element move
		 all files to a special list.  */
	      if (unlikely (ipos != ipos_none || oper == oper_move))
		{
		  if (after_memberelem == newp)
		    /* Since we remove this element even though we should
		       insert everything after it, we in fact insert
		       everything after the previous element.  */
		    after_memberelem = all;

		  goto next;
		}
	    }
	}

      if (all == NULL)
	all = newp->next = newp;
      else
	{
	  newp->next = all->next;
	  all = all->next = newp;
	}

    next:
      cmd = elf_next (subelf);
      if (elf_end (subelf) != 0)
	error (EXIT_FAILURE, 0, "%s: %s", arfname, elf_errmsg (-1));
    }

  if (oper != oper_qappend)
    hdestroy ();

 no_old:
  if (member != NULL)
    error (EXIT_FAILURE, 0, gettext ("position member %s not found"),
	   member);

  if (oper == oper_move)
    {
      /* Make sure all requested elements are found in the archive.  */
      for (int cnt = 0; cnt < argc; ++cnt)
	{
	  if (found[cnt] == NULL)
	    {
	      fprintf (stderr, gettext ("%s: no entry %s in archive!\n"),
		       program_invocation_short_name, argv[cnt]);
	      status = 1;
	    }

	  if (verbose)
	    printf ("m - %s\n", argv[cnt]);
	}
    }
  else
    {
      /* Open all the new files, get their sizes and add all symbols.  */
      for (int cnt = 0; cnt < argc; ++cnt)
	{
	  const char *bname = base_name (argv[cnt]);
	  size_t bnamelen = strlen (bname);
	  if (found[cnt] == NULL)
	    {
	      found[cnt] = alloca (sizeof (struct armem));
	      found[cnt]->old_off = -1;

	      remember_long_name (found[cnt], bname, bnamelen);
	    }

	  struct stat newst;
	  Elf *newelf;
	  int newfd = open (argv[cnt], O_RDONLY);
	  if (newfd == -1)
	    {
	      error (0, errno, gettext ("cannot open %s"), argv[cnt]);
	      status = 1;
	    }
	  else if (fstat (newfd, &newst) == -1)
	    {
	      error (0, errno, gettext ("cannot stat %s"), argv[cnt]);
	      close (newfd);
	      status = 1;
	    }
	  else if (!S_ISREG (newst.st_mode))
	    {
	      error (0, errno, gettext ("%s is no regular file"), argv[cnt]);
	      close (newfd);
	      status = 1;
	    }
	  else if (update_newer
		   && found[cnt]->old_off != -1l
		   && found[cnt]->sec > st.st_mtime)
	    /* Do nothing, the file in the archive is younger.  */
	    close (newfd);
	  else if ((newelf = elf_begin (newfd, ELF_C_READ_MMAP, NULL))
		   == NULL)
	    {
	      fprintf (stderr,
		       gettext ("cannot get ELF descriptor for %s: %s\n"),
		       argv[cnt], elf_errmsg (-1));
	      status = 1;
	    }
	  else
	    {
	      if (verbose)
		printf ("%c - %s\n",
			found[cnt]->old_off == -1l ? 'a' : 'r', argv[cnt]);

	      found[cnt]->elf = newelf;
	      found[cnt]->sec = arlib_deterministic_output ? 0 : newst.st_mtime;
	      found[cnt]->uid = arlib_deterministic_output ? 0 : newst.st_uid;
	      found[cnt]->gid = arlib_deterministic_output ? 0 : newst.st_gid;
	      found[cnt]->mode = newst.st_mode;
	      found[cnt]->name = bname;

	      found[cnt]->mem = elf_rawfile (newelf, &found[cnt]->size);
	      if (found[cnt]->mem == NULL
		  || elf_cntl (newelf, ELF_C_FDDONE) != 0)
		error (EXIT_FAILURE, 0, gettext ("cannot read %s: %s"),
		       argv[cnt], elf_errmsg (-1));

	      close (newfd);

	      if (found[cnt]->old_off != -1l)
		/* Remember long file names.  */
		remember_long_name (found[cnt], bname, bnamelen);
	    }
	}
    }

  if (status != 0)
    {
      elf_end (elf);

      arlib_fini ();

      close (fd);

      return status;
    }

  /* If we have no entry point so far add at the end.  AFTER_MEMBERELEM
     being NULL when adding before an entry means add at the beginning.  */
  if (ipos != ipos_before && after_memberelem == NULL)
    after_memberelem = all;

  /* Convert the circular list into a normal list first.  */
  if (all != NULL)
    {
      struct armem *tmp = all;
      all = all->next;
      tmp->next = NULL;
    }

  struct armem *last_added = after_memberelem;
  for (int cnt = 0; cnt < argc; ++cnt)
    if (oper != oper_replace || found[cnt]->old_off == -1)
      {
	if (last_added == NULL)
	  {
	    found[cnt]->next = all;
	    last_added = all = found[cnt];
	  }
	else
	  {
	    found[cnt]->next = last_added->next;
	    last_added = last_added->next = found[cnt];
	  }
      }

  /* Finally compute the offset and add the symbols for the files
     after the insert point.  */
  if (likely (all != NULL))
    for (struct armem *memp = all; memp != NULL; memp = memp->next)
      {
	memp->off = cur_off;

	if (memp->mem == NULL)
	  {
	    Elf_Arhdr *arhdr;
	    /* Fake initializing arhdr and subelf to keep gcc calm.  */
	    asm ("" : "=m" (arhdr), "=m" (subelf));
	    if (elf_rand (elf, memp->old_off) == 0
		|| (subelf = elf_begin (fd, ELF_C_READ_MMAP, elf)) == NULL
		|| (arhdr = elf_getarhdr (subelf)) == NULL)
	      /* This should never happen since we already looked at the
		 archive content.  But who knows...  */
	      error (EXIT_FAILURE, 0, "%s: %s", arfname, elf_errmsg (-1));

	    arlib_add_symbols (subelf, arfname, arhdr->ar_name, cur_off);

	    elf_end (subelf);
	  }
	else
	  arlib_add_symbols (memp->elf, arfname, memp->name, cur_off);

	cur_off += (((memp->size + 1) & ~((off_t) 1))
		    + sizeof (struct ar_hdr));
      }

  /* Now we have all the information for the symbol table and long
     file name table.  Construct the final layout.  */
  arlib_finalize ();

  /* Create a new, temporary file in the same directory as the
     original file.  */
  char tmpfname[strlen (arfname) + 7];
  strcpy (stpcpy (tmpfname, arfname), "XXXXXX");
  int newfd;
  if (fd != -1)
    newfd = mkstemp (tmpfname);
  else
    {
      newfd = open (arfname, O_RDWR | O_CREAT | O_EXCL, DEFFILEMODE);
      if (newfd == -1 && errno == EEXIST)
	/* Bah, first the file did not exist, now it does.  Restart.  */
	return do_oper_insert (oper, arfname, argv, argc, member);
    }
  if (unlikely (newfd == -1))
    goto nonew;

  /* Create the header.  */
  if (unlikely (write_retry (newfd, ARMAG, SARMAG) != SARMAG))
    {
    nonew_unlink:
      if (fd != -1)
	{
	  // XXX Use /prof/self/fd/%d ???
	  unlink (tmpfname);
	  if (newfd != -1)
	    close (newfd);
	}
    nonew:
      error (0, errno, gettext ("cannot create new file"));
      status = 1;
      goto errout;
    }

  /* If the new archive is not empty we actually have something to do.  */
  if (likely (all != NULL))
    {
      /* Write the symbol table or the long file name table or both.  */
      if (symtab.symsnamelen != 0
	  && ((write_retry (newfd, symtab.symsoff, symtab.symsofflen)
	       != (ssize_t) symtab.symsofflen)
	      || (write_retry (newfd, symtab.symsname, symtab.symsnamelen)
		  != (ssize_t) symtab.symsnamelen)))
	goto nonew_unlink;

      if (symtab.longnameslen > sizeof (struct ar_hdr)
	  && (write_retry (newfd, symtab.longnames, symtab.longnameslen)
	      != (ssize_t) symtab.longnameslen))
	goto nonew_unlink;

      off_t start = -1;
      off_t len = -1;

      while (all != NULL)
	{
	  if (all->mem != NULL)
	    {
	      /* This is a new file.  If there is anything from the
		 archive left to be written do it now.  */
	      if (start != -1  && copy_content (elf, newfd, start, len))
		goto nonew_unlink;

	      start = -1;
	      len = -1;

	      /* Create the header.  */
	      struct ar_hdr arhdr;
	      /* The ar_name is not actually zero teminated, but we
		 need that for snprintf.  Also if the name is too
		 long, then the string starts with '/' plus an index
		 off number (decimal).  */
	      char tmpbuf[sizeof (arhdr.ar_name) + 2];
	      if (all->long_name_off == -1)
		{
		  size_t namelen = strlen (all->name);
		  char *p = mempcpy (arhdr.ar_name, all->name, namelen);
		  *p++ = '/';
		  memset (p, ' ', sizeof (arhdr.ar_name) - namelen - 1);
		}
	      else
		{
		  snprintf (tmpbuf, sizeof (tmpbuf), "/%-*ld",
			    (int) sizeof (arhdr.ar_name), all->long_name_off);
		  memcpy (arhdr.ar_name, tmpbuf, sizeof (arhdr.ar_name));
		}

	      if (! no0print (false, arhdr.ar_date, sizeof (arhdr.ar_date),
			      all->sec))
		{
		  error (0, errno, gettext ("cannot represent ar_date"));
		  goto nonew_unlink;
		}
	      if (! no0print (false, arhdr.ar_uid, sizeof (arhdr.ar_uid),
			      all->uid))
		{
		  error (0, errno, gettext ("cannot represent ar_uid"));
		  goto nonew_unlink;
		}
	      if (! no0print (false, arhdr.ar_gid, sizeof (arhdr.ar_gid),
			      all->gid))
		{
		  error (0, errno, gettext ("cannot represent ar_gid"));
		  goto nonew_unlink;
		}
	      if (! no0print (true, arhdr.ar_mode, sizeof (arhdr.ar_mode),
			all->mode))
		{
		  error (0, errno, gettext ("cannot represent ar_mode"));
		  goto nonew_unlink;
		}
	      if (! no0print (false, arhdr.ar_size, sizeof (arhdr.ar_size),
			all->size))
		{
		  error (0, errno, gettext ("cannot represent ar_size"));
		  goto nonew_unlink;
		}
	      memcpy (arhdr.ar_fmag, ARFMAG, sizeof (arhdr.ar_fmag));

	      if (unlikely (write_retry (newfd, &arhdr, sizeof (arhdr))
			    != sizeof (arhdr)))
		goto nonew_unlink;

	      /* Now the file itself.  */
	      if (unlikely (write_retry (newfd, all->mem, all->size)
			    != (off_t) all->size))
		goto nonew_unlink;

	      /* Pad the file if its size is odd.  */
	      if ((all->size & 1) != 0)
		if (unlikely (write_retry (newfd, "\n", 1) != 1))
		  goto nonew_unlink;
	    }
	  else
	    {
	      /* This is a member from the archive.  */
	      if (write_member (all, &start, &len, elf, cur_off, newfd)
		  != 0)
		goto nonew_unlink;
	    }

	  all = all->next;
	}

      /* Write the last part.  */
      if (start != -1 && copy_content (elf, newfd, start, len))
	goto nonew_unlink;
    }

  /* Set the mode of the new file to the same values the original file
     has.  */
  if (fd != -1
      && (fchmod (newfd, st.st_mode & ALLPERMS) != 0
	  /* Never complain about fchown failing.  */
	  || (({asm ("" :: "r" (fchown (newfd, st.st_uid, st.st_gid))); }),
	      close (newfd) != 0)
	  || (newfd = -1, rename (tmpfname, arfname) != 0)))
      goto nonew_unlink;

 errout:
  for (int cnt = 0; cnt < argc; ++cnt)
    elf_end (found[cnt]->elf);

  elf_end (elf);

  arlib_fini ();

  if (fd != -1)
    close (fd);

  return status;
}


#include "debugpred.h"
