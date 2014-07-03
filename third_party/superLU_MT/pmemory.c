/*
 * -- SuperLU MT routine (version 1.0) --
 * Univ. of California Berkeley, Xerox Palo Alto Research Center,
 * and Lawrence Berkeley National Lab.
 * August 15, 1997
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include "pdsp_defs.h"

#define XPAND_HINT(memtype, new_next, jcol, param) {\
fprintf(stderr, "Storage for %12s exceeded; Current column %d; Need at least %d;\n",\
	memtype, jcol, new_next); \
fprintf(stderr, "You may set it by the %d-th parameter in routine sp_ienv().\n", param); \
SUPERLU_ABORT("Memory allocation failed"); \
}

/*
 * Set up pointers for integer working arrays.
 */
void
pxgstrf_SetIWork(int n, int panel_size, int *iworkptr, int **segrep,
		 int **parent, int **xplore, int **repfnz, int **panel_lsub,
		 int **marker, int **lbusy)
{
    *segrep = iworkptr;                                      /* n  */
    *parent = iworkptr + n;                                  /* n  */
    *xplore = iworkptr + 2*n;                                /* 2*n */
    *repfnz = iworkptr + 4*n;                                /* w*n */
    *panel_lsub = iworkptr + 4*n + panel_size*n;             /* w*n */
    *marker = iworkptr + 4*n + 2*panel_size*n;               /* 3*n */
    *lbusy  = iworkptr + (4+NO_MARKER)*n + 2*panel_size*n;   /* n   */
    ifill (*repfnz, n * panel_size, EMPTY);
}


void
copy_mem_int(int howmany, void *old, void *new)
{
    register int i;
    int *iold = old;
    int *inew = new;
    for (i = 0; i < howmany; i++) inew[i] = iold[i];
}


void
user_bcopy(char *src, char *dest, int bytes)
{
    char *s_ptr, *d_ptr;

    s_ptr = src + bytes - 1;
    d_ptr = dest + bytes - 1;
    for (; d_ptr >= dest; --s_ptr, --d_ptr ) *d_ptr = *s_ptr;
}



int *intMalloc(int n)
{
    int *buf;
    buf = (int *) SUPERLU_MALLOC( (size_t) n * sizeof(int));
    if ( !buf ) {
	fprintf(stderr, "SUPERLU_MALLOC failed for buf in intMalloc()\n");
	exit (1);
    }
    return (buf);
}

int *intCalloc(int n)
{
    int *buf;
    register int i;
    buf = (int *) SUPERLU_MALLOC( (size_t) n * sizeof(int));
    if ( !buf ) {
	fprintf(stderr, "SUPERLU_MALLOC failed for buf in intCalloc()\n");
	exit (1);
    }
    for (i = 0; i < n; ++i) buf[i] = 0;
    return (buf);
}

/*
 * Allocate n elements storage from a global array.
 * It uses lock for mutually exclusive access to the next position, so that
 * more than one processors can call aalloc on the same array correctly.
 * Return value: 0 - success
 *              >0 - number of bytes allocated when run out of space
 */
