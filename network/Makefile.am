pkglib_LTLIBRARIES = libnetwork.la
libnetwork_la_SOURCES =          \
	capbank.cpp     capbank.h     \
	check.cpp                     \
	export.cpp                    \
	fuse.cpp        fuse.h        \
	generator.cpp   generator.h   \
	globals.c                     \
	import.cpp                    \
	link.cpp        link.h        \
	main.cpp                      \
	meter.cpp       meter.h       \
	network.cpp     network.h     \
	node.cpp        node.h        \
	regulator.cpp   regulator.h   \
	relay.cpp       relay.h       \
	transformer.cpp transformer.h \
	varmap.c
libnetwork_la_LDFLAGS = -no-undefined -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)

