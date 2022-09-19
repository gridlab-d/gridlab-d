SET(GLD_MODULE_NAME residential)

add_library(${GLD_MODULE_NAME}
        appliance.cpp
        appliance.h
        clotheswasher.cpp
        clotheswasher.h
        dishwasher.cpp
        dishwasher.h
        dryer.cpp
        dryer.h
        evcharger.cpp
        evcharger_det.cpp
        evcharger_det.h
        evcharger.h
        freezer.cpp
        freezer.h
        house_e.cpp
        house_e.h
        init.cpp
        lights.cpp
        lights.h
        main.cpp
        microwave.cpp
        microwave.h
        occupantload.cpp
        occupantload.h
        plugload.cpp
        plugload.h
        range.cpp
        range.h
        refrigerator.cpp
        refrigerator.h
        residential_enduse.cpp
        residential_enduse.h
        residential.h
        rbsa2014.h
        solvers.cpp
        solvers.h
        thermal_storage.cpp
        thermal_storage.h
        waterheater.cpp
        waterheater.h
        zipload.cpp
        zipload.h
        elcap1990.h
        elcap2010.h
)

target_compile_options(${GLD_MODULE_NAME} PRIVATE ${GLD_COMPILE_OPTIONS})
set_target_properties(${GLD_MODULE_NAME} PROPERTIES PREFIX "")
target_include_directories(${GLD_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gldcore")
target_link_libraries(${GLD_MODULE_NAME} PRIVATE ${OS_SPECIFIC_LIBRARIES})

install(TARGETS ${GLD_MODULE_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Libraries)

if (GLD_USE_CLANG_TIDY)
    set_target_properties(${GLD_MODULE_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
            )
endif ()
