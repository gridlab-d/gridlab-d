/* residential.h
	Copyright (C) 2008 Battelle Memorial Institute
 * contains all utility functions and common definitions for the residential module
 */

#ifndef _RESIDENTIAL_H
#define _RESIDENTIAL_H

#include "gridlabd.h"

/* useful constants */
#define RHOWATER	(62.4)			// lb/cf
#define CFPGAL		(0.133681)		// cf/gal
#define GALPCF		(7.4805195)		// gal/cf
#define CWATER		(0.9994)		// BTU/lb/F
#define BTUPHPW		(3.4120)		// BTUPH/W
#define BTUPHPKW	(1e3 * 3.4120)		// BTUPH/kW
#define KWPBTUPH	(1e-3/BTUPHPW)	// kW/BTUPH
#define MWPBTUPH	(1e-6/BTUPHPW)	// MW/BTUPH
#define ROUNDOFF	1e-6			// numerical accuracy for zero in float comparisons

const double pi = 3.1415926535897931;
const double Cp = 1;					// Btu/lbm-F

/* approximate tests */
#define AEQ(A,B,C) (fabs(A-B)<C)
#define ANE(A,B,C) (fabs(A-B)>=C)
#define ALT(A,B,C) (A<=B+C)
#define AGT(Ak,B,C) (A>=B-C)

#define MAX(A,B) ((A)>(B)?(A):(B))
#define MIN(A,B) ((A)<(B)?(A):(B))

#endif  /* _RESIDENTIAL_H */

