#include <libdw.h>

struct obstack;			/* Use <obstack.h> */
struct location;		/* Opaque */


/* Translate a C fragment for the location expression, using *INPUT
   as the starting location, begin from scratch if *INPUT is null.
   If DW_OP_fbreg is used, it may have a subfragment computing from
   the FB_ATTR location expression.

   On errors, exit and never return (XXX ?).  On success, return the
   first fragment created, which is also chained onto (*INPUT)->next.
   *INPUT is then updated with the new tail of that chain.
   *USED_DEREF is set to true iff the "deref" runtime operation
   was used, otherwise it is not modified.  */
struct location *c_translate_location (struct obstack *, int indent,
				       Dwarf_Addr bias,
				       Dwarf_Attribute *loc_attr,
				       Dwarf_Addr address,
				       struct location **input,
				       Dwarf_Attribute *fb_attr);

/* Translate a fragment to dereference the given pointer type,
   where *INPUT is the location of the pointer with that type.  */
void c_translate_pointer (struct obstack *pool, int indent,
			  Dwarf_Addr dwbias, Dwarf_Die *typedie,
			  struct location **input);

/* Translate a fragment to index an array (turning the location
   of the array into the location of an element).  If IDX is non-null,
   it's a string of C code to emit in the fragment as the array index.
   If the index is a known constant, IDX should be null and CONST_IDX
   is used instead (this case can handle local arrays in registers).  */
void c_translate_array (struct obstack *pool, int indent,
			Dwarf_Addr dwbias, Dwarf_Die *typedie,
			struct location **input,
			const char *idx, Dwarf_Word const_idx);

/* Translate a fragment to compute the address of the input location
   and assign it to the variable TARGET.  This doesn't really do anything
   (it always emits "TARGET = addr;"), but it will barf if the location
   is a register or noncontiguous object.  */
void c_translate_addressof (struct obstack *pool, int indent,
			    Dwarf_Addr dwbias, Dwarf_Die *die,
			    Dwarf_Attribute *typeattr,
			    struct location **input, const char *target);

/* Translate a fragment to fetch the value of variable or member DIE
   at the *INPUT location and store it in variable TARGET.
   This handles base integer types and bit fields.  */
void c_translate_fetch (struct obstack *pool, int indent,
			Dwarf_Addr dwbias __attribute__ ((unused)),
			Dwarf_Die *die, Dwarf_Attribute *typeattr,
			struct location **input, const char *target);

/* Emit the C fragment built up at LOC (i.e., the return value from the
   first c_translate_location call made).  INDENT should match that
   passed to c_translate_* previously.

   Writes complete lines of C99, code forming a complete C block, to STREAM.
   Return value is true iff that code uses the `deref' runtime macros.  */
bool c_emit_location (FILE *stream, struct location *loc, int indent);
