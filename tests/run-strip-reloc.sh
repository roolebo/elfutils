#! /bin/sh
# Copyright (C) 2011 Red Hat, Inc.
# This file is part of Red Hat elfutils.
#
# Red Hat elfutils is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# Red Hat elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with Red Hat elfutils; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.
#
# Red Hat elfutils is an included package of the Open Invention Network.
# An included package of the Open Invention Network is a package for which
# Open Invention Network licensees cross-license their patents.  No patent
# license is granted, either expressly or impliedly, by designation as an
# included package.  Should you wish to participate in the Open Invention
# Network licensing program, please visit www.openinventionnetwork.com
# <http://www.openinventionnetwork.com>.

. $srcdir/test-subr.sh

testfiles hello_i386.ko hello_x86_64.ko hello_ppc64.ko hello_s390.ko

status=0
runtest() {
  infile=$1
  is_ET_REL=$2
  outfile1=out.stripped1
  debugfile1=out.debug1
  outfile2=out.stripped2
  debugfile2=out.debug2

  testrun ../src/strip -o $outfile1 -f $debugfile1 $infile ||
  { echo "*** failure strip $infile"; status=1; }

  testrun ../src/strip --reloc-debug-sections -o $outfile2 \
	-f $debugfile2 $infile ||
  { echo "*** failure strip --reloc-debug-sections $infile"; status=1; }

  # shouldn't make any difference for stripped files.
  testrun ../src/readelf -a $outfile1 > readelf.out ||
  { echo "*** failure readelf -a outfile1 $infile"; status=1; }

  testrun_compare ../src/readelf -a $outfile2 < readelf.out ||
  { echo "*** failure compare stripped files $infile"; status=1; }

  # debug files however should be smaller, when ET_REL.
  SIZE1=$(stat -c%s $debugfile1)
  SIZE2=$(stat -c%s $debugfile2)
  test \( \( $is_ET_REL -eq 1 \) -a \( $SIZE1 -gt $SIZE2 \) \) \
	-o \( \( $is_ET_REL -eq 0 \) -a \( $SIZE1 -eq $SIZE2 \) \) ||
  { echo "*** failure --reloc-debug-sections not smaller $infile"; status=1; }

  # Strip of DWARF section lines, offset will not match.
  # Everything else should match.
  testrun ../src/readelf -w $debugfile1 \
	| grep -v ^DWARF\ section > readelf.out1 ||
  { echo "*** failure readelf -w debugfile1 $infile"; status=1; }

  testrun ../src/readelf -w $debugfile2 \
	| grep -v ^DWARF\ section > readelf.out2 ||
  { echo "*** failure readelf -w debugfile2 $infile"; status=1; }

  testrun_compare cat readelf.out1 < readelf.out2 ||
  { echo "*** failure readelf -w compare $infile"; status=1; }

  rm -f $outfile1 $debugfile1 $outfile2 $debugfile2 readelf.out*
}

# Most simple hello world kernel module for various architectures.
# ::::::::::::::
# Makefile
# ::::::::::::::
# obj-m	:= hello.o
# hello-y := init.o exit.o
# 
# all:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
# ::::::::::::::
# init.c
# ::::::::::::::
# #include <linux/kernel.h>
# #include <linux/module.h>
# 
# int init_module(void)
# {
#   printk(KERN_INFO "Hello, world!\n");
#   return 0;
# }
# ::::::::::::::
# exit.c
# ::::::::::::::
# #include <linux/kernel.h>
# #include <linux/module.h>
# 
# void cleanup_module()
# {
#   printk(KERN_INFO "Goodbye, World!\n");
# }
runtest hello_i386.ko 1
runtest hello_x86_64.ko 1
runtest hello_ppc64.ko 1
runtest hello_s390.ko 1

# self test, shouldn't impact non-ET_REL files at all.
runtest ../src/strip 0
runtest ../src/strip.o 1

exit $status
