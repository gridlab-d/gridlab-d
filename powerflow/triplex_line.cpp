/** $Id: triplex_line.cpp 1186 2009-01-02 18:15:30Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file triplex_line.cpp
	@addtogroup triplex_line 
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

CLASS* triplex_line::oclass = NULL;
CLASS* triplex_line::pclass = NULL;

triplex_line::triplex_line(MODULE *mod) : line(mod)
{
	if(oclass == NULL)
	{
		pclass = line::oclass;
		
		oclass = gl_register_class(mod,"triplex_line",sizeof(triplex_line),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class triplex_line";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_INHERIT, "line",
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==NULL)
			GL_THROW("Unable to publish triplex line deltamode function");
		if (gl_publish_function(oclass,	"recalc_distribution_line", (FUNCTIONADDR)recalc_triplex_line)==NULL)
			GL_THROW("Unable to publish triplex line recalc function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==NULL)
			GL_THROW("Unable to publish triplex line external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==NULL)
			GL_THROW("Unable to publish triplex line external power limit calculation function");
    }
}

int triplex_line::create(void)
{
	int result = line::create();
	return result;
}

int triplex_line::isa(char *classname)
{
	return strcmp(classname,"triplex_line")==0 || line::isa(classname);
}

int triplex_line::init(OBJECT *parent)
{
	double *temp_rating_value = NULL;
	double temp_rating_continuous = 10000.0;
	double temp_rating_emergency = 20000.0;
	char index;
	OBJECT *temp_obj;
	OBJECT *obj = OBJECTHDR(this);
	triplex_line_configuration *temp_config = NULL;
	
	int result = line::init(parent);

	if (!has_phase(PHASE_S))
		gl_warning("%s (%s:%d) is triplex but doesn't have phases S set", obj->name, obj->oclass->name, obj->id);
		/*  TROUBLESHOOT
		A triplex line has been used, but this triplex line does not have the phase S designated to indicate it
		contains single-phase components.  Without this specified, you may get invalid results.
		*/

	//Check phase validity
	phase_conductor_checks();

	recalc();

	//Map the line configuration
	temp_config = OBJECTDATA(configuration,triplex_line_configuration);

	//Values are populated now - populate link ratings parameter
	for (index=0; index<4; index++)
	{
		if (index==0)
		{
			temp_obj = configuration;
		}
		else if (index==1)
		{
			temp_obj = temp_config->phaseA_conductor;
		}
		else if (index==2)
		{
			temp_obj = temp_config->phaseB_conductor;
		}
		else //Must be 2
		{
			temp_obj = temp_config->phaseC_conductor;
		}
		//PHASE_N shouldn't be used

		//See if Phase exists
		if (temp_obj != NULL)
		{
			//Get continuous - summer
			temp_rating_value = get_double(temp_obj,"rating.summer.continuous");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_continuous > *temp_rating_value)
				{
					temp_rating_continuous = *temp_rating_value;
				}
			}

			//Get continuous - winter
			temp_rating_value = get_double(temp_obj,"rating.winter.continuous");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_continuous > *temp_rating_value)
				{
					temp_rating_continuous = *temp_rating_value;
				}
			}

			//Now get emergency - summer
			temp_rating_value = get_double(temp_obj,"rating.summer.emergency");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_emergency > *temp_rating_value)
				{
					temp_rating_emergency = *temp_rating_value;
				}
			}

			//Now get emergency - winter
			temp_rating_value = get_double(temp_obj,"rating.winter.emergency");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_emergency > *temp_rating_value)
				{
					temp_rating_emergency = *temp_rating_value;
				}
			}
			//Populate link array
			if (index < 2)
			{
				link_rating[0][index] = temp_rating_continuous;
				link_rating[1][index] = temp_rating_emergency;
			}
		}//End Phase valid
	}//End FOR

	return result;
}

