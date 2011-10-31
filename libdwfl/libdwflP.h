/* Internal definitions for libdwfl.
   Copyright (C) 2005-2011 Red Hat, Inc.
   This file is part of Red Hat elfutils.

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

   In addition, as a special exception, Red Hat, Inc. gives You the
   additional right to link the code of Red Hat elfutils with code licensed
   under any Open Source Initiative certified open source license
   (http://www.opensource.org/licenses/index.php) which requires the
   distribution of source code with any binary distribution and to
   distribute linked combinations of the two.  Non-GPL Code permitted under
   this exception must only link to the code of Red Hat elfutils through
   those well defined interfaces identified in the file named EXCEPTION
   found in the source code files (the "Approved Interfaces").  The files
   of Non-GPL Code may instantiate templates or use macros or inline
   functions from the Approved Interfaces without causing the resulting
   work to be covered by the GNU General Public License.  Only Red Hat,
   Inc. may make changes or additions to the list of Approved Interfaces.
   Red Hat's grant of this exception is conditioned upon your not adding
   any new exceptions.  If you wish to add a new Approved Interface or
   exception, please contact Red Hat.  You must obey the GNU General Public
   License in all respects for all of the Red Hat elfutils code and other
   code used in conjunction with Red Hat elfutils except the Non-GPL Code
   covered by this exception.  If you modify this file, you may extend this
   exception to your version of the file, but you are not obligated to do
   so.  If you do not wish to provide this exception without modification,
   you must delete this exception statement from your version and license
   this file solely under the GPL without exception.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifndef _LIBDWFLP_H
#define _LIBDWFLP_H	1

#ifndef PACKAGE_NAME
# include <config.h>
#endif
#include <libdwfl.h>
#include <libebl.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../libdw/libdwP.h"	/* We need its INTDECLs.  */

/* gettext helper macros.  */
#define _(Str) dgettext ("elfutils", Str)

#define DWFL_ERRORS							      \
  DWFL_ERROR (NOERROR, N_("no error"))					      \
  DWFL_ERROR (UNKNOWN_ERROR, N_("unknown error"))			      \
  DWFL_ERROR (NOMEM, N_("out of memory"))				      \
  DWFL_ERROR (ERRNO, N_("See errno"))					      \
  DWFL_ERROR (LIBELF, N_("See elf_errno"))				      \
  DWFL_ERROR (LIBDW, N_("See dwarf_errno"))				      \
  DWFL_ERROR (LIBEBL, N_("See ebl_errno (XXX missing)"))		      \
  DWFL_ERROR (ZLIB, N_("gzip decompression failed"))			      \
  DWFL_ERROR (BZLIB, N_("bzip2 decompression failed"))			      \
  DWFL_ERROR (LZMA, N_("LZMA decompression failed"))			      \
  DWFL_ERROR (UNKNOWN_MACHINE, N_("no support library found for machine"))    \
  DWFL_ERROR (NOREL, N_("Callbacks missing for ET_REL file"))		      \
  DWFL_ERROR (BADRELTYPE, N_("Unsupported relocation type"))		      \
  DWFL_ERROR (BADRELOFF, N_("r_offset is bogus"))			      \
  DWFL_ERROR (BADSTROFF, N_("offset out of range"))			      \
  DWFL_ERROR (RELUNDEF, N_("relocation refers to undefined symbol"))	      \
  DWFL_ERROR (CB, N_("Callback returned failure"))			      \
  DWFL_ERROR (NO_DWARF, N_("No DWARF information found"))		      \
  DWFL_ERROR (NO_SYMTAB, N_("No symbol table found"))			      \
  DWFL_ERROR (NO_PHDR, N_("No ELF program headers"))			      \
  DWFL_ERROR (OVERLAP, N_("address range overlaps an existing module"))	      \
  DWFL_ERROR (ADDR_OUTOFRANGE, N_("address out of range"))		      \
  DWFL_ERROR (NO_MATCH, N_("no matching address range"))		      \
  DWFL_ERROR (TRUNCATED, N_("image truncated"))				      \
  DWFL_ERROR (ALREADY_ELF, N_("ELF file opened"))			      \
  DWFL_ERROR (BADELF, N_("not a valid ELF file"))			      \
  DWFL_ERROR (WEIRD_TYPE, N_("cannot handle DWARF type description"))	      \
  DWFL_ERROR (WRONG_ID_ELF, N_("ELF file does not match build ID"))	      \
  DWFL_ERROR (BAD_PRELINK, N_("corrupt .gnu.prelink_undo section data"))

