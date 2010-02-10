pkglib_LTLIBRARIES = tape.la
tape_la_SOURCES =            \
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
tape_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

