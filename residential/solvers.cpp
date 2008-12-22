// solvers.cpp: general equation solver for compound exponential decays
//	Copyright (C) 2008 Battelle Memorial Institute
// ETP solver utilities
//
// DP Chassin
// December 2002
// Copyright (C) 2002, Pacific Northwest National Laboratory
// All Rights Reserved
//
// Update with advanced solver in June 2008 by D. Chassin

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include "gridlabd.h"

#include "solvers.h"

#ifndef WIN32
#define _finite isfinite
#endif
static unsigned long _nan[2] = {0xffffffff,0x7fffffff};
static double NaN = *(double*)&_nan;
#define EVAL(t,a,n,b,m,c) (a*exp(n*t) + b*exp(m*t) + c)
/** solve an equation of the form $f[ ae^{nt} + be^{mt} + c = 0 $f]
	@returns the solution in $f[ t $f], or etp::NaN if none found
 **/
double e2solve( double a,	/**< the parameter \p a */
				double n,	/**< the parameter \p n (should be negative) */
				double b,	/**< the parameter \p b */
				double m,	/**< the parameter \p m (should be negative) */
				double c,	/**< the constant \p c */
				double p,	/**< the precision (1e-8 if omitted) */
				double *e)	/**< pointer to error estimate (null if none desired) */
{
	double t=0;
	double f = EVAL(t,a,n,b,m,c);

	// is there an extremum/inflexion to consider
	if (a*b<0)
	{
		// compute the time t and value fi at which it occurs
		double an_bm = -a*n/(b*m);
		double tm = log(an_bm)/(m-n);
		double fm = EVAL(tm,a,n,b,m,c);
		double ti = log(an_bm*n/m)/(m-n);
		double fi = EVAL(ti,a,n,b,m,c);
		if (tm>0) // extremum in domain
		{
			if (f*fm<0) // first solution is in range
				t = 0;
			else if (c*fm<0) // second solution is in range
				t = ti;
			else // no solution is in range
				return NaN;
		}
		else if (tm<0 && ti>0) // no extremum but inflexion in domain
		{
			if (fm*c<0) // solution in range
				t = ti;
			else // no solution in range
				return NaN;
		}
		else if (ti<0) // no extremum or inflexion in domain
		{
			if (fi*c<0) // solution in range
				t = ti;
			else // no solution in range
				return NaN;
		}
		else // no solution possible (includes tm==0 and ti==0)
			return NaN;
	}

	// solve using Newton's method
	int iter = 100;
	if (t!=0) // initial t changed to inflexion point
		double f = EVAL(t,a,n,b,m,c);
	double dfdt = EVAL(t,a*n,n,b*m,m,0); 
	while ( fabs(f)>p && isfinite(t) && iter-->0 )
	{
		t -= f/dfdt;
		f = EVAL(t,a,n,b,m,c);
		dfdt = EVAL(t,a*n,n,b*m,m,0);
	}
	if (iter==0)
	{
		gl_warning("etp::solve(a=%.4f,n=%.4f,b=%.4f,m=%.4f,c=%.4f,prec=%.g) failed to converge",a,n,b,m,c,p);
		return NaN;	// failed to catch limit condition above
	}
	if (e!=NULL)
		*e = p/dfdt;
	return t>0?t:NaN;
}

