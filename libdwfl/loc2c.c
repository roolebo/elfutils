#include <config.h>
#include <inttypes.h>
#include <stdbool.h>
#include <obstack.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <dwarf.h>
#include <libdw.h>
#include <assert.h>

#define _(x) x

#define STACK_TYPE	"intptr_t"  /* Must be the signed type.  */
#define UTYPE		"uintptr_t" /* Must be the unsigned type.  */
#define SFORMAT		"%" PRId64 "L"
#define UFORMAT		"%" PRIu64 "UL"
#define AFORMAT		"%#" PRIx64 "UL"
#define STACKFMT	"s%u"

struct location
{
  struct location *next;

  const Dwarf_Loc *ops;
  size_t nops;

  Dwarf_Word byte_size;

  enum { loc_address, loc_register, loc_noncontiguous, loc_final } type;
  union
  {
    struct			/* loc_address or loc_final */
    {
      char *program;		/* C fragment, leaves address in s0.  */
      unsigned int stack_depth;	/* Temporaries "s0..<N>" used by it.  */
      struct location *frame_base;
      bool used_deref;		/* Program uses "deref" macro.  */
    } address;
    unsigned int regno;		/* loc_register */
    struct location *pieces;	/* loc_noncontiguous */
  };
};


static const char *
dwarf_diename_integrate (Dwarf_Die *die)
{
  Dwarf_Attribute attr_mem;
  return dwarf_formstring (dwarf_attr_integrate (die, DW_AT_name, &attr_mem));
}

/* Synthesize a new loc_address using the program on the obstack.  */
static struct location *
new_synthetic_loc (struct obstack *pool, struct location *origin, bool deref)
{
  obstack_1grow (pool, '\0');
  char *program = obstack_finish (pool);

  struct location *loc = obstack_alloc (pool, sizeof *loc);
  loc->next = NULL;
  loc->byte_size = 0;
  loc->type = loc_address;
  loc->address.program = program;
  loc->address.stack_depth = 0;
  loc->address.frame_base = NULL;
  loc->address.used_deref = deref;

  if (origin->type == loc_register)
    {
      loc->ops = origin->ops;
      loc->nops = origin->nops;
    }
  else
    {
      loc->ops = NULL;
      loc->nops = 0;
    }

  return loc;
}


/* Die in the middle of an expression.  */
static struct location *
lose (const char *failure, const Dwarf_Loc *lexpr, size_t i)
{
  error (2, 0, _("%s in DWARF expression [%Zu] at %" PRIu64
		 " (%#x: %" PRId64 ", %" PRId64 ")"),
	 failure, i, lexpr[i].offset,
	 lexpr[i].atom, lexpr[i].number, lexpr[i].number2);
  return NULL;
}

/* Translate a (constrained) DWARF expression into C code
   emitted to the obstack POOL.  INDENT is the number of indentation levels.
   ADDRBIAS is the difference between runtime and Dwarf info addresses.
   INPUT is null or an expression to be initially pushed on the stack.
   If NEED_FB is null, fail on DW_OP_fbreg, else set *NEED_FB to true
   and emit "frame_base" for it.  On success, set *MAX_STACK to the number
   of stack slots required.  On failure, set *LOSER to the index in EXPR
   of the operation we could not handle.

   Returns a failure message or null for success.  */

static const char *
translate (struct obstack *pool, int indent, Dwarf_Addr addrbias,
	   const Dwarf_Loc *expr, const size_t len,
	   struct location *input,
	   bool *need_fb, size_t *loser,
	   struct location *loc)
{
  loc->ops = expr;
  loc->nops = len;

#define DIE(msg) return (*loser = i, _(msg))

#define emit(fmt, ...) obstack_printf (pool, fmt, ## __VA_ARGS__)

  unsigned int stack_depth = 0, max_stack = 0;
  inline void deepen (void)
    {
      if (stack_depth == max_stack)
	++max_stack;
    }

#define POP(var)							      \
    if (stack_depth > 0)						      \
      --stack_depth;							      \
    else if (tos_register != -1)					      \
      fetch_tos_register ();						      \
    else								      \
      goto underflow;							      \
    int var = stack_depth
#define PUSH 		(deepen (), stack_depth++)
#define STACK(idx)	(stack_depth - 1 - (idx))

