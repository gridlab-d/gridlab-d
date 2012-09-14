///* varmap.c
// * 	Copyright (C) 2008 Battelle Memorial Institute
// */
//
//#include <stdlib.h>
//#include <stdio.h>
//#include <errno.h>
//#include <math.h>
//#include "network.h"
//
//// Variable export table
//typedef struct {
//	char *name;
//	double *addr;
//	double min, max;
//} VARMAP;
//VARMAP varmap[] = {
//#define MAP(X,LO,HI) {#X,&X,LO,HI}
//	/* add global variables you want to be available to core */
//	//MAP(convergence_limit,1e-8,1e-0),
//	//MAP(acceleration_factor,0.5,2.0),
//	//MAP(mvabase,0.0,0.0),
//0};
//
//EXPORT int setvar(char *varname, char *value)
//{
//	VARMAP *p;
//	for (p=varmap; p<varmap+sizeof(varmap)/sizeof(varmap[0]); p++)
//	{
//		if (strcmp(p->name,varname)==0)
//		{
//			double v = atof(value);
//			if (p->min<p->max && v>=p->min && v<=p->max)
//			{	*(p->addr) = v; return 1;}
//			else
//				return 0;
//		}
//	}
//	return 0;
//}
//
//EXPORT void* getvar(char *varname, char *value, unsigned int size)
//{
//	VARMAP *p;
//	if (value==NULL && varname[0]=='\0') // first iterator
//	{
//		strcpy(varname,varmap[0].name);
//		return (void*)1;
//	}
//	for (p=varmap; p<varmap+sizeof(varmap)/sizeof(varmap[0]); p++)
//	{
//		if (strcmp(p->name,varname)==0)
//		{
//			if (size==-1) // indicates the addr is desired
//			{
//				return (void*)p->addr;
//			}
//			else if (value==NULL) // next iterator
//			{
//				if (p+1<varmap+sizeof(varmap)/sizeof(varmap[0])) 
//				{
//					strcpy(varname,(p+1)->name);
//					return (void*)1;
//				}
//				else
//				{
//					return (void*)0; // done iterating
//				}
//			}
//			else {
//				double *v = p->addr;
//				sprintf(value,"%lg",*v);
//				return (void*)1;
//			}
//		}
//	}
//	return (void*)0;
//}
