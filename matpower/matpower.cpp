/** $Id: matpower.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
#include "matpower.h"
#include "bus.h"
#include "gen.h"
#include "branch.h"
//#include "gen_cost.h"
#include "areas.h"
#include "baseMVA.h"
#include <math.h>

#include <iostream>




//#include "libopf.h"
//#include <stdlib.h>
//#include "matrix.h"
//#include "solver_matpower.h"

mxArray* initArray(double rdata[], int nRow, int nColumn) 
{
	mxArray* X = mxCreateDoubleMatrix(nRow, nColumn, mxREAL);
	memcpy(mxGetPr(X), rdata, nRow*nColumn*sizeof(double));
	return X;
}

double* getArray(mxArray *X)
{
	unsigned int NumOfElement = mxGetNumberOfElements(X);
	//printf("Number of element %d\n",NumOfElement);
	double *outputdata;
	outputdata = (double *) malloc(NumOfElement*sizeof(double));
	memcpy(outputdata, mxGetPr(X),NumOfElement*sizeof(double));
	return outputdata;
	
}


/* Sync is called when the clock needs to advance on the bottom-up pass */
//int solver_matpower(double *rbus, unsigned int nbus, double *rgen, unsigned int ngen, 
//	double *rbranch, unsigned int nbranch, double *rgencost, unsigned int ngencost,
//	double *rareas,	unsigned int nareas)
int solver_matpower(vector<unsigned int> bus_BUS_I, vector<unsigned int> branch_F_BUS, vector<unsigned int> branch_T_BUS, vector<unsigned int> gen_GEN_BUS, vector<unsigned int> gen_NCOST,unsigned int BASEMVA)
{	
	unsigned int nbus = 0;
	unsigned int ngen = 0;
	unsigned int nbranch = 0;
	//unsigned int ngencost = 0;
	//unsigned int nareas = 0;
	//unsigned int nbaseMVA = 0;

	vector<bus> vec_bus;
	vector<gen> vec_gen;
	vector<branch> vec_branch;
	//vector<areas> vec_areas;
	//vector<gen_cost> vec_gencost;
	//vector<baseMVA> vec_baseMVA;
	

	
	//printf("========Getting Data=============\n");

	// Get Bus objects
	OBJECT *temp_obj = NULL;
	bus *list_bus;
	FINDLIST *bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"bus",FT_END);
	while (gl_find_next(bus_list,temp_obj)!=NULL)
	{
		
		temp_obj = gl_find_next(bus_list,temp_obj);
		list_bus = OBJECTDATA(temp_obj,bus);
		vec_bus.push_back(*list_bus);
		
        };

	// Get Generator objects
	gen *list_gen;
	FINDLIST *gen_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"gen",FT_END);
	temp_obj = NULL;
	
	while (gl_find_next(gen_list,temp_obj)!=NULL)
	{
		temp_obj = gl_find_next(gen_list,temp_obj);
		list_gen = OBJECTDATA(temp_obj,gen);
		vec_gen.push_back(*list_gen);
        };

	// Get Line/Branch Objects
	branch *list_branch;
	FINDLIST *branch_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"branch",FT_END);
	temp_obj = NULL;

	while (gl_find_next(branch_list,temp_obj)!=NULL)	
	{
		temp_obj = gl_find_next(branch_list,temp_obj);
		list_branch = OBJECTDATA(temp_obj,branch);
		vec_branch.push_back(*list_branch);
	}

	// Get Area Objects
