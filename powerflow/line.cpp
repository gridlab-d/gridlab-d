/** $Id: line.cpp 1186 2009-01-02 18:15:30Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file line.cpp
	@addtogroup line 
	@ingroup powerflow

	This file contains the definition for 4 types of lines, their configurations
	and the conductors associated with these lines.  The line is one of the major
	parts of the method used for solving a powerflow network.  In essense the
	distribution network can be seen as a series of nodes and links.  The links
	(or lines as in this file) do most of the work, solving a series of 3
	dimensional matrices, while the nodes (defined in node.cpp) are containers
	and aggregators, storing the values to be used in the equation solvers.
	
	In general, a line contains a configuration that contains a spacing object, and
	one or more conductor objects, exported as phaseA_conductor, phaseB_conductor, 
	phaseC_conductor, and phaseN_conductor.  The line spacing object hold 
	information about the distances between each of the phase conductors, while the 
	conductor objects contain the information on the specifics of the conductor 
	(resistance, diameter, etc...)
	
	Line itself is not exported, and cannot be used in a model.  Instead one of 3
	types of lines are exported for use in models; underground lines, overhead lines,
	and triplex (or secondary) lines.  Underground lines and overhead lines use the
	basic line configuration and line spacing objects, but have different conductor
	classes defined for them.  The overhead line conductor is the simplest of these
	with just a resistance and a geometric mean radius defined.  The underground
	line conductor is more complex, due to the fact that each phase conductor may
	have its own neutral.  
	
	Triplex lines are the third type of line exported.  A special triplex line
	configuration is also exported which presents 3 line conductors; l1_conductor,
	l2_conductor, and lN_conductor.  Also exported is a general insulation 
	thickness and the total diameter of the line.  The conductor for triplex lines
	is much the same as the overhead line conductor.
	
	All line objects export a phase property that is a set of phases and may be 
	set using the bitwise or operator (A|B|C for a 3 phase line).
	
	@{
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
using namespace std;

#include "line.h"

CLASS* line::oclass = nullptr;
CLASS* line::pclass = nullptr;
CLASS *line_class = nullptr;

line::line(MODULE *mod) : link_object(mod) {
	if(oclass == nullptr)
	{
		pclass = link_object::oclass;
		
		line_class = oclass = gl_register_class(mod,"line",sizeof(line),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class line";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_object, "configuration",PADDR(configuration),
			PT_double, "length[ft]",PADDR(length),
			NULL) < 1) GL_THROW("unable to publish line properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==nullptr)
			GL_THROW("Unable to publish line deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==nullptr)
			GL_THROW("Unable to publish line external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==nullptr)
			GL_THROW("Unable to publish line external power limit calculation function");
		if (gl_publish_function(oclass,	"perform_current_calculation_pwr_link", (FUNCTIONADDR)currentcalculation_link)==nullptr)
			GL_THROW("Unable to publish line external current calculation function");
	}
}

int line::create()
{
	int result = link_object::create();

	configuration = nullptr;
	length = 0;

	return result;
}

int line::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	gld_property *fNode_nominal, *tNode_nominal;
	double f_nominal_voltage, t_nominal_voltage;
	gld::complex Zabc_mat_temp[3][3], Yabc_mat_temp[3][3];

	int result = link_object::init(parent);

	//Check for deferred
	if (result == 2)
		return 2;	//Return the deferment - no sense doing everything else!

	//Map the nodes nominal_voltage values
	fNode_nominal = new gld_property(from,"nominal_voltage");

	//Check it
	if (!fNode_nominal->is_valid() || !fNode_nominal->is_double())
	{
		GL_THROW("line:%d - %s - Unable to map nominal_voltage from connected node!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBESHOOT
		While attempting to map the nominal_voltage property of the from or to node on a line, an error occurred.
		Please try again.  If the error persists, please submit your code and a bug report via the issue tracking system.
		*/
	}

	//Get the other one
	tNode_nominal = new gld_property(to,"nominal_voltage");

	//Check it
	if (!tNode_nominal->is_valid() || !tNode_nominal->is_double())
	{
		GL_THROW("line:%d - %s - Unable to map nominal_voltage from connected node!",obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	//Pull the values
	f_nominal_voltage = fNode_nominal->get_double();
	t_nominal_voltage = tNode_nominal->get_double();

	//Remove the property pointers, since they are no longer needed
	delete fNode_nominal;
	delete tNode_nominal;

	/* check for node nominal voltage mismatch */
	if (fabs(f_nominal_voltage - t_nominal_voltage) > (0.001*f_nominal_voltage))
		throw "from and to node nominal voltage mismatch of greater than 0.1%%";

	if (solver_method == SM_NR && length == 0.0)
		throw "Newton-Raphson method does not support zero length lines at this time";
	
	//Now see if we are truly "just a line" (not overhead or anything) - if so, do a recalc for us
	if (strcmp(obj->oclass->name,"line")==0)
	{
		gl_warning("line:%d - %s - use of an overhead_line, underground_line, or other line-based subclass is highly recommended!",obj->id,(obj->name?obj->name:"Unnamed"));
		/*  TROUBLESHOOT
		Users are encouraged to use one of the specific sub-classes of line objects, not line itself.  Functionality with line objects is limited and mostly handled
		by their appropriate subclasses (e.g., overhead_line, underground_line, triplex_line).
		*/

		//Make sure a configuration was actually specified
		if ((configuration == nullptr) || (!gl_object_isa(configuration,"line_configuration","powerflow")))
		{
			GL_THROW("line:%d - %s - configuration object either doesn't exist, or is not a valid configuration object!",obj->id,(obj->name?obj->name:"Unnamed"));
			/*  TROUBLESHOOT
			The configuration field for the line object is either empty, or does not contain a line_configuration object.  Please fix this and try again.
			*/
		}

		//Perform the calculation assuming we're matrix-based (because we basically have to be)
		load_matrix_based_configuration(Zabc_mat_temp, Yabc_mat_temp);

		//Now push it
		recalc_line_matricies(Zabc_mat_temp, Yabc_mat_temp);
	}

	return result;
}

int line::isa(char *classname)
{
	return strcmp(classname,"line")==0 || link_object::isa(classname);
}

void line::load_matrix_based_configuration(gld::complex Zabc_mat[3][3], gld::complex Yabc_mat[3][3])
{
	line_configuration *config = OBJECTDATA(configuration, line_configuration);
	double miles;
	gld::complex cap_freq_mult;

	// ft -> miles as z-matrix and c-matrix values are specified in Ohms / nF per mile.
	miles = length / 5280.0;

	//Set capacitor frequency/distance/scaling factor (rad/s*S)
	//scale factor allows for the fact that the internal representation of the C matrix
	//is specified in nF/mile and converts back to Siemens
	cap_freq_mult = gld::complex(0,(2.0*PI*nominal_frequency*0.000000001*miles));

	// Setting Zabc_mat based on the z-matrix configuration parameters
	// Setting Yabc_mat based on the c-matrix configuration parameters and the application of Kersting (5.15)

	// User defined z-matrix and c-matrix - only assign parts of matrix if phase exists
	if (has_phase(PHASE_A))
	{
		Zabc_mat[0][0] = config->impedance11 * miles;
		Yabc_mat[0][0] = cap_freq_mult * config->capacitance11;
		
		if (has_phase(PHASE_B))
		{
			Zabc_mat[0][1] = config->impedance12 * miles;
			Zabc_mat[1][0] = config->impedance21 * miles;
			Yabc_mat[0][1] = cap_freq_mult * config->capacitance12;
			Yabc_mat[1][0] = cap_freq_mult * config->capacitance21;
		}
		if (has_phase(PHASE_C))
		{
			Zabc_mat[0][2] = config->impedance13 * miles;
			Zabc_mat[2][0] = config->impedance31 * miles;
			Yabc_mat[0][2] = cap_freq_mult * config->capacitance13;
			Yabc_mat[2][0] = cap_freq_mult * config->capacitance31;
		}
	}
	if (has_phase(PHASE_B))
	{
		Zabc_mat[1][1] = config->impedance22 * miles;
		Yabc_mat[1][1] = cap_freq_mult * config->capacitance22;
		
		if (has_phase(PHASE_C))
		{
			Zabc_mat[1][2] = config->impedance23 * miles;
			Zabc_mat[2][1] = config->impedance32 * miles;
			Yabc_mat[1][2] = cap_freq_mult * config->capacitance23;
			Yabc_mat[2][1] = cap_freq_mult * config->capacitance32;
		}
	
	}
	if (has_phase(PHASE_C))
	{
		Zabc_mat[2][2] = config->impedance33 * miles;
		Yabc_mat[2][2] = cap_freq_mult * config->capacitance33;
	}

	// Make sure use_line_cap is turned on if capacitance values have been specified.  Otherwise we need to warn
	// the user and zero out Yabc_mat as otherwise the powerflow engine will give quirky results.
	if (config->capacitance11 != 0 || config->capacitance22 != 0 || config->capacitance33 != 0)
	{
		if (!use_line_cap)
		{
			gl_warning("Shunt capacitance of line:%s specified without setting powerflow::line_capacitance = TRUE. Shunt capacitance will be ignored.",OBJECTHDR(this)->name);

			for (int i = 0; i < 3; i++) 
			{
				for (int j = 0; j < 3; j++) 
				{
					Yabc_mat[i][j] = 0.0;
				}
			}
		}
	}
}

void line::recalc_line_matricies(gld::complex Zabc_mat[3][3], gld::complex Yabc_mat[3][3])
{
	gld::complex U_mat[3][3], temp_mat[3][3];

	//Do an initial zero
	U_mat[0][0] = U_mat[0][1] = U_mat[0][2] = 0.0;
	U_mat[1][0] = U_mat[1][1] = U_mat[1][2] = 0.0;
	U_mat[2][0] = U_mat[2][1] = U_mat[2][2] = 0.0;

	// Setup unity matrix - by phase
	if (has_phase(PHASE_A))
	{
		U_mat[0][0] = 1.0;
	}

	if (has_phase(PHASE_B))
	{
		U_mat[1][1] = 1.0;
	}

	if (has_phase(PHASE_C))
	{
		U_mat[2][2] = 1.0;
	}

	//b_mat = Zabc_mat as per Kersting (6.10)
		equalm(Zabc_mat,b_mat);

	//a_mat & d_mat as per Kersting (6.9) and (6.18)
		//Zabc*Yabc
		multiply(Zabc_mat,Yabc_mat,temp_mat);

		//0.5*Above - use A_mat temporarily
		multiply(0.5,temp_mat,A_mat);

		//Add unity to make a_mat
		addition(U_mat,A_mat,a_mat);

		//d_mat is the same as a_mat
		equalm(a_mat,d_mat);

	//c_mat as per Kersting (6.17)
		//Zabc*Yabc is temp_mat from above
		//So Yabc*(Zabc*Yabc) - use A_mat again
		multiply(Yabc_mat,temp_mat,A_mat);

		//Multiply by 1/4 - use B_mat temporarily
		multiply(0.25,A_mat,B_mat);

		//Add to Yabc
		addition(Yabc_mat,B_mat,c_mat);

	//A_mat is phase dependent inversion - B_mat is a product associated with it
	//Zero them first
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			A_mat[i][j] = B_mat[i][j] = 0.0;
		}
	}

	//Invert appropriately - A_mat = inv(a_mat) as per Kersting (6.27)
	if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
	{
		//Inverted value
		A_mat[0][0] = gld::complex(1.0) / a_mat[0][0];
	}
	else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //only B
	{
		//Inverted value
		A_mat[1][1] = gld::complex(1.0) / a_mat[1][1];
	}
	else if (!has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //only C
	{
		//Inverted value
		A_mat[2][2] = gld::complex(1.0) / a_mat[2][2];
	}
	else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //has A & C
	{
		gld::complex detvalue = a_mat[0][0]*a_mat[2][2] - a_mat[0][2]*a_mat[2][0];

		//Inverted value
		A_mat[0][0] = a_mat[2][2] / detvalue;
		A_mat[0][2] = a_mat[0][2] * -1.0 / detvalue;
		A_mat[2][0] = a_mat[2][0] * -1.0 / detvalue;
		A_mat[2][2] = a_mat[0][0] / detvalue;
	}
	else if (has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //has A & B
	{
		gld::complex detvalue = a_mat[0][0]*a_mat[1][1] - a_mat[0][1]*a_mat[1][0];

		//Inverted value
		A_mat[0][0] = a_mat[1][1] / detvalue;
		A_mat[0][1] = a_mat[0][1] * -1.0 / detvalue;
		A_mat[1][0] = a_mat[1][0] * -1.0 / detvalue;
		A_mat[1][1] = a_mat[0][0] / detvalue;
	}
	else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C))	//has B & C
	{
		gld::complex detvalue = a_mat[1][1]*a_mat[2][2] - a_mat[1][2]*a_mat[2][1];

		//Inverted value
		A_mat[1][1] = a_mat[2][2] / detvalue;
		A_mat[1][2] = a_mat[1][2] * -1.0 / detvalue;
		A_mat[2][1] = a_mat[2][1] * -1.0 / detvalue;
		A_mat[2][2] = a_mat[1][1] / detvalue;
	}
	else if ((has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C)) || (has_phase(PHASE_D))) //has ABC or D (D=ABC)
	{
		gld::complex detvalue = a_mat[0][0]*a_mat[1][1]*a_mat[2][2] - a_mat[0][0]*a_mat[1][2]*a_mat[2][1] - a_mat[0][1]*a_mat[1][0]*a_mat[2][2] + a_mat[0][1]*a_mat[2][0]*a_mat[1][2] + a_mat[1][0]*a_mat[0][2]*a_mat[2][1] - a_mat[0][2]*a_mat[1][1]*a_mat[2][0];

		//Invert it
		A_mat[0][0] = (a_mat[1][1]*a_mat[2][2] - a_mat[1][2]*a_mat[2][1]) / detvalue;
		A_mat[0][1] = (a_mat[0][2]*a_mat[2][1] - a_mat[0][1]*a_mat[2][2]) / detvalue;
		A_mat[0][2] = (a_mat[0][1]*a_mat[1][2] - a_mat[0][2]*a_mat[1][1]) / detvalue;
		A_mat[1][0] = (a_mat[2][0]*a_mat[1][2] - a_mat[1][0]*a_mat[2][2]) / detvalue;
		A_mat[1][1] = (a_mat[0][0]*a_mat[2][2] - a_mat[0][2]*a_mat[2][0]) / detvalue;
		A_mat[1][2] = (a_mat[1][0]*a_mat[0][2] - a_mat[0][0]*a_mat[1][2]) / detvalue;
		A_mat[2][0] = (a_mat[1][0]*a_mat[2][1] - a_mat[1][1]*a_mat[2][0]) / detvalue;
		A_mat[2][1] = (a_mat[0][1]*a_mat[2][0] - a_mat[0][0]*a_mat[2][1]) / detvalue;
		A_mat[2][2] = (a_mat[0][0]*a_mat[1][1] - a_mat[0][1]*a_mat[1][0]) / detvalue;
	}

	//Now make B_mat value as per Kersting (6.28)
	multiply(A_mat,b_mat,B_mat);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: line
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/



/* This can be added back in after tape has been moved to commit
EXPORT TIMESTAMP commit_line(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	if (solver_method==SM_FBS)
	{
		line *plink = OBJECTDATA(obj,line);
		plink->calculate_power();
	}
	return TS_NEVER;
}
*/
EXPORT int create_line(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(line::oclass);
		if (*obj!=nullptr)
		{
			line *my = OBJECTDATA(*obj,line);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(line);
}

EXPORT TIMESTAMP sync_line(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		line *pObj = OBJECTDATA(obj,line);
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
	SYNC_CATCHALL(line);
}

EXPORT int init_line(OBJECT *obj)
{
	try {
		line *my = OBJECTDATA(obj,line);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(line);
}

EXPORT int isa_line(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,line)->isa(classname);
}

/**@}**/
