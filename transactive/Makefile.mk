pkglib_LTLIBRARIES += transactive/transactive.la

transactive_transactive_la_CPPFLAGS = -I/usr/local/cpplapack/include
transactive_transactive_la_CPPFLAGS = $(MATLAB_CPPFLAGS)
transactive_transactive_la_CPPFLAGS += $(AM_CPPFLAGS)

transactive_transactive_la_LDFLAGS = -L/usr/local/cpplapack/lib/mac
transactive_transactive_la_LDFLAGS += $(MATLAB_LDFLAGS)
transactive_transactive_la_LDFLAGS += $(AM_LDFLAGS)

transactive_transactive_la_LIBADD = 
transactive_transactive_la_LIBADD += $(MATLAB_LIBS) 

transactive_transactive_la_SOURCES =
transactive_transactive_la_SOURCES += transactive/linalg.cpp
transactive_transactive_la_SOURCES += transactive/linalg.h
transactive_transactive_la_SOURCES += transactive/solver.cpp
transactive_transactive_la_SOURCES += transactive/solver.h
transactive_transactive_la_SOURCES += transactive/weather.cpp
transactive_transactive_la_SOURCES += transactive/weather.h
transactive_transactive_la_SOURCES += transactive/dispatcher.cpp
transactive_transactive_la_SOURCES += transactive/dispatcher.h
transactive_transactive_la_SOURCES += transactive/generator.cpp
transactive_transactive_la_SOURCES += transactive/generator.h
transactive_transactive_la_SOURCES += transactive/load.cpp
transactive_transactive_la_SOURCES += transactive/load.h
transactive_transactive_la_SOURCES += transactive/intertie.cpp
transactive_transactive_la_SOURCES += transactive/intertie.h
transactive_transactive_la_SOURCES += transactive/controlarea.cpp
transactive_transactive_la_SOURCES += transactive/controlarea.h
transactive_transactive_la_SOURCES += transactive/scheduler.cpp
transactive_transactive_la_SOURCES += transactive/scheduler.h
transactive_transactive_la_SOURCES += transactive/interconnection.cpp
transactive_transactive_la_SOURCES += transactive/interconnection.h
transactive_transactive_la_SOURCES += transactive/transactive.h
transactive_transactive_la_SOURCES += transactive/main.cpp
