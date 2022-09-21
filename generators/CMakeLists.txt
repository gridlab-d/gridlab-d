project(generators)

add_library(${PROJECT_NAME}
        battery.cpp
        battery.h
        central_dg_control.cpp
        central_dg_control.h
        controller_dg.cpp
        controller_dg.h
        diesel_dg.cpp
        diesel_dg.h
        energy_storage.cpp
        energy_storage.h
        generators.h
        init.cpp
        inverter.cpp
        inverter.h
        inverter_dyn.cpp
        inverter_dyn.h
        main.cpp
        rectifier.cpp
        rectifier.h
        sec_control.cpp
        sec_control.h
        solar.cpp
        solar.h
        sync_ctrl.cpp
        sync_ctrl.h
        windturb_dg.cpp
        windturb_dg.h
        )

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")


if (GLD_USE_EIGEN)
    target_include_directories(${PROJECT_NAME} PRIVATE
            "${CMAKE_SOURCE_DIR}/third_party/Eigen"
            )
    target_link_libraries(${PROJECT_NAME} PRIVATE
            Eigen3::Eigen
            )
else ()
    target_include_directories(${PROJECT_NAME} PRIVATE
            #            "${CMAKE_SOURCE_DIR}/third_party/SuperLU/SRC"
            "${CMAKE_SOURCE_DIR}/third_party/superLU_MT"
            )
    target_link_libraries(${PROJECT_NAME} PRIVATE
            #            superlu
            superlu_lib
            )
endif ()

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
