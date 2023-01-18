#include "ibr_skeleton.h"

CLASS *ibr_skeleton::oclass = nullptr;
ibr_skeleton *ibr_skeleton::defaults = nullptr;

static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
ibr_skeleton::ibr_skeleton(MODULE *module)
{
	if (oclass == nullptr)
	{
		oclass = gl_register_class(module, "ibr_skeleton", sizeof(ibr_skeleton), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == nullptr)
			throw "unable to register class ibr_skeleton";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration, "control_mode", PADDR(control_mode), PT_DESCRIPTION, "Inverter control mode: grid-forming or grid-following",
				PT_KEYWORD, "GRID_FORMING", (enumeration)GRID_FORMING,
				PT_KEYWORD, "GRID_FOLLOWING", (enumeration)GRID_FOLLOWING,
				PT_KEYWORD, "GFL_CURRENT_SOURCE", (enumeration)GFL_CURRENT_SOURCE,

			PT_enumeration, "grid_following_mode", PADDR(grid_following_mode), PT_DESCRIPTION, "grid-following mode, positive sequency or balanced three phase power",
				PT_KEYWORD, "BALANCED_POWER", (enumeration)BALANCED_POWER,
				PT_KEYWORD, "POSITIVE_SEQUENCE", (enumeration)POSITIVE_SEQUENCE,

			PT_enumeration, "grid_forming_mode", PADDR(grid_forming_mode), PT_DESCRIPTION, "grid-forming mode, CONSTANT_DC_BUS or DYNAMIC_DC_BUS",
				PT_KEYWORD, "CONSTANT_DC_BUS", (enumeration)CONSTANT_DC_BUS,
				PT_KEYWORD, "DYNAMIC_DC_BUS", (enumeration)DYNAMIC_DC_BUS,
			PT_complex, "phaseA_I_Out[A]", PADDR(terminal_current_val[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system",
			PT_complex, "phaseB_I_Out[A]", PADDR(terminal_current_val[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system",
			PT_complex, "phaseC_I_Out[A]", PADDR(terminal_current_val[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system",
			PT_complex, "phaseA_I_Out_PU[pu]", PADDR(terminal_current_val_pu[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system, pu",
			PT_complex, "phaseB_I_Out_PU[pu]", PADDR(terminal_current_val_pu[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system, pu",
			PT_complex, "phaseC_I_Out_PU[pu]", PADDR(terminal_current_val_pu[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system, pu",

			PT_complex, "IA_Out_PU_temp[pu]", PADDR(I_out_PU_temp[0]), PT_DESCRIPTION, " Phase A current for current limiting calculation of a grid-forming inverter, pu",
			PT_complex, "IB_Out_PU_temp[pu]", PADDR(I_out_PU_temp[1]), PT_DESCRIPTION, " Phase B current for current limiting calculation of a grid-forming inverter, pu",
			PT_complex, "IC_Out_PU_temp[pu]", PADDR(I_out_PU_temp[2]), PT_DESCRIPTION, " Phase C current for current limiting calculation of a grid-forming inverter, pu",


			PT_complex, "power_A[VA]", PADDR(power_val[0]), PT_DESCRIPTION, "AC power on A phase in three-phase system",
			PT_complex, "power_B[VA]", PADDR(power_val[1]), PT_DESCRIPTION, "AC power on B phase in three-phase system",
			PT_complex, "power_C[VA]", PADDR(power_val[2]), PT_DESCRIPTION, "AC power on C phase in three-phase system",
			PT_complex, "VA_Out[VA]", PADDR(VA_Out), PT_DESCRIPTION, "AC power",

			// 3 phase average value of terminal voltage
			PT_double, "pCircuit_V_Avg_pu", PADDR(pCircuit_V_Avg_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: three-phase average value of terminal voltage, per unit value",

			//Input
			PT_double, "rated_power[VA]", PADDR(S_base), PT_DESCRIPTION, " The rated power of the inverter",
			PT_double, "rated_DC_Voltage[V]", PADDR(Vdc_base), PT_DESCRIPTION, " The rated dc bus of the inverter",

			// Grid-Following Controller Parameters
			PT_double, "Pref[W]", PADDR(Pref), PT_DESCRIPTION, "DELTAMODE: The real power reference.",
			PT_double, "Qref[VAr]", PADDR(Qref), PT_DESCRIPTION, "DELTAMODE: The reactive power reference.",
			PT_double, "F_current", PADDR(F_current), PT_DESCRIPTION, "DELTAMODE: feed forward term gain in current loop.",
			PT_double, "Tif", PADDR(Tif), PT_DESCRIPTION, "DELTAMODE: time constant of first-order low-pass filter of current loop when using current source representation.",

			PT_double, "Pref_max[pu]", PADDR(Pref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Pref_min[pu]", PADDR(Pref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Qref_max[pu]", PADDR(Qref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "Qref_min[pu]", PADDR(Qref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "frequency_convergence_criterion[rad/s]", PADDR(GridForming_freq_convergence_criterion), PT_DESCRIPTION, "Max frequency update for grid-forming inverters to return to QSTS",
			PT_double, "voltage_convergence_criterion[V]", PADDR(GridForming_volt_convergence_criterion), PT_DESCRIPTION, "Max voltage update for grid-forming inverters to return to QSTS",
			PT_double, "current_convergence_criterion[A]", PADDR(GridFollowing_curr_convergence_criterion), PT_DESCRIPTION, "Max current magnitude update for grid-following inverters to return to QSTS, or initialize",

			// Grid-Forming Controller Parameters
			PT_double, "Vdc_min_pu[pu]", PADDR(Vdc_min_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: The reference voltage of the Vdc_min controller",
			PT_double, "C_pu[pu]", PADDR(C_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: capacitance of dc bus",

			// IBR_SKELETON_NOTE: Any variables that need to be published should go here.

			//DC Bus portions
			PT_double, "V_In[V]", PADDR(V_DC), PT_DESCRIPTION, "DC input voltage",
			PT_double, "I_In[A]", PADDR(I_DC), PT_DESCRIPTION, "DC input current",
			PT_double, "P_In[W]", PADDR(P_DC), PT_DESCRIPTION, "DC input power",

			PT_double, "pvc_Pmax[W]", PADDR(pvc_Pmax), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "P max from the PV curve",
			nullptr) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		defaults = this;

		memset(this, 0, sizeof(ibr_skeleton));

		if (gl_publish_function(oclass, "preupdate_gen_object", (FUNCTIONADDR)preupdate_ibr_skeleton) == nullptr)
			GL_THROW("Unable to publish ibr_skeleton deltamode function");
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_ibr_skeleton) == nullptr)
			GL_THROW("Unable to publish ibr_skeleton deltamode function");
		// if (gl_publish_function(oclass, "postupdate_gen_object", (FUNCTIONADDR)postupdate_ibr_skeleton) == nullptr)
		// 	GL_THROW("Unable to publish ibr_skeleton deltamode function");
		if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)ibr_skeleton_NR_current_injection_update) == nullptr)
			GL_THROW("Unable to publish ibr_skeleton current injection update function");
		if (gl_publish_function(oclass, "register_gen_DC_object", (FUNCTIONADDR)ibr_skeleton_DC_object_register) == nullptr)
			GL_THROW("Unable to publish ibr_skeleton DC registration function");
	}
}

//Isa function for identification
int ibr_skeleton::isa(char *classname)
{
	return strcmp(classname,"ibr_skeleton")==0;
}

/* Object creation is called once for each object that is created by the core */
int ibr_skeleton::create(void)
{
	////////////////////////////////////////////////////////
	// DELTA MODE
	////////////////////////////////////////////////////////
	deltamode_inclusive = false; //By default, don't be included in deltamode simulations
	first_sync_delta_enabled = false;
	deltamode_exit_iteration_met = false;	//Init - will be set elsewhere

	inverter_start_time = TS_INVALID;
	inverter_first_step = true;
	first_iteration_current_injection = -1; //Initialize - mainly for tracking SWING_PQ status
	first_deltamode_init = true;	//First time it goes in will be the first time

	//Variable mapping items
	parent_is_a_meter = false;		//By default, no parent meter
	parent_is_single_phase = false; //By default, we're three-phase
	parent_is_triplex = false;		//By default, not a triplex
	attached_bus_type = 0;			//By default, we're basically a PQ bus
	swing_test_fxn = nullptr;			//By default, no mapping

	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = nullptr;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = nullptr;
	pLine_unrotI[0] = pLine_unrotI[1] = pLine_unrotI[2] = nullptr;
	pPower[0] = pPower[1] = pPower[2] = nullptr;
	pIGenerated[0] = pIGenerated[1] = pIGenerated[2] = nullptr;

	pMeterStatus = nullptr; // check if the meter is in service
	pbus_full_Y_mat = nullptr;
	pGenerated = nullptr;

	//Zero the accumulators
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0, 0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = gld::complex(0.0, 0.0);
	value_Line_unrotI[0] = value_Line_unrotI[1] = value_Line_unrotI[2] = gld::complex(0.0, 0.0);
	value_Power[0] = value_Power[1] = value_Power[2] = gld::complex(0.0, 0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = gld::complex(0.0, 0.0);
	prev_value_IGenerated[0] = prev_value_IGenerated[1] = prev_value_IGenerated[2] = gld::complex(0.0, 0.0);
	value_MeterStatus = 1; //Connected, by default

	f_nominal = 60;

	// Grid-Following controller parameters
	F_current = 0;
	Tif = 0.005; // only used for current source representation

	Pref_max = 1.0; // per unit
	Pref_min = -1.0;	// per unit

	Qref_max = 1.0; // per unit
	Qref_min = -1.0;	// per unit

	Vdc_base = 850; // default value of dc bus voltage
	Vdc_min_pu = 1; // default reference of the Vdc_min controller

	// Capacitance of dc bus
	C_pu = 0.1;	 //per unit

	GridForming_freq_convergence_criterion = 1e-5;
	GridForming_volt_convergence_criterion = 0.01;
	GridFollowing_curr_convergence_criterion = 0.01;

	//Set up the deltamode "next state" tracking variable
	desired_simulation_mode = SM_EVENT;

	//Tracking variable
	last_QSTS_GF_Update = TS_NEVER_DBL;

	//Clear the DC interface list - paranoia
	dc_interface_objects.clear();

	//DC Bus items
	P_DC = 0.0;
	V_DC = Vdc_base;
	I_DC = 0.0;

	node_nominal_voltage = 120.0;		//Just pick a value

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int ibr_skeleton::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	double temp_volt_mag;
	double temp_volt_ang[3];
	gld_property *temp_property_pointer = nullptr;
	gld_property *Frequency_mapped = nullptr;
	gld_wlock *test_rlock = nullptr;
	bool temp_bool_value;
	int temp_idx_x, temp_idx_y;
	unsigned iindex, jindex;
	gld::complex temp_complex_value;
	complex_array temp_complex_array, temp_child_complex_array;
	OBJECT *tmp_obj = nullptr;
	gld_object *tmp_gld_obj = nullptr;
	STATUS return_value_init;
	bool childed_connection = false;

	//Deferred initialization code
	if (parent != nullptr)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("ibr_skeleton::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	// Data sanity check
	if (S_base <= 0)
	{
		S_base = 1000;
		gl_warning("ibr_skeleton:%d - %s - The rated power of this inverter must be positive - set to 1 kVA.",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		The ibr_skeleton has a rated power that is negative.  It defaulted to a 1 kVA inverter.  If this is not desired, set the property properly in the GLM.
		*/
	}

	//See if the global flag is set - if so, add the object flag
	if (all_generator_delta)
	{
		obj->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true; //Set the flag and off we go
	}

	// find parent meter or triplex_meter, if not defined, use default voltages, and if
	// the parent is not a meter throw an exception
	if (parent != nullptr)
	{
		//See what kind of parent we are
		if (gl_object_isa(parent, "meter", "powerflow") || gl_object_isa(parent, "node", "powerflow") || gl_object_isa(parent, "load", "powerflow") ||
			gl_object_isa(parent, "triplex_meter", "powerflow") || gl_object_isa(parent, "triplex_node", "powerflow") || gl_object_isa(parent, "triplex_load", "powerflow"))
		{
			//See if we're in deltamode and VSI - if not, we don't care about the "parent-ception" mapping
			//Normal deltamode just goes through current interfaces, so don't need this craziness
			if (deltamode_inclusive)
			{
				//See if this attached node is a child or not
				if (parent->parent != nullptr)
				{
					//Map parent - wherever it may be
					temp_property_pointer = new gld_property(parent,"NR_powerflow_parent");

					//Make sure it worked
					if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_objectref())
					{
						GL_THROW("ibr_skeleton:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
						//Defined elsewhere
					}

					//Pull the mapping - gld_object
					tmp_gld_obj = temp_property_pointer->get_objectref();

					//Pull the proper object reference
					tmp_obj = tmp_gld_obj->my();

					//free the property
					delete temp_property_pointer;

					//See what it is
					if (!gl_object_isa(tmp_obj, "meter", "powerflow") && !gl_object_isa(tmp_obj, "node", "powerflow") && !gl_object_isa(tmp_obj, "load", "powerflow") &&
						!gl_object_isa(tmp_obj, "triplex_meter", "powerflow") && !gl_object_isa(tmp_obj, "triplex_node", "powerflow") && !gl_object_isa(tmp_obj, "triplex_load", "powerflow"))
					{
						//Not a wierd map, just use normal parent
						tmp_obj = parent;
					}
					else //Implies it is a powerflow parent
					{
						//Set the flag
						childed_connection = true;

						//See if we are deltamode-enabled -- if so, flag our parent while we're here
						//Map our deltamode flag and set it (parent will be done below)
						temp_property_pointer = new gld_property(parent, "Norton_dynamic");

						//Make sure it worked
						if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
						{
							GL_THROW("ibr_skeleton:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", parent->name ? parent->name : "unnamed");
							/*  TROUBLESHOOT
							While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
							Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
							*/
						}

						//Flag it to true
						temp_bool_value = true;
						temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);

						//Remove it
						delete temp_property_pointer;

						//Set the child accumulator flag too
						temp_property_pointer = new gld_property(tmp_obj,"Norton_dynamic_child");

						//Make sure it worked
						if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
						{
							GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
							//Defined elsewhere
						}

						//Flag it to true
						temp_bool_value = true;
						temp_property_pointer->setp<bool>(temp_bool_value,*test_rlock);

						//Remove it
						delete temp_property_pointer;
					} //End we were a powerflow child
				}	  //Parent has a parent
				else  //It is null
				{
					//Just point it to the normal parent
					tmp_obj = parent;
				}
			}	 //End deltamode inclusive
			else //Not deltamode and VSI -- all other combinations just use standard parents
			{
				//Just point it to the normal parent
				tmp_obj = parent;
			}

			//Map and pull the phases - true parent
			temp_property_pointer = new gld_property(parent, "phases");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_set())
			{
				GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
				/*  TROUBLESHOOT
				While attempting to map the phases property from the parent object, an error was encountered.
				Please check and make sure your parent object is a meter or triplex_meter inside the powerflow module and try
				again.  If the error persists, please submit your code and a bug report via the Trac website.
				*/
			}

			//Pull the phase information
			phases = temp_property_pointer->get_set();

			//Clear the temporary pointer
			delete temp_property_pointer;

			//Simple initial test - if we aren't three-phase, but are grid-forming, toss a warning
			if (((phases & 0x07) != 0x07) && (control_mode == GRID_FORMING))
			{
				gl_warning("ibr_skeleton:%s is in grid-forming mode, but is not a three-phase connection.  This is untested and may not behave properly.", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The ibr_skeleton was set up to be grid-forming, but is either a triplex or a single-phase-connected inverter.  This implementaiton is not fully tested and may either not
				work, or produce unexpecte results.
				*/
			}

			//Pull the bus type
			temp_property_pointer = new gld_property(tmp_obj, "bustype");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("ibr_skeleton:%s failed to map bustype variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
				/*  TROUBLESHOOT
				While attempting to map the bustype variable from the parent node, an error was encountered.  Please try again.  If the error
				persists, please report it with your GLM via the issues tracking system.
				*/
			}

			//Pull the value of the bus
			attached_bus_type = temp_property_pointer->get_enumeration();

			//Remove it
			delete temp_property_pointer;

			//Determine parent type
			//Triplex first, otherwise it tries to map to three-phase (since all triplex are nodes)
			if (gl_object_isa(tmp_obj, "triplex_meter", "powerflow") || gl_object_isa(tmp_obj, "triplex_node", "powerflow") || gl_object_isa(tmp_obj, "triplex_load", "powerflow"))
			{
				//Indicate this is a meter, but is also a single-phase variety
				parent_is_a_meter = true;
				parent_is_single_phase = true;
				parent_is_triplex = true;

				//Map the various powerflow variables
				//Map the other two here for initialization problem
				pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_12");
				pCircuit_V[1] = map_complex_value(tmp_obj, "voltage_1");
				pCircuit_V[2] = map_complex_value(tmp_obj, "voltage_2");

				//Get 12 and Null the rest
				pLine_I[0] = map_complex_value(tmp_obj, "current_12");
				pLine_I[1] = nullptr;
				pLine_I[2] = nullptr;

				//Pull power12, then null the rest
				pPower[0] = map_complex_value(tmp_obj, "power_12");
				pPower[1] = nullptr; //Not used
				pPower[2] = nullptr; //Not used

				pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_12");
				pLine_unrotI[1] = nullptr; //Not used
				pLine_unrotI[2] = nullptr; //Not used

				//Map IGenerated, even though triplex can't really use this yet (just for the sake of doing so)
				pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_12");
				pIGenerated[1] = nullptr;
				pIGenerated[2] = nullptr;
			} //End triplex parent
			else if (gl_object_isa(tmp_obj, "meter", "powerflow") || gl_object_isa(tmp_obj, "node", "powerflow") || gl_object_isa(tmp_obj, "load", "powerflow"))
			{
				//See if we're three-phased
				if ((phases & 0x07) == 0x07) //We are
				{
					parent_is_a_meter = true;
					parent_is_single_phase = false;
					parent_is_triplex = false;

					//Map the various powerflow variables
					pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_A");
					pCircuit_V[1] = map_complex_value(tmp_obj, "voltage_B");
					pCircuit_V[2] = map_complex_value(tmp_obj, "voltage_C");

					pLine_I[0] = map_complex_value(tmp_obj, "current_A");
					pLine_I[1] = map_complex_value(tmp_obj, "current_B");
					pLine_I[2] = map_complex_value(tmp_obj, "current_C");

					pPower[0] = map_complex_value(tmp_obj, "power_A");
					pPower[1] = map_complex_value(tmp_obj, "power_B");
					pPower[2] = map_complex_value(tmp_obj, "power_C");

					pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_A");
					pLine_unrotI[1] = map_complex_value(tmp_obj, "prerotated_current_B");
					pLine_unrotI[2] = map_complex_value(tmp_obj, "prerotated_current_C");

					//Map the current injection variables
					pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_A");
					pIGenerated[1] = map_complex_value(tmp_obj, "deltamode_generator_current_B");
					pIGenerated[2] = map_complex_value(tmp_obj, "deltamode_generator_current_C");
				}
				else //Not three-phase
				{
					//Just assume this is true - the only case it isn't is a GL_THROW, so it won't matter
					parent_is_a_meter = true;
					parent_is_single_phase = true;
					parent_is_triplex = false;

					//NULL all the secondary indices - we won't use any of them
					pCircuit_V[1] = nullptr;
					pCircuit_V[2] = nullptr;

					pLine_I[1] = nullptr;
					pLine_I[2] = nullptr;

					pPower[1] = nullptr;
					pPower[2] = nullptr;

					pLine_unrotI[1] = nullptr;
					pLine_unrotI[2] = nullptr;

					pIGenerated[1] = nullptr;
					pIGenerated[2] = nullptr;

					if ((phases & 0x07) == 0x01) //A
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_A");
						pLine_I[0] = map_complex_value(tmp_obj, "current_A");
						pPower[0] = map_complex_value(tmp_obj, "power_A");
						pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_A");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_A");
					}
					else if ((phases & 0x07) == 0x02) //B
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_B");
						pLine_I[0] = map_complex_value(tmp_obj, "current_B");
						pPower[0] = map_complex_value(tmp_obj, "power_B");
						pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_B");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_B");
					}
					else if ((phases & 0x07) == 0x04) //C
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_C");
						pLine_I[0] = map_complex_value(tmp_obj, "current_C");
						pPower[0] = map_complex_value(tmp_obj, "power_C");
						pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_C");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_C");
					}
					else //Not three-phase, but has more than one phase - fail, because we don't do this right
					{
						GL_THROW("ibr_skeleton:%s is not connected to a single-phase or three-phase node - two-phase connections are not supported at this time.", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						The ibr_skeleton only supports single phase (A, B, or C or triplex) or full three-phase connections.  If you try to connect it differntly than this, it will not work.
						*/
					}
				} //End non-three-phase
			}	  //End non-triplex parent

			//*** Common items ****//
			// Many of these go to the "true parent", not the "powerflow parent"

			//Map the nominal frequency
			temp_property_pointer = new gld_property("powerflow::nominal_frequency");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("ibr_skeleton:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the nominal_frequency property, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			//Must be valid, read it
			f_nominal = temp_property_pointer->get_double();

			//Remove it
			delete temp_property_pointer;
			
			//See if we are deltamode-enabled -- powerflow parent version
			if (deltamode_inclusive)
			{
				if (control_mode != GFL_CURRENT_SOURCE)
				{
					//Map our deltamode flag and set it (parent will be done below)
					temp_property_pointer = new gld_property(tmp_obj, "Norton_dynamic");

					//Make sure it worked
					if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
					{
						GL_THROW("ibr_skeleton:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
						//Defined elsewhere
					}

					//Flag it to true
					temp_bool_value = true;
					temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);

					//Remove it
					delete temp_property_pointer;
				}
				else
				{
					//Map our deltamode flag and set it (parent will be done below)
					temp_property_pointer = new gld_property(tmp_obj, "generator_dynamic");

					//Make sure it worked
					if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
					{
						GL_THROW("ibr_skeleton:%s failed to map generator deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
						/*  TROUBLESHOOT
						While trying to map a flag indicating a dynamic generator is attached to the system, an error was encountered.  Please
						try your file again.  If the error persists, please submit an issue ticket.
						*/
					}

					//Flag it to true
					temp_bool_value = true;
					temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);

					//Remove it
					delete temp_property_pointer;
				}

				// Obtain the Z_base of the system for calculating filter impedance
				//Link to nominal voltage
				temp_property_pointer = new gld_property(parent, "nominal_voltage");

				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
				{
					gl_error("ibr_skeleton:%d %s failed to map the nominal_voltage property", obj->id, (obj->name ? obj->name : "Unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the nominal_voltage property, an error occurred.  Please try again.
					If the error persists, please submit your GLM and a bug report to the ticketing system.
					*/

					return FAILED;
				}
				//Default else, it worked

				//Copy that value out - adjust if triplex
				if (parent_is_triplex)
				{
					node_nominal_voltage = 2.0 * temp_property_pointer->get_double(); //Adjust to 240V
				}
				else
				{
					node_nominal_voltage = temp_property_pointer->get_double();
				}

				//Remove the property pointer
				delete temp_property_pointer;

				V_base = node_nominal_voltage;

				if ((phases & 0x07) == 0x07)
				{
					I_base = S_base / V_base / 3.0;
					Z_base = (node_nominal_voltage * node_nominal_voltage) / (S_base / 3.0); // voltage is phase to ground voltage, S_base is three phase capacity


				}
				else //Must be triplex or single-phased (above check should remove others)
				{
					I_base = S_base / V_base;
					Z_base = (node_nominal_voltage * node_nominal_voltage) / S_base; // voltage is phase to ground voltage, S_base is single phase capacity
				}

				if (control_mode != GFL_CURRENT_SOURCE)
				{
					//Map the full_Y parameter to inject the admittance portion into it
					pbus_full_Y_mat = new gld_property(tmp_obj, "deltamode_full_Y_matrix");

					//Check it
					if (!pbus_full_Y_mat->is_valid() || !pbus_full_Y_mat->is_complex_array())
					{
						GL_THROW("ibr_skeleton:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
						/*  TROUBLESHOOT
						While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
						Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
						*/
					}

					//Pull down the variable
					pbus_full_Y_mat->getp<complex_array>(temp_complex_array, *test_rlock);

					//See if it is valid
					if (!temp_complex_array.is_valid(0, 0))
					{
						//Create it
						temp_complex_array.grow_to(3, 3);

						//Zero it, by default
						for (temp_idx_x = 0; temp_idx_x < 3; temp_idx_x++)
						{
							for (temp_idx_y = 0; temp_idx_y < 3; temp_idx_y++)
							{
								temp_complex_array.set_at(temp_idx_x, temp_idx_y, gld::complex(0.0, 0.0));
							}
						}
					}
					else //Already populated, make sure it is the right size!
					{
						if ((temp_complex_array.get_rows() != 3) && (temp_complex_array.get_cols() != 3))
						{
							GL_THROW("ibr_skeleton:%s exposed Norton-equivalent matrix is the wrong size!", obj->name ? obj->name : "unnamed");
						}
						//Default else -- right size
					}

					//See if we were connected to a powerflow child
					if (childed_connection)
					{
						temp_property_pointer = new gld_property(parent,"deltamode_full_Y_matrix");

						//Check it
						if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_complex_array())
						{
							GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
							//Defined above
						}

						//Pull down the variable
						temp_property_pointer->getp<complex_array>(temp_child_complex_array,*test_rlock);

						//See if it is valid
						if (!temp_child_complex_array.is_valid(0,0))
						{
							//Create it
							temp_child_complex_array.grow_to(3,3);

							//Zero it, by default
							for (temp_idx_x=0; temp_idx_x<3; temp_idx_x++)
							{
								for (temp_idx_y=0; temp_idx_y<3; temp_idx_y++)
								{
									temp_child_complex_array.set_at(temp_idx_x,temp_idx_y,gld::complex(0.0,0.0));
								}
							}
						}
						else	//Already populated, make sure it is the right size!
						{
							if ((temp_child_complex_array.get_rows() != 3) && (temp_child_complex_array.get_cols() != 3))
							{
								GL_THROW("diesel_dg:%s exposed Norton-equivalent matrix is the wrong size!",obj->name?obj->name:"unnamed");
								//Defined above
							}
							//Default else -- right size
						}
					}//End childed powerflow parent

					//Loop through and store the values
					for (temp_idx_x = 0; temp_idx_x < 3; temp_idx_x++)
					{
						for (temp_idx_y = 0; temp_idx_y < 3; temp_idx_y++)
						{
							//Read the existing value
							temp_complex_value = temp_complex_array.get_at(temp_idx_x, temp_idx_y);

							//Accumulate admittance into it
							temp_complex_value += generator_admittance[temp_idx_x][temp_idx_y];

							//Store it
							temp_complex_array.set_at(temp_idx_x, temp_idx_y, temp_complex_value);

							//Do the childed object, if exists
							if (childed_connection)
							{
								//Read the existing value
								temp_complex_value = temp_child_complex_array.get_at(temp_idx_x,temp_idx_y);

								//Accumulate into it
								temp_complex_value += generator_admittance[temp_idx_x][temp_idx_y];

								//Store it
								temp_child_complex_array.set_at(temp_idx_x,temp_idx_y,temp_complex_value);
							}
						}
					}

					//Push it back up
					pbus_full_Y_mat->setp<complex_array>(temp_complex_array, *test_rlock);

					//See if the childed powerflow exists
					if (childed_connection)
					{
						temp_property_pointer->setp<complex_array>(temp_child_complex_array,*test_rlock);

						//Clear it
						delete temp_property_pointer;
					}
				}

				//Map the power variable
				pGenerated = map_complex_value(tmp_obj, "deltamode_PGenTotal");
			} //End VSI common items

			//Map status - true parent
			pMeterStatus = new gld_property(parent, "service_status");

			//Check it
			if (!pMeterStatus->is_valid() || !pMeterStatus->is_enumeration())
			{
				GL_THROW("ibr_skeleton failed to map powerflow status variable");
				/*  TROUBLESHOOT
				While attempting to map the service_status variable of the parent
				powerflow object, an error occurred.  Please try again.  If the error
				persists, please submit your code and a bug report via the trac website.
				*/
			}

			//Pull initial voltages, but see which ones we should grab
			if (parent_is_single_phase)
			{
				//Powerflow values -- pull the initial value (should be nominals)
				value_Circuit_V[0] = pCircuit_V[0]->get_complex(); //A, B, C, or 12
				value_Circuit_V[1] = gld::complex(0.0, 0.0);			   //Reset, for giggles
				value_Circuit_V[2] = gld::complex(0.0, 0.0);			   //Same
			}
			else //Must be a three-phase, pull them all
			{
				//Powerflow values -- pull the initial value (should be nominals)
				value_Circuit_V[0] = pCircuit_V[0]->get_complex(); //A
				value_Circuit_V[1] = pCircuit_V[1]->get_complex(); //B
				value_Circuit_V[2] = pCircuit_V[2]->get_complex(); //C
			}
		}	 //End valid powerflow parent
		else //Not sure what it is
		{
			GL_THROW("ibr_skeleton must have a valid powerflow object as its parent, or no parent at all");
			/*  TROUBLESHOOT
			Check the parent object of the inverter.  The ibr_skeleton is only able to be childed via to powerflow objects.
			Alternatively, you can also choose to have no parent, in which case the ibr_skeleton will be a stand-alone application
			using default voltage values for solving purposes.
			*/
		}
	}	 //End parent call
	else //Must not have a parent
	{
		//Indicate we don't have a meter parent, nor is it single phase (though the latter shouldn't matter)
		parent_is_a_meter = false;
		parent_is_single_phase = false;
		parent_is_triplex = false;

		gl_warning("ibr_skeleton:%d has no parent meter object defined; using static voltages", obj->id);
		/*  TROUBLESHOOT
		An ibr_skeleton in the system does not have a parent attached.  It is using static values for the voltage.
		*/

		// Declare all 3 phases
		phases = 0x07;

		//Powerflow values -- set defaults here -- sets up like three-phase connection - use rated kV, just because
		//Set that magnitude
		value_Circuit_V[0].SetPolar(default_line_voltage, 0);
		value_Circuit_V[1].SetPolar(default_line_voltage, -2.0 / 3.0 * PI);
		value_Circuit_V[2].SetPolar(default_line_voltage, 2.0 / 3.0 * PI);

		//Define the default
		value_MeterStatus = 1;

		//Double-set the nominal frequency to NA - no powerflow available
		f_nominal = 60.0;
	}

	///////////////////////////////////////////////////////////////////////////
	// DELTA MODE
	///////////////////////////////////////////////////////////////////////////
	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (!enable_subsecond_models)
		{
			gl_warning("ibr_skeleton:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The ibr_skeleton object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
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
		if (enable_subsecond_models)
		{
			gl_warning("ibr_skeleton:%d %s - Deltamode is enabled for the module, but not this inverter!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The ibr_skeleton is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this ibr_skeleton may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}		
	}

	//Other initialization variables
	inverter_start_time = gl_globalclock;

	Idc_base = S_base / Vdc_base;

	// Initialize parameters
	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_skeleton:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The values for Pref and Qref are specified such that they will exceed teh rated_power of the inverter.  They have been truncated, with real power taking priority.
		*/
	}

	if(Pref > S_base)
	{
		Pref = S_base;
	}
	else if (Pref < -S_base)
	{
		Pref = -S_base;
	}
	else
	{
		if(Qref > sqrt(S_base*S_base-Pref*Pref))
		{
			Qref = sqrt(S_base*S_base-Pref*Pref);
		}
		else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
		{
			Qref = -sqrt(S_base*S_base-Pref*Pref);
		}
	}

	VA_Out = gld::complex(Pref, Qref);

	pvc_Pmax = 0;

	//See if we had a single phase connection
	if (parent_is_single_phase)
	{
		power_val[0] = VA_Out; //This probably needs to be adjusted for phasing?
	}
	else //Just assume three-phased
	{
		power_val[0] = VA_Out / 3.0;
		power_val[1] = VA_Out / 3.0;
		power_val[2] = VA_Out / 3.0;
	}

	//Init tracking variables
	prev_timestamp_dbl = (double)gl_globalclock;

	return 1;
}

TIMESTAMP ibr_skeleton::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		//Reset
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	return t2;
}

TIMESTAMP ibr_skeleton::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret_value;

	FUNCTIONADDR test_fxn;
	STATUS fxn_return_status;

	gld::complex temp_power_val[3];
	gld::complex temp_intermed_curr_val[3];
	char loop_var;

	gld::complex temp_complex_value;
	gld_wlock *test_rlock = nullptr;
	double curr_ts_dbl, diff_dbl;
	TIMESTAMP new_ret_value;

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		//Reset
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//Deltamode check/init items
	if (first_sync_delta_enabled) //Deltamode first pass
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models) //We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects == nullptr)) && enable_subsecond_models)
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
			if (delta_functions[gen_object_current] == nullptr)
			{
				GL_THROW("Failure to map deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			/* post_delta_functions removed, since it didn't seem to be doing anything - empty it out/delete it if this is the case! */
			// //Map up the function for postupdate
			// post_delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "postupdate_gen_object"));

			// //Make sure it worked
			// if (post_delta_functions[gen_object_current] == nullptr)
			// {
			// 	GL_THROW("Failure to map post-deltamode function for device:%s", obj->name);
			// 	/*  TROUBLESHOOT
			// 	Attempts to map up the postupdate function of a specific device failed.  Please try again and ensure
			// 	the object supports deltamode.  If the error persists, please submit your code and a bug report via the
			// 	trac website.
			// 	*/
			// }

			//Map up the function for postupdate
			delta_preupdate_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "preupdate_gen_object"));

			//Make sure it worked
			if (delta_preupdate_functions[gen_object_current] == nullptr)
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

			if (parent_is_a_meter)
			{

				//Accumulate the starting power
				if (sqrt(Pref*Pref+Qref*Qref) > S_base)
				{
					gl_warning("ibr_skeleton:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

				if(Pref > S_base)
				{
					Pref = S_base;
				}
				else if (Pref < -S_base)
				{
					Pref = -S_base;
				}
				else
				{
					if(Qref > sqrt(S_base*S_base-Pref*Pref))
					{
						Qref = sqrt(S_base*S_base-Pref*Pref);
					}
					else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
					{
						Qref = -sqrt(S_base*S_base-Pref*Pref);
					}
				}

				temp_complex_value = gld::complex(Pref, Qref);


				//Push it up
				pGenerated->setp<gld::complex>(temp_complex_value, *test_rlock);

				//Map the current injection function
				test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_current_injection_update_map"));

				//See if it was located
				if (test_fxn == nullptr)
				{
					GL_THROW("ibr_skeleton:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the additional current injection function, an error was encountered.
					Please try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}

				//Call the mapping function
				fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *))(*test_fxn))(obj->parent, obj);

				//Make sure it worked
				if (fxn_return_status != SUCCESS)
				{
					GL_THROW("ibr_skeleton:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
					//Defined above
				}
			}

			//Flag it
			first_sync_delta_enabled = false;

		}	 //End deltamode specials - first pass
		else //Somehow, we got here and deltamode isn't properly enabled...odd, just deflag us
		{
			first_sync_delta_enabled = false;
		}
	} //End first delta timestep
	//default else - either not deltamode, or not the first timestep

	//Deflag first timestep tracker
	if (inverter_start_time != t1)
	{
		inverter_first_step = false;
	}

	//Calculate power based on measured terminal voltage and currents
	if (parent_is_single_phase) // single phase/split-phase implementation
	{
		if (!inverter_first_step)
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

			//Update per-unti value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

			//Update power output variables, just so we can see what is going on

			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

			VA_Out = power_val[0];

			if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE))
			{
				if (attached_bus_type != 2)
				{

					//Compute desired output - sign convention appears to be backwards
					if (sqrt(Pref*Pref+Qref*Qref) > S_base)
					{
						gl_warning("ibr_skeleton:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
						//Defined above
					}

					if(Pref > S_base)
					{
						Pref = S_base;
					}
					else if (Pref < -S_base)
					{
						Pref = -S_base;
					}
					else
					{
						if(Qref > sqrt(S_base*S_base-Pref*Pref))
						{
							Qref = sqrt(S_base*S_base-Pref*Pref);
						}
						else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
						{
							Qref = -sqrt(S_base*S_base-Pref*Pref);
						}
					}

					gld::complex temp_VA = gld::complex(Pref, Qref);

					//Force the output power the same as glm pre-defined values
					if (value_Circuit_V[0].Mag() > 0.0)
					{
						value_IGenerated[0] = ~(temp_VA / value_Circuit_V[0]) + filter_admittance * value_Circuit_V[0];
					}
					else
					{
						value_IGenerated[0] = gld::complex(0.0,0.0);
					}
				}
			}
		}
	}
	else //Three phase variant
	{

		if (!inverter_first_step)
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
			terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			if (control_mode == GRID_FORMING)
			{

			}
			else // grid following or GFL_CURRENT_SOURCE
			{
				if (grid_following_mode == BALANCED_POWER) // Assume the grid-following inverter inject balanced power to the grid
				{
					if (attached_bus_type != 2)
					{

						if (sqrt(Pref*Pref+Qref*Qref) > S_base)
						{
							gl_warning("ibr_skeleton:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Pref > S_base)
						{
							Pref = S_base;
						}
						else if (Pref < -S_base)
						{
							Pref = -S_base;
						}
						else
						{
							if(Qref > sqrt(S_base*S_base-Pref*Pref))
							{
								Qref = sqrt(S_base*S_base-Pref*Pref);
							}
							else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
							{
								Qref = -sqrt(S_base*S_base-Pref*Pref);
							}
						}

						gld::complex temp_VA = gld::complex(Pref, Qref);

						//Copy in value
						//temp_power_val[0] = power_val[0] + (temp_VA - VA_Out) / 3.0;
						//temp_power_val[1] = power_val[1] + (temp_VA - VA_Out) / 3.0;
						//temp_power_val[2] = power_val[2] + (temp_VA - VA_Out) / 3.0;

						//Copy in value
						temp_power_val[0] = temp_VA / 3.0;
						temp_power_val[1] = temp_VA / 3.0;
						temp_power_val[2] = temp_VA / 3.0;

						//Back out the current injection - do voltage checks
						for (loop_var=0; loop_var<3; loop_var++)
						{
							if (value_Circuit_V[loop_var].Mag() > 0.0)
							{
								temp_intermed_curr_val[loop_var] = ~(temp_power_val[loop_var] / value_Circuit_V[loop_var]);
							}
							else
							{
								temp_intermed_curr_val[loop_var] = gld::complex(0.0,0.0);
							}
						}
						
						
						terminal_current_val[0] =  temp_intermed_curr_val[0];
						terminal_current_val[1] =  temp_intermed_curr_val[1];					
						terminal_current_val[2] =  temp_intermed_curr_val[2];
						
						
						//Update per-unit value
						terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
						terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
						terminal_current_val_pu[2] = terminal_current_val[2] / I_base;					
						
						
						value_IGenerated[0] = temp_intermed_curr_val[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
						value_IGenerated[1] = temp_intermed_curr_val[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
						value_IGenerated[2] = temp_intermed_curr_val[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];
					}
				}	 // end of balanced power
				else // Positive sequence. Assume the grid-following inverter inject balanced currents to the grid
				{
					if (attached_bus_type != 2)
					{

						if (sqrt(Pref*Pref+Qref*Qref) > S_base)
						{
							gl_warning("ibr_skeleton:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Pref > S_base)
						{
							Pref = S_base;
						}
						else if (Pref < -S_base)
						{
							Pref = -S_base;
						}
						else
						{
							if(Qref > sqrt(S_base*S_base-Pref*Pref))
							{
								Qref = sqrt(S_base*S_base-Pref*Pref);
							}
							else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
							{
								Qref = -sqrt(S_base*S_base-Pref*Pref);
							}
						}

						gld::complex temp_VA = gld::complex(Pref, Qref);

						// Obtain the positive sequence voltage
						value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
						
						// Obtain the positive sequence current
						if (value_Circuit_V_PS.Mag() > 0.0)
						{
							value_Circuit_I_PS[0] = ~(temp_VA / value_Circuit_V_PS) / 3.0;
						}
						else
						{
							value_Circuit_I_PS[0] = gld::complex(0.0,0.0);
						}
						
						value_Circuit_I_PS[1] = value_Circuit_I_PS[0] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI));
						value_Circuit_I_PS[2] = value_Circuit_I_PS[0] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI));

                        terminal_current_val[0] = value_Circuit_I_PS[0];
                        terminal_current_val[1] = value_Circuit_I_PS[1];						
                        terminal_current_val[2] = value_Circuit_I_PS[2];	
						
						//Update per-unit value
						terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
						terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
						terminal_current_val_pu[2] = terminal_current_val[2] / I_base;
						

						//Back out the current injection
						value_IGenerated[0] = value_Circuit_I_PS[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
						value_IGenerated[1] = value_Circuit_I_PS[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
						value_IGenerated[2] = value_Circuit_I_PS[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];


					}
				}
			}
		}
	}

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}

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

/* Update the injected currents with respect to VA_Out */
void ibr_skeleton::update_iGen(gld::complex VA_Out)
{
	char loop_var;

	if (parent_is_single_phase) // single phase/split-phase implementation
	{
		// power_val[0], terminal_current_val[0], & value_IGenerated[0]
		if (value_Circuit_V[0].Mag() > 0.0)
		{
			power_val[0] = VA_Out;
			terminal_current_val[0] = ~(power_val[0] / value_Circuit_V[0]);
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			value_IGenerated[0] = terminal_current_val[0] + filter_admittance * value_Circuit_V[0];
		}
		else
		{
			power_val[0] = gld::complex(0.0,0.0);
			terminal_current_val[0] = gld::complex(0.0,0.0);
			terminal_current_val_pu[0] = gld::complex(0.0,0.0);
			value_IGenerated[0] = gld::complex(0.0,0.0);
		}
	}
	else
	{
		//Loop through for voltage check
		for (loop_var=0; loop_var<3; loop_var++)
		{
			if (value_Circuit_V[loop_var].Mag() > 0.0)
			{
				// power_val, terminal_current_val, & value_IGenerated
				power_val[loop_var] = VA_Out / 3;

				terminal_current_val[loop_var] = ~(power_val[loop_var] / value_Circuit_V[loop_var]);
				terminal_current_val_pu[loop_var] = terminal_current_val[loop_var] / I_base;
			}
			else
			{
				power_val[loop_var] = gld::complex(0.0,0.0);
				terminal_current_val[loop_var] = gld::complex(0.0,0.0);
				terminal_current_val_pu[loop_var] = gld::complex(0.0,0.0);
			}
		}

		value_IGenerated[0] = terminal_current_val[0] - (-generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
		value_IGenerated[1] = terminal_current_val[1] - (-generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
		value_IGenerated[2] = terminal_current_val[2] - (-generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);
	}
}

/* Check the inverter output and make sure it is in the limit */
void ibr_skeleton::check_and_update_VA_Out(OBJECT *obj)
{
	if (grid_forming_mode == DYNAMIC_DC_BUS)
	{
		// Update the P_DC
		P_DC = VA_Out.Re(); //Lossless

		// Update V_DC
		if (!dc_interface_objects.empty())
		{
			int temp_idx;
			STATUS fxn_return_status;

			//Loop through and call the DC objects
			for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
			{
				//DC object, calling object (us), init mode (true/false)
				//False at end now, because not initialization
				fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

				//Make sure it worked
				if (fxn_return_status == FAILED)
				{
					//Pull the object from the array - this is just for readability (otherwise the
					OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

					//Error it up
					GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
					//Defined above
				}
			}
		}

		// Update I_DC
		I_DC = P_DC/V_DC;
	}
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP ibr_skeleton::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER; //By default, we're done forever!

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		//Also pull the current values
		pull_complex_powerflow_values();
	}

		//Check to see if vaalid connection
	if (value_MeterStatus == 1)
	{
		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			VA_Out = power_val[0];
		}
		else //Three-phase, by default
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Update per-unit values
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
			terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];
		}
	}
	else	//Disconnected somehow
	{
		VA_Out = gld::complex(0.0,0.0);
	}

	// Limit check for P_Out & Q_Out
	check_and_update_VA_Out(obj);

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Preupdate
STATUS ibr_skeleton::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	FUNCTIONADDR funadd = nullptr;
	OBJECT *hdr = OBJECTHDR(this);

	// Call to initialization function
	stat_val = init_dynamics();

	if (stat_val != SUCCESS)
	{
		gl_error("ibr_skeleton failed pre_deltaupdate call");
		/*  TROUBLESHOOT
		While attempting to call the pre_deltaupdate portion of the ibr_skeleton code, an error
		was encountered.  Please submit your code and a bug report via the ticketing system.
		*/

		return FAILED;
	}

	if (control_mode == GRID_FORMING)
	{
		//If we're a voltage-source inverter, also swap our SWING bus, just because
		//map the function
		funadd = (FUNCTIONADDR)(gl_get_function(hdr->parent, "pwr_object_swing_swapper"));

		//make sure it worked
		if (funadd == nullptr)
		{
			gl_error("ibr_skeleton:%s -- Failed to find node swing swapper function", (hdr->name ? hdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the function to change the swing status of the parent bus, the function could not be found.
			Ensure the ibr_skeleton is actually attached to something.  If the error persists, please submit your code and a bug report
			via the ticketing/issues system.
			*/

			return FAILED;
		}

		//Call the swap
		stat_val = ((STATUS(*)(OBJECT *, bool))(*funadd))(hdr->parent, false);

		if (stat_val == 0) //Failed :(
		{
			gl_error("Failed to swap SWING status of node:%s on ibr_skeleton:%s", (hdr->parent->name ? hdr->parent->name : "Unnamed"), (hdr->name ? hdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
			failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
			*/

			return FAILED;
		}
	}
	else	//Implies grid-following
	{
		//Copy the trackers - just do a bulk copy (don't care if three-phase or not)
		memcpy(prev_value_IGenerated,value_IGenerated,3*sizeof(gld::complex));
	}

	//Reset the QSTS criterion flag
	deltamode_exit_iteration_met = false;

	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

//Module-level call
SIMULATIONMODE ibr_skeleton::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltath;
	double mag_diff_val;
	bool proceed_to_qsts = true;	//Starts true - prevent QSTS by exception
	int i;
	int temp_dc_idx;
	STATUS fxn_DC_status;
	OBJECT *temp_DC_obj;

	OBJECT *obj = OBJECTHDR(this);

	SIMULATIONMODE simmode_return_value = SM_EVENT;
	
	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	//Get timestep value
	deltat = (double)dt / (double)DT_SECOND;
	deltath = deltat / 2.0;

	//Update time tracking variables
	prev_timestamp_dbl = gl_globaldeltaclock;

	if (control_mode == GRID_FORMING)
	{
		if ((iteration_count_val == 0) && (delta_time == 0) && (grid_forming_mode == DYNAMIC_DC_BUS))
		{
			P_DC = I_DC = 0; // Clean the buffer, only on the very first delta timestep
		}

		//See if we just came from QSTS and not the first timestep
		if ((prev_timestamp_dbl == last_QSTS_GF_Update) && (delta_time == 0))
		{
			//Technically, QSTS already updated us for this timestamp, so skip
			desired_simulation_mode = SM_DELTA;	//Go forward

			//In order to keep the rest of the inverter up-to-date, call the DC bus routines
			if (grid_forming_mode == DYNAMIC_DC_BUS)
			{
				if (!dc_interface_objects.empty())
				{
					//Loop through and call the DC objects
					for (temp_dc_idx = 0; temp_dc_idx < dc_interface_objects.size(); temp_dc_idx++)
					{
						//DC object, calling object (us), init mode (true/false)
						//False at end now, because not initialization
						fxn_DC_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_dc_idx].fxn_address))(dc_interface_objects[temp_dc_idx].dc_object, obj, false);

						//Make sure it worked
						if (fxn_DC_status == FAILED)
						{
							//Pull the object from the array - this is just for readability (otherwise the
							temp_DC_obj = dc_interface_objects[temp_dc_idx].dc_object;

							//Error it up
							GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_DC_obj->id, (temp_DC_obj->name ? temp_DC_obj->name : "Unnamed"));
							//Defined elsewhere
						}
					}

					//I_DC is done now.  If P_DC isn't, it could be calculated
				} //End DC object update
			}//End DYNAMIC_DC_BUS

			return SM_DELTA;	//Short circuit it
		}

		// Check pass
		if (iteration_count_val == 0) // Predictor pass
		{
			//Calculate injection current based on voltage source magnitude and angle obtained
			if (parent_is_single_phase) // single phase/split-phase implementation
			{


				//Update output power
				//Get current injected

				terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];


				terminal_current_val_pu[0] = terminal_current_val[0]/I_base;


				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];


				VA_Out = power_val[0];

				// The following code is only for three phase system
				// Function: Low pass filter of P
				P_out_pu = VA_Out.Re() / S_base;
				
				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
				{
					if (!dc_interface_objects.empty())
					{
						int temp_idx;
						//V_DC was set here, somehow
						STATUS fxn_return_status;

						//Loop through and call the DC objects
						for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
						{
							//DC object, calling object (us), init mode (true/false)
							//False at end now, because not initialization
							fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

							//Make sure it worked
							if (fxn_return_status == FAILED)
							{
								//Pull the object from the array - this is just for readability (otherwise the
								OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

								//Error it up
								GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
								//Defined above
							}
						}

						//I_DC is done now.  If P_DC isn't, it could be calculated
					} //End DC object update

					I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				}

				// Function: Low pass filter of V
				pCircuit_V_Avg_pu = value_Circuit_V[0].Mag() / V_base;

				if(grid_forming_mode == DYNAMIC_DC_BUS)
				{

				}
				else
				{

				}

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{

				}

				for (i = 0; i < 1; i++)
				{

				}

				simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass



			}
			else //Three-phase
			{
				//Update output power
				//Get current injected

				terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

				terminal_current_val_pu[0] = terminal_current_val[0]/I_base;
				terminal_current_val_pu[1] = terminal_current_val[1]/I_base;
				terminal_current_val_pu[2] = terminal_current_val[2]/I_base;


				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				// The following code is only for three phase system
				// Function: Low pass filter of P
				P_out_pu = VA_Out.Re() / S_base;

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
				{
					if (!dc_interface_objects.empty())
					{
						int temp_idx;
						//V_DC was set here, somehow
						STATUS fxn_return_status;


						//Loop through and call the DC objects
						for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
						{
							//DC object, calling object (us), init mode (true/false)
							//False at end now, because not initialization
							fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

							//Make sure it worked
							if (fxn_return_status == FAILED)
							{
								//Pull the object from the array - this is just for readability (otherwise the
								OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

								//Error it up
								GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
								//Defined above
							}
						}

						//I_DC is done now.  If P_DC isn't, it could be calculated
					} //End DC object update

					I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				}

				// Function: Low pass filter of V
				pCircuit_V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;

				if(grid_forming_mode == DYNAMIC_DC_BUS)
				{
				}
				else
				{

				}

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{

				}

				for (i = 0; i < 3; i++)
				{

				}

				simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass
			}
		}								   // end of grid-forming, predictor pass
		else if (iteration_count_val == 1) // Corrector pass
		{
			if (parent_is_single_phase) // single phase/split-phase implementation
			{

				//Update output power
				//Get current injected

				terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];


				terminal_current_val_pu[0] = terminal_current_val[0]/I_base;


				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];


				VA_Out = power_val[0];

				// The following code is only for three phase system
				// Function: Low pass filter of P
				P_out_pu = VA_Out.Re() / S_base;


				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
				{
					if (!dc_interface_objects.empty())
					{
						int temp_idx;
						//V_DC was set here, somehow
						STATUS fxn_return_status;

						//Loop through and call the DC objects
						for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
						{
							//DC object, calling object (us), init mode (true/false)
							//False at end now, because not initialization
							fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

							//Make sure it worked
							if (fxn_return_status == FAILED)
							{
								//Pull the object from the array - this is just for readability (otherwise the
								OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

								//Error it up
								GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
								//Defined above
							}
						}

						//I_DC is done now.  If P_DC isn't, it could be calculated
					} //End DC object update

					I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				}

				// Function: Low pass filter of V
				pCircuit_V_Avg_pu = value_Circuit_V[0].Mag() / V_base;

				if(grid_forming_mode == DYNAMIC_DC_BUS)
				{
				}
				else
				{

				}


				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{

				}

				// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
				for (i = 0; i < 1; i++)
				{

				  // IBR_SKELETON_NOTE: Add check for convergence check for qsts here

				}

				// IBR_SKELETON_NOTE: Add logic for returning to QSTS mode, if applicable, here
			}
			else //Three-phase
			{
				//Update output power
				//Get current injected

				terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);


				terminal_current_val_pu[0] = terminal_current_val[0]/I_base;
				terminal_current_val_pu[1] = terminal_current_val[1]/I_base;
				terminal_current_val_pu[2] = terminal_current_val[2]/I_base;


				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				// The following code is only for three phase system
				// Function: Low pass filter of P
				P_out_pu = VA_Out.Re() / S_base;

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
				{
					if (!dc_interface_objects.empty())
					{
						int temp_idx;
						//V_DC was set here, somehow
						STATUS fxn_return_status;

						//Loop through and call the DC objects
						for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
						{
							//DC object, calling object (us), init mode (true/false)
							//False at end now, because not initialization
							fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

							//Make sure it worked
							if (fxn_return_status == FAILED)
							{
								//Pull the object from the array - this is just for readability (otherwise the
								OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

								//Error it up
								GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
								//Defined above
							}
						}

						//I_DC is done now.  If P_DC isn't, it could be calculated
					} //End DC object update

					I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				}

				// Function: Low pass filter of V
				pCircuit_V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;


				if(grid_forming_mode == DYNAMIC_DC_BUS)
				{

				}
				else
				{

				}

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{

				}

				// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
				for (i = 0; i < 3; i++)
				{


				}

				// IBR_SKELETON_NOTE: Add logic for returning to QSTS mode, if applicable, here
			}
		}
		else //Additional iterations
		{
			//Just return whatever our "last desired" was
			simmode_return_value = desired_simulation_mode;
		}
	} // end of grid-forming
	else if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE))
	{
		//Make sure we're active/valid - if we've been tripped/disconnected, don't do an update
		if (value_MeterStatus == 1)
		{
			// Check pass
			if (iteration_count_val == 0) // Predictor pass
			{
				//Calculate injection current based on voltage soruce magtinude and angle obtained
				if (parent_is_single_phase) // single phase/split-phase implementation
				{
					//Update output power
					//Get current injected
					terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

					//Update per-unit value
					terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

					power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

					//Update power output variables, just so we can see what is going on
					VA_Out = power_val[0];


					// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
					if(Pref > S_base)
					{
						Pref = S_base;
						gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						The active power Pref for the ibr_skeleton is above the rated value.  It has been capped at the rated value.
						*/
					}

					if(Pref < -S_base)
					{
						Pref = -S_base;
						gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
						//Defined above
					}
					//Function end

					if (control_mode == GRID_FOLLOWING)
					{

					}
					else if (control_mode == GFL_CURRENT_SOURCE)
					{

					}

					simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass
				}
				else //Three-phase
				{
					if ((grid_following_mode == BALANCED_POWER)||(grid_following_mode == POSITIVE_SEQUENCE))
					{
						//Update output power
						//Get current injected
						terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
						terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
						terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

						//Update per-unit values
						terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
						terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
						terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

						//Update power output variables, just so we can see what is going on
						power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
						power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
						power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

						VA_Out = power_val[0] + power_val[1] + power_val[2];

						// Function: Coordinate Tranformation, xy to dq
						for (i = 0; i < 3; i++)
						{

						}

						if(grid_following_mode == BALANCED_POWER)
						{
							// Function: Phase-Lock_Loop, PLL
							for (i = 0; i < 3; i++)
							{

							}
						}
						else if(grid_following_mode == POSITIVE_SEQUENCE)
						{
							// Obtain the positive sequence voltage
							value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;


							// Function: Phase-Lock_Loop, PLL, only consider positive sequence voltage
							for (i = 0; i < 1; i++)
							{

							}
							
						}


						// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
						if(Pref > S_base)
						{
							Pref = S_base;
							gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Pref < -S_base)
						{
							Pref = -S_base;
							gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
						//Function end

						// Function: Current Control Loop
						for (i = 0; i < 3; i++)
						{
							if (control_mode == GRID_FOLLOWING)
							{

							}
							else if (control_mode == GFL_CURRENT_SOURCE)
							{

							}
						}

						simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass

					}	 // end of grid-following
				}	  // end of three phase code of grid-following
			}
			else if (iteration_count_val == 1) // Corrector pass
			{
				//Caluclate injection current based on voltage soruce magtinude and angle obtained
				if (parent_is_single_phase) // single phase/split-phase implementation
				{
					//Update output power
					//Get current injected
					terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

					//Update per-unit values
					terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

					power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

					//Update power output variables, just so we can see what is going on
					VA_Out = power_val[0];

					// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
					if(Pref > S_base)
					{
						Pref = S_base;
						gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
						//Defined above
					}

					if(Pref < -S_base)
					{
						Pref = -S_base;
						gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
						//Defined above
					}

					if(control_mode == GRID_FOLLOWING)
					{

					}
					else if (control_mode == GFL_CURRENT_SOURCE)
					{

					}

					//Compute a difference
					mag_diff_val = (value_IGenerated[0]-prev_value_IGenerated[0]).Mag();

					//Update tracker
					prev_value_IGenerated[0] = value_IGenerated[0];
					
					//Get the "convergence" test to see if we can exit to QSTS
					if (mag_diff_val > GridFollowing_curr_convergence_criterion)
					{
						//Set return mode
						simmode_return_value = SM_DELTA;

						//Set flag
						deltamode_exit_iteration_met = false;
					}
					else
					{
						//Check and see if we met our iteration criterion (due to how sequencing happens)
						if (deltamode_exit_iteration_met)
						{
							simmode_return_value = SM_EVENT;
						}
						else
						{
							//Reiterate once
							simmode_return_value = SM_DELTA;

							//Set the flag
							deltamode_exit_iteration_met = true;
						}
					}
				}
				else //Three-phase
				{
					if ((grid_following_mode == BALANCED_POWER)||(grid_following_mode == POSITIVE_SEQUENCE))
					{
						//Update output power
						//Get current injected
						terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
						terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
						terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

						//Update per-unit values
						terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
						terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
						terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

						//Update power output variables, just so we can see what is going on
						power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
						power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
						power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

						VA_Out = power_val[0] + power_val[1] + power_val[2];

						// Function: Coordinate Tranformation, xy to dq
						for (i = 0; i < 3; i++)
						{

						}


						if(grid_following_mode == BALANCED_POWER)
						{
							// Function: Phase-Lock_Loop, PLL
							for (i = 0; i < 3; i++)
							{

							}

						}
						else if(grid_following_mode == POSITIVE_SEQUENCE)
						{
							// Obtain the positive sequence voltage
							value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;


							// Function: Phase-Lock_Loop, PLL, only consider the positive sequence voltage
							for (i = 0; i < 1; i++)
							{

							}

						}

						// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
						if(Pref > S_base)
						{
							Pref = S_base;
							gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Pref < -S_base)
						{
							Pref = -S_base;
							gl_warning("ibr_skeleton:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
						//Function end



						//Set default return state
						simmode_return_value = SM_EVENT;	//default to event-driven

						// Function: Current Control Loop
						for (i = 0; i < 3; i++)
						{

							if(control_mode == GRID_FOLLOWING)
							{

							}
							else if(control_mode == GFL_CURRENT_SOURCE)
							{

							}

							//Compute the difference
							mag_diff_val = (value_IGenerated[i]-prev_value_IGenerated[i]).Mag();

							//Update tracker
							prev_value_IGenerated[i] = value_IGenerated[i];
							
							//Get the "convergence" test to see if we can exit to QSTS
							if (mag_diff_val > GridFollowing_curr_convergence_criterion)
							{
								simmode_return_value = SM_DELTA;

								//Force the iteration flag
								deltamode_exit_iteration_met = false;
							}
							//Default - was set to SM_EVENT above
						}

						//Check and see if we met our iteration criterion (due to how sequencing happens)
						if (simmode_return_value == SM_EVENT)
						{
							if (!deltamode_exit_iteration_met)
							{
								//Reiterate once
								simmode_return_value = SM_DELTA;

								//Set the flag
								deltamode_exit_iteration_met = true;
							}
							//Must be true, and already SM_EVENT, so exit
						}

					}	 // end of grid-following
				}
			}	 // end of three phase grid-following, corrector pass
			else //Additional iterations
			{
				//Just return whatever our "last desired" was
				simmode_return_value = desired_simulation_mode;
			}
		}//End valid meter
		else
		{
			//Something disconnected - just flag us for event
			simmode_return_value = SM_EVENT;

			//Actually "zeroing" of powerflow is handled in the current update

			//Zero VA_Out though
			VA_Out = gld::complex(0.0,0.0);

			//Zero the "recordable current" outputs
			terminal_current_val[0] = gld::complex(0.0,0.0);
			terminal_current_val[1] = gld::complex(0.0,0.0);
			terminal_current_val[2] = gld::complex(0.0,0.0);

			//Set the per-unit to zero too, to be thorough
			terminal_current_val_pu[0] = gld::complex(0.0,0.0);
			terminal_current_val_pu[1] = gld::complex(0.0,0.0);
			terminal_current_val_pu[2] = gld::complex(0.0,0.0);
		}
	} // end of grid-following

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}

	//Set the mode tracking variable for this exit
	desired_simulation_mode = simmode_return_value;
	
	return simmode_return_value;
}

//Initializes dynamic equations for first entry
STATUS ibr_skeleton::init_dynamics()
{
	OBJECT *obj = OBJECTHDR(this);

	//Pull the powerflow values
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	if (control_mode == GRID_FORMING)
	{

		if (parent_is_single_phase) // single phase/split-phase implementation
		{

			//Update output power
			//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
			terminal_current_val[0] = value_IGenerated[0] - (filter_admittance * value_Circuit_V[0]);

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];


			VA_Out = power_val[0];

			//
			pCircuit_V_Avg_pu = value_Circuit_V[0].Mag() / V_base;

			for (int i = 0; i < 1; i++)
			{

			}

			//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
			if (first_deltamode_init)
			{
				//Set it false in here, for giggles
				first_deltamode_init = false;
			}
			//Default else - all changes should be in deltamode

			// Initialize Vdc_min controller and DC bus voltage
			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
			{
				//See if there are any DC objects to handle
				if (!dc_interface_objects.empty())
				{
					//Figure out what our DC power is
					P_DC = VA_Out.Re();

					int temp_idx;
					STATUS fxn_return_status;

					//Loop through and call the DC objects
					for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
					{
						//May eventually have a "DC ratio" or similar, if multiple objects on bus or "you are DC voltage master"
						//DC object, calling object (us), init mode (true/false)
						fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, true);

						//Make sure it worked
						if (fxn_return_status == FAILED)
						{
							//Pull the object from the array - this is just for readability (otherwise the
							OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

							//Error it up
							GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
							/*  TROUBLESHOOT
							While performing the update to a DC-bus object on this inverter, an error occurred.  Please try again.
							If the error persists, please check your model.  If the model appears correct, please submit a bug report via the issues tracker.
							*/
						}
					}

					//Theoretically, the DC objects have no set V_DC and I_DC appropriately - updated equations would go here
				} //End DC object update

			}

		}
		//  three-phase system
		if ((phases & 0x07) == 0x07)
		{

			//Update output power
			//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
			terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			//Compute the average value of three-phase terminal voltages
			pCircuit_V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;

			for (int i = 0; i < 3; i++)
			{

			}

			//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
			if (first_deltamode_init)
			{
				//Set it false in here, for giggles
				first_deltamode_init = false;
			}
			//Default else - all changes should be in deltamode

			// Initialize Vdc_min controller and DC bus voltage
			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
			{
				//See if there are any DC objects to handle
				if (!dc_interface_objects.empty())
				{
					//Figure out what our DC power is
					P_DC = VA_Out.Re();

					int temp_idx;
					STATUS fxn_return_status;

					//Loop through and call the DC objects
					for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
					{
						//May eventually have a "DC ratio" or similar, if multiple objects on bus or "you are DC voltage master"
						//DC object, calling object (us), init mode (true/false)
						fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, true);

						//Make sure it worked
						if (fxn_return_status == FAILED)
						{
							//Pull the object from the array - this is just for readability (otherwise the
							OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

							//Error it up
							GL_THROW("ibr_skeleton:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
							/*  TROUBLESHOOT
							While performing the update to a DC-bus object on this inverter, an error occurred.  Please try again.
							If the error persists, please check your model.  If the model appears correct, please submit a bug report via the issues tracker.
							*/
						}
					}

					//Theoretically, the DC objects have no set V_DC and I_DC appropriately - updated equations would go here
				} //End DC object update

			}
		}
	}
	else if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE))
	{

		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			//Update output power
			//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
			terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

			VA_Out = power_val[0];

			//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
			if (first_deltamode_init)
			{
				//Set it false in here, for giggles
				first_deltamode_init = false;
			}
			//Default else - all changes should be in deltamode

			if(control_mode == GRID_FOLLOWING)
			{

			}
			else if(control_mode == GFL_CURRENT_SOURCE)
			{

			}

		}
		else //Three-phase
		{
			if ((grid_following_mode == BALANCED_POWER)||(grid_following_mode == POSITIVE_SEQUENCE))
			{
				//Update output power
				//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
				terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

				//Update per-unit values
				terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
				terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
				terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				//Pref = VA_Out.Re();
				//Qref = VA_Out.Im();
				//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
				if (first_deltamode_init)
				{
					//See if it was set


					//Set it false in here, for giggles
					first_deltamode_init = false;
				}
				//Default else - all changes should be in deltamode

				if(grid_following_mode == BALANCED_POWER)
				{
					for (int i = 0; i < 3; i++)
					{

					}
				}
				else if(grid_following_mode == POSITIVE_SEQUENCE)
				{
					// Obtain the positive sequence voltage
					value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;

					for (int i = 0; i < 1; i++)
					{

					}
				}

				for (int i = 0; i < 3; i++)
				{

					if (control_mode == GRID_FOLLOWING)
					{

					}
					else if(control_mode == GFL_CURRENT_SOURCE)
					{

					}
				}

			}	 // end of three phase initialization

		}
	}
	return SUCCESS;
}

