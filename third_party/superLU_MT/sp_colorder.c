#include "pdsp_defs.h"

void
sp_colorder(SuperMatrix *A, int *perm_c, superlumt_options_t *options,
	    SuperMatrix *AC)
{
/*
 * -- SuperLU MT routine (version 2.0) --
 * Lawrence Berkeley National Lab, Univ. of California Berkeley,
 * and Xerox Palo Alto Research Center.
 * September 10, 2007
 *
 *
 * Purpose
 * =======
 *
 * sp_colorder() permutes the columns of the original matrix A into AC. 
 * It performs the following steps:
 *
 *    1. Apply column permutation perm_c[] to A's column pointers to form AC;
 *
 *    2. If options->refact = NO, then
 *       (1) Allocate etree[], and compute column etree etree[] of AC'AC;
 *       (2) Post order etree[] to get a postordered elimination tree etree[],
 *           and a postorder permutation post[];
 *       (3) Apply post[] permutation to columns of AC;
 *       (4) Overwrite perm_c[] with the product perm_c * post.
 *       (5) Allocate storage, and compute the column count (colcnt_h) and the
 *           supernode partition (part_super_h) for the Householder matrix H.
 *
 * Arguments
 * =========
 *
 * A      (input) SuperMatrix*
 *        Matrix A in A*X=B, of dimension (A->nrow, A->ncol). The number
 *        of the linear equations is A->nrow. Currently, the type of A can be:
 *        Stype = NC or NCP; Dtype = _D; Mtype = GE.
 *
 * perm_c (input/output) int*
 *	  Column permutation vector of size A->ncol, which defines the 
 *        permutation matrix Pc; perm_c[i] = j means column i of A is 
 *        in position j in A*Pc.
 *
 * options (input/output) superlumt_options_t*
 *        If options->refact = YES, then options is an
 *        input argument. The arrays etree[], colcnt_h[] and part_super_h[]
 *        are available from a previous factor and will be re-used.
 *        If options->refact = NO, then options is an output argument. 
 *
 * AC     (output) SuperMatrix*
 *        The resulting matrix after applied the column permutation
 *        perm_c[] to matrix A. The type of AC can be:
 *        Stype = NCP; Dtype = _D; Mtype = GE.
 *
 */

    NCformat  *Astore;
    NCPformat *ACstore;
    int i, n, nnz, nlnz;
    yes_no_t  refact = options->refact;
    int *etree;
    int *colcnt_h;
    int *part_super_h;
    int *iwork, *post, *iperm;
    int *invp;
    int *part_super_ata;

    n     = A->ncol;
    iwork = intMalloc(n+1);
    part_super_ata = intMalloc(n);
    
    /* Apply column permutation perm_c to A's column pointers so to
       obtain NCP format in AC = A*Pc.  */
    AC->Stype       = SLU_NCP;
    AC->Dtype       = A->Dtype;
    AC->Mtype       = A->Mtype;
    AC->nrow        = A->nrow;
    AC->ncol        = A->ncol;
    Astore          = A->Store;
    ACstore = AC->Store = (void *) malloc( sizeof(NCPformat) );
    ACstore->nnz    = Astore->nnz;
    ACstore->nzval  = Astore->nzval;
    ACstore->rowind = Astore->rowind;
    ACstore->colbeg = intMalloc(n);
    ACstore->colend = intMalloc(n);
    nnz             = Astore->nnz;

#ifdef CHK_COLORDER
    print_int_vec("pre_order:", n, perm_c);
    dcheck_perm("Initial perm_c", n, perm_c);
#endif      

    for (i = 0; i < n; i++) {
	ACstore->colbeg[perm_c[i]] = Astore->colptr[i]; 
	ACstore->colend[perm_c[i]] = Astore->colptr[i+1];
    }
	
    if ( refact == NO ) {
	
	options->etree = etree = intMalloc(n);
	options->colcnt_h = colcnt_h = intMalloc(n);
	options->part_super_h = part_super_h = intMalloc(n);
	
	/* Compute the column elimination tree. */
	sp_coletree(ACstore->colbeg, ACstore->colend, ACstore->rowind,
		    A->nrow, A->ncol, etree);
#ifdef CHK_COLORDER	
	print_int_vec("etree:", n, etree);
#endif	

	/* In symmetric mode, do not do postorder here. */
	if ( options->SymmetricMode == NO ) {

	    /* Post order etree. */
	    post = (int *) TreePostorder(n, etree);
	    invp  = intMalloc(n);
	    for (i = 0; i < n; ++i) invp[post[i]] = i;

#ifdef CHK_COLORDER
	    print_int_vec("post:", n+1, post);
	    dcheck_perm("post", n, post);	
#endif	

	    /* Renumber etree in postorder. */
	    for (i = 0; i < n; ++i) iwork[post[i]] = post[etree[i]];
	    for (i = 0; i < n; ++i) etree[i] = iwork[i];

#ifdef CHK_COLORDER	
	    print_int_vec("postorder etree:", n, etree);
#endif

	    /* Postmultiply A*Pc by post[]. */
	    for (i = 0; i < n; ++i) iwork[post[i]] = ACstore->colbeg[i];
	    for (i = 0; i < n; ++i) ACstore->colbeg[i] = iwork[i];
	    for (i = 0; i < n; ++i) iwork[post[i]] = ACstore->colend[i];
	    for (i = 0; i < n; ++i) ACstore->colend[i] = iwork[i];

	    for (i = 0; i < n; ++i)
		iwork[i] = post[perm_c[i]];  /* product of perm_c and post */
	    for (i = 0; i < n; ++i) perm_c[i] = iwork[i];
	    for (i = 0; i < n; ++i) invp[perm_c[i]] = i; /* inverse of perm_c*/

	    iperm = post;

#ifdef ZFD_PERM
	    /* Permute the rows of AC to have zero-free diagonal. */
	    printf("** Permute the rows to have zero-free diagonal....\n");
	    for (i = 0; i < n; ++i)
		iwork[i] = ACstore->colend[i] - ACstore->colbeg[i];
	    zfdperm(n, nnz, ACstore->rowind, ACstore->colbeg, iwork, iperm);
#else
	    for (i = 0; i < n; ++i) iperm[i] = i;
#endif	

	    /* NOTE: iperm is returned as column permutation so that
	     * the diagonal is nonzero. Since a symmetric permutation
	     * preserves the diagonal, we can do the following:
	     *     P'(AP')P = P'A
	     * That is, we apply the inverse of iperm to rows of A
	     * to get zero-free diagonal. But since iperm is defined
	     * in MC21A inversely as our definition of permutation,
	     * so it is indeed an inverse for our purpose. We can
	     * apply it directly.
	     */
	
	    /* Determine the row and column counts in the QR factor. */
	    qrnzcnt(n, nnz, Astore->colptr, Astore->rowind, iperm,
		    invp, perm_c, etree, colcnt_h, &nlnz,
		    part_super_ata, part_super_h);

#if 0	
	    dCheckZeroDiagonal(n, ACstore->rowind, ACstore->colbeg,
			       ACstore->colend, iperm);
	    dPrintSuperPart("Hpart", n, part_super_h);
	    exit(0);
	    print_int_vec("iperm", n, iperm);
#endif	
	
#ifdef CHK_COLORDER
	    print_int_vec("Pc*post:", n, perm_c);
	    dcheck_perm("final perm_c", n, perm_c);	
#endif

	    SUPERLU_FREE (post);
	    SUPERLU_FREE (invp);
	}

    } /* if refact == NO */

    SUPERLU_FREE (iwork);
    SUPERLU_FREE (part_super_ata);
}

