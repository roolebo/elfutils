dnl Process this file with autoconf to produce a configure script.
dnl Configure input file for elfutils.                     -*-autoconf-*-
dnl
dnl Copyright (C) 1996-2002, 2003, 2004, 2005, 2006, 2007 Red Hat, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, version 2.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software Foundation,
dnl Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
dnl
AC_INIT([Red Hat elfutils],[0.126],[http://bugzilla.redhat.com/bugzilla/],
	[elfutils])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_FILES([config/Makefile])

AC_COPYRIGHT([Copyright (C) 1996-2003, 2004, 2005, 2006, 2007 Red Hat, Inc.])
AC_PREREQ(2.59)			dnl Minimum Autoconf version required.

AM_INIT_AUTOMAKE([gnits 1.7])
AM_MAINTAINER_MODE

dnl Unique ID for this build.
MODVERSION="Build on $(hostname) $(date +%FT%R:%S%z)"
AC_SUBST([MODVERSION])
AC_DEFINE_UNQUOTED(MODVERSION, "$MODVERSION")
AH_TEMPLATE([MODVERSION], [Identifier for modules in the build.])

AC_CONFIG_SRCDIR([libelf/libelf.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([m4/Makefile])

dnl The RPM spec file.  We substitute a few values in the file.
AC_CONFIG_FILES([elfutils.spec:config/elfutils.spec.in])


AC_CANONICAL_HOST

AC_ARG_ENABLE([tls],
AS_HELP_STRING([--enable-tls], [enable use of thread local storage]),
AC_DEFINE(USE_TLS))
AH_TEMPLATE([USE_TLS], [Defined if thread local storage should be used.])

dnl Add all the languages for which translations are available.
ALL_LINGUAS=

AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_YACC
AM_PROG_LEX

AC_CACHE_CHECK([for gcc with C99 support], ac_cv_c99, [dnl
old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -std=gnu99"
AC_COMPILE_IFELSE([dnl
int foo (int a) { for (int i = 0; i < a; ++i) if (i % 4) break; int s = a; }],
		  ac_cv_c99=yes, ac_cv_c99=no)
CFLAGS="$old_CFLAGS"])
AS_IF([test "x$ac_cv_c99" != xyes],
      AC_MSG_ERROR([gcc with C99 support required]))

LOCALEDIR=$datadir
AC_SUBST(LOCALEDIR)
AC_DEFINE_UNQUOTED(LOCALEDIR, "$LOCALEDIR")
AH_TEMPLATE([LOCALEDIR], [Directory to place translation files in.])

DATADIRNAME=$datadir
AC_SUBST(DATADIRNAME)

dnl This test must come as early as possible after the compiler configuration
dnl tests, because the choice of the file model can (in principle) affect
dnl whether functions and headers are available, whether they work, etc.
AC_SYS_LARGEFILE

dnl Enable the linker to be build as a native-only linker.  By default it
dnl can handle all architectures but this comes at a cost.  A native
dnl will be slightly faster, small, and has fewer dependencies.
native_ld=no
AC_ARG_ENABLE([generic],
AS_HELP_STRING([--disable-generic], [do not build generic linker]), [dnl
if test "$enable_generic" = no; then
  case "$host_cpu" in
   i?86)
    AC_DEFINE(NATIVE_ELF, 32)
    native_ld=yes
    base_cpu=i386
    ;;
   *)
    AC_MSG_ERROR([no machine-specific linker for this configuration available])
    ;;
  esac
fi])
AH_TEMPLATE([NATIVE_ELF],
[Define to 32 or 64 if a specific implementation is wanted.])
AM_CONDITIONAL(NATIVE_LD, test "$native_ld" = yes)
dnl The automake generated Makefile cannot deal with macros in the name
dnl of files if at any time there is no such file, even if the filename
dnl would not be used.
AS_IF([test -z "$base_cpu"], [base_cpu=none])
AC_SUBST(base_cpu)
dnl Support to work around automake's inflexible dependency generation.
dnl See src/Makefile.am for more information.
AM_CONDITIONAL(NEVER, false)

dnl Enable debugging via mudflap.  This option will cause most libraries
dnl to be built as archives which are statically linked into the applications.
dnl All code, as far as possible, is compiled instrumented to catch all
dnl the bugs valgrind is able to catch.
use_mudflap=no
AC_ARG_ENABLE([mudflap],
AS_HELP_STRING([--enable-mudflap],
[build binaries with mudflap instrumentation]), [dnl
if test "x$enable_mudflap" = xyes; then
  # Check whether the compiler support -fmudflap.
  old_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -fmudflap"
  AC_TRY_COMPILE([], [], use_mudflap=yes, use_mudflap=fail)
  CFLAGS="$old_CFLAGS"
fi])
if test "$use_mudflap" = fail; then
  AC_MSG_FAILURE([--enable-mudflap requires a compiler which understands this option])
fi
AM_CONDITIONAL(MUDFLAP, test "$use_mudflap" = yes)

# Enable gprof suport.
AC_ARG_ENABLE([gprof],
AC_HELP_STRING([--enable-gprof],
[build binaries with gprof support]), [use_gprof=yes], [use_gprof=no])
if test "$use_gprof" = yes; then
  CFLAGS="$CFLAGS -pg"
  LDFLAGS="$LDFLAGS -pg"
fi
AM_CONDITIONAL(GPROF, test "$use_gprof" = yes)

# Enable gcov suport.
AC_ARG_ENABLE([gcov],
AC_HELP_STRING([--enable-gcov],
[build binaries with gcov support]), [use_gcov=yes], [use_gcov=no])
if test "$use_gcov" = yes; then
  CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
  LDFLAGS="$LDFLAGS -fprofile-arcs"
fi
AM_CONDITIONAL(GCOV, test "$use_gcov" = yes)

AM_CONDITIONAL(BUILD_STATIC, [dnl
test "$use_mudflap" = yes -o "$use_gprof" = yes -o "$use_gcov" = yes])

AC_ARG_ENABLE([tests-rpath],
AC_HELP_STRING([--enable-tests-rpath], [build $ORIGIN-using rpath into tests]),
 	       [tests_use_rpath=yes], [tests_use_rpath=no])
AM_CONDITIONAL(TESTS_RPATH, test "$tests_use_rpath" = yes)

LIBEBL_SUBDIR="$PACKAGE"
AC_ARG_ENABLE([libebl-subdir],
AS_HELP_STRING([--enable-libebl-subdir=DIR],
[install libebl_CPU modules in $(libdir)/DIR]), [dnl
LIBEBL_SUBDIR="$enable_libebl_subdir"])
AC_SUBST([LIBEBL_SUBDIR])
AC_DEFINE_UNQUOTED(LIBEBL_SUBDIR, "$LIBEBL_SUBDIR")
AH_TEMPLATE([LIBEBL_SUBDIR], [$libdir subdirectory containing libebl modules.])

dnl The directories with content.

dnl Documentation.
dnl Commented out for now.
dnl AC_CONFIG_FILES([doc/Makefile])

dnl Support library.
AC_CONFIG_FILES([lib/Makefile])

dnl ELF library.
AC_CONFIG_FILES([libelf/Makefile libelf-po/Makefile.in])

dnl Higher-level ELF support library.
AC_CONFIG_FILES([libebl/Makefile])

dnl DWARF library.
AC_CONFIG_FILES([libdw/Makefile])

dnl Higher-level DWARF support library.
AC_CONFIG_FILES([libdwfl/Makefile])

dnl CPU handling library.
AC_CONFIG_FILES([libcpu/Makefile])

dnl Assembler library.
AM_CONDITIONAL(HAVE_LIBASM, true)dnl Used in tests/Makefile.am, which see.
AC_CONFIG_FILES([libasm/Makefile])

dnl CPU-specific backend libraries.
AC_CONFIG_FILES([backends/Makefile])

dnl Tools.
AC_CONFIG_FILES([src/Makefile po/Makefile.in])

dnl Test suite.
AM_CONDITIONAL(STANDALONE, false)dnl Used in tests/Makefile.am, which see.
AC_CONFIG_FILES([tests/Makefile])

# Get the definitions necessary to create the Makefiles in the po
# subdirectories.  This is a small subset of the gettext rules.
AC_SUBST(USE_NLS, yes)
AM_PO_SUBDIRS


dnl Test of the config.h file.  We hide all kinds of configuration magic
dnl in there.
AH_BOTTOM([
/* Eventually we will allow multi-threaded applications to use the
   libraries.  Therefore we will add the necessary locking although
   the macros used expand to nothing for now.  */
#define lock_lock(lock) ((void) (lock))
#define rwlock_define(class,name) class int name
#define rwlock_init(lock) ((void) (lock))
#define rwlock_fini(lock) ((void) (lock))
#define rwlock_rdlock(lock) ((void) (lock))
#define rwlock_wrlock(lock) ((void) (lock))
#define rwlock_unlock(lock) ((void) (lock))
#define tls_key_t void *
#define key_create(keyp, freefct) (1)
#define getspecific(key) key
#define setspecific(key,val) key = val
#define once_define(class,name) class int name
#define once_execute(name,fct) \
  do {									      \
    if (name == 0)							      \
      fct ();								      \
    name = 1;								      \
  } while (0)

/* gettext helper macro.  */
#define N_(Str) Str

/* Compiler-specific definitions.  */
#define strong_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((alias (#name)));

#ifdef __i386__
# define internal_function __attribute__ ((regparm (3), stdcall))
#else
# define internal_function /* nothing */
#endif

#define internal_strong_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((alias (#name))) internal_function;

#define attribute_hidden \
  __attribute__ ((visibility ("hidden")))

/* Define ALLOW_UNALIGNED if the architecture allows operations on
   unaligned memory locations.  */
#if defined __i386__ || defined __x86_64__
# define ALLOW_UNALIGNED	1
#else
# define ALLOW_UNALIGNED	0
#endif

#define unlikely(expr) __builtin_expect (expr, 0)
#define likely(expr) __builtin_expect (expr, 1)

#define obstack_calloc(ob, size) \
  ({ size_t _s = (size); memset (obstack_alloc (ob, _s), '\0', _s); })
#define obstack_strdup(ob, str) \
  ({ const char *_s = (str); obstack_copy0 (ob, _s, strlen (_s)); })
#define obstack_strndup(ob, str, n) \
  ({ const char *_s = (str); obstack_copy0 (ob, _s, strnlen (_s, n)); })

#if __STDC_VERSION__ >= 199901L
# define flexarr_size /* empty */
#else
# define flexarr_size 0
#endif

/* Calling conventions.  */
#ifdef __i386__
# define CALLING_CONVENTION regparm (3), stdcall
# define AND_CALLING_CONVENTION , regparm (3), stdcall
#else
# define CALLING_CONVENTION
# define AND_CALLING_CONVENTION
#endif

/* Avoid PLT entries.  */
#ifdef PIC
# define INTUSE(name) _INTUSE(name)
# define _INTUSE(name) __##name##_internal
# define INTDEF(name) _INTDEF(name)
# define _INTDEF(name) \
  extern __typeof__ (name) __##name##_internal __attribute__ ((alias (#name)));
# define INTDECL(name) _INTDECL(name)
# define _INTDECL(name) \
  extern __typeof__ (name) __##name##_internal attribute_hidden;
#else
# define INTUSE(name) name
# define INTDEF(name) /* empty */
# define INTDECL(name) /* empty */
#endif

/* This macro is used by the tests conditionalize for standalone building.  */
#define ELFUTILS_HEADER(name) <lib##name.h>
])

AC_OUTPUT
