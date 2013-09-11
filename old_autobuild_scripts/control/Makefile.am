pkglib_LTLIBRARIES = control.la
control_la_SOURCES = \
	ss_model.cpp ss_model.h \
	init.cpp main.cpp 
control_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

