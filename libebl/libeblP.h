/* Internal definitions for interface for libebl.
   Copyright (C) 2000, 2001, 2002, 2004, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBEBLP_H
#define _LIBEBLP_H 1

#include <gelf.h>
#include <libebl.h>
#include <libintl.h>


/* Backend handle.  */
struct ebl
{
  /* Machine name.  */
  const char *name;

  /* Emulation name.  */
  const char *emulation;

  /* ELF machine, class, and data encoding.  */
  int machine;
  int class;
  int data;

  /* The libelf handle (if known).  */
  Elf *elf;

  /* Return symbol representaton of object file type.  */
  const char *(*object_type_name) (int, char *, size_t);

  /* Return symbolic representation of relocation type.  */
  const char *(*reloc_type_name) (int, char *, size_t);

  /* Check relocation type.  */
  bool (*reloc_type_check) (int);

  /* Check if relocation type is for simple absolute relocations.  */
  Elf_Type (*reloc_simple_type) (Elf *, int);

  /* Check relocation type use.  */
  bool (*reloc_valid_use) (Elf *, int);

  /* Return true if the symbol type is that referencing the GOT.  */
  bool (*gotpc_reloc_check) (Elf *, int);

  /* Return symbolic representation of segment type.  */
  const char *(*segment_type_name) (int, char *, size_t);

  /* Return symbolic representation of section type.  */
  const char *(*section_type_name) (int, char *, size_t);

  /* Return section name.  */
  const char *(*section_name) (int, int, char *, size_t);

  /* Return next machine flag name.  */
  const char *(*machine_flag_name) (GElf_Word *);

  /* Check whether machine flags are valid.  */
  bool (*machine_flag_check) (GElf_Word);

  /* Return symbolic representation of symbol type.  */
  const char *(*symbol_type_name) (int, char *, size_t);

  /* Return symbolic representation of symbol binding.  */
  const char *(*symbol_binding_name) (int, char *, size_t);

  /* Return symbolic representation of dynamic tag.  */
  const char *(*dynamic_tag_name) (int64_t, char *, size_t);

  /* Check dynamic tag.  */
  bool (*dynamic_tag_check) (int64_t);

  /* Combine section header flags values.  */
  GElf_Word (*sh_flags_combine) (GElf_Word, GElf_Word);

  /* Return symbolic representation of OS ABI.  */
  const char *(*osabi_name) (int, char *, size_t);

  /* Name of a note entry type for core files.  */
  const char *(*core_note_type_name) (uint32_t, char *, size_t);

  /* Name of a note entry type for object files.  */
  const char *(*object_note_type_name) (uint32_t, char *, size_t);

  /* Handle core note.  */
  bool (*core_note) (const char *, uint32_t, uint32_t, const char *);

  /* Handle object file note.  */
  bool (*object_note) (const char *, uint32_t, uint32_t, const char *);

  /* Check section name for being that of a debug informatino section.  */
  bool (*debugscn_p) (const char *);

  /* Check whether given relocation is a copy relocation.  */
  bool (*copy_reloc_p) (int);

  /* Check whether given symbol's value is ok despite normal checks.  */
  bool (*check_special_symbol) (Elf *elf,
				const GElf_Sym *sym, const char *name,
				const GElf_Shdr *destshdr);

  /* Check if backend uses a bss PLT in this file.  */
  bool (*bss_plt_p) (Elf *elf);

  /* Destructor for ELF backend handle.  */
  void (*destr) (struct ebl *);

  /* Internal data.  */
  void *dlhandle;
};


/* Type of the initialization functions in the backend modules.  */
typedef const char *(*ebl_bhinit_t) (Elf *, GElf_Half, Ebl *, size_t);


/* gettext helper macros.  */
#define _(Str) dgettext ("elfutils", Str)

#endif	/* libeblP.h */
