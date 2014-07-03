
#include "pzsp_defs.h"

void
pzgstrf_relax_snode(
		    const int n, /* number of columns in the matrix */
		    //pzgstrf_options_t *pzgstrf_options,
			superlumt_options_t *pzgstrf_options, //sj
		    pxgstrf_relax_t *pxgstrf_relax /* relaxed s-nodes */
		    )
{
/*
 * -- SuperLU MT routine (version 2.0) --
 * Lawrence Berkeley National Lab, Univ. of California Berkeley,
 * and Xerox Palo Alto Research Center.
 * September 10, 2007
 *
 * Purpose
 * =======
 *   pzgstrf_relax_snode() identifes the initial relaxed supernodes, 
 *   assuming that the matrix has been reordered according to the postorder
 *   of the etree.
 *
 */ 
    register int j, parent, rs;
    register int fcol;	 /* beginning of a snode */
    int *desc;  /* no of descendants of each etree node. */
    int *etree = pzgstrf_options->etree; /* column elimination tree */
    int relax = pzgstrf_options->relax; /* maximum no of columns allowed 
					   in a relaxed s-node */
    
    desc = intCalloc(n+1);

    /* Compute the number of descendants of each node in the etree */
    for (j = 0; j < n; j++) {
	parent = etree[j];
	desc[parent] += desc[j] + 1;
    }
    
    rs = 1;
    
    /* Identify the relaxed supernodes by postorder traversal of the etree. */
    for (j = 0; j < n; ) { 
     	parent = etree[j];
        fcol = j;
 	while ( parent != n && desc[parent] < relax ) {
	    j = parent;
	    parent = etree[j];
	}
	/* found a supernode with j being the last column. */
	pxgstrf_relax[rs].fcol = fcol;
	pxgstrf_relax[rs].size = j - fcol + 1;
#ifdef DOMAINS
	for (i = fcol; i <= j; ++i) in_domain[i] = RELAXED_SNODE;
#endif
	j++;    rs++;
	/* Search for a new leaf */
	while ( desc[j] != 0 && j < n ) j++;
    }

    pxgstrf_relax[rs].fcol = n;
    pxgstrf_relax[0].size = rs-1; /* number of relaxed supernodes */

#if (PRNTlevel==1)
    printf(".. No of relaxed s-nodes %d\n", pxgstrf_relax[0].size);
#endif
    
    SUPERLU_FREE (desc);

}


