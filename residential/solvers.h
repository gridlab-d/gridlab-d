// solvers.h
//	Copyright (C) 2008 Battelle Memorial Institute
//
// ETP solver utilities
//
// Update with advanced solver in June 2008 by D. Chassin

#ifndef _SOLVERS_H
#define _SOLVERS_H

#include "gridlabd.h"
extern int64 default_etp_iterations;

double e2solve( double a,double n,double b,double m,double c,double p=1e-8,double *e=NULL);

#endif
