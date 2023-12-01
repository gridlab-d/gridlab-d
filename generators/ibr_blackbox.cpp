// Skeleton/empty object with deltamode capabilities
// Comments in //**** ****// are brief descriptions of what a function does or where it may go.
#include<memory>

#include "ibr_blackbox.h"

CLASS *ibr_blackbox::oclass = nullptr;
ibr_blackbox *ibr_blackbox::defaults = nullptr;

static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
ibr_blackbox::ibr_blackbox(MODULE *module)
{
	if (oclass == nullptr)
	{
		oclass = gl_register_class(module, "ibr_blackbox", sizeof(ibr_blackbox), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == nullptr)
			throw "unable to register class ibr_blackbox";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			//**** This is where the mapping between the C/C++ programming variables and the "GLM-accessible" variables is done ****//
			//**** Anything you want to set in the GLM or have a player/recorder/HELICS manipulate need to be published here. ****//
			PT_complex, "phaseA_I_Out[A]", PADDR(value_IGenerated[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system",
			PT_complex, "phaseB_I_Out[A]", PADDR(value_IGenerated[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system",
			PT_complex, "phaseC_I_Out[A]", PADDR(value_IGenerated[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system",
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

			PT_double, "dc_input_voltage[V]", PADDR(dc_input_voltage),
			PT_double, "dc_output_current[A]", PADDR(dc_output_current),

			//Input
			PT_double, "rated_power[VA]", PADDR(S_base), PT_DESCRIPTION, " The rated power of the inverter",
			// Grid-Following Controller Parameters
			PT_double, "Pref[W]", PADDR(Pref), PT_DESCRIPTION, "DELTAMODE: The real power reference.",
			PT_double, "Qref[VAr]", PADDR(Qref), PT_DESCRIPTION, "DELTAMODE: The reactive power reference.",

			// ibr_blackbox_NOTE: Any variables that need to be published should go here.

			nullptr) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		defaults = this;

		memset(this, 0, sizeof(ibr_blackbox));

		if (gl_publish_function(oclass, "preupdate_gen_object", (FUNCTIONADDR)preupdate_ibr_blackbox) == nullptr)
			GL_THROW("Unable to publish ibr_blackbox deltamode function");
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_ibr_blackbox) == nullptr)
			GL_THROW("Unable to publish ibr_blackbox deltamode function");
		// if (gl_publish_function(oclass, "postupdate_gen_object", (FUNCTIONADDR)postupdate_ibr_blackbox) == nullptr)
		// 	GL_THROW("Unable to publish ibr_blackbox deltamode function");
		if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)ibr_blackbox_NR_current_injection_update) == nullptr)
			GL_THROW("Unable to publish ibr_blackbox current injection update function");
	}
}

//Isa function for identification
int ibr_blackbox::isa(char *classname)
{
	return strcmp(classname,"ibr_blackbox")==0;
}

/* Object creation is called once for each object that is created by the core */
//**** This sets up object defaults.  This is done before the GLM is read, so GLM properties will override these ****//
int ibr_blackbox::create(void)
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
	pIGenerated[0] = pIGenerated[1] = pIGenerated[2] = nullptr;

	pMeterStatus = nullptr; // check if the meter is in service

	//Zero the accumulators
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0, 0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = gld::complex(0.0, 0.0);
	prev_value_IGenerated[0] = prev_value_IGenerated[1] = prev_value_IGenerated[2] = gld::complex(0.0, 0.0);
	value_MeterStatus = 1; //Connected, by default

	f_nominal = 60;

	//Set up the deltamode "next state" tracking variable
	desired_simulation_mode = SM_EVENT;

	//Tracking variable
	last_QSTS_GF_Update = TS_NEVER_DBL;

	node_nominal_voltage = 120.0;		//Just pick a value

	//Initial values
	dc_input_voltage = 0.508587;
	dc_output_current = 0.0;

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
//**** This is after the GLM read, so it is where parameters can be checked for validity (e.g., make sure rated power isn't negative)  ****//
//**** This is also where a lot of "one-time actions", like mapping variables to parent or other objects, occurs.  ****//
int ibr_blackbox::init(OBJECT *parent)
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
	STATUS return_value_init, fxn_return_status;

	// // added by sunil
	// fdeep::tensor input_tensor(fdeep::tensor_shape(5, 7, 1), fdeep::float_vec(5 * 7 * 1, 0.0));
	// std::vector<std::vector<std::vector<float>>> simulation_data = {
	//     {{0, 0, 0, 0, 0, 0,0}},
	// 	{{0, 0, 0, 0, 0, 0,0}},
	// 	{{0, 0, 0, 0, 0, 0,0}},
	// 	{{0, 0, 0, 0, 0, 0,0}},
	//     {{0.508587, 0.747836, 0.749498, 0.749134, 0.405156, 0.11261,0.739529}}
	// };


	//Deferred initialization code
	if (parent != nullptr)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("ibr_blackbox::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	// Data sanity check
	if (S_base <= 0)
	{
		S_base = 1000;
		gl_warning("ibr_blackbox:%d - %s - The rated power of this inverter must be positive - set to 1 kVA.",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		The ibr_blackbox has a rated power that is negative.  It defaulted to a 1 kVA inverter.  If this is not desired, set the property properly in the GLM.
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
						GL_THROW("ibr_blackbox:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
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
					//Default else - Implies it is a powerflow parent
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

			//Pull the bus type
			temp_property_pointer = new gld_property(tmp_obj, "bustype");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("ibr_blackbox:%s failed to map bustype variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
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

				//Map IGenerated, even though triplex can't really use this yet (just for the sake of doing so)
				pIGenerated[0] = map_complex_value(tmp_obj, "prerotated_current_12");
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

					//Map the current injection variables
					pIGenerated[0] = map_complex_value(tmp_obj, "prerotated_current_A");
					pIGenerated[1] = map_complex_value(tmp_obj, "prerotated_current_B");
					pIGenerated[2] = map_complex_value(tmp_obj, "prerotated_current_C");
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

					pIGenerated[1] = nullptr;
					pIGenerated[2] = nullptr;

					if ((phases & 0x07) == 0x01) //A
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_A");
						pIGenerated[0] = map_complex_value(tmp_obj, "prerotated_current_A");
					}
					else if ((phases & 0x07) == 0x02) //B
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_B");
						pIGenerated[0] = map_complex_value(tmp_obj, "prerotated_current_B");
					}
					else if ((phases & 0x07) == 0x04) //C
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_C");
						pIGenerated[0] = map_complex_value(tmp_obj, "prerotated_current_C");
					}
					else //Not three-phase, but has more than one phase - fail, because we don't do this right
					{
						GL_THROW("ibr_blackbox:%s is not connected to a single-phase or three-phase node - two-phase connections are not supported at this time.", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						The ibr_blackbox only supports single phase (A, B, or C or triplex) or full three-phase connections.  If you try to connect it differntly than this, it will not work.
						*/
					}
				} //End non-three-phase
			}	  //End non-triplex parent

			//*** Common items ****//
			//*** Many of these go to the "true parent", not the "powerflow parent" ****//

			//Map the nominal frequency
			temp_property_pointer = new gld_property("powerflow::nominal_frequency");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("ibr_blackbox:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
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
				// Obtain the Z_base of the system for calculating filter impedance
				//Link to nominal voltage
				temp_property_pointer = new gld_property(parent, "nominal_voltage");

				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
				{
					gl_error("ibr_blackbox:%d %s failed to map the nominal_voltage property", obj->id, (obj->name ? obj->name : "Unnamed"));
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
			} //End VSI common items

			//Map status - true parent
			pMeterStatus = new gld_property(parent, "service_status");

			//Check it
			if (!pMeterStatus->is_valid() || !pMeterStatus->is_enumeration())
			{
				GL_THROW("ibr_blackbox failed to map powerflow status variable");
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
			GL_THROW("ibr_blackbox must have a valid powerflow object as its parent, or no parent at all");
			/*  TROUBLESHOOT
			Check the parent object of the inverter.  The ibr_blackbox is only able to be childed via to powerflow objects.
			Alternatively, you can also choose to have no parent, in which case the ibr_blackbox will be a stand-alone application
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

		gl_warning("ibr_blackbox:%d has no parent meter object defined; using static voltages", obj->id);
		/*  TROUBLESHOOT
		An ibr_blackbox in the system does not have a parent attached.  It is using static values for the voltage.
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

    //Frugrally deep manual allocation (since it lacks a constructor)
    auto model_construct = std::make_unique<fdeep::model>(fdeep::load_model("fdeep_CNN_ge1.json"));
    bb_model.swap(model_construct);
	first_sync_delta_enabled = true;

	///////////////////////////////////////////////////////////////////////////
	// DELTA MODE
	///////////////////////////////////////////////////////////////////////////
	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (!enable_subsecond_models)
		{
			gl_warning("ibr_blackbox:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The ibr_blackbox object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			//Add us to the list
			fxn_return_status = add_gen_delta_obj(obj,false);

			//Check it
			if (fxn_return_status == FAILED)
			{
				GL_THROW("ibr_blackbox:%s - failed to add object to generator deltamode object list", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The ibr_blackbox object encountered an issue while trying to add itself to the generator deltamode object list.  If the error
				persists, please submit an issue via GitHub.
				*/
			}

		}
		//Default else - don't do anything

	}	 //End deltamode inclusive
	else //This particular model isn't enabled
	{
		if (enable_subsecond_models)
		{
			gl_warning("ibr_blackbox:%d %s - Deltamode is enabled for the module, but not this object!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The ibr_blackbox is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this ibr_blackbox may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}		
	}

	//Other initialization variables
	inverter_start_time = gl_globalclock;

	// Initialize parameters
	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_blackbox:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
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

//**** First call of timestep iteration - before players ****//
//**** Usually used to reset accumulators and related variables for the model ****//
TIMESTAMP ibr_blackbox::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//**** Relevant reset code would probably go here ****//

	return t2;
}

//**** Main QSTS timestep function.  Executes before powerflow solves ****//
//**** Typically is any QSTS model updates ****//
TIMESTAMP ibr_blackbox::sync(TIMESTAMP t0, TIMESTAMP t1)
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
		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//**** Initialization items - does any last-minute value checks, as well as registering a powerflow interface function ****//
	//Deltamode check/init items
	if (first_sync_delta_enabled) //Deltamode first pass
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models) //We want deltamode - see if it's populated yet
		{
			if (parent_is_a_meter)
			{
				//Accumulate the starting power
				if (sqrt(Pref*Pref+Qref*Qref) > S_base)
				{
					gl_warning("ibr_blackbox:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
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
			}

			// Assuming these variables holding the latest values
			float Vdc_latest = dc_input_voltage;
			float Va_mag_latest = value_Circuit_V[0].Mag()/node_nominal_voltage;
			float Vb_mag_latest = value_Circuit_V[1].Mag()/node_nominal_voltage;
			float Vc_mag_latest = value_Circuit_V[2].Mag()/node_nominal_voltage;
			float Va_ang_latest = value_Circuit_V[0].Arg()/(2.0*PI);
			float Vb_ang_latest = value_Circuit_V[1].Arg()/(2.0*PI);
			float Vc_ang_latest = value_Circuit_V[2].Arg()/(2.0*PI);

			simulation_data = {
				{{Vdc_latest, Va_mag_latest, Vb_mag_latest, Vc_mag_latest, Va_ang_latest, Vb_ang_latest, Vc_ang_latest}}, // Initialize with zeros
				{{Vdc_latest, Va_mag_latest, Vb_mag_latest, Vc_mag_latest, Va_ang_latest, Vb_ang_latest, Vc_ang_latest}},
				{{Vdc_latest, Va_mag_latest, Vb_mag_latest, Vc_mag_latest, Va_ang_latest, Vb_ang_latest, Vc_ang_latest}},
				{{Vdc_latest, Va_mag_latest, Vb_mag_latest, Vc_mag_latest, Va_ang_latest, Vb_ang_latest, Vc_ang_latest}},
				{{Vdc_latest, Va_mag_latest, Vb_mag_latest, Vc_mag_latest, Va_ang_latest, Vb_ang_latest, Vc_ang_latest}}
			};

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

    //Fdeep add
//    result_output = bb_model->predict({fdeep::tensor(fdeep::tensor_shape(4), fdeep::float_vec({1., 2., 3., 4.}))});

    //added by sunil

	// Create a tensor with shape (5, 7, 1)
	//fdeep::tensor input_tensor(fdeep::tensor_shape(5, 7, 1), fdeep::float_vec(5 * 7 * 1, 0.0));

	// std::vector<std::vector<std::vector<float>>> simulation_data = {
	//     {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}, // Initialize with zeros
	//     {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},
	//     {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},
	//     {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},
	//     {{0.508587, 0.747836, 0.749498, 0.749134, 0.405156, 0.11261, 0.739529}}
	// };




	// Assuming these variables holding the latest values
	float Vdc_latest = dc_input_voltage;
	float Va_mag_latest = value_Circuit_V[0].Mag()/node_nominal_voltage;
	float Vb_mag_latest = value_Circuit_V[1].Mag()/node_nominal_voltage;
	float Vc_mag_latest = value_Circuit_V[2].Mag()/node_nominal_voltage;
	float Va_ang_latest = value_Circuit_V[0].Arg()/(2.0*PI);
	float Vb_ang_latest = value_Circuit_V[1].Arg()/(2.0*PI);
	float Vc_ang_latest = value_Circuit_V[2].Arg()/(2.0*PI);

	// Update simulation_data for rolling updates
	for (int i = 0; i < simulation_data.size() - 1; ++i) {
	    simulation_data[i] = simulation_data[i + 1];
	}

	// Update the latest block with the new values from variables
	simulation_data[simulation_data.size() - 1] = {
	    {Vdc_latest, Va_mag_latest, Vb_mag_latest, Vc_mag_latest, Va_ang_latest, Vb_ang_latest, Vc_ang_latest}
	};


	// Create a vector to hold the input data
	std::vector<float> input_data;

	// Flatten the updated simulated data and store it in input_data
	for (int time_step = 0; time_step < 5; ++time_step) {
	    for (int variable = 0; variable < 7; ++variable) {
	        float value = simulation_data[time_step][0][variable];
	        input_data.push_back(value);
	    }
	}

	// Create an input tensor from the updated simulation data
	fdeep::tensor input_tensor(fdeep::tensor_shape(5, 7, 1), input_data);

	// Use the input tensor for prediction
	result_output = bb_model->predict({input_tensor});

	fdeep::tensor one_tensor = result_output[0];

	std::vector<float> vec = one_tensor.to_vector();

	//Put this into current - and unnormalize
	dc_output_current = vec[0];
	value_IGenerated[0].SetPolar(vec[1]*I_base,vec[4]*2*PI);
	value_IGenerated[1].SetPolar(vec[2]*I_base,vec[5]*2*PI);
	value_IGenerated[2].SetPolar(vec[3]*I_base,vec[6]*2*PI);

	// Print the example
	std::cout << fdeep::show_tensors(result_output) << std::endl;

	//Calculate power based on measured terminal voltage and currents
	if (parent_is_single_phase) // single phase/split-phase implementation
	{
		if (!inverter_first_step)
		{
			//Update output power
			//Update per-unti value
			terminal_current_val_pu[0] = value_IGenerated[0] / I_base;

			//Update power output variables, just so we can see what is going on

			power_val[0] = value_Circuit_V[0] * ~value_IGenerated[0];

			VA_Out = power_val[0];

		}
	}
	else //Three phase variant
	{

		if (!inverter_first_step)
		{
			//Update per-unit value
			terminal_current_val_pu[0] = value_IGenerated[0] / I_base;
			terminal_current_val_pu[1] = value_IGenerated[1] / I_base;
			terminal_current_val_pu[2] = value_IGenerated[2] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~value_IGenerated[0];
			power_val[1] = value_Circuit_V[1] * ~value_IGenerated[1];
			power_val[2] = value_Circuit_V[2] * ~value_IGenerated[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

		}
	}

	//**** QSTS update items would go here ****//

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

/* Postsync is called when the clock needs to advance on the second top-down pass */
//**** Typically used for any "post-powerflow solution" updates, like calculating output power. ****//
TIMESTAMP ibr_blackbox::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER; //By default, we're done forever!

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		//Also pull the current values
		pull_complex_powerflow_values();
	}

	//**** This (and below) is where post-powerflow updates would go (for QSTS) ****//

	//Check to see if valid connection
	if (value_MeterStatus == 1)
	{
		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			//Update output power
			//Update per-unit value
			terminal_current_val_pu[0] = value_IGenerated[0] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~value_IGenerated[0];
			VA_Out = power_val[0];
		}
		else //Three-phase, by default
		{
			//Update output power

			//Update per-unit values
			terminal_current_val_pu[0] = value_IGenerated[0] / I_base;
			terminal_current_val_pu[1] = value_IGenerated[1] / I_base;
			terminal_current_val_pu[2] = value_IGenerated[2] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~value_IGenerated[0];
			power_val[1] = value_Circuit_V[1] * ~value_IGenerated[1];
			power_val[2] = value_Circuit_V[2] * ~value_IGenerated[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];
		}
	}
	else	//Disconnected somehow
	{
		VA_Out = gld::complex(0.0,0.0);
	}

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//**** Preupdate - called when deltamode is starting ****//
//**** Can be used to initalize variables for integrators/etc. ****//
STATUS ibr_blackbox::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	FUNCTIONADDR funadd = nullptr;
	OBJECT *hdr = OBJECTHDR(this);

	// Call to initialization function
	stat_val = init_dynamics();

	if (stat_val != SUCCESS)
	{
		gl_error("ibr_blackbox failed pre_deltaupdate call");
		/*  TROUBLESHOOT
		While attempting to call the pre_deltaupdate portion of the ibr_blackbox code, an error
		was encountered.  Please submit your code and a bug report via the ticketing system.
		*/

		return FAILED;
	}

	//Reset the QSTS criterion flag
	deltamode_exit_iteration_met = false;

	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

//Module-level call
//**** Main "deltamode timestep" call ****//
//**** Where differential equations and timestep-oriented model updates for deltamode would reside ****//
SIMULATIONMODE ibr_blackbox::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltath;

	OBJECT *obj = OBJECTHDR(this);

	SIMULATIONMODE simmode_return_value = SM_EVENT;

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		pull_complex_powerflow_values();
	}

	//Get timestep value
	deltat = (double)dt / (double)DT_SECOND;
	deltath = deltat / 2.0;

	//Update time tracking variables
	prev_timestamp_dbl = gl_globaldeltaclock;

    //Fdeep add
    //result_output = bb_model->predict({fdeep::tensor(fdeep::tensor_shape(4), fdeep::float_vec({1., 2., 3., 4.}))});


    //added by sunil
	// Assuming these variables holding the latest values
	float Vdc_latest = dc_input_voltage;
	float Va_mag_latest = value_Circuit_V[0].Mag()/node_nominal_voltage;
	float Vb_mag_latest = value_Circuit_V[1].Mag()/node_nominal_voltage;
	float Vc_mag_latest = value_Circuit_V[2].Mag()/node_nominal_voltage;
	float Va_ang_latest = value_Circuit_V[0].Arg()/(2.0*PI);
	float Vb_ang_latest = value_Circuit_V[1].Arg()/(2.0*PI);
	float Vc_ang_latest = value_Circuit_V[2].Arg()/(2.0*PI);

	// Update simulation_data for rolling updates
	for (int i = 0; i < simulation_data.size() - 1; ++i) {
	    simulation_data[i] = simulation_data[i + 1];
	}

	// Update the latest block with the new values from variables
	simulation_data[simulation_data.size() - 1] = {
	    {Vdc_latest, Va_mag_latest, Vb_mag_latest, Vc_mag_latest, Va_ang_latest, Vb_ang_latest, Vc_ang_latest}
	};


	// Create a vector to hold the input data
	std::vector<float> input_data;

	// Flatten the updated simulated data and store it in input_data
	for (int time_step = 0; time_step < 5; ++time_step) {
	    for (int variable = 0; variable < 7; ++variable) {
	        float value = simulation_data[time_step][0][variable];
	        input_data.push_back(value);
	    }
	}

	// Create an input tensor from the updated simulation data
	fdeep::tensor input_tensor(fdeep::tensor_shape(5, 7, 1), input_data);

	// Use the input tensor for prediction
	result_output = bb_model->predict({input_tensor});

	fdeep::tensor one_tensor = result_output[0];

	std::vector<float> vec = one_tensor.to_vector();

	//Put this into current - and unnormalize
	dc_output_current = vec[0];
	value_IGenerated[0].SetPolar(vec[1]*I_base,vec[4]*2*PI);
	value_IGenerated[1].SetPolar(vec[2]*I_base,vec[5]*2*PI);
	value_IGenerated[2].SetPolar(vec[3]*I_base,vec[6]*2*PI);

	//Print the example
	std::cout << fdeep::show_tensors(result_output) << std::endl;

	//*** Deltamode/differential equation updates would go here ****//
	// Check pass
	if (iteration_count_val == 0) // Predictor pass
	{
		//filt_test_out_pred = filt_test_obj.getoutput(filt_test_in,deltat,PREDICTOR);
		desired_simulation_mode = SM_DELTA_ITER;
		simmode_return_value = SM_DELTA_ITER;
	}
	else if (iteration_count_val == 1) // Corrector pass
	{
		//filt_test_out_corr = filt_test_obj.getoutput(filt_test_in,deltat,CORRECTOR);
		desired_simulation_mode = SM_DELTA;	//Just keep in deltamode
		simmode_return_value = SM_DELTA;
	}
	else //Additional iterations
	{
		//Just return whatever our "last desired" was
		simmode_return_value = desired_simulation_mode;
	}
	
	
	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}
	
	//**** Return here designates how to proceed ****//
	//**** SM_EVENT = we desire to go back to QSTS mode ****//
	//**** SM_DELTA = we are ready for the next deltamode timestep ****//
	//**** SM_DELTA_ITER = we want to reiterate this timestep, so don't move forward (e.g., predictor step) ****//
	//**** SM_ERROR = an error occurred and we should terminate ****//

	return simmode_return_value;
}

