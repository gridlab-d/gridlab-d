pkglib_LTLIBRARIES += mysql/mysql.la

mysql_mysql_la_CPPFLAGS =
mysql_mysql_la_CPPFLAGS += $(MYSQL_CPPFLAGS)
mysql_mysql_la_CPPFLAGS += $(AM_CPPFLAGS)

mysql_mysql_la_LDFLAGS =
mysql_mysql_la_LDFLAGS += $(MYSQL_LDFLAGS)
mysql_mysql_la_LDFLAGS += $(AM_LDFLAGS)

mysql_mysql_la_LIBADD =
mysql_mysql_la_LIBADD += $(MYSQL_LIBS)

mysql_mysql_la_SOURCES =
mysql_mysql_la_SOURCES += mysql/collector.cpp
mysql_mysql_la_SOURCES += mysql/collector.h
mysql_mysql_la_SOURCES += mysql/database.cpp
mysql_mysql_la_SOURCES += mysql/database.h
mysql_mysql_la_SOURCES += mysql/init.cpp
mysql_mysql_la_SOURCES += mysql/main.cpp
mysql_mysql_la_SOURCES += mysql/player.cpp
mysql_mysql_la_SOURCES += mysql/player.h
mysql_mysql_la_SOURCES += mysql/recorder.cpp
mysql_mysql_la_SOURCES += mysql/recorder.h
