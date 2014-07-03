pkglib_LTLIBRARIES = climate.la
climate_la_SOURCES =                   \
	climate.cpp        climate.h        \
	csv_reader.cpp     csv_reader.h     \
	init.cpp                            \
	main.cpp                            \
	solar_angles.cpp   solar_angles.h   \
	test.cpp           test.h           \
	weather.cpp        weather.h        \
	weather_reader.cpp weather_reader.h
climate_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

