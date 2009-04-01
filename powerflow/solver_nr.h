/* $Id
 * Newton-Raphson solver
 */

#ifndef _SOLVER_NR
#define _SOLVER_NR

#include "complex.h"

typedef struct  {
	complex *V[3];	///< bus voltage
	complex *S[3];	///< constant power
	complex *Z[3];	///< constant impedance
	complex *I[3];	///< constant current
} BUSDATA;

typedef struct {
	complex Y[3]; ///< branch admittance
	int from; ///< index into bus data
	int to; ///< index into bus data
} BRANCHDATA;

int solve_nr(int bus_count, BUSDATA *bus, int branch_count, BRANCHDATA *branch);

#endif
