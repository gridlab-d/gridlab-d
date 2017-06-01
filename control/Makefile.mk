pkglib_LTLIBRARIES = control/control.la

control_control_la_CPPFLAGS =
control_control_la_CPPFLAGS =+ $(AM_CPPFLAGS)

control_control_la_LDFLAGS =
control_control_la_LDFLAGS =+ $(AM_LDFLAGS

control_control_la_LIBADD =

control_control_la_SOURCES =
control_control_la_SOURCES += control/init.cpp
control_control_la_SOURCES += control/main.cpp
control_control_la_SOURCES += control/ss_model.cpp
control_control_la_SOURCES += control/ss_model.h
