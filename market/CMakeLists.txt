project(market)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        market/auction.cpp
        market/auction.h
        market/bid.cpp
        market/bid.h
        market/collect.cpp
        market/collect.h
        market/controller.cpp
        market/controller.h
        market/curve.cpp
        market/curve.h
        market/double_controller.cpp
        market/double_controller.h
        market/generator_controller.cpp
        market/generator_controller.h
        market/init.cpp
        market/main.cpp
        market/market.h
        market/passive_controller.cpp
        market/passive_controller.h
        market/stub_bidder.cpp
        market/stub_bidder.h
        market/stubauction.cpp
        market/stubauction.h
        market/supervisory_control.cpp
        market/supervisory_control.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
