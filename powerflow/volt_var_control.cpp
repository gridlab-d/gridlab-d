/** $Id: volt_var_control.cpp 2010-02-26 15:00:00Z d3x593 $
	
	Implemented integrated Volt-VAr control scheme detailed in:

	Baran, M.E. and M.Y Hsu, "Volt/Var Control at Distribution Substations," in IEEE Transactions on Power Systems,
	vol. 14, no. 1, February 1999, pp. 312-318.

	Borozan, V., M.E. Baran, and D. Novosel, "Integrated Volt/Var Control in Distribution Systems," in Proceedings
	of the 2001 Power Engineering Society Winter Meeting, vol. 3, Jan. 28-Feb. 01, 2001, Columbus, OH, USA, pp. 1485-1490.

	Copyright (C) 2010 Battelle Memorial Institute
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "volt_var_control.h"

//////////////////////////////////////////////////////////////////////////
// volt_var_control CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* volt_var_control::oclass = NULL;
CLASS* volt_var_control::pclass = NULL;

volt_var_control::volt_var_control(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"volt_var_control",sizeof(volt_var_control),PC_PRETOPDOWN|PC_POSTTOPDOWN);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		if(gl_publish_variable(oclass,
			PT_enumeration, "control_method", PADDR(control_method),PT_DESCRIPTION,"IVVC activated or in standby",
				PT_KEYWORD, "ACTIVE", ACTIVE,
				PT_KEYWORD, "STANDBY", STANDBY,
			PT_double, "capacitor_delay[s]", PADDR(cap_time_delay),PT_DESCRIPTION,"Capacitor default time delay - overridden by local defintions",
			PT_double, "regulator_delay[s]", PADDR(reg_time_delay),PT_DESCRIPTION,"Regulator default time delay - not overriden by local definitions",
			PT_double, "desired_pf", PADDR(desired_pf),PT_DESCRIPTION,"Desired power-factor magnitude at the substation transformer or regulator",
			PT_double, "d_max", PADDR(d_max),PT_DESCRIPTION,"Scaling constant for capacitor switching on - typically 0.3 - 0.6",
			PT_double, "d_min", PADDR(d_min),PT_DESCRIPTION,"Scaling constant for capacitor switching off - typically 0.1 - 0.4",
			PT_object, "substation_link",PADDR(substation_lnk_obj),PT_DESCRIPTION,"Substation link, transformer, or regulator to measure power factor",
			PT_set, "pf_phase", PADDR(pf_phase),PT_DESCRIPTION,"Phase to include in power factor monitoring",
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
			PT_object, "feeder_regulator",PADDR(feeder_regulator_obj),PT_DESCRIPTION,"Voltage regulator for the system",
			PT_char1024, "capacitor_list",PADDR(capacitor_list),PT_DESCRIPTION,"List of controllable capacitors on the system separated by commas",
			PT_char1024, "voltage_measurements",PADDR(measurement_list),PT_DESCRIPTION,"List of voltage measurement devices, separated by commas",
			PT_double, "minimum_voltage[V]",PADDR(minimum_voltage),PT_DESCRIPTION,"Minimum voltage allowed for feeder",
			PT_double, "maximum_voltage[V]",PADDR(maximum_voltage),PT_DESCRIPTION,"Maximum voltage allowed for feeder",
			PT_double, "desired_voltage[V]",PADDR(desired_voltage),PT_DESCRIPTION,"Desired operating voltage for the feeder",
			PT_double, "max_vdrop[V]",PADDR(max_vdrop),PT_DESCRIPTION,"Maximum voltage drop between feeder and end measurement",
			PT_double, "high_load_deadband[V]",PADDR(vbw_high),PT_DESCRIPTION,"High loading case voltage deadband",
			PT_double, "low_load_deadband[V]",PADDR(vbw_low),PT_DESCRIPTION,"Low loading case voltage deadband",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int volt_var_control::isa(char *classname)
{
	return strcmp(classname,"volt_var_control")==0;
}

int volt_var_control::create(void)
{
	int result = powerflow_object::create();
	phases = PHASE_A;	//Arbitrary - set so powerflow_object shuts up (library doesn't grant us access to time functions)

	control_method = ACTIVE;	//Turn it on by default
	cap_time_delay = 5.0;		//5 second default capacitor delay
	reg_time_delay = 5.0;		//5 second default regulator delay
	desired_pf = 0.98;
	d_min = 0.3;				//Chosen from Borozan paper defaults
	d_max = 0.6;				//Chosen from Borozan paper defaults
	substation_lnk_obj = NULL;
	feeder_regulator_obj = NULL;
	pCapacitor_list = NULL;
	Capacitor_size = NULL;
	pMeasurement_list = NULL;
	minimum_voltage = -1;		//Flag to ensure it is set
	maximum_voltage = -1;		//Flag to ensure it is set
	desired_voltage = -1;		//Flag to ensure it is set
	max_vdrop = -1;				//Flag to ensure it is set

	vbw_low = -1;				//Flag to ensure is set
	vbw_high = -1;

	num_caps = 1;				//Default to 1 for parsing.  Will update in init
	num_meas = 1;				//Default to 1 for parsing.  Will update in init

	Regulator_Change = false;	//Start by assuming no regulator change is occurring
	TRegUpdate = 0;
	TCapUpdate = 0;
	TUpdateStatus = false;		//Flag for control_method transitions
	pf_phase = 0;				//No phases monitored by default - will drop to link if unpopulated
	
	first_cycle = true;		//Set up the variable

	prev_time = 0;

	PrevRegState = regulator_configuration::MANUAL;	//Assumes was in manual


	return result;
}

int volt_var_control::init(OBJECT *parent)
{
	int retval = powerflow_object::init(parent);

	int index;
	char *token;
	OBJECT *temp_obj;
	capacitor **pCapacitor_list_temp;
	double *temp_cap_size;
	int *temp_cap_idx, *temp_cap_idx_work;
	double cap_adder;
	set temp_phase;

	OBJECT *obj = OBJECTHDR(this);

	//General error checks
	if ((d_max <= 0.0) || (d_max > 1.0) || (d_min <= 0.0) || (d_min > 1.0))
	{
		GL_THROW("volt_var_control %s: d_max and d_min must be a number between 0 and 1",obj->name);
		/*  TROUBLESHOOT
		The capacitor threshold values d_max and d_min represent a fraction of a capacitor's total kVA rating.
		They must be specified greater than 0, but less than or equal to 1.
		*/
	}

	if (d_min >= d_max)
	{
		GL_THROW("volt_var_control %s: d_min must be less than d_max",obj->name);
		/*  TROUBLESHOOT
		The capacitor threshold fraction for d_min must be less than d_max.  Otherwise, improper (or no) operation
		will occur.
		*/
	}

	if (cap_time_delay < 0)
	{
		GL_THROW("volt_var_control %s: capacitor_delay must be a positive number!",obj->name);
		/*  TROUBLESHOOT
		The default capacitor delay must be a positive number.  Non-causal or negative time delays are not permitted.
		*/
	}

	if (cap_time_delay < 1)	//Has to be positive in leiu of previous error check
	{
		gl_warning("volt_var_control %s: capacitor_delay should be greater than or equal to 1 to prevent convergence issues.",obj->name);
		/*  TROUBLESHOOT
		The capacitor_delay should be greater than or equal to 1.0 to avoid having a system oscillate switch positions until the
		convergence limit is reached.
		*/
	}

	if (reg_time_delay < 0)
	{
		GL_THROW("volt_var_control %s: regulator_delay must be a positive number!",obj->name);
		/*  TROUBLESHOOT
		The default regilator delay must be a positive number.  Non-causal or negative time delays are not permitted.
		*/
	}

	if (reg_time_delay < 1)	//Has to be positive in leiu of previous error check
	{
		gl_warning("volt_var_control %s: regulator_delay should be greater than or equal to 1 to prevent convergence issues.",obj->name);
		/*  TROUBLESHOOT
		The regulator_delay should be greater than or equal to 1.0 to avoid having a system oscillate switch positions until the
		convergence limit is reached.
		*/
	}

	//Make sure the regulator is populated as well (may be the same as substation_lnk, but we'll keep separate just in case)
	if (feeder_regulator_obj == NULL)
	{
		GL_THROW("volt_var_control %s: feeder regulator is not specified",obj->name);
		/*  TROUBLESHOOT
		The volt_var_control object requires a regulator to control for coordinated action.  Please specify
		an appropriate regulator on the system.
		*/
	}

	//Link the regulator
	feeder_regulator = OBJECTDATA(feeder_regulator_obj,regulator);

	//Grab it's config as well
	feeder_reg_config = OBJECTDATA(feeder_regulator->configuration,regulator_configuration);

	//Compute the regulator voltage steps
	reg_step_up = feeder_reg_config->band_center * feeder_reg_config->regulation / ((double) feeder_reg_config->raise_taps);
	reg_step_down = feeder_reg_config->band_center * feeder_reg_config->regulation / ((double) feeder_reg_config->lower_taps);

	//See if the regulator is in manual mode, if we are on right now
	if (control_method == ACTIVE)
	{
		if (feeder_reg_config->Control != regulator_configuration::MANUAL)	//We're on, but regulator not in manual.  Set up as a transition
		{
			prev_mode = STANDBY;
		}
		else	//We're on and the regulator is in manual, start up that way
		{
			prev_mode = ACTIVE;
		}
	}
	else	//We're in standby, so we don't care what the regulator is doing right now
	{
		prev_mode = STANDBY;
	}

	if (substation_lnk_obj == NULL)	//If nothing specified, just use the feeder regulator
	{
		gl_warning("volt_var_control %s: A link to monitor power-factor was not specified, using feeder regulator.",obj->name);
		/*  TROUBLESHOOT
		The volt_var_control object requires a link-based object to measure power factor.  Since one was not specified, the control
		scheme will just monitor the power values of the feeder regulator.
		*/

		//Assigns regulator to the link object
		substation_lnk_obj = feeder_regulator_obj;
	}

	//Link it up
	substation_link = OBJECTDATA(substation_lnk_obj,link);

	//See if minimum and maximum voltages are set.  If not, base on nominal
	if ((minimum_voltage == -1) || (maximum_voltage == -1))
	{
		node *temp_node = OBJECTDATA(feeder_regulator->to,node);
		double nom_volt = temp_node->nominal_voltage;

		if (minimum_voltage == -1)
		{
			minimum_voltage = 0.95*nom_volt;
			gl_warning("volt_var_control %s: Minimum voltage not specified, taking as 0.95 p.u. of regulator TO-end nominal",obj->name);
		}

		if (maximum_voltage == -1)
		{
			maximum_voltage = 1.05*nom_volt;
			gl_warning("volt_var_control %s: Maximum voltage not specified, taking as 1.05 p.u. of regulator TO-end nominal",obj->name);
		}
	}

	//Make sure voltages are ok
	if (minimum_voltage >= maximum_voltage)
	{
		GL_THROW("volt_var_control %s: Minimum voltage limit must be less than the maximum voltage limit of the system",obj->name);
		/*  TROUBLESHOOT
		The minimum_voltage value must be less than the maximum_voltage value to ensure proper system operation.
		*/
	}

	//Check desired voltage
	if (desired_voltage == -1)	//Uninitialized
	{
		desired_voltage = (minimum_voltage+maximum_voltage)/2;	//Just set to mid-point of range
		gl_warning("volt_var_control %s: Desired voltage not specified, so taking as midpoint of min and max limits",obj->name);
	}

	//Make sure desired voltage is within operating limits
	if ((desired_voltage < minimum_voltage) || (desired_voltage > maximum_voltage))
	{
		GL_THROW("volt_var_control %s: Desired voltage is outside the minimum and maximum set points.",obj->name);
		/*  TROUBLESHOOT
		The desired_voltage property is specified in the range outside of the minimum_voltage and maximum_voltage
		set points.  It needs to lie between these points for proper operation.  Please adjust the values, or leave
		desired_voltage blank to let the system decide.
		*/
	}

	//See if max_vdrop has been set.  Validity will be check next
	if (max_vdrop == -1)	//Not set
	{
		max_vdrop = 1.5*reg_step_up;	//Arbitrarily set to 1.5x a step change

		gl_warning("volt_var_control %s: Maximum expected voltage drop was set to 1.5x an upper tap change.",obj->name);
		/*  TROUBLESHOOT
		The max_vdrop property was not set, so it was defaulted to 1.5x the upper tap change of the regulator.  If a tighter
		constraint is required, please specify a value in max_vdrop.
		*/
	}

	//Check to make sure max_vdrop isn't negative
	if (max_vdrop <= 0)
	{
		GL_THROW("volt_var_control %s: Maximum expected voltage drop should be a positive non-zero number.",obj->name);
		/*  TROUBLESHOOT
		The max_vdrop property must be greater than 0 for proper operation of the coordinated Volt-VAr control scheme.
		*/
	}

	//See if vbw_low is set (may be invalidly set at this point, but that gets checked below
	if (vbw_low == -1)	//Not set
	{
		//Set to 2 tap positions out of whack
		vbw_low = 2.0*reg_step_up;

		gl_warning("voltage_var_control %s: Low loading deadband not specified, so set to 2x upper tap change",obj->name);
		/*  TROUBLESHOOT
		The low_load_deadband value was not specified, so it is taken as 2x an upper tap change.  If a tighter constraint is
		desired, please specify a value in low_load_deadband.
		*/
	}

	//See if vbw_high is set (may be invalidly set at this point, but that gets checked below
	if (vbw_high == -1)	//Not set
	{
		//Set to 1 tap positions out of whack (more constrained than vbw_low)
		vbw_high = reg_step_up;

		gl_warning("voltage_var_control %s: High loading deadband not specified, so set to an upper tap change",obj->name);
		/*  TROUBLESHOOT
		The high_load_deadband value was not specified, so it is taken as an upper tap change.  If a tighter constraint is
		desired, please specify a value in high_load_deadband.
		*/
	}

	//Make sure bandwidth values aren't negative
	if (vbw_low <= 0)
	{
		GL_THROW("volt_var_control %s: Low loading deadband (bandwidth) must be a positive non-zero number",obj->name);
		/*  TROUBLESHOOT
		The low_load_deadband setting must be greater than 0 for proper operation of the Volt-VAr controller.
		*/
	}

	if (vbw_high <= 0)
	{
		GL_THROW("volt_var_control %s: High loading deadband (bandwidth) must be a positive non-zero number",obj->name);
		/*  TROUBLESHOOT
		The high_load_deadband setting must be greater than 0 for proper operation of the Volt-VAr controller.
		*/
	}

	//vbw_high is supposed to be less than vbw_low (heavy loading means we should be more constrained)
	if (vbw_high > vbw_low)
	{
		gl_warning("volt_var_control %s: Low loading deadband is expected to be larger than the high loading deadband",obj->name);
		/*  TROUBLESHOOT
		The high_load_deadband is larger than the low_load_deadband setting.  The algorithm proposed in the referenced paper
		suggests low_load_deadband should be larger (less constrained) than the high load.  This may need to be fixed to be
		consistent with expected results.
		*/
	}

	//Determine how many capacitors we have to play with
	index=0;
	while (capacitor_list[index] != NULL)
	{
		if (capacitor_list[index] == 44)	//Found a comma!
			num_caps++;						//Increment the number of capacitors present
		
		index++;	//Increment the pointer
	}

	if (num_caps==1)	//Only 1 column, make sure something is actually in there
	{
		temp_obj = gl_get_object((char *)capacitor_list);

		if (temp_obj == NULL)	//Not really an object, must be no controllable capacitors
			num_caps = 0;
	}

	if (num_caps > 0)
	{
		if (num_caps == 1)	//Only one capacitor
		{
			//malloc the list (there is only 1, so this pretty stupid, but meh)
			pCapacitor_list = (capacitor **)gl_malloc(sizeof(capacitor));

			if (pCapacitor_list == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				/*  TROUBLESHOOT
				The volt_var_control failed to allocate memory for the controllable capacitors table.
				Please try again.  If the error persists, please submit your code and a bug report via
				the trac website.
				*/
			}

			//malloc the size (there is only 1, so this again pretty stupid, but meh)
			Capacitor_size = (double*)gl_malloc(sizeof(double));

			if (Capacitor_size == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//Parse the list now
			token = strtok(capacitor_list,",");

			temp_obj = gl_get_object((char *)token);
				
			if (temp_obj != NULL)	//Valid object!
			{
				pCapacitor_list[0] = OBJECTDATA(temp_obj,capacitor);	//Store it as a capacitor

				//Add up the values
				if ((pCapacitor_list[0]->phases_connected & PHASE_D) == PHASE_D)	//Delta - grab all three (assumed ABC)
				{
					cap_adder = pCapacitor_list[0]->capacitor_A + pCapacitor_list[0]->capacitor_B + pCapacitor_list[0]->capacitor_C;
				}
				else	//Non-delta
				{
					cap_adder = 0;	//Clear the accumulator

					if ((pCapacitor_list[0]->phases_connected & PHASE_A) == PHASE_A)	//Add in phase A contribution
						cap_adder += pCapacitor_list[0]->capacitor_A;

					if ((pCapacitor_list[0]->phases_connected & PHASE_B) == PHASE_B)	//Add in phase B contribution
						cap_adder += pCapacitor_list[0]->capacitor_B;

					if ((pCapacitor_list[0]->phases_connected & PHASE_C) == PHASE_C)	//Add in phase C contribution
						cap_adder += pCapacitor_list[0]->capacitor_C;
				}
				Capacitor_size[0] = cap_adder;	//Store the size

			}
			else	//General catch, not sure how it would get here
			{
				GL_THROW("volt_var_control %s: Single capacitor list population failed",obj->name);
				/*  TROUBLESHOOT
				While attempting to populate the capacitor list with a single capacitor, it somehow
				failed to recognize this was a valid object.  Please submit your code and a bug report
				using the trac website.
				*/
			}
		}//End only 1 cap
		else	//More than one
		{
			//malloc the temp list
			pCapacitor_list_temp = (capacitor **)gl_malloc(num_caps*sizeof(capacitor));

			if (pCapacitor_list_temp == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//malloc the actual list
			pCapacitor_list = (capacitor **)gl_malloc(num_caps*sizeof(capacitor));

			if (pCapacitor_list == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//malloc the temp index
			temp_cap_idx = (int*)gl_malloc(num_caps*sizeof(int));

			if (temp_cap_idx == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//malloc the temp work index
			temp_cap_idx_work = (int*)gl_malloc(num_caps*sizeof(int));

			if (temp_cap_idx_work == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//malloc the temp size
			temp_cap_size = (double*)gl_malloc(num_caps*sizeof(double));

			if (temp_cap_size == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//malloc the actual size
			Capacitor_size = (double*)gl_malloc(num_caps*sizeof(double));

			if (Capacitor_size == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//Parse the list now
			token = strtok(capacitor_list,",");
			for (index = 0; index < num_caps; index++)
			{
				temp_obj = gl_get_object((char *)token);
				
				if (temp_obj != NULL)	//Valid object!
				{
					pCapacitor_list_temp[index] = OBJECTDATA(temp_obj,capacitor);	//Store it as a capacitor
					temp_cap_idx[index] = index;	//Store the index

					//Add up the values
					if ((pCapacitor_list_temp[index]->phases_connected & PHASE_D) == PHASE_D)	//Delta - grab all three (assumed ABC)
					{
						cap_adder = pCapacitor_list_temp[index]->capacitor_A + pCapacitor_list_temp[index]->capacitor_B + pCapacitor_list_temp[index]->capacitor_C;
					}
					else	//Non-delta
					{
						cap_adder = 0;	//Clear the accumulator

						if ((pCapacitor_list_temp[index]->phases_connected & PHASE_A) == PHASE_A)	//Add in phase A contribution
							cap_adder += pCapacitor_list_temp[index]->capacitor_A;

						if ((pCapacitor_list_temp[index]->phases_connected & PHASE_B) == PHASE_B)	//Add in phase B contribution
							cap_adder += pCapacitor_list_temp[index]->capacitor_B;

						if ((pCapacitor_list_temp[index]->phases_connected & PHASE_C) == PHASE_C)	//Add in phase C contribution
							cap_adder += pCapacitor_list_temp[index]->capacitor_C;
					}
					Capacitor_size[index] = cap_adder;	//Store the size
				}
				else	//General catch
				{
					GL_THROW("volt_var_control %s: item %s in capacitor_list not found",obj->name,(char *)token);
					/*  TROUBLESHOOT
					While attempting to populate the capacitor list, an invalid object was found.  Please check
					the object name.
					*/
				}

				token = strtok(NULL,",");
			}//end token FOR

			//Sort the lists on size and distance (assumes put in distance order) - Biggest to smallest, and furthest to closest
			size_sorter(Capacitor_size, temp_cap_idx, num_caps, temp_cap_size, temp_cap_idx_work);

			//Now populate the end capacitor list based on size
			for (index=0; index < num_caps; index++)
			{
				pCapacitor_list[index] = pCapacitor_list_temp[temp_cap_idx[index]];
			}

			//Free up all of the extra values malloced
			gl_free(pCapacitor_list_temp);
			gl_free(temp_cap_idx);
			gl_free(temp_cap_idx_work);
			gl_free(temp_cap_size);
		}//end more than one cap

		//Populate the "state holder"
		PrevCapState = (capacitor::CAPCONTROL*)gl_malloc(num_caps*sizeof(capacitor::CAPCONTROL));
		
		if (PrevCapState == NULL)
		{
			GL_THROW("volt_var_control %s: capacitor previous state allocation failure",obj->name);
			/*  TROUBLESHOOT
			While attempting to allocate space for the previous capacitor control state vector, a
			memory allocation error occurred.  Please try again.  If the problem persists, please
			submit your code and a bug report via the trac website.
			*/
		}

		//Populate it with MANUAL (just cause)
		for (index = 0; index < num_caps; index++)
		{
			PrevCapState[index] = capacitor::MANUAL;
		}

		//Another loop, make sure if they are in manual, they have a PT_PHASE defined (other modes will check this inside capacitors)
		for (index = 0; index < num_caps; index++)
		{
			if (pCapacitor_list[index]->control == capacitor::MANUAL)	//Only applies to manual mode
			{
				if ((pCapacitor_list[index]->pt_phase & (PHASE_A | PHASE_B | PHASE_C)) == NO_PHASE)
				{
					temp_obj = OBJECTHDR(pCapacitor_list[index]);
					gl_warning("Capacitor %s has no pt_phase set, volt_var_control %s will pick the first phases_connected",temp_obj->name,obj->name);
					/*  TROUBLESHOOT
					To properly operate capacitors from the volt_var_control object, a pt_phase property must be defined.  One was missing,
					so the volt_var_control object assigned the first phase_connected value to pt_phase on that capacitor for proper operation.
					*/

					if ((pCapacitor_list[index]->phases_connected & PHASE_A) == PHASE_A)
					{
						pCapacitor_list[index]->pt_phase = PHASE_A;
					}
					else if ((pCapacitor_list[index]->phases_connected & PHASE_B) == PHASE_B)
					{
						pCapacitor_list[index]->pt_phase = PHASE_B;
					}
					else	//We'll just say it has to be C.  Not sure if this is true, but pt_phase needs to be populated for proper operation (especially banked)
					{
						pCapacitor_list[index]->pt_phase = PHASE_C;
					}
				}//end no phase if
			}//end manual mode if
		}//End cap list traversion for manual mode caps
	}//End cap list if

	//Now figure out the measurements
	index=0;
	while (measurement_list[index] != NULL)
	{
		if (measurement_list[index] == 44)	//Found a comma!
			num_meas++;						//Increment the number of measurements present
		
		index++;	//Increment the pointer
	}

	if (num_meas==1)	//Only 1 column, make sure something is actually in there
	{
		temp_obj = gl_get_object((char *)measurement_list);

		if (temp_obj == NULL)	//Not really an object, must be no measurement points
			num_meas = 0;
	}

	if (num_meas > 0)
	{
		//malloc the list (if there is only 1, this might be pretty stupid)
		pMeasurement_list = (node **)gl_malloc(num_meas*sizeof(node));

		if (pMeasurement_list == NULL)
		{
			GL_THROW("volt_var_control %s: Failed to allocate measurement memory",obj->name);
			/*  TROUBLESHOOT
			The volt_var_control failed to allocate memory for the voltage measurements table.
			Please try again.  If the error persists, please submit your code and a bug report via
			the trac website.
			*/
		}

		//Parse the list now
		token = strtok(measurement_list,",");
		for (index = 0; index < num_meas; index++)
		{
			temp_obj = gl_get_object((char *)token);
			
			if (temp_obj != NULL)	//Valid object!
			{
				pMeasurement_list[index] = OBJECTDATA(temp_obj,node);	//Store it as a node
			}
			else
			{
				GL_THROW("volt_var_control %s: measurement object %s was not found!",obj->name,token);
				/*  TROUBLESHOOT
				While parsing the measurment list for the volt_var_control object, a measurement point
				was not found.  Please check the name and try again.  If the problem persists, please submit 
				your code and a bug report via the trac website.
				*/
			}

			token = strtok(NULL,",");
		}//end token FOR
	}//End measurement list if
	else	//0 measurements
	{
		gl_warning("volt_var_control %s: No measurement point specified - defaulting to load side of regulator",obj->name);
		/*  TROUBLESHOOT
		If no measuremnt points are specified for the volt_var_control object, it will default to the load side of the 
		feeder regulator.  Please specify measurement points for better control.
		*/

		num_meas = 1;	//We're going to give it one

		//malloc the list - there's only 1, so this is probably stupid, but meh
		pMeasurement_list = (node **)gl_malloc(sizeof(node));

		if (pMeasurement_list == NULL)
		{
			GL_THROW("volt_var_control %s: Failed to allocate measurement memory",obj->name);
			/*  TROUBLESHOOT
			The volt_var_control failed to allocate memory for the voltage measurements table.
			Please try again.  If the error persists, please submit your code and a bug report via
			the trac website.
			*/
		}

		//Populate the list now
		pMeasurement_list[0] = OBJECTDATA(feeder_regulator->to,node);	//Store it as a node
	}//End 0 measurements default

	//Populate the two side of the regulator for future use - need it for VO in feeder logic
	RegToNode = OBJECTDATA(feeder_regulator->to,node);

	//Determine phases to monitor - if none selected, default to the link phases
	if ((pf_phase & (PHASE_A | PHASE_B | PHASE_C)) == 0)	//No phase (that we care about)
	{
		pf_phase = (substation_link->phases & (PHASE_A | PHASE_B | PHASE_C));	//Just bring in the phases of the link
		gl_warning("volt_var_control %s: Power factor monitored phases not set - defaulting to substation_link phases",obj->name);
		/*  TROUBLESHOOT
		The pf_phases variable was not set, so the volt_var_control object defaulted to the phases of the substation_link object.
		If this is not desired, please specify the phases on pf_phases.
		*/
	}

	//Make sure the phases are valid - strip off any N or D of either
	temp_phase = (pf_phase & (PHASE_A | PHASE_B | PHASE_C));	//Extract the phases we want

	if ((substation_link->phases & temp_phase) != temp_phase)	//Mismatch
	{
		GL_THROW("volt_var_control %s: Power factor monitored phases mismatch",obj->name);
		/*  TROUBLESHOOT
		One or more of the phases specified in pf_phase does not match with the phases of
		substation_link.  Please double check the values for pf_phases on the volt_var_control
		and phases on the substation_link object.
		*/
	}

	if (solver_method == SM_NR)
	{
		//Set our rank above links - this will put us before regs on the down sweep and before caps on the upsweep (but after current calculations)
		gl_set_rank(obj,2);
	}
	else	//FBS mainly - put us above the regulator
	{
		//TBD
		GL_THROW("Solver methods other than Newton-Raphson are unsupported at this time.");
		/*  TROUBLESHOOT
		The volt_var_control object only supports the Newton-Raphson solver at this time.
		Forward-Back Sweep will be implemented at a future date.
		*/
	}

	return retval;
}

TIMESTAMP volt_var_control::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::presync(t0);
	double vmin[3], VDrop[3], VSet[3], VRegTo[3];
	int indexer;
	char temp_var_u, temp_var_d;
	int prop_tap_changes[3];
	bool limit_hit = false;	//mainly for banked operations
	char LimitExceed = 0x00;	// U_D - XCBA_XCBA

	if (t0 >= TRegUpdate)
	{
		Regulator_Change = false;	//Unflag the regulator change status
	}

	if (TUpdateStatus && (TRegUpdate <= t0))	//See if we're "recovering" from a transition
	{
		TUpdateStatus = false;	//Clear the flag
		TRegUpdate = TS_NEVER;		//Let us proceed
	}

	if (NR_cycle == true)	//Accumulation cycle checks
	{
		if (t0 != prev_time)	//New timestep
		{
			first_cycle = true;		//Flag as first entrant this time step (used to reactive control)
			prev_time = t0;			//Update tracking variable
		}
		else
		{
			first_cycle = false;	//No longer the first cycle
		}
	}

	if (control_method != prev_mode)	//We've altered our mode
	{
		if (control_method == ACTIVE)	//We're active, implies were in standby before
		{
			//Store the state of the regulator
			PrevRegState = feeder_reg_config->Control;

			//Now force it into manual mode
			feeder_reg_config->Control = regulator_configuration::MANUAL;

			//Now let's do the same for capacitors
			for (indexer=0; indexer < num_caps; indexer++)
			{
				PrevCapState[indexer] = pCapacitor_list[indexer]->control;	//Store the old
				pCapacitor_list[indexer]->control = capacitor::MANUAL;		//Put it into manual mode
			}
		}
		else	//Now in standby, implies were active before
		{
			feeder_reg_config->Control = PrevRegState;	//Put the regulator's mode back

			//Put capacitors back in previous mode
			for (indexer=0; indexer < num_caps; indexer++)
			{
				pCapacitor_list[indexer]->control = PrevCapState[indexer];
			}
		}

		TRegUpdate = t0;				//Flag us to stay here.  This will force another iteration
		TUpdateStatus = true;		//Flag the update
		prev_mode = control_method;	//Update the tracker
	}

	if ((solver_method == SM_NR) && (NR_cycle == false) && (Regulator_Change == false) && (control_method == ACTIVE))	//Solution pass and not in an existing change - also are turned on
	{
		//Initialize "minimums" to something big
		vmin[0] = vmin[1] = vmin[2] = 999999999999.0;

		//Initialize VDrop and VSet value
		VDrop[0] = VDrop[1] = VDrop[2] = 0.0;
		VSet[0] = VSet[1] = VSet[2] = desired_voltage;	//Default VSet to where we want

		//Initialize tap changes
		prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;

		//Parse through the measurement list - find the lowest voltage - do by PT_PHASE
		for (indexer=0; indexer<num_meas; indexer++)
		{
			//See if this node has phase A
			if ((pMeasurement_list[indexer]->phases & PHASE_A) == PHASE_A)	//Has phase A
			{
				if (pMeasurement_list[indexer]->voltage[0].Mag() < vmin[0])	//New minimum
				{
					vmin[0] = pMeasurement_list[indexer]->voltage[0].Mag();
				}
			}

			//See if this node has phase B
			if ((pMeasurement_list[indexer]->phases & PHASE_B) == PHASE_B)	//Has phase B
			{
				if (pMeasurement_list[indexer]->voltage[1].Mag() < vmin[1])	//New minimum
				{
					vmin[1] = pMeasurement_list[indexer]->voltage[1].Mag();
				}
			}

			//See if this node has phase C
			if ((pMeasurement_list[indexer]->phases & PHASE_C) == PHASE_C)	//Has phase A
			{
				if (pMeasurement_list[indexer]->voltage[2].Mag() < vmin[2])	//New minimum
				{
					vmin[2] = pMeasurement_list[indexer]->voltage[2].Mag();
				}
			}
		}

		//Populate VRegTo (to end voltages)
		VRegTo[0] = RegToNode->voltage[0].Mag();
		VRegTo[1] = RegToNode->voltage[1].Mag();
		VRegTo[2] = RegToNode->voltage[2].Mag();

		//Populate VDrop and VSet based on PT_PHASE
		if ((feeder_reg_config->PT_phase & PHASE_A) == PHASE_A)	//We have an A
		{
			VDrop[0] = VRegTo[0] - vmin[0];		//Calculate the drop
			VSet[0] = desired_voltage + VDrop[0];			//Calculate where we want to be

			//Make sure it isn't too big or small - otherwise toss a warning (it will cause problem)
			if (VSet[0] > maximum_voltage)	//Will exceed
			{
				gl_verbose("volt_var_control %s: The set point for phase A will exceed the maximum allowed voltage!",OBJECTHDR(this)->name);
				/*  TROUBLESHOOT
				The set point necessary to maintain the end point voltage exceeds the maximum voltage limit specified by the system.  Either
				increase this maximum_voltage limit, or configure your system differently.
				*/

				//See what region we are currently in
				if (feeder_regulator->tap[0] > 0)	//In raise region
				{
					if ((VRegTo[0] + reg_step_up) > maximum_voltage)
					{
						LimitExceed |= 0x10;	//Flag A as above
					}
				}
				else	//Must be in lower region
				{
					if ((VRegTo[0] + reg_step_down) > maximum_voltage)
					{
						LimitExceed |= 0x10;	//Flag A as above
					}
				}
			}
			else if (VSet[0] < minimum_voltage)	//Will exceed
			{
				gl_verbose("volt_var_control %s: The set point for phase A will exceed the minimum allowed voltage!",OBJECTHDR(this)->name);
				/*  TROUBLESHOOT
				The set point necessary to maintain the end point voltage exceeds the minimum voltage limit specified by the system.  Either
				decrease this minimum_voltage limit, or configure your system differently.
				*/

				//See what region we are currently in
				if (feeder_regulator->tap[0] > 0)	//In raise region
				{
					if ((VRegTo[0] - reg_step_up) < minimum_voltage)
					{
						LimitExceed |= 0x01;	//Flag A as below
					}
				}
				else	//Must be in lower region
				{
					if ((VRegTo[0] - reg_step_down) < minimum_voltage)
					{
						LimitExceed |= 0x01;	//Flag A as below
					}
				}
			}
		}

		if ((feeder_reg_config->PT_phase & PHASE_B) == PHASE_B)	//We have a B
		{
			VDrop[1] = VRegTo[1] - vmin[1];		//Calculate the drop
			VSet[1] = desired_voltage + VDrop[1];			//Calculate where we want to be

			//Make sure it isn't too big or small - otherwise toss a warning (it will cause problem)
			if (VSet[1] > maximum_voltage)	//Will exceed
			{
				gl_verbose("volt_var_control %s: The set point for phase B will exceed the maximum allowed voltage!",OBJECTHDR(this)->name);

				//See what region we are currently in
				if (feeder_regulator->tap[1] > 0)	//In raise region
				{
					if ((VRegTo[1] + reg_step_up) > maximum_voltage)
					{
						LimitExceed |= 0x20;	//Flag B as above
					}
				}
				else	//Must be in lower region
				{
					if ((VRegTo[1] + reg_step_down) > maximum_voltage)
					{
						LimitExceed |= 0x20;	//Flag B as above
					}
				}
			}
			else if (VSet[1] < minimum_voltage)	//Will exceed
			{
				gl_verbose("volt_var_control %s: The set point for phase B will exceed the minimum allowed voltage!",OBJECTHDR(this)->name);

				//See what region we are currently in
				if (feeder_regulator->tap[1] > 0)	//In raise region
				{
					if ((VRegTo[1] - reg_step_up) < minimum_voltage)
					{
						LimitExceed |= 0x02;	//Flag B as below
					}
				}
				else	//Must be in lower region
				{
					if ((VRegTo[1] - reg_step_down) > minimum_voltage)
					{
						LimitExceed |= 0x02;	//Flag B as below
					}
				}
			}
		}

		if ((feeder_reg_config->PT_phase & PHASE_C) == PHASE_C)	//We have a C
		{
			VDrop[2] = VRegTo[2] - vmin[2];		//Calculate the drop
			VSet[2] = desired_voltage + VDrop[2];			//Calculate where we want to be
			//Make sure it isn't too big or small - otherwise toss a warning (it will cause problem)

			if (VSet[2] > maximum_voltage)	//Will exceed
			{
				gl_verbose("volt_var_control %s: The set point for phase C will exceed the maximum allowed voltage!",OBJECTHDR(this)->name);

				//See what region we are currently in
				if (feeder_regulator->tap[2] > 0)	//In raise region
				{
					if ((VRegTo[2] + reg_step_up) > maximum_voltage)
					{
						LimitExceed |= 0x40;	//Flag C as above
					}
				}
				else	//Must be in lower region
				{
					if ((VRegTo[2] + reg_step_down) > maximum_voltage)
					{
						LimitExceed |= 0x40;	//Flag C as above
					}
				}
			}
			else if (VSet[2] < minimum_voltage)	//Will exceed
			{
				gl_verbose("volt_var_control %s: The set point for phase C will exceed the minimum allowed voltage!",OBJECTHDR(this)->name);

				//See what region we are currently in
				if (feeder_regulator->tap[2] > 0)	//In raise region
				{
					if ((VRegTo[2] - reg_step_up) < minimum_voltage)
					{
						LimitExceed |= 0x04;	//Flag C as below
					}
				}
				else	//Must be in lower region
				{
					if ((VRegTo[2] - reg_step_down) < minimum_voltage)
					{
						LimitExceed |= 0x04;	//Flag C as below
					}
				}
			}
		}

		//Now determine what kind of regulator we are
		if (feeder_reg_config->control_level == regulator_configuration::INDIVIDUAL)	//Individually handled
		{
			//Handle phases
			for (indexer=0; indexer<3; indexer++)	//Loop through phases
			{
				LimitExceed &= 0x7F;	//Use bit 8 as a validity flag (to save a variable)
				
				if ((indexer==0) && ((feeder_reg_config->PT_phase & PHASE_A) == PHASE_A))	//We have an A
				{
					temp_var_d = 0x01;		//A base lower "Limit" checker
					temp_var_u = 0x10;		//A base upper "Limit" checker
					LimitExceed |= 0x80;	//Valid phase
				}

				if ((indexer==1) && ((feeder_reg_config->PT_phase & PHASE_B) == PHASE_B))	//We have a B
				{
					temp_var_d = 0x02;		//B base lower "Limit" checker
					temp_var_u = 0x20;		//B base upper "Limit" checker
					LimitExceed |= 0x80;	//Valid phase
				}

				if ((indexer==2) && ((feeder_reg_config->PT_phase & PHASE_C) == PHASE_C))	//We have a C
				{
					temp_var_d = 0x04;		//C base lower "Limit" checker
					temp_var_u = 0x40;		//C base upper "Limit" checker
					LimitExceed |= 0x80;	//Valid phase
				}

				if ((LimitExceed & 0x80) == 0x80)	//Valid phase
				{
					//Make sure we aren't below the minimum or above the maximum first				  (***** This below here \/ \/ *************) - sub with step check! **************
					if (((vmin[indexer] > maximum_voltage) || (VRegTo[indexer] > maximum_voltage)) && ((LimitExceed & temp_var_d) != temp_var_d))		//Above max, but won't go below
					{
						//Flag us for a down tap
						prop_tap_changes[indexer] = -1;
					}
					else if (((vmin[indexer] < minimum_voltage) || (VRegTo[indexer] < minimum_voltage)) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below min, but won't exceed
					{
						//Flag us for an up tap
						prop_tap_changes[indexer] = 1;
					}
					else	//Normal operation
					{
						//See if we are in high load or low load conditions
						if (VDrop[indexer] > max_vdrop)	//High loading
						{
							//See if we're outside our range
							if (((VSet[indexer] + vbw_high) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d)) 	//Above deadband, but can go down
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (feeder_regulator->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step down will get us theoretically
									if ((VRegTo[indexer] - reg_step_up) < minimum_voltage)	//We'll fall below
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = -1;  //Try to tap us down
									}
								}
								else	//Must be lower region
								{
									//Find out what a step down will get us theoretically
									if ((VRegTo[indexer] - reg_step_down) < minimum_voltage)	//We'll fall below
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = -1;  //Try to tap us down
									}
								}
							}//end above deadband
							else if (((VSet[indexer] - vbw_high) > VRegTo[indexer]) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below deadband, but can go up
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (feeder_regulator->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step up will get us theoretically
									if ((VRegTo[indexer] + reg_step_up) > maximum_voltage)	//We'll exceed
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = 1;  //Try to tap us up
									}
								}
								else	//Must be lower region
								{
									//Find out what a step up will get us theoretically
									if ((VRegTo[indexer] + reg_step_down) > maximum_voltage)	//We'll fall exceed
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = 1;  //Try to tap us up
									}
								}
							}//end below deadband
							//defaulted else - inside the deadband, so we don't care
						}//end high loading
						else	//Low loading
						{
							//See if we're outside our range
							if (((VSet[indexer] + vbw_low) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Above deadband, but can go down
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (feeder_regulator->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step down will get us theoretically
									if ((VRegTo[indexer] - reg_step_up) < minimum_voltage)	//We'll fall below
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = -1;  //Try to tap us down
									}
								}
								else	//Must be lower region
								{
									//Find out what a step down will get us theoretically
									if ((VRegTo[indexer] - reg_step_down) < minimum_voltage)	//We'll fall below
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = -1;  //Try to tap us down
									}
								}
							}//end above deadband
							else if (((VSet[indexer] - vbw_low) > VRegTo[indexer]) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below deadband, but can go up
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (feeder_regulator->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step up will get us theoretically
									if ((VRegTo[indexer] + reg_step_up) > maximum_voltage)	//We'll exceed
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = 1;  //Try to tap us up
									}
								}
								else	//Must be lower region
								{
									//Find out what a step up will get us theoretically
									if ((VRegTo[indexer] + reg_step_down) > maximum_voltage)	//We'll fall exceed
									{
										prop_tap_changes[indexer] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[indexer] = 1;  //Try to tap us up
									}
								}
							}//end below deadband
							//defaulted else - inside the deadband, so we don't care
						}//end low loading
					}//end normal ops
				}//End valid phase
			}//end phase FOR

			//Apply the taps - loop through phases (nonexistant phases should just be 0
			//Default assume no change will occur
			Regulator_Change = false;
			TRegUpdate = TS_NEVER;

			for (indexer=0; indexer<3; indexer++)
			{
				if (prop_tap_changes[indexer] > 0)	//Want to tap up
				{
					//Make sure we aren't railed
					if (feeder_regulator->tap[indexer] >= feeder_reg_config->raise_taps)
					{
						//Just leave us railed - explicity done in case it somehow ends up over raise_taps (not sure how that would happen)
						feeder_regulator->tap[indexer] = feeder_reg_config->raise_taps;
					}
					else	//Must have room
					{
						feeder_regulator->tap[indexer]++;	//Increment

						//Flag as change
						Regulator_Change = true;
						TRegUpdate = t0 + (TIMESTAMP)reg_time_delay;	//Set return time
					}
				}//end tap up
				else if (prop_tap_changes[indexer] < 0)	//Tap down
				{
					//Make sure we aren't railed
					if (feeder_regulator->tap[indexer] <= -feeder_reg_config->lower_taps)
					{
						//Just leave us railed - explicity done in case it somehow ends up under lower_taps (not sure how that would happen)
						feeder_regulator->tap[indexer] = -feeder_reg_config->lower_taps;
					}
					else	//Must have room
					{
						feeder_regulator->tap[indexer]--;	//Decrement

						//Flag the change
						Regulator_Change = true;
						TRegUpdate = t0 + (TIMESTAMP)reg_time_delay;	//Set return time
					}
				}//end tap down
				//Defaulted else - no change
			}//end phase FOR
		}//end individual
		else	//Must be banked then
		{
			//Banked will take first PT_PHASE it matches.  If there's more than one, I don't want to know
			if ((feeder_reg_config->PT_phase & PHASE_A) == PHASE_A)	//We have an A
			{
				indexer = 0;		//Index for A-based voltages
				temp_var_d = 0x01;	//A base lower "Limit" checker
				temp_var_u = 0x10;	//A base upper "Limit" checker
			}
			else if ((feeder_reg_config->PT_phase & PHASE_B) == PHASE_B)	//We have a B
			{
				indexer = 1;		//Index for B-based voltages
				temp_var_d = 0x02;	//B base lower "Limit" checker
				temp_var_u = 0x20;	//B base upper "Limit" checker
			}
			else	//Must be C then
			{
				indexer = 2;		//Index for C-based voltages
				temp_var_d = 0x04;	//C base lower "Limit" checker
				temp_var_u = 0x40;	//C base upper "Limit" checker
			}

			//Make sure we aren't below the minimum or above the maximum first
			if (((vmin[indexer] > maximum_voltage) || (VRegTo[indexer] > maximum_voltage)) && ((LimitExceed & temp_var_d) != temp_var_d))		//Above max, but can go down
			{
				//Flag us for a down tap
				prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;
			}
			else if (((vmin[indexer] < minimum_voltage) || (VRegTo[indexer] < minimum_voltage)) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below min, but can go up
			{
				//Flag us for an up tap
				prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 1;
			}
			else	//Normal operation
			{
				//See if we are in high load or low load conditions
				if (VDrop[indexer] > max_vdrop)	//High loading
				{
					//See if we're outside our range
					if (((VSet[indexer] + vbw_high) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Above deadband, but can go down too
					{
						//Check the theoretical change - make sure we won't exceed any limits
						if (feeder_regulator->tap[indexer] > 0)	//Step up region
						{
							//Find out what a step down will get us theoretically
							if ((VRegTo[indexer] - reg_step_up) < minimum_voltage)	//We'll fall below
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;  //Try to tap us down
							}
						}
						else	//Must be lower region
						{
							//Find out what a step down will get us theoretically
							if ((VRegTo[indexer] - reg_step_down) < minimum_voltage)	//We'll fall below
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;  //Try to tap us down
							}
						}
					}//end above deadband
					else if (((VSet[indexer] - vbw_high) > VRegTo[indexer]) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below deadband, but can go up
					{
						//Check the theoretical change - make sure we won't exceed any limits
						if (feeder_regulator->tap[indexer] > 0)	//Step up region
						{
							//Find out what a step up will get us theoretically
							if ((VRegTo[indexer] + reg_step_up) > maximum_voltage)	//We'll exceed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 1;  //Try to tap us up
							}
						}
						else	//Must be lower region
						{
							//Find out what a step up will get us theoretically
							if ((VRegTo[indexer] + reg_step_down) > maximum_voltage)	//We'll fall exceed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 1;  //Try to tap us up
							}
						}
					}//end below deadband
					//defaulted else - inside the deadband, so we don't care
				}//end high loading
				else	//Low loading
				{
					//See if we're outside our range
					if (((VSet[indexer] + vbw_low) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Above deadband, but can go down
					{
						//Check the theoretical change - make sure we won't exceed any limits
						if (feeder_regulator->tap[indexer] > 0)	//Step up region
						{
							//Find out what a step down will get us theoretically
							if ((VRegTo[indexer] - reg_step_up) < minimum_voltage)	//We'll fall below
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;  //Try to tap us down
							}
						}
						else	//Must be lower region
						{
							//Find out what a step down will get us theoretically
							if ((VRegTo[indexer] - reg_step_down) < minimum_voltage)	//We'll fall below
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;  //Try to tap us down
							}
						}
					}//end above deadband
					else if (((VSet[indexer] - vbw_low) > VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Below deadband, but can go up
					{
						//Check the theoretical change - make sure we won't exceed any limits
						if (feeder_regulator->tap[indexer] > 0)	//Step up region
						{
							//Find out what a step up will get us theoretically
							if ((VRegTo[indexer] + reg_step_up) > maximum_voltage)	//We'll exceed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 1;  //Try to tap us up
							}
						}
						else	//Must be lower region
						{
							//Find out what a step up will get us theoretically
							if ((VRegTo[indexer] + reg_step_down) > maximum_voltage)	//We'll fall exceed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
							}
							else	//Change allowed
							{
								prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 1;  //Try to tap us up
							}
						}
					}//end below deadband
					//defaulted else - inside the deadband, so we don't care
				}//end low loading
			}//end normal ops

			//Check on the assumption of differential banked (offsets can be present, just all move simultaneously)
			//We'll only check prop->A, since it is banked
			Regulator_Change = false;	//Start with no change assumption
			TRegUpdate = TS_NEVER;
			if (prop_tap_changes[0] > 0)	//Want a tap up
			{
				//Check individually - set to rail if they are at or exceed - this may lose the offset, but I don't know how they'd ever exceed a limit anyways
				if (feeder_regulator->tap[0] >= feeder_reg_config->raise_taps)
				{
					feeder_regulator->tap[0] = feeder_reg_config->raise_taps;	//Set at limit
					limit_hit = true;											//Flag that a limit was hit
				}

				if (feeder_regulator->tap[1] >= feeder_reg_config->raise_taps)
				{
					feeder_regulator->tap[1] = feeder_reg_config->raise_taps;	//Set at limit
					limit_hit = true;											//Flag that a limit was hit
				}

				if (feeder_regulator->tap[2] >= feeder_reg_config->raise_taps)
				{
					feeder_regulator->tap[2] = feeder_reg_config->raise_taps;	//Set at limit
					limit_hit = true;											//Flag that a limit was hit
				}

				if (limit_hit == false)	//We can still proceed
				{
					feeder_regulator->tap[0]++;	//Increment them all
					feeder_regulator->tap[1]++;
					feeder_regulator->tap[2]++;

					Regulator_Change = true;					//Flag the change
					TRegUpdate = t0 + (TIMESTAMP)reg_time_delay;	//Set return time
				}
				//Default else - limit hit, so "no change"
			}//end tap up
			else if (prop_tap_changes[0] < 0)	//Want a tap down
			{
				//Check individually - set to rail if they are at or exceed - this may lose the offset, but I don't know how they'd ever exceed a limit anyways
				if (feeder_regulator->tap[0] <= -feeder_reg_config->lower_taps)
				{
					feeder_regulator->tap[0] = -feeder_reg_config->lower_taps;	//Set at limit
					limit_hit = true;											//Flag that a limit was hit
				}

				if (feeder_regulator->tap[1] <= -feeder_reg_config->lower_taps)
				{
					feeder_regulator->tap[1] = -feeder_reg_config->lower_taps;	//Set at limit
					limit_hit = true;											//Flag that a limit was hit
				}

				if (feeder_regulator->tap[2] <= -feeder_reg_config->lower_taps)
				{
					feeder_regulator->tap[2] = -feeder_reg_config->lower_taps;	//Set at limit
					limit_hit = true;											//Flag that a limit was hit
				}

				if (limit_hit == false)	//We can still proceed
				{
					feeder_regulator->tap[0]--;	//Decrement them all
					feeder_regulator->tap[1]--;
					feeder_regulator->tap[2]--;

					Regulator_Change = true;					//Flag the change
					TRegUpdate = t0 + (TIMESTAMP)reg_time_delay;	//Set return time
				}
				//Default else - limit hit, so "no change"
			}//end tap down
			//Defaulted else - no change requested
		}//End banked mode
	}//end valid cycle

	//See how we need to proceed
	if (tret < TRegUpdate)	//Normal return is before the timer
		return tret;
	else	//Timer return is first - make this a "suggested" return
		return -TRegUpdate;
}

TIMESTAMP volt_var_control::postsync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::postsync(t0);
	complex link_power_vals;
	int index;
	bool change_requested;
	capacitor::CAPSWITCH bank_status;
	double temp_size;

	//Grab power values and all of those related calculations
	if ((((solver_method == SM_NR) && (NR_cycle == true)) || (solver_method != SM_NR)) && (control_method == ACTIVE))	//Accumulation cycle, or not NR
	{
		link_power_vals = complex(0.0,0.0);	//Zero the power

		//Pull out phases as necessary
		if ((pf_phase & PHASE_A) == PHASE_A)
			link_power_vals += substation_link->indiv_power_in[0];

		if ((pf_phase & PHASE_B) == PHASE_B)
			link_power_vals += substation_link->indiv_power_in[1];

		if ((pf_phase & PHASE_C) == PHASE_C)
			link_power_vals += substation_link->indiv_power_in[2];

		//Populate the variables of interest
		react_pwr = link_power_vals.Im();						//Pull in reactive power

		curr_pf = link_power_vals.Re()/link_power_vals.Mag();	//Pull in power factor

		//Now check to see if this is a first pass or not
		if ((curr_pf < desired_pf) && (first_cycle==true) && (TCapUpdate <= t0))
		{
			change_requested = false;	//Start out assuming no change

			//Parse through the capacitor list - see where they sit in the categories - break after one switching operation
			for (index=0; index < num_caps; index++)
			{
				//Find the phases being watched, check their switch
				if ((pCapacitor_list[index]->pt_phase & PHASE_A) == PHASE_A)
					bank_status = pCapacitor_list[index]->switchA_state;
				else if ((pCapacitor_list[index]->pt_phase & PHASE_B) == PHASE_B)
					bank_status = pCapacitor_list[index]->switchB_state;
				else	//Must be C, right?
					bank_status = pCapacitor_list[index]->switchC_state;

				//Now perform logic based on where it is
				if (bank_status == capacitor::CLOSED)	//We are on
				{
					temp_size = Capacitor_size[index] * d_min;

					if (react_pwr < temp_size)
					{
						pCapacitor_list[index]->toggle_bank_status(false);	//Turn all off
						break;	//No more loop, only one control per loop
					}
				}//end cap was on
				else	//Must be false, so we're off
				{
					temp_size = Capacitor_size[index] * d_max;

					if (react_pwr > temp_size)
					{
						pCapacitor_list[index]->toggle_bank_status(true);	//Turn all on
						break;	//No more loop, only one control per loop
					}
				}//end cap was off
			}//End capacitor list traversion

			if (change_requested == true)	//Something changed
			{
				TCapUpdate = t0 + (TIMESTAMP)cap_time_delay;	//Figure out where we wnat to go

				return t0;	//But then stay here, mainly so the capacitor change we just enacted goes through
			}
			else	//See where we want to go - no change requested, so just proceed
			{
				if (tret != TS_NEVER)
				{
					return -tret;	//Recommend a progression there
				}
				else
				{
					return tret;	//Implies tret = TS_NEVER
				}
			}//end no change requested progression
		}//end outside constraints and able to change
		else	//Still OK or intermediate period
		{
			if (TCapUpdate > t0)	//Make sure it's not a backwards time
			{
				return -TCapUpdate;	//Recommend a progression there
			}
			else	//Implies TCapUpdate is past
			{
				return tret;
			}
		}//End still OK or intermediate perio
	}//End accumulation cycle or non-NR
	else	//Non-Accumulation cycle
	{
		if (TCapUpdate > t0)	//In an update, or it is TS_NEVER
		{
			if (TCapUpdate < tret)	//Smaller than tret though (tret may be TS_NEVER)
			{
				return -TCapUpdate;	//Recommend we progress there
			}
			else
			{
				if (tret == TS_NEVER)	//This could be a TCapUpdate == tret situation
				{
					return tret;
				}
				else
				{
					return -tret;	//Implies smaller time, but not TS_NEVER
				}
			}//End TCap bigger than tret
		}//End TCap is a valid return time
		else	//TCap is in the past, so it's gone
		{
			return tret;	//Return where ever we wanted to go
		}//End TCap nevative
	}//End Non-accumulation cycle
}

// Function to sort capacitors based on size - assumes they are banked in operation (only 1 size provided)
// Utilizes merge sorting algorithm - sorts largest to smallest
void volt_var_control::size_sorter(double *cap_size, int *cap_Index, int cap_num, double *temp_cap_size, int *temp_cap_Index){	
	int split_point;
	int right_length;
	double *leftside, *rightside;
	int *leftsideidx, *rightsideidx;
	double *Final_Val;
	int *Final_Idx;

	if (num_caps>0)	//Only occurs if over zero
	{
		split_point = cap_num/2;	//Find the middle point
		right_length = cap_num - split_point;	//Figure out how big the right hand side is

		//Make the appropriate pointers
		leftside = cap_size;
		rightside = cap_size+split_point;

		//Index pointers
		leftsideidx = cap_Index;
		rightsideidx = cap_Index + split_point;

		//Check to see what condition we are in (keep splitting it)
		if (split_point>1)
			size_sorter(leftside,leftsideidx,split_point,temp_cap_size,temp_cap_Index);

		if (right_length>1)
			size_sorter(rightside,rightsideidx,right_length,temp_cap_size,temp_cap_Index);

		//Point at the first location
		Final_Val = temp_cap_size;
		Final_Idx = temp_cap_Index;

		//Merge them now
		do {
			if (*leftside > *rightside) //if (*leftside <= *rightside)
			{
				*Final_Val++ = *leftside++;
				*Final_Idx++ = *leftsideidx++;
			}
			else
			{
				*Final_Val++ = *rightside++;
				*Final_Idx++ = *rightsideidx++;
			}
		} while ((leftside<(cap_size+split_point)) && (rightside<(cap_size+cap_num)));	//Sort the list until one of them empties

		while (leftside<(cap_size+split_point))	//Put any remaining entries into the list
		{
			*Final_Val++ = *leftside++;
			*Final_Idx++ = *leftsideidx++;
		}

		while (rightside<(cap_size+cap_num))		//Put any remaining entries into the list
		{
			*Final_Val++ = *rightside++;
			*Final_Idx++ = *rightsideidx++;
		}

		memcpy(cap_size,temp_cap_size,sizeof(double)*cap_num);	//Copy the size result back into the input
		memcpy(cap_Index,temp_cap_Index,sizeof(int)*cap_num);	//Copy the index result back into the input

	}	//End length > 0
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: volt_var_control
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_volt_var_control(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(volt_var_control::oclass);
		if (*obj!=NULL)
		{
			volt_var_control *my = OBJECTDATA(*obj,volt_var_control);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("%s %s (id=%d): %s", (*obj)->name?(*obj)->name:"unnamed", (*obj)->oclass->name, (*obj)->id, msg);
		return 0;
	}
	return 1;
}

EXPORT int init_volt_var_control(OBJECT *obj, OBJECT *parent)
{
	try {
			return OBJECTDATA(obj,volt_var_control)->init(parent);
	}
	catch (const char *msg)
	{
		gl_error("%s %s (id=%d): %s", obj->name?obj->name:"unnamed", obj->oclass->name, obj->id, msg);
		return 0;
	}
	return 1;
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_volt_var_control(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	volt_var_control *pObj = OBJECTDATA(obj,volt_var_control);
	try {
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
	catch (char *msg)
	{
		gl_error("volt_var_control %s (%s:%d): %s", obj->name, obj->oclass->name, obj->id, msg);
	}
	catch (...)
	{
		gl_error("volt_var_control %s (%s:%d): unknown exception", obj->name, obj->oclass->name, obj->id);
	}
	return TS_INVALID; /* stop the clock */
}

EXPORT int isa_volt_var_control(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,volt_var_control)->isa(classname);
}

/**@}**/
