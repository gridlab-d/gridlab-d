project(libsuperlu)

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_PTHREAD -DAdd_ -lm" )
SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_PTHREAD -DAdd_ -lm" )

add_library(${PROJECT_NAME} STATIC
        # from ../CBLAS
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
        # from ../superLU_MT

        ../superLU_MT/await.c
        ../superLU_MT/cgscon.c
        ../superLU_MT/cgsequ.c
        ../superLU_MT/cgsrfs.c
        ../superLU_MT/cgstrs.c
        ../superLU_MT/cholnzcnt.c
        ../superLU_MT/clacon.c
        ../superLU_MT/clangs.c
        ../superLU_MT/claqgs.c
        ../superLU_MT/cmatgen.c
        ../superLU_MT/cmyblas2.c
        ../superLU_MT/colamd.c
        ../superLU_MT/colamd.h
        ../superLU_MT/cpivotgrowth.c
        ../superLU_MT/creadhb.c
#        ../superLU_MT/creadmt.c
        ../superLU_MT/creadrb.c
        ../superLU_MT/csp_blas2.c
        ../superLU_MT/csp_blas3.c
        ../superLU_MT/dclock.c
        ../superLU_MT/dcomplex.c
        ../superLU_MT/dgscon.c
        ../superLU_MT/dgsequ.c
        ../superLU_MT/dgsrfs.c
        ../superLU_MT/dgstrs.c
        ../superLU_MT/dlacon.c
        ../superLU_MT/dlamch.c
        ../superLU_MT/dlangs.c
        ../superLU_MT/dlaqgs.c
        ../superLU_MT/dmatgen.c
        ../superLU_MT/dmyblas2.c
        ../superLU_MT/dpivotgrowth.c
        ../superLU_MT/dreadhb.c
#        ../superLU_MT/dreadmt.c
        ../superLU_MT/dreadrb.c
        ../superLU_MT/dsp_blas2.c
        ../superLU_MT/dsp_blas3.c
        ../superLU_MT/dzsum1.c
        ../superLU_MT/get_perm_c.c
        ../superLU_MT/heap_relax_snode.c
        ../superLU_MT/icmax1.c
        ../superLU_MT/izmax1.c
        ../superLU_MT/lsame.c
        ../superLU_MT/mmd.c
        ../superLU_MT/pcgssv.c
        ../superLU_MT/pcgssvx.c
        ../superLU_MT/pcgstrf_bmod1D.c
        ../superLU_MT/pcgstrf_bmod1D_mv2.c
        ../superLU_MT/pcgstrf_bmod2D.c
        ../superLU_MT/pcgstrf_bmod2D_mv2.c
        ../superLU_MT/pcgstrf.c
        ../superLU_MT/pcgstrf_column_bmod.c
        ../superLU_MT/pcgstrf_column_dfs.c
        ../superLU_MT/pcgstrf_copy_to_ucol.c
        ../superLU_MT/pcgstrf_factor_snode.c
        ../superLU_MT/pcgstrf_init.c
        ../superLU_MT/pcgstrf_panel_bmod.c
        ../superLU_MT/pcgstrf_panel_dfs.c
        ../superLU_MT/pcgstrf_pivotL.c
        ../superLU_MT/pcgstrf_snode_bmod.c
        ../superLU_MT/pcgstrf_snode_dfs.c
        ../superLU_MT/pcgstrf_thread.c
        ../superLU_MT/pcgstrf_thread_finalize.c
        ../superLU_MT/pcgstrf_thread_init.c
        ../superLU_MT/pcmemory.c
        ../superLU_MT/pcutil.c
        ../superLU_MT/pdgssv.c
        ../superLU_MT/pdgssvx.c
        ../superLU_MT/pdgstrf_bmod1D.c
        ../superLU_MT/pdgstrf_bmod1D_mv2.c
        ../superLU_MT/pdgstrf_bmod2D.c
        ../superLU_MT/pdgstrf_bmod2D_mv2.c
        ../superLU_MT/pdgstrf.c
        ../superLU_MT/pdgstrf_column_bmod.c
        ../superLU_MT/pdgstrf_column_dfs.c
        ../superLU_MT/pdgstrf_copy_to_ucol.c
        ../superLU_MT/pdgstrf_factor_snode.c
        ../superLU_MT/pdgstrf_init.c
        ../superLU_MT/pdgstrf_panel_bmod.c
        ../superLU_MT/pdgstrf_panel_dfs.c
        ../superLU_MT/pdgstrf_pivotL.c
        ../superLU_MT/pdgstrf_snode_bmod.c
        ../superLU_MT/pdgstrf_snode_dfs.c
        ../superLU_MT/pdgstrf_thread.c
        ../superLU_MT/pdgstrf_thread_finalize.c
        ../superLU_MT/pdgstrf_thread_init.c
        ../superLU_MT/pdmemory.c
        ../superLU_MT/pdutil.c
        ../superLU_MT/pmemory.c
        ../superLU_MT/psgssv.c
        ../superLU_MT/psgssvx.c
        ../superLU_MT/psgstrf_bmod1D.c
        ../superLU_MT/psgstrf_bmod1D_mv2.c
        ../superLU_MT/psgstrf_bmod2D.c
        ../superLU_MT/psgstrf_bmod2D_mv2.c
        ../superLU_MT/psgstrf.c
        ../superLU_MT/psgstrf_column_bmod.c
        ../superLU_MT/psgstrf_column_dfs.c
        ../superLU_MT/psgstrf_copy_to_ucol.c
        ../superLU_MT/psgstrf_factor_snode.c
        ../superLU_MT/psgstrf_init.c
        ../superLU_MT/psgstrf_panel_bmod.c
        ../superLU_MT/psgstrf_panel_dfs.c
        ../superLU_MT/psgstrf_pivotL.c
        ../superLU_MT/psgstrf_snode_bmod.c
        ../superLU_MT/psgstrf_snode_dfs.c
        ../superLU_MT/psgstrf_thread.c
        ../superLU_MT/psgstrf_thread_finalize.c
        ../superLU_MT/psgstrf_thread_init.c
        ../superLU_MT/psmemory.c
        ../superLU_MT/psutil.c
        ../superLU_MT/pxgstrf_finalize.c
        ../superLU_MT/pxgstrf_mark_busy_descends.c
        ../superLU_MT/pxgstrf_pruneL.c
        ../superLU_MT/pxgstrf_relax_snode.c
        ../superLU_MT/pxgstrf_scheduler.c
        ../superLU_MT/pxgstrf_super_bnd_dfs.c
        ../superLU_MT/pxgstrf_synch.c
        ../superLU_MT/pxgstrf_synch.h
        ../superLU_MT/pzgssv.c
        ../superLU_MT/pzgssvx.c
        ../superLU_MT/pzgstrf_bmod1D.c
        ../superLU_MT/pzgstrf_bmod1D_mv2.c
        ../superLU_MT/pzgstrf_bmod2D.c
        ../superLU_MT/pzgstrf_bmod2D_mv2.c
        ../superLU_MT/pzgstrf.c
        ../superLU_MT/pzgstrf_column_bmod.c
        ../superLU_MT/pzgstrf_column_dfs.c
        ../superLU_MT/pzgstrf_copy_to_ucol.c
        ../superLU_MT/pzgstrf_factor_snode.c
        ../superLU_MT/pzgstrf_init.c
        ../superLU_MT/pzgstrf_panel_bmod.c
        ../superLU_MT/pzgstrf_panel_dfs.c
        ../superLU_MT/pzgstrf_pivotL.c
        ../superLU_MT/pzgstrf_snode_bmod.c
        ../superLU_MT/pzgstrf_snode_dfs.c
        ../superLU_MT/pzgstrf_thread.c
        ../superLU_MT/pzgstrf_thread_finalize.c
        ../superLU_MT/pzgstrf_thread_init.c
        ../superLU_MT/pzmemory.c
        ../superLU_MT/pzutil.c
        ../superLU_MT/qrnzcnt.c
        ../superLU_MT/scomplex.c
        ../superLU_MT/scsum1.c
        ../superLU_MT/sgscon.c
        ../superLU_MT/sgsequ.c
        ../superLU_MT/sgsrfs.c
        ../superLU_MT/sgstrs.c
        ../superLU_MT/slacon.c
        ../superLU_MT/slamch.c
        ../superLU_MT/slangs.c
        ../superLU_MT/slaqgs.c
        ../superLU_MT/slu_dcomplex.h
        ../superLU_MT/slu_mt_cdefs.h
        ../superLU_MT/slu_mt_Cnames.h
        ../superLU_MT/slu_mt_ddefs.h
        ../superLU_MT/slu_mt_machines.h
        ../superLU_MT/slu_mt_sdefs.h
        ../superLU_MT/slu_mt_util.h
        ../superLU_MT/slu_mt_zdefs.h
        ../superLU_MT/slu_scomplex.h
        ../superLU_MT/smatgen.c
        ../superLU_MT/smyblas2.c
        ../superLU_MT/sp_coletree.c
        ../superLU_MT/sp_colorder.c
        ../superLU_MT/sp_ienv.c
        ../superLU_MT/spivotgrowth.c
        ../superLU_MT/sreadhb.c
#        ../superLU_MT/sreadmt.c
        ../superLU_MT/sreadrb.c
        ../superLU_MT/ssp_blas2.c
        ../superLU_MT/ssp_blas3.c
        ../superLU_MT/superlu_timer.c
        ../superLU_MT/supermatrix.h
        ../superLU_MT/util.c
        ../superLU_MT/xerbla.c
        ../superLU_MT/zgscon.c
        ../superLU_MT/zgsequ.c
        ../superLU_MT/zgsrfs.c
        ../superLU_MT/zgstrs.c
        ../superLU_MT/zlacon.c
        ../superLU_MT/zlangs.c
        ../superLU_MT/zlaqgs.c
        ../superLU_MT/zmatgen.c
        ../superLU_MT/zmyblas2.c
        ../superLU_MT/zpivotgrowth.c
        ../superLU_MT/zreadhb.c
#        ../superLU_MT/zreadmt.c
        ../superLU_MT/zreadrb.c
        ../superLU_MT/zsp_blas2.c
        ../superLU_MT/zsp_blas3.c
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
        )

if ( CMAKE_COMPILER_IS_GNUCC )
    target_compile_options(${PROJECT_NAME} PRIVATE "-w")
endif()
if ( MSVC )
    target_compile_options(${PROJECT_NAME} PRIVATE "/w")
endif()

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(${PROJECT_NAME} PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(${PROJECT_NAME} "${CMAKE_THREAD_LIBS_INIT}")
endif()

install(TARGETS ${PROJECT_NAME}
        PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        )

