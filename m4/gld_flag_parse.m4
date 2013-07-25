# GLD_FLAG_PARSE(ARG, VAR_LIBS, VAR_LDFLAGS, VAR_CPPFLAGS)
# --------------------------------------------------------
# Parse whitespace-separated ARG into appropriate LIBS, LDFLAGS, and
# CPPFLAGS variables.
AC_DEFUN([GLD_FLAG_PARSE],
[AC_COMPUTE_INT([gld_flag_parse_sizeof_voidp], [(long int) (sizeof (void*))])
for arg in $$1 ; do
    gld_flag_parse_ok=yes
    AS_CASE([$arg],
        [-l*],          [$2="$$2 $arg"],
        [-L*],          [$3="$$3 $arg"],
        [-WL*],         [$3="$$3 $arg"],
        [-Wl*],         [$3="$$3 $arg"],
        [-I*],          [$4="$$4 $arg"],
        [*.a],          [$2="$$2 $arg"],
        [*.so],         [$2="$$2 $arg"],
        [gld_flag_parse_ok=no])
# warn user that $arg fell through
     AS_IF([test "x$gld_flag_parse_ok" = xno],
        [AC_MSG_WARN([$arg of $1 not parsed])])
done])dnl
