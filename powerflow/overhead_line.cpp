/** $Id: overhead_line.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file overhead_line.cpp
	@addtogroup overhead_line 
	@ingroup line

	@{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "line.h"

CLASS* overhead_line::oclass = NULL;
CLASS* overhead_line::pclass = NULL;

overhead_line::overhead_line(MODULE *mod) : line(mod)
{
	if(oclass == NULL)
	{
		pclass = line::oclass;
		
		oclass = gl_register_class(mod,"overhead_line",sizeof(overhead_line),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class overhead_line";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_INHERIT, "line",
			NULL) < 1) GL_THROW("unable to publish overhead_line properties in %s",__FILE__);
			if (gl_publish_function(oclass,	"create_fault", (FUNCTIONADDR)create_fault_ohline)==NULL)
				GL_THROW("Unable to publish fault creation function");
			if (gl_publish_function(oclass,	"fix_fault", (FUNCTIONADDR)fix_fault_ohline)==NULL)
				GL_THROW("Unable to publish fault restoration function");
    }
}

int overhead_line::create(void)
{
	int result = line::create();

	return result;
}

int overhead_line::init(OBJECT *parent)
{
	line::init(parent);

	if (!configuration)
		throw "no overhead line configuration specified.";
		/*  TROUBLESHOOT
		No overhead line configuration was specified.  Please use object line_configuration
		with appropriate values to specify an overhead line configuration
		*/

	if (!gl_object_isa(configuration, "line_configuration"))
		throw "invalid line configuration for overhead line";
		/*  TROUBLESHOOT
		The object specified as the configuration for the overhead line is not a valid
		configuration object.  Please ensure you have a line_configuration object selected.
		*/

	//Test the phases
	line_configuration *config = OBJECTDATA(configuration, line_configuration);

	test_phases(config,'A');
	test_phases(config,'B');
	test_phases(config,'C');
	test_phases(config,'N');
	
	if ((!config->line_spacing || !gl_object_isa(config->line_spacing, "line_spacing")) && config->impedance11 == 0.0 && config->impedance22 == 0.0 && config->impedance33 == 0.0)
		throw "invalid or missing line spacing on overhead line";
		/*  TROUBLESHOOT
		The configuration object for the overhead line is missing the line_spacing configuration
		or the item specified in that location is invalid.
		*/

	recalc();

	return 1;
}

