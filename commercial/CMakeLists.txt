project(commercial)

include_directories(../${GLDCORE_LOC})

add_library(${PROJECT_NAME} SHARED
        commercial/hvac.cpp
        commercial/hvac.h
        commercial/init.cpp
        commercial/main.cpp
        commercial/multizone.cpp
        commercial/multizone.h
        commercial/office.cpp
        commercial/office.h
        commercial/solvers.cpp
        commercial/solvers.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})