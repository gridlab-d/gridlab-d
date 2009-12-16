pkglib_LTLIBRARIES = libresidential.la
libresidential_la_SOURCES =                    \
	clotheswasher.cpp      clotheswasher.h      \
	dishwasher.cpp         dishwasher.h         \
	dryer.cpp              dryer.h              \
	evcharger.cpp          evcharger.h          \
	freezer.cpp            freezer.h            \
	house_a.cpp            house_a.h            \
	house_e.cpp            house_e.h            \
	init.cpp                                    \
	lights.cpp             lights.h             \
	main.cpp                                    \
	meter.h                                     \
	microwave.cpp          microwave.h          \
	occupantload.cpp       occupantload.h       \
	plugload.cpp           plugload.h           \
	range.cpp              range.h              \
	refrigerator.cpp       refrigerator.h       \
	residential_enduse.cpp residential_enduse.h \
	residential.h                               \
	solvers.cpp            solvers.h            \
	waterheater.cpp        waterheater.h        \
	zipload.cpp            zipload.h
libresidential_la_LDFLAGS = -no-undefined -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