// //Module-level post update call
// /* Think this was just put here as an example - not sure what it would do */
// STATUS ibr_skeleton::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
// {
// 	//If we have a meter, reset the accumulators
// 	if (parent_is_a_meter)
// 	{
// 		reset_complex_powerflow_accumulators();
// 	}

// 	//Should have a parent, but be paranoid
// 	if (parent_is_a_meter)
// 	{
// 		push_complex_powerflow_values();
// 	}

// 	return SUCCESS; //Always succeeds right now
// }

//Map Complex value
gld_property *ibr_skeleton::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("ibr_skeleton:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *ibr_skeleton::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("ibr_skeleton:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull all the complex properties from powerflow into local variables
void ibr_skeleton::pull_complex_powerflow_values(void)
{
	//Pull in the various values from powerflow - straight reads
	//Pull status
	value_MeterStatus = pMeterStatus->get_enumeration();

	//********** TODO - Portions of this may need to be a "deltamode only" pull	 **********//
	//Update IGenerated, in case the powerflow is overriding it
	if (parent_is_single_phase)
	{
		//Just pull "zero" values
		value_Circuit_V[0] = pCircuit_V[0]->get_complex();

		value_IGenerated[0] = pIGenerated[0]->get_complex();
	}
	else
	{
		//Pull all voltages
		value_Circuit_V[0] = pCircuit_V[0]->get_complex();
		value_Circuit_V[1] = pCircuit_V[1]->get_complex();
		value_Circuit_V[2] = pCircuit_V[2]->get_complex();

		//Pull currents
		value_IGenerated[0] = pIGenerated[0]->get_complex();
		value_IGenerated[1] = pIGenerated[1]->get_complex();
		value_IGenerated[2] = pIGenerated[2]->get_complex();
	}
}

//Function to reset the various accumulators, so they don't double-accumulate if they weren't used
void ibr_skeleton::reset_complex_powerflow_accumulators(void)
{
	int indexval;

	//See which one we are, since that will impact things
	if (!parent_is_single_phase) //Three-phase
	{
		//Loop through the three-phases/accumulators
		for (indexval = 0; indexval < 3; indexval++)
		{
			//**** Current value ***/
			value_Line_I[indexval] = gld::complex(0.0, 0.0);

			//**** Power value ***/
			value_Power[indexval] = gld::complex(0.0, 0.0);

			//**** pre-rotated Current value ***/
			value_Line_unrotI[indexval] = gld::complex(0.0, 0.0);
		}
	}
	else //Assumes must be single phased - else how did it get here?
	{
		//Reset the relevant values -- all single pulls

		//**** single current value ***/
		value_Line_I[0] = gld::complex(0.0, 0.0);

		//**** power value ***/
		value_Power[0] = gld::complex(0.0, 0.0);

		//**** prerotated value ***/
		value_Line_unrotI[0] = gld::complex(0.0, 0.0);
	}
}

//Function to push up all changes of complex properties to powerflow from local variables
void ibr_skeleton::push_complex_powerflow_values(bool update_voltage)
{
	gld::complex temp_complex_val;
	gld_wlock *test_rlock = nullptr;
	int indexval;

	//See which one we are, since that will impact things
	if (!parent_is_single_phase) //Three-phase
	{
		//See if we were a voltage push or not
		if (update_voltage)
		{
			//Loop through the three-phases/accumulators
			for (indexval=0; indexval<3; indexval++)
			{
				//**** push voltage value -- not an accumulator, just force ****/
				pCircuit_V[indexval]->setp<gld::complex>(value_Circuit_V[indexval],*test_rlock);
			}
		}
		else
		{
			//Loop through the three-phases/accumulators
			for (indexval = 0; indexval < 3; indexval++)
			{
				//**** Current value ***/
				//Pull current value again, just in case
				temp_complex_val = pLine_I[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Line_I[indexval];

				//Push it back up
				pLine_I[indexval]->setp<gld::complex>(temp_complex_val, *test_rlock);

				//**** Power value ***/
				//Pull current value again, just in case
				temp_complex_val = pPower[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Power[indexval];

				//Push it back up
				pPower[indexval]->setp<gld::complex>(temp_complex_val, *test_rlock);

				//**** pre-rotated Current value ***/
				//Pull current value again, just in case
				temp_complex_val = pLine_unrotI[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Line_unrotI[indexval];

				//Push it back up
				pLine_unrotI[indexval]->setp<gld::complex>(temp_complex_val, *test_rlock);

				/* If was VSI, adjust Norton injection */
				{
					//**** IGenerated Current value ***/
					//Direct write, not an accumulator
					pIGenerated[indexval]->setp<gld::complex>(value_IGenerated[indexval], *test_rlock);
				}
			}//End phase loop
		}//End not voltage push
	}//End three-phase
	else //Assumes must be single-phased - else how did it get here?
	{
		//Check for voltage push - in case that's ever needed here
		if (update_voltage)
		{
			//Should just be zero
			//**** push voltage value -- not an accumulator, just force ****/
			pCircuit_V[0]->setp<gld::complex>(value_Circuit_V[0],*test_rlock);
		}
		else
		{
			//Pull the relevant values -- all single pulls

			//**** Current value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_I[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Line_I[0];

			//Push it back up
			pLine_I[0]->setp<gld::complex>(temp_complex_val, *test_rlock);

			//**** power value ***/
			//Pull current value again, just in case
			temp_complex_val = pPower[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Power[0];

			//Push it back up
			pPower[0]->setp<gld::complex>(temp_complex_val, *test_rlock);

			//**** prerotated value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_unrotI[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Line_unrotI[0];

			//Push it back up
			pLine_unrotI[0]->setp<gld::complex>(temp_complex_val, *test_rlock);

			//**** IGenerated ****/
			//********* TODO - Does this need to be deltamode-flagged? *************//
			//Direct write, not an accumulator
			pIGenerated[0]->setp<gld::complex>(value_IGenerated[0], *test_rlock);
		}//End not voltage update
	}//End single-phase
}

// Function to update current injection IGenerated for VSI
STATUS ibr_skeleton::updateCurrInjection(int64 iteration_count,bool *converged_failure)
{
	double temp_time;
	OBJECT *obj = OBJECTHDR(this);
	gld::complex temp_VA;
	gld::complex temp_V1, temp_V2;
	bool bus_is_a_swing, bus_is_swing_pq_entry;
	STATUS temp_status_val;
	gld_property *temp_property_pointer;
	bool running_in_delta;
	bool limit_hit;
	double freq_diff_angle_val, tdiff;
	double mag_diff_val[3];
	gld::complex rotate_value;
	char loop_var;
	gld::complex intermed_curr_calc[3];

	//Assume starts converged (no failure)
	*converged_failure = false;

	if (deltatimestep_running > 0.0) //Deltamode call
	{
		//Get the time
		temp_time = gl_globaldeltaclock;
		running_in_delta = true;
	}
	else
	{
		//Grab the current time
		temp_time = (double)gl_globalclock;
		running_in_delta = false;
	}

	//Pull the current powerflow values
	if (parent_is_a_meter)
	{
		//Reset the accumulators, just in case
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//See if we're in QSTS and a grid-forming inverter - update if we are
	if (!running_in_delta && (control_mode == GRID_FORMING))
	{
		//Update trackers
		prev_timestamp_dbl = temp_time;
		last_QSTS_GF_Update = temp_time;
	}

	//External call to internal variables -- used by powerflow to iterate the VSI implementation, basically

	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_skeleton:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	if(Pref > S_base)
	{
		Pref = S_base;
	}
	else if (Pref < -S_base)
	{
		Pref = -S_base;
	}

	if(Qref > sqrt(S_base*S_base-Pref*Pref))
	  {
	    Qref = sqrt(S_base*S_base-Pref*Pref);
	  }
	else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
	  {
	    Qref = -sqrt(S_base*S_base-Pref*Pref);
	  }
	

	temp_VA = gld::complex(Pref, Qref);

	//See if we're a meter
	if (parent_is_a_meter)
	{
		//By default, assume we're not a SWING
		bus_is_a_swing = false;

		//Determine our status
		if (attached_bus_type > 1) //SWING or SWING_PQ
		{
			//Map the function, if we need to
			if (swing_test_fxn == nullptr)
			{
				//Map the swing status check function
				swing_test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_object_swing_status_check"));

				//See if it was located
				if (swing_test_fxn == nullptr)
				{
					GL_THROW("ibr_skeleton:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the swing-checking function, an error was encountered.
					Please try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}
			}

			//Call the testing function
			temp_status_val = ((STATUS (*)(OBJECT *,bool * , bool*))(*swing_test_fxn))(obj->parent,&bus_is_a_swing,&bus_is_swing_pq_entry);

			//Make sure it worked
			if (temp_status_val != SUCCESS)
			{
				GL_THROW("ibr_skeleton:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
				//Defined above
			}

			//Now see how we've gotten here
			if (first_iteration_current_injection == -1) //Haven't entered before
			{
				//See if we are truely the first iteration
				if (iteration_count != 0)
				{
					//We're not, which means we were a SWING or a SWING_PQ "demoted" after balancing was completed - override the indication
					//If it was already true, well, we just set it again
					bus_is_a_swing = true;
				}
				//Default else - we are zero, so we can just leave the "SWING status" correct

				//Update the iteration counter
				first_iteration_current_injection = iteration_count;
			}
			else if ((first_iteration_current_injection != 0) || bus_is_swing_pq_entry) //We didn't enter on the first iteration, or we're a repatrioted SWING_PQ
			{
				//Just override the indication - this only happens if we were a SWING or a SWING_PQ that was "demoted"
				bus_is_a_swing = true;
			}
			//Default else is zero - we entered on the first iteration, which implies we are either a PQ bus,
			//or were a SWING_PQ that was behaving as a PQ from the start
		}
		//Default else - keep bus_is_a_swing as false
	}
	else
	{
		//Just assume the object is not a swing
		bus_is_a_swing = false;
	}

	if (inverter_first_step)
	{
		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			if (control_mode == GRID_FORMING)
			{
				if (!bus_is_a_swing)
				{
					//Balance voltage and satisfy power
					gld::complex temp_total_power_val;
					gld::complex temp_total_power_internal;
					gld::complex temp_pos_current;

					//Calculate the Norton-shunted power
					temp_total_power_val = value_Circuit_V[0] * ~(filter_admittance * value_Circuit_V[0] );

					//Figure out what we should be generating internally
					temp_total_power_internal = temp_VA + temp_total_power_val;

					//Compute the positive sequence current
					temp_pos_current = ~(temp_total_power_internal / value_Circuit_V[0]);

					//Now populate this into the output
					value_IGenerated[0] = temp_pos_current;
				}
			}

			if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE)) //
			{
				// **FT Note** Not sure what this code does.  Mapps are to value_circuit_V[0], which should already be 12
				// //Special exception for triplex (since delta doesn't get updated inside the powerflow)
				// if ((phases & 0x10) == 0x10)
				// {
				// 	//Pull in other voltages
				// 	temp_V1 = pCircuit_V[1]->get_complex();
				// 	temp_V2 = pCircuit_V[2]->get_complex();

				// 	//Just push it in as the update
				// 	value_Circuit_V[0] = temp_V1 + temp_V2;
				// }

				//Calculate the terminate current
				if (value_Circuit_V[0].Mag() > 0.0)
				{
					terminal_current_val[0] = ~(temp_VA / value_Circuit_V[0]);
				}
				else
				{
					terminal_current_val[0] = gld::complex(0.0,0.0);
				}

				//Update per-unit value
				terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

				//Calculate the internal current
				value_IGenerated[0] = terminal_current_val[0] + filter_admittance * value_Circuit_V[0] ; //

				//Calculate the difference
				mag_diff_val[0] = (value_IGenerated[0]-prev_value_IGenerated[0]).Mag();

				//Update tracker
				prev_value_IGenerated[0] = value_IGenerated[0];
				
				//Get the "convergence" test to see if we can exit to QSTS
				if (mag_diff_val[0] > GridFollowing_curr_convergence_criterion)
				{
					//Mark as a failure
					*converged_failure = true;
				}
			}
		}
		else //Three-phase
		{

			if (control_mode == GRID_FORMING)
			{
				if (!bus_is_a_swing)
				{
					//Balance voltage and satisfy power
					gld::complex aval, avalsq;
					gld::complex temp_total_power_val[3];
					gld::complex temp_total_power_internal;
					gld::complex temp_pos_voltage, temp_pos_current;

					//Conversion variables - 1@120-deg
					aval = gld::complex(-0.5, (sqrt(3.0) / 2.0));
					avalsq = aval * aval; //squared value is used a couple places too

					//Calculate the Norton-shunted power
					temp_total_power_val[0] = value_Circuit_V[0] * ~(generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2]);
					temp_total_power_val[1] = value_Circuit_V[1] * ~(generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2]);
					temp_total_power_val[2] = value_Circuit_V[2] * ~(generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2]);

					//Figure out what we should be generating internally
					temp_total_power_internal = temp_VA + temp_total_power_val[0] + temp_total_power_val[1] + temp_total_power_val[2];

					//Compute the positive sequence voltage (*3)
					temp_pos_voltage = value_Circuit_V[0] + value_Circuit_V[1] * aval + value_Circuit_V[2] * avalsq;

					//Compute the positive sequence current
					temp_pos_current = ~(temp_total_power_internal / temp_pos_voltage);

					//Now populate this into the output
					value_IGenerated[0] = temp_pos_current;      // for grid-following inverters, the internal voltages need to be three phase balanced
					value_IGenerated[1] = temp_pos_current * avalsq;
					value_IGenerated[2] = temp_pos_current * aval;
				}
				//If it is a swing, this is all handled internally to solver_nr (though it is similar)
			}
			else //grid following, or current source representation of grid-following
			{
				if (grid_following_mode == BALANCED_POWER) // Assume the inverter injects balanced power
				{
					for (loop_var=0; loop_var<3; loop_var++)
					{
						if (value_Circuit_V[loop_var].Mag() > 0.0)
						{
							intermed_curr_calc[loop_var] = ~(temp_VA / 3.0 / value_Circuit_V[loop_var]);
						}
						else
						{
							intermed_curr_calc[loop_var] = gld::complex(0.0,0.0);
						}
					}


                    terminal_current_val[0] = intermed_curr_calc[0];
                    terminal_current_val[1] = intermed_curr_calc[1];					
                    terminal_current_val[2] = intermed_curr_calc[2];	
					
					//Update per-unit value
					terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
					terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
					terminal_current_val_pu[2] = terminal_current_val[2] / I_base;
					

					//Back out the current injection
					value_IGenerated[0] = intermed_curr_calc[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
					value_IGenerated[1] = intermed_curr_calc[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
					value_IGenerated[2] = intermed_curr_calc[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];


				}
				else //Assume injects balanced current, positive sequence
				{
					// Obtain the positive sequence voltage
					value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
					
					//Check value
					if (value_Circuit_V_PS.Mag() > 0.0)
					{
						// Obtain the positive sequence current
						value_Circuit_I_PS[0] = ~(temp_VA / 3.0 / value_Circuit_V_PS);
					}
					else
					{
						value_Circuit_I_PS[0] = gld::complex(0.0,0.0);
					}
					
					value_Circuit_I_PS[1] = value_Circuit_I_PS[0] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI));
					value_Circuit_I_PS[2] = value_Circuit_I_PS[0] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI));


                    terminal_current_val[0] = value_Circuit_I_PS[0];
                    terminal_current_val[1] = value_Circuit_I_PS[1];					
                    terminal_current_val[2] = value_Circuit_I_PS[2];
					
					//Update per-unit values
					terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
					terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
					terminal_current_val_pu[2] = terminal_current_val[2] / I_base;					

					//Back out the current injection
					value_IGenerated[0] = value_Circuit_I_PS[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
					value_IGenerated[1] = value_Circuit_I_PS[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
					value_IGenerated[2] = value_Circuit_I_PS[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];

				}

				//Calculate the difference
				mag_diff_val[0] = (value_IGenerated[0]-prev_value_IGenerated[0]).Mag();
				mag_diff_val[1] = (value_IGenerated[1]-prev_value_IGenerated[1]).Mag();
				mag_diff_val[2] = (value_IGenerated[2]-prev_value_IGenerated[2]).Mag();

				//Update trackers
				prev_value_IGenerated[0] = value_IGenerated[0];
				prev_value_IGenerated[1] = value_IGenerated[1];
				prev_value_IGenerated[2] = value_IGenerated[2];
				
				//Get the "convergence" test to see if we can exit to QSTS
				if ((mag_diff_val[0] > GridFollowing_curr_convergence_criterion) || (mag_diff_val[1] > GridFollowing_curr_convergence_criterion) || (mag_diff_val[2] > GridFollowing_curr_convergence_criterion))
				{
					//Mark as a failure
					*converged_failure = true;
				}
			}//End Grid-following
		}//End three-phase
	}
	//Default else - things are handled elsewhere

	//Check to see if we're disconnected
	if (value_MeterStatus == 0)
	{
		//Disconnected - offset our admittance contribution - could remove it from powerflow - visit this in the future
		if (parent_is_single_phase)
		{
			value_IGenerated[0] = filter_admittance * value_Circuit_V[0];
		}
		else	//Assumed to be a three-phase variant
		{
			value_IGenerated[0] = generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
			value_IGenerated[1] = generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
			value_IGenerated[2] = generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];
		}
	}//End Disconnected
	else
	{
		if(control_mode == GRID_FORMING)
		{
			//Connected of some form - do a current limiting check (if supported)
			if (parent_is_single_phase)
			{

				terminal_current_val[0] = value_IGenerated[0] - value_Circuit_V[0] / (gld::complex(Rfilter, Xfilter) * Z_base);

				//Make a per-unit value for comparison
				terminal_current_val_pu[0] = terminal_current_val[0]/I_base;

				//Update the injection
				value_IGenerated[0] = terminal_current_val[0] + value_Circuit_V[0] / (gld::complex(Rfilter, Xfilter) * Z_base);

				//Update the power, just to be consistent
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

				//Update the overall power
				VA_Out = power_val[0];

				//Default else - no need to adjust
			}
			else	//Three phase
			{
				//Reset the flag
				//limit_hit = false;

				//Loop the comparison - just because
				for (loop_var=0; loop_var<3; loop_var++)
				{
					//Compute "current" current value
					terminal_current_val[loop_var] = value_IGenerated[loop_var] - value_Circuit_V[loop_var] / (gld::complex(Rfilter, Xfilter) * Z_base);

					//Make a per-unit value for comparison
					terminal_current_val_pu[loop_var] = terminal_current_val[loop_var]/I_base;

					//Update the injection
					value_IGenerated[loop_var] = terminal_current_val[loop_var] + value_Circuit_V[loop_var] / (gld::complex(Rfilter, Xfilter) * Z_base);

					//Update the power, just to be consistent
					power_val[loop_var] = value_Circuit_V[loop_var] * ~terminal_current_val[loop_var];


					//Default else - no need to adjust
				}//End phase for

				VA_Out = power_val[0] + power_val[1] + power_val[2];

			}//End three-phase checks

			//Update the output variable and per-unit
		}
	}//End connected/working

	//Push the changes up
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}

	//Always a success, but power flow solver may not like it if VA_OUT exceeded the rating and thus changed
	return SUCCESS;
}

