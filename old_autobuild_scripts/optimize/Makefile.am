pkglib_LTLIBRARIES = optimize.la
optimize_la_SOURCES = \
	init.cpp main.cpp optimize.h \
	simple.cpp simple.h \
	# DO NOT EDIT THE FOLLOWING LINE \
	# NEWCLASS
optimize_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
