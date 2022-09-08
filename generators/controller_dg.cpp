/*
 * controller_dg.cpp
 *
 *  Created on: Sep 21, 2016
 *      Author: tang526
 */

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gld_complex.h"

#include "controller_dg.h"

CLASS* controller_dg::oclass = nullptr;
controller_dg *controller_dg::defaults = nullptr;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

controller_dg::controller_dg(MODULE *mod)
{
	if(oclass == nullptr)
	{
		oclass = gl_register_class(mod,"controller_dg",sizeof(controller_dg),passconfig|PC_AUTOLOCK);
		if (oclass==nullptr)
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
			nullptr) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		defaults = this;

		memset(this,0,sizeof(controller_dg));

		if (gl_publish_function(oclass,	"interupdate_controller_object", (FUNCTIONADDR)interupdate_controller_dg)==nullptr)
			GL_THROW("Unable to publish controller_dg deltamode function");
		if (gl_publish_function(oclass,	"postupdate_controller_object", (FUNCTIONADDR)postupdate_controller_dg)==nullptr)
			GL_THROW("Unable to publish controller_dg deltamode function");

    }
}

int controller_dg::create(void)
{
	// Give default values for frequency reference
	omega_ref=2*PI*60;

	deltamode_inclusive = false;
	mapped_freq_variable = nullptr;

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

	pDG = nullptr;
	GenPobj = nullptr;
	Switch_Froms = nullptr;
	pSwitchObjs = nullptr;

	return 1;
}

