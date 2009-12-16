pkglib_LTLIBRARIES = libreliability.la
libreliability_la_SOURCES = \
	eventgen.cpp eventgen.h  \
	init.cpp                 \
	main.cpp                 \
	metrics.cpp  metrics.h   \
	reliability.h            \
	# NEWCLASS
libreliability_la_LDFLAGS = -no-undefined -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

