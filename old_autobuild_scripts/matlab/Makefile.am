pkglib_LTLIBRARIES = matlab.la
matlab_la_SOURCES = \
	init.cpp \
	main.cpp \
	matlab.h \
	# DO NOT EDIT THE FOLLOWING LINE \
	# NEWCLASS
matlab_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

