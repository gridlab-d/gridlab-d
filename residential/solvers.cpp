/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
**/

#define USE_GLSOLVERS
#include "gridlabd.h"

double e2solve(double a, double n, double b, double m, double c, double p, double *e)
{
	// load the solver if not yet loaded
	static glsolver *etp = NULL;
	if ( etp==NULL )
	{
		etp = new glsolver("etp");
		etp->init(NULL);
	}

	// setup the data
	double t=0;
	struct etpdata {
		double t,a,n,b,m,c,p,*e;
	} data = {t,a,n,b,m,c,p,e};

	// solve it
	if ( etp->solve(&data) )
		return data.t;
	else
		return NaN;
}

