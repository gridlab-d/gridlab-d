cmake_minimum_required(VERSION 3.10)
project(market)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(market SHARED
        auction.cpp
        auction.h
        bid.cpp
        bid.h
        collect.cpp
        collect.h
        controller.cpp
        controller.h
#       controller2.cpp
#       controller2.h
        curve.cpp
        curve.h
        double_controller.cpp
        double_controller.h
        generator_controller.cpp
        generator_controller.h
        init.cpp
        main.cpp
        passive_controller.cpp
        passive_controller.h
        stub_bidder.cpp
        stub_bidder.h
        stubauction.cpp
        stubauction.h
        supervisory_control.cpp
        supervisory_control.h
        market.h
)

set_target_properties(market
        PROPERTIES PREFIX ""
        )

install(TARGETS market
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )