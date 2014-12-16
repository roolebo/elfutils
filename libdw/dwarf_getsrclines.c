/* Return line number information of CU.
   Copyright (C) 2004-2010, 2013, 2014 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>

#include "dwarf.h"
#include "libdwP.h"


struct filelist
{
  Dwarf_Fileinfo info;
  struct filelist *next;
};

struct linelist
{
  Dwarf_Line line;
  struct linelist *next;
  size_t sequence;
};


/* Compare by Dwarf_Line.addr, given pointers into an array of pointers.  */
static int
compare_lines (const void *a, const void *b)
{
  struct linelist *const *p1 = a;
  struct linelist *const *p2 = b;
  struct linelist *list1 = *p1;
  struct linelist *list2 = *p2;
  Dwarf_Line *line1 = &list1->line;
  Dwarf_Line *line2 = &list2->line;

  if (line1->addr != line2->addr)
    return (line1->addr < line2->addr) ? -1 : 1;

  /* An end_sequence marker precedes a normal record at the same address.  */
  if (line1->end_sequence != line2->end_sequence)
    return line2->end_sequence - line1->end_sequence;

  /* Otherwise, the linelist sequence maintains a stable sort.  */
  return (list1->sequence < list2->sequence) ? -1
    : (list1->sequence > list2->sequence) ? 1
    : 0;
}

