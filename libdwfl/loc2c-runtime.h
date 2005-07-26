/* target operations */

#include <linux/types.h>
#define intptr_t long
#define uintptr_t unsigned long


/* These three macro definitions are generic, just shorthands
   used by the generated code.  */

#define op_abs(x)	(x < 0 ? -x : x)

#define fetch_bitfield(target, base, higherbits, nbits)			      \
  target = (((base) >> (sizeof (base) * 8 - (higherbits) - (nbits)))	      \
	    & (((__typeof (base)) 1 << (nbits)) - 1))

#define store_bitfield(target, base, higherbits, nbits)			      \
  target = (target							      \
	    &~ ((((__typeof (base)) 1 << (nbits)) - 1)			      \
		<< (sizeof (base) * 8 - (higherbits) - (nbits)))	      \
	    | ((__typeof (base)) (value)				      \
	       << (sizeof (base) * 8 - (higherbits) - (nbits))))


/* These operations are target-specific.  */
#include <asm/uaccess.h>

#define fetch_register(regno) ((intptr_t) regs->dwarf_register_##regno)

#if defined __i386__

#define dwarf_register_0 eax
#define dwarf_register_1 ecx
#define dwarf_register_2 edx
#define dwarf_register_3 ebx
#define dwarf_register_4 esp
#define dwarf_register_5 ebp
#define dwarf_register_6 esi
#define dwarf_register_7 edi

#elif defined __x86_64__

#define dwarf_register_0 eax
#define dwarf_register_1 edx
#define dwarf_register_2 ecx
#define dwarf_register_3 ebx
#define dwarf_register_4 esi
#define dwarf_register_5 edi
#define dwarf_register_6 ebp
#define dwarf_register_7 esp
#define dwarf_register_8 r8
#define dwarf_register_9 r9
#define dwarf_register_10 r10
#define dwarf_register_11 r11
#define dwarf_register_12 r12
#define dwarf_register_13 r13
#define dwarf_register_14 r14
#define dwarf_register_15 r15

#elif defined __powerpc__

#undef fetch_register
#define fetch_register(regno) ((intptr_t) regs->gpr[regno])

#endif

#if defined __i386__ || defined __x86_64__

#define deref(size, addr)						      \
  ({									      \
    int _bad = 0;							      \
    u8 _b; u16 _w; u32 _l; u64 _q;					      \
    intptr_t _v;							      \
    switch (size)							      \
      {									      \
      case 1: __get_user_asm(_b,addr,_bad,"b","b","=q",1); _v = _b; break;    \
      case 2: __get_user_asm(_w,addr,_bad,"w","w","=r",1); _v = _w; break;    \
      case 4: __get_user_asm(_l,addr,_bad,"l","","=r",1); _v = _l; break;     \
      case 8: __get_user_asm(_q,addr,_bad,"q","","=r",1); _v = _q; break;     \
      default: _v = __get_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
    _v;									      \
  })

#elif defined __powerpc64__

#define deref(size, addr)						      \
  ({									      \
    int _bad = 0;							      \
    intptr_t _v;							      \
    switch (size)							      \
      {									      \
      case 1: __get_user_asm(_v,addr,_bad,"lbz",1); break;		      \
      case 2: __get_user_asm(_v,addr,_bad,"lhz",1); break;		      \
      case 4: __get_user_asm(_v,addr,_bad,"lwz",1); break;		      \
      case 8: __get_user_asm(_v,addr,_bad,"ld",1); break;		      \
      default: _v = __get_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
    _v;									      \
  })

#elif defined __powerpc__

#define deref(size, addr)						      \
  ({									      \
    int _bad = 0;							      \
    intptr_t _v;							      \
    switch (size)							      \
      {									      \
      case 1: __get_user_asm(_v,addr,_bad,"lbz"); break;		      \
      case 2: __get_user_asm(_v,addr,_bad,"lhz"); break;		      \
      case 4: __get_user_asm(_v,addr,_bad,"lwz"); break;		      \
      case 8: __get_user_asm(_v,addr,_bad,"ld"); break;			      \
      default: _v = __get_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
    _v;									      \
  })

#endif
