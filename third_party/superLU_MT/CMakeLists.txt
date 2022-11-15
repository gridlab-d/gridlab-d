project(superlu_lib)

if (APPLE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__PTHREAD -DAdd_")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__PTHREAD -DAdd_ -Wno-implicit-function-declaration")
elseif (NOT MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__PTHREAD -DAdd_ -lm")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__PTHREAD -DAdd_ -lm")
else ()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__OPENMP -DAdd_")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__OPENMP -DAdd_")
endif ()

add_library(CBLAS OBJECT
        # from ${THIRD_PARTY_PATH}/CBLAS
        ../CBLAS/dasum.c
        ../CBLAS/daxpy.c
        ../CBLAS/dcopy.c
        ../CBLAS/ddot.c
        ../CBLAS/dgemv.c
        ../CBLAS/dger.c
        ../CBLAS/dnrm2.c
        ../CBLAS/drot.c
        ../CBLAS/dscal.c
        ../CBLAS/dsymv.c
        ../CBLAS/dsyr2.c
        ../CBLAS/dtrsv.c
        ../CBLAS/f2c.h
        ../CBLAS/idamax.c
        ../CBLAS/slu_Cnames.h
        ../CBLAS/superlu_f2c.h
        )

add_library(${PROJECT_NAME} STATIC
        $<TARGET_OBJECTS:CBLAS>

        # from ${THIRD_PARTY_PATH}/superLU_MT
        await.c
        cgscon.c
        cgsequ.c
        cgsrfs.c
        cgstrs.c
        cholnzcnt.c
        clacon.c
        clangs.c
        claqgs.c
        cmatgen.c
        cmyblas2.c
        colamd.c
        colamd.h
        cpivotgrowth.c
        creadhb.c
        #        creadmt.c
        creadrb.c
        csp_blas2.c
        csp_blas3.c
        dclock.c
        dcomplex.c
        dgscon.c
        dgsequ.c
        dgsrfs.c
        dgstrs.c
        dlacon.c
        dlamch.c
        dlangs.c
        dlaqgs.c
        dmatgen.c
        dmyblas2.c
        dpivotgrowth.c
        dreadhb.c
        #        dreadmt.c
        dreadrb.c
        dsp_blas2.c
        dsp_blas3.c
        dzsum1.c
        get_perm_c.c
        heap_relax_snode.c
        icmax1.c
        izmax1.c
        lsame.c
        mmd.c
        pcgssv.c
        pcgssvx.c
        pcgstrf_bmod1D.c
        pcgstrf_bmod1D_mv2.c
        pcgstrf_bmod2D.c
        pcgstrf_bmod2D_mv2.c
        pcgstrf.c
        pcgstrf_column_bmod.c
        pcgstrf_column_dfs.c
        pcgstrf_copy_to_ucol.c
        pcgstrf_factor_snode.c
        pcgstrf_init.c
        pcgstrf_panel_bmod.c
        pcgstrf_panel_dfs.c
        pcgstrf_pivotL.c
        pcgstrf_snode_bmod.c
        pcgstrf_snode_dfs.c
        pcgstrf_thread.c
        pcgstrf_thread_finalize.c
        pcgstrf_thread_init.c
        pcmemory.c
        pcutil.c
        pdgssv.c
        pdgssvx.c
        pdgstrf_bmod1D.c
        pdgstrf_bmod1D_mv2.c
        pdgstrf_bmod2D.c
        pdgstrf_bmod2D_mv2.c
        pdgstrf.c
        pdgstrf_column_bmod.c
        pdgstrf_column_dfs.c
        pdgstrf_copy_to_ucol.c
        pdgstrf_factor_snode.c
        pdgstrf_init.c
        pdgstrf_panel_bmod.c
        pdgstrf_panel_dfs.c
        pdgstrf_pivotL.c
        pdgstrf_snode_bmod.c
        pdgstrf_snode_dfs.c
        pdgstrf_thread.c
        pdgstrf_thread_finalize.c
        pdgstrf_thread_init.c
        pdmemory.c
        pdutil.c
        pmemory.c
        psgssv.c
        psgssvx.c
        psgstrf_bmod1D.c
        psgstrf_bmod1D_mv2.c
        psgstrf_bmod2D.c
        psgstrf_bmod2D_mv2.c
        psgstrf.c
        psgstrf_column_bmod.c
        psgstrf_column_dfs.c
        psgstrf_copy_to_ucol.c
        psgstrf_factor_snode.c
        psgstrf_init.c
        psgstrf_panel_bmod.c
        psgstrf_panel_dfs.c
        psgstrf_pivotL.c
        psgstrf_snode_bmod.c
        psgstrf_snode_dfs.c
        psgstrf_thread.c
        psgstrf_thread_finalize.c
        psgstrf_thread_init.c
        psmemory.c
        psutil.c
        pxgstrf_finalize.c
        pxgstrf_mark_busy_descends.c
        pxgstrf_pruneL.c
        pxgstrf_relax_snode.c
        pxgstrf_scheduler.c
        pxgstrf_super_bnd_dfs.c
        pxgstrf_synch.c
        pxgstrf_synch.h
        pzgssv.c
        pzgssvx.c
        pzgstrf_bmod1D.c
        pzgstrf_bmod1D_mv2.c
        pzgstrf_bmod2D.c
        pzgstrf_bmod2D_mv2.c
        pzgstrf.c
        pzgstrf_column_bmod.c
        pzgstrf_column_dfs.c
        pzgstrf_copy_to_ucol.c
        pzgstrf_factor_snode.c
        pzgstrf_init.c
        pzgstrf_panel_bmod.c
        pzgstrf_panel_dfs.c
        pzgstrf_pivotL.c
        pzgstrf_snode_bmod.c
        pzgstrf_snode_dfs.c
        pzgstrf_thread.c
        pzgstrf_thread_finalize.c
        pzgstrf_thread_init.c
        pzmemory.c
        pzutil.c
        qrnzcnt.c
        scomplex.c
        scsum1.c
        sgscon.c
        sgsequ.c
        sgsrfs.c
        sgstrs.c
        slacon.c
        slamch.c
        slangs.c
        slaqgs.c
        slu_dcomplex.h
        slu_mt_cdefs.h
        slu_mt_Cnames.h
        slu_mt_ddefs.h
        slu_mt_machines.h
        slu_mt_sdefs.h
        slu_mt_util.h
        slu_mt_zdefs.h
        slu_scomplex.h
        smatgen.c
        smyblas2.c
        sp_coletree.c
        sp_colorder.c
        sp_ienv.c
        spivotgrowth.c
        sreadhb.c
        #        sreadmt.c
        sreadrb.c
        ssp_blas2.c
        ssp_blas3.c
        superlu_timer.c
        supermatrix.h
        util.c
        xerbla.c
        zgscon.c
        zgsequ.c
        zgsrfs.c
        zgstrs.c
        zlacon.c
        zlangs.c
        zlaqgs.c
        zmatgen.c
        zmyblas2.c
        zpivotgrowth.c
        zreadhb.c
        #        zreadmt.c
        zreadrb.c
        zsp_blas2.c
        zsp_blas3.c
        )

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /w)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -w)
endif ()

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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Libraries)

