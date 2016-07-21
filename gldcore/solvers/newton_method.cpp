// $Id: newton_method.cpp 4738 2014-07-03 00:55:39Z dchassin $
// Copyright (C) 2012 Battelle Memorial Institute
// DP Chassin

#include <stdlib.h>
#include "gridlabd.h"

static unsigned int solver_version = 1; // change this each time to data structure changes
typedef struct {
	unsigned int n; // dimensions (default 1)
	double *x; // current values of x
	double (**f)(double); // functions
	double (**df)(double); // derivatives
	double *p; // precisions
	unsigned int *m; // multiplicities (default is 1)
	unsigned char *s; // status (0=failed, 1=converge, 2=non-converged)
} NMDATA;

static unsigned int max_iterations = 100;
static unsigned int dimensions = 1;

EXPORT int newton_method_init(CALLBACKS *fntable)
{
	callback = fntable;
	return 1;
}

EXPORT int newton_method_set(char *param, ...)
{
	int n=0;
	va_list arg;
	va_start(arg,param);
	char *tag = param;
	while ( tag!=NULL )
	{
		if ( strcmp(param,"max_iterations")==0 )
			max_iterations = va_arg(arg,unsigned int);
		else if ( strcmp(param,"dimensions")==0 )
			dimensions = va_arg(arg,unsigned int);
		else
		{
			gl_error("newton_method_set(char *param='%s',...): tag '%s' is not recognized",param,tag);
			return n;
		}
		tag = va_arg(arg,char*);
		n++;
	}
	return n;
}

EXPORT int newton_method_get(char *param, ...)
{
	int n=0;
	va_list arg;
	va_start(arg,param);
	char *tag = param;
	while ( tag!=NULL )
	{
		if ( strcmp(param,"solver_version")==0 )
			*va_arg(arg,unsigned int*) = solver_version;
		else if ( strcmp(param,"max_iterations")==0 )
			*va_arg(arg,unsigned int*) = max_iterations;
		else if ( strcmp(param,"dimensions")==0 )
			*va_arg(arg,unsigned int*) = dimensions;
		else if ( strcmp(param,"init")==0 )
		{
			NMDATA *data = (NMDATA*)va_arg(arg,NMDATA*);
			data->n = dimensions;
			data->df = (double(**)(double))malloc(sizeof(void*)*dimensions);
			data->f = (double(**)(double))malloc(sizeof(void*)*dimensions);
			data->x = (double*)malloc(sizeof(double)*dimensions);
			data->m = (unsigned int*)malloc(sizeof(unsigned int)*dimensions);
			data->p = (double*)malloc(sizeof(double)*dimensions);
			data->s = (unsigned char*)malloc(sizeof(unsigned int)*dimensions);
			size_t i;
			for ( i=0 ; i<dimensions ; i++ )
			{
				data->x[i] = 0;
				data->df[i] = NULL;
				data->f[i] = NULL;
				data->m[i] = 1;
				data->p[i] = 1e-8;
				data->s[i] = 1;
			}
		}
		else
		{
			gl_error("newton_method_get(char *param='%s',...): tag '%s' is not recognized",param,tag);
			return n;
		}
		tag = va_arg(arg,char*);
		n++;
	}
	return n;
}

EXPORT int newton_method_solve(NMDATA *data)
{
	unsigned int n, s=1;
	for ( n=0 ; n<data->n ; n++ )
	{
		double &p = data->p[n];
		double &x = data->x[n];
		double (*f)(double) = data->f[n];
		double (*df)(double) = data->df[n];
		unsigned int &m = data->m[n];
		double dx;
		unsigned int i=0;
		do { 
			double dydx = (*df)(x);
			if ( isnan(dydx) || dydx==0 )
			{
				x = NaN;
				s = data->s[n]  =0;
				break;
			}
			else if ( !isfinite(dydx) )
				break;
			else
			{
				dx = m*(*f)(x)/dydx;
				x -= dx;
			}
			if ( i++>max_iterations )
			{
				if ( s>0 ) s=2; // only flag if not already flagged for failure
				data->s[n] = 2;
			}
		} while ( fabs(dx)>p );
	}
	return s;
}