#define DWFL_ERROR(name, text) DWFL_E_##name,
typedef enum { DWFL_ERRORS DWFL_E_NUM } Dwfl_Error;
#undef	DWFL_ERROR

#define OTHER_ERROR(name)	((unsigned int) DWFL_E_##name << 16)
#define DWFL_E(name, errno)	(OTHER_ERROR (name) | (errno))

extern int __libdwfl_canon_error (Dwfl_Error) internal_function;
extern void __libdwfl_seterrno (Dwfl_Error) internal_function;

struct Dwfl
{
  const Dwfl_Callbacks *callbacks;

  Dwfl_Module *modulelist;    /* List in order used by full traversals.  */

  GElf_Addr offline_next_address;

  GElf_Addr segment_align;	/* Smallest granularity of segments.  */

  /* Binary search table in three parallel malloc'd arrays.  */
  size_t lookup_elts;		/* Elements in use.  */
  size_t lookup_alloc;		/* Elements allococated.  */
  GElf_Addr *lookup_addr;	/* Start address of segment.  */
  Dwfl_Module **lookup_module;	/* Module associated with segment, or null.  */
  int *lookup_segndx;		/* User segment index, or -1.  */

  /* Cache from last dwfl_report_segment call.  */
  const void *lookup_tail_ident;
  GElf_Off lookup_tail_vaddr;
  GElf_Off lookup_tail_offset;
  int lookup_tail_ndx;
};

#define OFFLINE_REDZONE		0x10000

struct dwfl_file
{
  char *name;
  int fd;
  bool valid;			/* The build ID note has been matched.  */
  bool relocated;		/* Partial relocation of all sections done.  */

  Elf *elf;

  /* This is the lowest p_vaddr in this ELF file, aligned to p_align.
     For a file without phdrs, this is zero.  */
  GElf_Addr vaddr;

  /* This is an address chosen for synchronization between the main file
     and the debug file.  See dwfl_module_getdwarf.c for how it's chosen.  */
  GElf_Addr address_sync;
};

struct Dwfl_Module
{
  Dwfl *dwfl;
  struct Dwfl_Module *next;	/* Link on Dwfl.modulelist.  */

  void *userdata;

  char *name;			/* Iterator name for this module.  */
  GElf_Addr low_addr, high_addr;

  struct dwfl_file main, debug;
  GElf_Addr main_bias;
  Ebl *ebl;
  GElf_Half e_type;		/* GElf_Ehdr.e_type cache.  */
  Dwfl_Error elferr;		/* Previous failure to open main file.  */

  struct dwfl_relocation *reloc_info; /* Relocatable sections.  */

  struct dwfl_file *symfile;	/* Either main or debug.  */
  Elf_Data *symdata;		/* Data in the ELF symbol table section.  */
  size_t syments;		/* sh_size / sh_entsize of that section.  */
  int first_global;		/* Index of first global symbol of table.  */
  Elf_Data *symstrdata;		/* Data for its string table.  */
  Elf_Data *symxndxdata;	/* Data in the extended section index table. */

  Dwarf *dw;			/* libdw handle for its debugging info.  */

  Dwfl_Error symerr;		/* Previous failure to load symbols.  */
  Dwfl_Error dwerr;		/* Previous failure to load DWARF.  */

  /* Known CU's in this module.  */
  struct dwfl_cu *first_cu, **cu;

  void *lazy_cu_root;		/* Table indexed by Dwarf_Off of CU.  */

  struct dwfl_arange *aranges;	/* Mapping of addresses in module to CUs.  */

  void *build_id_bits;		/* malloc'd copy of build ID bits.  */
  GElf_Addr build_id_vaddr;	/* Address where they reside, 0 if unknown.  */
  int build_id_len;		/* -1 for prior failure, 0 if unset.  */

  unsigned int ncu;
  unsigned int lazycu;		/* Possible users, deleted when none left.  */
  unsigned int naranges;

  Dwarf_CFI *dwarf_cfi;		/* Cached DWARF CFI for this module.  */
  Dwarf_CFI *eh_cfi;		/* Cached EH CFI for this module.  */

  int segment;			/* Index of first segment table entry.  */
  bool gc;			/* Mark/sweep flag.  */
};



