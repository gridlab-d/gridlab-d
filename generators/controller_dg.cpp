/*
 * controller_dg.cpp
 *
 *  Created on: Sep 21, 2016
 *      Author: tang526
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "controller_dg.h"

CLASS* controller_dg::oclass = NULL;
controller_dg *controller_dg::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

controller_dg::controller_dg(MODULE *mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"controller_dg",sizeof(controller_dg),passconfig|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class controller_dg";
		else
			oclass->trl = TRL_PROOF;

		if(gl_publish_variable(oclass,
			//Overall inputs for the PI controller
			PT_char32,"groupid_dg", PADDR(controlled_dgs), PT_DESCRIPTION, "the group ID of the dg objects the controller controls.",
			PT_double,"ki", PADDR(ki), PT_DESCRIPTION, "parameter of the propotional control",
			PT_double,"kp", PADDR(kp), PT_DESCRIPTION, "parameter of the integral control",
			PT_double,"gain", PADDR(gain), PT_DESCRIPTION, "Gain of the controller",
			PT_double,"ki_QV", PADDR(ki_QV), PT_DESCRIPTION, "parameter of the propotional control for secondary voltage control",
			PT_double,"kp_QV", PADDR(kp_QV), PT_DESCRIPTION, "parameter of the integral control for secondary voltage control",
			PT_double,"gain_QV", PADDR(gain_QV), PT_DESCRIPTION, "Gain of the controller for secondary voltage control",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		defaults = this;

		memset(this,0,sizeof(controller_dg));

		if (gl_publish_function(oclass,	"interupdate_controller_object", (FUNCTIONADDR)interupdate_controller_dg)==NULL)
			GL_THROW("Unable to publish controller_dg deltamode function");
		if (gl_publish_function(oclass,	"postupdate_controller_object", (FUNCTIONADDR)postupdate_controller_dg)==NULL)
			GL_THROW("Unable to publish controller_dg deltamode function");

    }
}

int controller_dg::create(void)
{
	// Give default values for frequency reference
	omega_ref=2*PI*60;

	deltamode_inclusive = false;
	mapped_freq_variable = NULL;

	dgswitchFound = 0;

	//Pref convegence of the controller
	controller_Pref_convergence_criterion = 0.0001;

	// Default values for the PI controller parameters
	gain = 150;
	kp = 0.005;
	ki = 0.2;

	first_run = true;
	flag_switchOn = false;

	controlTime = 0;

	return 1;
}

/* Object initialization is called once after all object have been created */
int controller_dg::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	// Obtain the pointer to the diesel_dg objects
	if(controlled_dgs[0] == '\0'){
		dgs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"diesel_dg",FT_END);
		if(dgs == NULL){
			gl_error("No diesel_dg objects were found.");
			return 0;
			/* TROUBLESHOOT
			No diesel_dg objects were found in your .glm file.
			*/
		}
	}
	else {
		//Find all dgs with the controller group id
		dgs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"diesel_dg",AND,FT_GROUPID,SAME,controlled_dgs.get_string(),FT_END);
		if(dgs == NULL){
			gl_error("Although controller given group id, no dgs with given group id found.");
			/*  TROUBLESHOOT
			While trying to put together a list of all dg objects with the specified controller groupid, no such dg objects were found.
			*/

			return 0;
		}
	}

	// Creat controller for each generator
	obj = NULL;
	int index = 0;
	if(dgs != NULL){
		pDG = (diesel_dg **)gl_malloc(dgs->hit_count*sizeof(diesel_dg*));
		DGpNdName = (char **)gl_malloc(dgs->hit_count*sizeof(char*));
		if(pDG == NULL){
			gl_error("Failed to allocate diesel_dg array.");
			return TS_NEVER;
		}

		GenPobj = (node **)gl_malloc(dgs->hit_count*sizeof(node*));

		while(obj = gl_find_next(dgs,obj)){

			// Store each generator parented node name,
			// so that the corresponding connected switch can be found
			OBJECT *dgParent = obj->parent;
			if(dgParent == NULL){
				gl_error("Failed to find diesel_dg parent node object.");
				return 0;
			}
			DGpNdName[index] = dgParent->name;

			// Store generator data pointer
			if(index >= dgs->hit_count){
				break;
			}
			pDG[index] = OBJECTDATA(obj,diesel_dg);
			if(pDG[index] == NULL){
				gl_error("Unable to map object as diesel_dg object.");
				return 0;
			}


			// Find DG parent node, and corresponding positive voltage
			// Find DG parent object data
			node *tempNode = OBJECTDATA(dgParent,node);
			if(tempNode == NULL){
				gl_error("Unable to map object as diesel_dg object.");
			}
			GenPobj[index] = tempNode;

			++index;
		}

		//Assign space for the array of CTRL_Gen
		ctrlGen = (CTRL_Gen **)gl_malloc(dgs->hit_count*sizeof(CTRL_Gen*));
		prev_Pref_val = (double*)gl_malloc(dgs->hit_count*sizeof(double));
		prev_Vset_val = (double*)gl_malloc(dgs->hit_count*sizeof(double));

		//See if it worked
		if (ctrlGen == NULL || prev_Pref_val == NULL)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		// Assign space for each pointer
		for (int i = 0; i < dgs->hit_count; i++) {
			ctrlGen[i] = (CTRL_Gen *)gl_malloc(sizeof(CTRL_Gen));
			//See if it worked
			if (ctrlGen[i] == NULL)
			{
				GL_THROW("Restoration:Failed to allocate new chain");
				//Defined above
			}
			ctrlGen[i]->curr_state = (CTRL_VARS*)gl_malloc(sizeof(CTRL_VARS));
			ctrlGen[i]->next_state = (CTRL_VARS*)gl_malloc(sizeof(CTRL_VARS));

			prev_Pref_val[i] = 0; //Assign value to each prev_Pref_val
			prev_Vset_val[i] = 0; //Assign value to each prev_Vset_val
			ctrlGen[i]->curr_state->Vset_ref = -1; // Assign negative initial values to Vset_ref as an indicator
		}


	}

	// Obtain the pointer to the switch objects
	obj = NULL;
	switches = gl_find_objects(FL_NEW,FT_CLASS,SAME,"switch",FT_END);
	if(switches == NULL){
		gl_error("No switch objects were found.");
		return 0;
		/* TROUBLESHOOT
		No switch objects were found in your .glm file.
		*/
	}
	index = 0;
	if(switches != NULL){

		dgSwitchObj = (OBJECT**)gl_malloc(dgs->hit_count*sizeof(OBJECT*));
		pSwitch = (switch_object **)gl_malloc(dgs->hit_count*sizeof(switch_object*));
		if(pSwitch == NULL){
			gl_error("Failed to allocate switch array.");
			return TS_NEVER;
		}

		while(obj = gl_find_next(switches,obj)){
			if(index >= switches->hit_count){
				break;
			}

			// Search for switches that connected to the generators
			switch_object *temp_switch = OBJECTDATA(obj,switch_object);

			// Obtain the voltage and frequency values for each switch
			// Get the switch from node object
			char *temp_from_name = temp_switch->from->name;
			char *temp_to_name = temp_switch->to->name;

			bool found = false;
			for (int i = 0; i < dgs->hit_count; i++) {
				if (strcmp(temp_from_name, DGpNdName[i]) == 0 || strcmp(temp_to_name, DGpNdName[i]) == 0) {
					found = true;
					break;
				}
			}
			if (found == true) {
				// Store the switches that connected to the generatos
				pSwitch[dgswitchFound] = OBJECTDATA(obj,switch_object);
				dgSwitchObj[dgswitchFound] = obj;
				if(pSwitch[dgswitchFound] == NULL){
					gl_error("Unable to map object as switch object.");
					return 0;
				}
				dgswitchFound++;
			}

			++index;
		}
	}

	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models!=true)
		{
			gl_warning("diesel_dg:%s indicates it wants to run deltamode, but the module-level flag is not set!",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			//Map the powerflow frequency
			mapped_freq_variable = (double *)gl_get_module_var(gl_find_module("powerflow"),"current_frequency");

			//Make sure it isn't empty
			if (mapped_freq_variable == NULL)
			{
				GL_THROW("controller_dg:%s - Failed to map frequency checking variable from powerflow for deltamode",obj->name?obj->name:"unnamed");
				//Defined above
			}

			gen_object_count++;	//Increment the counter
		}
	}//End deltamode inclusive
	else	//Not enabled for this model
	{
		if (enable_subsecond_models == true)
		{
			gl_warning("diesel_dg:%d %s - Deltamode is enabled for the module, but not this generator!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The diesel_dg is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this generator may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
	}

	// Set rank as 1 so that the controller_dg can be executed after the generators (ranked 0) in sync (and delta-mode) process
	obj = OBJECTHDR(this);
	gl_set_rank(obj,1);

	return 1;
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP controller_dg::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	//Does nothing right now - presync not part of the sync list for this object
	return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called at first run, mainly for registration of the delta mode functions */
TIMESTAMP controller_dg::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret_value;

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//First run allocation - in diesel_dg for now, but may need to move elsewhere
	if (first_run == true)	//First run
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models )	//We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects==NULL)) && (enable_subsecond_models == true))
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (gen_object_current>=gen_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the generators module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the generator
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Add us into the list
			delta_objects[gen_object_current] = obj;

			//Map up the function for interupdate
			delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"interupdate_controller_object"));

			//Make sure it worked
			if (delta_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Map up the function for postupdate
			post_delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"postupdate_controller_object"));

			//Make sure it worked
			if (post_delta_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map post-deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the postupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Update pointer
			gen_object_current++;

			//Force us to reiterate one
			tret_value = t1;

		}//End deltamode specials - first pass
		//Default else - no deltamode stuff
	}//End first timestep

	return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP controller_dg::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	int ret_state;
	OBJECT *obj = OBJECTHDR(this);

	//Update global, if necessary - assume everyone grabbed by sync
	if (deltamode_endtime != TS_NEVER)
	{
		deltamode_endtime = TS_NEVER;
		deltamode_endtime_dbl = TSNVRDBL;
	}

	//
	if (first_run == true)	//Final init items - namely deltamode supersecond exciter
	{
		if (deltamode_inclusive && enable_subsecond_models) //Still "first run", but at least one powerflow has completed (call init dyn now)
		{

			for (int index = 0; index < dgs->hit_count; index++) {
				// initialize control for each generator
				ret_state = init_dynamics(ctrlGen[index]->curr_state, index);

				if (ret_state == FAILED)
				{
					GL_THROW("diesel_dg:%s - unsuccessful call to dynamics initialization",(obj->name?obj->name:"unnamed"));
					/*  TROUBLESHOOT
					While attempting to call the dynamics initialization function of the diesel_dg object, a failure
					state was encountered.  See other error messages for further details.
					*/
				}
			}

		}

	}

	first_run = false;

	return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////