//Internal function to the mapping of the DC object update function
STATUS ibr_skeleton::DC_object_register(OBJECT *DC_object)
{
	FUNCTIONADDR temp_add = nullptr;
	DC_OBJ_FXNS_IBR temp_DC_struct;
	OBJECT *obj = OBJECTHDR(this);

	//Put the object into the structure
	temp_DC_struct.dc_object = DC_object;

	//Find the update function
	temp_DC_struct.fxn_address = (FUNCTIONADDR)(gl_get_function(DC_object, "DC_gen_object_update"));

	//Make sure it worked
	if (temp_DC_struct.fxn_address == nullptr)
	{
		gl_error("ibr_skeleton:%s - failed to map DC update for object %s", (obj->name ? obj->name : "unnamed"), (DC_object->name ? DC_object->name : "unnamed"));
		/*  TROUBLESHOOT
		While attempting to map the update function for a DC-bus device, an error was encountered.
		Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
		*/

		return FAILED;
	}

	//Push us onto the memory
	dc_interface_objects.push_back(temp_DC_struct);

    if (gl_object_isa(DC_object,"energy_storage","generators"))
    {

		//Map the battery SOC
    	pSOC = new gld_property(DC_object, "SOC_ES");

		//Check it
		if (!pSOC->is_valid() || !pSOC->is_double())
		{
			GL_THROW("ibr_skeleton:%s - failed to map battery SOC ", (obj->name ? obj->name : "unnamed"));
			/*  TROUBLESHOOT
			Failed to map battery SOC.
			*/
		}

		SOC = pSOC->get_double();  //

    }

	//If we made it this far, all should be good!
	return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_ibr_skeleton(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(ibr_skeleton::oclass);
		if (*obj != nullptr)
		{
			ibr_skeleton *my = OBJECTDATA(*obj, ibr_skeleton);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(ibr_skeleton);
}

EXPORT int init_ibr_skeleton(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != nullptr)
			return OBJECTDATA(obj, ibr_skeleton)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(ibr_skeleton);
}

EXPORT TIMESTAMP sync_ibr_skeleton(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	ibr_skeleton *my = OBJECTDATA(obj, ibr_skeleton);
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
	SYNC_CATCHALL(ibr_skeleton);
	return t2;
}

EXPORT int isa_ibr_skeleton(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,ibr_skeleton)->isa(classname);
}

