#include "pdsp_defs.h"


void
pxgstrf_finalize(superlumt_options_t *superlumt_options, SuperMatrix *AC)
{
/*
 * -- SuperLU MT routine (version 2.0) --
 * Lawrence Berkeley National Lab, Univ. of California Berkeley,
 * and Xerox Palo Alto Research Center.
 * September 10, 2007
 *
 * Purpose
 * =======
 * 
 * pxgstrf_finalize() deallocates storage after factorization pxgstrf().
 *
 * Arguments
 * =========
 *
 * superlumt_options (input) superlumt_options_t*
 *        The structure contains the parameters to facilitate sparse
 *        LU factorization.
 *
 * AC (input) SuperMatrix*
 *        The original matrix with columns permuted.
 */
    SUPERLU_FREE(superlumt_options->etree);
    SUPERLU_FREE(superlumt_options->colcnt_h);
    SUPERLU_FREE(superlumt_options->part_super_h);
    Destroy_CompCol_Permuted(AC);
#if ( DEBUGlevel>=1 )
    printf("** pxgstrf_finalize() called\n");
#endif
}