/* Information cached about each CU in Dwfl_Module.dw.  */
struct dwfl_cu
{
  /* This caches libdw information about the CU.  It's also the
     address passed back to users, so we take advantage of the
     fact that it's placed first to cast back.  */
  Dwarf_Die die;

  Dwfl_Module *mod;		/* Pointer back to containing module.  */

  struct dwfl_cu *next;		/* CU immediately following in the file.  */

  struct Dwfl_Lines *lines;
};

struct Dwfl_Lines
{
  struct dwfl_cu *cu;

  /* This is what the opaque Dwfl_Line * pointers we pass to users are.
     We need to recover pointers to our struct dwfl_cu and a record in
     libdw's Dwarf_Line table.  To minimize the memory used in addition
     to libdw's Dwarf_Lines buffer, we just point to our own index in
     this table, and have one pointer back to the CU.  The indices here
     match those in libdw's Dwarf_CU.lines->info table.  */
  struct Dwfl_Line
  {
    unsigned int idx;		/* My index in the dwfl_cu.lines table.  */
  } idx[0];
};

static inline struct dwfl_cu *
dwfl_linecu_inline (const Dwfl_Line *line)
{
  const struct Dwfl_Lines *lines = ((const void *) line
				    - offsetof (struct Dwfl_Lines,
						idx[line->idx]));
  return lines->cu;
}
#define dwfl_linecu dwfl_linecu_inline

static inline GElf_Addr
dwfl_adjusted_address (Dwfl_Module *mod, GElf_Addr addr)
{
  return addr + mod->main_bias;
}

static inline GElf_Addr
dwfl_deadjust_address (Dwfl_Module *mod, GElf_Addr addr)
{
  return addr - mod->main_bias;
}

static inline Dwarf_Addr
dwfl_adjusted_dwarf_addr (Dwfl_Module *mod, Dwarf_Addr addr)
{
  return dwfl_adjusted_address (mod, (addr
				      - mod->debug.address_sync
				      + mod->main.address_sync));
}

static inline Dwarf_Addr
dwfl_deadjust_dwarf_addr (Dwfl_Module *mod, Dwarf_Addr addr)
{
  return (dwfl_deadjust_address (mod, addr)
	  - mod->main.address_sync
	  + mod->debug.address_sync);
}

static inline GElf_Addr
dwfl_adjusted_st_value (Dwfl_Module *mod, GElf_Addr addr)
{
  if (mod->symfile == &mod->main)
    return dwfl_adjusted_address (mod, addr);
  return dwfl_adjusted_dwarf_addr (mod, addr);
}

static inline GElf_Addr
dwfl_deadjust_st_value (Dwfl_Module *mod, GElf_Addr addr)
{
  if (mod->symfile == &mod->main)
    return dwfl_deadjust_address (mod, addr);
  return dwfl_deadjust_dwarf_addr (mod, addr);
}

/* This describes a contiguous address range that lies in a single CU.
   We condense runs of Dwarf_Arange entries for the same CU into this.  */
struct dwfl_arange
{
  struct dwfl_cu *cu;
  size_t arange;		/* Index in Dwarf_Aranges.  */
};



extern void __libdwfl_module_free (Dwfl_Module *mod) internal_function;

/* Find the main ELF file, update MOD->elferr and/or MOD->main.elf.  */
extern void __libdwfl_getelf (Dwfl_Module *mod) internal_function;

/* Process relocations in debugging sections in an ET_REL file.
   FILE must be opened with ELF_C_READ_MMAP_PRIVATE or ELF_C_READ,
   to make it possible to relocate the data in place (or ELF_C_RDWR or
   ELF_C_RDWR_MMAP if you intend to modify the Elf file on disk).  After
   this, dwarf_begin_elf on FILE will read the relocated data.

   When DEBUG is false, apply partial relocation to all sections.  */
extern Dwfl_Error __libdwfl_relocate (Dwfl_Module *mod, Elf *file, bool debug)
  internal_function;

/* Process (simple) relocations in arbitrary section TSCN of an ET_REL file.
   RELOCSCN is SHT_REL or SHT_RELA and TSCN is its sh_info target section.  */
extern Dwfl_Error __libdwfl_relocate_section (Dwfl_Module *mod, Elf *relocated,
					      Elf_Scn *relocscn, Elf_Scn *tscn,
					      bool partial)
  internal_function;

/* Adjust *VALUE from section-relative to absolute.
   MOD->dwfl->callbacks->section_address is called to determine the actual
   address of a loaded section.  */
