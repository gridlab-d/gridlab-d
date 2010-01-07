
/*
 * -- SuperLU MT routine (version 2.0) --
 * Lawrence Berkeley National Lab, Univ. of California Berkeley,
 * and Xerox Palo Alto Research Center.
 * September 10, 2007
 *
 */
#include "pdsp_defs.h"

main(int argc, char *argv[])
{
    SuperMatrix   A;
    NCformat *Astore;
    double   *a;
    int      *asub, *xa;
    int      *perm_r; /* row permutations from partial pivoting */
    int      *perm_c; /* column permutation vector */
    SuperMatrix   L;       /* factor L */
    SCPformat *Lstore;
    SuperMatrix   U;       /* factor U */
    NCPformat *Ustore;
    SuperMatrix   B;
    int      nrhs, ldx, info, m, n, nnz, b;
    int      nprocs; /* maximum number of processors to use. */
    int      panel_size, relax, maxsup;
    int      permc_spec;
    trans_t  trans;
    double   *xact, *rhs;
    superlu_memusage_t   superlu_memusage;
    void   parse_command_line();

    nrhs              = 1;
    trans             = NOTRANS;
    nprocs             = 1;
    n                 = 1000;
    b                 = 1;
    panel_size        = sp_ienv(1);
    relax             = sp_ienv(2);
    maxsup            = sp_ienv(3);
    
    parse_command_line(argc, argv, &nprocs, &n, &b, &panel_size, 
		       &relax, &maxsup);

#if ( PRNTlevel>=1 || DEBUGlevel>=1 )
    cpp_defs();
#endif

#define HB
#if defined( DEN )
    m = n;
    nnz = n * n;
    dband(n, n, nnz, &a, &asub, &xa);
#elif defined( BAND )
    m = n;
    nnz = (2*b+1) * n;
    dband(n, b, nnz, &a, &asub, &xa);
#elif defined( BD )
    nb = 5;
    bs = 200;
    m = n = bs * nb;
    nnz = bs * bs * nb;
    dblockdiag(nb, bs, nnz, &a, &asub, &xa);
#elif defined( HB )
    dreadhb(&m, &n, &nnz, &a, &asub, &xa);
#else    
    dreadmt(&m, &n, &nnz, &a, &asub, &xa);
#endif

    dCreate_CompCol_Matrix(&A, m, n, nnz, a, asub, xa, SLU_NC, SLU_D, SLU_GE);
    Astore = A.Store;
    printf("Dimension %dx%d; # nonzeros %d\n", A.nrow, A.ncol, Astore->nnz);
    
    if (!(rhs = doubleMalloc(m * nrhs))) SUPERLU_ABORT("Malloc fails for rhs[].");
    dCreate_Dense_Matrix(&B, m, nrhs, rhs, m, SLU_DN, SLU_D, SLU_GE);
    xact = doubleMalloc(n * nrhs);
    ldx = n;
    dGenXtrue(n, nrhs, xact, ldx);
    dFillRHS(trans, nrhs, xact, ldx, &A, &B);

    if (!(perm_r = intMalloc(m))) SUPERLU_ABORT("Malloc fails for perm_r[].");
    if (!(perm_c = intMalloc(n))) SUPERLU_ABORT("Malloc fails for perm_c[].");

    /*
     * Get column permutation vector perm_c[], according to permc_spec:
     *   permc_spec = 0: natural ordering 
     *   permc_spec = 1: minimum degree ordering on structure of A'*A
     *   permc_spec = 2: minimum degree ordering on structure of A'+A
     *   permc_spec = 3: approximate minimum degree for unsymmetric matrices
     */    	
    permc_spec = 1;
    get_perm_c(permc_spec, &A, perm_c);

    pdgssv(nprocs, &A, perm_c, perm_r, &L, &U, &B, &info);
    
    if ( info == 0 ) {
	dinf_norm_error(nrhs, &B, xact); /* Inf. norm of the error */

	Lstore = (SCPformat *) L.Store;
	Ustore = (NCPformat *) U.Store;
    	printf("#NZ in factor L = %d\n", Lstore->nnz);
    	printf("#NZ in factor U = %d\n", Ustore->nnz);
    	printf("#NZ in L+U = %d\n", Lstore->nnz + Ustore->nnz - L.ncol);
	
	superlu_dQuerySpace(nprocs, &L, &U, panel_size, &superlu_memusage);
	printf("L\\U MB %.3f\ttotal MB needed %.3f\texpansions %d\n",
	       superlu_memusage.for_lu/1024/1024, 
	       superlu_memusage.total_needed/1024/1024,
	       superlu_memusage.expansions);

    }

    SUPERLU_FREE (rhs);
    SUPERLU_FREE (xact);
    SUPERLU_FREE (perm_r);
    SUPERLU_FREE (perm_c);
    Destroy_SuperMatrix_Store(&B);
    Destroy_SuperNode_SCP(&L);
    Destroy_CompCol_NCP(&U);

}

//sj: Rewrite the parse_command_line to make it run in Win32
void
parse_command_line(int argc, char *argv[], int *procs, int *n,
		   int *b, int *w, int *r, int *maxsup)
{
	register int c;
	int i;

	*procs = 2;
	/*for (i=0; i<argc; i++)
	{
		printf( " argv[%d]  %s\n", i, argv[i] );
	}*/
}

///*  
// * Parse command line to get relaxed snode size, panel size, etc.
// */
//void
//parse_command_line(int argc, char *argv[], int *procs, int *n,
//		   int *b, int *w, int *r, int *maxsup)
//{
//    register int c;
//    extern char *optarg;
//
//    while ( (c = getopt(argc, argv, "ht:p:n:b:w:x:s:")) != EOF ) {
//	switch (c) {
//	  case 'h':
//	    printf("Options: (default values are in parenthesis)\n");
//	    printf("\t-p <int> - number of processes     ( %d )\n", *procs);
//	    printf("\t-n <int> - dimension               ( %d )\n", *n);  
//	    printf("\t-b <int> - semi-bandwidth          ( %d )\n", *b);
//	    printf("\t-w <int> - panel size              ( %d )\n", *w);
//	    printf("\t-x <int> - relax                   ( %d )\n", *r);
//	    printf("\t-s <int> - maximum supernode size  ( %d )\n", *maxsup);
//	    exit(1);
//	    break;
//	  case 'p': *procs = atoi(optarg); 
//	            break;
//	  case 'n': *n = atoi(optarg);
//	            break;
//	  case 'b': *b = atoi(optarg);
//	            break;
//	  case 'w': *w = atoi(optarg);
//	            break;
//	  case 'x': *r = atoi(optarg);
//	            break;
//	  case 's': *maxsup = atoi(optarg);
//	            break;
//  	}
//    }
//}

