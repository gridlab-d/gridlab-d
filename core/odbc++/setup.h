/*
   This file is part of libodbc++.

   Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ODBCXX_SETUP_H
#define __ODBCXX_SETUP_H

#if defined(__WIN32__) && !defined(WIN32)
# define WIN32 1
#endif

#if !defined(WIN32)
# include <odbc++/config.h>
#else
# include <odbc++/config-win32.h>
#endif


#if defined(IN_ODBCXX) && defined(ODBCXX_ENABLE_THREADS)
# if !defined(_REENTRANT)
#  define _REENTRANT 1
# endif
# if !defined(_THREAD_SAFE)
#  define _THREAD_SAFE 1
# endif
#endif

// set the UNICODE define activate wide versions of ODBC functions
#if defined(ODBCXX_UNICODE)
# if !defined(UNICODE)
#  define UNICODE
# endif
#else
# if defined(UNICODE)
#  undef UNICODE
# endif
#endif

// check whether we use strstream or stringstream
#if defined(IN_ODBCXX)
# if defined(ODBCXX_UNICODE)
#   define ODBCXX_SSTREAM std::wstringstream
# else
# if defined(ODBCXX_HAVE_SSTREAM)
#  define ODBCXX_SSTREAM std::stringstream
# else
#  define ODBCXX_SSTREAM std::strstream
# endif
# endif
#endif

// check if ODBCVER is forced to something
#if defined(ODBCXX_ODBCVER)
# define ODBCVER ODBCXX_ODBCVER
#endif

// this can confuse our Types::CHAR
#ifdef CHAR
#undef CHAR
#endif

// NDEBUG and cassert
#if defined(IN_ODBCXX)
# if !defined(ODBCXX_DEBUG)
#  define NDEBUG
# endif
# include <cassert>
#endif

// this should do the trick
#if defined(__GNUC__) && __GNUC__>=3
# define ODBCXX_HAVE_ISO_CXXLIB
#endif


#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__)
# if defined(ODBCXX_DLL)
#  if defined(IN_ODBCXX)
#   define ODBCXX_EXPORT __declspec(dllexport)
#  else
#   define ODBCXX_EXPORT __declspec(dllimport)
#  endif
# endif
#endif

#if !defined(ODBCXX_EXPORT)
# define ODBCXX_EXPORT
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
# define ODBCXX_DUMMY_RETURN(x) return (x)
#else
# define ODBCXX_DUMMY_RETURN(x) ((void)0)
#endif


// environment abstractions

#if defined(ODBCXX_QT)

# define ODBCXX_STRING QString
# define ODBCXX_STRING_C(s) QString::fromLocal8Bit(s)
# define ODBCXX_STRING_CL(s,l) QString::fromLocal8Bit(s,l)
# define ODBCXX_STRING_LEN(s) s.length()
# define ODBCXX_STRING_DATA(s) s.local8Bit().data()
# define ODBCXX_STRING_CSTR(s) s.local8Bit().data()

# define ODBCXX_STREAM QIODevice

# define ODBCXX_BYTES QByteArray
# define ODBCXX_BYTES_SIZE(b) b.size()
# define ODBCXX_BYTES_DATA(b) b.data()
# define ODBCXX_BYTES_C(buf,len) QByteArray().duplicate(buf,len)

#else

# if defined(ODBCXX_UNICODE)

#  define ODBCXX_STRING std::wstring
#  define ODBCXX_STRING_C(s) std::wstring(s)
#  define ODBCXX_STRING_CL(s,l) std::wstring(s,l)
#  define ODBCXX_STRING_LEN(s) s.length()
#  define ODBCXX_STRING_DATA(s) s.data()
#  define ODBCXX_STRING_CSTR(s) s.c_str()

#  define ODBCXX_STREAM std::wistream
#  define ODBCXX_STREAMBUF std::wstreambuf

#  define ODBCXX_BYTES odbc::Bytes
#  define ODBCXX_BYTES_SIZE(b) b.getSize()
#  define ODBCXX_BYTES_DATA(b) b.getData()
#  define ODBCXX_BYTES_C(buf,len) odbc::Bytes((wchar_t*)buf,(size_t)len)

# else

#  define ODBCXX_STRING std::string
#  define ODBCXX_STRING_C(s) std::string(s)
#  define ODBCXX_STRING_CL(s,l) std::string(s,l)
#  define ODBCXX_STRING_LEN(s) s.length()
#  define ODBCXX_STRING_DATA(s) s.data()
#  define ODBCXX_STRING_CSTR(s) s.c_str()

#  define ODBCXX_STREAM std::istream
#  define ODBCXX_STREAMBUF std::streambuf

#  define ODBCXX_BYTES odbc::Bytes
#  define ODBCXX_BYTES_SIZE(b) b.getSize()
#  define ODBCXX_BYTES_DATA(b) b.getData()
#  define ODBCXX_BYTES_C(buf,len) odbc::Bytes((signed char*)buf,(size_t)len)

# endif // ODBCXX_UNICODE

#endif // ODBCXX_QT

#if defined(ODBCXX_UNICODE)
# define ODBCXX_CHAR_TYPE wchar_t
# define ODBCXX_SIGNED_CHAR_TYPE wchar_t
# define ODBCXX_SQLCHAR SQLWCHAR
# define ODBCXX_STRING_CONST(s) L ## s
# define ODBCXX_COUT std::wcout
# define ODBCXX_CERR std::wcerr
# define ODBCXX_STRTOL wcstol
#else
# define ODBCXX_CHAR_TYPE char
# define ODBCXX_SIGNED_CHAR_TYPE signed char
# define ODBCXX_SQLCHAR SQLCHAR
# define ODBCXX_STRING_CONST(s) s
# define ODBCXX_COUT std::cout
# define ODBCXX_CERR std::cerr
# define ODBCXX_STRTOL strtol
#endif

#endif // __ODBCXX_SETUP_H