static int
read_srclines (Dwarf *dbg,
	       const unsigned char *linep, const unsigned char *lineendp,
	       const char *comp_dir, unsigned address_size,
	       Dwarf_Lines **linesp, Dwarf_Files **filesp)
{
  int res = -1;

  struct linelist *linelist = NULL;
  size_t nlinelist = 0;

  /* If there are a large number of lines don't blow up the stack.
     Keep track of the last malloced linelist record and free them
     through the next pointer at the end.  */
#define MAX_STACK_ALLOC 4096
  struct linelist *malloc_linelist = NULL;

  if (unlikely (linep + 4 > lineendp))
    {
    invalid_data:
      __libdw_seterrno (DWARF_E_INVALID_DEBUG_LINE);
      goto out;
    }

  Dwarf_Word unit_length = read_4ubyte_unaligned_inc (dbg, linep);
  unsigned int length = 4;
  if (unlikely (unit_length == DWARF3_LENGTH_64_BIT))
    {
      if (unlikely (linep + 8 > lineendp))
	goto invalid_data;
      unit_length = read_8ubyte_unaligned_inc (dbg, linep);
      length = 8;
    }

  /* Check whether we have enough room in the section.  */
  if (unlikely (unit_length > (size_t) (lineendp - linep)
      || unit_length < 2 + length + 5 * 1))
    goto invalid_data;
  lineendp = linep + unit_length;

  /* The next element of the header is the version identifier.  */
  uint_fast16_t version = read_2ubyte_unaligned_inc (dbg, linep);
  if (unlikely (version < 2) || unlikely (version > 4))
    {
      __libdw_seterrno (DWARF_E_VERSION);
      goto out;
    }

  /* Next comes the header length.  */
  Dwarf_Word header_length;
  if (length == 4)
    header_length = read_4ubyte_unaligned_inc (dbg, linep);
  else
    header_length = read_8ubyte_unaligned_inc (dbg, linep);
  const unsigned char *header_start = linep;

  /* Next the minimum instruction length.  */
  uint_fast8_t minimum_instr_len = *linep++;

  /* Next the maximum operations per instruction, in version 4 format.  */
  uint_fast8_t max_ops_per_instr = 1;
  if (version >= 4)
    {
      if (unlikely (lineendp - linep < 5))
	goto invalid_data;
      max_ops_per_instr = *linep++;
      if (unlikely (max_ops_per_instr == 0))
	goto invalid_data;
    }

  /* Then the flag determining the default value of the is_stmt
     register.  */
  uint_fast8_t default_is_stmt = *linep++;

  /* Now the line base.  */
  int_fast8_t line_base = (int8_t) *linep++;

  /* And the line range.  */
  uint_fast8_t line_range = *linep++;

  /* The opcode base.  */
  uint_fast8_t opcode_base = *linep++;

  /* Remember array with the standard opcode length (-1 to account for
     the opcode with value zero not being mentioned).  */
  const uint8_t *standard_opcode_lengths = linep - 1;
  if (unlikely (lineendp - linep < opcode_base - 1))
    goto invalid_data;
  linep += opcode_base - 1;

  /* First comes the list of directories.  Add the compilation
     directory first since the index zero is used for it.  */
  struct dirlist
  {
    const char *dir;
    size_t len;
    struct dirlist *next;
  } comp_dir_elem =
    {
      .dir = comp_dir,
      .len = comp_dir ? strlen (comp_dir) : 0,
      .next = NULL
    };
  struct dirlist *dirlist = &comp_dir_elem;
  unsigned int ndirlist = 1;

  // XXX Directly construct array to conserve memory?
  while (*linep != 0)
    {
      struct dirlist *new_dir =
	(struct dirlist *) alloca (sizeof (*new_dir));

      new_dir->dir = (char *) linep;
      uint8_t *endp = memchr (linep, '\0', lineendp - linep);
      if (endp == NULL)
	goto invalid_data;
      new_dir->len = endp - linep;
      new_dir->next = dirlist;
      dirlist = new_dir;
      ++ndirlist;
      linep = endp + 1;
    }
  /* Skip the final NUL byte.  */
  ++linep;

  /* Rearrange the list in array form.  */
  struct dirlist **dirarray
    = (struct dirlist **) alloca (ndirlist * sizeof (*dirarray));
  for (unsigned int n = ndirlist; n-- > 0; dirlist = dirlist->next)
    dirarray[n] = dirlist;

  /* Now read the files.  */
  struct filelist null_file =
    {
      .info =
      {
	.name = "???",
	.mtime = 0,
	.length = 0
      },
      .next = NULL
    };
  struct filelist *filelist = &null_file;
  unsigned int nfilelist = 1;

  if (unlikely (linep >= lineendp))
    goto invalid_data;
  while (*linep != 0)
    {
      struct filelist *new_file =
	(struct filelist *) alloca (sizeof (*new_file));

      /* First comes the file name.  */
      char *fname = (char *) linep;
      uint8_t *endp = memchr (fname, '\0', lineendp - linep);
      if (endp == NULL)
	goto invalid_data;
      size_t fnamelen = endp - (uint8_t *) fname;
      linep = endp + 1;

      /* Then the index.  */
      Dwarf_Word diridx;
      if (unlikely (linep >= lineendp))
	goto invalid_data;
      get_uleb128 (diridx, linep, lineendp);
      if (unlikely (diridx >= ndirlist))
	{
	  __libdw_seterrno (DWARF_E_INVALID_DIR_IDX);
	  goto out;
	}

      if (*fname == '/')
	/* It's an absolute path.  */
	new_file->info.name = fname;
      else
	{
	  new_file->info.name = libdw_alloc (dbg, char, 1,
					     dirarray[diridx]->len + 1
					     + fnamelen + 1);
	  char *cp = new_file->info.name;

	  if (dirarray[diridx]->dir != NULL)
	    {
	      /* This value could be NULL in case the DW_AT_comp_dir
		 was not present.  We cannot do much in this case.
		 The easiest thing is to convert the path in an
		 absolute path.  */
	      cp = stpcpy (cp, dirarray[diridx]->dir);
	    }
	  *cp++ = '/';
	  strcpy (cp, fname);
	  assert (strlen (new_file->info.name)
		  < dirarray[diridx]->len + 1 + fnamelen + 1);
	}

      /* Next comes the modification time.  */
      if (unlikely (linep >= lineendp))
	goto invalid_data;
      get_uleb128 (new_file->info.mtime, linep, lineendp);

      /* Finally the length of the file.  */
      if (unlikely (linep >= lineendp))
	goto invalid_data;
      get_uleb128 (new_file->info.length, linep, lineendp);

      new_file->next = filelist;
      filelist = new_file;
      ++nfilelist;
    }
  /* Skip the final NUL byte.  */
  ++linep;

  /* Consistency check.  */
  if (unlikely (linep != header_start + header_length))
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      goto out;
    }

  /* We are about to process the statement program.  Initialize the
     state machine registers (see 6.2.2 in the v2.1 specification).  */
  Dwarf_Word addr = 0;
  unsigned int op_index = 0;
  unsigned int file = 1;
  int line = 1;
  unsigned int column = 0;
  uint_fast8_t is_stmt = default_is_stmt;
  bool basic_block = false;
  bool prologue_end = false;
  bool epilogue_begin = false;
  unsigned int isa = 0;
  unsigned int discriminator = 0;

  /* Apply the "operation advance" from a special opcode or
     DW_LNS_advance_pc (as per DWARF4 6.2.5.1).  */
  inline void advance_pc (unsigned int op_advance)
  {
    addr += minimum_instr_len * ((op_index + op_advance)
				 / max_ops_per_instr);
    op_index = (op_index + op_advance) % max_ops_per_instr;
  }

  /* Process the instructions.  */

  /* Adds a new line to the matrix.
     We cannot simply define a function because we want to use alloca.  */
