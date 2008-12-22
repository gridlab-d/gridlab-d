#if !defined(__ODBCXX_SETUP_H)
# error "Do not include this file directly. Use <odbc++/setup.h> instead"
#endif

#define ODBCXX_HAVE_SQL_H
#define ODBCXX_HAVE_SQLEXT_H
#define ODBCXX_HAVE_SQLUCODE_H

#define ODBCXX_HAVE_CSTDIO
#define ODBCXX_HAVE_CSTDLIB
#define ODBCXX_HAVE_CSTRING
#define ODBCXX_HAVE_CTIME
#define ODBCXX_HAVE_IOSTREAM
#define ODBCXX_HAVE_SSTREAM
#define ODBCXX_HAVE_SET
#define ODBCXX_HAVE_VECTOR


#define ODBCXX_ENABLE_THREADS

#if defined(_MSC_VER)
// MSVC has a rather compliant CXX lib
# define ODBCXX_HAVE_ISO_CXXLIB

# define ODBCXX_HAVE__ITOA
# define ODBCXX_HAVE__STRICMP
# define ODBCXX_HAVE__SNPRINTF

# define ODBCXX_HAVE__I64TOA
# define ODBCXX_HAVE__ATOI64
# define ODBCXX_HAVE__ATOI

// disable the 'identifier name truncated in debug info' warning
# pragma warning(disable:4786)

// disable the 'class blah blah should be exported' warning
// don't know if this is dangerous, but it only whines about templated
// and/or inlined classes and it really bothers me =)
# if defined(ODBCXX_DLL)
#  pragma warning(disable:4251)
# endif

# if _MSC_VER <= 1200
#  define ODBCXX_NO_STD_TIME_T
# endif

#endif // _MSC_VER



#if defined(__BORLANDC__)

// FIXME: this should check for older versions
# define ODBCXX_HAVE_ISO_CXXLIB

# if !defined(_RWSTD_NO_EX_SPEC)
#  define _RWSTD_NO_EX_SPEC 1
# endif
// sql.h only defines this for msc, but borland has __int64 as well
# if !defined(ODBCINT64)
#  define ODBCINT64 __int64
# endif

# define ODBCXX_HAVE_ITOA 1
# define ODBCXX_HAVE_STRICMP 1

#endif // __BORLANDC__


#if defined(__MINGW32__)

// the MS runtime has those
# if defined(__MSVCRT__)
#  define ODBCXX_HAVE__I64TOA
#  define ODBCXX_HAVE__ATOI64
# endif

#define ODBCXX_HAVE__ITOA
#define ODBCXX_HAVE__STRICMP

#define ODBCXX_HAVE__SNPRINTF

// same as with borland
# if !defined(ODBCINT64)
#  define ODBCINT64 __int64
# endif

#endif // __MINGW32__

#define WIN32_LEAN_AND_MEAN

#if defined(ODBCXX_UNICODE)
# if !defined(_UNICODE)
#  define _UNICODE
# endif
# if defined(_MBCS)
#  undef _MBCS
# endif
#endif

#include <windows.h>