/* Object initialization is called once after all object have been created */
int controller_dg::init(OBJECT *parent)
{
	OBJECT *thisobj = OBJECTHDR(this);
	OBJECT *obj;
	gld_property *temp_prop;
	gld_object *temp_from, *temp_to;

	//See if the global flag is set - if so, add the object flag
	if (all_generator_delta)
	{
		obj->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((thisobj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	// Obtain the pointer to the diesel_dg objects
	if(controlled_dgs[0] == '\0'){
		dgs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"diesel_dg",FT_END);
		if(dgs == nullptr){
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
		if(dgs == nullptr){
			gl_error("Although controller given group id, no dgs with given group id found.");
			/*  TROUBLESHOOT
			While trying to put together a list of all dg objects with the specified controller groupid, no such dg objects were found.
			*/

			return 0;
		}
	}

	// Creat controller for each generator
	obj = nullptr;
	int index = 0;
	if(dgs != nullptr){
		pDG = (DG_VARS *)gl_malloc(dgs->hit_count*sizeof(DG_VARS));
		if(pDG == nullptr){
			gl_error("Failed to allocate diesel_dg array.");
			return TS_NEVER;
		}

		GenPobj = (NODE_VARS *)gl_malloc(dgs->hit_count*sizeof(NODE_VARS));

		while(obj = gl_find_next(dgs,obj)){

			//Verify it is a proper object
			if (!gl_object_isa(obj,"diesel_dg","generators"))
			{
				gl_error("Invalid diesel_dg object");
				return 0;
			}

			// Store generator data pointer
			if(index >= dgs->hit_count){
				break;
			}

			// Store each generator parented node name,
			// so that the corresponding connected switch can be found
			pDG[index].parent = obj->parent;
			pDG[index].obj = obj;

			if(pDG[index].parent == nullptr){
				gl_error("Failed to find diesel_dg parent node object.");
				return 0;
			}

			//Map properties
			pDG[index].Pref_prop = map_double_value(obj,"Pref");
			pDG[index].Vset_prop = map_double_value(obj,"Vset");
			pDG[index].omega_prop = map_double_value(obj,"rotor_speed");

			//Pull the values
			pDG[index].Pref = pDG[index].Pref_prop->get_double();
			pDG[index].Vset = pDG[index].Vset_prop->get_double();
			pDG[index].omega = pDG[index].omega_prop->get_double();

			//Pull the omega_ref (one time)
			temp_prop = map_double_value(obj,"omega_ref");
			pDG[index].omega_ref = temp_prop->get_double();

			//clear it
			delete temp_prop;

			//Pull parent properties
			GenPobj[index].obj = pDG[index].parent;
			GenPobj[index].voltage_A_prop = map_complex_value(GenPobj[index].obj,"voltage_A");
			GenPobj[index].voltage_B_prop = map_complex_value(GenPobj[index].obj,"voltage_B");
			GenPobj[index].voltage_C_prop = map_complex_value(GenPobj[index].obj,"voltage_C");

			//Get initial values
			GenPobj[index].voltage_A = GenPobj[index].voltage_A_prop->get_complex();
			GenPobj[index].voltage_B = GenPobj[index].voltage_B_prop->get_complex();
			GenPobj[index].voltage_C = GenPobj[index].voltage_C_prop->get_complex();

			//Get nominal voltage (temporary)
			temp_prop = map_double_value(GenPobj[index].obj,"nominal_voltage");
			GenPobj[index].nominal_voltage = temp_prop->get_double();

			//Clear it
			delete temp_prop;

			++index;
		}

		//Assign space for the array of CTRL_Gen
		ctrlGen = (CTRL_Gen **)gl_malloc(dgs->hit_count*sizeof(CTRL_Gen*));
		prev_Pref_val = (double*)gl_malloc(dgs->hit_count*sizeof(double));
		prev_Vset_val = (double*)gl_malloc(dgs->hit_count*sizeof(double));

		//See if it worked
		if (ctrlGen == nullptr || prev_Pref_val == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		// Assign space for each pointer
		for (int i = 0; i < dgs->hit_count; i++) {
			ctrlGen[i] = (CTRL_Gen *)gl_malloc(sizeof(CTRL_Gen));
			//See if it worked
			if (ctrlGen[i] == nullptr)
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
	obj = nullptr;
	switches = gl_find_objects(FL_NEW,FT_CLASS,SAME,"switch",FT_END);
	if(switches == nullptr){
		gl_error("No switch objects were found.");
		return 0;
		/* TROUBLESHOOT
		No switch objects were found in your .glm file.
		*/
	}
	index = 0;
	if(switches != nullptr){

		//Allocate switch "from" node items
		Switch_Froms = (NODE_VARS *)gl_malloc(dgs->hit_count*sizeof(NODE_VARS));
		if (Switch_Froms == nullptr)
		{
			gl_error("Failed to allocate switch array.");
			return 0;
		}

		pSwitchObjs = (SWITCH_VARS *)gl_malloc(dgs->hit_count*sizeof(pSwitchObjs));
		if (pSwitchObjs == nullptr)
		{
			gl_error("Failed to allocate switch array");
			return 0;
		}

		while(obj = gl_find_next(switches,obj)){
			if(index >= switches->hit_count){
				break;
			}

			// Obtain the voltage and frequency values for each switch
			// Get the switch from node object

			/* Get the from node */
			temp_prop = new gld_property(obj, "from");
			// Double check the validity
			if (!temp_prop->is_valid() || !temp_prop->is_objectref())
			{
				GL_THROW("controller_dg:%d %s Failed to map the switch property 'from'!",thisobj->id, (thisobj->name ? thisobj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the a property from the switch object, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			temp_from = temp_prop->get_objectref();
			delete temp_prop;

			/* Get the from node */
			temp_prop = new gld_property(obj, "to");
			// Double check the validity
			if (!temp_prop->is_valid() || !temp_prop->is_objectref())
			{
				GL_THROW("controller_dg:%d %s Failed to map the switch property 'to'!",thisobj->id, (thisobj->name ? thisobj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the a property to the switch object, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			temp_to = temp_prop->get_objectref();
			delete temp_prop;

			bool found = false;
			OBJECT *fnd_obj = nullptr;
			for (int i = 0; i < dgs->hit_count; i++) {
				if (strcmp(temp_from->get_name(), pDG[i].parent->name) == 0 || strcmp(temp_to->get_name(), pDG[i].parent->name) == 0) {
					found = true;
					fnd_obj = pDG[i].parent;
					break;
				}
			}
			if (found) {
				// Store the switches that connected to the generatos
				//Get node properties
				Switch_Froms[dgswitchFound].obj = fnd_obj;
				Switch_Froms[dgswitchFound].voltage_A_prop = map_complex_value(fnd_obj,"voltage_A");
				Switch_Froms[dgswitchFound].voltage_B_prop = map_complex_value(fnd_obj,"voltage_B");
				Switch_Froms[dgswitchFound].voltage_C_prop = map_complex_value(fnd_obj,"voltage_C");
				Switch_Froms[dgswitchFound].voltage_A = Switch_Froms[dgswitchFound].voltage_A_prop->get_complex();
				Switch_Froms[dgswitchFound].voltage_B = Switch_Froms[dgswitchFound].voltage_B_prop->get_complex();
				Switch_Froms[dgswitchFound].voltage_C = Switch_Froms[dgswitchFound].voltage_C_prop->get_complex();

				//Get nominal voltage (temporary)
				temp_prop = map_double_value(fnd_obj,"nominal_voltage");
				Switch_Froms[dgswitchFound].nominal_voltage = temp_prop->get_double();

				//Clear it
				delete temp_prop;

				//Store the explicit switch
				pSwitchObjs[dgswitchFound].obj = obj;
				pSwitchObjs[dgswitchFound].power_out_A_prop = map_complex_value(obj,"power_out_A");
				pSwitchObjs[dgswitchFound].power_out_B_prop = map_complex_value(obj,"power_out_B");
				pSwitchObjs[dgswitchFound].power_out_C_prop = map_complex_value(obj,"power_out_C");
				pSwitchObjs[dgswitchFound].power_in_prop = map_complex_value(obj,"power_in");

				//Map status
				pSwitchObjs[dgswitchFound].status_prop = new gld_property(obj,"status");

				//Check it
				if (!pSwitchObjs[dgswitchFound].status_prop->is_valid() || !pSwitchObjs[dgswitchFound].status_prop->is_enumeration())
				{
					GL_THROW("controller_dg:%d %s Failed to map the switch property 'status'!",thisobj->id, (thisobj->name ? thisobj->name : "Unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the a property to the switch object, an error occurred.  Please try again.
					If the error persists, please submit your GLM and a bug report to the ticketing system.
					*/
				}

				//Pull the values
				pSwitchObjs[dgswitchFound].power_out_A = pSwitchObjs[dgswitchFound].power_out_A_prop->get_complex();
				pSwitchObjs[dgswitchFound].power_out_B = pSwitchObjs[dgswitchFound].power_out_B_prop->get_complex();
				pSwitchObjs[dgswitchFound].power_out_C = pSwitchObjs[dgswitchFound].power_out_C_prop->get_complex();
				pSwitchObjs[dgswitchFound].power_in = pSwitchObjs[dgswitchFound].power_in_prop->get_complex();
				pSwitchObjs[dgswitchFound].status_val = pSwitchObjs[dgswitchFound].status_prop->get_enumeration();

				dgswitchFound++;
			}

			++index;
		}
	}

	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (!enable_subsecond_models)
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
			if (mapped_freq_variable == nullptr)
			{
				GL_THROW("controller_dg:%s - Failed to map frequency checking variable from powerflow for deltamode",obj->name?obj->name:"unnamed");
				//Defined above
			}

			gen_object_count++;	//Increment the counter
		}
	}//End deltamode inclusive
	else	//Not enabled for this model
	{
		if (enable_subsecond_models)
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
	if (first_run)	//First run
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models )	//We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects==nullptr)) && enable_subsecond_models)
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
			if (delta_functions[gen_object_current] == nullptr)
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
			if (post_delta_functions[gen_object_current] == nullptr)
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
		deltamode_endtime_dbl = TS_NEVER_DBL;
	}

	//
	if (first_run)	//Final init items - namely deltamode supersecond exciter
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

	gld::complex vtemp[3];
	double nominal_voltage;
	FUNCTIONADDR funadd = nullptr;
	int return_val;
	gld_wlock *test_rlock = nullptr;
	unsigned char openPhases[] = {0x04, 0x02, 0x01};

	// Control of the generator switch
	// Detect abnormal feeder frequency, switch terminal voltage, and reverse real power flow from generators (absorbing real power)
	// Switch will be turned off. Delay has been added to avoid simultaneously turning off all switches
	OBJECT *obj = nullptr;
	for (int index = 0; index < dgswitchFound; index++) {

		// Obtain the switch object so that function can be called
		obj = pSwitchObjs[index].obj;

		// Obtain the voltage and frequency values for each switch
		//Pull the switch voltages
		Switch_Froms[index].voltage_A = Switch_Froms[index].voltage_A_prop->get_complex();
		Switch_Froms[index].voltage_B = Switch_Froms[index].voltage_B_prop->get_complex();
		Switch_Froms[index].voltage_C = Switch_Froms[index].voltage_C_prop->get_complex();

		// Check switch from node voltages
		vtemp[0] = Switch_Froms[index].voltage_A;
		vtemp[1] = Switch_Froms[index].voltage_B;
		vtemp[2] = Switch_Froms[index].voltage_C;
		nominal_voltage = Switch_Froms[index].nominal_voltage;

		//Pull the switch properties
		pSwitchObjs[index].power_out_A = pSwitchObjs[index].power_out_A_prop->get_complex();
		pSwitchObjs[index].power_out_B = pSwitchObjs[index].power_out_B_prop->get_complex();
		pSwitchObjs[index].power_out_C = pSwitchObjs[index].power_out_C_prop->get_complex();
		pSwitchObjs[index].power_in = pSwitchObjs[index].power_in_prop->get_complex();
		pSwitchObjs[index].status_val = pSwitchObjs[index].status_prop->get_enumeration();

		// Obtain switch real power value of each phase
		double total_phase_P = pSwitchObjs[index].power_in.Re();
		double phase_A_P = pSwitchObjs[index].power_out_A.Re();
		double phase_B_P = pSwitchObjs[index].power_out_B.Re();
		double phase_C_P = pSwitchObjs[index].power_out_C.Re();

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
		if ((pSwitchObjs[index].status_val == 1) &&
			(((*mapped_freq_variable > omega_ref/(2.0*PI)*1.01) || vtemp[0].Mag() > 1.2*nominal_voltage || vtemp[1].Mag() > 1.2*nominal_voltage || vtemp[2].Mag() > 1.2*nominal_voltage)) ||
			(phase_A_P > 0 || phase_B_P > 0 || phase_C_P > 0))
		{

			if (!flag_switchOn || (controlTime == delta_time)) {

				//map the function
				funadd = (FUNCTIONADDR)(gl_get_function(obj,"change_switch_state"));

				//make sure it worked
				if (funadd==nullptr)
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

				// Set up flags and time control time every time the switch status is changed
				if (!flag_switchOn) {
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

			//Set value
			pDG[index].Pref = ctrlGen[index]->next_state->Pref_ctrl;
			pDG[index].Pref_prop->setp<double>(pDG[index].Pref,*test_rlock);

			// Apply prediction update
			ctrlGen[index]->next_state->x_QV = ctrlGen[index]->curr_state->x_QV + predictor_vals.x_QV*deltat;
			ctrlGen[index]->next_state->Vset_ctrl = ctrlGen[index]->next_state->x_QV + predictor_vals.x_QV*(kp_QV/ki_QV);

			pDG[index].Vset = ctrlGen[index]->next_state->Vset_ctrl;
			pDG[index].Vset_prop->setp<double>(pDG[index].Vset,*test_rlock);
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

			//Set value
			pDG[index].Pref = ctrlGen[index]->next_state->Pref_ctrl;
			pDG[index].Pref_prop->setp<double>(pDG[index].Pref,*test_rlock);

			// Apply prediction update
			ctrlGen[index]->next_state->x_QV = ctrlGen[index]->curr_state->x_QV + (predictor_vals.x_QV + corrector_vals.x_QV)*deltat;
			ctrlGen[index]->next_state->Vset_ctrl = ctrlGen[index]->next_state->x_QV + (predictor_vals.x_QV + corrector_vals.x_QV)*0.5*(kp_QV/ki_QV);

			pDG[index].Vset = ctrlGen[index]->next_state->Vset_ctrl;
			pDG[index].Vset_prop->setp<double>(pDG[index].Vset,*test_rlock);

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

STATUS controller_dg::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
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

	//Pull values
	GenPobj[index].voltage_A = GenPobj[index].voltage_A_prop->get_complex();
	GenPobj[index].voltage_B = GenPobj[index].voltage_B_prop->get_complex();
	GenPobj[index].voltage_C = GenPobj[index].voltage_C_prop->get_complex();

	// Control of the v/q droop
	double V_pos_temp = (GenPobj[index].voltage_A.Mag() + GenPobj[index].voltage_B.Mag() + GenPobj[index].voltage_C.Mag())/3.0;
	curr_delta->x_QV = gain_QV * (curr_time->Vset_ref-V_pos_temp)/nominal_voltage*ki_QV;

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
STATUS controller_dg::init_dynamics(CTRL_VARS *curr_time, int index)
{
	OBJECT *obj = nullptr;
	double omega = *mapped_freq_variable*2*PI; // not used here since speed of each generator deirectly used

	//Get current frequency
	pDG[index].omega = pDG[index].omega_prop->get_double();
	pDG[index].Pref = pDG[index].Pref_prop->get_double();
	pDG[index].Vset = pDG[index].Vset_prop->get_double();

	curr_time->w_measured = pDG[index].omega/pDG[index].omega_ref;
	curr_time->x = pDG[index].Pref;
	curr_time->wref_ctrl = omega_ref;
	curr_time->Pref_ctrl = curr_time->x;

	// If Vset_ref has not been set yet, define it as the positive voltage values measured before entering the delta mode
	if (curr_time->Vset_ref < 0) {

		//Pull values
		GenPobj[index].voltage_A = GenPobj[index].voltage_A_prop->get_complex();
		GenPobj[index].voltage_B = GenPobj[index].voltage_B_prop->get_complex();
		GenPobj[index].voltage_C = GenPobj[index].voltage_C_prop->get_complex();

		// Obatain DG terminal positive voltage
		curr_time->Vset_ref = (GenPobj[index].voltage_A.Mag() + GenPobj[index].voltage_B.Mag() + GenPobj[index].voltage_C.Mag())/3.0;
		nominal_voltage = GenPobj[index].nominal_voltage;
	}
	curr_time->x_QV = pDG[index].Vset;
	curr_time->Vset_ctrl = curr_time->x_QV;

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//Map Complex value
gld_property *controller_dg::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("controller_dg:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in controller_dg.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *controller_dg::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("controller_dg:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in controller_dg.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller_dg(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller_dg::oclass);
		if (*obj!=nullptr)
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
		if (obj!=nullptr)
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

EXPORT STATUS postupdate_controller_dg(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
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


