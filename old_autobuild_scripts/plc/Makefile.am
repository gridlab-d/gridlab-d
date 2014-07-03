pkglib_LTLIBRARIES = plc.la
plc_la_SOURCES =         \
	comm.cpp	   comm.h    \
	init.cpp              \
	machine.cpp	machine.h \
	main.cpp              \
	plc.cpp     plc.h     \
	plc_test.h            \
	test.cpp
plc_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0 -ldl

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

