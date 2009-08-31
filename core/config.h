#ifndef _CORE_CONFIG
#define _CORE_CONFIG

#ifndef DLEXT
#ifdef WIN32
#define DLEXT "" 
#elif MACOSX
#define DLEXT ".dylib"
#elif LINUX
#define DLEXT ".so"
#endif
#endif // LIBPREFIX

#endif