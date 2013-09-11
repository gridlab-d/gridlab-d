#
# SYNOPSIS
#
#   GLD_CHECK_BUILTIN(FUNCTION, PROLOGUE, BODY,
#                     [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   This macro provides a test for builtin compiler functions.  The FUNCTION
#   parameter is used for output to logs and for the preprocessor variable
#   name used in config.h (converted to uppercase).  PROLOGUE and BODY are
#   passed on to AC_LANG_PROGRAM where PROLOGUE would include globals that
#   appear above main() and BODY is the body of main() (see AC_LANG_PROGRAM
#   for mor information).
#
#   The macro attempts to compile and link a program using the given
#   prologue and body and, on success, defines the HAVE_$FUNCTION macro
#   in config.h.
#
#   This macro calls:
#
#     AC_DEFINE(HAVE_$FUNCTION)
#
#   where FUNCTION is the first argument with all letters converted to
#   uppercase.
#
# LICENSE
#
#   Copyright (c) 2009 Battelle Memorial Institute
#
#   This file is distributed under the same terms as GridLAB-D.
# 

AC_DEFUN([GLD_CHECK_BUILTIN], [
	AC_MSG_CHECKING([if compiler has $1 builtin])
	AC_LINK_IFELSE(
		[AC_LANG_PROGRAM([$2], [$3])], [have_builtin=yes], [have_builtin=no])
	AC_MSG_RESULT([$have_builtin])
	if test x"$have_builtin" = xyes; then
		AC_DEFINE(m4_toupper([HAVE_$1]), [1],
			[Define to 1 if you have the `$1' builtin.])
		$4
	else
		:
		$5
	fi
])
