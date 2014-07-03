pkglib_LTLIBRARIES = market.la
market_la_SOURCES =                                       \
	auction.cpp                auction.h              \
	bid.cpp                    bid.h                  \
	controller.cpp             controller.h           \
	curve.cpp                  curve.h                \
	double_controller.cpp      double_controller.h    \
	generator_controller.cpp   generator_controller.h \
	init.cpp                                          \
	main.cpp                                          \
	market.h                                          \
	passive_controller.cpp     passive_controller.h   \
	stub_bidder.cpp            stub_bidder.h	  \
	stubauction.cpp            stubauction.h          \
	# DO NOT EDIT THE FOLLOWING LINE \
	# NEWCLASS
market_la_LDFLAGS = -module -no-undefined -avoid-version -version-info 0:0:0

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
