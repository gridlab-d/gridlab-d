// etp.h
//
// Equivalent Thermal Parameters
// Concept by Rob G. Pratt
//
// Nov 26 2002
// DP Chassin
//
// Copyright (C) 2002
// Pacific Northwest National Laboratory
//
// This work was completed under FY03 LDRD
// Cost account #: K38478
//

#ifndef _ETP_H
#define _ETP_H

#ifdef WIN32
// this is ok
#define BINDIR "\\bin\\win32\\"
#define ETPDIR "\\toolbox\\etp\\"
#define EXEFORMAT "%s\\%s.exe"
#else
#error only WIN32 is supported
#endif

// maximum size of a string passed in from MATLAB
#define MAXNAME 32

#ifdef DEBUG_TRACE
#include <stdio.h>
#define TRACE(M) mexPrintf(M);
#define TRACE1(M,A) mexPrintf(M,A);
#define TRACE2(M,A,B) mexPrintf(M,A,B);
#define TRACE2(M,A,B) mexPrintf(M,A,B);
#define TRACE3(M,A,B,C) mexPrintf(M,A,B,C);
#else
#define TRACE(M)
#define TRACE1(M,A)
#define TRACE2(M,A,B)
#define TRACE3(M,A,B,C)
#endif

// useful constants
#define RHOWATER	(61.82)			// lb/cf
#define CFPGAL		(0.133681)		// cf/gal
#define CWATER		(0.9994)		// BTU/lb/F
#define BTUPHPW		(3.4120)		// BTUPH/W
#define MWPBTUPH	(1e-6/BTUPHPW)	// MW/BTUPH

// random distribution functions
#define RANDU(L,H) (((double)rand()/(double)RAND_MAX)*((H)-(L))+(L))

// approximate tests
#define AEQ(A,B,C) (fabs(A-B)<C)
#define ANE(A,B,C) (fabs(A-B)>=C)
#define ALT(A,B,C) (A<=B+C)
#define AGT(A,B,C) (A>=B-C)

#define MAX(A,B) ((A)>(B)?(A):(B))
#define MIN(A,B) ((A)<(B)?(A):(B))

typedef struct
{
	double *data;
	double time;
	double value;
} EVENT;

typedef enum {INTEGER, REAL} PROPERTYTYPE;
typedef struct {
	PROPERTYTYPE type;
	union {
		double *real;
		int *integer;
		void *addr;
	};
	int limit;
} PROPERTY;

#include "market.h"
#include "house.h"
#include "solvers.h"

#endif
