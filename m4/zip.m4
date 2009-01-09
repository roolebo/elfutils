dnl Test for either zlib or bzlib.
dnl Defines --with-$1lib argument, $2LIB automake conditional,
dnl and sets AC_DEFINE(USE_$2LIB) and LIBS.

AC_DEFUN([eu_ZIPLIB], [with_[$1]lib=default
AC_ARG_WITH([[$1]lib],
AC_HELP_STRING([--with-[$1]lib], [support g[$1]ip compression in libdwfl]))
if test $with_[$1]lib != no; then
  AC_SEARCH_LIBS([$4], [$3], [with_[$1]lib=yes],
  	         [test $with_[$1]lib = default ||
		  AC_MSG_ERROR([missing -l[$3] for --with-[$1]lib])])
fi
AM_CONDITIONAL([$2]LIB, test $with_[$1]lib = yes)
if test $with_[$1]lib = yes; then
  AC_DEFINE(USE_[$2]LIB)
fi
AH_TEMPLATE(USE_[$2]LIB, [Support $5 decompression via -l$3.])])
