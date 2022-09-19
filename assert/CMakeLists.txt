project(assert)

add_library(${PROJECT_NAME}
        gld_assert.cpp
        gld_assert.h
        complex_assert.cpp
        complex_assert.h
        double_assert.cpp
        double_assert.h
        enum_assert.cpp
        enum_assert.h
        int_assert.cpp
        int_assert.h
        init.cpp
        main.cpp
        )

target_compile_options(${PROJECT_NAME} PRIVATE ${GLD_COMPILE_OPTIONS})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Libraries)

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()