//Initializes dynamic equations for first entry
STATUS ibr_blackbox::init_dynamics()
{
	OBJECT *obj = OBJECTHDR(this);

	//Pull the powerflow values
	if (parent_is_a_meter)
	{
		pull_complex_powerflow_values();
	}

	//**** This would be where any initalizations would occur, especially to initalize differential equations from QSTS values ****//

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	return SUCCESS;
}

//Module-level post update call
//**** Called right before returning to QSTS mode.  Useful to initialize QSTS model to any deltamode changes (if different model)****//
//**** Not used in all models, so commented out here ****//
// STATUS ibr_blackbox::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
// {

// 	//Should have a parent, but be paranoid
// 	if (parent_is_a_meter)
// 	{
// 		push_complex_powerflow_values();
// 	}

// 	return SUCCESS; //Always succeeds right now
// }

//Map Complex value
gld_property *ibr_blackbox::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("ibr_blackbox:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *ibr_blackbox::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("ibr_blackbox:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull all the complex properties from powerflow into local variables
void ibr_blackbox::pull_complex_powerflow_values(void)
{
	//Pull in the various values from powerflow - straight reads
	//Pull status
	value_MeterStatus = pMeterStatus->get_enumeration();

	//Update IGenerated, in case the powerflow is overriding it
	if (parent_is_single_phase)
	{
		//Just pull "zero" values
		value_Circuit_V[0] = pCircuit_V[0]->get_complex();
	}
	else
	{
		//Pull all voltages
		value_Circuit_V[0] = pCircuit_V[0]->get_complex();
		value_Circuit_V[1] = pCircuit_V[1]->get_complex();
		value_Circuit_V[2] = pCircuit_V[2]->get_complex();
	}
}

//Function to push up all changes of complex properties to powerflow from local variables
void ibr_blackbox::push_complex_powerflow_values(bool update_voltage)
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
				//**** pre-rotated Current value ***/
				//Pull current value again, just in case
				temp_complex_val = pIGenerated[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_IGenerated[indexval] - prev_value_IGenerated[indexval];

				//Push it back up
				pIGenerated[indexval]->setp<gld::complex>(temp_complex_val,*test_rlock);

				//Update tracker
				prev_value_IGenerated[indexval] = value_IGenerated[indexval];
			}//End phase loop
		}//End not voltage push
	}//End three-phase
	else //Assumes must be single-phased - else how did it get here?
	{
		//Check for voltage push - in case that's ever needed here
		if (update_voltage)
		{
			//Should just be zero
			//*** push voltage value -- not an accumulator, just force ***/
			pCircuit_V[0]->setp<gld::complex>(value_Circuit_V[0],*test_rlock);
		}
		else
		{
			//Pull the relevant values -- all single pulls
			//**** pre-rotated Current value ***/
			//Pull current value again, just in case
			temp_complex_val = pIGenerated[0]->get_complex();

			//Add the difference
			temp_complex_val += value_IGenerated[0] - prev_value_IGenerated[0];

			//Push it back up
			pIGenerated[0]->setp<gld::complex>(temp_complex_val,*test_rlock);

			//Update tracker
			prev_value_IGenerated[0] = value_IGenerated[0];
		}//End not voltage update
	}//End single-phase
}

