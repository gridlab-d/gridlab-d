project(reliability)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        reliability/eventgen.cpp
        reliability/eventgen.h
        reliability/init.cpp
        reliability/main.cpp
        reliability/metrics.cpp
        reliability/metrics.h
        reliability/reliability.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )