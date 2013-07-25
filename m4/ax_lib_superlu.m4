#
# SYNOPSIS
#
#   AX_LIB_SUPERLU
#
# DESCRIPTION
#
#   This macro provides tests of availability of SuperLU matrix libraries of
#   particular version or newer. This macros checks for SuperLU C headears
#   and libraries and defines compilation flags
#
#   Macro supports following options and their values:
#
#   1) Single-option usage:
#
#     --with-superlu - yes, no or path to SuperLU installation prefix
#
#   2) Three-options usage (all options are required):
#
#     --with-superlu=yes
#     --with-superlu-inc - path to base directory with SuperLU headers
#     --with-superlu-lib - linker flags for SuperLU
#
#   This macro calls:
#
#     AC_SUBST(SUPERLU_CPPFLAGS)
#     AC_SUBST(SUPERLU_LDFLAGS)
#     AC_SUBST(SUPERLU_LIBS)
#     AC_DEFINE_UNQUOTED(HAVE_SUPERLU)
#
#   And sets:
#
#     HAVE_SUPERLU
#
# LICENSE
#
#   Copyright (c) 2009 Battelle Memorial Institute
#
#   This file is distributed under the same terms as GridLAB-D.
# 
#   Inspired by ax_lib_xerces.m4 by Mateusz Loskot.
#

AC_DEFUN([AX_LIB_SUPERLU],
[
    AC_ARG_WITH([superlu],
        AS_HELP_STRING([--with-superlu=@<:@ARG@:>@],
            [use SuperLU from given prefix (ARG=path); check standard prefixes (ARG=yes); disable (ARG=no)]
        ),
        [want_superlu="$withval"],
        [want_superlu="yes"]
    )
    AC_ARG_WITH([superlu-inc],
        AS_HELP_STRING([--with-superlu-inc=@<:@DIR@:>@],
            [path to SuperLU headers]
        ),
        [superlu_include_dir="$withval"],
        [superlu_include_dir=""]
    )
    AC_ARG_WITH([superlu-lib],
        AS_HELP_STRING([--with-superlu-lib=@<:@ARG@:>@],
            [link options for SuperLU libraries]
        ),
        [superlu_ldflags="-L$withval"],
        [superlu_ldflags=""]
    )

    if test "$want_superlu" = "yes"; then
        if test -d /usr/local/include/superlu ; then
            superlu_prefix=/usr/local
        elif test -d /usr/include/superlu ; then
            superlu_prefix=/usr
        else
            superlu_prefix=""
        fi
        superlu_requested="yes"
    elif test -d "$want_superlu"; then
        superlu_prefix="$want_superlu"
        superlu_requested="yes"
    else
        superlu_prefix=""
        superlu_requested="no"
    fi

    SUPERLU_CPPFLAGS=""
    SUPERLU_LDFLAGS=""
    SUPERLU_LIBS=""

    dnl
    dnl Collect include/lib paths and flags
    dnl
    run_superlu_test="no"

    if test -n "$superlu_prefix"; then
        superlu_include_dir="$superlu_prefix/include"
        superlu_include_dir2="$superlu_prefix/include/superlu"
        superlu_ldflags="-L$superlu_prefix/lib"
        run_superlu_test="yes"
    elif test "$superlu_requested" = "yes"; then
        if test -n "$superlu_include_dir" -a -n "$superlu_ldflags"; then
            superlu_include_dir2="$superlu_include_dir/superlu"
            run_superlu_test="yes"
        fi
    else
        run_superlu_test="no"
    fi

    superlu_libs="-lsuperlu"

    dnl
    dnl Check SuperLU Files
    dnl
    if test "$run_superlu_test" = "yes"; then
        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS -I$superlu_include_dir -I$superlu_include_dir2"

        saved_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS $superlu_ldflags"

        saved_LIBS="$LIBS"
        LIBS="$superlu_libs $LIBS"

        AC_LANG_PUSH([C])
        AC_CHECK_HEADER([slu_cdefs.h],
            [superlu_header_found="yes"],
            [superlu_header_found="no"])
        AC_CHECK_LIB([superlu], [exit],
            [superlu_lib_found="yes"],
            [superlu_lib_found="no"])
        AC_LANG_POP([C])

        CPPFLAGS="$saved_CPPFLAGS"
        LDFLAGS="$saved_LDFLAGS"
        LIBS="$saved_LIBS"

        if test "$superlu_header_found" = "yes" -a "$superlu_lib_found" = "yes"; then
            SUPERLU_CPPFLAGS="-I$superlu_include_dir -I$superlu_include_dir2"
            SUPERLU_LDFLAGS="$superlu_ldflags"
            SUPERLU_LIBS="$superlu_libs"
            AC_SUBST([SUPERLU_CPPFLAGS])
            AC_SUBST([SUPERLU_LDFLAGS])
            AC_SUBST([SUPERLU_LIBS])

            AC_DEFINE_UNQUOTED([HAVE_SUPERLU], [1], "Define to 1 if you have the SuperLU libraries (-lsuperlu)")
            HAVE_SUPERLU="yes"
        else
            HAVE_SUPERLU="no"
        fi
    fi
])
