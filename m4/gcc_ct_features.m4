
AC_DEFUN([GCC_PARSE_CT_OUTPUT],[
	[GCC_OS=`grep ' OS = ' conftest.err | sed 's/^.* OS = \(.*\)$/\1/m'`]
	[GCC_GNU=`grep ' GNU = ' conftest.err | sed 's/^.* GNU = \(.*\)$/\1/m'`]
	[GCC_ARCH=`grep ' ARCH = ' conftest.err | sed 's/^.* ARCH = \(.*\)$/\1/m'`]
	])

AC_DEFUN([GCC_CT_SET_UNKNOWN],[
	[GCC_OS=unknown]
	[GCC_ARCH=unknown]
	])

AC_DEFUN([GCC_CT_FEATURES],[
	AC_MSG_NOTICE([Checking gcc compile-time features...])
	AC_COMPILE_IFELSE([
#if defined __APPLE__
	#warning OS = macosx
#elif defined linux
	#warning OS = linux
#elif defined _WIN32
	#warning OS = windows
#else
	#warning OS = unknown
#endif

#ifdef __GNUC__
	#warning GNU = yes
#else
	#warning GNU = no
#endif

#if defined(__x86_64__)
	#warning ARCH = x86_64
#elif defined(__i386__)
	#warning ARCH = i386
#elif defined(__ppc64__)
	#warning ARCH = ppc64
#elif defined(__ppc__)
	#warning ARCH = ppc
#endif
							 ],
							[GCC_PARSE_CT_OUTPUT()],
							[GCC_CT_SET_UNKNOWN()]
	)
	AC_MSG_NOTICE([   OS = ${GCC_OS}])
	AC_MSG_NOTICE([   GNU = ${GCC_GNU}])
	AC_MSG_NOTICE([   ARCH = ${GCC_ARCH}])
	AC_SUBST(GCC_OS)
	AC_SUBST(GCC_GNU)
	AC_SUBST(GCC_ARCH)
])