int
Glu_alloc(
	  const int pnum,     /* process number */
	  const int jcol,
	  const int num,      /* number of elements requested */
	  const MemType mem_type,
          int   *prev_next,   /* return "next" value before allocation */
	  pxgstrf_shared_t *pxgstrf_shared
	  )
{
    GlobalLU_t *Glu = pxgstrf_shared->Glu;
    Gstat_t    *Gstat = pxgstrf_shared->Gstat;
    register int fsupc, nextl, nextu, new_next;
#ifdef PROFILE
    double   t;
#endif
    
    switch ( mem_type ) {
	
      case LUSUP: 
	/* Storage for the H-supernode is already set aside, so we do
	   not use lock here. */
	if ( Glu->map_in_sup[jcol] < 0 )
	    fsupc = jcol + Glu->map_in_sup[jcol];
	else fsupc = jcol;
	*prev_next = Glu->map_in_sup[fsupc];
	Glu->map_in_sup[fsupc] += num;

#if 0
	{
	    register int i, j;
	    i = fsupc + part_super_h[fsupc];
	    if ( Glu->dynamic_snode_bound == YES )
	      new_next = Glu->nextlu;
	    else new_next = Glu->map_in_sup[i];
	    if (new_next < 0) { /* relaxed s-node */
		for (i = fsupc+1; Glu->map_in_sup[i] < 0; ++i) ;
		new_next = Glu->map_in_sup[i];
	    }
	    if ( Glu->map_in_sup[fsupc]>Glu->nzlumax 
		|| Glu->map_in_sup[fsupc]>new_next ) {
		printf("(%d) jcol %d, map_[%d]=%d, map_[%d]=new_next %d\n",
		       pnum, jcol, fsupc, Glu->map_in_sup[fsupc],
		       i, new_next);
		printf("(%d) snode type %d,size %d, |H-snode| %d\n",
		       pnum, Glu->pan_status[fsupc].type, 
		       Glu->pan_status[fsupc].size, part_super_h[fsupc]);
		for (j = fsupc; j < i; j += part_super_h[j])
		    printf("(%d) H snode %d, size %d\n",
			   pnum, j, part_super_h[j]);
		SUPERLU_ABORT("LUSUP exceeded.");  /* xiaoye */
	    }
	}	    
#endif	
	break;

	
      case UCOL: case USUB:

#ifdef PROFILE
	t = SuperLU_timer_();
#endif
	
#if ( MACH==SUN )
	mutex_lock( &pxgstrf_shared->lu_locks[ULOCK] );
#elif ( MACH==DEC || MACH==PTHREAD )
	pthread_mutex_lock( &pxgstrf_shared->lu_locks[ULOCK] );
#elif ( MACH==SGI || MACH==ORIGIN )
#pragma critical lock( pxgstrf_shared->lu_locks[ULOCK] )
#elif ( MACH==CRAY_PVP )
#pragma _CRI guard ULOCK
#elif ( MACH==OPENMP )
#pragma omp critical (ULOCK)
#endif	
	{
	    nextu = Glu->nextu;
	    new_next = nextu + num;
	    if ( new_next > Glu->nzumax ) {
	        XPAND_HINT("U columns", new_next, jcol, 7);
	    }
	    *prev_next = nextu;
	    Glu->nextu = new_next;

	} /* end of critical region */
	
#if ( MACH==SUN )
	mutex_unlock( &pxgstrf_shared->lu_locks[ULOCK] );
#elif ( MACH==DEC || MACH==PTHREAD )
	pthread_mutex_unlock( &pxgstrf_shared->lu_locks[ULOCK] );
#elif ( MACH==CRAY_PVP )
#pragma _CRI endguard ULOCK
#endif

#ifdef PROFILE
	Gstat->procstat[pnum].cs_time += SuperLU_timer_() - t;
#endif
	
	break;

	
	case LSUB:

#ifdef PROFILE
	t = SuperLU_timer_();
#endif
	
#if ( MACH==SUN )
	mutex_lock( &pxgstrf_shared->lu_locks[LLOCK] );
#elif ( MACH==DEC || MACH==PTHREAD )
	pthread_mutex_lock( &pxgstrf_shared->lu_locks[LLOCK] );
#elif ( MACH==SGI || MACH==ORIGIN )
#pragma critical lock( pxgstrf_shared->lu_locks[LLOCK] )
#elif ( MACH==CRAY_PVP )
#pragma _CRI guard LLOCK
#elif ( MACH==OPENMP )
#pragma omp critical (LLOCK)
#endif	
	{
	  nextl = Glu->nextl;
	  new_next = nextl + num;
	  if ( new_next > Glu->nzlmax ) {
	      XPAND_HINT("L subscripts", new_next, jcol, 8);
	  }
	  *prev_next = nextl;
	  Glu->nextl = new_next;
	  
	} /* end of #pragama critical lock() */
	
#if ( MACH==SUN )
	mutex_unlock( &pxgstrf_shared->lu_locks[LLOCK] );
#elif ( MACH==DEC || MACH==PTHREAD )
	pthread_mutex_unlock( &pxgstrf_shared->lu_locks[LLOCK]);
#elif ( MACH==CRAY_PVP )
#pragma _CRI endguard LLOCK
#endif	

#ifdef PROFILE
	Gstat->procstat[pnum].cs_time += SuperLU_timer_() - t;
#endif
	
	  break;

    }
    
    return 0;
}

/*
 * Dynamically set up storage image in lusup[*], using the supernode
 * boundaries in H.
 */
int
DynamicSetMap(
	      const int pnum,      /* process number */
	      const int jcol,      /* leading column of the s-node in H */
	      const int num,       /* number of elements requested */
	      pxgstrf_shared_t *pxgstrf_shared
	      )
{
    GlobalLU_t *Glu = pxgstrf_shared->Glu;
    Gstat_t    *Gstat = pxgstrf_shared->Gstat;
    register int nextlu, new_next;
    int *map_in_sup = Glu->map_in_sup; /* modified; memory mapping function */
    
#ifdef PROFILE
    double t = SuperLU_timer_();
#endif

#if ( MACH==SUN )
    mutex_lock( &pxgstrf_shared->lu_locks[LULOCK] );
#elif ( MACH==DEC || MACH==PTHREAD )
    pthread_mutex_lock( &pxgstrf_shared->lu_locks[LULOCK] );
#elif ( MACH==SGI || MACH==ORIGIN )
#pragma critical lock ( pxgstrf_shared->lu_locks[LULOCK] )
#elif ( MACH==CRAY_PVP )
#pragma _CRI guard LULOCK
#elif ( MACH==OPENMP )
#pragma omp critical (LULOCK)
#endif
    {
	nextlu = Glu->nextlu;
	map_in_sup[jcol] = nextlu;
	new_next = nextlu + num;
	if ( new_next > Glu->nzlumax ) {
	    XPAND_HINT("L supernodes", new_next, jcol, 6);
	}
	Glu->nextlu = new_next;
    } /* end of critical region */

#if ( MACH==SUN )
    mutex_unlock( &pxgstrf_shared->lu_locks[LULOCK] );
#elif ( MACH==DEC || MACH==PTHREAD )
    pthread_mutex_unlock( &pxgstrf_shared->lu_locks[LULOCK] );
#elif ( MACH==CRAY_PVP )
#pragma _CRI endguard LULOCK
#endif

#ifdef PROFILE
    Gstat->procstat[pnum].cs_time += SuperLU_timer_() - t;
#endif

    return 0;
}