#define NEW_LINE(end_seq)						\
  do {								\
    struct linelist *ll = (nlinelist < MAX_STACK_ALLOC		\
			   ? alloca (sizeof (struct linelist))	\
			   : malloc (sizeof (struct linelist)));	\
    if (nlinelist >= MAX_STACK_ALLOC)				\
      malloc_linelist = ll;						\
    if (unlikely (add_new_line (ll, end_seq)))			\
      goto invalid_data;						\
  } while (0)

  inline bool add_new_line (struct linelist *new_line, bool end_sequence)
  {
    new_line->next = linelist;
    new_line->sequence = nlinelist;
    linelist = new_line;
    ++nlinelist;

    /* Set the line information.  For some fields we use bitfields,
       so we would lose information if the encoded values are too large.
       Check just for paranoia, and call the data "invalid" if it
       violates our assumptions on reasonable limits for the values.  */
#define SET(field)							      \
    do {								      \
      new_line->line.field = field;					      \
      if (unlikely (new_line->line.field != field))			      \
	return true;						      \
    } while (0)

    SET (addr);
    SET (op_index);
    SET (file);
    SET (line);
    SET (column);
    SET (is_stmt);
    SET (basic_block);
    SET (end_sequence);
    SET (prologue_end);
    SET (epilogue_begin);
    SET (isa);
    SET (discriminator);

#undef SET

    return false;
  }

  while (linep < lineendp)
    {
      unsigned int opcode;
      unsigned int u128;
      int s128;

      /* Read the opcode.  */
      opcode = *linep++;

      /* Is this a special opcode?  */
      if (likely (opcode >= opcode_base))
	{
	  if (unlikely (line_range == 0))
	    goto invalid_data;

	  /* Yes.  Handling this is quite easy since the opcode value
	     is computed with

	     opcode = (desired line increment - line_base)
		       + (line_range * address advance) + opcode_base
	  */
	  int line_increment = (line_base
				+ (opcode - opcode_base) % line_range);

	  /* Perform the increments.  */
	  line += line_increment;
	  advance_pc ((opcode - opcode_base) / line_range);

	  /* Add a new line with the current state machine values.  */
	  NEW_LINE (0);

	  /* Reset the flags.  */
	  basic_block = false;
	  prologue_end = false;
	  epilogue_begin = false;
	  discriminator = 0;
	}
      else if (opcode == 0)
	{
	  /* This an extended opcode.  */
	  if (unlikely (lineendp - linep < 2))
	    goto invalid_data;

	  /* The length.  */
	  uint_fast8_t len = *linep++;

	  if (unlikely ((size_t) (lineendp - linep) < len))
	    goto invalid_data;

	  /* The sub-opcode.  */
	  opcode = *linep++;

	  switch (opcode)
	    {
	    case DW_LNE_end_sequence:
	      /* Add a new line with the current state machine values.
		 The is the end of the sequence.  */
	      NEW_LINE (1);

	      /* Reset the registers.  */
	      addr = 0;
	      op_index = 0;
	      file = 1;
	      line = 1;
	      column = 0;
	      is_stmt = default_is_stmt;
	      basic_block = false;
	      prologue_end = false;
	      epilogue_begin = false;
	      isa = 0;
	      discriminator = 0;
	      break;

	    case DW_LNE_set_address:
	      /* The value is an address.  The size is defined as
		 apporiate for the target machine.  We use the
		 address size field from the CU header.  */
	      op_index = 0;
	      if (unlikely (lineendp - linep < (uint8_t) address_size))
		goto invalid_data;
	      if (__libdw_read_address_inc (dbg, IDX_debug_line, &linep,
					    address_size, &addr))
		goto out;
	      break;

	    case DW_LNE_define_file:
	      {
		char *fname = (char *) linep;
		uint8_t *endp = memchr (linep, '\0', lineendp - linep);
		if (endp == NULL)
		  goto invalid_data;
		size_t fnamelen = endp - linep;
		linep = endp + 1;

		unsigned int diridx;
		if (unlikely (linep >= lineendp))
		  goto invalid_data;
		get_uleb128 (diridx, linep, lineendp);
		if (unlikely (diridx >= ndirlist))
		  {
		    __libdw_seterrno (DWARF_E_INVALID_DIR_IDX);
		    goto invalid_data;
		  }
		Dwarf_Word mtime;
		if (unlikely (linep >= lineendp))
		  goto invalid_data;
		get_uleb128 (mtime, linep, lineendp);
		Dwarf_Word filelength;
		if (unlikely (linep >= lineendp))
		  goto invalid_data;
		get_uleb128 (filelength, linep, lineendp);

		struct filelist *new_file =
		  (struct filelist *) alloca (sizeof (*new_file));
		if (fname[0] == '/')
		  new_file->info.name = fname;
		else
		  {
		    new_file->info.name =
		      libdw_alloc (dbg, char, 1, (dirarray[diridx]->len + 1
						  + fnamelen + 1));
		    char *cp = new_file->info.name;

		    if (dirarray[diridx]->dir != NULL)
		      /* This value could be NULL in case the
			 DW_AT_comp_dir was not present.  We
			 cannot do much in this case.  The easiest
			 thing is to convert the path in an
			 absolute path.  */
		      cp = stpcpy (cp, dirarray[diridx]->dir);
		    *cp++ = '/';
		    strcpy (cp, fname);
		  }

		new_file->info.mtime = mtime;
		new_file->info.length = filelength;
		new_file->next = filelist;
		filelist = new_file;
		++nfilelist;
	      }
	      break;

	    case DW_LNE_set_discriminator:
	      /* Takes one ULEB128 parameter, the discriminator.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 1))
		goto invalid_data;

	      if (unlikely (linep >= lineendp))
		goto invalid_data;
	      get_uleb128 (discriminator, linep, lineendp);
	      break;

	    default:
	      /* Unknown, ignore it.  */
	      if (unlikely ((size_t) (lineendp - (linep - 1)) < len))
		goto invalid_data;
	      linep += len - 1;
	      break;
	    }
	}
      else if (opcode <= DW_LNS_set_isa)
	{
	  /* This is a known standard opcode.  */
	  switch (opcode)
	    {
	    case DW_LNS_copy:
	      /* Takes no argument.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 0))
		goto invalid_data;

	      /* Add a new line with the current state machine values.  */
	      NEW_LINE (0);

	      /* Reset the flags.  */
	      basic_block = false;
	      prologue_end = false;
	      epilogue_begin = false;
	      discriminator = 0;
	      break;

	    case DW_LNS_advance_pc:
	      /* Takes one uleb128 parameter which is added to the
		 address.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 1))
		goto invalid_data;

	      if (unlikely (linep >= lineendp))
		goto invalid_data;
	      get_uleb128 (u128, linep, lineendp);
	      advance_pc (u128);
	      break;

	    case DW_LNS_advance_line:
	      /* Takes one sleb128 parameter which is added to the
		 line.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 1))
		goto invalid_data;

	      if (unlikely (linep >= lineendp))
		goto invalid_data;
	      get_sleb128 (s128, linep, lineendp);
	      line += s128;
	      break;

	    case DW_LNS_set_file:
	      /* Takes one uleb128 parameter which is stored in file.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 1))
		goto invalid_data;

	      if (unlikely (linep >= lineendp))
		goto invalid_data;
	      get_uleb128 (u128, linep, lineendp);
	      file = u128;
	      break;

	    case DW_LNS_set_column:
	      /* Takes one uleb128 parameter which is stored in column.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 1))
		goto invalid_data;

	      if (unlikely (linep >= lineendp))
		goto invalid_data;
	      get_uleb128 (u128, linep, lineendp);
	      column = u128;
	      break;

	    case DW_LNS_negate_stmt:
	      /* Takes no argument.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 0))
		goto invalid_data;

	      is_stmt = 1 - is_stmt;
	      break;

	    case DW_LNS_set_basic_block:
	      /* Takes no argument.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 0))
		goto invalid_data;

	      basic_block = true;
	      break;

	    case DW_LNS_const_add_pc:
	      /* Takes no argument.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 0))
		goto invalid_data;

	      if (unlikely (line_range == 0))
		goto invalid_data;

	      advance_pc ((255 - opcode_base) / line_range);
	      break;

	    case DW_LNS_fixed_advance_pc:
	      /* Takes one 16 bit parameter which is added to the
		 address.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 1)
		  || unlikely (lineendp - linep < 2))
		goto invalid_data;

	      addr += read_2ubyte_unaligned_inc (dbg, linep);
	      op_index = 0;
	      break;

	    case DW_LNS_set_prologue_end:
	      /* Takes no argument.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 0))
		goto invalid_data;

	      prologue_end = true;
	      break;

	    case DW_LNS_set_epilogue_begin:
	      /* Takes no argument.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 0))
		goto invalid_data;

	      epilogue_begin = true;
	      break;

	    case DW_LNS_set_isa:
	      /* Takes one uleb128 parameter which is stored in isa.  */
	      if (unlikely (standard_opcode_lengths[opcode] != 1))
		goto invalid_data;

	      if (unlikely (linep >= lineendp))
		goto invalid_data;
	      get_uleb128 (isa, linep, lineendp);
	      break;
	    }
	}
      else
	{
	  /* This is a new opcode the generator but not we know about.
	     Read the parameters associated with it but then discard
	     everything.  Read all the parameters for this opcode.  */
	  for (int n = standard_opcode_lengths[opcode]; n > 0; --n)
	    {
	      if (unlikely (linep >= lineendp))
		goto invalid_data;
	      get_uleb128 (u128, linep, lineendp);
	    }

	  /* Next round, ignore this opcode.  */
	  continue;
	}
    }

  /* Put all the files in an array.  */
  Dwarf_Files *files = libdw_alloc (dbg, Dwarf_Files,
				    sizeof (Dwarf_Files)
				    + nfilelist * sizeof (Dwarf_Fileinfo)
				    + (ndirlist + 1) * sizeof (char *),
				    1);
  const char **dirs = (void *) &files->info[nfilelist];

  files->nfiles = nfilelist;
  while (nfilelist-- > 0)
    {
      files->info[nfilelist] = filelist->info;
      filelist = filelist->next;
    }
  assert (filelist == NULL);

  /* Put all the directory strings in an array.  */
  files->ndirs = ndirlist;
  for (unsigned int i = 0; i < ndirlist; ++i)
    dirs[i] = dirarray[i]->dir;
  dirs[ndirlist] = NULL;

  /* Pass the file data structure to the caller.  */
  if (filesp != NULL)
    *filesp = files;

  size_t buf_size = (sizeof (Dwarf_Lines) + (sizeof (Dwarf_Line) * nlinelist));
  void *buf = libdw_alloc (dbg, Dwarf_Lines, buf_size, 1);

  /* First use the buffer for the pointers, and sort the entries.
     We'll write the pointers in the end of the buffer, and then
     copy into the buffer from the beginning so the overlap works.  */
  assert (sizeof (Dwarf_Line) >= sizeof (struct linelist *));
  struct linelist **sortlines = (buf + buf_size
				 - sizeof (struct linelist **) * nlinelist);

  /* The list is in LIFO order and usually they come in clumps with
     ascending addresses.  So fill from the back to probably start with
     runs already in order before we sort.  */
  for (size_t i = nlinelist; i-- > 0; )
    {
      sortlines[i] = linelist;
      linelist = linelist->next;
    }
  assert (linelist == NULL);

  /* Sort by ascending address.  */
  qsort (sortlines, nlinelist, sizeof sortlines[0], &compare_lines);

  /* Now that they are sorted, put them in the final array.
     The buffers overlap, so we've clobbered the early elements
     of SORTLINES by the time we're reading the later ones.  */
  Dwarf_Lines *lines = buf;
  lines->nlines = nlinelist;
  for (size_t i = 0; i < nlinelist; ++i)
    {
      lines->info[i] = sortlines[i]->line;
      lines->info[i].files = files;
    }

  /* Make sure the highest address for the CU is marked as end_sequence.
     This is required by the DWARF spec, but some compilers forget and
     dwfl_module_getsrc depends on it.  */
  if (nlinelist > 0)
    lines->info[nlinelist - 1].end_sequence = 1;

  /* Pass the line structure back to the caller.  */
  if (linesp != NULL)
    *linesp = lines;

  /* Success.  */
  res = 0;

 out:
  /* Free malloced line records, if any.  */
  for (size_t i = MAX_STACK_ALLOC; i < nlinelist; i++)
    {
      struct linelist *ll = malloc_linelist->next;
      free (malloc_linelist);
      malloc_linelist = ll;
    }

  return res;
}

