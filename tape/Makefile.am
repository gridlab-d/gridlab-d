pkglib_LTLIBRARIES = libtape.la
libtape_la_SOURCES =         \
	collector.c               \
	file.c        file.h      \
	histogram.cpp histogram.h \
	loadshape.cpp loadshape.h \
	main.cpp                  \
	memory.c      memory.h    \
	odbc.c        odbc.h      \
	player.c                  \
	recorder.c                \
	shaper.c                  \
	tape.c        tape.h
libtape_la_LDFLAGS = -no-undefined -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

