pkglib_LTLIBRARIES = glsolvers.la
glxmatlab_la_SOURCES = main.cpp etp.cpp
glxmatlab_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
