/** $Id: solvers.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/

#define USE_GLSOLVERS
#include "gridlabd.h"

#ifdef USE_NEWSOLVER

double a=0,n=0,b=0,m=0,c=0;

double ft(double t)
{
	return a*exp(t*n) + b*exp(t*m) + c;
}
double dfdt(double t)
{
	return a*n*exp(t*n) + b*m*exp(t*m);
}
double e2solve(double _a, double _n, double _b, double _m, double _c, double p, double *e)
{
	static glsolver *newton_method = NULL;
	static struct {
		unsigned int n; // dimensions (default 1)
		double *x; // current values of x
		double (**f)(double); // functions
		double (**df)(double); // derivatives
		double *p; // precision
		unsigned int *m; // multiplicities (default is 1)
		unsigned char *s; // status (0=failed, 1=converge, 2=non-converged)
	} data;

	// load the solver if not yet loaded
	if ( newton_method==NULL )
	{
		newton_method = new glsolver("newton_method");
		unsigned int version=0;
		if ( newton_method->get("version",&version,NULL)==0 || version!=1 )
			throw "newton_method version incorrect";
		if ( newton_method->set("dimensions",1,NULL)==0 )
			throw "newton_method dimensions can't be set";
		if ( newton_method->get("init",&data,NULL)==0 )
			throw "newton_method data can't be obtained";
		data.f[0] = ft;
		data.df[0] = dfdt;
	}

	// solve it
	data.x[0] = 0.0;
	data.p[0] = p;
	a=_a; b=_b; c=_c; n=_n; m=_m; 
	if ( newton_method->solve(&data) )
	{
		if ( e!=NULL )
			*e = data.x[0] / dfdt(data.x[0]);
		return data.x[0];
	}
	else
		return NaN;
}

#else

double e2solve(double a, double n, double b, double m, double c, double p, double *e)
{
	// load the solver if not yet loaded
	static glsolver *etp = NULL;
	static struct etpdata {
		double t,a,n,b,m,c,p,e;
		unsigned int i;
	} data;
	if ( etp==NULL )
	{
		etp = new glsolver("etp");
		int version;
		if ( etp->get("version",&version,NULL)==0 || version!=1 )
			throw "incorrect ETP solver version";
		if ( etp->get("init",&data,NULL)==0 )
			throw "unable to initialize ETP solver data";
		data.i = 100;
	}

	// solve it
	data.t = 0;
	data.a = a;
	data.b = b;
	data.c = c;
	data.n = n;
	data.m = m;
	data.p = p;
	if ( etp->solve(&data) )
	{
		if ( e!=NULL )
			*e = data.e;
		return data.t;
	}
	else
		return NaN;
}

#endif
