#include "sec_control.h"

CLASS *sec_control::oclass = NULL;
sec_control *sec_control::defaults = NULL;

static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
sec_control::sec_control(MODULE *module)
{
	if (oclass == NULL)
	{
		oclass = gl_register_class(module, "sec_control", sizeof(sec_control), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == NULL)
			throw "unable to register class sec_control";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			//**************** Sample/test published property - remove ****************//
			PT_complex, "test_pub_prop[kW]", PADDR(test_published_property), PT_DESCRIPTION, "test published property - has units of kW",
			PT_char1024, "participant_input", PADDR(participant_input), PT_DESCRIPTION, "command string for creating/modifing secondary controller participants",
			//PID controller input
			PT_double, "f0[Hz]", PADDR(f0), PT_DESCRIPTION, "Nominal frequency in Hz",
			PT_double, "underfrequency_limit[Hz]", PADDR(underfrequency_limit), PT_DESCRIPTION, "Maximum positive input limit to PID controller is f0 - underfreuqnecy_limit",
			PT_double, "overfrequency_limit[Hz]", PADDR(overfrequency_limit), PT_DESCRIPTION, "Maximum negative input limit to PID controller is f0 - overfrequency_limit",
			PT_double, "deadband[Hz]", PADDR(deadband), PT_DESCRIPTION, "Deadpand for PID controller input in Hz",
			//PID controller
			PT_double, "kpPID[MW/Hz]", PADDR(kpPID), PT_DESCRIPTION, "PID proportional gain in MW/Hz",
			PT_double, "kiPID[MW/Hz/s]", PADDR(kiPID), PT_DESCRIPTION, "PID integral gain in MW/Hz/s",
			PT_double, "kdPID[MW/Hz*s]", PADDR(kdPID), PT_DESCRIPTION, "PID derivative gain in MW/Hz*s",
			
			PT_enumeration,"anti_windup",PADDR(anti_windup), PT_DESCRIPTION, "Integrator windup handling",
				PT_KEYWORD,"NONE",(enumeration)NONE,PT_DESCRIPTION,"No anti-windup active",
				PT_KEYWORD,"ZERO_IN_DEADBAND",(enumeration)ZERO_IN_DEADBAND,PT_DESCRIPTION,"Zero integrator when frequency is within deadband",
				PT_KEYWORD,"FEEDBACK",(enumeration)FEEDBACK,PT_DESCRIPTION,"Feedback difference between PIDout and signal passed to generators",

			//
			PT_double, "Ts[s]", PADDR(Ts), PT_DESCRIPTION, "Secondary controller sampling period in sec.",
			//
			PT_double, "alpha_tol", PADDR(alpha_tol), PT_DESCRIPTION, "Tolerance for participation values not summing to exactly one.",
			//
			PT_bool, "sampleflag", PADDR(sampleflag), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Boolean flag whether output should be published or not by secondary controller",
			PT_double, "sample_time", PADDR(sample_time), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Next update time for the secondary controller",
			PT_bool, "deadbandflag", PADDR(deadbandflag), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "false=frequency within deadband, true=frequency outside of deadband",
			//States
			PT_double, "deltaf(t)[Hz]", PADDR(curr_state.deltaf[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Frequency error in current iteration [Hz]",
			PT_double, "deltaf(t-1)[Hz]", PADDR(curr_state.deltaf[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Frequency error in previous timestep [Hz]",
			PT_double, "dxi[MW]", PADDR(curr_state.dxi), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Change in PID integrator output",
			PT_double, "xi[MW]", PADDR(curr_state.xi), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "PID integrator output",
			PT_double, "PIDout[MW]", PADDR(curr_state.PIDout), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "PID output",
			NULL) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		defaults = this;

		memset(this, 0, sizeof(sec_control));

		//**************** Function publish for deltamode - postupdate may not be needed ****************//
		if (gl_publish_function(oclass, "preupdate_gen_object", (FUNCTIONADDR)preupdate_sec_control) == NULL)
			GL_THROW("Unable to publish sec_control deltamode function");
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_sec_control) == NULL)
			GL_THROW("Unable to publish sec_control deltamode function");
		if (gl_publish_function(oclass, "postupdate_gen_object", (FUNCTIONADDR)postupdate_sec_control) == NULL)
			GL_THROW("Unable to publish sec_control deltamode function");
	}
}

//Isa function for identification
int sec_control::isa(char *classname)
{
	return strcmp(classname,"sec_control")==0;
}

/* Object creation is called once for each object that is created by the core */
int sec_control::create(void)
{
	////////////////////////////////////////////////////////
	// DELTA MODE
	////////////////////////////////////////////////////////
	deltamode_inclusive = false; //By default, don't be included in deltamode simulations
	first_sync_delta_enabled = false;

	//**************** created/default values go here ****************//
	test_published_property = complex(-1.0,-1.0);	//Flag value for example

	pFrequency = NULL;
	memset(participant_input,'\0', sizeof(char1024)); //initialize to empty

	//flag vlaues to be checked later in init.
	// These are intended to be IMPOSSIBLE values so that there is no way they
	// would be set as such via glm
	alpha_tol = -1.0; //Note: should actually be > 0
	dp_dn_default = -1.0;  //Note: should actually be > 0
	dp_up_default = -1.0; //Note: should actually be > 0
	Tlp_default = -1.0; //Note: should actually be >= dt which is > 0
	f0 = -1.0; //Note: should actually be > 0
	underfrequency_limit = -1.0; //Note: should actually be > 0
	overfrequency_limit = -1.0; //Note: should actually be > 0
	frequency_delta_default = -1.0; //Note: should actually be > 0
	deadband = -1.0; //Note: should actually be > 0
	kpPID = -1.0; //Note: should actually be >= 0
	kiPID = -1.0; //Note: should actually be >= 0
	kdPID = -1.0; //Note: should actually be >= 0
	Ts = -1.0; //Note: should actually be >= dt which is > 0

	curr_dt = -1.0; // indicates that deltat is not available.
	anti_windup = NONE;
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int sec_control::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	//Deferred initialization code
	if (parent != NULL)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("sec_control::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	//Map up the frequency pointer
	pFrequency = new gld_property(parent,"measured_frequency");

	//Make sure it worked
	if ((pFrequency->is_valid() != true) || (pFrequency->is_double() != true))
	{
		GL_THROW("sec_control:%s failed to map measured_frequency variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
		/*  TROUBLESHOOT
		While attempting to map the frequency variable from the parent node, an error was encountered.  Please try again.  If the error
		persists, please report it with your GLM via the issues tracking system.
		*/
	}

	//Pull the current value, because
	fmeas = pFrequency->get_double();

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true; //Set the flag and off we go
	}


	///////////////////////////////////////////////////////////////////////////
	// DELTA MODE
	///////////////////////////////////////////////////////////////////////////
	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models != true)
		{
			gl_warning("sec_control:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The sec_control object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			gen_object_count++; //Increment the counter
			first_sync_delta_enabled = true;
		}
		//Default else - don't do anything
	}	 //End deltamode inclusive
	else //This particular model isn't enabled
	{
		if (enable_subsecond_models == true)
		{
			gl_warning("sec_control:%d %s - Deltamode is enabled for the module, but not this controller!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The sec_control is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this sec_control may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
		
	}

	//**************** Other initialization or "input check" items ****************//
	//Example check
	if (test_published_property.Re() == -1.0)	//Not set in GLM, so apply a default
	{
		//Arbitrary
		test_published_property = complex(1000.0,1000.0);
	}


	//both Tlp_default and Ts are not altered here.
	//Reason is, that their default value should be dt, which is not known at this point.
	//Keeping them -1 is an indication for later that they should be adjusted.
	//Similarly for dp_dn_default and dp_up_default, we'd like to pick those on an object by object basis, e.g. rating
	init_check(alpha_tol, -1.0, 0.001); 
	init_check(f0, -1.0, 60); 
	init_check(underfrequency_limit, -1.0, 57.0); 
	init_check(overfrequency_limit, -1.0, 62.0);
	init_check(frequency_delta_default, -1.0, 2.0);
	init_check(deadband, -1.0, 0.2);  //200mHz deadband
	// default PID is only integrator
	init_check(kpPID, -1.0, 0);
	init_check(kiPID, -1.0, 10);
	init_check(kdPID, -1.0, 0);

	parse_praticipant_input(participant_input); //parse the participating objects

	return 1;
}

// Check if value changed from the flaged one in creat.
// If not, set the default value.
void sec_control::init_check(double &var, double ck, double def)
{
	if (var == ck) // Not set in GLM so apply default
	{
		var = def;
	}
}

TIMESTAMP sec_control::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	//**************** Initial top-down pass for QSTS - typically empty accumulators and re-init working variable for current pass ****************//

	return t2;
}

TIMESTAMP sec_control::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret_value;

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//Deltamode check/init items
	if (first_sync_delta_enabled == true) //Deltamode first pass
	{
		//TODO: LOCKING!
		if ((deltamode_inclusive == true) && (enable_subsecond_models == true)) //We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects == NULL)) && (enable_subsecond_models == true))
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (gen_object_current >= gen_object_count)
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
			delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "interupdate_gen_object"));

			//Make sure it worked
			if (delta_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			/* post_delta_functions are often removed, since they don't typically do anything - empty it out/delete it if this is the case! */
			//Map up the function for postupdate
			post_delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "postupdate_gen_object"));

			//Make sure it worked
			if (post_delta_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map post-deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the postupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Map up the function for postupdate
			delta_preupdate_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "preupdate_gen_object"));

			//Make sure it worked
			if (delta_preupdate_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map pre-deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the preupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Update pointer
			gen_object_current++;

			//Flag it
			first_sync_delta_enabled = false;

		}	 //End deltamode specials - first pass
		else //Somehow, we got here and deltamode isn't properly enabled...odd, just deflag us
		{
			first_sync_delta_enabled = false;
		}
	} //End first delta timestep
	//default else - either not deltamode, or not the first timestep

	//**************** Bottom-up execution events for QSTS - usually the actual "logic" before a powerflow call ****************//

	//Return
	if (tret_value != TS_NEVER)
	{
		return -tret_value;
	}
	else
	{
		return TS_NEVER;
	}
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP sec_control::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER; //By default, we're done forever!

	//**************** second top-down call - items that need to be reconciled after powerflow has executed ***************//

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Preupdate
STATUS sec_control::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	//**************** Deltamode start initialization function, if any ****************//
	
	// clear curr_state structure
	curr_state.deltaf[0] = 0;
	curr_state.deltaf[1] = 0;
	curr_state.dxi = 0;
	curr_state.xi = 0;
	curr_state.PIDout = 0;

	// clear next_state structure
	memcpy(&next_state, &curr_state, sizeof(SEC_CNTRL_STATE));

	// initialize exit to qsts mode variables
	qsts_time = -1;
	deltaf_flag = false;

	deadbandflag = false;

	// zero participant setpoint change and get initial output
	for (auto & obj : part_obj){
		obj.dP[0] = 0;
		obj.dP[1] = 0;
		obj.dP[2] = 0;
		obj.ddP[0] = 0;
		obj.ddP[1] = 0;

		obj.pinit = get_pelec(obj); //get initial schedule
	}

	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