static int
files_lines_compare (const void *p1, const void *p2)
{
  const struct files_lines_s *t1 = p1;
  const struct files_lines_s *t2 = p2;

  if (t1->debug_line_offset < t2->debug_line_offset)
    return -1;
  if (t1->debug_line_offset > t2->debug_line_offset)
    return 1;

  return 0;
}

int
internal_function
__libdw_getsrclines (Dwarf *dbg, Dwarf_Off debug_line_offset,
		     const char *comp_dir, unsigned address_size,
		     Dwarf_Lines **linesp, Dwarf_Files **filesp)
{
  struct files_lines_s fake = { .debug_line_offset = debug_line_offset };
  struct files_lines_s **found = tfind (&fake, &dbg->files_lines,
					files_lines_compare);
  if (found == NULL)
    {
      Elf_Data *data = __libdw_checked_get_data (dbg, IDX_debug_line);
      if (data == NULL
	  || __libdw_offset_in_section (dbg, IDX_debug_line,
					debug_line_offset, 1) != 0)
	return -1;

      const unsigned char *linep = data->d_buf + debug_line_offset;
      const unsigned char *lineendp = data->d_buf + data->d_size;

      struct files_lines_s *node = libdw_alloc (dbg, struct files_lines_s,
						sizeof *node, 1);

      if (read_srclines (dbg, linep, lineendp, comp_dir, address_size,
			 &node->lines, &node->files) != 0)
	return -1;

      node->debug_line_offset = debug_line_offset;

      found = tsearch (node, &dbg->files_lines, files_lines_compare);
      if (found == NULL)
	{
	  __libdw_seterrno (DWARF_E_NOMEM);
	  return -1;
	}
    }

  if (linesp != NULL)
    *linesp = (*found)->lines;

  if (filesp != NULL)
    *filesp = (*found)->files;

  return 0;
}

