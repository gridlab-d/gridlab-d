# $Id: configure.ac,v 1.31 2008/01/25 18:05:07 d3p181 Exp $                                            -*- Autoconf -*-
# Copyright (C) 2008 Battelle Memorial Institute
# Process this file with autoconf to produce a configure script.
# This file is distributed under the same terms as GridLAB-D.

AC_PREREQ(2.59)
AC_INIT([Gridlab-D], 1.1, [David Chassin <david.chassin@pnl.gov>], gridlabd)
AC_CONFIG_SRCDIR([core/aggregate.c])
# Older versions of automake may require the that following macro be replaced
# with AM_CONFIG_HEADER([core/config.h]).
AC_CONFIG_HEADER([core/config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])

AC_PREFIX_DEFAULT([/usr])
AC_CANONICAL_HOST

AC_ARG_ENABLE([debug],
				  AS_HELP_STRING([--enable-debug],
				  					  [turn on debugging @<:@default=yes@:>@]),
				  [case "${enableval}" in
				  	 yes) debug=true ;;
				  	 no) debug=false ;;
				  	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
				  	esac], [debug=true])

AC_ARG_ENABLE([optimization],
				  AS_HELP_STRING([--enable-optimization@<:@=LEVEL@:>@],
				  					  [turn on optimization @<:@default=0@:>@]),
				  [case "${enableval}" in
				  	 0|1|2|3|s) optimization=${enableval} ;;
				  	 yes) optimization=2 ;;
				  	 no) optimization=0 ;;
				  	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-optimization]) ;;
				  	esac], [optimization=0])

#AC_ARG_ENABLE([cppunit],
#				  AS_HELP_STRING([--enable-cppunit],
#				  					  [turn on unit testing @<:@default=yes@:>@]),
#				  [case "${enableval}" in
#				  	 yes) WANT_CPPUNIT=true ;;
#				  	 no) WANT_CPPUNIT=false ;;
#				  	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-cppunit]) ;;
#				  	esac], [WANT_CPPUNIT=true])
#AM_CONDITIONAL(WANT_CPPUNIT, test x$WANT_CPPUNIT = xtrue)

GLFLAGS="$GLFLAGS -I`pwd`/core"
if [[ "$debug" = "true" ]]; then
	GLFLAGS="$GLFLAGS -g"
fi
if [[ "$optimization" ]]; then
	GLFLAGS="$GLFLAGS -O$optimization"
fi

# Putting -ansi in CPPFLAGS causes errors that prevent building.
#CPPFLAGS="$CPPFLAGS $GLFLAGS -ansi"
CPPFLAGS="$CPPFLAGS $GLFLAGS"
CXXFLAGS="$CXXFLAGS $GLFLAGS"
CFLAGS="$CFLAGS $GLFLAGS"

case "$host_os" in
darwin*)
     LIBEXT=.dylib
     ;;
cygwin*)
     AC_LIBTOOL_WIN32_DLL
     LIBEXT=.a
     ;;
mingw*)
     AC_LIBTOOL_WIN32_DLL
     LIBEXT=.dll
     ;;
*)
     LIBEXT=.so
     ;;
esac
AC_SUBST(LIBEXT)
AC_DEFINE_UNQUOTED([DLEXT], ["$LIBEXT"], "Dynamic library extension")

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LN_S
#AC_PROG_MKDIR_P
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AX_LIB_XERCES
test "$HAVE_XERCES" = "yes" || \
		AC_MSG_FAILURE([Xerces library not found. If Xerces is not properly \
installed, you probably need to use one or more of the --with-xerces options. \
Try `./configure --help[`] for more information.])
AX_LIB_SUPERLU
#if test "$HAVE_SUPERLU" != "yes"; then
#	if test -f third_party/SuperLU_3.1/lib/libsuperlu_3.1.a; then
#		SUPERLU_LIBS=$PWD/third_party/SuperLU_3.1/lib/libsuperlu_3.1.a
#	elif test -f third_party/SuperLU_3.1/lib/libsuperlu.a; then
#		SUPERLU_LIBS=$PWD/third_party/SuperLU_3.1/lib/libsuperlu.a
#	else
#		AC_MSG_FAILURE([SuperLU library not found. If SuperLU is not properly \
#installed, you probably need to use one or more of the --with-superlu options. \
#Try `./configure --help[`] for more information.])
#	fi
	SUPERLU_CPPFLAGS=-I$PWD/third_party/superLU_MT/SRC
	SUPERLU_LDFLAGS=""
	AC_SUBST(SUPERLU_CPPFLAGS)
	AC_SUBST(SUPERLU_LDFLAGS)
	AC_SUBST(SUPERLU_LIBS)
	AC_DEFINE_UNQUOTED(HAVE_SUPERLU)
	HAVE_SUPERLU=yes
#fi

DX_INIT_DOXYGEN(gridlabd, [doxygen/gridlabd.conf])
DX_HTML_FEATURE (ON)
DX_CHM_FEATURE (OFF)
DX_CHI_FEATURE (OFF)
DX_MAN_FEATURE (OFF)
DX_RTF_FEATURE (OFF)
DX_XML_FEATURE (OFF)
DX_PDF_FEATURE (OFF)
DX_PS_FEATURE (OFF)

# Checks for libraries.
AC_LANG_PUSH([C++])
AC_CHECK_HEADER([cppunit/config-auto.h], [],
   [CPPFLAGS="$CPPFLAGS -D_NO_CPPUNIT"])
#   [AC_MSG_FAILURE([Missing cppunit headers.])])
#AC_CHECK_LIB([cppunit], [CppUnit::TestFactoryRegistry::getRegistry])
LIBS_ORIG=$LIBS
echo "LIBS: " $LIBS
LIBS="-ldl -lcppunit $LIBS"
echo "LIBS: " $LIBS
AC_LINK_IFELSE(AC_LANG_PROGRAM([[#include <cppunit/extensions/TestFactoryRegistry.h>]],[[CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();]]),[],[LIBS=$LIBS_ORIG])
#LIBS=$LIBS_ORIG
AC_LANG_POP([C++])

#AM_PATH_CPPUNIT(1.12.0)
#AC_CHECK_LIB([cppunit], [main], [],
#   [AC_MSG_FAILURE([Missing cppunit shared library.])])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([malloc.h memory.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([isfinite atexit getcwd memset pow putenv sqrt strchr strerror strrchr tzset])
ACX_PTHREAD
GLD_GET_NPROCS
#GCC_CT_FEATURES

# DO NOT EDIT THE FOLLOWING LINE
AC_CONFIG_FILES([./Makefile \
	core/Makefile \
	core/rt/Makefile \
	assert/Makefile \
	climate/Makefile \
	climate/tmy/Makefile \
	commercial/Makefile \
	network/Makefile \
	powerflow/Makefile \
	residential/Makefile \
	reliability/Makefile \
	generators/Makefile\
	plc/Makefile \
	market/Makefile \
	tape_file/Makefile \
	tape_plot/Makefile \
	tape/Makefile \
	])

AC_SUBST(CPPFLAGS)
AC_OUTPUT

