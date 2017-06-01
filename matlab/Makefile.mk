pkglib_LTLIBRARIES = matlab/matlab.la

matlab_matlab_la_CPPFLAGS =
matlab_matlab_la_CPPFLAGS += $(AM_CPPFLAGS)

matlab_matlab_la_LDFLAGS =
matlab_matlab_la_LDFLAGS += $(AM_LDFLAGS)

matlab_matlab_la_LIBADD =

matlab_matlab_la_SOURCES =
matlab_matlab_la_SOURCES += matlab/init.cpp
matlab_matlab_la_SOURCES += matlab/main.cpp
matlab_matlab_la_SOURCES += matlab/matlab.h
