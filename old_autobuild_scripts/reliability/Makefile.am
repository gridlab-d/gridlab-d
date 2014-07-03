pkglib_LTLIBRARIES = reliability.la
reliability_la_SOURCES =    \
	eventgen.cpp eventgen.h  \
	init.cpp                 \
	main.cpp                 \
	metrics.cpp  metrics.h   \
	reliability.h            \
	# NEWCLASS
reliability_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

