pkglib_LTLIBRARIES += climate/climate.la

climate_climate_la_CPPFLAGS =
climate_climate_la_CPPFLAGS += $(AM_CPPFLAGS)

climate_climate_la_LDFLAGS =
climate_climate_la_LDFLAGS += $(AM_LDFLAGS)

climate_climate_la_LIBADD =

climate_climate_la_SOURCES =
climate_climate_la_SOURCES += climate/climate.cpp
climate_climate_la_SOURCES += climate/climate.h
climate_climate_la_SOURCES += climate/csv_reader.cpp
climate_climate_la_SOURCES += climate/csv_reader.h
climate_climate_la_SOURCES += climate/init.cpp
climate_climate_la_SOURCES += climate/main.cpp
climate_climate_la_SOURCES += climate/solar_angles.cpp
climate_climate_la_SOURCES += climate/solar_angles.h
climate_climate_la_SOURCES += climate/test.cpp
climate_climate_la_SOURCES += climate/test.h
climate_climate_la_SOURCES += climate/weather.cpp
climate_climate_la_SOURCES += climate/weather.h
climate_climate_la_SOURCES += climate/weather_reader.cpp
climate_climate_la_SOURCES += climate/weather_reader.h