/*
	areas *list_areas;
	FINDLIST *areas_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"areas",FT_END);
	temp_obj = NULL;
	
	while (gl_find_next(areas_list,temp_obj) != NULL)
	{
		temp_obj = gl_find_next(areas_list,temp_obj);
		list_areas = OBJECTDATA(temp_obj,areas);
		vec_areas.push_back(*list_areas);
	}
*/
	
	// Get Generator Cost objects
	/*
	gen_cost *list_gen_cost;
	FINDLIST *gen_cost_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"gen_cost",FT_END);
	temp_obj = NULL;

	while (gl_find_next(gen_cost_list,temp_obj)!=NULL)
	{
		temp_obj = gl_find_next(gen_cost_list,temp_obj);
		list_gen_cost = OBJECTDATA(temp_obj,gen_cost);
		vec_gencost.push_back(*list_gen_cost);

	}
	*/
	// Get Base Information object
	//baseMVA *list_baseMVA;
	//FINDLIST *baseMVA_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"baseMVA",FT_END);
	//temp_obj = NULL;
	//temp_obj = gl_find_next(baseMVA_list,temp_obj);
	//list_baseMVA = OBJECTDATA(temp_obj,baseMVA);
	//vec_baseMVA.push_back(*list_baseMVA);

	// Get the size of each class
	nbus = vec_bus.size();
	ngen = vec_gen.size();
	nbranch = vec_branch.size();
	//ngencost = vec_gencost.size();
	//nareas = vec_areas.size();
	//nbaseMVA = vec_baseMVA.size();


	// create arrays for input and allocate memory
	double *rbus;
	rbus = (double *) calloc(nbus*BUS_ATTR,sizeof(double));

	double *rgen;
	rgen = (double *) calloc(ngen*GEN_ATTR,sizeof(double));	

	double *rbranch;
	rbranch = (double *) calloc(nbranch*BRANCH_ATTR,sizeof(double));

	double *rareas;
	rareas = (double *) calloc(AREA_ATTR,sizeof(double));

	double rbaseMVA;

	double *rgencost; // allocation of memory is in the following part
	
	

	
	// insert bus data for rbus
	vector<bus>::iterator iter_bus = vec_bus.begin();
	if (nbus > 1)
	{
		for (unsigned int i=0; i < nbus; i++)
		{
			//rbus[i+0*nbus] = (double)iter_bus->BUS_I;
			rbus[i+0*nbus] = bus_BUS_I[i];
			rbus[i+1*nbus] = (double)iter_bus->BUS_TYPE;
			rbus[i+2*nbus] = iter_bus->PD;
			rbus[i+3*nbus] = iter_bus->QD;
			rbus[i+4*nbus] = iter_bus->GS;
			rbus[i+5*nbus] = iter_bus->BS;
			//rbus[i+6*nbus] = (double)iter_bus->BUS_AREA;
			rbus[i+6*nbus] = 1;
			rbus[i+7*nbus] = iter_bus->VM;
			rbus[i+8*nbus] = iter_bus->VA;
			rbus[i+9*nbus] = iter_bus->BASE_KV;
			rbus[i+10*nbus] = (double)iter_bus->ZONE;
			rbus[i+11*nbus] = iter_bus->VMAX;
			rbus[i+12*nbus] = iter_bus->VMIN;
			iter_bus++;
		}
	}

	
	// insert data for rgen
	vector<gen>::iterator iter_gen = vec_gen.begin();
	unsigned int max_order = 0;
	for (unsigned int i =0; i< ngen; i++)
	{
		if (gen_NCOST[i] > max_order)
			max_order = gen_NCOST[i];
		iter_gen++;
	}
	rgencost = (double *) calloc(ngen*(GENCOST_ATTR+max_order),sizeof(double));

	iter_gen = vec_gen.begin();

	for (unsigned int i = 0; i < ngen; i++)
	{
		//rgen[i+0*ngen] = (double) iter_gen->GEN_BUS;
		rgen[i+0*ngen] = gen_GEN_BUS[i];
		rgen[i+1*ngen] = iter_gen->PG;
		rgen[i+2*ngen] = iter_gen->QG;
		rgen[i+3*ngen] = iter_gen->QMAX;
		rgen[i+4*ngen] = iter_gen->QMIN;
		rgen[i+5*ngen] = iter_gen->VG;
		rgen[i+6*ngen] = iter_gen->MBASE;
		rgen[i+7*ngen] = iter_gen->GEN_STATUS;
		rgen[i+8*ngen] = iter_gen->PMAX;
		rgen[i+9*ngen] = iter_gen->PMIN;
		rgen[i+10*ngen] = iter_gen->PC1;
		rgen[i+11*ngen] = iter_gen->PC2;
		rgen[i+12*ngen] = iter_gen->QC1MIN;
		rgen[i+13*ngen] = iter_gen->QC1MAX;
		rgen[i+14*ngen] = iter_gen->QC2MIN;
		rgen[i+15*ngen] = iter_gen->QC2MAX;
		rgen[i+16*ngen] = iter_gen->RAMP_AGC;
		rgen[i+17*ngen] = iter_gen->RAMP_10;
		rgen[i+18*ngen] = iter_gen->RAMP_30;
		rgen[i+19*ngen] = iter_gen->RAMP_Q;
		rgen[i+20*ngen] = iter_gen->APF;

		// Cost info
		rgencost[i+0*ngen] = iter_gen->MODEL;
		rgencost[i+1*ngen] = iter_gen->STARTUP;
		rgencost[i+2*ngen] = iter_gen->SHUTDOWN;
		//rgencost[i+3*ngen] = (double)iter_gen->NCOST;
		rgencost[i+3*ngen] = gen_NCOST[i];
		string double_string(iter_gen->COST);
		vector<string> v;
		v = split(double_string,',');
		for (unsigned int j=0; j<v.size();j++)
		{
			rgencost[i+(4+j)*ngen] = atof(v[j].c_str());
		}
		if (gen_NCOST[i] != max_order)
		{
			for (unsigned int j = gen_NCOST[i]; j< max_order; j++)
			{
				rgencost[i+(4+j)*ngen] = 0.0;
			}
		}

		iter_gen++;
	}	



	// insert data for rbranch
	vector<branch>::iterator iter_branch = vec_branch.begin();
	for (unsigned int i = 0; i < nbranch; i++)
	{
		//rbranch[i+0*nbranch] = (double)iter_branch->F_BUS;
		rbranch[i+0*nbranch] = branch_F_BUS[i];
		//rbranch[i+1*nbranch] = (double)iter_branch->T_BUS;
		rbranch[i+1*nbranch] = branch_T_BUS[i];
		rbranch[i+2*nbranch] = iter_branch->BR_R;
		rbranch[i+3*nbranch] = iter_branch->BR_X;
		rbranch[i+4*nbranch] = iter_branch->BR_B;
		rbranch[i+5*nbranch] = iter_branch->RATE_A;
		rbranch[i+6*nbranch] = iter_branch->RATE_B;		
		rbranch[i+7*nbranch] = iter_branch->RATE_C;
		rbranch[i+8*nbranch] = iter_branch->TAP;
		rbranch[i+9*nbranch] = iter_branch->SHIFT;
		rbranch[i+10*nbranch] = (double)iter_branch->BR_STATUS;
		rbranch[i+11*nbranch] = iter_branch->ANGMIN;
		rbranch[i+12*nbranch] = iter_branch->ANGMAX;
		iter_branch++;
	}

	
	// insert data for rareas
	//vector<areas>::const_iterator iter_areas = vec_areas.begin();
	//for (unsigned int i = 0; i < nareas; i++)
	//{
		rareas[0] = 1;
		rareas[1] = 1;
	//	iter_areas++;
	//} 

	// insert data for rbaseMVA
	//vector<baseMVA>::const_iterator iter_baseMVA = vec_baseMVA.begin();
	//rbaseMVA = iter_baseMVA->BASEMVA;
	rbaseMVA = BASEMVA;

	// insert data for rgencost
	/*
	vector<gen_cost>::const_iterator iter_gencost = vec_gencost.begin();

	unsigned int max_order = 0;
	for (unsigned int i = 0; i<ngencost; i++)
	{
		if (iter_gencost->NCOST > max_order)
			max_order = iter_gencost->NCOST;
		iter_gencost++;
		
	}
	
	rgencost = (double *) calloc(ngencost*(GENCOST_ATTR+max_order),sizeof(double));
	
	iter_gencost = vec_gencost.begin();
	for (unsigned int i = 0; i<ngencost; i++)
	{
		// Only support model 2: ticket 4
		if (iter_gencost -> MODEL != 2)
			GL_THROW("Unsupported model for generation cost\n");

		rgencost[i+0*ngencost] = iter_gencost->MODEL;
		rgencost[i+1*ngencost] = iter_gencost->STARTUP;
		rgencost[i+2*ngencost] = iter_gencost->SHUTDOWN;
		rgencost[i+3*ngencost] = (double)iter_gencost->NCOST;
		string double_string(iter_gencost->COST);
		vector<string> v;
		v = split(double_string,',');
		for (unsigned int j = 0; j<v.size();j++)
		{
			rgencost[i+(4+j)*ngencost] = atof(v[j].c_str());
		}
		if (iter_gencost->NCOST != max_order)
		{
			for (unsigned int j = iter_gencost->NCOST; j < max_order; j++)
				rgencost[i+(4+j)*ngencost] = 0.0;
		}
		iter_gencost++;
	}
	*/



	// Run the Solver function
	//printf("Running Test\n");
        libopfInitialize();
	//mxArray* basemva = initArray(rbaseMVA,nbaseMVA,BASEMVA_ATTR);
	mxArray* basemva_array = mxCreateDoubleMatrix(1,1,mxREAL);
	*mxGetPr(basemva_array) = rbaseMVA;

	// change to MATLAB MAT format
	mxArray* bus_array = initArray(rbus, nbus, BUS_ATTR);	
	mxArray* gen_array = initArray(rgen, ngen, GEN_ATTR);
	mxArray* branch_array = initArray(rbranch, nbranch, BRANCH_ATTR);
	mxArray* gencost_array = initArray(rgencost, ngen, GENCOST_ATTR+max_order);
	//mxArray* areas_array = initArray(rareas, nareas, AREA_ATTR);
	mxArray* areas_array = initArray(rareas, 1, AREA_ATTR);

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

	prhs[0] = basemva_array;
	prhs[1] = bus_array;
	prhs[2] = gen_array;
	prhs[3] = branch_array;
	prhs[4] = areas_array;
	prhs[5] = gencost_array;

	mlxOpf(5, plhs, 6, prhs); // cout if first parameter is 0;
	//mlxOpf(0,plhs,6,prhs);


	// Get data from array
	double *obus = getArray(plhs[0]);
	double *ogen = getArray(plhs[1]);
	double *obranch = getArray(plhs[2]);

	// Update class bus

	temp_obj = NULL;
	for (unsigned int i=0; i < nbus; i++)
	{
		/*		
		iter_bus->PD = obus[i+2*nbus];
		iter_bus->QD = obus[i+3*nbus];
		iter_bus->GS = obus[i+4*nbus];
		iter_bus->BS = obus[i+5*nbus];
		iter_bus->VM = obus[i+7*nbus];		
		iter_bus->VA = obus[i+8*nbus];
		iter_bus->VMAX = obus[i+11*nbus];
		iter_bus->VMIN = obus[i+12*nbus];
		iter_bus->LAM_P = obus[i+13*nbus];
		iter_bus->LAM_Q = obus[i+14*nbus];
		iter_bus->MU_VMAX = obus[i+15*nbus];
		iter_bus->MU_VMIN = obus[i+16*nbus];
		//iter_bus++;
		*/
		//printf("====Before Test part VM %f; %f\n",iter_bus->VM,obus[i+7*nbus]);
		
		temp_obj = gl_find_next(bus_list,temp_obj);

		
		//Matpower does not overwrite the generator power in bus class
		setObjectValue_Double(temp_obj,"PD",obus[i+2*nbus]);
		setObjectValue_Double(temp_obj,"QD",obus[i+3*nbus]);
		setObjectValue_Double(temp_obj,"GS",obus[i+4*nbus]);
		setObjectValue_Double(temp_obj,"BS",obus[i+5*nbus]);
		setObjectValue_Double(temp_obj,"VM",obus[i+7*nbus]);
		setObjectValue_Double(temp_obj,"VA",obus[i+8*nbus]);
		setObjectValue_Double(temp_obj,"VMAX",obus[i+11*nbus]);
		setObjectValue_Double(temp_obj,"VMIN",obus[i+12*nbus]);
		setObjectValue_Double(temp_obj,"LAM_P",obus[i+13*nbus]);
		setObjectValue_Double(temp_obj,"LAM_Q",obus[i+14*nbus]);
		setObjectValue_Double(temp_obj,"MU_VMAX",obus[i+15*nbus]);
		setObjectValue_Double(temp_obj,"MU_VMIN",obus[i+16*nbus]);

		// obus[i+9*nbus] is BASE_KV. The unit is KV.
		setObjectValue_Double2Complex_inDegree(temp_obj,"CVoltageA",obus[i+7*nbus]*obus[i+9*nbus]*1000,obus[i+8*nbus]);
		setObjectValue_Double2Complex_inDegree(temp_obj,"CVoltageB",obus[i+7*nbus]*obus[i+9*nbus]*1000,obus[i+8*nbus]+2/3*PI);
		setObjectValue_Double2Complex_inDegree(temp_obj,"CVoltageC",obus[i+7*nbus]*obus[i+9*nbus]*1000,obus[i+8*nbus]-2/3*PI);
		setObjectValue_Double(temp_obj,"V_nom",obus[i+7*nbus]*obus[i+9*nbus]*1000);
		
		//printf("BUS: %f LAM_P %f\n",obus[i+0*nbus],obus[i+13*nbus]);
		//cout<<"BUS "<<obus[i+0*nbus]<<"LAM_P "<<obus[i+13*nbus]<<endl;
	}