extern Dwfl_Error __libdwfl_relocate_value (Dwfl_Module *mod, Elf *elf,
					    size_t *shstrndx_cache,
					    Elf32_Word shndx,
					    GElf_Addr *value)
     internal_function;


/* Ensure that MOD->ebl is set up.  */
extern Dwfl_Error __libdwfl_module_getebl (Dwfl_Module *mod) internal_function;

/* Install a new Dwarf_CFI in *SLOT (MOD->eh_cfi or MOD->dwarf_cfi).  */
extern Dwarf_CFI *__libdwfl_set_cfi (Dwfl_Module *mod, Dwarf_CFI **slot,
				     Dwarf_CFI *cfi)
  internal_function;

/* Iterate through all the CU's in the module.  Start by passing a null
   LASTCU, and then pass the last *CU returned.  Success return with null
   *CU no more CUs.  */
extern Dwfl_Error __libdwfl_nextcu (Dwfl_Module *mod, struct dwfl_cu *lastcu,
				    struct dwfl_cu **cu) internal_function;

/* Find the CU by address.  */
extern Dwfl_Error __libdwfl_addrcu (Dwfl_Module *mod, Dwarf_Addr addr,
				    struct dwfl_cu **cu) internal_function;

/* Ensure that CU->lines (and CU->cu->lines) is set up.  */
extern Dwfl_Error __libdwfl_cu_getsrclines (struct dwfl_cu *cu)
  internal_function;

/* Look in ELF for an NT_GNU_BUILD_ID note.  If SET is true, store it
   in MOD and return its length.  If SET is false, instead compare it
   to that stored in MOD and return 2 if they match, 1 if they do not.
   Returns -1 for errors, 0 if no note is found.  */
extern int __libdwfl_find_build_id (Dwfl_Module *mod, bool set, Elf *elf)
  internal_function;

/* Open a main or debuginfo file by its build ID, returns the fd.  */
extern int __libdwfl_open_by_build_id (Dwfl_Module *mod, bool debug,
				       char **file_name) internal_function;

extern uint32_t __libdwfl_crc32 (uint32_t crc, unsigned char *buf, size_t len)
  attribute_hidden;
extern int __libdwfl_crc32_file (int fd, uint32_t *resp) attribute_hidden;


/* Meat of dwfl_report_elf, given elf_begin just called.
   Consumes ELF on success, not on failure.  */
extern Dwfl_Module *__libdwfl_report_elf (Dwfl *dwfl, const char *name,
					  const char *file_name, int fd,
					  Elf *elf, GElf_Addr base, bool sanity)
  internal_function;

/* Meat of dwfl_report_offline.  */
extern Dwfl_Module *__libdwfl_report_offline (Dwfl *dwfl, const char *name,
					      const char *file_name,
					      int fd, bool closefd,
					      int (*predicate) (const char *,
								const char *))
  internal_function;

/* Decompression wrappers: decompress whole file into memory.  */
extern Dwfl_Error __libdw_gunzip  (int fd, off64_t start_offset,
				   void *mapped, size_t mapped_size,
				   void **whole, size_t *whole_size)
  internal_function;
extern Dwfl_Error __libdw_bunzip2 (int fd, off64_t start_offset,
				   void *mapped, size_t mapped_size,
				   void **whole, size_t *whole_size)
  internal_function;
extern Dwfl_Error __libdw_unlzma (int fd, off64_t start_offset,
				  void *mapped, size_t mapped_size,
				  void **whole, size_t *whole_size)
  internal_function;

/* Skip the image header before a file image: updates *START_OFFSET.  */
extern Dwfl_Error __libdw_image_header (int fd, off64_t *start_offset,
					void *mapped, size_t mapped_size)
  internal_function;

/* Open Elf handle on *FDP.  This handles decompression and checks
   elf_kind.  Succeed only for ELF_K_ELF, or also ELF_K_AR if ARCHIVE_OK.
   Returns DWFL_E_NOERROR and sets *ELFP on success, resets *FDP to -1 if
   it's no longer used.  Resets *FDP on failure too iff CLOSE_ON_FAIL.  */
extern Dwfl_Error __libdw_open_file (int *fdp, Elf **elfp,
				     bool close_on_fail, bool archive_ok)
  internal_function;

/* These are working nicely for --core, but are not ready to be
   exported interfaces quite yet.  */

