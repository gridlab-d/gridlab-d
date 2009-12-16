SUBDIRS=tmy
pkglib_LTLIBRARIES = libclimate.la
libclimate_la_SOURCES =                \
	climate.cpp        climate.h        \
	csv_reader.cpp     csv_reader.h     \
	init.cpp                            \
	main.cpp                            \
	solar_angles.cpp   solar_angles.h   \
	test.cpp           test.h           \
	weather.cpp        weather.h        \
	weather_reader.cpp weather_reader.h
libclimate_la_LDFLAGS = -no-undefined -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

