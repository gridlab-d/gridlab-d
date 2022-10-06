SET(GLD_MODULE_NAME reliability)

add_library(${GLD_MODULE_NAME}
        eventgen.cpp
        eventgen.h
        init.cpp
        main.cpp
        metrics.cpp
        metrics.h
        reliability.h
)

target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${JSONCPP_LIB})

target_compile_options(${GLD_MODULE_NAME} PRIVATE ${GLD_COMPILE_OPTIONS})
set_target_properties(${GLD_MODULE_NAME} PROPERTIES PREFIX "")
target_include_directories(${GLD_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})
if (GLD_USE_CLANG_TIDY)
    set_target_properties(${GLD_MODULE_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()
