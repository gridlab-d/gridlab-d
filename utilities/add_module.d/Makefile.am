pkglib_LTLIBRARIES = MODULENAME.la
MODULENAME_la_SOURCES = init.cpp main.cpp MODULENAME.h \
	# NEWCLASS 
# DO NOT EDIT THE PREVIOUS LINE

# TODO set the version info as desired
MODULENAME_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

