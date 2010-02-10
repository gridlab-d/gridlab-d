pkglib_LTLIBRARIES = market.la
market_la_SOURCES =                 \
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
market_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
