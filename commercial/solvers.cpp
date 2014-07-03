/** $Id: solvers.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <time.h>

#include "gridlabd.h"
#include "solvers.h"

#define EVAL(t,a,n,b,m,c) (a*exp(n*t) + b*exp(m*t) + c)
/** solve an equation of the form $f[ ae^{nt} + be^{mt} + c = 0 $f]
	@returns the solution in $f[ t $f], or etp::NaN if none found
 **/
double e2solve(double a,/**< the parameter \p a */
	double n,	/**< the parameter \p n (should be negative) */
	double b,	/**< the parameter \p b */
	double m,	/**< the parameter \p m (should be negative) */
	double c,	/**< the constant \p c */
	double p,	/**< the precision (1e-8 if omitted) */
	double *e)	/**< pointer to error estimate (null if none desired) */
{
	double t=0;
	double f = EVAL(t,a,n,b,m,c);

	// check for degenerate cases (1 exponential term is dominant)
	// solve for t in dominant exponential, but only when a solution exists
	// (c must be opposite sign of scalar and have less magnitude than scalar)
	if (fabs(a/b)<p) // a is dominant
		return c*b<0 && fabs(c)<fabs(b) ? log(-c/b)/m : NaN;
	else if (fabs(b/a)<p) // b is dominant
		return c*a<0 && fabs(c)<fabs(a) ? log(-c/a)/n : NaN;

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
	else if (f*c>0) // solution is not reachable from t=0 (same sign)
	{
		return NaN;
	}

	// solve using Newton's method
	int iter = 100;
	if (t!=0) // initial t changed to inflexion point
		double f = EVAL(t,a,n,b,m,c);
	double dfdt = EVAL(t,a*n,n,b*m,m,0); 
	while ( fabs(f)>p && isfinite(t) && iter-->0)
	{
		t -= f/dfdt;
		f = EVAL(t,a,n,b,m,c);
		dfdt = EVAL(t,a*n,n,b*m,m,0);
	}
	if (iter==0)
	{
		gl_error("etp::solve(a=%.4f,n=%.4f,b=%.4f,m=%.4f,c=%.4f,prec=%.g) failed to converge",a,n,b,m,c,p);
		return NaN;	// failed to catch limit condition above
	}
	if (e!=NULL)
		*e = p/dfdt;
	return t>0?t:NaN;
}

