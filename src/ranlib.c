/* Generate an index to speed access to archives.
   Copyright (C) 2005, 2006, 2007 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#include <ar.h>
#include <argp.h>
#include <assert.h>
#include <byteswap.h>
#include <endian.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <libintl.h>
#include <locale.h>
#include <mcheck.h>
#include <obstack.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <system.h>


#if __BYTE_ORDER == __LITTLE_ENDIAN
# define le_bswap_32(val) bswap_32 (val)
#else
# define le_bswap_32(val) (val)
#endif


/* Prototypes for local functions.  */
static int handle_file (const char *fname);


/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/* Bug report address.  */
const char *argp_program_bug_address = PACKAGE_BUGREPORT;


/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("Generate an index to speed access to archives.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("ARCHIVE");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, NULL, NULL
};


int
main (int argc, char *argv[])
{
  /* Make memory leak detection possible.  */
  mtrace ();

  /* We use no threads here which can interfere with handling a stream.  */
  (void) __fsetlocking (stdin, FSETLOCKING_BYCALLER);
  (void) __fsetlocking (stdout, FSETLOCKING_BYCALLER);
  (void) __fsetlocking (stderr, FSETLOCKING_BYCALLER);

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  /* Make sure the message catalog can be found.  */
  (void) bindtextdomain (PACKAGE, LOCALEDIR);

  /* Initialize the message catalog.  */
  (void) textdomain (PACKAGE);

  /* Parse and process arguments.  */
  int remaining;
  (void) argp_parse (&argp, argc, argv, ARGP_IN_ORDER, &remaining, NULL);

  /* Tell the library which version we are expecting.  */
  (void) elf_version (EV_CURRENT);

  /* There must at least be one more parameter specifying the archive.   */
  if (remaining == argc)
    {
      error (0, 0, gettext ("Archive name required"));
      argp_help (&argp, stderr, ARGP_HELP_SEE, "ranlib");
      exit (EXIT_FAILURE);
    }

  /* We accept the names of multiple archives.  */
  int status = 0;
  do
    status |= handle_file (argv[remaining]);
  while (++remaining < argc);

  return status;
}