// Function to update current injection IGenerated for VSI
//**** Called by powerflow at each iteration of NR to update for any voltage solution changes ****//
//**** Mostly used to adjust current angle to match any voltage angle change as a result of the new powerflow ****//
STATUS ibr_blackbox::updateCurrInjection(int64 iteration_count,bool *converged_failure)
{
	double temp_time;
	OBJECT *obj = OBJECTHDR(this);
	gld::complex temp_V1, temp_V2;
	bool bus_is_a_swing, bus_is_swing_pq_entry;
	STATUS temp_status_val;
	gld_property *temp_property_pointer;
	bool running_in_delta;
	bool limit_hit;
	double freq_diff_angle_val, tdiff;

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
		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//See if we're in QSTS and a grid-forming inverter - update if we are
	if (!running_in_delta)
	{
		//Update trackers
		prev_timestamp_dbl = temp_time;
		last_QSTS_GF_Update = temp_time;
	}

	//External call to internal variables -- used by powerflow to iterate the VSI implementation, basically

	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_blackbox:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
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
					GL_THROW("ibr_blackbox:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
				GL_THROW("ibr_blackbox:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
		}
		else //Three-phase
		{

		}//End three-phase
	}
	//Default else - things are handled elsewhere

	//Push the changes up
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}

	//Always a success, but power flow solver may not like it if VA_OUT exceeded the rating and thus changed
	return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