/* Get the compilation directory, if any is set.  */
const char *
__libdw_getcompdir (Dwarf_Die *cudie)
{
  Dwarf_Attribute compdir_attr_mem;
  Dwarf_Attribute *compdir_attr = INTUSE(dwarf_attr) (cudie,
						      DW_AT_comp_dir,
						      &compdir_attr_mem);
  return INTUSE(dwarf_formstring) (compdir_attr);
}

int
dwarf_getsrclines (Dwarf_Die *cudie, Dwarf_Lines **lines, size_t *nlines)
{
  if (unlikely (cudie == NULL
		|| (INTUSE(dwarf_tag) (cudie) != DW_TAG_compile_unit
		    && INTUSE(dwarf_tag) (cudie) != DW_TAG_partial_unit)))
    return -1;

  /* Get the information if it is not already known.  */
  struct Dwarf_CU *const cu = cudie->cu;
  if (cu->lines == NULL)
    {
      /* Failsafe mode: no data found.  */
      cu->lines = (void *) -1l;
      cu->files = (void *) -1l;

      /* The die must have a statement list associated.  */
      Dwarf_Attribute stmt_list_mem;
      Dwarf_Attribute *stmt_list = INTUSE(dwarf_attr) (cudie, DW_AT_stmt_list,
						       &stmt_list_mem);

      /* Get the offset into the .debug_line section.  NB: this call
	 also checks whether the previous dwarf_attr call failed.  */
      Dwarf_Off debug_line_offset;
      if (__libdw_formptr (stmt_list, IDX_debug_line, DWARF_E_NO_DEBUG_LINE,
			   NULL, &debug_line_offset) == NULL)
	return -1;

      if (__libdw_getsrclines (cu->dbg, debug_line_offset,
			       __libdw_getcompdir (cudie),
			       cu->address_size, &cu->lines, &cu->files) < 0)
	return -1;
    }
  else if (cu->lines == (void *) -1l)
    return -1;

  *lines = cu->lines;
  *nlines = cu->lines->nlines;

  // XXX Eventually: unlocking here.

  return 0;
}
INTDEF(dwarf_getsrclines)
