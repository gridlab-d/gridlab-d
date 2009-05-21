/* $Id
 * Newton-Raphson solver
 */

#ifndef _SOLVER_NR
#define _SOLVER_NR

#include "complex.h"

typedef struct  {
	int type;		///< bus type (0=PQ, 1=PV, 2=SWING)
	complex *V[3];	///< bus voltage
	complex *S[3];	///< constant power
	complex *Y[3];	///< constant admittance (impedance loads)
	complex *I[3];	///< constant current
	double *PL[3]; ///< system load at each bus in terms of P
	double *QL[3]; ///< system load at each bus in terms of Q
	double kv_base; ///< kV basis
	double mva_base; /// MVA basis
} BUSDATA;

typedef struct {
	complex *Y[3][3]; ///< branch admittance
	int from;         ///< index into bus data
	int to;	          ///< index into bus data
	double v_ratio;   ///< voltage ratio (v_from/v_to)
} BRANCHDATA;

int solver_nr(int bus_count, BUSDATA *bus, int branch_count, BRANCHDATA *branch);

#endif
