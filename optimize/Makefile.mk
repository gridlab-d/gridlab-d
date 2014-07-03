pkglib_LTLIBRARIES += optimize/optimize.la

optimize_optimize_la_CPPFLAGS =
optimize_optimize_la_CPPFLAGS += $(AM_CPPFLAGS)

optimize_optimize_la_LDFLAGS =
optimize_optimize_la_LDFLAGS += $(AM_LDFLAGS)

optimize_optimize_la_LIBADD =

optimize_optimize_la_SOURCES =
optimize_optimize_la_SOURCES += optimize/init.cpp
optimize_optimize_la_SOURCES += optimize/main.cpp
optimize_optimize_la_SOURCES += optimize/optimize.h
optimize_optimize_la_SOURCES += optimize/simple.cpp
optimize_optimize_la_SOURCES += optimize/simple.h
