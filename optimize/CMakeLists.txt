project(optimize)

include_directories(../${GLDCORE_LOC})

add_library(${PROJECT_NAME}
        optimize/init.cpp
        optimize/main.cpp
        optimize/optimize.h
        optimize/simple.cpp
        optimize/simple.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
