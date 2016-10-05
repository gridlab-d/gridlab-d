#!/bin/sh
# This file is used by autoconf to generate the version string.
# It is assumed that this script is run from the top-level srcdir.
FIL="gldcore/version.h"
MAJ=`cat $FIL | sed -rn 's/#define REV_MAJOR ([0-9]+).*/\1/p' | tr -d '\n'`
MIN=`cat $FIL | sed -rn 's/#define REV_MINOR ([0-9]+).*/\1/p' | tr -d '\n'`
PAT=`cat $FIL | sed -rn 's/#define REV_PATCH ([0-9]+).*/\1/p' | tr -d '\n'`
BRA=`cat $FIL | sed -rn 's/#define BRANCH "([A-Za-z0-9]+)".*/\1/p' | tr -d '\n'`
SVN=`git --version | cut -f3 -d ' '`
case $1 in
    -version | --version | --versio | --versi | --vers | --ver | --ver | --v)
        echo "$MAJ.$MIN.$PAT" ;;
    -branch | --branch | --branc | --bran | --bra | --br | --b)
        echo "$BRA" ;;
    -svnversion | --svnversion | --svnversio | --svnversi | --svnversi \
    | --svnvers | --svnver | --svnve | --svnv | --svn | --sv | --s)
        echo "$SVN" ;;
esac
