project(glsolvers)

include_directories(..)

add_library(${PROJECT_NAME} SHARED
        gldcore/solvers/etp.cpp
        gldcore/solvers/main.cpp
        gldcore/solvers/modified_euler.cpp
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )
