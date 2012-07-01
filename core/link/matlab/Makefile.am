pkglib_LTLIBRARIES = glxmatlab.la
glxmatlab_la_SOURCES = matlab.cpp
glxmatlab_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
