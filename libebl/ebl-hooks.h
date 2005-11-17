/* Backend hook signatures internal interface for libebl.
   Copyright (C) 2000, 2001, 2002, 2004, 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

/* Return symbol representaton of object file type.  */
const char *EBLHOOK(object_type_name) (int, char *, size_t);

/* Return symbolic representation of relocation type.  */
const char *EBLHOOK(reloc_type_name) (int, char *, size_t);

/* Check relocation type.  */
bool EBLHOOK(reloc_type_check) (int);

/* Check if relocation type is for simple absolute relocations.  */
Elf_Type EBLHOOK(reloc_simple_type) (Ebl *, int);

/* Check relocation type use.  */
bool EBLHOOK(reloc_valid_use) (Elf *, int);

/* Return true if the symbol type is that referencing the GOT.  */
bool EBLHOOK(gotpc_reloc_check) (Elf *, int);

/* Return symbolic representation of segment type.  */
const char *EBLHOOK(segment_type_name) (int, char *, size_t);

/* Return symbolic representation of section type.  */
const char *EBLHOOK(section_type_name) (int, char *, size_t);

/* Return section name.  */
const char *EBLHOOK(section_name) (int, int, char *, size_t);

/* Return next machine flag name.  */
const char *EBLHOOK(machine_flag_name) (GElf_Word *);

/* Check whether machine flags are valid.  */
bool EBLHOOK(machine_flag_check) (GElf_Word);

/* Return symbolic representation of symbol type.  */
const char *EBLHOOK(symbol_type_name) (int, char *, size_t);

/* Return symbolic representation of symbol binding.  */
const char *EBLHOOK(symbol_binding_name) (int, char *, size_t);

/* Return symbolic representation of dynamic tag.  */
const char *EBLHOOK(dynamic_tag_name) (int64_t, char *, size_t);

/* Check dynamic tag.  */
bool EBLHOOK(dynamic_tag_check) (int64_t);

/* Combine section header flags values.  */
GElf_Word EBLHOOK(sh_flags_combine) (GElf_Word, GElf_Word);

/* Return symbolic representation of OS ABI.  */
const char *EBLHOOK(osabi_name) (int, char *, size_t);

/* Name of a note entry type for core files.  */
const char *EBLHOOK(core_note_type_name) (uint32_t, char *, size_t);

/* Name of a note entry type for object files.  */
const char *EBLHOOK(object_note_type_name) (uint32_t, char *, size_t);

/* Handle core note.  */
bool EBLHOOK(core_note) (const char *, uint32_t, uint32_t, const char *);

/* Handle object file note.  */
bool EBLHOOK(object_note) (const char *, uint32_t, uint32_t, const char *);

/* Check section name for being that of a debug informatino section.  */
bool EBLHOOK(debugscn_p) (const char *);

/* Check whether given relocation is a copy relocation.  */
bool EBLHOOK(copy_reloc_p) (int);

/* Check whether given symbol's value is ok despite normal checks.  */
bool EBLHOOK(check_special_symbol) (Elf *, GElf_Ehdr *, const GElf_Sym *,
			      const char *, const GElf_Shdr *);

/* Check if backend uses a bss PLT in this file.  */
bool EBLHOOK(bss_plt_p) (Elf *, GElf_Ehdr *);

/* Return location expression to find return value given the
   DW_AT_type DIE of a DW_TAG_subprogram DIE.  */
int EBLHOOK(return_value_location) (Dwarf_Die *functypedie,
				    const Dwarf_Op **locp);

/* Destructor for ELF backend handle.  */
void EBLHOOK(destr) (struct ebl *);
