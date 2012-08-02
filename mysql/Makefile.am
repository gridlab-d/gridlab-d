pkglib_LTLIBRARIES = mysql.la
mysql_la_SOURCES = \
	database.cpp database.h \
	recorder.cpp recorder.h \
	player.cpp player.h \
	collector.cpp collector.h \
	init.cpp main.cpp 
mysql_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

