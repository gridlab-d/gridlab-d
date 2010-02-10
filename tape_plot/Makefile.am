pkglib_LTLIBRARIES = tape_plot.la
tape_plot_la_SOURCES = tape_plot.cpp tape_plot.h  
tape_plot_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

