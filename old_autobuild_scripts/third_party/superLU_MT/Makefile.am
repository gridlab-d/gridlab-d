CC = $(PTHREAD_CC)
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CPPFLAGS = -D_PTHREAD -DAdd_
noinst_LTLIBRARIES = libsuperlu.la
libsuperlu_la_SOURCES =                                         \
	dreadhb.c dmatgen.c pdgssv.c pdgssvx.c dgstrs.c dgsrfs.c    \
	dgscon.c dlacon.c dlangs.c dgsequ.c dlaqgs.c dpivotgrowth.c \
	pdmemory.c pdutil.c dmyblas2.c pdgstrf.c pdgstrf_init.c     \
	pdgstrf_thread.c pdgstrf_thread_init.c                      \
	pdgstrf_thread_finalize.c pdgstrf_factor_snode.c            \
	pdgstrf_snode_dfs.c pdgstrf_snode_bmod.c                    \
	pdgstrf_panel_dfs.c pdgstrf_panel_bmod.c                    \
	pdgstrf_copy_to_ucol.c pdgstrf_pivotL.c                     \
	pdgstrf_column_dfs.c pdgstrf_column_bmod.c pdgstrf_bmod1D.c \
	pdgstrf_bmod2D.c pdgstrf_bmod1D_mv2.c                       \
	pdgstrf_bmod2D_mv2.c dsp_blas2.c dsp_blas3.c dclock.c       \
	sp_ienv.c lsame.c xerbla.c util.c pmemory.c qrnzcnt.c       \
	get_perm_c.c mmd.c colamd.c sp_coletree.c                   \
	pxgstrf_scheduler.c sp_colorder.c                           \
	pxgstrf_mark_busy_descends.c pxgstrf_pruneL.c               \
	pxgstrf_super_bnd_dfs.c pxgstrf_relax_snode.c               \
	heap_relax_snode.c pxgstrf_synch.c pxgstrf_finalize.c       \
	await.c colamd.h dlamch.c pdsp_defs.h pxgstrf_synch.h       \
	slu_mt_Cnames.h slu_mt_machines.h slu_mt_util.h             \
	superlu_timer.c supermatrix.h

libsuperlu_la_LDFLAGS = -lm -lpthread
libsuperlu_la_LIBADD = ../CBLAS/libblas.la

