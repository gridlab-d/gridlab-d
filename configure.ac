# $Id: configure.ac 4738 2014-07-03 00:55:39Z dchassin $                                            -*- Autoconf -*-
# Copyright (C) 2008 Battelle Memorial Institute
# Process this file with autoconf to produce a configure script.
# This file is distributed under the same terms as GridLAB-D.

###############################################################################
# Init autoconf
###############################################################################

dnl m4 magic from Eric Blake <e...@byu.net>
dnl autoconf 2.63 does not have m4_esyscmd_s
m4_define([sbuild_m4_chomp],
[m4_format([[%.*s]], m4_bregexp(m4_translit([$1], [
]]m4_dquote(_m4_defn([m4_cr_all]))[, [/]]m4_format([%255s], [])[),
  [/*$]), [$1])])
m4_define([sbuild_m4_esyscmd_s],
[sbuild_m4_chomp(m4_esyscmd([$1]))])

dnl GridLAB-D version information
m4_define([pkgversion], sbuild_m4_esyscmd_s([build-aux/version.sh --version]))
m4_define([pkgbranch],  sbuild_m4_esyscmd_s([build-aux/version.sh --branch]))
m4_define([svnversion], sbuild_m4_esyscmd_s([build-aux/version.sh --svn]))

AC_PREREQ([2.63])
AC_INIT([GridLAB-D], pkgversion, [gridlabd@pnnl.gov], [gridlabd])
AC_CONFIG_SRCDIR([core/aggregate.c])
AC_CONFIG_HEADER([core/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

###############################################################################
# Init automake
###############################################################################
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

###############################################################################
# Misc. information and package setup.
###############################################################################
GLD_TOP_BUILDDIR="`pwd`"
cd "$srcdir"
GLD_TOP_SRCDIR="`pwd`"
cd "$GLD_TOP_BUILDDIR"
AS_IF([test "$GLD_TOP_BUILDDIR" != "$GLD_TOP_SRCDIR"],
      [AC_MSG_NOTICE([Detected VPATH build])])

AC_CANONICAL_HOST

# Export version info as autoconf variables
AC_SUBST(PACKAGE_BASEVERSION, [pkgversion])
AC_SUBST(PACKAGE_BRANCH, [pkgbranch])
AC_SUBST(PACKAGE_REVISION, [svnversion])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [turn on debugging @<:@default=yes@:>@])],
    [AS_CASE([$enableval],
        [yes], [debug=true],
        [no],  [debug=false],
        [AC_MSG_ERROR([bad value $enableval for --enable-debug])])],
    [debug=true])

AC_ARG_ENABLE([optimization],
    [AS_HELP_STRING([--enable-optimization@<:@=LEVEL@:>@],
                    [turn on optimization @<:@default=0@:>@])],
    [AS_CASE([${enableval}],
        [0|1|2|3|s], [optimization=${enableval}],
        [yes],       [optimization=2],
        [no],        [optimization=0],
        [AC_MSG_ERROR([bad value ${enableval} for --enable-optimization])])],
    [optimization=0])

###############################################################################
# C compiler
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([C compiler])
AC_MSG_NOTICE

AC_PROG_CC
AC_GNU_SOURCE   # defines _GNU_SOURCE
AM_PROG_CC_C_O  # required for automake to support subdir-objects

AS_IF([test "x$debug" = "true"], [GLD_CFLAGS="$GLD_CFLAGS -g"])
GLD_CFLAGS="$GLD_CFLAGS -O$optimization"

#--------------------------------------
# Checks for C header files.
#--------------------------------------

#--------------------------------------
# Checks for C typedefs, structures, and compiler characteristics.
#--------------------------------------
AX_PTHREAD
# check whether -lpthread is ok when linking and add it if so
AC_MSG_CHECKING([whether to add -lpthread to PTHREAD_LIBS])
AS_CASE([$PTHREAD_LIBS],
[*-lpthread*], [result=no],
[SAVE_LIBS="$LIBS"
LIBS="$LIBS -lpthread"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
[[
#include <pthread.h>
static void routine(void *a) { a = 0; }
static void *start_routine(void *a) { return a; }
void function() {
    pthread_t th;
    pthread_attr_t attr;
    pthread_create(&th, 0, start_routine, 0);
    pthread_join(th, 0);
    pthread_attr_init(&attr);
    pthread_cleanup_push(routine, 0);
    pthread_cleanup_pop(0);
}
]],
[[function();]])],
    [PTHREAD_LIBS="$PTHREAD_LIBS -lpthread"
     result=yes],
    [result=no])
LIBS="$SAVE_LIBS"
])
AC_MSG_RESULT([$result])


#--------------------------------------
# Checks for C type sizes.
#--------------------------------------

#--------------------------------------
# Checks for C library functions.
#--------------------------------------

#--------------------------------------
# Checks for C libraries.
#--------------------------------------

# Check for curses
AX_WITH_CURSES
AS_IF([test "x$ax_cv_curses" = xyes],
      [HAVE_CURSES=yes
       GLD_CPPFLAGS="$GLD_CPPFLAGS -DHAVE_CURSES"],
      [HAVE_CURSES=no])

AC_SUBST([GLD_CFLAGS])

###############################################################################
# C++ compiler
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([C++ compiler])
AC_MSG_NOTICE

AC_LANG_PUSH([C++])

AC_PROG_CXX

AS_IF([test "x$debug" = "true"], [GLD_CXXFLAGS="$GLD_CXXFLAGS -g"])
GLD_CXXFLAGS="$GLD_CXXFLAGS -O$optimization"

#--------------------------------------
# Checks for C++ header files.
#--------------------------------------
AC_PATH_XTRA
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([float.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([math.h])
AC_CHECK_HEADERS([memory.h])
AC_CHECK_HEADERS([memory.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([sched.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/timeb.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([wchar.h])

#--------------------------------------
# Checks for C++ typedefs, structures, and compiler characteristics.
#--------------------------------------
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_STRUCT_TM
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

GLD_CHECK_BUILTIN(
    [__sync_bool_compare_and_swap],
    [],
    [volatile unsigned int value = 0;
     __sync_bool_compare_and_swap(&value, value, 1);],
    [HAVE_SYNC_BOOL_COMPARE_AND_SWAP=yes],
    [HAVE_SYNC_BOOL_COMPARE_AND_SWAP=no])

GLD_CHECK_BUILTIN(
    [__sync_add_and_fetch],
    [],
    [volatile unsigned int value = 0;
     __sync_add_and_fetch(&value, 1);],
    [HAVE_SYNC_ADD_AND_FETCH=yes],
    [HAVE_SYNC_ADD_AND_FETCH=no])

# AX_PTHREAD is a C-only test. We assume its flags work for CXX, as well. But
# we need to introduce a similar PTHREAD_CXX variable.
# More AIX lossage: compile with *_r variant
if test "x$GCC" != xyes; then
    case $host_os in
        aix*)
        AS_CASE(["x/$CXX"],
          [x*/c89|x*/c89_128|x*/c99|x*/c99_128|x*/cc|x*/cc128|x*/xlc|x*/xlc_v6|x*/xlc128|x*/xlc128_v6],
          [#handle absolute path differently from PATH based program lookup
           AS_CASE(["x$CXX"],
             [x/*],
             [AS_IF([AS_EXECUTABLE_P([${CXX}_r])],[PTHREAD_CC="${CXX}_r"])],
             [AC_CHECK_PROGS([PTHREAD_CC],[${CXX}_r],[$CXX])])])
        ;;
    esac
fi
test -n "$PTHREAD_CXX" || PTHREAD_CXX="$CXX"
AC_SUBST([PTHREAD_CXX])

#--------------------------------------
# Checks for C++ type sizes.
#--------------------------------------

#--------------------------------------
# Checks for C++ library functions.
#--------------------------------------
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm])
AC_CHECK_FUNCS([atexit])
AC_CHECK_FUNCS([floor])
AC_CHECK_FUNCS([ftime])
AC_CHECK_FUNCS([getcwd])
AC_CHECK_FUNCS([gethostbyname])
AC_CHECK_FUNCS([gethrtime])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([inet_ntoa])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([putenv])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([setenv])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strcspn])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strpbrk])
AC_CHECK_FUNCS([strrchr])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([tzset])

#--------------------------------------
# Checks for C++ function-like macros.
#--------------------------------------

# isfinite
AC_CACHE_CHECK([for isfinite], [gld_cv_func_isfinite],
    [AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <math.h>]],
            [[int result = isfinite(1.0);]])],
        [gld_cv_func_isfinite=yes],
        [gld_cv_func_isfinite=no])])
AS_IF([test "x$gld_cv_func_isfinite" = xyes],
    [AC_DEFINE([HAVE_ISFINITE], [1],
        [Define to 1 if you have the isfinite function.])])

# CPU_ALLOC
AC_CACHE_CHECK([for CPU_ALLOC], [gld_cv_func_CPU_ALLOC],
    [AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <sched.h>]],
            [[cpu_set_t *cpu = CPU_ALLOC(2);]])],
        [gld_cv_func_CPU_ALLOC=yes],
        [gld_cv_func_CPU_ALLOC=no])])
AS_IF([test "x$gld_cv_func_CPU_ALLOC" = xyes],
    [AC_DEFINE([DYN_PROC_AFFINITY],[1],
        ["Check for existence of dynamic proc functions"])])
GLD_GET_NPROCS

# Check for the Linux functions for controlling processor affinity.
# LINUX: sched_setaffinity
# AIX:   bindprocessor
# OSX (Leopard): thread_policy_set
AC_CHECK_FUNCS([sched_setaffinity])
AC_CHECK_FUNCS([sched_getaffinity])
AC_CHECK_FUNCS([bindprocessor])
AC_CHECK_FUNCS([thread_policy_set])
AC_CACHE_CHECK([whether cpu_set_t available],
    [gld_cv_have_cpu_set_t],
    [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <sched.h>]],
            [[cpu_set_t t;]])],
        [gld_cv_have_cpu_set_t=yes],
        [gld_cv_have_cpu_set_t=no])])
AS_IF([test "$gld_cv_have_cpu_set_t" = "yes"],
    [AC_DEFINE([HAVE_CPU_SET_T], [1],
        [Define if cpu_set_t is defined in sched.h])])
AC_CACHE_CHECK([whether the CPU_SET and CPU_ZERO macros are defined],
    [gld_cv_cpu_set_defined],
    [AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <sched.h>]],
            [[cpu_set_t t; CPU_ZERO(&t); CPU_SET(1,&t);]])],
        [gld_cv_cpu_set_defined=yes],
        [gld_cv_cpu_set_defined=no])])
AS_IF([test "$gld_cv_cpu_set_defined" = "yes"],
    [AC_DEFINE([HAVE_CPU_SET_MACROS], [1],
        [Define if CPU_SET and CPU_ZERO defined])])
AC_CACHE_CHECK([whether thread affinity macros defined],
    [gld_cv_have_thread_affinity_policy],
    [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <mach/thread_policy.h>]],
[[
#if !defined(THREAD_AFFINITY_POLICY) || !defined(THREAD_AFFINITY_TAG_NULL)
    :'thread macros not defined
#endif
]],
        [gld_cv_have_thread_affinity_policy=yes],
        [gld_cv_have_thread_affinity_policy=no])])])
AS_IF([test "$gld_cv_have_thread_affinity_policy" = "yes"],
    [AC_DEFINE([HAVE_OSX_THREAD_AFFINITY], [1],
        [Define is the OSX thread affinity policy macros defined])])

#--------------------------------------
# Checks for C++ libraries.
#--------------------------------------

# Check for xerces
unset XERCES_VERSION
AX_LIB_XERCES([2.7])
AM_CONDITIONAL([MISSING_XERCES], [test x$HAVE_XERCES != xyes])
AC_SUBST([HAVE_XERCES])

# Check for CppUnit
AM_PATH_CPPUNIT([1.12], [HAVE_CPPUNIT=yes], [HAVE_CPPUNIT=no])
AS_IF([test x"$HAVE_CPPUNIT" = xyes],
    [AC_MSG_CHECKING([if compiler can link libcppunit])
     SAVED_CXXFLAGS="$CXXFLAGS"
     SAVED_LIBS="$LIBS"
     CXXFLAGS="$CXXFLAGS $CPPUNIT_CFLAGS"
     LIBS="$LIBS $CPPUNIT_LIBS"
     AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <cppunit/extensions/TestFactoryRegistry.h>]],
            [[CppUnit::TestFactoryRegistry::getRegistry();]])],
        [],
        [HAVE_CPPUNIT=no])
     CXXFLAGS="$SAVED_CXXFLAGS"
     LIBS="$SAVED_LIBS"
     AC_MSG_RESULT([$HAVE_CPPUNIT])])
AS_IF([test x"$HAVE_CPPUNIT" = xyes],
    [# CppUnit doesn't separate its flags appropriately
     COMBINED_CPPUNIT_FLAGS="$CPPUNIT_CFLAGS $CPPUNIT_LIBS"
     CPPUNIT_CFLAGS=
     CPPUNIT_LIBS=
     GLD_FLAG_PARSE([COMBINED_CPPUNIT_FLAGS],
        [CPPUNIT_LIBS], [CPPUNIT_LDFLAGS], [CPPUNIT_CPPFLAGS])],
    [GLD_CPPFLAGS="$GLD_CPPFLAGS -D_NO_CPPUNIT"])
AC_SUBST([CPPUNIT_CPPFLAGS])
AC_SUBST([CPPUNIT_LDFLAGS])
AC_SUBST([CPPUNIT_LIBS])

AC_SUBST([GLD_CPPFLAGS])

AC_LANG_POP([C++])

###############################################################################
# Libtool
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Libtool])
AC_MSG_NOTICE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_PREREQ([2.2])
LT_INIT([dlopen win32-dll shared disable-static])
AC_SUBST([LIBTOOL_DEPS])

AS_CASE([$host_os],
[darwin*],
    [LIBEXT=.so],
[cygwin*],
    [LIBEXT=.a],
[mingw*],
    [LIBEXT=.dll],
[LIBEXT=.so
 GLD_LDFLAGS="$GLD_LDFLAGS -Wl,--no-undefined"])

AC_DEFINE_UNQUOTED([DLEXT], ["$LIBEXT"], [Dynamic library extension])

AC_SUBST([LIBEXT])
AC_SUBST([GLD_LDFLAGS])
AC_SUBST([GLD_LIBS])

###############################################################################
# Checks for additional programs.
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Checks for additional programs])
AC_MSG_NOTICE

AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

DX_INIT_DOXYGEN([gridlabd], [['$(top_srcdir)/doxygen/gridlabd.conf']])
DX_HTML_FEATURE([ON])
DX_CHM_FEATURE([OFF])
DX_CHI_FEATURE([OFF])
DX_MAN_FEATURE([OFF])
DX_RTF_FEATURE([OFF])
DX_XML_FEATURE([OFF])
DX_PDF_FEATURE([OFF])
DX_PS_FEATURE([OFF])
AS_IF([test -n "$DX_DOXYGEN"],
      [HAVE_DOXYGEN="$DX_DOXYGEN"],
      [HAVE_DOXYGEN=no])

# Check for Matlab
AC_CHECK_PROGS([MATLAB], [matlab])
AC_CACHE_CHECK([for matlab environment (matlab -e)], [gld_cv_matlab_e],
    [AS_IF([test "x$MATLAB" != x],
        [AS_IF([$MATLAB -e > /dev/null 2>&1],
            [gld_cv_matlab_e=yes; have_matlab_val=1],
            [gld_cv_matlab_e=no; have_matlab_val=0])])])
AC_CACHE_CHECK([for matlab path], [gld_cv_matlab_path],
    [AS_IF([test "x$gld_cv_matlab_e" = xyes],
        [gld_cv_matlab_path=`$MATLAB -e | grep ^MATLAB= | cut -f2 -d=`])])
AC_CACHE_CHECK([for matlab LD_LIBRARY_PATH], [gld_cv_matlab_ldpath],
    [AS_IF([test "x$gld_cv_matlab_e" = xyes],
        [gld_cv_matlab_ldpath=`$MATLAB -e | grep ^LD_LIBRARY_PATH= | cut -f2 -d=`
         AS_IF([test "x$gld_cv_matlab_ldpath" = x],
            [gld_cv_matlab_ldpath=`$MATLAB -e | grep ^DYLD_LIBRARY_PATH= | cut -f2 -d=`
])])])
AS_IF([test "x$MATLAB" != x],
    [MATLAB_CPPFLAGS="-I$MATLAB/extern/include"
     MATLAB_LDFLAGS="-L${MATLAB_LDPATH//:/ -L}"
     MATLAB_LIBS="-lmx -leng"
     GLD_CPPFLAGS="$GLD_CPPFLAGS -DHAVE_MATLAB"
     HAVE_MATLAB=yes],
    [HAVE_MATLAB=no])
AM_CONDITIONAL([HAVE_MATLAB], [test "x$MATLAB" != x])
AC_SUBST([MATLAB_CPPFLAGS])
AC_SUBST([MATLAB_LDFLAGS])
AC_SUBST([MATLAB_LDPATH])
AC_SUBST([MATLAB_LIBS])

# Check for mysql
m4_define([mysql_include], [/usr/local/mysql-connector-c/include])
m4_define([mysql_lib],     [/usr/local/mysql-connector-c/lib])
AC_CACHE_CHECK([for mysql_include/mysql.h], [gld_cv_mysql],
    [AS_IF([test -f mysql_include/mysql.h],
           [gld_cv_mysql=yes],
           [gld_cv_mysql=no])])
AS_IF([test "x$gld_cv_mysql" = xyes],
    [MYSQL_CPPFLAGS=[-I]mysql_include
     MYSQL_LIBS="-lmysqlclient"
     GLD_CPPFLAGS="$GLD_CPPFLAGS -DHAVE_MYSQL"
     HAVE_MYSQL=yes],
    [HAVE_MYSQL=no])
AC_CACHE_CHECK([for mysql_lib], [gld_cv_mysql_lib],
    [AS_IF([test -d mysql_lib],
           [gld_cv_mysql_lib=yes],
           [gld_cv_mysql_lib=no])])
AS_IF([test "x$gld_cv_mysql_lib" = xyes],
    [MYSQL_LDFLAGS=[-L]mysql_lib])
AM_CONDITIONAL([HAVE_MYSQL], [test "x$HAVE_MYSQL" = xyes])
AC_SUBST([MYSQL_CPPFLAGS])
AC_SUBST([MYSQL_LDFLAGS])
AC_SUBST([MYSQL_LIBS])

###############################################################################
# The End
###############################################################################

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([gridlabd.spec])
AC_CONFIG_FILES([core/gridlabd], [chmod +x core/gridlabd])
AC_OUTPUT

BINDIR=`eval echo $(eval echo $bindir)`
LIBDIR=`eval echo $(eval echo $libdir)`
DOCDIR=`eval echo $(eval echo $docdir)`
AS_IF([test "x$HAVE_XERCES" = xyes],
    [CONFIG_RESULT=OK
     AS_IF([test -z "$XERCES_VERSION" || expr "$XERCES_VERSION" \< 3 >/dev/null],
        [AC_MSG_WARN([libxerces-c version $XERCES_VERSION])
         AC_MSG_WARN([may not be suitable for building $PACKAGE])])],
    [CONFIG_RESULT=failed
     HAVE_XERCES="$HAVE_XERCES (required)"
     AC_MSG_WARN([missing libxerces-c; build will fail until dependency is met])])

AC_MSG_RESULT([
-----------------------------------------------------------------------

  $PACKAGE $VERSION: Automatic configuration $CONFIG_RESULT.

  Dependencies:

    Doxygen: .................... $HAVE_DOXYGEN
    libcppunit: ................. $HAVE_CPPUNIT
    libxerces-c: ................ $HAVE_XERCES
    matlab: ..................... $HAVE_MATLAB
    mysql-connector-c: .......... $HAVE_MYSQL
    curses: ..................... $HAVE_CURSES

  Atomic Builtins:

    __sync_bool_compare_and_swap: $HAVE_SYNC_BOOL_COMPARE_AND_SWAP
    __sync_add_and_fetch: ....... $HAVE_SYNC_ADD_AND_FETCH

  Installation paths:

    Scripts: .................... $BINDIR
    Libraries: .................. $LIBDIR/$PACKAGE
    Documentation: .............. $DOCDIR

  Building:

    type 'make' to compile $PACKAGE.
    type 'make install' to install $PACKAGE in $prefix.

-----------------------------------------------------------------------
])
