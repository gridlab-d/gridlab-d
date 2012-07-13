// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
// DP Chassin

#include <stdlib.h>
#include "gridlabd.h"

static int64 default_etp_iterations = 100;
#define EVAL(t,a,n,b,m,c) (a*exp(n*t) + b*exp(m*t) + c)

EXPORT int etp_init(void *arg)
{
	return 1;
}

EXPORT int etp_set(char *param, ...)
{
	int n=0;
	va_list arg;
	va_start(arg,param);
	char *tag = param;
	while ( tag!=NULL )
	{
		if ( strcmp(param,"max_iterations")==0 )
			default_etp_iterations = va_arg(arg,int64);
		else
		{
			gl_error("etp_config(char *param='%s',...): tag '%s' is not recognized");
			return n;
		}
		tag = va_arg(arg,char*);
		n++;
	}
	return n;
}

EXPORT int etp_get(char *param, ...)
{
	int n=0;
	va_list arg;
	va_start(arg,param);
	char *tag = param;
	while ( tag!=NULL )
	{
		if ( strcmp(param,"max_iterations")==0 )
			*va_arg(arg,int64*) = default_etp_iterations;
		else
		{
			gl_error("etp_config(char *param='%s',...): tag '%s' is not recognized");
			return n;
		}
		tag = va_arg(arg,char*);
		n++;
	}
	return n;
}

/** solve an equation of the form $f[ ae^{nt} + be^{mt} + c = 0 $f]
	@returns 1 = t is set ok, 0 if not converged; t is NaN if no solution found
 **/
typedef struct s_etpsolver {
	double t,a,n,b,m,c,p,*e;
} ETPSOLVER;
EXPORT int etp_solve(ETPSOLVER *etp)
{
	const double &a = etp->a;
	const double &n = etp->n;
	const double &b = etp->b;
	const double &m = etp->m;
	const double &c = etp->c;
	const double &p = etp->p;
	double *e = etp->e;
	double &t= etp->t;
	double f = EVAL(t,a,n,b,m,c);

	// check for degenerate cases (1 exponential term is dominant)
	// solve for t in dominant exponential, but only when a solution exists
	// (c must be opposite sign of scalar and have less magnitude than scalar)
	if (fabs(a/b)<p) // a is dominant
	{
		t = c*b<0 && fabs(c)<fabs(b) ? log(-c/b)/m : NaN;
		return 1;
	}
	else if (fabs(b/a)<p) // b is dominant
	{
		t = c*a<0 && fabs(c)<fabs(a) ? log(-c/a)/n : NaN;
		return 1;
	}

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
			{
				t = NaN;
				return 1;
			}
		}
		else if (tm<0 && ti>0) // no extremum but inflexion in domain
		{
			if (fm*c<0) // solution in range
				t = ti;
			else // no solution in range
			{
				t = NaN;
				return 1;
			}
		}
		else if (ti<0) // no extremum or inflexion in domain
		{
			if (fi*c<0) // solution in range
				t = ti;
			else // no solution in range
			{
				t = NaN;
				return 1;
			}
		}
		else // no solution possible (includes tm==0 and ti==0)
		{
			t = NaN;
			return 1;
		}
	}
	else if (f*c>0) // solution is not reachable from t=0 (same sign)
	{
		t = NaN;
		return 1;
	}

	// solve using Newton's method
	int64 iter = default_etp_iterations;
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
		t = NaN;	// failed to catch limit condition above
		return 2;
	}
	if (e!=NULL)
		*e = p/dfdt;
	if (t<=0) t = NaN;
	return 1;
}