// Checks tlp against deltat 
void sec_control::participant_tlp_check(SEC_CNTRL_PARTICIPANT *obj)
{
	if (curr_dt > 0) // in delta mode therefore deltat is present
	{	
		// checks if Tlp was somehow set (could be by setting the Tlp_default value)
		init_check(obj->Tlp, -1, curr_dt); 
		if (obj->Tlp < curr_dt) // Specified but is too small
		{
			gl_warning("sec_control: participant %s: Tlp = %0.5f < dt = %0.5f. Setting Tlp = dt", obj->ptr->name, obj->Tlp, curr_dt);
			obj->Tlp = curr_dt;
		}
	}
}

// Checks tlp against deltat 
void sec_control::participant_tlp_check(SEC_CNTRL_PARTICIPANT & obj)
{
	SEC_CNTRL_PARTICIPANT * ptr = &obj;
	participant_tlp_check(ptr);
}


//Module-level call
SIMULATIONMODE sec_control::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltath;
	OBJECT *obj = OBJECTHDR(this);
	double debugval;

	SIMULATIONMODE simmode_return_value = SM_EVENT;

	//Get timestep value - used for predictor/corrector calculations
	deltat = (double)dt / (double)DT_SECOND;
	deltath = deltat / 2.0;
	curr_dt = deltat;

	//Update time tracking variables
	curr_timestamp_dbl = gl_globaldeltaclock;
	
	
	//**************** Primary deltamode executor - dynamics usually go/get called here ****************//

	if (iteration_count_val == 0) // Predictor pass
	{
		// verify the that the sampling timeconstant is set and makes sense
		init_check(Ts, -1, deltat);
		if (Ts < deltat) // Ts was specified but is too small
		{
			gl_warning("sec_control: Ts = %0.5f < dt = %0.5f. Setting Ts = dt", Ts, deltat);
			Ts = deltat;
		}
		if (delta_time == 0)
		{
			// Some initialization on first deltamode pass
			// These are done here because in pre_deltaupdate we don't have deltat yet.
			sample_time = curr_timestamp_dbl + Ts;

			//check participant Tlps
			for (auto & obj : part_obj)
			{
				participant_tlp_check(obj);
			}
		}

		// check whether participation has been altered in any way
		parse_praticipant_input(participant_input);
		
		
		//Determine wheter to pass set point changes on this timestep or not
		// Note: passing changes in this iteration means that those changes will be
		// reflected in time curr_timestamp_dbl + deltat.
		// For example with t=0 and dt = 0.01 then by the end of this step we will reach t=0.01.
		// If we want to sample at t=0.01 then we need to sample in this pass i.e. at cur_timestamp_dbl+deltat 
		if ((curr_timestamp_dbl + deltat) >= sample_time)
		{
			sampleflag = true; 	//sample output on this timestep
			sample_time += Ts;  // set next timestep to sample
		}
		else
		{
			sampleflag = false;
		}

		// Get Frequency Deviation (sets curr_state.deltaf[0] and deadbandflag)
		get_deltaf();

		// === PID Controller
		curr_state.dxi = kiPID*curr_state.deltaf[0];
		next_state.xi = curr_state.xi + deltat*curr_state.dxi;
		switch (anti_windup)
		{
		case ZERO_IN_DEADBAND:
			if (!deadbandflag)
			{
				// if frequency is within deadband, reset the integrator
				next_state.xi = 0;
			}
			break;
		case FEEDBACK:
			// feedback difference between PIDout and actual "actuated" output
			curr_state.dxi += curr_state.xi*(sampleflag * deadbandflag - 1);
			next_state.xi = curr_state.xi + deltat*curr_state.dxi;
			break;
		default:
			break;
		}
		next_state.PIDout = next_state.xi + (kpPID + kdPID/deltat)*curr_state.deltaf[0] - kdPID/deltat*curr_state.deltaf[1];
		//=== End PID controller
		
		//iterate over participating objects
		for (auto & obj : part_obj){
			// update change for each participant based on participation factor alpha and low pass filter.
			obj.ddP[0] = (obj.alpha*next_state.PIDout - obj.dP[0])/obj.Tlp;
			obj.dP[1] = obj.dP[0] + deltat*obj.ddP[0];
			if (sampleflag && deadbandflag)
			{
				//***************code to update object setpoints********************
				// on the predictor pass we just want to update with the calculated value dP[1]
				update_pdisp(obj, obj.dP[1]);
			}
		}
		simmode_return_value = SM_DELTA_ITER;  //call iteration to get to corrector pass
	}
	else if (iteration_count_val == 1) // corrector pass
	{
		// Get Frequency Deviation (sets curr_state.deltaf[0] and deadbandflag)
		get_deltaf();
		
		//=== PID Controller
		next_state.dxi = kiPID*curr_state.deltaf[0];
		next_state.xi = curr_state.xi + deltath*(curr_state.dxi + next_state.dxi);
		switch (anti_windup)
		{
		case ZERO_IN_DEADBAND:
			if (!deadbandflag)
			{
				// if frequency is within deadband, reset the integrator
				next_state.xi = 0;
			}
			break;
		case FEEDBACK:
			// feedback difference between PIDout and actual "actuated" output
			next_state.dxi += curr_state.xi*(sampleflag * deadbandflag - 1);
			next_state.xi = curr_state.xi + deltath*(curr_state.dxi + next_state.dxi);
			break;
		default:
			break;
		}
		next_state.PIDout = next_state.xi + (kpPID + kdPID/deltat)*curr_state.deltaf[0] - kdPID/deltat*curr_state.deltaf[1];
		//=== End PID controller

		//iterate over participating objects
		for (auto & obj : part_obj){
			// update change for ieach participant based on participation factor alpha and low pass filter.
			obj.ddP[1] = 1/obj.Tlp*(obj.alpha*next_state.PIDout - obj.dP[1]);
			obj.dP[0] += deltath*(obj.ddP[0] + obj.ddP[1]);
			if (sampleflag && deadbandflag)
			{
				//***************code to update object setpoints********************
				// The actuall delta we want to pass is obj.dP[0] - obj.dP[1]
				// since obj.dP[1] was already passed in the predictor iteration
				// Since various limits can affect the passed value and what is actually set,
				// what we *really* want is obj.dP[0] - obj.dP[2], where obj.dP[2] stores the final
				// set value via the call to update_pdisp.
				update_pdisp(obj, obj.dP[0] - obj.dP[2]);
			}
		}

		// Determine desired simulation mode in next timestep
		if (deadbandflag)
		{
			// Frequency error is not within deadband, stay in deltamode
			simmode_return_value =  SM_DELTA;
			deltaf_flag = true;
			qsts_time = -1;
		}
		else 
		{
			// Frequency error within deadband, wait 2xTs and then switch to QSTS
			if (deltaf_flag)
			{
				if (qsts_time < 0){
					qsts_time = curr_timestamp_dbl + 2*Ts;
					simmode_return_value =  SM_DELTA;
				}
				else if (curr_timestamp_dbl < qsts_time){
					simmode_return_value =  SM_DELTA;
				}
				else{
					simmode_return_value = SM_EVENT;
				}
			}
			else{
				simmode_return_value = SM_EVENT;
			}
		}
		// store in case of further iterations
		prev_simmode_return_value = simmode_return_value;

		// State update for next timestep
		next_state.deltaf[1] = curr_state.deltaf[0]; //cycle frequency error state values
		memcpy(&curr_state, &next_state, sizeof(SEC_CNTRL_STATE));
	}
	else
	{
		// Do nothing
		simmode_return_value = prev_simmode_return_value;
	}
	
	return simmode_return_value;
}

