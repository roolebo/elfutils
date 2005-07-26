/* Find line information for given file/line/column triple.
   Copyright (C) 2005 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "libdwP.h"


int
dwarf_getsrc_file (Dwarf *dbg, const char *fname, int lineno, int column,
		   Dwarf_Line ***srcsp, size_t *nsrcs)
{
  if (dbg == NULL)
    return -1;

  bool is_basename = strchr (fname, '/') == NULL;

  size_t max_match = *nsrcs ?: ~0u;
  size_t act_match = *nsrcs;
  size_t cur_match = 0;
  Dwarf_Line **match = *nsrcs == 0 ? NULL : *srcsp;

  Dwarf_Off off = 0;
  size_t cuhl;
  Dwarf_Off noff;

  while (INTUSE(dwarf_nextcu) (dbg, off, &noff, &cuhl, NULL, NULL, NULL) == 0)
    {
      Dwarf_Die cudie_mem;
      Dwarf_Die *cudie = INTUSE(dwarf_offdie) (dbg, off + cuhl, &cudie_mem);
      if (cudie == NULL)
	continue;

      /* Get the line number information for this file.  */
      Dwarf_Lines *lines;
      size_t nlines;
      if (INTUSE(dwarf_getsrclines) (cudie, &lines, &nlines) != 0)
	return -1;

      /* Search through all the line number records for a matching
	 file and line/column number.  If any of the numbers is zero,
	 no match is performed.  */
      unsigned int lastfile = UINT_MAX;
      bool lastmatch = false;
      for (size_t cnt = 0; cnt < nlines; ++cnt)
	{
	  Dwarf_Line *line = &lines->info[cnt];

	  if (lastfile != line->file)
	    {
	      lastfile = line->file;
	      if (lastfile >= line->files->nfiles)
		{
		  __libdw_seterrno (DWARF_E_INVALID_DWARF);
		  return -1;
		}

	      /* Match the name with the name the user provided.  */
	      const char *fname2 = line->files->info[lastfile].name;
	      if (is_basename)
		lastmatch = strcmp (basename (fname2), fname) == 0;
	      else
		lastmatch = strcmp (fname2, fname) == 0;
	    }
	  if (!lastmatch)
	    continue;

	  /* See whether line and possibly column match.  */
	  if (lineno != 0
	      && (lineno > line->line
		  || (column != 0 && column > line->column)))
	    /* Cannot match.  */
	    continue;

	  /* Determine whether this is the best match so far.  */
	  size_t inner;
	  for (inner = 0; inner < cur_match; ++inner)
	    if (match[inner]->files == line->files
		&& match[inner]->file == line->file)
	      break;
	  if (inner < cur_match
	      && (match[inner]->line != line->line
		  || match[inner]->line != lineno
		  || (column != 0
		      && (match[inner]->column != line->column
			  || match[inner]->column != column))))
	    {
	      /* We know about this file already.  If this is a better
		 match for the line number, use it.  */
	      if (match[inner]->line >= line->line
		  && (match[inner]->line != line->line
		      || match[inner]->column >= line->column))
		/*  Use the new line.  Otherwise the old one.  */
		match[inner] = line;
	      continue;
	    }

	  if (cur_match < max_match)
	    {
	      if (cur_match == act_match)
		{
		  /* Enlarge the array for the results.  */
		  act_match += 10;
		  Dwarf_Line **newp = realloc (match,
					       act_match
					       * sizeof (Dwarf_Line *));
		  if (newp == NULL)
		    {
		      free (match);
		      __libdw_seterrno (DWARF_E_NOMEM);
		      return -1;
		    }
		  match = newp;
		}

	      match[cur_match++] = line;
	    }
	}

      /* If we managed to find as many matches as the user requested
	 already, there is no need to go on to the next CU.  */
      if (cur_match == max_match)
	break;

      off = noff;
    }

  if (cur_match > 0)
    {
      assert (*nsrcs == 0 || *srcsp == match);

      *nsrcs = cur_match;
      *srcsp = match;

      return 0;
    }

  __libdw_seterrno (DWARF_E_NO_MATCH);
  return -1;
}
