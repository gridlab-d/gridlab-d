project(climate)

add_library(${PROJECT_NAME}
        climate.cpp
        climate.h
        csv_reader.cpp
        csv_reader.h
        init.cpp
        main.cpp
        solar_angles.cpp
        solar_angles.h
        test.cpp
        test.h
        weather.cpp
        weather.h
        weather_reader.cpp
        weather_reader.h
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