void sec_control::update_pdisp(SEC_CNTRL_PARTICIPANT & obj, double val)
{
	if (val != 0)
	{
		gld_wlock *test_rlock;
		double pdisp = obj.pdisp->get_double();
		double poffset = obj.poffset->get_double();

		// if the change will take place as desired the total change
		// for the initial schedule, pinit, is:
		// + the current out (get_pelec)
		// + the proposed change (val)
		// - the original schedule (pinit) 
		// double dp_desired = pout + val - obj.pinit;

		// ========= Handle change limits (in units)
		//if (dp_desired > obj.dp_up){
		if ((val + poffset*obj.rate) > obj.dp_up)
		{
			gl_warning("sec_control: Object %s desired setpoint change of %0.6f MW, would result in a total change of %0.6f > than maximum %0.6f MW. Changing to limit.", obj.ptr->name, val, val+poffset*obj.rate, obj.dp_up);
			val = obj.dp_up - poffset*obj.rate;
			// val = (obj.pinit + obj.dp_up) - pout;
		}
		// else if (dp_desired < -obj.dp_dn){
		else if ((val + poffset*obj.rate) < -obj.dp_dn){
			gl_warning("sec_control: Object %s desired setpoint change of %0.6f MW, would result in a total change of %0.6f < than maximum -%0.6f MW. Changing to limit.", obj.ptr->name, val, val+poffset*obj.rate, obj.dp_dn);
			val = -obj.dp_dn - poffset*obj.rate;
			// val = (obj.pinit - obj.dp_dn) - pout;
		}
		

		// ===== Handle dispatch limits (these are in p.u.)
		if ((pdisp + poffset + val/obj.rate) > obj.pmax)
		{
			gl_warning("sec_control: generator %s is dispatched at %0.4f pu. The desired setpoint change of %0.4f pu will exceed the upper limit of %0.4f pu. Setting to upper limit.", obj.ptr->name, pdisp+poffset, val/obj.rate, obj.pmax);
			val = (obj.pmax - pdisp - poffset) * obj.rate;
		}
		else if ((pdisp + poffset + val/obj.rate) < obj.pmin)
		{
			gl_warning("sec_control: generator %s is dispatched at %0.4f pu. The desired setpoint change of %0.4f pu will exceed the lower limit of %0.4f pu. Setting to lower limit.", obj.ptr->name, pdisp+poffset, val/obj.rate, obj.pmin);
			val = (obj.pmin - pdisp - poffset) * obj.rate;
		}

		//=== Store value (potentially changed by limits)
		obj.dP[2] = val; 
		
		// ========== Perform update
		poffset += val/obj.rate; // increment offset in p.u.
		obj.poffset->setp<double>(poffset,*test_rlock);
	}
}


