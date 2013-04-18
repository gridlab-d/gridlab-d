pkglib_LTLIBRARIES = residential.la
residential_la_SOURCES =                            \
	appliance.cpp          appliance.h          \
	clotheswasher.cpp      clotheswasher.h      \
	dishwasher.cpp         dishwasher.h         \
	dryer.cpp              dryer.h              \
	evcharger.cpp          evcharger.h          \
	evcharger_det.cpp      evcharger_det.h      \
	freezer.cpp            freezer.h            \
	house_a.cpp            house_a.h            \
	house_e.cpp            house_e.h            \
	init.cpp                                    \
	lights.cpp             lights.h             \
	main.cpp                                    \
	microwave.cpp          microwave.h          \
	occupantload.cpp       occupantload.h       \
	plugload.cpp           plugload.h           \
	range.cpp              range.h              \
	residential.h                               \
	refrigerator.cpp       refrigerator.h       \
	residential_enduse.cpp residential_enduse.h \
	solvers.cpp            solvers.h            \
	thermal_storage.cpp    thermal_storage.h    \
	waterheater.cpp        waterheater.h        \
	zipload.cpp            zipload.h
residential_la_LDFLAGS = -ldl -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