/*
	unsigned int NumOfElement = mxGetNumberOfElements(plhs[0]);
	for (unsigned int i =0; i<NumOfElement; i++)
	{
		printf("%f ",obus[i]);
		if ((i+1)%nbus == 0)
			printf("\n");
	}
	

	printf("========================\n");

	iter_bus = vec_bus.begin();
	for (unsigned int i=0;i< nbus; i++)
	{
		printf("Bus %d; PD %f; QD %f; VM %f; VA %f;\n",iter_bus->BUS_I,iter_bus->PD,iter_bus->QD,iter_bus->VM,iter_bus->VA);
		iter_bus++;
	}
*/	
	// Update class gen
	
	iter_gen = vec_gen.begin();
	temp_obj = NULL;
	for (unsigned int i = 0; i < ngen; i++)
	{
/*		
		iter_gen->PG = ogen[i+1*ngen];
		iter_gen->QG = ogen[i+2*ngen];
		iter_gen->QMAX = ogen[i+3*ngen];
		iter_gen->QMIN = ogen[i+4*ngen];
		iter_gen->VG = ogen[i+5*ngen];
		iter_gen->PC1 = ogen[i+10*ngen];
		iter_gen->PC2 = ogen[i+11*ngen];
		iter_gen->RAMP_AGC = ogen[i+16*ngen];
		iter_gen->RAMP_10 = ogen[i+17*ngen];
		iter_gen->RAMP_30 = ogen[i+18*ngen];
		iter_gen->RAMP_Q = ogen[i+19*ngen];
		iter_gen->APF = ogen[i+20*ngen];
		iter_gen->MU_PMAX = ogen[i+21*ngen];
		iter_gen->MU_PMIN = ogen[i+22*ngen];
		iter_gen->MU_QMAX = ogen[i+23*ngen];
		iter_gen->MU_QMIN = ogen[i+24*ngen];
		iter_gen++;
*/
		temp_obj = gl_find_next(gen_list,temp_obj);
		setObjectValue_Double(temp_obj,"PG",ogen[i+1*ngen]);
		setObjectValue_Double(temp_obj,"QG",ogen[i+2*ngen]);
		setObjectValue_Double(temp_obj,"QMAX",ogen[i+3*ngen]);
		setObjectValue_Double(temp_obj,"QMIN",ogen[i+4*ngen]);
		setObjectValue_Double(temp_obj,"VG",ogen[i+5*ngen]);
		setObjectValue_Double(temp_obj,"PC1",ogen[i+10*ngen]);
		setObjectValue_Double(temp_obj,"PC2",ogen[i+11*ngen]);
		setObjectValue_Double(temp_obj,"RAMP_AGC",ogen[i+16*ngen]);
		setObjectValue_Double(temp_obj,"RAMP_10",ogen[i+17*ngen]);
		setObjectValue_Double(temp_obj,"RAMP_30",ogen[i+18*ngen]);
		setObjectValue_Double(temp_obj,"RAMP_Q",ogen[i+19*ngen]);
		setObjectValue_Double(temp_obj,"APF",ogen[i+20*ngen]);
		setObjectValue_Double(temp_obj,"MU_PMAX",ogen[i+21*ngen]);
		setObjectValue_Double(temp_obj,"MU_PMIN",ogen[i+22*ngen]);
		setObjectValue_Double(temp_obj,"MU_QMAX",ogen[i+23*ngen]);
		setObjectValue_Double(temp_obj,"MU_QMIN",ogen[i+24*ngen]);



		// Calculate Price
		double price = 0;
		unsigned int NCOST = (unsigned int)rgencost[i+3*ngen];
		//printf("Bus %d, order %d ",i,NCOST);
		for (unsigned int j = 0; j < NCOST; j++)
		{
			price += pow(ogen[i+1*ngen],NCOST-1-j)*rgencost[i+(4+j)*ngen];
			//printf("Coeff %d: %f and price %f",j,rgencost[i+(4+j)*ngencost],price);
		}

		setObjectValue_Double(temp_obj,"Price",price);
		//printf("\nBus %d, Price %f\n",i,price);
		
		iter_gen++;
	}

	// Update class branch	
	
	
	//iter_branch = vec_branch.begin();
	temp_obj = NULL;
	for (unsigned int i = 0; i<nbranch; i++)
	{
/*
		iter_branch->PF = obranch[i+13*nbranch];
		iter_branch->QF = obranch[i+14*nbranch];
		iter_branch->PT = obranch[i+15*nbranch];
		iter_branch->QT = obranch[i+16*nbranch];
		iter_branch->MU_SF = obranch[i+17*nbranch];
		iter_branch->MU_ST = obranch[i+18*nbranch];
		iter_branch->MU_ANGMIN = obranch[i+19*nbranch];
		iter_branch->MU_ANGMAX = obranch[i+20*nbranch];
		iter_branch++;
*/

		temp_obj = gl_find_next(branch_list,temp_obj);
		setObjectValue_Double(temp_obj,"PF",obranch[i+13*nbranch]);
		setObjectValue_Double(temp_obj,"QF",obranch[i+14*nbranch]);
		setObjectValue_Double(temp_obj,"PT",obranch[i+15*nbranch]);
		setObjectValue_Double(temp_obj,"QT",obranch[i+16*nbranch]);
		setObjectValue_Double(temp_obj,"MU_SF",obranch[i+17*nbranch]);
		setObjectValue_Double(temp_obj,"MU_ST",obranch[i+18*nbranch]);
		setObjectValue_Double(temp_obj,"MU_ANGMIN",obranch[i+19*nbranch]);
		setObjectValue_Double(temp_obj,"MU_ANGMAX",obranch[i+20*nbranch]);
	}
	
	// free space
	//printf("Free r..\n");
	free(rbus);
	free(rgen);
	free(rbranch);
	free(rareas);
	//free(rbaseMVA);
	free(rgencost);
	
	//printf("Free o..\n");	
	free(obus);
	free(ogen);
	free(obranch);

	vec_bus.clear();
	vec_gen.clear();
	vec_branch.clear();
	//vec_gencost.clear();
	//vec_baseMVA.clear();


	short ifsuccess = (short)*getArray(plhs[3]);
	//printf("suceess %f\n",*getArray(plhs[4]));
	return ifsuccess;

}

