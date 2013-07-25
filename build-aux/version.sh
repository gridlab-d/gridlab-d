#!/bin/sh
# This file is used by autoconf to generate the version string.
# It is assumed that this script is run from the top-level srcdir.
FIL="core/version.h"
MAJ=`cat $FIL | sed -rn 's/#define REV_MAJOR ([0-9]+).*/\1/p'`
MIN=`cat $FIL | sed -rn 's/#define REV_MINOR ([0-9]+).*/\1/p'`
PAT=`cat $FIL | sed -rn 's/#define REV_PATCH ([0-9]+).*/\1/p'`
BRA=`cat $FIL | sed -rn 's/#define BRANCH "([A-Za-z0-9]+)".*/\1/p'`
SVN=`svnversion | sed 's/^exported$/0/;s/[^A-Za-z0-9]/+/g' | tr -d '\n'`
case $1 in
    -version | --version | --versio | --versi | --vers | --ver | --ver | --v)
        echo "$MAJ.$MIN.$PAT" ;;
    -branch | --branch | --branc | --bran | --bra | --br | --b)
        echo "$BRA" ;;
    -svnversion | --svnversion | --svnversio | --svnversi | --svnversi \
    | --svnvers | --svnver | --svnve | --svnv | --svn | --sv | --s)
        echo "$SVN" ;;
esac
