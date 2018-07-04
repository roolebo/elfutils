#!/bin/bash

# Any error is fatal
set -e

# We take one arguent, the version (e.g. 0.173)
if [ $# -ne 1 ]; then
  echo "$0 <version> (e.g. 0.169)"
  exit 1
fi

VERSION="$1"

# Check we are in the build dir already configured.
ELFUTILS_VERSION=$(echo $VERSION | cut -f2 -d\.)
grep $ELFUTILS_VERSION version.h \
	|| (echo "Must be run in configured build dir for $VERSION"; exit -1)

make dist

mkdir $VERSION
cp elfutils-$VERSION.tar.bz2 $VERSION/
cd $VERSION/
gpg -b elfutils-$VERSION.tar.bz2
cd ..
scp -r $VERSION sourceware.org:/sourceware/ftp/pub/elfutils/

ssh sourceware.org "(cd /sourceware/ftp/pub/elfutils \
  && ln -sf $VERSION/elfutils-$VERSION.tar.bz2 elfutils-latest.tar.bz2 \
  && ln -sf $VERSION/elfutils-$VERSION.tar.bz2.sig elfutils-latest.tar.bz2.sig \
  && ls -lah elfutils-latest*)"