//**** These functions are the GridLAB-D interface points it expects from every object ****//
//**** Without them, GridLAB-D doesn't know what to do with your object ****//

EXPORT int create_ibr_blackbox(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(ibr_blackbox::oclass);
		if (*obj != nullptr)
		{
			ibr_blackbox *my = OBJECTDATA(*obj, ibr_blackbox);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(ibr_blackbox);
}

EXPORT int init_ibr_blackbox(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != nullptr)
			return OBJECTDATA(obj, ibr_blackbox)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(ibr_blackbox);
}

EXPORT TIMESTAMP sync_ibr_blackbox(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	ibr_blackbox *my = OBJECTDATA(obj, ibr_blackbox);
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
	SYNC_CATCHALL(ibr_blackbox);
	return t2;
}

EXPORT int isa_ibr_blackbox(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,ibr_blackbox)->isa(classname);
}

EXPORT STATUS preupdate_ibr_blackbox(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	ibr_blackbox *my = OBJECTDATA(obj, ibr_blackbox);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0, delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_ibr_blackbox(obj=%d;%s): %s", obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_ibr_blackbox(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	ibr_blackbox *my = OBJECTDATA(obj, ibr_blackbox);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_ibr_blackbox(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

// EXPORT STATUS postupdate_ibr_blackbox(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
// {
// 	ibr_blackbox *my = OBJECTDATA(obj, ibr_blackbox);
// 	STATUS status = FAILED;
// 	try
// 	{
// 		status = my->post_deltaupdate(useful_value, mode_pass);
// 		return status;
// 	}
// 	catch (char *msg)
// 	{
// 		gl_error("postupdate_ibr_blackbox(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
// 		return status;
// 	}
// }

//// Define export function that update the VSI current injection IGenerated to the grid
//Updates mid-solve on powerflow calls - allows values to update for moving voltages
EXPORT STATUS ibr_blackbox_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure)
{
	STATUS temp_status;

	//Map the node
	ibr_blackbox *my = OBJECTDATA(obj, ibr_blackbox);

	//Call the function, where we can update the IGenerated injection
	temp_status = my->updateCurrInjection(iteration_count,converged_failure);

	//Return what the sub function said we were
	return temp_status;
}