vector<string> split(const string s, char c)
{
	vector<string> v;	
	string::size_type i = 0;
	string::size_type j = s.find(c);
	while (j != string::npos)
	{
		v.push_back(s.substr(i,j-i));
		i = ++j;
		j = s.find(c,j);
		if (j == string::npos)
			v.push_back(s.substr(i,s.length()));
	}
	return v;
}

void setObjectValue_Double(OBJECT* obj, char* Property, double value)
{
	char buffer[1024];
	snprintf(buffer,sizeof(buffer),"%g",value);
	gl_set_value_by_name(obj,Property,buffer);
	
}

void setObjectValue_Double2Complex_inDegree(OBJECT* obj, char* Property, double Mag, double Ang)
{
	complex temp;
	temp.SetPolar(Mag,Ang);	
	char buffer[1024];
	//snprintf(buffer,sizeof(buffer),"+%g+%gj",temp.Re(),temp.Im());
	//gl_set_value_by_name(obj,Property,buffer);
	setObjectValue_Double2Complex(obj,Property,temp.Re(),temp.Im());
}

void setObjectValue_Double2Complex(OBJECT* obj, char* Property, double Re, double Im)
{
	char buffer[1024];
	snprintf(buffer,sizeof(buffer),"%g+%gj",Re,Im);
	gl_set_value_by_name(obj,Property,buffer);
}

void setObjectValue_Complex(OBJECT* obj, char* Property, complex val)
{
	setObjectValue_Double2Complex(obj,Property,val.Re(),val.Im());
}

void setObjectValue_Char(OBJECT* obj, char* Property, char* value)
{
	gl_set_value_by_name(obj,Property,value);
}