// Get Frequency Deviation including limiting and deadband handling.
// Functinalized since identical for any pass.
void sec_control::get_deltaf(void)
{
	if (overfrequency_limit < f0)
	{
		gl_warning("sec_control: overfrequency_limit (%0.3f Hz) is SMALLER than nominal (%0.3f Hz). Setting to default (%0.3f Hz)", overfrequency_limit, f0, f0+frequency_delta_default);
		overfrequency_limit = f0 + frequency_delta_default;
	}
	if (underfrequency_limit > f0)
	{
		gl_warning("sec_control: under (%0.3f Hz) is GREATER than nominal (%0.3f Hz). Setting to default (%0.3f Hz)", overfrequency_limit, f0, f0-frequency_delta_default);
		overfrequency_limit = f0 - frequency_delta_default;
	}
	
	fmeas = pFrequency->get_double(); //pull current frequency from parent node
	curr_state.deltaf[0] = f0 - fmeas; // frequency error [Hz] 
	if (curr_state.deltaf[0] > (f0 - underfrequency_limit)) // limit maximum positive deviation (underfrequency)
	{
		curr_state.deltaf[0] = f0 - underfrequency_limit;
	}
	else if (curr_state.deltaf[0] < (f0 - overfrequency_limit)) //limit maximum negative deviation (overfrequency)
	{
		curr_state.deltaf[0] = f0 - overfrequency_limit;
	}
	else if (abs(curr_state.deltaf[0]) < deadband) //frequency is within deadband
	{
		curr_state.deltaf[0] = 0;
	}
	deadbandflag = abs(curr_state.deltaf[0]) > deadband;
	
}

