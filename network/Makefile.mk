pkglib_LTLIBRARIES = network/network.la

network_network_la_CPPFLAGS =
network_network_la_CPPFLAGS = $(AM_CPPFLAGS)

network_network_la_LDFLAGS =
network_network_la_LDFLAGS = $(AM_LDFLAGS)

network_network_la_LIBADD =

network_network_la_SOURCES =
network_network_la_SOURCES += network/capbank.cpp
network_network_la_SOURCES += network/capbank.h
network_network_la_SOURCES += network/check.cpp
network_network_la_SOURCES += network/export.cpp
network_network_la_SOURCES += network/fuse.cpp
network_network_la_SOURCES += network/fuse.h
network_network_la_SOURCES += network/generator.cpp
network_network_la_SOURCES += network/generator.h
network_network_la_SOURCES += network/globals.c
network_network_la_SOURCES += network/import.cpp
network_network_la_SOURCES += network/link.cpp
network_network_la_SOURCES += network/link.h
network_network_la_SOURCES += network/main.cpp
network_network_la_SOURCES += network/meter.cpp
network_network_la_SOURCES += network/meter.h
network_network_la_SOURCES += network/network.cpp
network_network_la_SOURCES += network/network.h
network_network_la_SOURCES += network/node.cpp
network_network_la_SOURCES += network/node.h
network_network_la_SOURCES += network/regulator.cpp
network_network_la_SOURCES += network/regulator.h
network_network_la_SOURCES += network/relay.cpp
network_network_la_SOURCES += network/relay.h
network_network_la_SOURCES += network/transformer.cpp
network_network_la_SOURCES += network/transformer.h
network_network_la_SOURCES += network/varmap.c