void overhead_line::recalc(void)
{
	line_configuration *config = OBJECTDATA(configuration, line_configuration);
	double miles = length / 5280.0;

	if (config->impedance11 != 0 || config->impedance22 != 0 || config->impedance33 != 0)
	{
		for (int i = 0; i < 3; i++) 
		{
			for (int j = 0; j < 3; j++) 
			{
				b_mat[i][j] = 0.0;
			}
		}
		// User defined z-matrix - only assign parts of matrix if phase exists
		if (has_phase(PHASE_A))
		{
			b_mat[0][0] = config->impedance11 * miles;
			
			if (has_phase(PHASE_B))
			{
				b_mat[0][1] = config->impedance12 * miles;
				b_mat[1][0] = config->impedance21 * miles;
			}
			if (has_phase(PHASE_C))
			{
				b_mat[0][2] = config->impedance13 * miles;
				b_mat[2][0] = config->impedance31 * miles;
			}
		}
		if (has_phase(PHASE_B))
		{
			b_mat[1][1] = config->impedance22 * miles;
			
			if (has_phase(PHASE_C))
			{
				b_mat[1][2] = config->impedance23 * miles;
				b_mat[2][1] = config->impedance32 * miles;
			}
		
		}
		if (has_phase(PHASE_C))
			b_mat[2][2] = config->impedance33 * miles;
	}
	else
	{
		// Use Kersting's equations to define the z-matrix
		double dab, dbc, dac, dan, dbn, dcn;
		double gmr_a, gmr_b, gmr_c, gmr_n, res_a, res_b, res_c, res_n;
		complex z_aa, z_ab, z_ac, z_an, z_bb, z_bc, z_bn, z_cc, z_cn, z_nn;
		

		#define GMR(ph) (has_phase(PHASE_##ph) && config->phase##ph##_conductor ? \
			OBJECTDATA(config->phase##ph##_conductor, overhead_line_conductor)->geometric_mean_radius : 0.0)
		#define RES(ph) (has_phase(PHASE_##ph) && config->phase##ph##_conductor ? \
			OBJECTDATA(config->phase##ph##_conductor, overhead_line_conductor)->resistance : 0.0)
		#define DIST(ph1, ph2) (has_phase(PHASE_##ph1) && has_phase(PHASE_##ph2) && config->line_spacing ? \
			OBJECTDATA(config->line_spacing, line_spacing)->distance_##ph1##to##ph2 : 0.0)

		gmr_a = GMR(A);
		gmr_b = GMR(B);
		gmr_c = GMR(C);
		gmr_n = GMR(N);
		//res_a = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(A);
		//res_b = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(B);
		//res_c = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(C);
		//res_n = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(N);
		res_a = RES(A);
		res_b = RES(B);
		res_c = RES(C);
		res_n = RES(N);
		dab = DIST(A, B);
		dbc = DIST(B, C);
		dac = DIST(A, C);
		dan = DIST(A, N);
		dbn = DIST(B, N);
		dcn = DIST(C, N);

		#undef GMR
		#undef RES
		#undef DIST

		if (has_phase(PHASE_A)) {
			if (gmr_a > 0.0 && res_a > 0.0)
				z_aa = complex(res_a + 0.0953, 0.12134 * (log(1.0 / gmr_a) + 7.93402));
			else
				z_aa = 0.0;
			if (has_phase(PHASE_B) && dab > 0.0)
				z_ab = complex(0.0953, 0.12134 * (log(1.0 / dab) + 7.93402));
			else
				z_ab = 0.0;
			if (has_phase(PHASE_C) && dac > 0.0)
				z_ac = complex(0.0953, 0.12134 * (log(1.0 / dac) + 7.93402));
			else
				z_ac = 0.0;
			if (has_phase(PHASE_N) && dan > 0.0)
				z_an = complex(0.0953, 0.12134 * (log(1.0 / dan) + 7.93402));
			else
				z_an = 0.0;
		} else {
			z_aa = z_ab = z_ac = z_an = 0.0;
		}

		if (has_phase(PHASE_B)) {
			if (gmr_b > 0.0 && res_b > 0.0)
				z_bb = complex(res_b + 0.0953, 0.12134 * (log(1.0 / gmr_b) + 7.93402));
			else
				z_bb = 0.0;
			if (has_phase(PHASE_C) && dbc > 0.0)
				z_bc = complex(0.0953, 0.12134 * (log(1.0 / dbc) + 7.93402));
			else
				z_bc = 0.0;
			if (has_phase(PHASE_N) && dbn > 0.0)
				z_bn = complex(0.0953, 0.12134 * (log(1.0 / dbn) + 7.93402));
			else
				z_bn = 0.0;
		} else {
			z_bb = z_bc = z_bn = 0.0;
		}

		if (has_phase(PHASE_C)) {
			if (gmr_c > 0.0 && res_c > 0.0)
				z_cc = complex(res_c + 0.0953, 0.12134 * (log(1.0 / gmr_c) + 7.93402));
			else
				z_cc = 0.0;
			if (has_phase(PHASE_N) && dcn > 0.0)
				z_cn = complex(0.0953, 0.12134 * (log(1.0 / dcn) + 7.93402));
			else
				z_cn = 0.0;
		} else {
			z_cc = z_cn = 0.0;
		}

		complex z_nn_inv = 0;
		if (has_phase(PHASE_N) && gmr_n > 0.0 && res_n > 0.0){
			z_nn = complex(res_n + 0.0953, 0.12134 * (log(1.0 / gmr_n) + 7.93402));
			z_nn_inv = z_nn^(-1.0);
		}
		else
			z_nn = 0.0;

		b_mat[0][0] = (z_aa - z_an * z_an * z_nn_inv) * miles;
		b_mat[0][1] = (z_ab - z_an * z_bn * z_nn_inv) * miles;
		b_mat[0][2] = (z_ac - z_an * z_cn * z_nn_inv) * miles;
		b_mat[1][0] = (z_ab - z_bn * z_an * z_nn_inv) * miles;
		b_mat[1][1] = (z_bb - z_bn * z_bn * z_nn_inv) * miles;
		b_mat[1][2] = (z_bc - z_bn * z_cn * z_nn_inv) * miles;
		b_mat[2][0] = (z_ac - z_cn * z_an * z_nn_inv) * miles;
		b_mat[2][1] = (z_bc - z_cn * z_bn * z_nn_inv) * miles;
		b_mat[2][2] = (z_cc - z_cn * z_cn * z_nn_inv) * miles;
	}

	// This part is the same in both methods.
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			a_mat[i][j] = d_mat[i][j] = A_mat[i][j] = (i == j ? 1.0 : 0.0);
			c_mat[i][j] = 0.0;
			B_mat[i][j] = b_mat[i][j];
		}
	}

#ifdef _TESTING
	if (show_matrix_values)
	{
		OBJECT *obj = GETOBJECT(this);

		gl_testmsg("overhead_line: %d a matrix",obj->id);
		print_matrix(a_mat);

		gl_testmsg("overhead_line: %d A matrix",obj->id);
		print_matrix(A_mat);

		gl_testmsg("overhead_line: %d b matrix",obj->id);
		print_matrix(b_mat);

		gl_testmsg("overhead_line: %d B matrix",obj->id);
		print_matrix(B_mat);

		gl_testmsg("overhead_line: %d c matrix",obj->id);
		print_matrix(c_mat);

		gl_testmsg("overhead_line: %d d matrix",obj->id);
		print_matrix(d_mat);
	}
#endif
}

