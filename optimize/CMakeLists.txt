project(optimize)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        optimize/init.cpp
        optimize/main.cpp
        optimize/optimize.h
        optimize/particle_swarm_optimization.cpp
        optimize/particle_swarm_optimization.h
        optimize/simple.cpp
        optimize/simple.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )
