AC_DEFUN([GLD_GET_NPROCS],
    [AC_MSG_CHECKING([for get_nprocs])
     AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <sys/sysinfo.h>]],
                         [[get_nprocs();]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([HAVE_GET_NPROCS], [1],
            [Define this if get_nprocs is available])],
        [AC_MSG_RESULT([no])])
]) dnl