  /* Don't put stack operations in the arguments to this.  */
#define push(fmt, ...) \
  emit ("%*s" STACKFMT " = " fmt ";\n", indent * 2, "", PUSH, ## __VA_ARGS__)

  int tos_register = -1;
  inline void fetch_tos_register (void)
    {
      deepen ();
      emit ("%*s" STACKFMT " = fetch_register (%d);\n",
	    indent * 2, "", stack_depth, tos_register);
      tos_register = -1;
    }

  if (input != NULL)
    switch (input->type)
      {
      case loc_address:
	push ("addr");
	break;

      case loc_register:
	tos_register = input->regno;
	break;

      default:
	abort ();
	break;
      }

  size_t i;

  inline const char *finish (struct location *piece)
    {
      if (stack_depth > 1)
	DIE ("multiple values left on stack");
      if (stack_depth == 1)
	{
	  obstack_1grow (pool, '\0');
	  char *program = obstack_finish (pool);
	  piece->type = loc_address;
	  piece->address.program = program;
	  piece->address.stack_depth = max_stack;
	  piece->address.frame_base = NULL;
	}
      else if (tos_register == -1)
	DIE ("stack underflow");
      else if (obstack_object_size (pool) != 0)
	DIE ("register value must stand alone in location expression");
      else
	{
	  piece->type = loc_register;
	  piece->regno = tos_register;
	}
      return NULL;
    }

  struct location *pieces = NULL, **tailpiece = &pieces;
  size_t piece_expr_start = 0;
  for (i = 0; i < len; ++i)
    {
      unsigned int reg;
      uint_fast8_t sp;
      Dwarf_Word value;

      switch (expr[i].atom)
	{
	  /* Basic stack operations.  */
	case DW_OP_nop:
	  break;

	case DW_OP_dup:
	  if (stack_depth < 1)
	    goto underflow;
	  else
	    {
	      unsigned int tos = STACK (0);
	      push (STACKFMT, tos);
	    }
	  break;

	case DW_OP_drop:
	  POP (ignore);
	  emit ("%*s/* drop " STACKFMT "*/\n", indent * 2, "", ignore);
	  break;

	case DW_OP_pick:
	  sp = expr[i].number;
	op_pick:
	  if (sp >= stack_depth)
	    goto underflow;
	  sp = STACK (sp);
	  push (STACKFMT, sp);
	  break;

	case DW_OP_over:
	  sp = 1;
	  goto op_pick;

	case DW_OP_swap:
	  if (stack_depth < 2)
	    goto underflow;
	  deepen ();		/* Use a temporary slot.  */
	  emit ("%*s"
		STACKFMT " = " STACKFMT ", "
		STACKFMT " = " STACKFMT ", "
		STACKFMT " = " STACKFMT ";\n",
		indent * 2, "",
		STACK (-1), STACK (0),
		STACK (0), STACK (1),
		STACK (1), STACK (-1));
	  break;

	case DW_OP_rot:
	  if (stack_depth < 3)
	    goto underflow;
	  deepen ();		/* Use a temporary slot.  */
	  emit ("%*s"
		STACKFMT " = " STACKFMT ", "
		STACKFMT " = " STACKFMT ", "
		STACKFMT " = " STACKFMT ", "
		STACKFMT " = " STACKFMT ";\n",
		indent * 2, "",
		STACK (-1), STACK (0),
		STACK (0), STACK (1),
		STACK (1), STACK (2),
		STACK (3), STACK (-1));
	  break;


	  /* Control flow operations.  */
	case DW_OP_skip:
	  {
	    Dwarf_Off target = expr[i].offset + 3 + expr[i].number;
	    while (i + 1 < len && expr[i + 1].offset < target)
	      ++i;
	    if (expr[i + 1].offset != target)
	      DIE ("invalid skip target");
	    break;
	  }

	case DW_OP_bra:
	  DIE ("conditional branches not supported");
	  break;


	  /* Memory access.  */
	case DW_OP_deref:
	  {
	    POP (addr);
	    push ("deref (sizeof (void *), " STACKFMT ")", addr);
	    loc->address.used_deref = true;
	  }
	  break;

	case DW_OP_deref_size:
	  {
	    POP (addr);
	    push ("deref (" UFORMAT ", " STACKFMT ")",
		  expr[i].number, addr);
	    loc->address.used_deref = true;
	  }
	  break;

	case DW_OP_xderef:
	  {
	    POP (addr);
	    POP (as);
	    push ("xderef (sizeof (void *), " STACKFMT ", " STACKFMT ")",
		  addr, as);
	    loc->address.used_deref = true;
	  }
	  break;

	case DW_OP_xderef_size:
	  {
	    POP (addr);
	    POP (as);
	    push ("xderef (" UFORMAT ", " STACKFMT ", " STACKFMT ")",
		  expr[i].number, addr, as);
	    loc->address.used_deref = true;
	  }
	  break;

	  /* Constant-value operations.  */

	case DW_OP_addr:
	  push (AFORMAT, addrbias + expr[i].number);
	  break;

	case DW_OP_lit0 ... DW_OP_lit31:
	  value = expr[i].atom - DW_OP_lit0;
	  goto op_const;

	case DW_OP_const1u:
	case DW_OP_const1s:
	case DW_OP_const2u:
	case DW_OP_const2s:
	case DW_OP_const4u:
	case DW_OP_const4s:
	case DW_OP_const8u:
	case DW_OP_const8s:
	case DW_OP_constu:
	case DW_OP_consts:
	  value = expr[i].number;
	op_const:
	  push (SFORMAT, value);
	  break;

	  /* Arithmetic operations.  */
#define UNOP(dw_op, c_op)						      \
	case DW_OP_##dw_op:						      \
	  {								      \
	    POP (tos);							      \
	    push ("%s (" STACKFMT ")", #c_op, tos);			      \
	  }								      \
	  break
#define BINOP(dw_op, c_op)						      \
	case DW_OP_##dw_op:						      \
	  {								      \
	    POP (b);							      \
	    POP (a);							      \
	    push (STACKFMT " %s " STACKFMT, a, #c_op, b);		      \
	  }								      \
	  break

	  UNOP (abs, op_abs);
	  BINOP (and, &);
	  BINOP (div, /);
	  BINOP (minus, -);
	  BINOP (mod, %);
	  BINOP (mul, *);
	  UNOP (neg, -);
	  UNOP (not, ~);
	  BINOP (or, |);
	  BINOP (plus, +);
	  BINOP (shl, <<);
	  BINOP (shra, >>);
	  BINOP (xor, ^);

	  /* Comparisons are binary operators too.  */
	  BINOP (le, <=);
	  BINOP (ge, >=);
	  BINOP (eq, ==);
	  BINOP (lt, <);
	  BINOP (gt, >);
	  BINOP (ne, !=);

#undef	UNOP
#undef	BINOP

	case DW_OP_shr:
	  {
	    POP (b);
	    POP (a);
	    push ("(%s) " STACKFMT " >> (%s)" STACKFMT,
		  UTYPE, a, UTYPE, b);
	    break;
	  }

	case DW_OP_plus_uconst:
	  {
	    POP (x);
	    push (STACKFMT " + " UFORMAT, x, expr[i].number);
	  }
	  break;


	  /* Register-relative addressing.  */
	case DW_OP_breg0 ... DW_OP_breg31:
	  reg = expr[i].atom - DW_OP_breg0;
	  value = expr[i].number;
	  goto op_breg;

	case DW_OP_bregx:
	  reg = expr[i].number;
	  value = expr[i].number2;
	op_breg:
	  push ("fetch_register (%u) + " SFORMAT, reg, value);
	  break;

	case DW_OP_fbreg:
	  if (need_fb == NULL)
	    DIE ("DW_OP_fbreg from DW_AT_frame_base");
	  *need_fb = true;
	  push ("frame_base + " SFORMAT, expr[i].number);
	  break;

	  /* Direct register contents.  */
	case DW_OP_reg0 ... DW_OP_reg31:
	  reg = expr[i].atom - DW_OP_reg0;
	  goto op_reg;

	case DW_OP_regx:
	  reg = expr[i].number;
	op_reg:
	  tos_register = reg;
	  break;

	  /* Special magic.  */
	case DW_OP_piece:
	  if (stack_depth > 1)
	    /* If this ever happens we could copy the program.  */
	    DIE ("DW_OP_piece left multiple values on stack");
	  else
	    {
	      struct location *piece = obstack_alloc (pool, sizeof *piece);
	      const char *failure = finish (piece);
	      if (unlikely (failure != NULL))
		return failure;

	      piece->ops = &expr[piece_expr_start];
	      piece->nops = i - piece_expr_start;
	      piece_expr_start = i + 1;

	      piece->byte_size = expr[i].number;

	      *tailpiece = piece;
	      tailpiece = &piece->next;
	      piece->next = NULL;
	    }
	  break;

	case DW_OP_push_object_address:
	  DIE ("XXX DW_OP_push_object_address");
	  break;

	default:
	  DIE ("unrecognized operation");
	  break;
	}
    }

  if (likely (pieces == NULL))
    return finish (loc);

  if (piece_expr_start != i)
    DIE ("extra operations after last DW_OP_piece");

  loc->type = loc_noncontiguous;
  loc->pieces = pieces;

  return NULL;

 underflow:
  DIE ("stack underflow");

#undef emit
#undef push
#undef PUSH
#undef POP
#undef STACK
#undef DIE
}

/* Translate a location starting from an address or nothing.  */
static struct location *
location_from_address (struct obstack *pool,
		       int indent, Dwarf_Addr dwbias,
		       const Dwarf_Loc *expr, size_t len, Dwarf_Addr address,
		       struct location **input, Dwarf_Attribute *fb_attr)
{
  bool need_fb = false;
  size_t loser;
  struct location *loc = obstack_alloc (pool, sizeof *loc);
  const char *failure = translate (pool, indent + 1, dwbias, expr, len,
				   *input, &need_fb, &loser, loc);
  if (unlikely (failure != NULL))
    return lose (failure, expr, loser);

  loc->next = NULL;
  if (need_fb)
    {
      /* The main expression uses DW_OP_fbreg, so we need to compute
	 the DW_AT_frame_base attribute expression's value first.  */

      Dwarf_Loc *fb_expr;
      size_t fb_len;
      switch (dwarf_addrloclists (fb_attr, address - dwbias,
				  &fb_expr, &fb_len, 1))
	{
	case 1:			/* Should always happen.  */
	  if (fb_len == 0)
	    goto fb_inaccessible;
	  break;

	default:		/* Shouldn't happen.  */
	case -1:
	  error (2, 0, "dwarf_addrloclists (form %#x): %s",
		 dwarf_whatform (fb_attr), dwarf_errmsg (-1));
	  return NULL;

	case 0:			/* Shouldn't happen.  */
	fb_inaccessible:
	  error (2, 0, "DW_AT_frame_base not accessible at this address");
	  return NULL;
	}

      loc->address.frame_base = obstack_alloc (pool, sizeof *loc);
      failure = translate (pool, indent + 1, dwbias, fb_expr, fb_len, NULL,
			   NULL, &loser, loc->address.frame_base);
      if (unlikely (failure != NULL))
	return lose (failure, fb_expr, loser);
    }

  if (*input != NULL)
    (*input)->next = loc;
  *input = loc;

  return loc;
}

/* Translate a location starting from a non-address "on the top of the
   stack".  The *INPUT location is a register name or noncontiguous
   object specification, and this expression wants to find the "address"
   of an object relative to that "address".  */

static struct location *
location_relative (struct obstack *pool,
		   int indent, Dwarf_Addr dwbias,
		   const Dwarf_Loc *expr, size_t len, Dwarf_Addr address,
		   struct location **input, Dwarf_Attribute *fb_attr)
{
  Dwarf_Sword *stack;
  unsigned int stack_depth = 0, max_stack = 0;
  inline void deepen (void)
    {
      if (stack_depth == max_stack)
	{
	  ++max_stack;
	  obstack_blank (pool, sizeof stack[0]);
	  stack = (void *) obstack_base (pool);
	}
    }

#define POP(var)							      \
    if (stack_depth > 0)						      \
      --stack_depth;							      \
    else								      \
      goto underflow;							      \
    int var = stack_depth
#define PUSH 		(deepen (), stack_depth++)
#define STACK(idx)	(stack_depth - 1 - (idx))
#define STACKWORD(idx)	stack[STACK (idx)]

  /* Don't put stack operations in the arguments to this.  */
#define push(value) (stack[PUSH] = (value))

  const char *failure = NULL;
#define DIE(msg) do { failure = _(msg); goto fail; } while (0)

  struct location *head = NULL;
  size_t i;
  for (i = 0; i < len; ++i)
    {
      uint_fast8_t sp;
      Dwarf_Word value;

      switch (expr[i].atom)
	{
	  /* Basic stack operations.  */
	case DW_OP_nop:
	  break;

	case DW_OP_dup:
	  if (stack_depth < 1)
	    goto underflow;
	  else
	    {
	      unsigned int tos = STACK (0);
	      push (stack[tos]);
	    }
	  break;

	case DW_OP_drop:
	  if (stack_depth > 0)
	    --stack_depth;
	  else if (*input != NULL)
	    /* Mark that we have consumed the input.  */
	    *input = NULL;
	  else
	    /* Hits if cleared above, or if we had no input at all.  */
	    goto underflow;
	  break;

	case DW_OP_pick:
	  sp = expr[i].number;
	op_pick:
	  if (sp >= stack_depth)
	    goto underflow;
	  sp = STACK (sp);
	  push (stack[sp]);
	  break;

	case DW_OP_over:
	  sp = 1;
	  goto op_pick;

	case DW_OP_swap:
	  if (stack_depth < 2)
	    goto underflow;
	  deepen ();		/* Use a temporary slot.  */
	  STACKWORD (-1) = STACKWORD (0);
	  STACKWORD (0) = STACKWORD (1);
	  STACKWORD (1) = STACKWORD (-1);
	  break;

	case DW_OP_rot:
	  if (stack_depth < 3)
	    goto underflow;
	  deepen ();		/* Use a temporary slot.  */
	  STACKWORD (-1) = STACKWORD (0);
	  STACKWORD (0) = STACKWORD (1);
	  STACKWORD (2) = STACKWORD (2);
	  STACKWORD (2) = STACKWORD (-1);
	  break;


	  /* Control flow operations.  */
	case DW_OP_bra:
	  {
	    POP (taken);
	    if (stack[taken] == 0)
	      break;
	  }
	  /*FALLTHROUGH*/

	case DW_OP_skip:
	  {
	    Dwarf_Off target = expr[i].offset + 3 + expr[i].number;
	    while (i + 1 < len && expr[i + 1].offset < target)
	      ++i;
	    if (expr[i + 1].offset != target)
	      DIE ("invalid skip target");
	    break;
	  }

	  /* Memory access.  */
	case DW_OP_deref:
	case DW_OP_deref_size:
	case DW_OP_xderef:
	case DW_OP_xderef_size:

	  /* Register-relative addressing.  */
	case DW_OP_breg0 ... DW_OP_breg31:
	case DW_OP_bregx:
	case DW_OP_fbreg:

	  /* This started from a register, but now it's following a pointer.
	     So we can do the translation starting from address here.  */
	  return location_from_address (pool, indent, dwbias,
					expr, len, address, input, fb_attr);


	  /* Constant-value operations.  */
	case DW_OP_addr:
	  push (dwbias + expr[i].number);
	  break;

	case DW_OP_lit0 ... DW_OP_lit31:
	  value = expr[i].atom - DW_OP_lit0;
	  goto op_const;

	case DW_OP_const1u:
	case DW_OP_const1s:
	case DW_OP_const2u:
	case DW_OP_const2s:
	case DW_OP_const4u:
	case DW_OP_const4s:
	case DW_OP_const8u:
	case DW_OP_const8s:
	case DW_OP_constu:
	case DW_OP_consts:
	  value = expr[i].number;
	op_const:
	  push (value);
	  break;

	  /* Arithmetic operations.  */
#define UNOP(dw_op, c_op)						      \
	case DW_OP_##dw_op:						      \
	  {								      \
	    POP (tos);							      \
	    push (c_op (stack[tos])); 					      \
	  }								      \
	  break
#define BINOP(dw_op, c_op)						      \
	case DW_OP_##dw_op:						      \
	  {								      \
	    POP (b);							      \
	    POP (a);							      \
 	    push (stack[a] c_op stack[b]);				      \
	  }								      \
	  break

#define op_abs(x) (x < 0 ? -x : x)
	  UNOP (abs, op_abs);
	  BINOP (and, &);
	  BINOP (div, /);
	  BINOP (mod, %);
	  BINOP (mul, *);
	  UNOP (neg, -);
	  UNOP (not, ~);
	  BINOP (or, |);
	  BINOP (shl, <<);
	  BINOP (shra, >>);
	  BINOP (xor, ^);

	  /* Comparisons are binary operators too.  */
	  BINOP (le, <=);
	  BINOP (ge, >=);
	  BINOP (eq, ==);
	  BINOP (lt, <);
	  BINOP (gt, >);
	  BINOP (ne, !=);

#undef	UNOP
#undef	BINOP

	case DW_OP_shr:
	  {
	    POP (b);
	    POP (a);
	    push ((Dwarf_Word) stack[a] >> (Dwarf_Word) stack[b]);
	    break;
	  }

	  /* Simple addition we may be able to handle relative to
	     the starting register name.  */
	case DW_OP_minus:
	  {
	    POP (tos);
	    value = -stack[tos];
	    goto plus;
	  }
	case DW_OP_plus:
	  {
	    POP (tos);
	    value = stack[tos];
	    goto plus;
	  }
	case DW_OP_plus_uconst:
	  value = expr[i].number;
	plus:
	  if (stack_depth > 0)
	    {
	      /* It's just private diddling after all.  */
	      POP (a);
	      push (stack[a] + value);
	      break;
	    }
	  if (*input == NULL)
	    goto underflow;

	  /* This is the primary real-world case: the expression takes
	     the input address and adds a constant offset.  */

	  while ((*input)->type == loc_noncontiguous)
	    {
	      /* We are starting from a noncontiguous object (DW_OP_piece).
		 Find the piece we want.  */

	      struct location *piece = (*input)->pieces;
	      while (piece != NULL && value >= piece->byte_size)
		{
		  value -= piece->byte_size;
		  piece = piece->next;
		}
	      if (piece == NULL)
		DIE ("offset outside available pieces");

	      *input = piece;
	    }

	  switch ((*input)->type)
	    {
	    case loc_address:
	      {
		/* The piece we want is actually in memory.  Use the same
		   program to compute the address from the preceding input.  */

		struct location *loc = obstack_alloc (pool, sizeof *loc);
		*loc = **input;
		if (head == NULL)
		  head = loc;
		(*input)->next = loc;
		if (value == 0)
		  {
		    /* The piece addresses exactly where we want to go.  */
		    loc->next = NULL;
		    *input = loc;
		  }
		else
		  {
		    /* Add a second fragment to offset the piece address.  */
		    obstack_printf (pool, "%*saddr += " SFORMAT "\n",
				    indent * 2, "", value);
		    *input = loc->next = new_synthetic_loc (pool, *input,
							    false);
		  }

		if (i + 1 < len)
		  {
		    /* This expression keeps going, but further
		       computations now have an address to start with.
		       So we can punt to the address computation generator.  */
		    loc = location_from_address (pool, indent, dwbias,
						 &expr[i + 1], len - i - 1,
						 address, input, fb_attr);
		    if (loc == NULL)
		      return NULL;
		  }

		/* That's all she wrote.  */
		return head;
	      }

	    case loc_register:
	      // XXX

	    default:
	      abort ();
	    }
	  break;

	  /* Direct register contents.  */
	case DW_OP_reg0 ... DW_OP_reg31:
	case DW_OP_regx:
	  DIE ("register");
	  break;

	  /* Special magic.  */
	case DW_OP_piece:
	  DIE ("DW_OP_piece");
	  break;

	case DW_OP_push_object_address:
	  DIE ("XXX DW_OP_push_object_address");
	  break;

	default:
	  DIE ("unrecognized operation");
	  break;
	}
    }

  if (stack_depth > 1)
    DIE ("multiple values left on stack");

  if (stack_depth > 0)		/* stack_depth == 1 */
    {
      if (*input != NULL)
	DIE ("multiple values left on stack");

      /* Could handle this if it ever actually happened.  */
      DIE ("relative expression computed constant");
    }

  return head;

 underflow:
  if (*input == NULL)
    DIE ("stack underflow");
  else
    DIE ("cannot handle location expression");

 fail:
  return lose (failure, expr, i);
}


/* Translate a C fragment for the location expression, using *INPUT
   as the starting location, begin from scratch if *INPUT is null.
   If DW_OP_fbreg is used, it may have a subfragment computing from
   the FB_ATTR location expression.

   On errors, exit and never return (XXX ?).  On success, return the
   first fragment created, which is also chained onto (*INPUT)->next.
   *INPUT is then updated with the new tail of that chain.  */

struct location *
c_translate_location (struct obstack *pool,
		      int indent, Dwarf_Addr dwbias,
		      Dwarf_Attribute *loc_attr, Dwarf_Addr address,
		      struct location **input, Dwarf_Attribute *fb_attr)
{
  Dwarf_Loc *expr;
  size_t len;
  switch (dwarf_addrloclists (loc_attr, address - dwbias, &expr, &len, 1))
    {
    case 1:			/* Should always happen.  */
      if (len == 0)
	goto inaccessible;
      break;

    default:			/* Shouldn't happen.  */
    case -1:
      error (2, 0, "dwarf_addrloclists (form %#x): %s",
	     dwarf_whatform (fb_attr), dwarf_errmsg (-1));
      return NULL;

    case 0:			/* Shouldn't happen.  */
    inaccessible:
      error (2, 0, "not accessible at this address");
      return NULL;
    }

  ++indent;
  switch (*input == NULL ? loc_address : (*input)->type)
    {
    case loc_address:
      /* We have a previous address computation.
	 This expression will compute starting with that on the stack.  */
      return location_from_address (pool, indent, dwbias, expr, len, address,
				    input, fb_attr);

    case loc_noncontiguous:
    case loc_register:
      /* The starting point is not an address computation, but a
	 register.  We can only handle limited computations from here.  */
      return location_relative (pool, indent, dwbias, expr, len, address,
				input, fb_attr);

    case loc_final:		/* Bogus caller.  */
    default:
      abort ();
      break;
    }

  return NULL;
}


/* Emit "uintNN_t TARGET = ...;".  */
static bool
emit_base_fetch (struct obstack *pool, Dwarf_Word byte_size,
		 const char *target, bool decl, struct location *loc)
{
  if (decl)
    obstack_printf (pool, "uint%" PRIu64 "_t ", byte_size * 8);

  switch (loc->type)
    {
    case loc_address:
      if (byte_size != 0 && byte_size != (Dwarf_Word) -1)
	obstack_printf (pool, "%s = deref (%" PRIu64 ", addr); ",
			target, byte_size);
      else
	obstack_printf (pool, "%s = deref (sizeof %s, addr); ",
			target, target);
      return true;

    case loc_register:
      obstack_printf (pool, "%s = fetch_register (%u);", target, loc->regno);
      break;

    case loc_noncontiguous:
      /* Could be handled if it ever happened.  */
      error (2, 0, _("noncontiguous locations not supported"));
      break;

    default:
      abort ();
      break;
    }

  return false;
}

/* Translate a fragment to dereference the given pointer type,
   where *INPUT is the location of the pointer with that type.

   We chain on a loc_address program that yields this pointer value
   (i.e. the location of what it points to).  */

void
c_translate_pointer (struct obstack *pool, int indent,
		     Dwarf_Addr dwbias __attribute__ ((unused)),
		     Dwarf_Die *typedie, struct location **input)
{
  obstack_printf (pool, "%*s{ ", (indent + 2) * 2, "");

  bool deref = false;
  Dwarf_Attribute attr_mem;
  Dwarf_Word byte_size;
  if (dwarf_attr_integrate (typedie, DW_AT_byte_size, &attr_mem) == NULL)
    {
      obstack_printf (pool, "uintptr_t ");
      emit_base_fetch (pool, 0, "tmp", false, *input);
    }
  else if (dwarf_formudata (&attr_mem, &byte_size) != 0)
    error (2, 0,
	   _("cannot get byte_size attribute for type %s: %s"),
	   dwarf_diename_integrate (typedie) ?: "<anonymous>",
	   dwarf_errmsg (-1));
  else
    deref = emit_base_fetch (pool, byte_size, "tmp", true, *input);

  obstack_printf (pool, " addr = tmp; }\n");

  struct location *loc = new_synthetic_loc (pool, *input, deref);
  (*input)->next = loc;
  *input = loc;
}

/* Determine the byte size of a base type.  */
static Dwarf_Word
base_byte_size (Dwarf_Die *typedie)
{
  Dwarf_Attribute attr_mem;
  Dwarf_Word size;
  if (dwarf_attr_integrate (typedie, DW_AT_byte_size, &attr_mem) != NULL
      && dwarf_formudata (&attr_mem, &size) == 0)
    return size;

  error (2, 0,
	 _("cannot get byte_size attribute for type %s: %s"),
	 dwarf_diename_integrate (typedie) ?: "<anonymous>",
	 dwarf_errmsg (-1));
  return -1;
}

/* Emit a code fragment like:
	{ uintNN_t tmp = ...; S1 S2 S3, tmp, B0, Bn); }
*/
static void
emit_bitfield (struct obstack *pool, int indent,
	       Dwarf_Die *die, Dwarf_Word byte_size, struct location *loc,
	       const char *s1, const char *s2, const char *s3)
{
  Dwarf_Word bit_offset, bit_size;
  Dwarf_Attribute attr_mem;
  if (dwarf_attr_integrate (die, DW_AT_bit_offset, &attr_mem) == NULL
      || dwarf_formudata (&attr_mem, &bit_offset) != 0
      || dwarf_attr_integrate (die, DW_AT_bit_size, &attr_mem) == NULL
      || dwarf_formudata (&attr_mem, &bit_size) != 0)
    error (2, 0, _("cannot get bit field parameters: %s"),
	   dwarf_errmsg (-1));

  /* Emit "{ uintNN_t tmp = ...;" to fetch the base type.  */

  obstack_printf (pool, "%*s{ ", indent * 2, "");
  emit_base_fetch (pool, byte_size, "tmp", true, loc);

  obstack_printf (pool, "%s%s%s, tmp, %" PRIu64 ", %" PRIu64 "); }\n",
		  s1, s2, s3, bit_offset, bit_size);
}

/* Translate a fragment to fetch the value of variable or member DIE
   at the *INPUT location and store it in variable TARGET.  */

void
c_translate_fetch (struct obstack *pool, int indent,
		   Dwarf_Addr dwbias __attribute__ ((unused)),
		   Dwarf_Die *die, Dwarf_Attribute *typeattr,
		   struct location **input, const char *target)
{
  ++indent;

  Dwarf_Attribute size_attr;
  Dwarf_Word byte_size;
  if (dwarf_attr_integrate (die, DW_AT_byte_size, &size_attr) == NULL
      || dwarf_formudata (&size_attr, &byte_size) != 0)
    {
      Dwarf_Die basedie;
      if (dwarf_formref_die (typeattr, &basedie) == NULL)
	error (2, 0, _("cannot get type of field: %s"), dwarf_errmsg (-1));
      byte_size = base_byte_size (&basedie);
    }

  bool deref = false;
  if (dwarf_hasattr_integrate (die, DW_AT_bit_offset))
    /* This is a bit field.  */
    emit_bitfield (pool, indent, die, byte_size, *input,
		   "fetch_bitfield (", target, "");
  else
    switch (byte_size)
      {
      case 1:
      case 2:
      case 4:
      case 8:
	obstack_printf (pool, "%*s", indent * 2, "");
	deref = emit_base_fetch (pool, byte_size, target, false, *input);
	obstack_printf (pool, "\n");
	break;

      default:
	/* Could handle this generating call to memcpy equivalent.  */
	error (2, 0, _("fetch is larger than base integer types"));
	break;
      }

  struct location *loc = new_synthetic_loc (pool, *input, deref);
  loc->type = loc_final;
  (*input)->next = loc;
  *input = loc;
}

void
c_translate_addressof (struct obstack *pool, int indent,
		       Dwarf_Addr dwbias __attribute__ ((unused)),
		       Dwarf_Die *die,
		       Dwarf_Attribute *typeattr  __attribute__ ((unused)),
		       struct location **input, const char *target)
{
  ++indent;

  if (dwarf_hasattr_integrate (die, DW_AT_bit_offset))
    error (2, 0, _("cannot take the address of a bit field"));

  switch ((*input)->type)
    {
    case loc_address:
      obstack_printf (pool, "%*s%s = addr;\n", indent * 2, "", target);
      (*input)->next = new_synthetic_loc (pool, *input, false);
      (*input)->next->type = loc_final;
      break;

    case loc_register:
      error (2, 0, _("cannot take address of object in register"));
      break;
    case loc_noncontiguous:
      error (2, 0, _("cannot take address of noncontiguous object"));
      break;

    default:
      abort();
      break;
    }
}


/* Determine the element stride of an array type.  */
static Dwarf_Word
array_stride (Dwarf_Die *typedie)
{
  Dwarf_Attribute attr_mem;
  if (dwarf_attr_integrate (typedie, DW_AT_stride_size, &attr_mem) != NULL)
    {
      Dwarf_Word stride;
      if (dwarf_formudata (&attr_mem, &stride) == 0)
	return stride;
      error (2, 0, _("cannot get stride_size attribute array type %s: %s"),
	     dwarf_diename_integrate (typedie) ?: "<anonymous>",
	     dwarf_errmsg (-1));
    }

  Dwarf_Die die_mem;
  if (dwarf_attr_integrate (typedie, DW_AT_type, &attr_mem) == NULL
      || dwarf_formref_die (&attr_mem, &die_mem) == NULL)
    error (2, 0, _("cannot get element type of array type %s: %s"),
	   dwarf_diename_integrate (typedie) ?: "<anonymous>",
	   dwarf_errmsg (-1));

  if (dwarf_attr_integrate (&die_mem, DW_AT_byte_size, &attr_mem) != NULL)
    {
      Dwarf_Word stride;
      if (dwarf_formudata (&attr_mem, &stride) == 0)
	return stride;
      error (2, 0,
	     _("cannot get byte_size attribute for array element type %s: %s"),
	     dwarf_diename_integrate (&die_mem) ?: "<anonymous>",
	     dwarf_errmsg (-1));
    }

  error (2, 0, _("confused about array element size"));
  return 0;
}

void
c_translate_array (struct obstack *pool, int indent,
		   Dwarf_Addr dwbias __attribute__ ((unused)),
		   Dwarf_Die *typedie, struct location **input,
		   const char *idx, Dwarf_Word const_idx)
{
  ++indent;

  Dwarf_Word stride = array_stride (typedie);

  struct location *loc = *input;
  while (loc->type == loc_noncontiguous)
    {
      if (idx != NULL)
	error (2, 0, _("cannot dynamically index noncontiguous array"));
      else
	{
	  Dwarf_Word offset = const_idx * stride;
	  struct location *piece = loc->pieces;
	  while (piece != NULL && offset >= piece->byte_size)
	    {
	      offset -= piece->byte_size;
	      piece = piece->next;
	    }
	  if (piece == NULL)
	    error (2, 0, _("constant index is outside noncontiguous array"));
	  if (offset % stride != 0)
	    error (2, 0, _("noncontiguous array splits elements"));
	  const_idx = offset / stride;
	  loc = piece;
	}
    }

  switch (loc->type)
    {
    case loc_address:
      ++indent;
      if (idx != NULL)
	obstack_printf (pool, "%*saddr += %s * " UFORMAT ";\n",
			indent * 2, "", idx, stride);
      else
	obstack_printf (pool, "%*saddr += " UFORMAT " * " UFORMAT ";\n",
			indent * 2, "", const_idx, stride);
      loc = new_synthetic_loc (pool, loc, false);
      break;

    case loc_register:
      error (2, 0, _("cannot index array stored in a register"));
      break;

    default:
      abort();
      break;
    }

  (*input)->next = loc;
  *input = (*input)->next;
}


/* Emitting C code for finalized fragments.  */


#define emit(fmt, ...) fprintf (out, fmt, ## __VA_ARGS__)

/* Open a block with a comment giving the original DWARF expression.  */
static void
emit_header (FILE *out, struct location *loc, unsigned int hindent)
{
  if (loc->ops == NULL)
    emit ("%*s{ // synthesized\n", hindent * 2, "");
  else
    {
      emit ("%*s{ // DWARF expression:", hindent * 2, "");
      for (size_t i = 0; i < loc->nops; ++i)
	{
	  emit (" %#x", loc->ops[i].atom);
	  if (loc->ops[i].number2 == 0)
	    {
	      if (loc->ops[i].number != 0)
		emit ("(%" PRId64 ")", loc->ops[i].number);
	    }
	  else
	    emit ("(%" PRId64 ",%" PRId64 ")",
		  loc->ops[i].number, loc->ops[i].number2);
	}
      emit ("\n");
    }
}

/* Emit a code fragment to assign the target variable to a register value.  */
static void
emit_loc_register (FILE *out, struct location *loc, unsigned int indent,
		   const char *target)
{
  assert (loc->type == loc_register);

  emit ("%*s%s = fetch_register (%u);\n",
	indent * 2, "", target, loc->regno);
}

/* Emit a code fragment to assign the target variable to an address.  */
static void
emit_loc_address (FILE *out, struct location *loc, unsigned int indent,
		  const char *target)
{
  assert (loc->type == loc_address);

  if (loc->address.stack_depth == 0)
    /* Synthetic program.  */
    emit ("%s", loc->address.program);
  else
    {
      emit ("%*s%s " STACKFMT, (indent + 1) * 2, "", STACK_TYPE, 0);
      for (unsigned int i = 1; i < loc->address.stack_depth; ++i)
	emit (", " STACKFMT, i);
      emit (";\n");

      emit ("%s%*s%s = " STACKFMT ";\n", loc->address.program,
	    (indent + 1) * 2, "", target, 0);
    }
}

/* Emit a code fragment to declare the target variable and
   assign it to an address-sized value.  */
static void
emit_loc_value (FILE *out, struct location *loc, unsigned int indent,
		const char *target, bool declare)
{
  if (declare)
    emit ("%*s%s %s;\n", indent * 2, "", STACK_TYPE, target);

  emit_header (out, loc, indent);

  switch (loc->type)
    {
    default:
      abort ();
      break;

    case loc_register:
      emit_loc_register (out, loc, indent, target);
      break;

    case loc_address:
      if (loc->address.frame_base != NULL)
	emit_loc_value (out, loc->address.frame_base, indent,
			"frame_base", true);
      emit_loc_address (out, loc, indent, target);
      break;
    }

  emit ("%*s}\n", indent * 2, "");
}

bool
c_emit_location (FILE *out, struct location *loc, int indent)
{
  emit ("%*s{\n", indent * 2, "");

  bool deref = false;
  for (bool declare_addr = true; loc->next != NULL; loc = loc->next)
    switch (loc->type)
      {
      case loc_address:
	/* Emit the program fragment to calculate the address.  */
	emit_loc_value (out, loc, indent + 1, "addr", declare_addr);
	declare_addr = false;
	deref = deref || loc->address.used_deref;
	break;

      case loc_register:
      case loc_noncontiguous:
	/* These don't produce any code directly.
	   The next address/final record incorporates the value.  */
	break;

      case loc_final:		/* Should be last in chain!  */
      default:
	abort ();
	break;
      }

  if (loc->type != loc_final)	/* Unfinished chain.  */
    abort ();

  emit ("%s%*s}\n", loc->address.program, indent * 2, "");

  return deref;
}

#undef emit