int
dCheckZeroDiagonal(int n, int *rowind, int *colbeg,
		  int *colend, int *iperm)
{
    register int i, j, nzd;

    for (j = 0; j < n; ++j) {
	nzd = 0;
	for (i = colbeg[j]; i < colend[j]; ++i) {
	    if ( iperm[rowind[i]] == j ) nzd = 1;
	}
	if ( nzd == 0 ) printf("Diagonal of column %d is zero.\n", j);
    }

    return 0;
}

int
dPrintSuperPart(char *pname, int n, int *part_super)
{
    register int i;
    FILE *fopen(), *fp;
    char fname[20];
    strcpy(fname, pname);
    strcat(fname, ".dat");
    fp = fopen(fname, "w");
    for (i = 0; i < n; ++i)
	if ( part_super[i] )
	    fprintf(fp, "%8d", i);
    fprintf(fp, "%8d", n);
    fclose(fp);
    return 0;
}

int dcheck_perm(char *what, int n, int *perm)
{
    register int i;
    int          *marker;
    marker = (int *) intCalloc(n);

    for (i = 0; i < n; ++i) {
	if ( marker[perm[i]] == 1 || perm[i] >= n ) {
	    printf("%s: Not a valid PERM[%d] = %d\n", what, i, perm[i]);
	    SUPERLU_ABORT("Invalid perm.");
	} else {
	    marker[perm[i]] = 1;
	}
    }

    return 0;
}
