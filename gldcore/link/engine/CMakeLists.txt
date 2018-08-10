project(glxengine)

include_directories(../../../${GLDCORE_LOC})

add_library(${PROJECT_NAME} SHARED
        ${GLDCORE_LOC}/link/engine/engine.cpp
        ${GLDCORE_LOC}/link/engine/engine.h
        ${GLDCORE_LOC}/link/engine/udpconnection.cpp
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
