pkglib_LTLIBRARIES = commercial.la
commercial_la_SOURCES =      \
	hvac.cpp      hvac.h      \
	init.cpp                  \
	main.cpp                  \
	multizone.cpp multizone.h \
	office.cpp    office.h    \
	solvers.cpp   solvers.h
commercial_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