EXPORT STATUS preupdate_ibr_skeleton(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	ibr_skeleton *my = OBJECTDATA(obj, ibr_skeleton);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0, delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_ibr_skeleton(obj=%d;%s): %s", obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_ibr_skeleton(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	ibr_skeleton *my = OBJECTDATA(obj, ibr_skeleton);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_ibr_skeleton(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

// EXPORT STATUS postupdate_ibr_skeleton(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
// {
// 	ibr_skeleton *my = OBJECTDATA(obj, ibr_skeleton);
// 	STATUS status = FAILED;
// 	try
// 	{
// 		status = my->post_deltaupdate(useful_value, mode_pass);
// 		return status;
// 	}
// 	catch (char *msg)
// 	{
// 		gl_error("postupdate_ibr_skeleton(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
// 		return status;
// 	}
// }

//// Define export function that update the VSI current injection IGenerated to the grid
EXPORT STATUS ibr_skeleton_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure)
{
	STATUS temp_status;

	//Map the node
	ibr_skeleton *my = OBJECTDATA(obj, ibr_skeleton);

	//Call the function, where we can update the IGenerated injection
	temp_status = my->updateCurrInjection(iteration_count,converged_failure);

	//Return what the sub function said we were
	return temp_status;
}

// Export function for registering a DC interaction object
EXPORT STATUS ibr_skeleton_DC_object_register(OBJECT *this_obj, OBJECT *DC_obj)
{
	STATUS temp_status;

	//Map us
	ibr_skeleton *this_inv = OBJECTDATA(this_obj, ibr_skeleton);

	//Call the function to register us
	temp_status = this_inv->DC_object_register(DC_obj);

	//Return the status
	return temp_status;
}
