pkglib_LTLIBRARIES += connection/connection.la

connection_connection_la_CPPFLAGS =
connection_connection_la_CPPFLAGS += -I$(top_srcdir)/third_party/jsonCpp
connection_connection_la_CPPFLAGS += $(AM_CPPFLAGS)
if HAVE_FNCS
connection_connection_la_CPPFLAGS += $(FNCS_CPPFLAGS)
endif

connection_connection_la_LDFLAGS =
connection_connection_la_LDFLAGS += $(AM_LDFLAGS)
if HAVE_FNCS
connection_connection_la_LDFLAGS += $(FNCS_LDFLAGS)
endif

connection_connection_la_LIBADD = 
connection_connection_la_LIBADD += third_party/jsonCpp/libjsoncpp.la
if HAVE_FNCS
connection_connection_la_LIBADD += $(FNCS_LIBS)
endif
connection_connection_la_LIBADD += $(PTHREAD_CFLAGS)
connection_connection_la_LIBADD += $(PTHREAD_LIBS)
connection_connection_la_LIBADD += -ldl

connection_connection_la_SOURCES =
connection_connection_la_SOURCES += connection/connection.cpp
connection_connection_la_SOURCES += connection/connection.h
connection_connection_la_SOURCES += connection/socket.cpp
connection_connection_la_SOURCES += connection/socket.h
connection_connection_la_SOURCES += connection/server.cpp
connection_connection_la_SOURCES += connection/server.h
connection_connection_la_SOURCES += connection/cache.cpp
connection_connection_la_SOURCES += connection/cache.h
connection_connection_la_SOURCES += connection/client.cpp
connection_connection_la_SOURCES += connection/client.h
connection_connection_la_SOURCES += connection/native.cpp
connection_connection_la_SOURCES += connection/native.h
connection_connection_la_SOURCES += connection/tcp.cpp
connection_connection_la_SOURCES += connection/tcp.h
connection_connection_la_SOURCES += connection/udp.cpp
connection_connection_la_SOURCES += connection/udp.h
connection_connection_la_SOURCES += connection/xml.cpp
connection_connection_la_SOURCES += connection/xml.h
connection_connection_la_SOURCES += connection/json.cpp
connection_connection_la_SOURCES += connection/json.h
if HAVE_FNCS
connection_connection_la_SOURCES += connection/fncs_msg.cpp
connection_connection_la_SOURCES += connection/fncs_msg.h
endif
connection_connection_la_SOURCES += connection/transport.cpp
connection_connection_la_SOURCES += connection/transport.h
connection_connection_la_SOURCES += connection/message.h
connection_connection_la_SOURCES += connection/varmap.cpp
connection_connection_la_SOURCES += connection/varmap.h
connection_connection_la_SOURCES += connection/init.cpp
connection_connection_la_SOURCES += connection/main.cpp
