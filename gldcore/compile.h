#ifndef _COMPILE_H
#define _COMPILED_H

#if defined(_WIN32) && !defined(__MINGW32__)
    #define _WIN32_WINNT 0x0400
    #undef int64                // wtypes.h also used int64
//    #define WIN32_LEAN_AND_MEAN // Exclude rarely used Windows headers
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
#else // LINUX/MAC/MINGW
    #include "dlfcn.h"
    #define PREFIX ""
    #ifndef DLEXT
        #ifdef __MINGW32__
            #define DLEXT ".dll"
        #else
            #define DLEXT ".so"
        #endif
    #endif
    #define DLLOAD(P) dlopen(P, RTLD_LAZY)
    #define DLSYM(H, S) dlsym(H, S)
    #define DLERR dlerror()
#endif

#endif //_COMPILED_H
