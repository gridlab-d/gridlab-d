pkglib_LTLIBRARIES = libmatlab.la
libmatlab_la_SOURCES = \
	init.cpp \
	main.cpp \
	matlab.h \
	# DO NOT EDIT THE FOLLOWING LINE \
	# NEWCLASS
libmatlab_la_LDFLAGS = -no-undefined -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

