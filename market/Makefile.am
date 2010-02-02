pkglib_LTLIBRARIES = libmarket.la
libmarket_la_SOURCES =              \
	auction.cpp    auction.h         \
	controller.cpp controller.h      \
	init.cpp                         \
	main.cpp                         \
	market.h                         \
	stubauction.cpp stubauction.h    \
	controller2.cpp controller2.h	 \
	double_controller.cpp double_controller.h	\
	# DO NOT EDIT THE FOLLOWING LINE \
	# NEWCLASS
libmarket_la_LDFLAGS = -no-undefined -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)