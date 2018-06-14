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
#        market/controller2.cpp
#        market/controller2.h
        market/curve.cpp
        market/curve.h
        market/double_controller.cpp
        market/double_controller.h
        market/generator_controller.cpp
        market/generator_controller.h
        market/init.cpp
        market/main.cpp
        market/passive_controller.cpp
        market/passive_controller.h
        market/stub_bidder.cpp
        market/stub_bidder.h
        market/stubauction.cpp
        market/stubauction.h
        market/supervisory_control.cpp
        market/supervisory_control.h
        market/market.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
