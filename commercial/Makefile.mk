pkglib_LTLIBRARIES += commercial/commercial.la

commercial_commercial_la_CPPFLAGS = -std=c++11
commercial_commercial_la_CPPFLAGS += -I$(top_srcdir)/gldcore
commercial_commercial_la_CPPFLAGS += -I$(top_srcdir)/powerflow
commercial_commercial_la_CPPFLAGS += $(AM_CPPFLAGS)

commercial_commercial_la_LDFLAGS =
commercial_commercial_la_LDFLAGS += $(AM_LDFLAGS)
commercial_commercial_la_LDFLAGS += -ldl

commercial_commercial_la_LIBADD =
commercial_commercial_la_LIBADD += -ldl

commercial_commercial_la_SOURCES =
commercial_commercial_la_SOURCES += commercial/commercial_exception.hpp
commercial_commercial_la_SOURCES += commercial/commercial_library.hpp
commercial_commercial_la_SOURCES += commercial/commercial_object.cpp
commercial_commercial_la_SOURCES += commercial/commercial_object.hpp
commercial_commercial_la_SOURCES += commercial/hvacmodel.hpp
commercial_commercial_la_SOURCES += commercial/regression_equation.cpp
commercial_commercial_la_SOURCES += commercial/regression_equation.hpp
commercial_commercial_la_SOURCES +=	commercial/commercial.cpp
commercial_commercial_la_SOURCES += commercial/commercial.h
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
