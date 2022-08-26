project(superlu_lib)

if (APPLE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_PTHREAD -DAdd_")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_PTHREAD -DAdd_ -Wno-implicit-function-declaration")
else ()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_PTHREAD -DAdd_ -lm")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_PTHREAD -DAdd_ -lm")
endif ()

add_library(CBLAS OBJECT
        # from ${THIRD_PARTY_PATH}/CBLAS
        ${THIRD_PARTY_PATH}/CBLAS/dasum.c
        ${THIRD_PARTY_PATH}/CBLAS/daxpy.c
        ${THIRD_PARTY_PATH}/CBLAS/dcopy.c
        ${THIRD_PARTY_PATH}/CBLAS/ddot.c
        ${THIRD_PARTY_PATH}/CBLAS/dgemv.c
        ${THIRD_PARTY_PATH}/CBLAS/dger.c
        ${THIRD_PARTY_PATH}/CBLAS/dnrm2.c
        ${THIRD_PARTY_PATH}/CBLAS/drot.c
        ${THIRD_PARTY_PATH}/CBLAS/dscal.c
        ${THIRD_PARTY_PATH}/CBLAS/dsymv.c
        ${THIRD_PARTY_PATH}/CBLAS/dsyr2.c
        ${THIRD_PARTY_PATH}/CBLAS/dtrsv.c
        ${THIRD_PARTY_PATH}/CBLAS/f2c.h
        ${THIRD_PARTY_PATH}/CBLAS/idamax.c
        ${THIRD_PARTY_PATH}/CBLAS/slu_Cnames.h
        ${THIRD_PARTY_PATH}/CBLAS/superlu_f2c.h
        )

