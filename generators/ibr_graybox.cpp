// Skeleton/empty object with deltamode capabilities
// Comments in //**** ****// are brief descriptions of what a function does or where it may go.
#include "ibr_graybox.h"

CLASS *ibr_graybox::oclass = nullptr;
ibr_graybox *ibr_graybox::defaults = nullptr;

static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
ibr_graybox::ibr_graybox(MODULE *module)
{
	if (oclass == nullptr)
	{
		oclass = gl_register_class(module, "ibr_graybox", sizeof(ibr_graybox), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == nullptr)
			throw "unable to register class ibr_graybox";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			//**** This is where the mapping between the C/C++ programming variables and the "GLM-accessible" variables is done ****//
			//**** Anything you want to set in the GLM or have a player/recorder/HELICS manipulate need to be published here. ****//
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
			PT_double, "P_out_pu", PADDR(P_out_pu), PT_DESCRIPTION, "active power",
			PT_double, "P_out_pu_Filtered", PADDR(P_out_pu_Filtered), PT_DESCRIPTION, "active power filter output",
			PT_double, "Q_out_pu", PADDR(Q_out_pu), PT_DESCRIPTION, "reactive power",
			PT_double, "Q_out_pu_Filtered", PADDR(Q_out_pu_Filtered), PT_DESCRIPTION, "reactive power filter output",

			//Input
			PT_double, "rated_power[VA]", PADDR(S_base), PT_DESCRIPTION, " The rated power of the inverter",
			// Grid-Following Controller Parameters
			PT_double, "Pref[W]", PADDR(Pref), PT_DESCRIPTION, "DELTAMODE: The real power reference.",
			PT_double, "Qref[VAr]", PADDR(Qref), PT_DESCRIPTION, "DELTAMODE: The reactive power reference.",

			// Inverter filter parameters
			PT_double, "Xfilter[pu]", PADDR(Xfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter.",
			PT_double, "Rfilter[pu]", PADDR(Rfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter.",

			// ibr_graybox_NOTE: Any variables that need to be published should go here.

			nullptr) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		defaults = this;

		memset(this, 0, sizeof(ibr_graybox));

		if (gl_publish_function(oclass, "preupdate_gen_object", (FUNCTIONADDR)preupdate_ibr_graybox) == nullptr)
			GL_THROW("Unable to publish ibr_graybox deltamode function");
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_ibr_graybox) == nullptr)
			GL_THROW("Unable to publish ibr_graybox deltamode function");
		// if (gl_publish_function(oclass, "postupdate_gen_object", (FUNCTIONADDR)postupdate_ibr_graybox) == nullptr)
		// 	GL_THROW("Unable to publish ibr_graybox deltamode function");
		if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)ibr_graybox_NR_current_injection_update) == nullptr)
			GL_THROW("Unable to publish ibr_graybox current injection update function");
	}
}

//Isa function for identification
int ibr_graybox::isa(char *classname)
{
	return strcmp(classname,"ibr_graybox")==0;
}

/* Object creation is called once for each object that is created by the core */
//**** This sets up object defaults.  This is done before the GLM is read, so GLM properties will override these ****//
int ibr_graybox::create(void)
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
	pbus_full_Y_mat = nullptr;
	pGenerated = nullptr;

	//Zero the accumulators
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0, 0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = gld::complex(0.0, 0.0);
	prev_value_IGenerated[0] = prev_value_IGenerated[1] = prev_value_IGenerated[2] = gld::complex(0.0, 0.0);
	value_MeterStatus = 1; //Connected, by default

	// Inverter filter
	Xfilter = 0.15; //per unit
	Rfilter = 0.01; // per unit

	f_nominal = 60;

	//Set up the deltamode "next state" tracking variable
	desired_simulation_mode = SM_EVENT;

	//Tracking variable
	last_QSTS_GF_Update = TS_NEVER_DBL;

	node_nominal_voltage = 120.0;		//Just pick a value

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
//**** This is after the GLM read, so it is where parameters can be checked for validity (e.g., make sure rated power isn't negative)  ****//
//**** This is also where a lot of "one-time actions", like mapping variables to parent or other objects, occurs.  ****//
int ibr_graybox::init(OBJECT *parent)
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
	bool childed_connection = false;

	//Deferred initialization code
	if (parent != nullptr)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("ibr_graybox::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	// Data sanity check
	if (S_base <= 0)
	{
		S_base = 1000;
		gl_warning("ibr_graybox:%d - %s - The rated power of this inverter must be positive - set to 1 kVA.",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		The ibr_graybox has a rated power that is negative.  It defaulted to a 1 kVA inverter.  If this is not desired, set the property properly in the GLM.
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
						GL_THROW("ibr_graybox:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
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
							GL_THROW("ibr_graybox:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", parent->name ? parent->name : "unnamed");
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

			//Pull the bus type
			temp_property_pointer = new gld_property(tmp_obj, "bustype");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("ibr_graybox:%s failed to map bustype variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
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

					pIGenerated[1] = nullptr;
					pIGenerated[2] = nullptr;

					if ((phases & 0x07) == 0x01) //A
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_A");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_A");
					}
					else if ((phases & 0x07) == 0x02) //B
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_B");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_B");
					}
					else if ((phases & 0x07) == 0x04) //C
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_C");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_C");
					}
					else //Not three-phase, but has more than one phase - fail, because we don't do this right
					{
						GL_THROW("ibr_graybox:%s is not connected to a single-phase or three-phase node - two-phase connections are not supported at this time.", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						The ibr_graybox only supports single phase (A, B, or C or triplex) or full three-phase connections.  If you try to connect it differntly than this, it will not work.
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
				GL_THROW("ibr_graybox:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
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
				temp_property_pointer = new gld_property(tmp_obj, "Norton_dynamic");
				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
				{
					GL_THROW("inverter_dyn:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
					//Defined elsewhere
				}
				//Flag it to true
				temp_bool_value = true;
				temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);
				//Remove it
				delete temp_property_pointer;

				// Obtain the Z_base of the system for calculating filter impedance
				//Link to nominal voltage
				temp_property_pointer = new gld_property(parent, "nominal_voltage");

				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
				{
					gl_error("ibr_graybox:%d %s failed to map the nominal_voltage property", obj->id, (obj->name ? obj->name : "Unnamed"));
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

				//Provide the Norton-equivalent admittance up to powerflow
				//**** generator_admittance is the Norton current source equivalent admittance ****//
				//**** This will need to be updated for the object (junk values put here for now) ****//

				//Convert filter to admittance
				//filter_admittance = gld::complex(1.0, 0.0) / (gld::complex(Rfilter, Xfilter) * Z_base);

				//Placeholder diagonals
				generator_admittance[0][0] = generator_admittance[1][1] = generator_admittance[2][2] = filter_admittance;
				generator_admittance[0][1] = generator_admittance[1][0] = generator_admittance[2][0] = complex(0.0,0.0);
				generator_admittance[0][2] = generator_admittance[1][2] = generator_admittance[2][1] = complex(0.0,0.0);

				//Map the full_Y parameter to inject the admittance portion into it
				pbus_full_Y_mat = new gld_property(tmp_obj, "deltamode_full_Y_matrix");

				//Check it
				if (!pbus_full_Y_mat->is_valid() || !pbus_full_Y_mat->is_complex_array())
				{
					GL_THROW("ibr_graybox:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
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
						GL_THROW("ibr_graybox:%s exposed Norton-equivalent matrix is the wrong size!", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						While mapping to an admittance matrix on the parent node device, it was found it is the wrong size.
						Please try again.  If the error persists, please submit your code and model via the issue tracking system.
						*/
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
						GL_THROW("ibr_graybox:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
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
							GL_THROW("ibr_graybox:%s exposed Norton-equivalent matrix is the wrong size!",obj->name?obj->name:"unnamed");
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

				//Map the power variable (intialization related)
				pGenerated = map_complex_value(tmp_obj, "deltamode_PGenTotal");
			} //End VSI common items

			//Map status - true parent
			pMeterStatus = new gld_property(parent, "service_status");

			//Check it
			if (!pMeterStatus->is_valid() || !pMeterStatus->is_enumeration())
			{
				GL_THROW("ibr_graybox failed to map powerflow status variable");
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
			GL_THROW("ibr_graybox must have a valid powerflow object as its parent, or no parent at all");
			/*  TROUBLESHOOT
			Check the parent object of the inverter.  The ibr_graybox is only able to be childed via to powerflow objects.
			Alternatively, you can also choose to have no parent, in which case the ibr_graybox will be a stand-alone application
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

		gl_warning("ibr_graybox:%d has no parent meter object defined; using static voltages", obj->id);
		/*  TROUBLESHOOT
		An ibr_graybox in the system does not have a parent attached.  It is using static values for the voltage.
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
			gl_warning("ibr_graybox:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The ibr_graybox object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
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
				GL_THROW("ibr_graybox:%s - failed to add object to generator deltamode object list", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The ibr_graybox object encountered an issue while trying to add itself to the generator deltamode object list.  If the error
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
			gl_warning("ibr_graybox:%d %s - Deltamode is enabled for the module, but not this object!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The ibr_graybox is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this ibr_graybox may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}		
	}

	//Other initialization variables
	inverter_start_time = gl_globalclock;

	// Initialize parameters
	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_graybox:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
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
TIMESTAMP ibr_graybox::presync(TIMESTAMP t0, TIMESTAMP t1)
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
TIMESTAMP ibr_graybox::sync(TIMESTAMP t0, TIMESTAMP t1)
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
					gl_warning("ibr_graybox:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
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
					GL_THROW("ibr_graybox:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
					GL_THROW("ibr_graybox:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
TIMESTAMP ibr_graybox::postsync(TIMESTAMP t0, TIMESTAMP t1)
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

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//**** Preupdate - called when deltamode is starting ****//
//**** Can be used to initalize variables for integrators/etc. ****//
STATUS ibr_graybox::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	FUNCTIONADDR funadd = nullptr;
	OBJECT *hdr = OBJECTHDR(this);

	// Call to initialization function
	stat_val = init_dynamics();

	if (stat_val != SUCCESS)
	{
		gl_error("ibr_graybox failed pre_deltaupdate call");
		/*  TROUBLESHOOT
		While attempting to call the pre_deltaupdate portion of the ibr_graybox code, an error
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
SIMULATIONMODE ibr_graybox::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
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

	//*** Deltamode/differential equation updates would go here ****//

	//* Physical model
	double dV_ref;
	double P_set = 0.4;
	double Q_set = 0.2;
	double V0 = 1.0;
	double m_p = 0.05;
	double n_q = 0.05;
	double delta_w;
	double delta_Vs;
	double Angle;
	double Vs;
	gld::complex generator_impedance;
	generator_impedance = gld::complex(0.03, 0.15)*Z_base;
	terminal_current_val[0] = (value_IGenerated_Nortan[0] - value_Circuit_V[0]/generator_impedance);
	terminal_current_val[1] = (value_IGenerated_Nortan[1] - value_Circuit_V[1]/generator_impedance);
	terminal_current_val[2] = (value_IGenerated_Nortan[2] - value_Circuit_V[2]/generator_impedance);

	//Update per-unit value
	terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
	terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
	terminal_current_val_pu[2] = terminal_current_val[2] / I_base;
	//Update power output variables
	power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
	power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
	power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];
	VA_Out = power_val[0] + power_val[1] + power_val[2];
	P_out_pu = VA_Out.Re()/S_base;
	Q_out_pu = VA_Out.Im()/S_base;

	// Check pass
	if (iteration_count_val == 0) // Predictor pass
	{
		P_out_pu_Filtered = Pmeas_blk.getoutput(P_out_pu,deltat,PREDICTOR);
		Q_out_pu_Filtered = Qmeas_blk.getoutput(Q_out_pu,deltat,PREDICTOR);
		// droop control
		delta_w = - m_p*(P_out_pu_Filtered - P_set);
		dV_ref = - n_q*(Q_out_pu_Filtered - Q_set);
		// Integral
		Angle = Angle_blk.getoutput(delta_w,deltat,PREDICTOR)*2*M_PI*60;
		// low-pass filter
		delta_Vs = Vmeas_blk.getoutput(dV_ref,deltat,PREDICTOR);
		//filt_test_out_pred = filt_test_obj.getoutput(filt_test_in,deltat,PREDICTOR);
		desired_simulation_mode = SM_DELTA_ITER;
		simmode_return_value = SM_DELTA_ITER;
	}
	else if (iteration_count_val == 1) // Corrector pass
	{
		P_out_pu_Filtered = Pmeas_blk.getoutput(P_out_pu,deltat,CORRECTOR);
		Q_out_pu_Filtered = Qmeas_blk.getoutput(Q_out_pu,deltat,CORRECTOR);
		// droop control
		delta_w = - m_p*(P_out_pu_Filtered - P_set);
		dV_ref = - n_q*(Q_out_pu_Filtered - Q_set);
		// Integral
		Angle = Angle_blk.getoutput(delta_w,deltat,CORRECTOR)*2*M_PI*60;
		// low-pass filter
		delta_Vs = Vmeas_blk.getoutput(dV_ref,deltat,CORRECTOR);
		//filt_test_out_corr = filt_test_obj.getoutput(filt_test_in,deltat,CORRECTOR);
		desired_simulation_mode = SM_DELTA;	//Just keep in deltamode
		simmode_return_value = SM_DELTA;
	}
	else //Additional iterations
	{
		//Just return whatever our "last desired" was
		simmode_return_value = desired_simulation_mode;
	}
	
	Vs = delta_Vs + V0;
	value_IGenerated_Nortan[0] = gld::complex(Vs*V_base*cos(Angle), Vs*V_base*sin(Angle))/generator_impedance;
	value_IGenerated_Nortan[1] = gld::complex(Vs*V_base*cos(Angle-2*M_PI/3), Vs*V_base*sin(Angle-2*M_PI/3))/generator_impedance;
	value_IGenerated_Nortan[2] = gld::complex(Vs*V_base*cos(Angle+2*M_PI/3), Vs*V_base*sin(Angle+2*M_PI/3))/generator_impedance;
	physical_output[0] = (value_IGenerated_Nortan[0] - value_Circuit_V[0]/generator_impedance);
	physical_output[1] = (value_IGenerated_Nortan[1] - value_Circuit_V[1]/generator_impedance);
	physical_output[2] = (value_IGenerated_Nortan[2] - value_Circuit_V[2]/generator_impedance);

	//NN data for case 1 - Small-Load change
	double u[4][3];
	double w1[1][4] = {{-0.000265151635782668, 0.000256068102657914, -0.0284316025094079, 0.0814581502249045}};
	double w2[2][1] = {{-3.79792733597627},{10.9740740072813}};;
	double b1[1][1] = {0.0116373550214972};
	double b2[2][1] = {{0.0140448114136540},{-0.137911469055814}};
	double z11[1][3] = {{0,0,0}};
	double z1[1][3];
	double z21[2][3] = {{0,0,0},{0,0,0}};
	double z2[2][3];
	double yout[2][3];
	double v_A_mag_max = 0.932640112448389;
	double v_A_arg_max = -0.120953657920666;
	double physical_output_i_A_mag_max = 0.613433462017583;
	double physical_output_i_A_arg_max = -0.754993319033009;
	double v_A_mag_min = 0.923668295904476;
	double v_A_arg_min = -0.143448237773265;
	double physical_output_i_A_mag_min = 0.489704248188363;
	double physical_output_i_A_arg_min = -0.866532706968217;
	double y_i_A_mag_max = 0.613433462017583;
	double y_i_A_arg_max = -0.754993319033009;
	double y_i_A_mag_min = 0.489704248188363;
	double y_i_A_arg_min = -0.866532706968217;
	// End of NN data of case 1

	/*
	// NN data of case 2 - Large Load Change
	double u[4][3];
	double w1[1][4] = {{-1.60234761435779e-05, -9.80348818674849e-05, 0.109866824026380, -0.111421282912237}};
	double w2[2][1] = {{4.49363521276080},{-4.55623522118514}};;
	double b1[1][1] = {0.0758360527274168};
	double b2[2][1] = {{-0.183440119382269},{0.499633703829218}};
	double z11[1][3] = {{0,0,0}};
	double z1[1][3];
	double z21[2][3] = {{0,0,0},{0,0,0}};
	double z2[2][3];
	double yout[2][3];
	double v_A_mag_max = 0.923750383279800;
	double v_A_arg_max = -0.136226632308439;
	double physical_output_i_A_mag_max = 1.09233592069872;
	double physical_output_i_A_arg_max = -0.788842380523714;
	double v_A_mag_min = 0.861859798415177;
	double v_A_arg_min = -0.292441151071582;
	double physical_output_i_A_mag_min = 0.421414701733923;
	double physical_output_i_A_arg_min = -1.83193660428916;
	double y_i_A_mag_max = 1.09233592069872;
	double y_i_A_arg_max = -0.788842380523714;
	double y_i_A_mag_min = 0.421414701733923;
	double y_i_A_arg_min = -1.83193660428916;
	// End of data of case 2
	*/

	double physical_output_i_A_mag;
	double physical_output_i_A_arg;
	int i,j,k;
	physical_output_i_A_mag = physical_output[0].Mag()/I_base;
	physical_output_i_A_arg = physical_output[0].Arg();
	//  Normalize
	//  phase 1
	u[0][0] = 2 * (value_Circuit_V[0].Mag()/V_base-v_A_mag_min) / (v_A_mag_max - v_A_mag_min) - 1;
	u[1][0] = 2 * (value_Circuit_V[0].Arg() - v_A_arg_min) / (v_A_arg_max-v_A_arg_min) - 1;
	u[2][0] = 2 * (physical_output_i_A_mag - physical_output_i_A_mag_min) / (physical_output_i_A_mag_max - physical_output_i_A_mag_min) - 1;
	u[3][0] = 2 * (physical_output_i_A_arg - physical_output_i_A_arg_min) / (physical_output_i_A_arg_max-physical_output_i_A_arg_min) - 1;
	//  phase2
	u[0][1] = 2 * (value_Circuit_V[1].Mag()/V_base- v_A_mag_min) / (v_A_mag_max - v_A_mag_min) - 1;
	u[1][1] = 2 * (value_Circuit_V[0].Arg()-2*M_PI/3 -(v_A_arg_min-2*M_PI/3)) / (v_A_arg_max-v_A_arg_min) - 1;
	u[2][1] = 2 * (physical_output_i_A_mag - physical_output_i_A_mag_min) / (physical_output_i_A_mag_max -  physical_output_i_A_mag_min) - 1;
	u[3][1] = 2 * (physical_output_i_A_arg - 2*M_PI/3-(physical_output_i_A_arg_min-2*M_PI/3)) / (physical_output_i_A_arg_max-physical_output_i_A_arg_min) - 1;
	// Phase 3
	u[0][2] = 2 * (value_Circuit_V[2].Mag()/V_base- v_A_mag_min) / (v_A_mag_max - v_A_mag_min) - 1;
	u[1][2] = 2 * (value_Circuit_V[0].Arg()+2*M_PI/3 -(v_A_arg_min+2*M_PI/3)) / (v_A_arg_max-v_A_arg_min) - 1;
	u[2][2] = 2 * (physical_output_i_A_mag - physical_output_i_A_mag_min) / (physical_output_i_A_mag_max -  physical_output_i_A_mag_min) - 1;
	u[3][2] = 2 * (physical_output_i_A_arg + 2*M_PI/3-(physical_output_i_A_arg_min+2*M_PI/3)) / (physical_output_i_A_arg_max-physical_output_i_A_arg_min) - 1;
	// From input layer to hidden layer 1
	for (i = 0; i < 1; i++) {
			for (j = 0; j < 3; j++) {
					for (k = 0; k < 4; k++) {
							z11[i][j] += w1[i][k] * u[k][j];
					}
			}
	}
	for (i = 0; i < 1; i++) {
			for (j = 0; j < 3; j++) {
					z1[i][j]=z11[i][j]+b1[i][0];
			}
	}
	for (i = 0; i < 1; i++) {
			for (j = 0; j < 3; j++) {
					z1[i][j]= tanh(z1[i][j]);
			}
	}
	// From hidden layer 1 to output layer
	for (i = 0; i < 2; i++) {
			for (j = 0; j < 3; j++) {
					for (k = 0; k < 1; k++) {
							z21[i][j] += w2[i][k] * z1[k][j];
					}
			}
	}
	 for (i = 0; i < 2; i++) {
			 for (j = 0; j < 3; j++) {
					 z2[i][j]=z21[i][j]+b2[i][0];
			 }
	 }
	// Reverse normalization
	// Phase 1
	yout[0][0] = (z2[0][0]+1)*(y_i_A_mag_max-y_i_A_mag_min)/2+y_i_A_mag_min;
	yout[1][0] = (z2[1][0]+1)*(y_i_A_arg_max-y_i_A_arg_min)/2+y_i_A_arg_min;
	// Phase 2
	yout[0][1] = (z2[0][1]+1)*(y_i_A_mag_max-y_i_A_mag_min)/2+y_i_A_mag_min;
	yout[1][1] = (z2[1][1]+1)*(y_i_A_arg_max-y_i_A_arg_min)/2+y_i_A_arg_min-2*M_PI/3;
	// Phase 3
	yout[0][2] = (z2[0][2]+1)*(y_i_A_mag_max-y_i_A_mag_min)/2+y_i_A_mag_min;
	yout[1][2] = (z2[1][2]+1)*(y_i_A_arg_max-y_i_A_arg_min)/2+y_i_A_arg_min+2*M_PI/3;
	value_IGenerated[0].SetPolar(yout[0][0]*I_base,yout[1][0]);
	value_IGenerated[1].SetPolar(yout[0][1]*I_base,yout[1][1]);
	value_IGenerated[2].SetPolar(yout[0][2]*I_base,yout[1][2]);
	prev_value_IGenerated[0] = value_IGenerated[0];
	prev_value_IGenerated[1] = value_IGenerated[1];
	prev_value_IGenerated[2] = value_IGenerated[2];
	terminal_current_val[0] = value_IGenerated[0];
	terminal_current_val[1] = value_IGenerated[1];
	terminal_current_val[2] = value_IGenerated[2];
	terminal_current_val_pu[0] = terminal_current_val[0]/I_base;
	terminal_current_val_pu[1] = terminal_current_val[1]/I_base;
	terminal_current_val_pu[2] = terminal_current_val[2]/I_base;

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
STATUS ibr_graybox::init_dynamics()
{
	OBJECT *obj = OBJECTHDR(this);

	//Pull the powerflow values
	if (parent_is_a_meter)
	{
		pull_complex_powerflow_values();
	}

	//**** This would be where any initalizations would occur, especially to initalize differential equations from QSTS values ****//

	// Initialize the Angle integrator
	Angle_blk.setparams(1.0);
	Angle_blk.init_given_y(0);
	// Initialize V, P, Q measurement filters
	Vmeas_blk.setparams(1,0.01);
	Vmeas_blk.init_given_y(0.0);
	Pmeas_blk.setparams(1,0.01);
	Pmeas_blk.init_given_y(0.0);
	Qmeas_blk.setparams(1,0.01);
	Qmeas_blk.init_given_y(0.0);

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	return SUCCESS;
}

//Module-level post update call
//**** Called right before returning to QSTS mode.  Useful to initialize QSTS model to any deltamode changes (if different model)****//
//**** Not used in all models, so commented out here ****//
// STATUS ibr_graybox::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
// {

// 	//Should have a parent, but be paranoid
// 	if (parent_is_a_meter)
// 	{
// 		push_complex_powerflow_values();
// 	}

// 	return SUCCESS; //Always succeeds right now
// }

//Map Complex value
gld_property *ibr_graybox::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("ibr_graybox:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *ibr_graybox::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("ibr_graybox:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull all the complex properties from powerflow into local variables
void ibr_graybox::pull_complex_powerflow_values(void)
{
	//Pull in the various values from powerflow - straight reads
	//Pull status
	value_MeterStatus = pMeterStatus->get_enumeration();

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

//Function to push up all changes of complex properties to powerflow from local variables
void ibr_graybox::push_complex_powerflow_values(bool update_voltage)
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
			//*** push voltage value -- not an accumulator, just force ***/
			pCircuit_V[0]->setp<gld::complex>(value_Circuit_V[0],*test_rlock);
		}
		else
		{
			//Pull the relevant values -- all single pulls

			//*** IGenerated ***/
			//********* TODO - Does this need to be deltamode-flagged? *************//
			//Direct write, not an accumulator
			pIGenerated[0]->setp<gld::complex>(value_IGenerated[0], *test_rlock);
		}//End not voltage update
	}//End single-phase
}

// Function to update current injection IGenerated for VSI
//**** Called by powerflow at each iteration of NR to update for any voltage solution changes ****//
//**** Mostly used to adjust current angle to match any voltage angle change as a result of the new powerflow ****//
STATUS ibr_graybox::updateCurrInjection(int64 iteration_count,bool *converged_failure)
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
		gl_warning("ibr_graybox:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
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
					GL_THROW("ibr_graybox:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
				GL_THROW("ibr_graybox:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
	}//End connected/working

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

EXPORT int create_ibr_graybox(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(ibr_graybox::oclass);
		if (*obj != nullptr)
		{
			ibr_graybox *my = OBJECTDATA(*obj, ibr_graybox);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(ibr_graybox);
}

EXPORT int init_ibr_graybox(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != nullptr)
			return OBJECTDATA(obj, ibr_graybox)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(ibr_graybox);
}

EXPORT TIMESTAMP sync_ibr_graybox(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	ibr_graybox *my = OBJECTDATA(obj, ibr_graybox);
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
	SYNC_CATCHALL(ibr_graybox);
	return t2;
}

EXPORT int isa_ibr_graybox(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,ibr_graybox)->isa(classname);
}

EXPORT STATUS preupdate_ibr_graybox(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	ibr_graybox *my = OBJECTDATA(obj, ibr_graybox);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0, delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_ibr_graybox(obj=%d;%s): %s", obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_ibr_graybox(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	ibr_graybox *my = OBJECTDATA(obj, ibr_graybox);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_ibr_graybox(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

// EXPORT STATUS postupdate_ibr_graybox(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
// {
// 	ibr_graybox *my = OBJECTDATA(obj, ibr_graybox);
// 	STATUS status = FAILED;
// 	try
// 	{
// 		status = my->post_deltaupdate(useful_value, mode_pass);
// 		return status;
// 	}
// 	catch (char *msg)
// 	{
// 		gl_error("postupdate_ibr_graybox(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
// 		return status;
// 	}
// }

//// Define export function that update the VSI current injection IGenerated to the grid
//Updates mid-solve on powerflow calls - allows values to update for moving voltages
EXPORT STATUS ibr_graybox_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure)
{
	STATUS temp_status;

	//Map the node
	ibr_graybox *my = OBJECTDATA(obj, ibr_graybox);

	//Call the function, where we can update the IGenerated injection
	temp_status = my->updateCurrInjection(iteration_count,converged_failure);

	//Return what the sub function said we were
	return temp_status;
}
