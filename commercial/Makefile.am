pkglib_LTLIBRARIES = libcommercial.la
libcommercial_la_SOURCES =   \
	hvac.cpp      hvac.h      \
	init.cpp                  \
	main.cpp                  \
	multizone.cpp multizone.h \
	office.cpp    office.h    \
	solvers.cpp   solvers.h
libcommercial_la_LDFLAGS = -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

