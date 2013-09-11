pkglib_LTLIBRARIES = glsolvers.la
glsolvers_la_SOURCES = main.cpp etp.cpp modified_euler.cpp
glsolvers_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
