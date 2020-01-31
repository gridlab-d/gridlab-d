project(commercial)

add_library(${PROJECT_NAME}
        hvac.cpp
        hvac.h
        init.cpp
        main.cpp
        multizone.cpp
        multizone.h
        office.cpp
        office.h
        solvers.cpp
        solvers.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()