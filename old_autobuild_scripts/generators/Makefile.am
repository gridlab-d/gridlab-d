pkglib_LTLIBRARIES = generators.la
generators_la_SOURCES =                      \
	battery.cpp           battery.h           \
	dc_dc_converter.cpp   dc_dc_converter.h   \
	diesel_dg.cpp         diesel_dg.h         \
	energy_storage.cpp    energy_storage.h    \
	generators.h                              \
	init.cpp                                  \
	inverter.cpp          inverter.h          \
	main.cpp                                  \
	microturbine.cpp      microturbine.h      \
	power_electronics.cpp power_electronics.h \
	rectifier.cpp         rectifier.h         \
	solar.cpp solar.h                         \
	windturb_dg.cpp       windturb_dg.h       \
	# DO NOT EDIT THE FOLLOWING LINE          \
	# NEWCLASS
generators_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

