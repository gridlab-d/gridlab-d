SET(GLD_MODULE_NAME generators)

add_library(${GLD_MODULE_NAME}
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
        ibr_gfl.cpp
        ibr_gfl.h
		ibr_gfm_vsm.cpp
		ibr_gfm_vsm.h
		ibr_gfm_drp.cpp
		ibr_gfm_drp.h
		ibr_blackbox.cpp
		ibr_blackbox.h
		ibr_graybox.cpp
		ibr_graybox.h
	ibr_skeleton.cpp
	ibr_skeleton.h
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
	cblock.cpp
	cblock.h
        )

set_target_properties(${GLD_MODULE_NAME} PROPERTIES PREFIX "")


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
    target_link_libraries(${GLD_MODULE_NAME} PRIVATE
            #            superlu
            superlu_lib
            )
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
