pkglib_LTLIBRARIES = libplc.la
libplc_la_SOURCES =      \
	comm.cpp	   comm.h    \
	init.cpp              \
	machine.cpp	machine.h \
	main.cpp              \
	plc.cpp     plc.h     \
	plc_test.h            \
	test.cpp
libplc_la_LDFLAGS = -no-undefined -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

