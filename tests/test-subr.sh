#! /bin/sh
# Copyright (C) 2005 Red Hat, Inc.
#
# This program is Open Source software; you can redistribute it and/or
# modify it under the terms of the Open Software License version 1.0 as
# published by the Open Source Initiative.
#
# You should have received a copy of the Open Software License along
# with this program; if not, you may obtain a copy of the Open Software
# License version 1.0 from http://www.opensource.org/licenses/osl.php or
# by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
# 3001 King Ranch Road, Ukiah, CA 95482.

# This file is sourced by ". $srcdir/test-subr.sh" at the start of
# each test script.  It defines some functions they use and sets up
# canonical sh state for test runs.

set -e

remove_files=
trap 'rm -f $remove_files' 0

tempfiles()
{
  remove_files="$remove_files $*"
}

testfiles()
{
  for file; do
    bunzip2 -c $srcdir/${file}.bz2 > ${file} 2>/dev/null || exit 77
    remove_files="$remove_files $file"
  done
}

testrun_out()
{
  outfile="$1"
  shift
  remove_files="$remove_files $outfile"
  testrun "$@" > $outfile 2>&1 || :
}

testrun_compare()
{
  outfile="${1##*/}.out"
  testrun_out $outfile "$@"
  diff -Bbu $outfile -
  # diff's exit status will kill the script.
}

test_cleanup()
{
  rm -f $remove_files
  remove_files=
}

# See test-wrapper.sh, which sets the environment for this.
testrun()
{
  ${elfutils_testrun}_testrun "$@"
}

built_testrun()
{
  LD_LIBRARY_PATH="${built_library_path}${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH"\
  "$@"
}

installed_testrun()
{
  program="$1"
  shift
  case "$program" in
  ./*)
    if [ "x$elfutils_tests_rpath" != xno ]; then
      echo >&2 installcheck not possible with --enable-tests-rpath
      exit 77
    fi
    ;;
  ../*)
    program=`program_transform ${program##*/}`
    ;;
  esac
  $program ${1+"$@"}
}

program_transform()
{
  echo "$*" | sed "${program_transform_name}"
}
