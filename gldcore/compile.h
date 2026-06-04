#ifndef _COMPILE_H
#define _COMPILED_H

#if defined(_WIN32) && !defined(__MINGW32__)
    #define _WIN32_WINNT 0x0400
    #undef int64                // wtypes.h also used int64
    #define WIN32_LEAN_AND_MEAN // Exclude rarely used Windows headers
    // Winsock2 must precede windows.h
    #include <winsock2.h>
    #include <windows.h>
    #define int64 _int64
    #define PREFIX ""
    #ifndef DLEXT
        #define DLEXT ".dll"
    #endif
    #define DLLOAD(P) LoadLibrary(P)
    #define DLSYM(H, S) GetProcAddress((HINSTANCE)H, S)
    #define DLERR "no diagnostics available"
    // #define snprintf _snprintf
#else // LINUX
    #ifndef __MINGW32__
        #include "dlfcn.h"
        #define DLEXT ".dll"
    #endif
    #define PREFIX ""
    #ifndef DLEXT
        #define DLEXT ".so"
    #endif
    #ifndef __MINGW32__
        #define DLLOAD(P) dlopen(P, RTLD_LAZY)
    #else
        #include "dlfcn.h"
        #define DLLOAD(P) dlopen(P, RTLD_LAZY)
    #endif
    #define DLSYM(H, S) dlsym(H, S)
    #define DLERR dlerror()
#endif

#endif //_COMPILED_H
