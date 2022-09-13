project(xml)

include_directories(../gldcore)

add_library(${PROJECT_NAME}
        loader/Xerces_loader.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
