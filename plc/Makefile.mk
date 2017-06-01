pkglib_LTLIBRARIES = plc/plc.la

plc_plc_la_CPPFLAGS =
plc_plc_la_CPPFLAGS += $(AM_CPPFLAGS)

plc_plc_la_LDFLAGS =
plc_plc_la_LDFLAGS += $(AM_LDFLAGS)

plc_plc_la_LIBADD =

plc_plc_la_SOURCES =
plc_plc_la_SOURCES += plc/comm.cpp
plc_plc_la_SOURCES += plc/comm.h
plc_plc_la_SOURCES += plc/init.cpp
plc_plc_la_SOURCES += plc/machine.cpp
plc_plc_la_SOURCES += plc/machine.h
plc_plc_la_SOURCES += plc/main.cpp
plc_plc_la_SOURCES += plc/plc.cpp
plc_plc_la_SOURCES += plc/plc.h
plc_plc_la_SOURCES += plc/plc_test.h
plc_plc_la_SOURCES += plc/test.cpp
