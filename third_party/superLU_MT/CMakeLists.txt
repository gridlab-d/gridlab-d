project(libsuperlu)

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_PTHREAD -DAdd_ -lm" )
SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_PTHREAD -DAdd_ -lm" )

add_library(${PROJECT_NAME} STATIC
        # from third_party/CBLAS
        third_party/CBLAS/dasum.c
        third_party/CBLAS/daxpy.c
        third_party/CBLAS/dcopy.c
        third_party/CBLAS/ddot.c
        third_party/CBLAS/dgemv.c
        third_party/CBLAS/dger.c
        third_party/CBLAS/dnrm2.c
        third_party/CBLAS/drot.c
        third_party/CBLAS/dscal.c
        third_party/CBLAS/dsymv.c
        third_party/CBLAS/dsyr2.c
        third_party/CBLAS/dtrsv.c
        third_party/CBLAS/f2c.h
        third_party/CBLAS/idamax.c
        third_party/CBLAS/slu_Cnames.h
        third_party/CBLAS/superlu_f2c.h
        # from third_party/superLU_MT

        third_party/superLU_MT/await.c
        third_party/superLU_MT/cgscon.c
        third_party/superLU_MT/cgsequ.c
        third_party/superLU_MT/cgsrfs.c
        third_party/superLU_MT/cgstrs.c
        third_party/superLU_MT/cholnzcnt.c
        third_party/superLU_MT/clacon.c
        third_party/superLU_MT/clangs.c
        third_party/superLU_MT/claqgs.c
        third_party/superLU_MT/cmatgen.c
        third_party/superLU_MT/cmyblas2.c
        third_party/superLU_MT/colamd.c
        third_party/superLU_MT/colamd.h
        third_party/superLU_MT/cpivotgrowth.c
        third_party/superLU_MT/creadhb.c
#        third_party/superLU_MT/creadmt.c
        third_party/superLU_MT/creadrb.c
        third_party/superLU_MT/csp_blas2.c
        third_party/superLU_MT/csp_blas3.c
        third_party/superLU_MT/dclock.c
        third_party/superLU_MT/dcomplex.c
        third_party/superLU_MT/dgscon.c
        third_party/superLU_MT/dgsequ.c
        third_party/superLU_MT/dgsrfs.c
        third_party/superLU_MT/dgstrs.c
        third_party/superLU_MT/dlacon.c
        third_party/superLU_MT/dlamch.c
        third_party/superLU_MT/dlangs.c
        third_party/superLU_MT/dlaqgs.c
        third_party/superLU_MT/dmatgen.c
        third_party/superLU_MT/dmyblas2.c
        third_party/superLU_MT/dpivotgrowth.c
        third_party/superLU_MT/dreadhb.c
#        third_party/superLU_MT/dreadmt.c
        third_party/superLU_MT/dreadrb.c
        third_party/superLU_MT/dsp_blas2.c
        third_party/superLU_MT/dsp_blas3.c
        third_party/superLU_MT/dzsum1.c
        third_party/superLU_MT/get_perm_c.c
        third_party/superLU_MT/heap_relax_snode.c
        third_party/superLU_MT/icmax1.c
        third_party/superLU_MT/izmax1.c
        third_party/superLU_MT/lsame.c
        third_party/superLU_MT/mmd.c
        third_party/superLU_MT/pcgssv.c
        third_party/superLU_MT/pcgssvx.c
        third_party/superLU_MT/pcgstrf_bmod1D.c
        third_party/superLU_MT/pcgstrf_bmod1D_mv2.c
        third_party/superLU_MT/pcgstrf_bmod2D.c
        third_party/superLU_MT/pcgstrf_bmod2D_mv2.c
        third_party/superLU_MT/pcgstrf.c
        third_party/superLU_MT/pcgstrf_column_bmod.c
        third_party/superLU_MT/pcgstrf_column_dfs.c
        third_party/superLU_MT/pcgstrf_copy_to_ucol.c
        third_party/superLU_MT/pcgstrf_factor_snode.c
        third_party/superLU_MT/pcgstrf_init.c
        third_party/superLU_MT/pcgstrf_panel_bmod.c
        third_party/superLU_MT/pcgstrf_panel_dfs.c
        third_party/superLU_MT/pcgstrf_pivotL.c
        third_party/superLU_MT/pcgstrf_snode_bmod.c
        third_party/superLU_MT/pcgstrf_snode_dfs.c
        third_party/superLU_MT/pcgstrf_thread.c
        third_party/superLU_MT/pcgstrf_thread_finalize.c
        third_party/superLU_MT/pcgstrf_thread_init.c
        third_party/superLU_MT/pcmemory.c
        third_party/superLU_MT/pcutil.c
        third_party/superLU_MT/pdgssv.c
        third_party/superLU_MT/pdgssvx.c
        third_party/superLU_MT/pdgstrf_bmod1D.c
        third_party/superLU_MT/pdgstrf_bmod1D_mv2.c
        third_party/superLU_MT/pdgstrf_bmod2D.c
        third_party/superLU_MT/pdgstrf_bmod2D_mv2.c
        third_party/superLU_MT/pdgstrf.c
        third_party/superLU_MT/pdgstrf_column_bmod.c
        third_party/superLU_MT/pdgstrf_column_dfs.c
        third_party/superLU_MT/pdgstrf_copy_to_ucol.c
        third_party/superLU_MT/pdgstrf_factor_snode.c
        third_party/superLU_MT/pdgstrf_init.c
        third_party/superLU_MT/pdgstrf_panel_bmod.c
        third_party/superLU_MT/pdgstrf_panel_dfs.c
        third_party/superLU_MT/pdgstrf_pivotL.c
        third_party/superLU_MT/pdgstrf_snode_bmod.c
        third_party/superLU_MT/pdgstrf_snode_dfs.c
        third_party/superLU_MT/pdgstrf_thread.c
        third_party/superLU_MT/pdgstrf_thread_finalize.c
        third_party/superLU_MT/pdgstrf_thread_init.c
        third_party/superLU_MT/pdmemory.c
        third_party/superLU_MT/pdutil.c
        third_party/superLU_MT/pmemory.c
        third_party/superLU_MT/psgssv.c
        third_party/superLU_MT/psgssvx.c
        third_party/superLU_MT/psgstrf_bmod1D.c
        third_party/superLU_MT/psgstrf_bmod1D_mv2.c
        third_party/superLU_MT/psgstrf_bmod2D.c
        third_party/superLU_MT/psgstrf_bmod2D_mv2.c
        third_party/superLU_MT/psgstrf.c
        third_party/superLU_MT/psgstrf_column_bmod.c
        third_party/superLU_MT/psgstrf_column_dfs.c
        third_party/superLU_MT/psgstrf_copy_to_ucol.c
        third_party/superLU_MT/psgstrf_factor_snode.c
        third_party/superLU_MT/psgstrf_init.c
        third_party/superLU_MT/psgstrf_panel_bmod.c
        third_party/superLU_MT/psgstrf_panel_dfs.c
        third_party/superLU_MT/psgstrf_pivotL.c
        third_party/superLU_MT/psgstrf_snode_bmod.c
        third_party/superLU_MT/psgstrf_snode_dfs.c
        third_party/superLU_MT/psgstrf_thread.c
        third_party/superLU_MT/psgstrf_thread_finalize.c
        third_party/superLU_MT/psgstrf_thread_init.c
        third_party/superLU_MT/psmemory.c
        third_party/superLU_MT/psutil.c
        third_party/superLU_MT/pxgstrf_finalize.c
        third_party/superLU_MT/pxgstrf_mark_busy_descends.c
        third_party/superLU_MT/pxgstrf_pruneL.c
        third_party/superLU_MT/pxgstrf_relax_snode.c
        third_party/superLU_MT/pxgstrf_scheduler.c
        third_party/superLU_MT/pxgstrf_super_bnd_dfs.c
        third_party/superLU_MT/pxgstrf_synch.c
        third_party/superLU_MT/pxgstrf_synch.h
        third_party/superLU_MT/pzgssv.c
        third_party/superLU_MT/pzgssvx.c
        third_party/superLU_MT/pzgstrf_bmod1D.c
        third_party/superLU_MT/pzgstrf_bmod1D_mv2.c
        third_party/superLU_MT/pzgstrf_bmod2D.c
        third_party/superLU_MT/pzgstrf_bmod2D_mv2.c
        third_party/superLU_MT/pzgstrf.c
        third_party/superLU_MT/pzgstrf_column_bmod.c
        third_party/superLU_MT/pzgstrf_column_dfs.c
        third_party/superLU_MT/pzgstrf_copy_to_ucol.c
        third_party/superLU_MT/pzgstrf_factor_snode.c
        third_party/superLU_MT/pzgstrf_init.c
        third_party/superLU_MT/pzgstrf_panel_bmod.c
        third_party/superLU_MT/pzgstrf_panel_dfs.c
        third_party/superLU_MT/pzgstrf_pivotL.c
        third_party/superLU_MT/pzgstrf_snode_bmod.c
        third_party/superLU_MT/pzgstrf_snode_dfs.c
        third_party/superLU_MT/pzgstrf_thread.c
        third_party/superLU_MT/pzgstrf_thread_finalize.c
        third_party/superLU_MT/pzgstrf_thread_init.c
        third_party/superLU_MT/pzmemory.c
        third_party/superLU_MT/pzutil.c
        third_party/superLU_MT/qrnzcnt.c
        third_party/superLU_MT/scomplex.c
        third_party/superLU_MT/scsum1.c
        third_party/superLU_MT/sgscon.c
        third_party/superLU_MT/sgsequ.c
        third_party/superLU_MT/sgsrfs.c
        third_party/superLU_MT/sgstrs.c
        third_party/superLU_MT/slacon.c
        third_party/superLU_MT/slamch.c
        third_party/superLU_MT/slangs.c
        third_party/superLU_MT/slaqgs.c
        third_party/superLU_MT/slu_dcomplex.h
        third_party/superLU_MT/slu_mt_cdefs.h
        third_party/superLU_MT/slu_mt_Cnames.h
        third_party/superLU_MT/slu_mt_ddefs.h
        third_party/superLU_MT/slu_mt_machines.h
        third_party/superLU_MT/slu_mt_sdefs.h
        third_party/superLU_MT/slu_mt_util.h
        third_party/superLU_MT/slu_mt_zdefs.h
        third_party/superLU_MT/slu_scomplex.h
        third_party/superLU_MT/smatgen.c
        third_party/superLU_MT/smyblas2.c
        third_party/superLU_MT/sp_coletree.c
        third_party/superLU_MT/sp_colorder.c
        third_party/superLU_MT/sp_ienv.c
        third_party/superLU_MT/spivotgrowth.c
        third_party/superLU_MT/sreadhb.c
#        third_party/superLU_MT/sreadmt.c
        third_party/superLU_MT/sreadrb.c
        third_party/superLU_MT/ssp_blas2.c
        third_party/superLU_MT/ssp_blas3.c
        third_party/superLU_MT/superlu_timer.c
        third_party/superLU_MT/supermatrix.h
        third_party/superLU_MT/util.c
        third_party/superLU_MT/xerbla.c
        third_party/superLU_MT/zgscon.c
        third_party/superLU_MT/zgsequ.c
        third_party/superLU_MT/zgsrfs.c
        third_party/superLU_MT/zgstrs.c
        third_party/superLU_MT/zlacon.c
        third_party/superLU_MT/zlangs.c
        third_party/superLU_MT/zlaqgs.c
        third_party/superLU_MT/zmatgen.c
        third_party/superLU_MT/zmyblas2.c
        third_party/superLU_MT/zpivotgrowth.c
        third_party/superLU_MT/zreadhb.c
#        third_party/superLU_MT/zreadmt.c
        third_party/superLU_MT/zreadrb.c
        third_party/superLU_MT/zsp_blas2.c
        third_party/superLU_MT/zsp_blas3.c
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
        )

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