int overhead_line::isa(char *classname)
{
	return strcmp(classname,"overhead_line")==0 || line::isa(classname);
}

/**
* test_phases is called to ensure all necessary conductors are included in the
* configuration object and are of the proper type.
*
* @param config the line configuration object
* @param ph the phase to check
*/
void overhead_line::test_phases(line_configuration *config, const char ph)
{
	bool condCheck, condNotPres;
	OBJECT *obj = GETOBJECT(this);

	if (ph=='A')
	{
		if (config->impedance11 == 0.0)
		{
			condCheck = (config->phaseA_conductor && !gl_object_isa(config->phaseA_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseA_conductor) && has_phase(PHASE_A));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	else if (ph=='B')
	{
		if (config->impedance22 == 0.0)
		{
			condCheck = (config->phaseB_conductor && !gl_object_isa(config->phaseB_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseB_conductor) && has_phase(PHASE_B));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	else if (ph=='C')
	{
		if (config->impedance33 == 0.0)
		{
			condCheck = (config->phaseC_conductor && !gl_object_isa(config->phaseC_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseC_conductor) && has_phase(PHASE_C));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	else if (ph=='N')
	{
		if (config->impedance11 == 0.0 && config->impedance22 == 0.0 && config->impedance33 == 0.0)
		{
			condCheck = (config->phaseN_conductor && !gl_object_isa(config->phaseN_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseN_conductor) && has_phase(PHASE_N));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	//Nothing else down here.  Should never get anything besides ABCN to check

	if (condCheck==true)
		GL_THROW("invalid conductor for phase %c of overhead line %s",ph,obj->name);
		/*	TROUBLESHOOT  The conductor specified for the indicated phase is not necessarily an overhead line conductor, it may be an underground or triplex-line only conductor */

	if (condNotPres==true)
		GL_THROW("missing conductor for phase %c of overhead line %s",ph,obj->name);
		/*  TROUBLESHOOT
		The conductor specified for the indicated phase for the overhead line is missing
		or invalid.
		*/
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: overhead_line
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/


/* This can be added back in after tape has been moved to commit

EXPORT TIMESTAMP commit_overhead_line(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	if ((solver_method==SM_FBS) || (solver_method==SM_NR))
	{
		overhead_line *plink = OBJECTDATA(obj,overhead_line);
		plink->calculate_power();
	}
	return TS_NEVER;
}
*/
EXPORT int create_overhead_line(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(overhead_line::oclass);
		if (*obj!=NULL)
		{
			overhead_line *my = OBJECTDATA(*obj,overhead_line);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(overhead_line);
}

EXPORT TIMESTAMP sync_overhead_line(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		overhead_line *pObj = OBJECTDATA(obj,overhead_line);
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
	} 
	SYNC_CATCHALL(overhead_line);
}

EXPORT int init_overhead_line(OBJECT *obj)
{
	try {
		overhead_line *my = OBJECTDATA(obj,overhead_line);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(overhead_line);
}

EXPORT int isa_overhead_line(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,line)->isa(classname);
}

EXPORT int recalc_overhead_line(OBJECT *obj)
{
	OBJECTDATA(obj,overhead_line)->recalc();
	return 1;
}
EXPORT int create_fault_ohline(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data)
{
	int retval;

	//Link to ourselves
	overhead_line *thisline = OBJECTDATA(thisobj,overhead_line);

	//Try to fault up
	retval = thisline->link_fault_on(protect_obj, fault_type, implemented_fault, repair_time, Extra_Data);

	return retval;
}
EXPORT int fix_fault_ohline(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void* Extra_Data)
{
	int retval;

	//Link to ourselves
	overhead_line *thisline = OBJECTDATA(thisobj,overhead_line);

	//Clear the fault
	retval = thisline->link_fault_off(implemented_fault, imp_fault_name, Extra_Data);
	
	//Clear the fault type
	*implemented_fault = -1;

	return retval;
}
/**@}**/
