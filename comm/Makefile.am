pkglib_LTLIBRARIES = comm.la
comm_la_SOURCES =                                   \
	comm.h                                      \
	init.cpp                                    \
	main.cpp                                    \
	network.cpp             network.h           \
	network_interface.cpp   network_interface.h \
	network_message.cpp     network_message.h
comm_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 1:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

