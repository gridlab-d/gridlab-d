// $Id: series_compensator.h 2018-12-06 $
//	Copyright (C) 2018 Battelle Memorial Institute
//
// Implements series-compensation device via a regulator analog.

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "series_compensator.h"

CLASS* series_compensator::oclass = NULL;
CLASS* series_compensator::pclass = NULL;

series_compensator::series_compensator(MODULE *mod) : link_object(mod)
{
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = link_object::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"series_compensator",sizeof(series_compensator),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class series_compensator";
		else
			oclass->trl = TRL_DEMONSTRATED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "link",

			PT_double, "vset_A[pu]", PADDR(vset_value[0]), PT_DESCRIPTION, "Voltage magnitude set point for phase A",
			PT_double, "vset_B[pu]", PADDR(vset_value[1]), PT_DESCRIPTION, "Voltage magnitude set point for phase B",
			PT_double, "vset_C[pu]", PADDR(vset_value[2]), PT_DESCRIPTION, "Voltage magnitude set point for phase C",

			PT_double, "voltage_update_tolerance[pu]", PADDR(voltage_iter_tolerance), PT_DESCRIPTION, "Largest absolute between vset_X and measured voltage that won't force a reiteration",

			PT_double, "turns_ratio_A", PADDR(turns_ratio[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase A series compensator equivalent",
			PT_double, "turns_ratio_B", PADDR(turns_ratio[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase B series compensator equivalent",
			PT_double, "turns_ratio_C", PADDR(turns_ratio[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase C series compensator equivalent",
			PT_double, "series_compensator_resistance[Ohm]", PADDR(series_compensator_resistance), PT_DESCRIPTION, "Baseline resistance for the series compensator device - needed for NR",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_series_compensator)==NULL)
			GL_THROW("Unable to publish series_compensator deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==NULL)
			GL_THROW("Unable to publish series_compensator external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==NULL)
			GL_THROW("Unable to publish series_compensator external power limit calculation function");
	}
}

int series_compensator::isa(char *classname)
{
	return strcmp(classname,"series_compensator")==0 || link_object::isa(classname);
}

int series_compensator::create()
{
	int result = link_object::create();
	
	//Working variables - set initial ratio to unity
	turns_ratio[0] = turns_ratio[1] = turns_ratio[2] = 1.0;
	prev_turns_ratio[0] = prev_turns_ratio[1] = prev_turns_ratio[2] = 0.0;	//Set different, so it forces an update

	//Null the voltage pointers
	ToNode_voltage[0] = ToNode_voltage[1] = ToNode_voltage[2] = NULL;

	//Zero the values
	val_ToNode_voltage[0] = val_ToNode_voltage[1] = val_ToNode_voltage[2] = complex(0.0,0.0);
	val_ToNode_voltage_pu[0] = val_ToNode_voltage_pu[1] = val_ToNode_voltage_pu[2] = 0.0;
	vset_value[0] = vset_value[1] = vset_value[2] = 1.0;	//Start out set to unity voltage
	val_FromNode_nominal_voltage = 0.0;

	voltage_iter_tolerance = 0.01;	//1% PU default

	return result;
}

int series_compensator::init(OBJECT *parent)
{
	char iindex, jindex;
	int result = link_object::init(parent);
	OBJECT *obj = OBJECTHDR(this);
	gld_property *temp_volt_prop;

	//Loop through and zero the various line matrices (link should really do this -- @TODO - Put this in link.cpp sometime)
	for (iindex=0; iindex<3; iindex++)
	{
		for (jindex=0; jindex<3; jindex++)
		{
			a_mat[iindex][jindex] = complex(0.0,0.0);
			b_mat[iindex][jindex] = complex(0.0,0.0);
			c_mat[iindex][jindex] = complex(0.0,0.0);
			d_mat[iindex][jindex] = complex(0.0,0.0);
			A_mat[iindex][jindex] = complex(0.0,0.0);
			B_mat[iindex][jindex] = complex(0.0,0.0);
			base_admittance_mat[iindex][jindex] = complex(0.0,0.0);
		}
	}

	//Map the to-node connections
	if (has_phase(PHASE_A))
	{
		//Map to the property of interest - voltage_A
		ToNode_voltage[0] = new gld_property(to,"voltage_A");

		//Make sure it worked
		if ((ToNode_voltage[0]->is_valid() != true) || (ToNode_voltage[0]->is_complex() != true))
		{
			GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
			/* TROUBLESHOOT
			While attempting to map the voltage for the to node, a property could not be properly mapped.
			Please try again.  If the error persists, please submit an issue in the ticketing system.
			*/
		}

		//Set the appropriate turns ratio matrices
		d_mat[0][0] = complex(turns_ratio[0],0.0);
		a_mat[0][0] = complex(1.0/turns_ratio[0],0.0);
		A_mat[0][0] = complex(turns_ratio[0],0.0);
	}
	else	//Nope
	{
		ToNode_voltage[0] = NULL;	//Null it -- should already be done, but be paranoid

		//Set the per-unit setpoint too -- otherwise we'll have issues later
		vset_value[0] = 0.0;
	}

	//Check for B
	if (has_phase(PHASE_B))
	{
		//Map to the property of interest - voltage_B
		ToNode_voltage[1] = new gld_property(to,"voltage_B");

		//Make sure it worked
		if ((ToNode_voltage[1]->is_valid() != true) || (ToNode_voltage[1]->is_complex() != true))
		{
			GL_THROW("series_compensator:%d - %s - Unable to map property for TO node",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		//Set the appropriate turns ratio matrices
		d_mat[1][1] = complex(turns_ratio[1],0.0);
		a_mat[1][1] = complex(1.0/turns_ratio[1],0.0);
		A_mat[1][1] = complex(turns_ratio[1],0.0);
	}
	else	//Not here
	{
		ToNode_voltage[1] = NULL;	//Null for paranoia

		//Set the per-unit setpoint too -- otherwise we'll have issues later
		vset_value[1] = 0.0;
	}

	//Check for C
	if (has_phase(PHASE_C))
	{
		//Map to the property of interest - voltage_C
		ToNode_voltage[2] = new gld_property(to,"voltage_C");

		//Make sure it worked
		if ((ToNode_voltage[2]->is_valid() != true) || (ToNode_voltage[2]->is_complex() != true))
		{
			GL_THROW("series_compensator:%d - %s - Unable to map property for TO node",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		//Set the appropriate turns ratio matrices
		d_mat[2][2] = complex(turns_ratio[2],0.0);
		a_mat[2][2] = complex(1.0/turns_ratio[2],0.0);
		A_mat[2][2] = complex(turns_ratio[2],0.0);
	}
	else	//Nope
	{
		ToNode_voltage[2] = NULL;

		//Set the per-unit setpoint too -- otherwise we'll have issues later
		vset_value[2] = 0.0;
	}

	//Pull the nominal voltage from our from node
	temp_volt_prop = new gld_property(from,"nominal_voltage");

	//Make sure it worked
	if ((temp_volt_prop->is_valid() != true) || (temp_volt_prop->is_double() != true))
	{
		GL_THROW("series_compensator:%d - %s - Unable to map nominal_voltage of FROM node",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While mapping the nominal_voltage property of the from node, an error occurred.  Please try again.
		If the error persists, please submit your code and a bug report via the ticketing system.
		*/
	}

	//Pull that voltage
	val_FromNode_nominal_voltage = temp_volt_prop->get_double();

	//Remove the pointer
	delete temp_volt_prop;

	if (solver_method == SM_NR)
	{
		if(series_compensator_resistance <= 0.0){
			gl_warning("series_compensator:%s series_compensator_resistance is a negative or zero value -- setting to the global default.",obj->name);
			/*  TROUBLESHOOT
			Under Newton-Raphson solution method, the impedance matrix cannot be a singular matrix for the inversion process (or have an invalid negative value).
			Change the value of series_compensator_resistance to something small but larger that zero.
			*/

			//Set the default
			series_compensator_resistance = default_resistance;
		}

		//Flag the special link as a rgulator, since right now, we work the same way
		SpecialLnk = REGULATOR;

		//Set the baseline impedance values
		if (has_phase(PHASE_A))
		{
			base_admittance_mat[0][0] = complex(1.0/series_compensator_resistance,0.0);
			b_mat[0][0] = series_compensator_resistance;
		}
		if (has_phase(PHASE_B))
		{
			base_admittance_mat[1][1] = complex(1.0/series_compensator_resistance,0.0);
			b_mat[1][1] = series_compensator_resistance;
		}
		if (has_phase(PHASE_C))
		{
			base_admittance_mat[2][2] = complex(1.0/series_compensator_resistance,0.0);
			b_mat[2][2] = series_compensator_resistance;
		}
	}

	return result;
}

TIMESTAMP series_compensator::presync(TIMESTAMP t0) 
{
	TIMESTAMP t1;

	//Call the pre-presync regulator code
	sercom_prePre_fxn();

	//Call the standard presync
	t1 = link_object::presync(t0);

	//Call the post-presync regulator code
	sercom_postPre_fxn();

	if (t1 != TSNVRDBL)
		return -t1; 	//Soft return
	else
		return TS_NEVER;
}

//Postsync
TIMESTAMP series_compensator::postsync(TIMESTAMP t0)
{
	int function_return_val;

	TIMESTAMP t1 = link_object::postsync(t0);

	function_return_val = sercom_postPost_fxn(0);	//Just give it an arbitary value -- used mostly for delta flagging

	//See if it was an error or not
	if (function_return_val == -1)
	{
		return TS_INVALID;
	}
	else if (function_return_val != 0)
	{
		//If this changes, re-evaluate this code
		return t0;
	}
	//Default else -- no returns were hit, so just do t1

	return t1;
}

//Functionalized "presync before link::presync" portions, mostly for deltamode functionality
void series_compensator::sercom_prePre_fxn(void)
{
	//Update turns ratio for various phases
	if (has_phase(PHASE_A))
	{
		d_mat[0][0] = complex(turns_ratio[0],0.0);
		a_mat[0][0] = complex(1.0/turns_ratio[0],0.0);
		A_mat[0][0] = complex(turns_ratio[0],0.0);
	}

	if (has_phase(PHASE_B))
	{
		d_mat[1][1] = complex(turns_ratio[1],0.0);
		a_mat[1][1] = complex(1.0/turns_ratio[1],0.0);
		A_mat[1][1] = complex(turns_ratio[1],0.0);
	}

	if (has_phase(PHASE_C))
	{
		d_mat[2][2] = complex(turns_ratio[2],0.0);
		a_mat[2][2] = complex(1.0/turns_ratio[2],0.0);
		A_mat[2][2] = complex(turns_ratio[2],0.0);
	}
}

//Functionalized version of the code for deltamode - "post-link::presync" portions
void series_compensator::sercom_postPre_fxn(void)
{
	char phaseWarn;
	int jindex,kindex;
	complex Ylefttemp[3][3];
	complex Yto[3][3];
	complex Yfrom[3][3];
	complex invratio[3];

	if (solver_method == SM_NR)
	{
		//See if we even need to update
		//Only perform update if a turns_ratio has changed.  Since the link calculations are replicated here and it directly
		//accesses the NR memory space, this won't cause any issues.
		if ((prev_turns_ratio[0] != turns_ratio[0]) || (prev_turns_ratio[1] != turns_ratio[1]) || (prev_turns_ratio[2] != turns_ratio[2]))	//Change has occurred
		{
			//Compute the inverse ratio - A
			if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)
			{
				invratio[0] = complex(1.0,0.0) / a_mat[0][0];
			}
			else
			{
				invratio[0] = complex(0.0,0.0);
			}

			//Compute the inverse ratio - B
			if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)
			{
				invratio[1] = complex(1.0,0.0) / a_mat[1][1];
			}
			else
			{
				invratio[1] = complex(0.0,0.0);
			}

			//Compute the inverse ratio - C
			if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)
			{
				invratio[2] = complex(1.0,0.0) / a_mat[2][2];
			}
			else
			{
				invratio[2] = complex(0.0,0.0);
			}

			//Get matrices for NR

			//Pre-admittancized matrix
			equalm(base_admittance_mat,Yto);

			//Store value into YSto
			for (jindex=0; jindex<3; jindex++)
			{
				for (kindex=0; kindex<3; kindex++)
				{
					YSto[jindex*3+kindex]=Yto[jindex][kindex];
				}
			}
			
			//Assumes diagonal
			for (jindex=0; jindex<3; jindex++)
			{
				Ylefttemp[jindex][jindex] = Yto[jindex][jindex] * invratio[jindex];
				Yfrom[jindex][jindex]=Ylefttemp[jindex][jindex] * invratio[jindex];
			}

			//Store value into YSfrom
			for (jindex=0; jindex<3; jindex++)
			{
				for (kindex=0; kindex<3; kindex++)
				{
					YSfrom[jindex*3+kindex]=Yfrom[jindex][kindex];
				}
			}

			for (jindex=0; jindex<3; jindex++)
			{
				To_Y[jindex][jindex] = Yto[jindex][jindex] * invratio[jindex];
				From_Y[jindex][jindex]=Yfrom[jindex][jindex] * a_mat[jindex][jindex];
			}

			//Flag an update
			LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this
			NR_admit_change = true;
			UNLOCK_OBJECT(NR_swing_bus);	//Unlock

			//Update our previous turns ratio - for tracking
			prev_turns_ratio[0] = turns_ratio[0];
			prev_turns_ratio[1] = turns_ratio[1];
			prev_turns_ratio[2] = turns_ratio[2];
		}//End updated needed
	}//End NR
}

//Functionalized "postsyc after link::postsync" items -- mostly for deltamode compatibility
//Return values -- 0 = no iteration, 1 = reiterate (deltamode or otherwise), 2 = proceed (deltamode)
//Pass value is for deltamode pass information (currently in modified Euler flagging)
int series_compensator::sercom_postPost_fxn(unsigned char pass_value)
{
	char index_val;
	double temp_diff_val;
	int return_val;

	//By default, assume we want to exit normal
	return_val = 0;

	//Pull the voltages
	if (has_phase(PHASE_A))
	{
		//Get raw value
		val_ToNode_voltage[0] = ToNode_voltage[0]->get_complex();

		//Do the per-unit conversion
		val_ToNode_voltage_pu[0] = val_ToNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
	}
	else
	{
		//Zero everything
		val_ToNode_voltage[0] = complex(0.0,0.0);
		val_ToNode_voltage_pu[0] = 0.0;
	}

	//Pull the voltages
	if (has_phase(PHASE_B))
	{
		//Get raw value
		val_ToNode_voltage[1] = ToNode_voltage[1]->get_complex();

		//Do the per-unit conversion
		val_ToNode_voltage_pu[1] = val_ToNode_voltage[1].Mag() / val_FromNode_nominal_voltage;
	}
	else
	{
		//Zero everything
		val_ToNode_voltage[1] = complex(0.0,0.0);
		val_ToNode_voltage_pu[1] = 0.0;
	}

	//Pull the voltages
	if (has_phase(PHASE_C))
	{
		//Get raw value
		val_ToNode_voltage[2] = ToNode_voltage[2]->get_complex();

		//Do the per-unit conversion
		val_ToNode_voltage_pu[2] = val_ToNode_voltage[2].Mag() / val_FromNode_nominal_voltage;
	}
	else
	{
		//Zero everything
		val_ToNode_voltage[2] = complex(0.0,0.0);
		val_ToNode_voltage_pu[2] = 0.0;
	}

	//Check and see what we should return
	for (index_val=0; index_val<3; index_val++)
	{
		temp_diff_val = fabs(vset_value[index_val] - val_ToNode_voltage_pu[index_val]);

		//Check it
		if (temp_diff_val > voltage_iter_tolerance)
		{
			return_val = 1;	//Arbitrary flag - could be used for other things in the future
		}
	}

	//******************* Code injection point ******************//
	//This may be a place to do the turns ratio update, or in the above logic (with the return value)
	//Probably where the integration-method code would go 
	//Need to flag if deltamode or not -- use "" to do that (possibly inside the function)

	//Return
	return return_val;
}

//Module-level deltamode call
SIMULATIONMODE series_compensator::inter_deltaupdate_series_compensator(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//OBJECT *hdr = OBJECTHDR(this);
	double curr_time_value;	//Current time of simulation
	int temp_return_val;
	unsigned char pass_mod;

	//Get the current time
	curr_time_value = gl_globaldeltaclock;

	//Figure out which pass we are (for integration methods)
	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Replicate presync behavior

		//Call the pre-presync regulator code
		sercom_prePre_fxn();

		//Link presync stuff
		NR_link_presync_fxn();
		
		//Call the post-presync regulator code
		sercom_postPre_fxn();

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Call postsync
		BOTH_link_postsync_fxn();

		//Call the regulator-specific post-postsync function
		//******************* Implementation -- Inside this function will probably be where the logic and/or pred/corr needs to go ******//
		temp_return_val = sercom_postPost_fxn(pass_mod);

		//Make sure it wasn't an error
		if (temp_return_val == -1)
		{
			return SM_ERROR;
		}
		else if (temp_return_val == 1)	//Force a reiteration
		{
			//Ask for a reiteration
			return SM_DELTA_ITER;
		}
		else if (temp_return_val == 2)	//Stay in deltamode
		{
			//Ask to proceed
			return SM_DELTA;
		}
		//Otherwise, it was a proceed forward -- probably returned a future state time

		return SM_EVENT;	//Always prompt for an exit
	}
}//End module deltamode

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: series_compensator
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/

EXPORT int create_series_compensator(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(series_compensator::oclass);
		if (*obj!=NULL)
		{
			series_compensator *my = OBJECTDATA(*obj,series_compensator);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(series_compensator);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_series_compensator(OBJECT *obj)
{
	try {
		series_compensator *my = OBJECTDATA(obj,series_compensator);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(series_compensator);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_series_compensator(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		series_compensator *pObj = OBJECTDATA(obj,series_compensator);
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
	SYNC_CATCHALL(series_compensator);
}

EXPORT int isa_series_compensator(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,series_compensator)->isa(classname);
}

//Export for deltamode
EXPORT SIMULATIONMODE interupdate_series_compensator(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	series_compensator *my = OBJECTDATA(obj,series_compensator);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_series_compensator(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_series_compensator(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}*/