/* Type of callback function ...
 */
typedef bool Dwfl_Memory_Callback (Dwfl *dwfl, int segndx,
				   void **buffer, size_t *buffer_available,
				   GElf_Addr vaddr, size_t minread, void *arg);

/* Type of callback function ...
 */
typedef bool Dwfl_Module_Callback (Dwfl_Module *mod, void **userdata,
				   const char *name, Dwarf_Addr base,
				   void **buffer, size_t *buffer_available,
				   GElf_Off cost, GElf_Off worthwhile,
				   GElf_Off whole, GElf_Off contiguous,
				   void *arg, Elf **elfp);

/* ...
 */
extern int dwfl_segment_report_module (Dwfl *dwfl, int ndx, const char *name,
				       Dwfl_Memory_Callback *memory_callback,
				       void *memory_callback_arg,
				       Dwfl_Module_Callback *read_eagerly,
				       void *read_eagerly_arg);

/* Report a module for entry in the dynamic linker's struct link_map list.
   For each link_map entry, if an existing module resides at its address,
   this just modifies that module's name and suggested file name.  If
   no such module exists, this calls dwfl_report_elf on the l_name string.

   If AUXV is not null, it points to AUXV_SIZE bytes of auxiliary vector
   data as contained in an NT_AUXV note or read from a /proc/pid/auxv
   file.  When this is available, it guides the search.  If AUXV is null
   or the memory it points to is not accessible, then this search can
   only find where to begin if the correct executable file was
   previously reported and preloaded as with dwfl_report_elf.

   Returns the number of modules found, or -1 for errors.  */
extern int dwfl_link_map_report (Dwfl *dwfl, const void *auxv, size_t auxv_size,
				 Dwfl_Memory_Callback *memory_callback,
				 void *memory_callback_arg);


/* Avoid PLT entries.  */
INTDECL (dwfl_begin)
INTDECL (dwfl_errmsg)
INTDECL (dwfl_errno)
INTDECL (dwfl_addrmodule)
INTDECL (dwfl_addrsegment)
INTDECL (dwfl_addrdwarf)
INTDECL (dwfl_addrdie)
INTDECL (dwfl_core_file_report)
INTDECL (dwfl_getmodules)
INTDECL (dwfl_module_addrdie)
INTDECL (dwfl_module_address_section)
INTDECL (dwfl_module_addrsym)
INTDECL (dwfl_module_build_id)
INTDECL (dwfl_module_getdwarf)
INTDECL (dwfl_module_getelf)
INTDECL (dwfl_module_getsym)
INTDECL (dwfl_module_getsymtab)
INTDECL (dwfl_module_getsrc)
INTDECL (dwfl_module_report_build_id)
INTDECL (dwfl_report_elf)
INTDECL (dwfl_report_begin)
INTDECL (dwfl_report_begin_add)
INTDECL (dwfl_report_module)
INTDECL (dwfl_report_segment)
INTDECL (dwfl_report_offline)
INTDECL (dwfl_report_end)
INTDECL (dwfl_build_id_find_elf)
INTDECL (dwfl_build_id_find_debuginfo)
INTDECL (dwfl_standard_find_debuginfo)
INTDECL (dwfl_link_map_report)
INTDECL (dwfl_linux_kernel_find_elf)
INTDECL (dwfl_linux_kernel_module_section_address)
INTDECL (dwfl_linux_proc_report)
INTDECL (dwfl_linux_proc_maps_report)
INTDECL (dwfl_linux_proc_find_elf)
INTDECL (dwfl_linux_kernel_report_kernel)
INTDECL (dwfl_linux_kernel_report_modules)
INTDECL (dwfl_linux_kernel_report_offline)
INTDECL (dwfl_offline_section_address)
INTDECL (dwfl_module_relocate_address)
INTDECL (dwfl_module_dwarf_cfi)
INTDECL (dwfl_module_eh_cfi)

/* Leading arguments standard to callbacks passed a Dwfl_Module.  */
#define MODCB_ARGS(mod)	(mod), &(mod)->userdata, (mod)->name, (mod)->low_addr
#define CBFAIL		(errno ? DWFL_E (ERRNO, errno) : DWFL_E_CB);


/* The default used by dwfl_standard_find_debuginfo.  */
#define DEFAULT_DEBUGINFO_PATH ":.debug:/usr/lib/debug"


#endif	/* libdwflP.h */
