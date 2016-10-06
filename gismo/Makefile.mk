pkglib_LTLIBRARIES += gismo/gismo.la

gismo_gismo_la_CPPFLAGS =
gismo_gismo_la_CPPFLAGS += $(AM_CPPFLAGS)

gismo_gismo_la_LDFLAGS =
gismo_gismo_la_LDFLAGS += $(AM_LDFLAGS)

gismo_gismo_la_LIBADD = 

gismo_gismo_la_SOURCES =
gismo_gismo_la_SOURCES += gismo/line_sensor.cpp 
gismo_gismo_la_SOURCES += gismo/line_sensor.h
gismo_gismo_la_SOURCES += gismo/switch_coordinator.cpp 
gismo_gismo_la_SOURCES += gismo/switch_coordinate.h
gismo_gismo_la_SOURCES += gismo/main.cpp