//Module-level call
SIMULATIONMODE controller_dg::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	unsigned char pass_mod;
	double deltat, deltath;
	double temp_double = 0;

	OBJECT *switchFromNdObj = NULL;
	node *switchFromNd = NULL;
	complex vtemp[3];
	double nominal_voltage;
	FUNCTIONADDR funadd = NULL;
	int return_val;
	unsigned char openPhases[] = {0x04, 0x02, 0x01};

	// Control of the generator switch
	// Detect abnormal feeder frequency, switch terminal voltage, and reverse real power flow from generators (absorbing real power)
	// Switch will be turned off. Delay has been added to avoid simultaneously turning off all switches
	OBJECT *obj = NULL;
	for (int index = 0; index < dgswitchFound; index++) {

		// Obtain the switch object so that function can be called
		obj = dgSwitchObj[index];

		// Obtain the voltage and frequency values for each switch
		// Get the switch from node object
		switchFromNdObj = gl_get_object(pSwitch[index]->from->name);
		if (switchFromNdObj==NULL) {
			throw "Switch from node is not specified";
			/*  TROUBLESHOOT
			The from node for a line or link is not connected to anything.
			*/
		}

		// Get the switch from node properties
		switchFromNd = OBJECTDATA(switchFromNdObj,node);

		// Check switch from node voltages
		vtemp[0] = switchFromNd->voltage[0];
		vtemp[1] = switchFromNd->voltage[1];
		vtemp[2] = switchFromNd->voltage[2];
		nominal_voltage = switchFromNd->nominal_voltage;

		// Call the switch status
		enumeration phase_A_state_check = pSwitch[index]->phase_A_state;
		enumeration phase_B_state_check = pSwitch[index]->phase_B_state;
		enumeration phase_C_state_check = pSwitch[index]->phase_C_state;

		// Obtain switch real power value of each phase
		int total_phase_P = pSwitch[index]->power_in.Re();
		int phase_A_P = pSwitch[index]->indiv_power_out[0].Re();
		int phase_B_P = pSwitch[index]->indiv_power_out[1].Re();
		int phase_C_P = pSwitch[index]->indiv_power_out[2].Re();

		// Throw warning
		if (phase_A_P > 0 || phase_B_P > 0 || phase_C_P > 0) {
			gl_warning("Reverse real power flow detected at generator switch %s, generator is absorbing real power now!", obj->name);
			/*  TROUBLESHOOT
			The diesel generator is absorbing real power, which is not allowed.
			Should check kVA rating and power_out_A/power_out_B/power_out_C value settings.
			Large kVA rating, or small initial power output than the total loads may result
			in negative real power output calculated.
			*/
		}


		// Check whether the voltage and frequency is out of limit when the switch is closed
		if ((phase_A_state_check == 1 || phase_B_state_check == 1 || phase_C_state_check == 1) &&
			(((*mapped_freq_variable > omega_ref/(2.0*PI)*1.01) || vtemp[0].Mag() > 1.2*nominal_voltage || vtemp[1].Mag() > 1.2*nominal_voltage || vtemp[2].Mag() > 1.2*nominal_voltage)) ||
			(phase_A_P > 0 || phase_B_P > 0 || phase_C_P > 0))
		{

			if ((flag_switchOn == false) || (controlTime == delta_time)) {

				//map the function
				funadd = (FUNCTIONADDR)(gl_get_function(obj,"change_switch_state"));

				//make sure it worked
				if (funadd==NULL)
				{
					GL_THROW("Failed to find reliability manipulation method on object %s",obj->name);
					/*  TROUBLESHOOT
					While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
					was not located.  Ensure this object type supports special actions and try again.  If the problem persists, please submit a bug
					report and your code to the trac website.
					*/
				}

				//Open each Phase of the switch
				for (int i = 0; i < 3; i++) {
					return_val = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(obj,openPhases[i],false);
					if (return_val == 0)	//Failed :(
					{
						GL_THROW("Failed to handle reliability manipulation on %s",obj->name);
						/*  TROUBLESHOOT
						While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
						failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
						*/
					}
				}

				//Call the switch status
				phase_A_state_check = pSwitch[index]->phase_A_state;
				phase_B_state_check = pSwitch[index]->phase_B_state;
				phase_C_state_check = pSwitch[index]->phase_C_state;

				// Set up flags and time control time every time the switch status is changed
				if (flag_switchOn == false) {
					flag_switchOn = true;
				}

				// Set delay so that all switches will not be opened together
				controlTime == delta_time + 100*dt;

			}
		}
	}

	//Create delta_t variable
	deltat = (double)dt/(double)DT_SECOND;
	deltath = deltat/2.0;

	// Implementiaon of the PI controller to control Pref input of each GGOV1
	// Initialization items
	if ((delta_time==0) && (iteration_count_val==0))	//First run of new delta call
	{
		// Initialize dynamics
		for (int index = 0; index < dgs->hit_count; index++) {

			init_dynamics(ctrlGen[index]->curr_state, index);

			// Initialize rotor variable
			prev_Pref_val[index] = ctrlGen[index]->curr_state->Pref_ctrl;
			prev_Vset_val[index] = ctrlGen[index]->curr_state->Vset_ref;

			// Replicate curr_state into next
			memcpy(ctrlGen[index]->next_state,ctrlGen[index]->curr_state,sizeof(CTRL_VARS));
		}

	} // End first pass and timestep of deltamode (initial condition stuff)

	// See what we're on, for tracking
	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

	// Check pass
	if (pass_mod==0)	// Predictor pass
	{
		// Update Pref of the GGOV1
		for (int index = 0; index < dgs->hit_count; index++) {

			// Call dynamics
			apply_dynamics(ctrlGen[index]->curr_state,&predictor_vals,deltat, index);

			// Apply prediction update
			ctrlGen[index]->next_state->x = ctrlGen[index]->curr_state->x + predictor_vals.x*deltat;
			ctrlGen[index]->next_state->Pref_ctrl = ctrlGen[index]->next_state->x + predictor_vals.x*(kp/ki);

			pDG[index]->gen_base_set_vals.Pref = ctrlGen[index]->next_state->Pref_ctrl;

			// Apply prediction update
			ctrlGen[index]->next_state->x_QV = ctrlGen[index]->curr_state->x_QV + predictor_vals.x_QV*deltat;
			ctrlGen[index]->next_state->Vset_ctrl = ctrlGen[index]->next_state->x_QV + predictor_vals.x_QV*(kp_QV/ki_QV);

			pDG[index]->gen_base_set_vals.vset = ctrlGen[index]->next_state->Vset_ctrl;
		}

		return SM_DELTA_ITER;	//Reiterate - to get us to corrector pass

	}
	else	// Corrector pass
	{
		// Update Pref of the GGOV1
		for (int index = 0; index < dgs->hit_count; index++) {

			// Call dynamics
			apply_dynamics(ctrlGen[index]->next_state,&corrector_vals,deltat, index);

			// Reconcile updates update
			ctrlGen[index]->next_state->x = ctrlGen[index]->curr_state->x + (predictor_vals.x + corrector_vals.x)*deltath;
			ctrlGen[index]->next_state->Pref_ctrl = ctrlGen[index]->next_state->x + (predictor_vals.x + corrector_vals.x)*0.5*(kp/ki);

			pDG[index]->gen_base_set_vals.Pref = ctrlGen[index]->next_state->Pref_ctrl;

			// Apply prediction update
			ctrlGen[index]->next_state->x_QV = ctrlGen[index]->curr_state->x_QV + (predictor_vals.x_QV + corrector_vals.x_QV)*deltat;
			ctrlGen[index]->next_state->Vset_ctrl = ctrlGen[index]->next_state->x_QV + (predictor_vals.x_QV + corrector_vals.x_QV)*0.5*(kp_QV/ki_QV);

			pDG[index]->gen_base_set_vals.vset = ctrlGen[index]->next_state->Vset_ctrl;

			// Copy everything back into curr_state, since we'll be back there
			memcpy(ctrlGen[index]->curr_state,ctrlGen[index]->next_state,sizeof(CTRL_VARS));

			// Check convergence - pick the max difference
			temp_double = fmax(temp_double, fabs(ctrlGen[index]->curr_state->Pref_ctrl - prev_Pref_val[index]));
			temp_double = fmax(temp_double, fabs(ctrlGen[index]->curr_state->Vset_ctrl - prev_Vset_val[index]));

			// Update tracking variable
			prev_Pref_val[index] = ctrlGen[index]->curr_state->Pref_ctrl;
			prev_Vset_val[index] = ctrlGen[index]->curr_state->Vset_ref;

		}

		// Determine our desired state - if rotor speed is settled, exit
		if (temp_double<=controller_Pref_convergence_criterion)
		{
			// Ready to leave Delta mode
			return SM_EVENT;
		}
		else	// Not "converged" -- I would like to do another update
		{
			return SM_DELTA;	//Next delta update
								//Could theoretically request a reiteration, but we're not allowing that right now
		}

	}// End corrector pass
}

