project(powerflow)

if (GLD_USE_EIGEN)
    SET(NR_SOLVER solver_nr_eigen.cpp solver_nr_eigen.h)
else ()
    SET(NR_SOLVER solver_nr.cpp solver_nr.h)
endif ()

add_library(${PROJECT_NAME}
        billdump.cpp
        billdump.h
        capacitor.cpp
        capacitor.h
        currdump.cpp
        currdump.h
        emissions.cpp
        emissions.h
        fault_check.cpp
        fault_check.h
        fuse.cpp
        fuse.h
        impedance_dump.cpp
        impedance_dump.h
        jsondump.cpp
        jsondump.h
        init.cpp
        line_configuration.cpp
        line_configuration.h
        line.cpp
        line.h
        line_spacing.cpp
        line_spacing.h
        link.cpp
        link.h
        load.cpp
        load.h
        load_tracker.cpp
        load_tracker.h
        main.cpp
        meter.cpp
        meter.h
        meter_test.h
        motor.cpp
        motor.h
        node.cpp
        node.h
        overhead_line_conductor.cpp
        overhead_line_conductor.h
        overhead_line.cpp
        overhead_line.h
        overheadline_test.h
        powerflow.cpp
        powerflow.h
        powerflow_library.cpp
        powerflow_library.h
        powerflow_object.cpp
        powerflow_object.h
        power_metrics.cpp
        power_metrics.h
        pqload.cpp
        pqload.h
        recloser.cpp
        recloser.h
        regulator_configuration.cpp
        regulator_configuration.h
        regulator.cpp
        regulator.h
        regulator_test.h
        restoration.cpp
        restoration.h
        sectionalizer.cpp
        sectionalizer.h
        series_reactor.cpp
        series_reactor.h
        substation.cpp
        substation.h
        switch_object.cpp
        switch_object.h
        test.cpp
        test.h
        transformer_configuration.cpp
        transformer_configuration.h
        transformer.cpp
        transformer.h
        transformer_test.h
        triplex_line_conductor.cpp
        triplex_line_conductor.h
        triplex_line_configuration.cpp
        triplex_line_configuration.h
        triplex_line.cpp
        triplex_line.h
        triplexline_test.h
        triplex_load.h
        triplex_load.cpp
        triplex_meter.cpp
        triplex_meter.h
        triplex_node.cpp
        triplex_node.h
        underground_line_conductor.cpp
        underground_line_conductor.h
        underground_line.cpp
        underground_line.h
        undergroundline_test.h
        vfd.cpp
        vfd.h
        voltdump.cpp
        voltdump.h
        volt_var_control.cpp
        volt_var_control.h

        ${NR_SOLVER}
        )

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party/Eigen/cmake) # Gives us FindKLU.cmake without replicating code
find_package(KLU)
if(GLD_USE_EIGEN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLD_USE_EIGEN)
    if (KLU_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE GLD_USE_KLU)
    endif ()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/SuperLU/SRC"
        "${CMAKE_SOURCE_DIR}/third_party/jsoncpp/include"
        "${CMAKE_SOURCE_DIR}/third_party/Eigen"
        "${KLU_INCLUDES}"
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        superlu
        jsoncpp_lib
        Eigen3::Eigen
        ${KLU_LIBRARIES}
        )

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT library)

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()