//Verify that participation factors sum to 1.
STATUS sec_control::check_alpha(void)
{	
	double asum = 0; //alpha accumulator
	for (auto & obj : part_obj){
		asum += obj.alpha;
	}
	if (abs(asum - 1) <= alpha_tol)
	{
		return SUCCESS; // alphas sum to 1 (within tolerance)
	}
	else
	{
		return FAILED;
	}
} 


//Module-level post update call
/* This is just put here as an example - not sure what it would do */
STATUS sec_control::post_deltaupdate(complex *useful_value, unsigned int mode_pass)
{
	//**************** Post-deltamode transition back to QSTS items ****************//
	
	curr_dt = -1; // clear the deltat store variable
	return SUCCESS; //Always succeeds right now
}

//Map Complex value
gld_property *sec_control::map_complex_value(OBJECT *obj, char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if ((pQuantity->is_valid() != true) || (pQuantity->is_complex() != true))
	{
		GL_THROW("sec_control:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in the secondary controller.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

complex sec_control::get_complex_value(OBJECT *obj, char *name)
{
	complex val;
	gld_property *ptr = map_complex_value(obj, name);
	val = ptr->get_complex();
	delete ptr;
	return val;
}

//Map double value
gld_property *sec_control::map_double_value(OBJECT *obj, char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if ((pQuantity->is_valid() != true) || (pQuantity->is_double() != true))
	{
		GL_THROW("sec_control:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in the secondary controller.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Get gld_property double value
double sec_control::get_double_value(OBJECT *obj, char *name)
{
	double val;
	gld_property * ptr;
	ptr = map_double_value(obj, name);
	val = ptr->get_double();
	delete ptr;
	return val;
}

//Map enumeration value
gld_property *sec_control::map_enum_value(OBJECT *obj, char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Check it
	if ((pQuantity->is_valid() != true) || (pQuantity->is_enumeration() != true))
	{
		GL_THROW("sec_control:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in the secondary controller.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

// Get an enumberation value
enumeration sec_control::get_enum_value(OBJECT *obj, char *name)
{
	enumeration val;
	gld_property *ptr = map_enum_value(obj, name);
	val = ptr->get_enumeration();
	delete ptr;
	return val;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF PARSING FUNCTIONALITY
//////////////////////////////////////////////////////////////////////////

// parse string by delimiter
// from https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
std::vector<std::string> sec_control::split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

// trim leading whitespace
// from https://www.techiedelight.com/trim-string-cpp-remove-leading-trailing-spaces/#:~:text=We%20can%20use%20a%20combination,functions%20to%20trim%20the%20string.
std::string sec_control::ltrim(const std::string &s)
{
	size_t start = s.find_first_not_of(" \n\r\t\f\v");
	return (start == std::string::npos) ? "" : s.substr(start);
}

// trim trailing whitespace
// from https://www.techiedelight.com/trim-string-cpp-remove-leading-trailing-spaces/#:~:text=We%20can%20use%20a%20combination,functions%20to%20trim%20the%20string.
std::string sec_control::rtrim(const std::string &s)
{
	size_t end = s.find_last_not_of(" \n\r\t\f\v");
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

// trim leading and trailing whitespace
// from https://www.techiedelight.com/trim-string-cpp-remove-leading-trailing-spaces/#:~:text=We%20can%20use%20a%20combination,functions%20to%20trim%20the%20string.
std::string sec_control::trim(const std::string &s) {
	return rtrim(ltrim(s));
}

//Convert string to double with default value fall-back.
double sec_control::str2double(std::string &s, double default_val)
{
    double out;
    try
    {
        out = std::stod(s);
    }
    catch(const std::invalid_argument& e)
    {
        out = default_val;
    }
    return out;
}

//Find object by name in the secondary control participant vector
// Note: the name property *must* be set 
std::vector<SEC_CNTRL_PARTICIPANT>::iterator sec_control::find_obj(std::string name)
{
    for (auto it=part_obj.begin(); it != part_obj.end(); it++)
    {
        if (it->ptr->name == name)
        {
            return it;
        }
    }
    return part_obj.end();
}

//Get the electrical output of the object
double sec_control::get_pelec(SEC_CNTRL_PARTICIPANT &obj)
{	
	double out;
	// Handle based on object type
	if (gl_object_isa(obj.ptr, "diesel_dg", "generators"))
	{
		out = (get_complex_value(obj.ptr, "pwr_electric")).Re() * 1e-6;
	}
	else if (gl_object_isa(obj.ptr, "inverter_dyn", "generators"))
	{
		out = (get_complex_value(obj.ptr, "VA_Out")).Re() * 1e-6;
	}

	return out;
}

//Add a memeber to the secondary control participant vector
void sec_control::add_obj(std::vector<std::string> &vals)
{
    SEC_CNTRL_PARTICIPANT tmp;
    // required
	std::string sname = trim(vals.at(0));
	char * cname = new char [sname.length() + 1];
	std::strcpy(cname, sname.c_str());
    tmp.ptr = gl_get_object(cname); //get pointer to object
    tmp.alpha = std::stod(vals.at(1)); //participation factor
	
	// Handle based on object type
	if (gl_object_isa(tmp.ptr, "diesel_dg", "generators"))
	{
		// ===== Diesel Generator Options =========
		tmp.pdisp = map_double_value(tmp.ptr, "pdispatch");
		tmp.poffset = map_double_value(tmp.ptr, "pdispatch_offset");
		//for now let's just say all generators can operate between 0 and 1 p.u
		tmp.rate = get_double_value(tmp.ptr, "Rated_VA") * 1e-6; //convert to MW for our purposes
		tmp.pmax = 1; // p.u
		tmp.pmin = 0; // p.u.
		tmp.pinit = get_pelec(tmp);
	} // END diesel_dg specific
	else if (gl_object_isa(tmp.ptr, "inverter_dyn", "generators"))
	{
		// ===== Inverter_dyn options ==========
		tmp.pdisp = map_double_value(tmp.ptr, "pdispatch");
		tmp.poffset = map_double_value(tmp.ptr, "pdispatch_offset");
		tmp.rate = get_double_value(tmp.ptr, "rated_power") * 1e-6; //convert to MW for our purposes
		tmp.pinit = get_pelec(tmp);
		enumeration inv_mode = get_enum_value(tmp.ptr, "control_mode");
		if (inv_mode == 0) //GRID_FORMING
		{
			tmp.pmax = get_double_value(tmp.ptr, "Pmax"); //This is in p.u.
			tmp.pmin = get_double_value(tmp.ptr, "Pmin"); //This is in p.u.
		}
		else if ((inv_mode == 1) || (inv_mode == 2)) //GRID_FOLLOWING or GFL_CURRENT_SOURCE
		{
			tmp.pmax = get_double_value(tmp.ptr, "Pref_max"); //Property is in p.u.
			tmp.pmin = get_double_value(tmp.ptr, "Pref_min"); //Property is in p.u.
		}
		else
		{
			GL_THROW("sec_control: inverter_dyn control_mode %d is not supported. For object %s.", inv_mode, tmp.ptr->name);
			/*  TROUBLESHOOT
			The secondary controller object is currently only able to interact with generator objects
			of type diese_dg and inverter_dyn. More objects might be added in the future.
			*/
		}
	}// END Inverter_dyn specific
	else
	{
		GL_THROW("sec_control: currently only diesel_dg and inverter_dyn objects supported. For object %s.", tmp.ptr->name);
		/*  TROUBLESHOOT
		The secondary controller object is currently only able to interact with generator objects
		of type diese_dg and inverter_dyn. More objects might be added in the future.
		*/
	}
	
	// unless a default rate is given, the default is the whole range, i.e. pmax-pmin
	tmp.dp_dn = (dp_dn_default < 0) ? ((tmp.pmax - tmp.pmin)*tmp.rate) : dp_dn_default;
	tmp.dp_up = (dp_up_default < 0) ? ((tmp.pmax - tmp.pmin)*tmp.rate) : dp_up_default;
	tmp.Tlp = Tlp_default; // set default (might be negative if not set but is handled later in check)

    // optional
    //now check inputs
    try
    {
		tmp.dp_dn = str2double(vals.at(2), tmp.dp_dn);
        tmp.dp_up = str2double(vals.at(3), tmp.dp_up);
		tmp.pmax = str2double(vals.at(4), tmp.pmax);
		tmp.pmin = str2double(vals.at(5), tmp.pmin);
        tmp.Tlp = str2double(vals.at(6), tmp.Tlp);
    }
    catch(std::out_of_range const& e)
    {
        //do nothing these are optional.
    }

	//TODO:
	// What happens if pmax/pmin chaged but dp_dn/dp_up are not???
    
	participant_tlp_check(tmp); //recheck  Tlp to make sure it is not too small or set default if in delta mode.

    // Add to list
    part_obj.push_back(tmp);
}

//Modify a memeber of the secondary control participant vector
void sec_control::mod_obj(std::vector<std::string> &vals)
{
    std::vector<SEC_CNTRL_PARTICIPANT>::iterator tmp = find_obj(trim(vals.at(0)));
    if (tmp != part_obj.end())
    {
        
        // all modifications are optional
        try
        {
            tmp->alpha = str2double(vals.at(1), tmp->alpha);
			tmp->dp_dn = str2double(vals.at(2), tmp->dp_dn);
            tmp->dp_up = str2double(vals.at(3), tmp->dp_up);
			tmp->pmax = str2double(vals.at(4), tmp->pmax);
			tmp->pmin = str2double(vals.at(5), tmp->pmin);
            tmp->Tlp = str2double(vals.at(6), tmp->Tlp);

			// convert iterator to pointer from:
			// https://iris.artins.org/software/converting-an-stl-vector-iterator-to-a-raw-pointer/#:~:text=Unfortunately%2C%20there%20is%20no%20way,to%20the%20vector%20in%20question.
			SEC_CNTRL_PARTICIPANT * tmpptr;
			tmpptr = &part_obj[0] + (tmp - part_obj.begin());
			participant_tlp_check(tmpptr); //recheck Tlp to make sure it is not too small
        }
        catch(std::out_of_range const& e)
        {
            //do nothing these are optional.
        }   
    }
}

//Remove a memeber of the secondary control participant vector
void sec_control::rem_obj(std::vector<std::string> &vals)
{
    std::vector<SEC_CNTRL_PARTICIPANT>::iterator it = find_obj(trim(vals.at(0)));
    if (it != part_obj.end())
    {
        part_obj.erase(it);
    }
}

//Parse a single entry of type PARSE_OPTS (ADD, REMOVE, MODIFY)
void sec_control::parse_obj(PARSE_OPTS key, std::string &valstring)
{
    // vals is a comma separated array
    std::vector<std::string> vals = split(valstring, ',');
    switch (key)
    {
    case ADD:
        add_obj(vals);
        break;
    case REMOVE:
        rem_obj(vals);
        break;
    case MODIFY:
        mod_obj(vals);
        break;
    default:
        break;
    }
}

//Parse csv input to participant_input
// 	- go line by line
// 	- identify key words,
// 	- pass key and line to parse_objs
void sec_control::parse_csv(std::string &s)
{
    std::fstream file;
    std::string line;
    std::string tline;
    PARSE_OPTS key;
    file.open(s, std::fstream::in);
    while (std::getline(file, line))
    {
        tline = trim(line);
        if (tline.compare("ADD") == 0)
        {
            key = ADD;
        }
        else if (tline.compare("MODIFY") == 0)
        {
            key = MODIFY;
        }
        else if (tline.compare("REMOVE") == 0)
        {
            key = REMOVE;
        }
        else
        {
            parse_obj(key, tline);
        }
    }
}

//Parse command string to participant_input
// - pick out key words
// - look for ; as "data chunks"
// - pass key and "data chunk" to parse_objs
void sec_control::parse_glm(std::string &s)
{
	
    std::vector<PARSE_KEYS> keys = { {ADD, 3, s.find("ADD")}, {MODIFY,6, s.find("MODIFY")}, {REMOVE, 6, s.find("REMOVE")}};

    std::sort(keys.begin(), keys.end());

    std::string substring;
    for (int i=0; i < keys.size(); i++){
        if (keys[i].pos == s.npos){
            break;
        }
        
		// Get the string between the current key and the next
		std::size_t start = keys[i].pos+keys[i].offset;
        std::size_t end   = keys[i+1].pos != s.npos ? keys[i+1].pos - start : s.npos;
        substring = trim(s.substr(start, end));

        std::istringstream tokenStream(substring);
        std::string token;
		// loop over data chuncks separated by ';'
        while (std::getline(tokenStream, token, ';'))
        {
            parse_obj(keys[i].key, token);
        }
    }
}

//Parse the participant_input property
void sec_control::parse_praticipant_input(char* participant_input)
{
	// Check if property has been updated.
	if (participant_input[0] != '\0')
	{
		std::string s = participant_input; // convert to string
		if (s.find(".csv")!= s.npos)
		{
			parse_csv(s);
        }
		else // glm
		{
			parse_glm(s);
		}

		//We should clear participant_input at the end here
		// so we can see later changes easily.
		memset(participant_input, '\0', sizeof(char1024));
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_sec_control(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(sec_control::oclass);
		if (*obj != NULL)
		{
			sec_control *my = OBJECTDATA(*obj, sec_control);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(sec_control);
}

EXPORT int init_sec_control(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != NULL)
			return OBJECTDATA(obj, sec_control)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(sec_control);
}

EXPORT TIMESTAMP sync_sec_control(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	sec_control *my = OBJECTDATA(obj, sec_control);
	try
	{
		switch (pass)
		{
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock, t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock, t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass == clockpass)
			obj->clock = t1;
	}
	SYNC_CATCHALL(sec_control);
	return t2;
}

EXPORT int isa_sec_control(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,sec_control)->isa(classname);
}

EXPORT STATUS preupdate_sec_control(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	sec_control *my = OBJECTDATA(obj, sec_control);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0, delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_sec_control(obj=%d;%s): %s", obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_sec_control(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	sec_control *my = OBJECTDATA(obj, sec_control);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_sec_control(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

EXPORT STATUS postupdate_sec_control(OBJECT *obj, complex *useful_value, unsigned int mode_pass)
{
	sec_control *my = OBJECTDATA(obj, sec_control);
	STATUS status = FAILED;
	try
	{
		status = my->post_deltaupdate(useful_value, mode_pass);
		return status;
	}
	catch (char *msg)
	{
		gl_error("postupdate_sec_control(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}
