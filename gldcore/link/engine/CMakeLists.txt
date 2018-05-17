project(glxengine)

include_directories(../../../gldcore)

add_library(${PROJECT_NAME} SHARED
        gldcore/link/engine/engine.cpp
        gldcore/link/engine/engine.h
        gldcore/link/engine/udpconnection.cpp
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )