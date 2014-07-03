pkglib_LTLIBRARIES = comm/comm.la

comm_comm_la_CPPFLAGS =
comm_comm_la_CPPFLAGS += $(AM_CPPFLAGS)

comm_comm_la_LDFLAGS =
comm_comm_la_LDFLAGS += $(AM_LDFLAGS)

comm_comm_la_LIBADD =

comm_comm_la_SOURCES =
comm_comm_la_SOURCES += comm/comm.h
comm_comm_la_SOURCES += comm/init.cpp
comm_comm_la_SOURCES += comm/main.cpp
comm_comm_la_SOURCES += comm/mpi_network.cpp
comm_comm_la_SOURCES += comm/mpi_network.h
comm_comm_la_SOURCES += comm/network.cpp
comm_comm_la_SOURCES += comm/network.h
comm_comm_la_SOURCES += comm/network_interface.cpp
comm_comm_la_SOURCES += comm/network_interface.h
comm_comm_la_SOURCES += comm/network_message.cpp
comm_comm_la_SOURCES += comm/network_message.h
