pkglib_LTLIBRARIES += commercial/commercial.la

commercial_commercial_la_CPPFLAGS =
commercial_commercial_la_CPPFLAGS += $(AM_CPPFLAGS)

commercial_commercial_la_LDFLAGS =
commercial_commercial_la_LDFLAGS += $(AM_LDFLAGS)

commercial_commercial_la_LIBADD =

commercial_commercial_la_SOURCES =
commercial_commercial_la_SOURCES += commercial/hvac.cpp
commercial_commercial_la_SOURCES += commercial/hvac.h
commercial_commercial_la_SOURCES += commercial/init.cpp
commercial_commercial_la_SOURCES += commercial/main.cpp
commercial_commercial_la_SOURCES += commercial/multizone.cpp
commercial_commercial_la_SOURCES += commercial/multizone.h
commercial_commercial_la_SOURCES += commercial/office.cpp
commercial_commercial_la_SOURCES += commercial/office.h
commercial_commercial_la_SOURCES += commercial/solvers.cpp
commercial_commercial_la_SOURCES += commercial/solvers.h
