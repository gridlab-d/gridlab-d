pkglib_LTLIBRARIES += market/market.la

market_market_la_CPPFLAGS =
market_market_la_CPPFLAGS += $(AM_CPPFLAGS)

market_market_la_LDFLAGS =
market_market_la_LDFLAGS += $(AM_LDFLAGS)

market_market_la_LIBADD =

market_market_la_SOURCES =
market_market_la_SOURCES += market/auction.cpp
market_market_la_SOURCES += market/auction.h
market_market_la_SOURCES += market/bid.cpp
market_market_la_SOURCES += market/bid.h
market_market_la_SOURCES += market/controller.cpp
market_market_la_SOURCES += market/controller.h
market_market_la_SOURCES += market/curve.cpp
market_market_la_SOURCES += market/curve.h
market_market_la_SOURCES += market/double_controller.cpp
market_market_la_SOURCES += market/double_controller.h
market_market_la_SOURCES += market/generator_controller.cpp
market_market_la_SOURCES += market/generator_controller.h
market_market_la_SOURCES += market/init.cpp
market_market_la_SOURCES += market/main.cpp
market_market_la_SOURCES += market/market.h
market_market_la_SOURCES += market/passive_controller.cpp
market_market_la_SOURCES += market/passive_controller.h
market_market_la_SOURCES += market/stub_bidder.cpp
market_market_la_SOURCES += market/stub_bidder.h
market_market_la_SOURCES += market/stubauction.cpp
market_market_la_SOURCES += market/stubauction.h
