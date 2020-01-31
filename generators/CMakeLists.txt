project(generators)

add_library(${PROJECT_NAME}
        battery.cpp
        battery.h
        central_dg_control.cpp
        central_dg_control.h
        diesel_dg.cpp
        diesel_dg.h
        energy_storage.cpp
        energy_storage.h
        generators.h
        init.cpp
        inverter.cpp
        inverter.h
        main.cpp
        rectifier.cpp
        rectifier.h
        solar.cpp
        solar.h
        windturb_dg.cpp
        windturb_dg.h
        controller_dg.cpp
        controller_dg.h
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/Eigen"
        )
target_link_libraries(${PROJECT_NAME} PRIVATE
        Eigen3::Eigen
        )

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()
