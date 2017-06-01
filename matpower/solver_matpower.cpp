/** $Id: solver_matpower.cpp 4738 2014-07-03 00:55:39Z dchassin $
	@file solver_matpower.cpp
	@defgroup solver_matpower Template for a new object class
	@ingroup MODULENAME

	You can add an object class to a module using the \e add_class
	command:
	<verbatim>
	linux% add_class CLASS
	</verbatim>

	You must be in the module directory to do this.

 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
//#include "libopf.h"
//#include <stdlib.h>
//#include "matrix.h"
#include "solver_matpower.h"

mxArray* initArray(double rdata[], int nRow, int nColumn) 
{
	mxArray* X = mxCreateDoubleMatrix(nRow, nColumn, mxREAL);
	memcpy(mxGetPr(X), rdata, nRow*nColumn*sizeof(double));
	return X;
}


/* Sync is called when the clock needs to advance on the bottom-up pass */
int solver_matpower()
{

	printf("Running Test\n");
        libopfInitialize();
	mxArray* baseMVA = mxCreateDoubleMatrix(1,1,mxREAL);
	*mxGetPr(baseMVA) = 100.0;

	double rbus[117] = {1,2,3,4,5,6,7,8,9,
		3,2,2,1,1,1,1,1,1,
		0,0,0,0,90,0,100,0,125,
		0,0,0,0,30,0,35,0,50,
		0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0,0,
		345,345,345,345,345,345,345,345,345,
		1,1,1,1,1,1,1,1,1,
		1.1,1.1,1.1,1.1,1.1,1.1,1.1,1.1,1.1,
		0.9,0.9,0.9,0.9,0.9,0.9,0.9,0.9,0.9}; 

	mxArray* bus = initArray(rbus, 9, 13);	

	double rgen[54] = {1,2,3,
			0,163,85,
			0,0,0,
			300,300,300,
			-300,-300,-300,
			1,1,1,
			100,100,100,
			1,1,1,
			250,300,270,
			10,10,10,
			0,0,0,
			0,0,0,
			0,0,0,
			0,0,0,
			0,0,0,
			0,0,0,
			0,0,0,
			0,0,0};


	mxArray* gen = initArray(rgen, 3, 18);

	double rbranch[117] = {1,4,5,3,6,7,8,8,9,
		4,5,6,6,7,8,2,9,4,
		0,0.017,0.039,0,0.0119,0.0085,0,0.032,0.01,
		0.0576,0.092,0.17,0.0586,0.1008,0.072,0.0625,0.161,0.085,
		0,0.158,0.358,0,0.209,0.149,0,0.306,0.176,
		250,250,150,300,150,250,250,250,250,
		250,250,150,300,150,250,250,250,250,
		250,250,150,300,150,250,250,250,250,
		0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,
		-360,-360,-360,-360,-360,-360,-360,-360,-360,
		360,360,360,360,360,360,360,360,360};

	mxArray* branch = initArray(rbranch, 9, 13);

	double rgencost[21] = {2,2,2,
			1500,2000,3000,
			0,0,0,
			3,3,3,
			0.11,0.085,0.1225,
			5,1.2,1,
			150,600,335};

	mxArray* gencost = initArray(rgencost, 3, 7);

	double rareas[2] = {1, 2};
	mxArray* areas = initArray(rareas, 1, 2);


	mxArray* busout;
	mxArray* genout;
	mxArray* branchout;
	mxArray* f;
	mxArray* success;



	mxArray* plhs[5];
	mxArray* prhs[6];
	plhs[0] = busout;
	plhs[1] = genout;
	plhs[2] = branchout;
	plhs[3] = f;
	plhs[4] = success;

	prhs[0] = baseMVA;
	prhs[1] = bus;
	prhs[2] = gen;
	prhs[3] = branch;
	prhs[4] = areas;
	prhs[5] = gencost;

	mlxOpf(0, plhs, 6, prhs);

	return 0;

}


