pkglib_LTLIBRARIES = debug/gldebug.la

debug_gldebug_la_CPPFLAGS =
debug_gldebug_la_CPPFLAGS += $(AM_CPPFLAGS)

debug_gldebug_la_LDFLAGS =
debug_gldebug_la_LDFLAGS += $(AM_LDFLAGS)

debug_gldebug_la_LIBADD =

debug_gldebug_la_SOURCES =
debug_gldebug_la_SOURCES += gldebug.cpp
debug_gldebug_la_SOURCES += gldebug.h
debug_gldebug_la_SOURCES += init.cpp
debug_gldebug_la_SOURCES += main.cpp