STATUS controller_dg::post_deltaupdate(complex *useful_value, unsigned int mode_pass)
{
	return SUCCESS;	//Allways succeeds right now
}

//Applies dynamic equations for predictor/corrector sets
//Functionalized since they are identical
//Returns a SUCCESS/FAIL
//curr_time is the current states/information
//curr_delta is the calculated differentials
STATUS controller_dg::apply_dynamics(CTRL_VARS *curr_time, CTRL_VARS *curr_delta, double deltaT, int index)
{

	// Control of the f/p droop
	curr_delta->x = gain * (curr_time->wref_ctrl-*mapped_freq_variable*2*PI)/omega_ref*ki;

	// Control of the v/q droop
	double V_pos_temp = (GenPobj[index]->voltage[0].Mag() + GenPobj[index]->voltage[1].Mag() + GenPobj[index]->voltage[2].Mag())/3.0;
	curr_delta->x_QV = gain_QV * (curr_time->Vset_ref-V_pos_temp)/nominal_voltage*ki_QV;

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
STATUS controller_dg::init_dynamics(CTRL_VARS *curr_time, int index)
{
	OBJECT *obj = NULL;
	double omega = *mapped_freq_variable*2*PI; // not used here since speed of each generator deirectly used

	curr_time->w_measured = pDG[index]->curr_state.omega/pDG[index]->omega_ref;
	curr_time->x = pDG[index]->gen_base_set_vals.Pref;
	curr_time->wref_ctrl = omega_ref;
	curr_time->Pref_ctrl = curr_time->x;

	// If Vset_ref has not been set yet, define it as the positive voltage values measured before entering the delta mode
	if (curr_time->Vset_ref < 0) {
		// Obatain DG terminal positive voltage
		curr_time->Vset_ref = (GenPobj[index]->voltage[0].Mag() + GenPobj[index]->voltage[1].Mag() + GenPobj[index]->voltage[2].Mag())/3.0;
		nominal_voltage = GenPobj[index]->nominal_voltage;
	}
	curr_time->x_QV = pDG[index]->gen_base_set_vals.vset;
	curr_time->Vset_ctrl = curr_time->x_QV;

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller_dg(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller_dg::oclass);
		if (*obj!=NULL)
		{
			controller_dg *my = OBJECTDATA(*obj,controller_dg);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(controller_dg);
}

EXPORT int init_controller_dg(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,controller_dg)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(controller_dg);
}

EXPORT TIMESTAMP sync_controller_dg(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = TS_INVALID;
	controller_dg *my = OBJECTDATA(obj,controller_dg);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t1 = my->presync(obj->clock,t0);
			break;
		case PC_BOTTOMUP:
			t1 = my->sync(obj->clock,t0);
			break;
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock,t0);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
	}
	SYNC_CATCHALL(controller_dg);
	return t1;
}

EXPORT SIMULATIONMODE interupdate_controller_dg(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	controller_dg *my = OBJECTDATA(obj,controller_dg);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_controller_dg(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

EXPORT STATUS postupdate_controller_dg(OBJECT *obj, complex *useful_value, unsigned int mode_pass)
{
	controller_dg *my = OBJECTDATA(obj,controller_dg);
	STATUS status = FAILED;
	try
	{
		status = my->post_deltaupdate(useful_value, mode_pass);
		return status;
	}
	catch (char *msg)
	{
		gl_error("postupdate_controller_dg(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}


