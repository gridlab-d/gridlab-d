pkglib_LTLIBRARIES += generators/generators.la

generators_generators_la_CPPFLAGS =
generators_generators_la_CPPFLAGS += $(AM_CPPFLAGS)

generators_generators_la_LDFLAGS =
generators_generators_la_LDFLAGS += $(AM_LDFLAGS)

generators_generators_la_LIBADD =

generators_generators_la_SOURCES =
generators_generators_la_SOURCES += generators/battery.cpp
generators_generators_la_SOURCES += generators/battery.h
generators_generators_la_SOURCES += generators/central_dg_control.cpp
generators_generators_la_SOURCES += generators/central_dg_control.h
generators_generators_la_SOURCES += generators/dc_dc_converter.cpp
generators_generators_la_SOURCES += generators/dc_dc_converter.h
generators_generators_la_SOURCES += generators/diesel_dg.cpp
generators_generators_la_SOURCES += generators/diesel_dg.h
generators_generators_la_SOURCES += generators/energy_storage.cpp
generators_generators_la_SOURCES += generators/energy_storage.h
generators_generators_la_SOURCES += generators/generators.h
generators_generators_la_SOURCES += generators/init.cpp
generators_generators_la_SOURCES += generators/inverter.cpp
generators_generators_la_SOURCES += generators/inverter.h
generators_generators_la_SOURCES += generators/main.cpp
generators_generators_la_SOURCES += generators/microturbine.cpp
generators_generators_la_SOURCES += generators/microturbine.h
generators_generators_la_SOURCES += generators/power_electronics.cpp
generators_generators_la_SOURCES += generators/power_electronics.h
generators_generators_la_SOURCES += generators/rectifier.cpp
generators_generators_la_SOURCES += generators/rectifier.h
generators_generators_la_SOURCES += generators/solar.cpp
generators_generators_la_SOURCES += generators/solar.h
generators_generators_la_SOURCES += generators/windturb_dg.cpp
generators_generators_la_SOURCES += generators/windturb_dg.h
