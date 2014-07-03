pkglib_LTLIBRARIES = tape_file.la
tape_file_la_SOURCES = tape_file.cpp tape_file.h  
tape_file_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