/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "ranlib (%s) %s\n", PACKAGE_NAME, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Red Hat, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2007");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
	   struct argp_state *state __attribute__ ((unused)))
{
  switch (key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static struct obstack offsob;
size_t offs_len;
static struct obstack namesob;
size_t names_len;


/* Add all exported, defined symbols from the given section to the table.  */
static void
add_symbols (Elf *elf, const char *fname, off_t off, Elf_Scn *scn,
	     GElf_Shdr *shdr)
{
  if (sizeof (off) > sizeof (uint32_t) && off > ~((uint32_t) 0))
    /* The archive is too big.  */
    error (EXIT_FAILURE, 0, gettext ("the archive '%s' is too large"), fname);

  Elf_Data *data = elf_getdata (scn, NULL);
  assert (data->d_size == shdr->sh_size);

  /* We can skip the local symbols in the table.  */
  for (int cnt = shdr->sh_info; cnt < (int) (shdr->sh_size / shdr->sh_entsize);
       ++cnt)
    {
      GElf_Sym sym_mem;
      GElf_Sym *sym = gelf_getsym (data, cnt, &sym_mem);
      if (sym == NULL)
	/* Should never happen.  */
	continue;

      /* Ignore undefined symbols.  */
      if (sym->st_shndx == SHN_UNDEF)
	continue;

      /* For all supported platforms the following is true.  */
      assert (sizeof (uint32_t) == sizeof (int));
      obstack_int_grow (&offsob, (int) le_bswap_32 (off));
      offs_len += sizeof (uint32_t);

      const char *symname = elf_strptr (elf, shdr->sh_link, sym->st_name);
      size_t symname_len = strlen (symname) + 1;
      obstack_grow (&namesob, symname, symname_len);
      names_len += symname_len;
    }
}


/* Look through ELF file and collect all symbols available for
   linking.  If available, we use the dynamic symbol section.
   Otherwise the normal one.  Relocatable files are allowed to have
   multiple symbol tables.  */
static void
handle_elf (Elf *elf, const char *arfname, off_t off)
{
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
  assert (ehdr != NULL);

  if (ehdr->e_type == ET_REL)
    {
      Elf_Scn *scn = NULL;
      while ((scn = elf_nextscn (elf, scn)) != NULL)
	{
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr != NULL && shdr->sh_type == SHT_SYMTAB)
	    add_symbols (elf, arfname, off, scn, shdr);
	}
    }
  else
    {
      Elf_Scn *symscn = NULL;
      GElf_Shdr *symshdr = NULL;
      Elf_Scn *scn = NULL;
      GElf_Shdr shdr_mem;
      while ((scn = elf_nextscn (elf, scn)) != NULL)
	{
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  symshdr = NULL;
	  if (shdr != NULL)
	    {
	      if (shdr->sh_type == SHT_DYNSYM)
		{
		  symscn = scn;
		  symshdr = shdr;
		  break;
		}
	      if (shdr->sh_type == SHT_SYMTAB)
		{
		  /* There better be only one symbol table in
		     executables in DSOs.  */
		  assert (symscn == NULL);
		  symscn = scn;
		  symshdr = shdr;
		}
	    }
	}

      add_symbols (elf, arfname, off, symscn,
		   symshdr ?: gelf_getshdr (scn, &shdr_mem));
    }
}


static int
copy_content (int oldfd, int newfd, off_t off, size_t n)
{
  while (n > 0)
    {
      char buf[32768];

      ssize_t nread = pread_retry (oldfd, buf, MIN (sizeof (buf), n), off);
      if (unlikely (nread <= 0))
	return 1;

      if (write_retry (newfd, buf, nread) != nread)
	return 1;

      n -= nread;
      off += nread;
    }

  return 0;
}


/* Handle a file given on the command line.  */
static int
handle_file (const char *fname)
{
  int fd = open (fname, O_RDONLY);
  if (fd == -1)
    {
      error (0, errno, gettext ("cannot open '%s'"), fname);
      return 1;
    }

  struct stat st;
  if (fstat (fd, &st) != 0)
    {
      error (0, errno, gettext ("cannot stat '%s'"), fname);
      close (fd);
      return 1;
    }

  /* First we walk through the file, looking for all ELF files to
     collect symbols from.  */
  Elf *arelf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
  if (arelf == NULL)
    {
      error (0, 0, gettext ("cannot create ELF descriptor for '%s': %s"),
	     fname, elf_errmsg (-1));
      close (fd);
      return 1;
    }

  if (elf_kind (arelf) != ELF_K_AR)
    {
      error (0, 0, gettext ("'%s' is no archive"), fname);
      elf_end (arelf);
      close (fd);
      return 1;
    }

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
  obstack_init (&offsob);
  offs_len = 0;
  obstack_init (&namesob);
  names_len = 0;

  /* The first word in the offset table specifies the size.  Create
     such an entry now.  The real value will be filled-in later.  For
     all supported platforms the following is true.  */
  assert (sizeof (uint32_t) == sizeof (int));
  obstack_int_grow (&offsob, 0);
  offs_len += sizeof (uint32_t);

  /* Iterate over the content of the archive.  */
  off_t index_off = -1;
  size_t index_size = 0;
  Elf *elf;
  Elf_Cmd cmd = ELF_C_READ_MMAP;
  while ((elf = elf_begin (fd, cmd, arelf)) != NULL)
    {
      Elf_Arhdr *arhdr = elf_getarhdr (elf);
      assert (arhdr != NULL);
      off_t off = elf_getaroff (elf);

      Elf_Kind kind = elf_kind (elf);
      if (kind == ELF_K_ELF)
	handle_elf (elf, fname, off);
#if 0
      else if (kind == ELF_K_AR)
	{
	  // XXX Should we implement this?
	}
#endif
      /* If this is the index, remember the location.  */
      else if (strcmp (arhdr->ar_name, "/") == 0)
	{
	  index_off = off;
	  index_size = arhdr->ar_size;
	}

      /* Get next archive element.  */
      cmd = elf_next (elf);
      if (elf_end (elf) != 0)
	error (0, 0,
	       gettext (" error while freeing sub-ELF descriptor: %s\n"),
	       elf_errmsg (-1));
    }

  elf_end (arelf);
  uint32_t *offs_arr = obstack_finish (&offsob);
  assert (offs_len % sizeof (uint32_t) == 0);
  if ((names_len & 1) != 0)
    {
      /* Add one more NUL byte to make length even.  */
      obstack_grow (&namesob, "", 1);
      ++names_len;
    }
  const char *names_str = obstack_finish (&namesob);

  /* If the file contains no symbols we need not do anything.  */
  if (names_len != 0
      /* We have to rewrite the file also if it initially had an index
	 but now does not need one anymore.  */
      || (names_len == 0 && index_off != -1))
    {
      /* Create a new, temporary file in the same directory as the
	 original file.  */
      char tmpfname[strlen (fname) + 8];
      strcpy (stpcpy (tmpfname, fname), ".XXXXXX");
      int newfd = mkstemp (tmpfname);
      if (newfd == -1)
      nonew:
	error (0, errno, gettext ("cannot create new file"));
      else
	{
	  /* Create the header.  */
	  if (write_retry (newfd, ARMAG, SARMAG) != SARMAG)
	    {
	      // XXX Use /prof/self/fd/%d ???
	    nonew_unlink:
	      unlink (tmpfname);
	      if (newfd != -1)
		close (newfd);
	      goto nonew;
	    }

	  struct ar_hdr ar_hdr;
	  memcpy (ar_hdr.ar_name, "/               ", sizeof (ar_hdr.ar_name));
	  /* Using snprintf here has a problem: the call always wants
	     to add a NUL byte.  We could use a trick whereby we
	     specify the target buffer size longer than it is and this
	     would not actually fail, since all the fields are
	     consecutive and we fill them in in sequence (i.e., the
	     NUL byte gets overwritten).  But _FORTIFY_SOURCE=2 would
	     not let us play these games.  Therefore we play it
	     safe.  */
	  char tmpbuf[MAX (sizeof (ar_hdr.ar_date), sizeof (ar_hdr.ar_size))
		      + 1];
	  memcpy (ar_hdr.ar_date, tmpbuf,
		  snprintf (tmpbuf, sizeof (tmpbuf), "%-*lld",
			    (int) sizeof (ar_hdr.ar_date),
			    (long long int) time (NULL)));

	  /* Note the string for the ar_uid and ar_gid cases is longer
	     than necessary.  This does not matter since we copy only as
	     much as necessary but it helps the compiler to use the same
	     string for the ar_mode case.  */
	  memcpy (ar_hdr.ar_uid, "0       ", sizeof (ar_hdr.ar_uid));
	  memcpy (ar_hdr.ar_gid, "0       ", sizeof (ar_hdr.ar_gid));
	  memcpy (ar_hdr.ar_mode, "0       ", sizeof (ar_hdr.ar_mode));

	  /* See comment for ar_date above.  */
	  memcpy (ar_hdr.ar_size, tmpbuf,
		  snprintf (tmpbuf, sizeof (tmpbuf), "%-*zu",
			    (int) sizeof (ar_hdr.ar_size),
			    offs_len + names_len));
	  memcpy (ar_hdr.ar_fmag, ARFMAG, sizeof (ar_hdr.ar_fmag));

	  /* Fill in the number of offsets now.  */
	  offs_arr[0] = le_bswap_32 (offs_len / sizeof (uint32_t) - 1);

	  /* Adjust the offset values for the name index size (if
	     necessary).  */
	  off_t disp = (offs_len + ((names_len + 1) & ~1ul)
			- ((index_size + 1) & ~1ul));
	  /* If there was no index so far but one is needed now we
	     have to take the archive header into account.  */
	  if (index_off == -1 && names_len != 0)
	    disp += sizeof (struct ar_hdr);
	  if (disp != 0)
	    for (size_t cnt = 1; cnt < offs_len / sizeof (uint32_t); ++cnt)
	      {
		uint32_t val;
		val = le_bswap_32 (offs_arr[cnt]);

		if (val > index_off)
		  {
		    val += disp;
		    offs_arr[cnt] = le_bswap_32 (val);
		  }
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

	  if ((index_off > SARMAG
	       && copy_content (fd, newfd, SARMAG, index_off - SARMAG))
	      || (names_len != 0
		  && ((write_retry (newfd, &ar_hdr, sizeof (ar_hdr))
		       != sizeof (ar_hdr))
		      || (write_retry (newfd, offs_arr, offs_len)
			  != (ssize_t) offs_len)
		      || (write_retry (newfd, names_str, names_len)
			  != (ssize_t) names_len)))
	      || copy_content (fd, newfd, rest_off, st.st_size - rest_off)
	      /* Set the mode of the new file to the same values the
		 original file has.  */
	      || fchmod (newfd, st.st_mode & ALLPERMS) != 0
	      || fchown (newfd, st.st_uid, st.st_gid) != 0
	      || close (newfd) != 0
	      || (newfd = -1, rename (tmpfname, fname) != 0))
	    goto nonew_unlink;
	}
    }

  obstack_free (&offsob, NULL);
  obstack_free (&namesob, NULL);

  close (fd);

  return 0;
}
