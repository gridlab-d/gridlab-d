// $Id: regulator.cpp 1186 2009-01-02 18:15:30Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file regulator.cpp
	@addtogroup powerflow_regulator Regulator
	@ingroup powerflow
		
	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "regulator.h"
#include "node.h"

CLASS* regulator::oclass = NULL;
CLASS* regulator::pclass = NULL;

regulator::regulator(MODULE *mod) : link(mod)
{
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = link::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"regulator",sizeof(regulator),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_object,"configuration",PADDR(configuration),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int regulator::isa(char *classname)
{
	return strcmp(classname,"regulator")==0 || link::isa(classname);
}

int regulator::create()
{
	int result = link::create();
	configuration = NULL;
	return result;
}

int regulator::init(OBJECT *parent)
{
	int result = link::init(parent);

	if (!configuration)
		throw "no regulator configuration specified.";

	if (!gl_object_isa(configuration, "regulator_configuration"))
		throw "invalid regulator configuration";

	regulator_configuration *pConfig = OBJECTDATA(configuration, regulator_configuration);
	node *pTo = OBJECTDATA(to, node);

	// D_mat - 3x3 matrix, 'D' matrix
	complex D_mat[3][3] = {{complex(1,0),complex(-1,0),complex(0,0)},
	                       {complex(0,0), complex(1,0),complex(-1,0)},
					       {complex(-1,0), complex(0,0), complex(1,0)}};   
	complex W_mat[3][3] = {{complex(2,0),complex(1,0),complex(0,0)},
	                       {complex(0,0),complex(2,0),complex(1,0)},
	                       {complex(1,0),complex(0,0),complex(2,0)}};
	multiply(1.0/3.0,W_mat,W_mat);
	
	complex volt[3] = {0.0, 0.0, 0.0};
	double tapChangePer = pConfig->regulation / (double) pConfig->raise_taps;
	double VtapChange = (pConfig->V_high / ((double) pConfig->PT_ratio * sqrt(3.0))) * tapChangePer;
	double Vlow = pConfig->band_center - pConfig->band_width / 2.0;
	double Vhigh = pConfig->band_center + pConfig->band_width / 2.0;
	complex V2[3], Vcomp[3];

	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			a_mat[i][j] = b_mat[i][j] = c_mat[i][j] = d_mat[i][j] =
					A_mat[i][j] = B_mat[i][j] = 0.0;
		}
	}

	if (pTo) 
	{
		volt[0] = pTo->voltageA;
		volt[1] = pTo->voltageB;
		volt[2] = pTo->voltageC;
	}
	else
		volt[0] = volt[1] = volt[2] = 0.0;

	for (int i = 0; i < 3; i++) 
	{
		V2[i] = volt[i] / ((double) pConfig->PT_ratio * sqrt(3.0));
		complex tI = volt[i] / B_mat[i][i];
		Vcomp[i] = V2[i] - (tI / (double) pConfig->CT_ratio) * complex(pConfig->ldc_R_V, pConfig->ldc_X_V);
		a_mat[i][i] = 1.0 - pConfig->tap_pos[i] * tapChangePer;
	}

	complex tmp_mat[3][3] = {{complex(1,0)/a_mat[0][0],complex(0,0),complex(0,0)},
			                 {complex(0,0), complex(1,0)/a_mat[1][1],complex(0,0)},
			                 {complex(-1,0)/a_mat[0][0],complex(-1,0)/a_mat[1][1],complex(0,0)}};
	complex tmp_mat1[3][3];

	switch (pConfig->connect_type) {
		case regulator_configuration::WYE_WYE:
			for (int i = 0; i < 3; i++)
				d_mat[i][i] = complex(1.0,0) / a_mat[i][i]; 
			inverse(a_mat,A_mat);
			break;
		case regulator_configuration::OPEN_DELTA_ABBC:
			d_mat[0][0] = complex(1,0) / a_mat[0][0];
			d_mat[1][0] = complex(-1,0) / a_mat[0][0];
			d_mat[1][2] = complex(-1,0) / a_mat[1][1];
			d_mat[2][2] = complex(1,0) / a_mat[1][1];

			a_mat[2][0] = -a_mat[0][0];
			a_mat[2][1] = -a_mat[1][1];
			a_mat[2][2] = 0;

			multiply(W_mat,tmp_mat,tmp_mat1);
			multiply(tmp_mat1,D_mat,A_mat);
			break;
		case regulator_configuration::OPEN_DELTA_BCAC:
			break;
		case regulator_configuration::OPEN_DELTA_CABA:
			break;
		case regulator_configuration::CLOSED_DELTA:
			break;
		default:
			throw "unknown regulator connect type";
			break;
	}
	return result;
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: regulator
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_regulator(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(regulator::oclass);
		if (*obj!=NULL)
		{
			regulator *my = OBJECTDATA(*obj,regulator);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_regulator: %s", msg);
	}
	return 0;
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_regulator(OBJECT *obj)
{
	regulator *my = OBJECTDATA(obj,regulator);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (regulator:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_regulator(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	regulator *pObj = OBJECTDATA(obj,regulator);
	try {
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	} catch (const char *error) {
		GL_THROW("%s (regulator:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (regulator:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

EXPORT int isa_regulator(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,regulator)->isa(classname);
}



/**@}*/