add_library(${PROJECT_NAME} STATIC
        $<TARGET_OBJECTS:CBLAS>

        # from ${THIRD_PARTY_PATH}/superLU_MT
        ${THIRD_PARTY_PATH}/superLU_MT/await.c
        ${THIRD_PARTY_PATH}/superLU_MT/cgscon.c
        ${THIRD_PARTY_PATH}/superLU_MT/cgsequ.c
        ${THIRD_PARTY_PATH}/superLU_MT/cgsrfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/cgstrs.c
        ${THIRD_PARTY_PATH}/superLU_MT/cholnzcnt.c
        ${THIRD_PARTY_PATH}/superLU_MT/clacon.c
        ${THIRD_PARTY_PATH}/superLU_MT/clangs.c
        ${THIRD_PARTY_PATH}/superLU_MT/claqgs.c
        ${THIRD_PARTY_PATH}/superLU_MT/cmatgen.c
        ${THIRD_PARTY_PATH}/superLU_MT/cmyblas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/colamd.c
        ${THIRD_PARTY_PATH}/superLU_MT/colamd.h
        ${THIRD_PARTY_PATH}/superLU_MT/cpivotgrowth.c
        ${THIRD_PARTY_PATH}/superLU_MT/creadhb.c
        #        ${THIRD_PARTY_PATH}/superLU_MT/creadmt.c
        ${THIRD_PARTY_PATH}/superLU_MT/creadrb.c
        ${THIRD_PARTY_PATH}/superLU_MT/csp_blas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/csp_blas3.c
        ${THIRD_PARTY_PATH}/superLU_MT/dclock.c
        ${THIRD_PARTY_PATH}/superLU_MT/dcomplex.c
        ${THIRD_PARTY_PATH}/superLU_MT/dgscon.c
        ${THIRD_PARTY_PATH}/superLU_MT/dgsequ.c
        ${THIRD_PARTY_PATH}/superLU_MT/dgsrfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/dgstrs.c
        ${THIRD_PARTY_PATH}/superLU_MT/dlacon.c
        ${THIRD_PARTY_PATH}/superLU_MT/dlamch.c
        ${THIRD_PARTY_PATH}/superLU_MT/dlangs.c
        ${THIRD_PARTY_PATH}/superLU_MT/dlaqgs.c
        ${THIRD_PARTY_PATH}/superLU_MT/dmatgen.c
        ${THIRD_PARTY_PATH}/superLU_MT/dmyblas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/dpivotgrowth.c
        ${THIRD_PARTY_PATH}/superLU_MT/dreadhb.c
        #        ${THIRD_PARTY_PATH}/superLU_MT/dreadmt.c
        ${THIRD_PARTY_PATH}/superLU_MT/dreadrb.c
        ${THIRD_PARTY_PATH}/superLU_MT/dsp_blas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/dsp_blas3.c
        ${THIRD_PARTY_PATH}/superLU_MT/dzsum1.c
        ${THIRD_PARTY_PATH}/superLU_MT/get_perm_c.c
        ${THIRD_PARTY_PATH}/superLU_MT/heap_relax_snode.c
        ${THIRD_PARTY_PATH}/superLU_MT/icmax1.c
        ${THIRD_PARTY_PATH}/superLU_MT/izmax1.c
        ${THIRD_PARTY_PATH}/superLU_MT/lsame.c
        ${THIRD_PARTY_PATH}/superLU_MT/mmd.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgssv.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgssvx.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_bmod1D.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_bmod1D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_bmod2D.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_bmod2D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_column_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_column_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_copy_to_ucol.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_factor_snode.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_panel_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_panel_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_pivotL.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_snode_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_snode_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_thread.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_thread_finalize.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcgstrf_thread_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcmemory.c
        ${THIRD_PARTY_PATH}/superLU_MT/pcutil.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgssv.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgssvx.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_bmod1D.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_bmod1D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_bmod2D.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_bmod2D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_column_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_column_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_copy_to_ucol.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_factor_snode.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_panel_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_panel_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_pivotL.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_snode_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_snode_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_thread.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_thread_finalize.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdgstrf_thread_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdmemory.c
        ${THIRD_PARTY_PATH}/superLU_MT/pdutil.c
        ${THIRD_PARTY_PATH}/superLU_MT/pmemory.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgssv.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgssvx.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_bmod1D.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_bmod1D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_bmod2D.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_bmod2D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_column_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_column_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_copy_to_ucol.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_factor_snode.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_panel_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_panel_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_pivotL.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_snode_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_snode_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_thread.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_thread_finalize.c
        ${THIRD_PARTY_PATH}/superLU_MT/psgstrf_thread_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/psmemory.c
        ${THIRD_PARTY_PATH}/superLU_MT/psutil.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_finalize.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_mark_busy_descends.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_pruneL.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_relax_snode.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_scheduler.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_super_bnd_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_synch.c
        ${THIRD_PARTY_PATH}/superLU_MT/pxgstrf_synch.h
        ${THIRD_PARTY_PATH}/superLU_MT/pzgssv.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgssvx.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_bmod1D.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_bmod1D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_bmod2D.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_bmod2D_mv2.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_column_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_column_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_copy_to_ucol.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_factor_snode.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_panel_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_panel_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_pivotL.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_snode_bmod.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_snode_dfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_thread.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_thread_finalize.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzgstrf_thread_init.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzmemory.c
        ${THIRD_PARTY_PATH}/superLU_MT/pzutil.c
        ${THIRD_PARTY_PATH}/superLU_MT/qrnzcnt.c
        ${THIRD_PARTY_PATH}/superLU_MT/scomplex.c
        ${THIRD_PARTY_PATH}/superLU_MT/scsum1.c
        ${THIRD_PARTY_PATH}/superLU_MT/sgscon.c
        ${THIRD_PARTY_PATH}/superLU_MT/sgsequ.c
        ${THIRD_PARTY_PATH}/superLU_MT/sgsrfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/sgstrs.c
        ${THIRD_PARTY_PATH}/superLU_MT/slacon.c
        ${THIRD_PARTY_PATH}/superLU_MT/slamch.c
        ${THIRD_PARTY_PATH}/superLU_MT/slangs.c
        ${THIRD_PARTY_PATH}/superLU_MT/slaqgs.c
        ${THIRD_PARTY_PATH}/superLU_MT/slu_dcomplex.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_mt_cdefs.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_mt_Cnames.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_mt_ddefs.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_mt_machines.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_mt_sdefs.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_mt_util.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_mt_zdefs.h
        ${THIRD_PARTY_PATH}/superLU_MT/slu_scomplex.h
        ${THIRD_PARTY_PATH}/superLU_MT/smatgen.c
        ${THIRD_PARTY_PATH}/superLU_MT/smyblas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/sp_coletree.c
        ${THIRD_PARTY_PATH}/superLU_MT/sp_colorder.c
        ${THIRD_PARTY_PATH}/superLU_MT/sp_ienv.c
        ${THIRD_PARTY_PATH}/superLU_MT/spivotgrowth.c
        ${THIRD_PARTY_PATH}/superLU_MT/sreadhb.c
        #        ${THIRD_PARTY_PATH}/superLU_MT/sreadmt.c
        ${THIRD_PARTY_PATH}/superLU_MT/sreadrb.c
        ${THIRD_PARTY_PATH}/superLU_MT/ssp_blas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/ssp_blas3.c
        ${THIRD_PARTY_PATH}/superLU_MT/superlu_timer.c
        ${THIRD_PARTY_PATH}/superLU_MT/supermatrix.h
        ${THIRD_PARTY_PATH}/superLU_MT/util.c
        ${THIRD_PARTY_PATH}/superLU_MT/xerbla.c
        ${THIRD_PARTY_PATH}/superLU_MT/zgscon.c
        ${THIRD_PARTY_PATH}/superLU_MT/zgsequ.c
        ${THIRD_PARTY_PATH}/superLU_MT/zgsrfs.c
        ${THIRD_PARTY_PATH}/superLU_MT/zgstrs.c
        ${THIRD_PARTY_PATH}/superLU_MT/zlacon.c
        ${THIRD_PARTY_PATH}/superLU_MT/zlangs.c
        ${THIRD_PARTY_PATH}/superLU_MT/zlaqgs.c
        ${THIRD_PARTY_PATH}/superLU_MT/zmatgen.c
        ${THIRD_PARTY_PATH}/superLU_MT/zmyblas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/zpivotgrowth.c
        ${THIRD_PARTY_PATH}/superLU_MT/zreadhb.c
        #        ${THIRD_PARTY_PATH}/superLU_MT/zreadmt.c
        ${THIRD_PARTY_PATH}/superLU_MT/zreadrb.c
        ${THIRD_PARTY_PATH}/superLU_MT/zsp_blas2.c
        ${THIRD_PARTY_PATH}/superLU_MT/zsp_blas3.c
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
        )

if (CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(${PROJECT_NAME} PRIVATE "-w")
endif ()
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/w")
endif ()

find_package(Threads REQUIRED)
if (THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(${PROJECT_NAME} PUBLIC "-pthread")
endif ()
if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(${PROJECT_NAME} "${CMAKE_THREAD_LIBS_INIT}")
endif ()

install(TARGETS ${PROJECT_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Libraries)

