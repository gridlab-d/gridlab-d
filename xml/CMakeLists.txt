SET(GLD_MODULE_NAME xml)

include_directories(../gldcore)

add_library(${GLD_MODULE_NAME}
        loader/Xerces_loader.cpp
)

set_target_properties(${GLD_MODULE_NAME} PROPERTIES PREFIX "")

target_link_libraries(${GLD_MODULE_NAME} ${OS_SPECIFIC_LIBRARIES})