//Phase checking routine -- make sure triplex components ARE triplex
void triplex_line::phase_conductor_checks(void)
{
	//Map the configuration and object header
	OBJECT *obj = OBJECTHDR(this);
	triplex_line_configuration *line_config = OBJECTDATA(configuration,triplex_line_configuration);

	//See if an impedance matrix is specified first -- if so, skip this
	if ((line_config->impedance11 == 0.0) && (line_config->impedance22 == 0.0))
	{
		//Check all three conductors -- make sure they're triplex
		if (line_config->phaseA_conductor != NULL)	//1
		{
			//Make sure it is a valid conductor
			if (gl_object_isa(line_config->phaseA_conductor,"triplex_line_conductor","powerflow") != true)
			{
				GL_THROW("triplex_line:%d - %s - configuration does not use a triplex_line_conductor for at least one phase!",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				A triplex_line has an object specified for conductor_1, conductor_2, or conductor_N that is not a triplex_line_conductor object.
				Fix this and try again.
				*/
			}
			//Default else -- it's a valid conductor
		}
		else
		{
			GL_THROW("triplex_line:%d - %s - configuration doesn't have a valid phase conductor!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLSHOOT
			A triplex_line's triplex_line_configuration does not have the proper phase specified for the conductor
			 */
		}

		if (line_config->phaseB_conductor != NULL)	//2
		{
			//Make sure it is a valid conductor
			if (gl_object_isa(line_config->phaseB_conductor,"triplex_line_conductor","powerflow") != true)
			{
				GL_THROW("triplex_line:%d - %s - configuration does not use a triplex_line_conductor for at least one phase!",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
			//Default else -- it's a valid conductor
		}
		else
		{
			GL_THROW("triplex_line:%d - %s - configuration doesn't have a valid phase conductor!",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		if (line_config->phaseC_conductor != NULL)	//N
		{
			//Make sure it is a valid conductor
			if (gl_object_isa(line_config->phaseC_conductor,"triplex_line_conductor","powerflow") != true)
			{
				GL_THROW("triplex_line:%d - %s - configuration does not use a triplex_line_conductor for at least one phase!",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
			//Default else -- it's a valid conductor
		}
		else
		{
			GL_THROW("triplex_line:%d - %s - configuration doesn't have a valid phase conductor!",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined above
		}
	}//End not specified as impedance directly
	//Default else -- was an impedance specification, so skip this
}

void triplex_line::recalc(void)
{
	triplex_line_configuration *line_config = OBJECTDATA(configuration,triplex_line_configuration);

	OBJECT *obj = OBJECTHDR(this);
	
	if (line_config->impedance11 != 0.0 || line_config->impedance22 != 0.0)
	{
		gl_warning("Using a 2x2 z-matrix, instead of geometric values, is an under-determined system. Ground and/or neutral currents will be incorrect.");
	
		complex miles = complex(length/5280,0);
		if ((solver_method == SM_FBS) || (solver_method == SM_NR))
		{
			b_mat[0][0] = B_mat[0][0] = line_config->impedance11 * miles;
			b_mat[0][1] = B_mat[0][1] = line_config->impedance12 * miles;
			b_mat[1][0] = B_mat[1][0] = line_config->impedance21 * miles;
			b_mat[1][1] = B_mat[1][1] = line_config->impedance22 * miles; 
			b_mat[2][0] = B_mat[2][0] = complex(0,0);
			b_mat[2][1] = B_mat[2][1] = complex(0,0);
			b_mat[2][2] = B_mat[2][2] = complex(0,0);
			b_mat[0][2] = B_mat[0][2] = complex(0,0);
			b_mat[1][2] = B_mat[1][2] = complex(0,0);

			tn[0] = 0;
			tn[1] = 0;
			tn[2] = 0;
		}
		else
			GL_THROW("Only NR and FBS support z-matrix components.");

	}
	else
	{
		// create local variables that will be used to calculate matrices.
		double dcond,ins_thick,D12,D13,D23;
		double r1,r2,rn,gmr1,gmr2,gmrn;
		complex zp11,zp22,zp33,zp12,zp13,zp23;
		complex zs[3][3];
		double freq_coeff_real, freq_coeff_imag, freq_additive_term;

		//Calculate coefficients for self and mutual impedance - incorporates frequency values
		//Per Kersting (4.39) and (4.40) - coefficients end up same as OHLs
		if (enable_frequency_dependence == true)	//See which frequency to use
		{
			freq_coeff_real = 0.00158836*current_frequency;
			freq_coeff_imag = 0.00202237*current_frequency;
			freq_additive_term = log(EARTH_RESISTIVITY/current_frequency)/2.0 + 7.6786;
		}
		else
		{
			freq_coeff_real = 0.00158836*nominal_frequency;
			freq_coeff_imag = 0.00202237*nominal_frequency;
			freq_additive_term = log(EARTH_RESISTIVITY/nominal_frequency)/2.0 + 7.6786;
		}

		// Gather data stored in configuration objects
		dcond = line_config->diameter;
		ins_thick = line_config->ins_thickness;

		triplex_line_conductor *l1 = OBJECTDATA(line_config->phaseA_conductor,triplex_line_conductor);
		triplex_line_conductor *l2 = OBJECTDATA(line_config->phaseB_conductor,triplex_line_conductor);
		triplex_line_conductor *lN = OBJECTDATA(line_config->phaseC_conductor,triplex_line_conductor);

		if (l1 == NULL || l2 == NULL || lN == NULL)
		{
			GL_THROW("triplex_line_configuration:%d (%s) is missing a conductor specification.",line_config->get_id(),line_config->get_name());
			/* TROUBLESHOOT
			At this point, triplex lines are assumed to have three conductor values: conductor_1, conductor_2,
			and conductor_N.  If any of these are missing, the triplex line cannot be specified.  Please
			verify that your triplex_line_configuration object contains all of the neccessary conductor values.
			*/
		}

		r1 = l1->resistance;
		r2 = l2->resistance;
		rn = lN->resistance;
		gmr1 = l1->geometric_mean_radius;
		gmr2 = l2->geometric_mean_radius;
		gmrn = lN->geometric_mean_radius;

		// Perform calculations and fill in values in the matrices
		D12 = (dcond + 2 * ins_thick)/12;
		D13 = (dcond + ins_thick)/12;
		D23 = D13;

		if (D12 <= 0.0 || D13 <= 0.0)
		{
			GL_THROW("triplex_line_configuration diameter and/or insulation_thickness are incorrectly set. Please set both of these values to a positive value.");
			/* TROUBLESHOOT
			The triplex line configuration requires that the spacing between conductors (diameter + 2*insulation_thickness) &
			(diameter + insulation_thickness) must be positive values.  Please look at your triplex_line_configuration to verify
			that one or both of these variables are set to positive values.  A good resource for the geometrical configuration is
			William H. Kersting, "Distribution System Modeling and Analysis, 3rd Ed.", Chapter 11.
			*/
		}

		zp11 = complex(r1,0) + freq_coeff_real + complex(0.0,freq_coeff_imag) * (log(1/gmr1) + freq_additive_term);
		zp22 = complex(r2,0) + freq_coeff_real + complex(0.0,freq_coeff_imag) * (log(1/gmr2) + freq_additive_term);
		zp33 = complex(rn,0) + freq_coeff_real + complex(0.0,freq_coeff_imag) * (log(1/gmrn) + freq_additive_term);
		zp12 = complex(freq_coeff_real,0.0) + complex(0.0,freq_coeff_imag) * (log(1/D12) + freq_additive_term);
		zp13 = complex(freq_coeff_real,0.0) + complex(0.0,freq_coeff_imag) * (log(1/D13) + freq_additive_term);
		zp23 = complex(freq_coeff_real,0.0) + complex(0.0,freq_coeff_imag) * (log(1/D23) + freq_additive_term);
		
		if ((solver_method==SM_FBS) || (solver_method==SM_NR))
		{
			zs[0][0] = zp11-((zp13*zp13)/zp33);
			zs[0][1] = zp12-((zp13*zp23)/zp33);
			zs[1][0] = -(zp12-((zp13*zp23)/zp33));
			zs[1][1] = -(zp22-((zp23*zp23)/zp33));
			zs[0][2] = complex(0,0);
			zs[1][2] = complex(0,0);
			zs[2][2] = complex(0,0);
			zs[2][1] = complex(0,0);
			zs[2][0] = complex(0,0);
		}
		else
		{
			GL_THROW("triplex_line:unsupported solver method");
			/*  TROUBLESHOOT
			While computing the impedance values for a triplex_line, an invalid solver method was used.
			Please use only the supported solvers (FBS and NR), or adjust the triplex_line code to accommodate
			your new method.
			*/
		}

		
		//No solver check here -- NR and FBS are the same, and if it is something else, it should have failed above
		tn[0] = -zp13/zp33;
		tn[1] = -zp23/zp33;
		tn[2] = 0;

		multiply(length/5280.0,zs,b_mat); // Length comes in ft, convert to miles.
		multiply(length/5280.0,zs,B_mat);
	}
	
	//Check for negative resistance in the line's impedance matrix
	bool neg_res = false;
	for (int n = 0; n < 2; n++){
		if(b_mat[0][n].Re() < 0.0){
			neg_res = true;
		}
	}
	
	if(neg_res == true){
		gl_warning("INIT: triplex_line:%s has a negative resistance in it's impedance matrix. This will result in unusual behavior. Please check the line's geometry and cable parameters.", obj->name);
		/*  TROUBLESHOOT
		A negative resistance value was found for one or more the real parts of the triplex_line's impedance matrix.
		While this is numerically possible, it is a physical impossibility. This resulted most likely from a improperly 
		defined conductor cable. Please check and modify the conductor objects used for this line to correct this issue.
		*/
	}

	// Same in all cases
	a_mat[0][0] = complex(1,0);
	a_mat[0][1] = complex(0,0);
	a_mat[0][2] = complex(0,0);
	a_mat[1][0] = complex(0,0);
	a_mat[1][1] = complex(1,0);
	a_mat[1][2] = complex(0,0);
	a_mat[2][0] = complex(0,0);
	a_mat[2][1] = complex(0,0);
	a_mat[2][2] = complex(1,0); 
	
	c_mat[0][0] = complex(0,0);
	c_mat[0][1] = complex(0,0);
	c_mat[0][2] = complex(0,0);
	c_mat[1][0] = complex(0,0);
	c_mat[1][1] = complex(0,0);
	c_mat[1][2] = complex(0,0);
	c_mat[2][0] = complex(0,0);
	c_mat[2][1] = complex(0,0);
	c_mat[2][2] = complex(0,0);
	
	d_mat[0][0] = complex(1,0);
	d_mat[0][1] = complex(0,0);
	d_mat[0][2] = complex(0,0);
	d_mat[1][0] = complex(0,0);
	d_mat[1][1] = complex(1,0);
	d_mat[1][2] = complex(0,0);
	d_mat[2][0] = complex(0,0);
	d_mat[2][1] = complex(0,0);
	d_mat[2][2] = complex(1,0);
	
	A_mat[0][0] = complex(1,0);
	A_mat[0][1] = complex(0,0);
	A_mat[0][2] = complex(0,0);
	A_mat[1][0] = complex(0,0);
	A_mat[1][1] = complex(1,0);
	A_mat[1][2] = complex(0,0);
	A_mat[2][0] = complex(0,0);
	A_mat[2][1] = complex(0,0);
	A_mat[2][2] = complex(1,0);

	// print out matrices when testing.
#ifdef _TESTING
	if (show_matrix_values)
	{
		gl_testmsg("triplex_line: a matrix");
		print_matrix(a_mat);

		gl_testmsg("triplex_line: A matrix");
		print_matrix(A_mat);

		gl_testmsg("triplex_line: b matrix");
		print_matrix(b_mat);

		gl_testmsg("triplex_line: B matrix");
		print_matrix(B_mat);

		gl_testmsg("triplex_line: c matrix");
		print_matrix(c_mat);

		gl_testmsg("triplex_line: d matrix");
		print_matrix(d_mat);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: triplex_line
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/



/* This can be added back in after tape has been moved to commit
EXPORT TIMESTAMP commit_triplex_line(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	if ((solver_method==SM_FBS) || (solver_method==SM_NR))
	{
		triplex_line *plink = OBJECTDATA(obj,triplex_line);
		plink->calculate_power_splitphase();
	}
	return TS_NEVER;
}
*/
EXPORT int create_triplex_line(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(triplex_line::oclass);
		if (*obj!=NULL)
		{
			triplex_line *my = OBJECTDATA(*obj,triplex_line);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(triplex_line);
}

EXPORT TIMESTAMP sync_triplex_line(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		triplex_line *pObj = OBJECTDATA(obj,triplex_line);
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
	SYNC_CATCHALL(triplex_line);
}

EXPORT int init_triplex_line(OBJECT *obj)
{
	try {
		triplex_line *my = OBJECTDATA(obj,triplex_line);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(triplex_line);
}

EXPORT int isa_triplex_line(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_line)->isa(classname);
}

EXPORT int recalc_triplex_line(OBJECT *obj)
{
	OBJECTDATA(obj,triplex_line)->recalc();
	return 1;
}

/**@}**/
