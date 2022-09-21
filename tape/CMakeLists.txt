SET(GLD_MODULE_NAME tape)

if (GLD_USE_HDF5)
    FIND_PACKAGE (HDF5 REQUIRED COMPONENTS CXX)
endif ()

add_library(${GLD_MODULE_NAME}
        collector.cpp
        collector.h
        file.cpp
        file.h
        group_recorder.h
        group_recorder.cpp
        metrics_collector.cpp
        metrics_collector.h
        metrics_collector_writer.cpp
        metrics_collector_writer.h
        violation_recorder.h
        violation_recorder.cpp
        histogram.cpp
        histogram.h
        loadshape.cpp
        loadshape.h
        main.cpp
        memory.cpp
        memory.h
        multi_recorder.cpp
        multi_recorder.h
        #        odbc.cpp
        #        odbc.h
        player.cpp
        player.h
        recorder.cpp
        recorder.h
        shaper.cpp
        shaper.h
        tape.cpp
        tape.h
        )

if (GLD_USE_EIGEN)
    target_include_directories(${GLD_MODULE_NAME} PRIVATE
            "${CMAKE_SOURCE_DIR}/third_party/Eigen"
            )
    target_link_libraries(${GLD_MODULE_NAME} PRIVATE
            Eigen3::Eigen
            )
else ()
    target_include_directories(${GLD_MODULE_NAME} PRIVATE
            #            "${CMAKE_SOURCE_DIR}/third_party/SuperLU/SRC"
            "${CMAKE_SOURCE_DIR}/third_party/superLU_MT"
            )
    target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${SUPERLU_LIB})
endif ()

target_include_directories(${GLD_MODULE_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/jsonCpp"
        )
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${JSONCPP_LIB})

if (HDF5_FOUND)
    target_link_libraries(${GLD_MODULE_NAME} PUBLIC ${HDF5_LIBRARIES})
    target_include_directories(${GLD_MODULE_NAME} PUBLIC ${HDF5_INCLUDE_DIRS})
    target_compile_definitions(${GLD_MODULE_NAME} PRIVATE HAVE_HDF5)
endif ()

target_compile_options(${GLD_MODULE_NAME} PRIVATE ${GLD_COMPILE_OPTIONS})
set_target_properties(${GLD_MODULE_NAME} PROPERTIES PREFIX "")
target_include_directories(${GLD_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})
if (GLD_USE_CLANG_TIDY)
    set_target_properties(${GLD_MODULE_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()
