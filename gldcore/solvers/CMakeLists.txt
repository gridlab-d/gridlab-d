project(glsolvers)

include_directories(..)

add_library(${PROJECT_NAME}
        ${GLDCORE_LOC}/solvers/etp.cpp
        ${GLDCORE_LOC}/solvers/main.cpp
        ${GLDCORE_LOC}/solvers/modified_euler.cpp
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
