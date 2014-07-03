/** $Id: volt_var_control.cpp 4738 2014-07-03 00:55:39Z dchassin $
	
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
		oclass = gl_register_class(mod,"volt_var_control",sizeof(volt_var_control),PC_PRETOPDOWN|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		if(gl_publish_variable(oclass,
			PT_enumeration, "control_method", PADDR(control_method),PT_DESCRIPTION,"IVVC activated or in standby",
				PT_KEYWORD, "ACTIVE", (enumeration)ACTIVE,
				PT_KEYWORD, "STANDBY", (enumeration)STANDBY,
			PT_double, "capacitor_delay[s]", PADDR(cap_time_delay),PT_DESCRIPTION,"Default capacitor time delay - overridden by local defintions",
			PT_double, "regulator_delay[s]", PADDR(reg_time_delay),PT_DESCRIPTION,"Default regulator time delay - overriden by local definitions",
			PT_double, "desired_pf", PADDR(desired_pf),PT_DESCRIPTION,"Desired power-factor magnitude at the substation transformer or regulator",
			PT_double, "d_max", PADDR(d_max),PT_DESCRIPTION,"Scaling constant for capacitor switching on - typically 0.3 - 0.6",
			PT_double, "d_min", PADDR(d_min),PT_DESCRIPTION,"Scaling constant for capacitor switching off - typically 0.1 - 0.4",
			PT_object, "substation_link",PADDR(substation_lnk_obj),PT_DESCRIPTION,"Substation link, transformer, or regulator to measure power factor",
			PT_set, "pf_phase", PADDR(pf_phase),PT_DESCRIPTION,"Phase to include in power factor monitoring",
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
			PT_char1024, "regulator_list",PADDR(regulator_list),PT_DESCRIPTION,"List of voltage regulators for the system, separated by commas",
			PT_char1024, "capacitor_list",PADDR(capacitor_list),PT_DESCRIPTION,"List of controllable capacitors on the system separated by commas",
			PT_char1024, "voltage_measurements",PADDR(measurement_list),PT_DESCRIPTION,"List of voltage measurement devices, separated by commas",
			PT_char1024, "minimum_voltages",PADDR(minimum_voltage_txt),PT_DESCRIPTION,"Minimum voltages allowed for feeder, separated by commas",
			PT_char1024, "maximum_voltages",PADDR(maximum_voltage_txt),PT_DESCRIPTION,"Maximum voltages allowed for feeder, separated by commas",
			PT_char1024, "desired_voltages",PADDR(desired_voltage_txt),PT_DESCRIPTION,"Desired operating voltages for the regulators, separated by commas",
			PT_char1024, "max_vdrop",PADDR(max_vdrop_txt),PT_DESCRIPTION,"Maximum voltage drop between feeder and end measurements for each regulator, separated by commas",
			PT_char1024, "high_load_deadband",PADDR(vbw_high_txt),PT_DESCRIPTION,"High loading case voltage deadband for each regulator, separated by commas",
			PT_char1024, "low_load_deadband",PADDR(vbw_low_txt),PT_DESCRIPTION,"Low loading case voltage deadband for each regulator, separated by commas",
			PT_bool, "pf_signed",PADDR(pf_signed),PT_DESCRIPTION,"Set to true to consider the sign on the power factor.  Otherwise, it just maintains the deadband of +/-desired_pf",
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
	prev_mode = ACTIVE;			//Flag so it will go straight in by default (subject to regulator checks)

	cap_time_delay = 5.0;		//5 second default capacitor delay
	reg_time_delay = 5.0;		//5 second default regulator delay
	desired_pf = 0.98;
	d_min = 0.3;				//Chosen from Borozan paper defaults
	d_max = 0.6;				//Chosen from Borozan paper defaults
	substation_lnk_obj = NULL;
	pRegulator_list = NULL;
	pRegulator_configs = NULL;
	pCapacitor_list = NULL;
	Capacitor_size = NULL;
	pMeasurement_list = NULL;
	minimum_voltage = NULL;		
	maximum_voltage = NULL;		
	desired_voltage = NULL;		
	max_vdrop = NULL;			

	vbw_low = NULL;				
	vbw_high = NULL;

	num_regs = 1;				//Default to 1 for parsing.  Will update in init
	num_caps = 1;				//Default to 1 for parsing.  Will update in init
	
	num_meas = NULL;
	
	Regulator_Change = false;	//Start by assuming no regulator change is occurring
	TRegUpdate = NULL;
	RegUpdateTimes = NULL;
	CapUpdateTimes = NULL;
	TCapUpdate = 0;
	TUpdateStatus = false;		//Flag for control_method transitions
	pf_phase = 0;				//No phases monitored by default - will drop to link if unpopulated
	
	first_cycle = true;		//Set up the variable
	pf_signed = false;		//By default, just run a "deadband"

	prev_time = 0;

	PrevRegState = NULL;


	return result;
}

int volt_var_control::init(OBJECT *parent)
{
	int retval = powerflow_object::init(parent);

	int index, indexa;
	int64 addy_math;
	char *token_a, *token_b, *token_c, *token_a1, *token_b1, *token_c1;
	char tempchar[1024];
	char numchar[3];	//Assumes we'll never have more than 99 regulators - I hope this is valid
	OBJECT *temp_obj;
	capacitor **pCapacitor_list_temp;
	double *temp_cap_size;
	int *temp_cap_idx, *temp_cap_idx_work, *temp_meas_idx;
	double cap_adder, temp_double, nom_volt, default_min, default_max, default_des, default_max_vdrop, default_vbw_low, default_vbw_high;
	set temp_phase;
	int num_min_volt, num_max_volt, num_des_volt, num_max_vdrop, num_vbw_low, num_vbw_high, total_meas;
	bool reg_list_type;

	OBJECT *obj = OBJECTHDR(this);

	//General error checks
	if (pf_signed==true)
	{
		if ((d_max <= 0.0) || (d_max > 1.0))
		{
			GL_THROW("volt_var_control %s: d_max must be a number between 0 and 1",obj->name);
			/*  TROUBLESHOOT
			The capacitor threshold value d_max represents a fraction of a capacitor's total kVA rating.
			It must be specified greater than 0, but less than or equal to 1.
			*/
		}

		//Check power factor range
		if ((desired_pf <= -1) || (desired_pf > 1))
		{
			GL_THROW("volt_var_control %s: Desired signed power factor is outside the valid range",obj->name);
			/*  TROUBLESHOOT
			With a signed power factor enabled, the desired_pf must be between -1 and 1.  Valid ranges are
			greater than -1 and less than or equal to 1 (-1 is not accepted, use 1.0 as unity power factor).
			*/
		}
	}
	else	//"Traditional" algorithm
	{
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

		//Check power factor
		if (desired_pf < 0)	//Negative - see if they meant the other mode
		{
			GL_THROW("volt_var_control %s: Desired power factor is negative.  Should pf_signed be set?",obj->name);
			/*  TROUBLESHOOT
			A negative power factor was entered in the "deadband" operation mode of the controller.  If a signed
			power factor value was desired, set the pf_signed paramter to true to proceed.  Otherwise, correct the
			desired power factor.
			*/
		}
		else if (desired_pf > 1)	//Greater than 1, invalid
		{
			GL_THROW("volt_var_control %s: Desired power factor is outside the valid range",obj->name);
			/*  TROUBLESHOOT
			With a normal power factor enabled, the desired_pf must be between 0 and 1.  Valid ranges are
			greater than or equal to 0 and less than or equal to 1.
			*/
		}
	}//End "traditional" implementation

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

	//Figure out number of minimum voltages specified
	index=0;
	num_min_volt = 1;
	while ((minimum_voltage_txt[index] != '\0') && (index < 1024))
	{
		if (minimum_voltage_txt[index] == ',')	//Comma
			num_min_volt++;					//increment the number of min voltages

		index++;	//increment the pointer
	}

	//See if one is really there
	if ((num_min_volt == 1) && (minimum_voltage_txt[0] == '\0'))	//Is empty :(
	{
		num_min_volt = 0;	//Primarily as a flag
	}
	
	//Figure out number of maximum voltages specified
	index=0;
	num_max_volt = 1;
	while ((maximum_voltage_txt[index] != '\0') && (index < 1024))
	{
		if (maximum_voltage_txt[index] == ',')	//Comma
			num_max_volt++;					//increment the number of max voltages

		index++;	//increment the pointer
	}

	//See if one is really there
	if ((num_max_volt == 1) && (maximum_voltage_txt[0] == '\0'))	//Is empty :(
	{
		num_max_volt = 0;	//Primarily as a flag
	}
	
	//Figure out number of desired voltages specified
	index=0;
	num_des_volt=1;
	while ((desired_voltage_txt[index] != '\0') && (index < 1024))
	{
		if (desired_voltage_txt[index] == ',')	//Comma
			num_des_volt++;					//increment the number of desired voltages

		index++;	//increment the pointer
	}

	//See if one is really there
	if ((num_des_volt == 1) && (desired_voltage_txt[0] == '\0'))	//Is empty :(
	{
		num_des_volt = 0;	//Primarily as a flag
	}

	//Figure out number of vdrops specified
	index=0;
	num_max_vdrop=1;
	while ((max_vdrop_txt[index] != '\0') && (index < 1024))
	{
		if (max_vdrop_txt[index] == ',')	//Comma
			num_max_vdrop++;					//increment the number of max voltage drops

		index++;	//increment the pointer
	}

	//See if one is really there
	if ((num_max_vdrop == 1) && (max_vdrop_txt[0] == '\0'))	//Is empty :(
	{
		num_max_vdrop = 0;	//Primarily as a flag
	}

	//Figure out number of low load bandwidths
	index=0;
	num_vbw_low=1;
	while ((vbw_low_txt[index] != '\0') && (index < 1024))
	{
		if (vbw_low_txt[index] == ',')	//Comma
			num_vbw_low++;					//increment the number of low bandwidths

		index++;	//increment the pointer
	}

	//See if one is really there
	if ((num_vbw_low == 1) && (vbw_low_txt[0] == '\0'))	//Is empty :(
	{
		num_vbw_low = 0;	//Primarily as a flag
	}

	//Figure out number of high load bandwidths
	index=0;
	num_vbw_high=1;
	while ((vbw_high_txt[index] != '\0') && (index < 1024))
	{
		if (vbw_high_txt[index] == ',')	//Comma
			num_vbw_high++;					//increment the number of high bandwidths

		index++;	//increment the pointer
	}

	//See if one is really there
	if ((num_vbw_high == 1) && (vbw_high_txt[0] == '\0'))	//Is empty :(
	{
		num_vbw_high = 0;	//Primarily as a flag
	}

	//Figure out the number of regulators we have
	index=0;
	while ((regulator_list[index] != '\0') && (index < 1024))
	{
		if (regulator_list[index] == ',')	//Found a comma!
			num_regs++;						//Increment the number of regulators present
		
		index++;	//Increment the pointer
	}

	if (num_regs==1)	//Only 1 column, make sure something is actually in there
	{
		temp_obj = gl_get_object((char *)regulator_list);

		if (temp_obj == NULL)	//Not really an object, must be no controllable capacitors
			num_regs = 0;
	}

	if (num_regs > 0)
	{
		//Allocate the relevant variables - lots of them - single allocs are probably pretty silly, but meh
			pRegulator_list = (regulator **)gl_malloc(num_regs*sizeof(regulator*));
			
			if (pRegulator_list == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				/*  TROUBLESHOOT
				While attempting to allocate one of the storage vectors for the regulators,
				an error was encountered.  Please try again.  If the error persists, please
				submit your code and a bug report via the trac website.
				*/
			}

			pRegulator_configs = (regulator_configuration **)gl_malloc(num_regs*sizeof(regulator_configuration*));

			if (pRegulator_configs == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			PrevRegState = (regulator_configuration::Control_enum*)gl_malloc(num_regs*sizeof(regulator_configuration::Control_enum));

			if (PrevRegState == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			//Allocate measurement pointers
			pMeasurement_list = (node***)gl_malloc(num_regs*sizeof(node**));
			if (pMeasurement_list == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			num_meas = (int*)gl_malloc(num_regs*sizeof(int));
			if (num_meas == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			TRegUpdate = (TIMESTAMP *)gl_malloc(num_regs*sizeof(TIMESTAMP));

			if (TRegUpdate == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			//Init this one right now
			for (index=0; index<num_regs; index++)
			{
				TRegUpdate[index] = TS_NEVER;
			}

			RegUpdateTimes = (double*)gl_malloc(num_regs*sizeof(double));

			if (RegUpdateTimes == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			reg_step_up = (double*)gl_malloc(num_regs*sizeof(double));

			if (reg_step_up == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			reg_step_down = (double*)gl_malloc(num_regs*sizeof(double));
			
			if (reg_step_down == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			RegToNodes = (node**)gl_malloc(num_regs*sizeof(node*));

			if (RegToNodes == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			vbw_high = (double*)gl_malloc(num_regs*sizeof(double));

			if (vbw_high == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			vbw_low = (double*)gl_malloc(num_regs*sizeof(double));

			if (vbw_low == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			max_vdrop = (double*)gl_malloc(num_regs*sizeof(double));

			if (max_vdrop == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			desired_voltage = (double*)gl_malloc(num_regs*sizeof(double));

			if (desired_voltage == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			maximum_voltage = (double*)gl_malloc(num_regs*sizeof(double));

			if (maximum_voltage == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

			minimum_voltage = (double*)gl_malloc(num_regs*sizeof(double));

			if (minimum_voltage == NULL)
			{
				GL_THROW("volt_var_control %s: regulator storage allocation failed",obj->name);
				//defined above
			}

		//Now populate each of these massive arrays
			//start with the regulators, their configurations, and related items
			token_a = regulator_list;
			for (index=0; index<num_regs; index++)
			{
				//Extract the object information
				token_a1 = obj_token(token_a, &temp_obj);
				
				if (temp_obj != NULL)	//Valid object!
				{
					pRegulator_list[index] = OBJECTDATA(temp_obj, regulator);	//Store the regulator

					pRegulator_configs[index] = OBJECTDATA(pRegulator_list[index]->configuration,regulator_configuration);	//Store the configuration

					RegToNodes[index] = OBJECTDATA(pRegulator_list[index]->to,node);	//Store the to node

					if (pRegulator_configs[index]->time_delay != 0)	//Delay specified
					{
						RegUpdateTimes[index] = pRegulator_configs[index]->time_delay;	//use this delay
					}
					else
					{
						RegUpdateTimes[index] = reg_time_delay;	//Use the default
					}

					reg_step_up[index] = pRegulator_configs[index]->band_center * pRegulator_configs[index]->regulation / ((double) pRegulator_configs[index]->raise_taps);	//Store upstep value

					reg_step_down[index] = pRegulator_configs[index]->band_center * pRegulator_configs[index]->regulation / ((double) pRegulator_configs[index]->lower_taps);	//Store downstep value

				}//end valid object 
				else	//General catch
				{
					if (token_a1 != NULL)
					{
						//Remove the comma from the list
						*--token_a1 = '\0';
					}
					//Else is the end of list, so it should already be \0-ed

					GL_THROW("volt_var_control %s: item %s in regulator_list not found",obj->name,token_a);
					/*  TROUBLESHOOT
					While attempting to populate the regulator list, an invalid object was found.  Please check
					the object name.
					*/
				}

				//Update the pointer
				token_a = token_a1;
			}//end token FOR - populate regulators, configurations, to nodes, and updates

			//Now parse out individual lists - see how they are set
				//General catch of all voltages
				if ((num_des_volt != num_min_volt) || (num_min_volt != num_max_volt))
				{
					gl_warning("volt_var_control %s: Desired, minimum, and maximum voltages don't match",obj->name);
					/*  TROUBLESHOOT
					The lists for desired_voltages, minimum_voltages, and maximum_voltages should all have the same
					number of elements.  If they don't, if at each field has at least one specification, the first for
					each will be used on all regulators.  If they don't all have at least one specification, the whole set
					will be set to default values.
					*/

					//Not equal, see if they all have at least one
					if ((num_des_volt > 0) && (num_min_volt > 0) && (num_max_volt > 0))
					{
						//Set all to 1 as a flag
						num_des_volt = num_min_volt = num_max_volt = 1;
					}
					else	//At least one must be 0, so they all get defaulted
					{
						num_des_volt = num_min_volt = num_max_volt = 0;
					}
				}//end list element mismatch

				if (num_des_volt > 1)	//One for each regulator
				{
					//Point to list
					token_a = minimum_voltage_txt;
					token_b = maximum_voltage_txt;
					token_c = desired_voltage_txt;

					//Initialize output pointers - just so compiler shuts up
					token_a1 = token_a;
					token_b1 = token_b;
					token_c1 = token_c;
				}
				else if (num_des_volt == 1)	//1 value for all
				{
					//Point to list
					token_a = minimum_voltage_txt;
					token_b = maximum_voltage_txt;
					token_c = desired_voltage_txt;

					//Initialize output pointers - just so compiler shuts up
					token_a1 = token_a;
					token_b1 = token_b;
					token_c1 = token_c;

					//Parse them out
					token_a1 = dbl_token(token_a,&default_min);
					token_b1 = dbl_token(token_b,&default_max);
					token_c1 = dbl_token(token_c,&default_des);
				}
				//Default else - calculated based off of nominal voltage

				//Extract all set point values
				for (index=0; index<num_regs; index++)
				{
					if (num_des_volt == num_regs)	//One for each regulator
					{
						//Convert them into numbers
						token_a1 = dbl_token(token_a,&minimum_voltage[index]);
						token_b1 = dbl_token(token_b,&maximum_voltage[index]);
						token_c1 = dbl_token(token_c,&desired_voltage[index]);

						//Progress
						token_a = token_a1;
						token_b = token_b1;
						token_c = token_c1;

					}//one for each regulator
					else if (num_des_volt == 1)		//Use one value for all
					{
						//Convert them into numbers
						minimum_voltage[index] = default_min;
						maximum_voltage[index] = default_max;
						desired_voltage[index] = default_des;
					}//one value for all
					else							//Use defaults on all
					{
						//If default, we need the to node nominal voltage
						nom_volt = RegToNodes[index]->nominal_voltage;

						minimum_voltage[index] = 0.95*nom_volt;
						maximum_voltage[index] = 1.05*nom_volt;
						desired_voltage[index] = nom_volt;
					}

					//Make sure voltages are ok
					if (minimum_voltage[index] >= maximum_voltage[index])
					{
						GL_THROW("volt_var_control %s: Minimum voltage limit on regulator %d must be less than the maximum voltage limit of the system",obj->name,(index+1));
						/*  TROUBLESHOOT
						The minimum_voltage value must be less than the maximum_voltage value to ensure proper system operation.
						*/
					}

					//Make sure desired voltage is within operating limits
					if ((desired_voltage[index] < minimum_voltage[index]) || (desired_voltage[index] > maximum_voltage[index]))
					{
						GL_THROW("volt_var_control %s: Desired voltage on regulator %d is outside the minimum and maximum set points.",obj->name,(index+1));
						/*  TROUBLESHOOT
						The desired_voltage property is specified in the range outside of the minimum_voltage and maximum_voltage
						set points.  It needs to lie between these points for proper operation.  Please adjust the values, or leave
						desired_voltage blank to let the system decide.
						*/
					}
				}//end regulator traversion for min/max/des voltages

				//Now figure out vbw_low, vbw_high, and max_vdrop
				if ((num_max_vdrop != num_vbw_low) || (num_vbw_low != num_vbw_high))
				{
					gl_warning("volt_var_control %s: VDrop, low V BW, and high V BW don't match",obj->name);
					/*  TROUBLESHOOT
					The lists for maximum voltage drop, voltage drop low loading, and voltage drop high loading
					should all have the same number of elements.  If they don't, if at each field has at least one specification, the first for
					each will be used on all regulators.  If they don't all have at least one specification, the whole set
					will be set to default values.
					*/

					//Not equal, see if they all have at least one
					if ((num_max_vdrop > 0) && (num_vbw_low > 0) && (num_vbw_high > 0))
					{
						//Set all to 1 as a flag
						num_max_vdrop = num_vbw_low = num_vbw_high = 1;
					}
					else	//At least one must be 0, so they all get defaulted
					{
						num_max_vdrop = num_vbw_low = num_vbw_high = 0;
					}
				}//end list element mismatch

				if (num_max_vdrop > 1)	//One for each regulator - loop precursor (point the tokens)
				{
					//Point to list
					token_a = max_vdrop_txt;
					token_b = vbw_low_txt;
					token_c = vbw_high_txt;

					//Initialize output pointers - just so compiler shuts up
					token_a1 = token_a;
					token_b1 = token_b;
					token_c1 = token_c;
				}
				else if (num_max_vdrop == 1)	//One for all regulators
				{
					//Point to list
					token_a = max_vdrop_txt;
					token_b = vbw_low_txt;
					token_c = vbw_high_txt;

					//Initialize output pointers - just so compiler shuts up
					token_a1 = token_a;
					token_b1 = token_b;
					token_c1 = token_c;

					//Extract the values
					token_a1 = dbl_token(token_a,&default_max_vdrop);
					token_b1 = dbl_token(token_b,&default_vbw_low);
					token_c1 = dbl_token(token_c,&default_vbw_high);
				}
				//Defaulted else - calculate

				//Extract all set point values
				for (index=0; index<num_regs; index++)
				{
					if (num_max_vdrop == num_regs)	//One for each regulator
					{
						//Convert them into numbers
						token_a1 = dbl_token(token_a,&max_vdrop[index]);
						token_b1 = dbl_token(token_b,&vbw_low[index]);
						token_c1 = dbl_token(token_c,&vbw_high[index]);

						//Grab next token
						token_a = token_a1;
						token_b = token_b1;
						token_c = token_c1;
					}//one for each regulator
					else if (num_max_vdrop == 1)		//Use one value for all
					{
						//Store values
						max_vdrop[index] = default_max_vdrop;
						vbw_low[index] = default_vbw_low;
						vbw_high[index] = default_vbw_high;
					}//one value for all
					else							//Use defaults on all
					{
						max_vdrop[index] = 1.5*reg_step_up[index];	//Arbitrarily set to 1.5x a step change

						//Set low bandwidth to 2 tap positions out of whack
						vbw_low[index] = 2.0*reg_step_up[index];

						//Set high bandwidth to 1 tap positions out of whack (more constrained than vbw_low)
						vbw_high[index] = reg_step_up[index];

					}//end defaults

					//Check to make sure max_vdrop isn't negative
					if (max_vdrop <= 0)
					{
						GL_THROW("volt_var_control %s: Maximum expected voltage drop specified for regulator %d should be a positive non-zero number.",obj->name,(index+1));
						/*  TROUBLESHOOT
						The max_vdrop property must be greater than 0 for proper operation of the coordinated Volt-VAr control scheme.
						*/
					}

					//Make sure bandwidth values aren't negative
					if (vbw_low[index] <= 0)
					{
						GL_THROW("volt_var_control %s: Low loading deadband (bandwidth) for regulator %d  must be a positive non-zero number",obj->name,(index+1));
						/*  TROUBLESHOOT
						The low_load_deadband setting must be greater than 0 for proper operation of the Volt-VAr controller.
						*/
					}

					if (vbw_high[index] <= 0)
					{
						GL_THROW("volt_var_control %s: High loading deadband (bandwidth) for regulator %d must be a positive non-zero number",obj->name,(index+1));
						/*  TROUBLESHOOT
						The high_load_deadband setting must be greater than 0 for proper operation of the Volt-VAr controller.
						*/
					}

					//vbw_high is supposed to be less than vbw_low (heavy loading means we should be more constrained)
					if (vbw_high[index] > vbw_low[index])
					{
						gl_warning("volt_var_control %s: Low loading deadband for regulator %d is expected to be larger than the high loading deadband",obj->name,(index+1));
						/*  TROUBLESHOOT
						The high_load_deadband is larger than the low_load_deadband setting.  The algorithm proposed in the referenced paper
						suggests low_load_deadband should be larger (less constrained) than the high load.  This may need to be fixed to be
						consistent with expected results.
						*/
					}
				}//Feeder traversion for vdrop, vbw's

	}//End num_regs > 0
	else	//no regulators found
	{
		GL_THROW("volt_var_control %s: regulator_list is empty",obj->name);
		/*  TROUBLESHOOT
		The volt_var_control object requires at one regulator regulator to control for coordinated action.  Please specify
		an appropriate regulator on the system.
		*/
	}

	//See if any regulator is in manual mode, if we are on right now
	if (control_method == ACTIVE)
	{
		for (index=0; index<num_regs; index++)	//See if any are in manual
		{
			if (pRegulator_configs[index]->Control != regulator_configuration::MANUAL)	//We're on, but regulator not in manual.  Set up as a transition
			{
				prev_mode = STANDBY;	
				break;		//Only takes one
			}
		}
	}
	else	//We're in standby, so we don't care what the regulator is doing right now
	{
		prev_mode = STANDBY;
	}

	if (substation_lnk_obj == NULL)	//If nothing specified, just use the first feeder regulator
	{
		gl_warning("volt_var_control %s: A link to monitor power-factor was not specified, using first regulator.",obj->name);
		/*  TROUBLESHOOT
		The volt_var_control object requires a link-based object to measure power factor.  Since one was not specified, the control
		scheme will just monitor the power values of the first feeder regulator.
		*/

		//Assigns regulator to the link object
		substation_link = pRegulator_list[index];

		if (solver_method == SM_FBS)
		{
			//populate the lnk_obj as well if FBS (we'll need it for parenting)
			substation_lnk_obj = OBJECTHDR(pRegulator_list[index]);
		}
	}
	else	//It is defined
	{
		//Link it up
		substation_link = OBJECTDATA(substation_lnk_obj,link_object);
	}

	//Determine how many capacitors we have to play with
	index=0;
	while ((capacitor_list[index] != '\0') && (index < 1024))
	{
		if (capacitor_list[index] == ',')	//Found a comma!
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
			pCapacitor_list = (capacitor **)gl_malloc(sizeof(capacitor*));

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

			//malloc the update time vector (there is only 1 again, but consistency is key)
			CapUpdateTimes = (double *)gl_malloc(sizeof(double));

			if (CapUpdateTimes == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//Parse the list now - the list is the capacitor
			token_a = capacitor_list;

			temp_obj = gl_get_object(token_a);
				
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

				//Check on the time delays - use default if unspecified
				if (pCapacitor_list[0]->time_delay != 0)
				{
					CapUpdateTimes[0] = pCapacitor_list[0]->time_delay;	//Store it
				}
				else
				{
					CapUpdateTimes[0] = cap_time_delay;		//Use the default
				}
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
			pCapacitor_list_temp = (capacitor **)gl_malloc(num_caps*sizeof(capacitor*));

			if (pCapacitor_list_temp == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//malloc the actual list
			pCapacitor_list = (capacitor **)gl_malloc(num_caps*sizeof(capacitor*));

			if (pCapacitor_list == NULL)
			{
				GL_THROW("volt_var_control %s: Failed to allocate capacitor memory",obj->name);
				//Defined above
			}

			//Malloc the update times
			CapUpdateTimes = (double *)gl_malloc(num_caps*sizeof(double));

			if (CapUpdateTimes == NULL)
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
			token_a = capacitor_list;
			for (index = 0; index < num_caps; index++)
			{
				//Extract the object information
				token_a1 = obj_token(token_a, &temp_obj);
				
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

					//Check on the time delays - use default if unspecified
					if (pCapacitor_list_temp[index]->time_delay != 0)
					{
						CapUpdateTimes[index] = pCapacitor_list_temp[index]->time_delay;	//Store it
					}
					else
					{
						CapUpdateTimes[index] = cap_time_delay;		//Use the default
					}
				}
				else	//General catch
				{
					if (token_a1 != NULL)
					{
						//Remove the comma from the list
						*--token_a1 = '\0';
					}
					//Else is the end of list, so it should already be \0-ed

					GL_THROW("volt_var_control %s: item %s in capacitor_list not found",obj->name,token_a);
					/*  TROUBLESHOOT
					While attempting to populate the capacitor list, an invalid object was found.  Please check
					the object name.
					*/
				}

				//Update the pointer
				token_a = token_a1;
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
	else	//Implies no capacitors
	{
		gl_warning("volt_var_control %s: No capacitors specified.",obj->name);
		/*  TROUBLESHOOT
		The capacitor_list in the volt_var_control is empty.  This means no
		capacitors are controlled by the volt_var_control object, which more or
		less means you have voltage setpoint regulators and no need for a central
		control.
		*/
	}

	//Now figure out the measurements
	//Initialize measurement count list
	for (index=0; index<num_regs; index++)
	{
		num_meas[index] = 0;
	}

	//Determine how many points are present
	index=0;
	total_meas=1;
	while ((measurement_list[index] != '\0') && (index < 1024))
	{
		if (measurement_list[index] == ',')	//Found a comma!
			total_meas++;						//Increment the number of measurements present
		
		index++;	//Increment the pointer
	}

	//Only 1 measurement found - figure out where it needs to go - or toss an error
	if (total_meas == 1)
	{
		//See if it is an actual list
		if (measurement_list[0] != 0)
		{
			if (num_regs != 1)	//More than one regulator, so this is unacceptable
			{
				GL_THROW("volt_var_control %s: A measurement is missing a regulator association",obj->name);
				//Defined below
			}
			//Defaulted else - 1 regulator, this will be checked below
		}//End the list is valid
		else	//Empty list, so we really don't have any measurements
		{
			total_meas = 0;
		}
	}//end 1 measurement

	//Determine how to proceed, are we referenced or not
	if ((num_regs == 1) && (total_meas > 1))	//1 regulator and more than 1 point found
	{
		//initialize the character array
		for (index=0; index<1024; index++)
			tempchar[index] = 0;

		//Find the first comma
		token_b = measurement_list;
		token_b1 = strchr(token_b,',');

		//Find the second comma
		token_b1++;						//Get us one past the comma
		token_a = strchr(token_b1,',');	//Find the next one

		//Reference the storage array
		token_b = tempchar;

		if (token_a == NULL)	//Only two items in the list
		{
			//Copy in the value
			while (*token_b1 != '\0')
			{
				*token_b++ = *token_b1++;
			}
		}
		else	//More to come, but we don't care
		{
			//Copy the contents in
			while (token_b1 < token_a)
			{
				*token_b++ = *token_b1++;	
			}
		}//End more than 2 items

		token_b1 = tempchar;

		//See if #2 is an object or a number
		temp_obj = gl_get_object(token_b1);

		if (temp_obj != NULL)	//It's real, so we are just an object list
		{
			reg_list_type=false;	//Arbitrary flagging 
		}
		else	//It's a number, so proceed like any other regulator
		{
			reg_list_type=true;		//Catch us
		}
	}
	else if ((num_regs == 1) && (total_meas == 1))
	{
		//1 reg and 1 measurement, make sure it is a valid object before proceeding
		token_a = measurement_list;	//Get first item (valid)

		//Extract the object information
		token_a1 = obj_token(token_a, &temp_obj);
		
		if (temp_obj == NULL)	//Not a valid object, error!
		{
			GL_THROW("volt_var_control %s: Measurement %s is not a valid node!",obj->name,token_a);
			/*  TROUBLESHOOT
			While parsing the measurement list, an invalid object was found.  Please verify the object
			name and try again.
			*/
		}

		reg_list_type = false;	//Flag as special - single measurement point for a single regulator
	}
	//Else implies no measurements and/or num_regs > 1 - those will be caught separately below

	//Make sure we found an even number if there is more than one regulator attached - otherwise it is mismatched
	if ((num_regs > 1) || (reg_list_type == true))	//more than one regulator, or we're a standard list
	{
		index = total_meas >> 1;
		temp_double = total_meas - ((double)(index << 1));

		if (temp_double != 0)	//Should be no remainder
		{
			GL_THROW("volt_var_control %s: A measurement is missing a regulator association",obj->name);
			/*  TROUBLESHOOT
			If more than one regulator is present, the measurement_list must be specified as a comma-delimited
			pair with the measurement node and the integer of the regulator it is associated with (e.g., 709,2 associates
			node 709 with regulator 2 of regulator_list).
			*/
		}

		total_meas = (total_meas >> 1);	//Assign in so we can use temp_double again

		if (total_meas != 0)	//There's at least something in there
		{
			//List is assumed valid, now figure out how big everything needs to be
			token_b1 = measurement_list;	//Start the list

			for (index=0; index<total_meas; index++)
			{
				//Find the first comma
				token_a = strchr(token_b1,',');

				//Find the second comma
				token_a++;						//Get us one past the comma
				token_b = strchr(token_a,',');	//Find the next one

				//Zero the character array
				numchar[0] = numchar[1] = numchar[2] = 0;

				//Reference the storage array
				token_c = numchar;

				//Make sure we aren't over two digits long - if we need more than 99 regulators, I don't want to hear about it
				addy_math = token_b - token_a;

				if (addy_math > 2)	//Too many characters
				{
					GL_THROW("volt_var_control %s: Measurement list item pair %d is invalid.",obj->name,(index+1));
					/*  TROUBLESHOOT
					While parsing the measurement list, an invalid pair was encountered.  This may be due to a typo, or
					there may not be a proper regulator specification.  if more than one regulator is being controlled,
					the list must be specified as "node,reg" pairs.
					*/
				}

				if (index == (total_meas-1))	//Last item of the list
				{
					//Copy in the value
					while (*token_a != '\0')
					{
						*token_c++ = *token_a++;
					}
				}
				else	//More to come, but we don't care
				{
					//Copy the contents in
					while (token_a < token_b)
					{
						*token_c++ = *token_a++;	
					}
				}//End normal

				token_b1 = numchar;	//Reference the temporary array

				indexa = ((int)(strtod(token_b1,NULL))-1);	//Convert back

				if ((indexa <0) || (indexa > (num_regs-1)))		//Pre-offset for C indexing
				{
					GL_THROW("volt_var_control %s: Measurement list references a nonexistant regulator %d",obj->name,(indexa+1));
					/*  TROUBLESHOOT
					While parsing the measurement list, an invalid regulator reference number was encountered.  These numbers should
					match the position of the regulator in the regulator_list variable.  Please double check your values and try again.
					*/
				}

				num_meas[indexa]++;		//Increment the counter
				token_b1 = ++token_b;		//Set up for the next item
			}
		}//End at least one measurement total (feeders checked individually next)
		//Else is no measurements specified, this will be handled in the general catch below

		//Check to see if any individual regulator has no measurment points
		for (index=0; index<num_regs; index++)
		{
			if (num_meas[index] == 0)
				num_meas[index] = -1;	//Default case - gets one
		}

		//Allocate working index
		temp_meas_idx = (int*)gl_malloc(num_regs*sizeof(int));
		if (temp_meas_idx == NULL)
		{
			GL_THROW("volt_var_control %s: measurement list allocation failure",obj->name);
			//Defined below
		}

		//Now time for mallocs
		for (index=0; index<num_regs; index++)
		{
			if (num_meas[index] == -1)	//Default flag - only 1 item
			{
				pMeasurement_list[index] = (node**)gl_malloc(sizeof(node*));
			}
			else	//There was at least one
			{
				pMeasurement_list[index] = (node**)gl_malloc(num_meas[index]*sizeof(node*));
			}
			
			if (pMeasurement_list[index] == NULL)
			{
				GL_THROW("volt_var_control %s: measurement list allocation failure",obj->name);
				/*  TROUBLESHOOT
				While attempting to allocate space for the measurment list, a memory allocation error occurred.
				Please try again.  If the problem persists, please submit your code and a bug report via the trac website.
				*/
			}

			//Zero the accumulator while we're at it
			temp_meas_idx[index] = 0;
		}

		//Now populate the list
		token_a = measurement_list;	//Start the list
		for (index=0; index<total_meas; index++)
		{
			//First item is the object, grab it
			token_a1 = obj_token(token_a, &temp_obj);
			
			//Make sure it is valid
			if (temp_obj == NULL)
			{
				if (token_a1 != NULL)
				{
					//Remove the comma from the list
					*--token_a1 = '\0';
				}
				//Else is the end of list, so it should already be \0-ed

				GL_THROW("volt_var_control %s: measurement object %s was not found!",obj->name,token_a);
				/*  TROUBLESHOOT
				While parsing the measurment list for the volt_var_control object, a measurement point
				was not found.  Please check the name and try again.  If the problem persists, please submit 
				your code and a bug report via the trac website.
				*/
			}
			
			//Update the pointer
			token_a = token_a1;

			token_a1 = dbl_token(token_a,&temp_double);	//Pull next - starts on obj, we want to do a count

			token_a = token_a1;	//Update the pointer

			indexa = ((int)temp_double)-1;	//Figure out the index

			pMeasurement_list[indexa][temp_meas_idx[indexa]] = OBJECTDATA(temp_obj,node);	//Store it in the list

			temp_meas_idx[indexa]++;	//Increment the storage pointer
		}

		//Free up our temporary memory use
		gl_free(temp_meas_idx);

		//Now loop through again - find any "zero" lists and populate them
		for (index=0; index<num_regs; index++)
		{
			if (num_meas[index] == -1)	//This was a default case
			{
				pMeasurement_list[index][0] = RegToNodes[index];	//Default is just use the load side

				gl_warning("volt_var_control %s: No measurement point specified for reg %d - defaulting to load side of regulator",obj->name,(index+1));
				/*  TROUBLESHOOT
				If no measuremnt points are specified for a regualtor in the volt_var_control object, it will default
				to the load side of the regulator.  Please specify measurement points for better control.
				*/

				num_meas[index] = 1;	//Update the number to be "normal"
			}
		}//end second pass of regulator list
	}//End more than one regulator, or dual listed
	else	//1 regulator - dualed list is not necessary
	{
		if (total_meas != 0)	//There's at least something in there
		{
			//Allocate space
			pMeasurement_list[0] = (node**)gl_malloc(total_meas*sizeof(node*));

			if (pMeasurement_list[0] == NULL)
			{
				GL_THROW("volt_var_control %s: measurement list allocation failure",obj->name);
				//Defined above
			}

			//Use a temporary index
			indexa=0;

			//Now populate the list
			token_a = measurement_list;	//Start the list
			for (index=0; index<total_meas; index++)
			{
				//First item is the object, grab it
				token_a1 = obj_token(token_a, &temp_obj);
				
				//Make sure it is valid
				if (temp_obj == NULL)
				{
					if (token_a1 != NULL)
					{
						//Remove the comma from the list
						*--token_a1 = '\0';
					}
					//Else is the end of list, so it should already be \0-ed

					GL_THROW("volt_var_control %s: measurement object %s was not found!",obj->name,token_a);
					/*  TROUBLESHOOT
					While parsing the measurment list for the volt_var_control object, a measurement point
					was not found.  Please check the name and try again.  If the problem persists, please submit 
					your code and a bug report via the trac website.
					*/
				}

				//Update the pointer - no numbers here, so this goes to next object
				token_a = token_a1;

				pMeasurement_list[0][indexa] = OBJECTDATA(temp_obj,node);	//Store it in the list

				indexa++;	//Increment the storage pointer
			}//End measurement list

			//Assign in our value to num_meas
			num_meas[0] = total_meas;
		}//End at least one measurement total
		else	//Empty, so default for us!
		{
			//Allocate space
			pMeasurement_list[0] = (node**)gl_malloc(sizeof(node*));

			if (pMeasurement_list[0] == NULL)
			{
				GL_THROW("volt_var_control %s: measurement list allocation failure",obj->name);
				//Defined above
			}

			//Put the to side of the regulator in there
			pMeasurement_list[0][0] = RegToNodes[0];

			//Set the measurement list
			num_meas[0] = 1;
		}//end default
	}//end 1 regulator, not dual listed

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
	else if (solver_method == SM_FBS)	//FBS - need to be below substation_link for power/voltage calculations.  Regs shouldn't be issue
	{
		//Parent us to the substation link to make sure our ranking is happy - do this explicitly.
		//If we already have a parent, over-write it - need to be below voltage updates and power calculations of substation_link
		index = gl_set_parent(obj,substation_lnk_obj);

		if (index < 0)	//Make sure it worked
		{
			GL_THROW("Error setting parent");
			/*  TROUBLESHOOT
			An error has occurred while setting the parent field of the volt_var_control.  Please
			submit a bug report and your code so this error can be diagnosed further.
			*/
		}

		//Rank us 1 below the parent (same as substation link to end)
		gl_set_rank(obj,substation_link->to->rank);
	}
	else	//Any future solvers and GS - GS doesn't play nice with regulators anyways
	{
		GL_THROW("Solver methods other than NR and FBS are unsupported at this time.");
		/*  TROUBLESHOOT
		The volt_var_control object only supports the Newton-Raphson and Forward-Back Sweep
		solvers at this time.  Other solvers (Gauss-Seidel) may be implemented at a future date.
		*/
	}

	return retval;
}

TIMESTAMP volt_var_control::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::presync(t0);
	TIMESTAMP treg_min;
	double vmin[3], VDrop[3], VSet[3], VRegTo[3];
	int indexer, index, reg_index;
	char temp_var_u, temp_var_d;
	int prop_tap_changes[3];
	bool limit_hit = false;	//mainly for banked operations
	char LimitExceed = 0x00;	// U_D - XCBA_XCBA

	//Start out assuming a regulator change hasn't occurred
	Regulator_Change = false;

	//Now loop through the list - if one is over t0, then it is still changing
	for (index=0; index<num_regs; index++)
	{
		if ((t0 < TRegUpdate[index]) && (TRegUpdate[index] != TS_NEVER))
		{
			Regulator_Change = true;	//Flag a regulator change status
			break;						//Only takes 1 true to be done
		}
	}

	//Secondary check - see if we're "recovering" from a transition
	if (TUpdateStatus == true)
	{
		//Old logic checked for TRegUpdate <= t0 as well - if we have TUpdateStatus true, all times should be t0 anyways
		TUpdateStatus = false;			//Clear the flag

		for (index=0; index<num_regs; index++)
		{
			TRegUpdate[index] = TS_NEVER;	//Let us proceed
		}
	}

	if (t0 != prev_time)	//New timestep
	{
		first_cycle = true;		//Flag as first entrant this time step (used to reactive control)
		prev_time = t0;			//Update tracking variable
	}
	else
	{
		first_cycle = false;	//No longer the first cycle
	}

	if (control_method != prev_mode)	//We've altered our mode
	{
		if (control_method == ACTIVE)	//We're active, implies were in standby before
		{
			for (indexer=0; indexer < num_regs; indexer++)
			{
				//Store the state of the regulator
				PrevRegState[indexer] =(regulator_configuration::Control_enum)pRegulator_configs[indexer]->Control;

				//Now force it into manual mode
				pRegulator_configs[indexer]->Control = regulator_configuration::MANUAL;
			}//end feeder change

			//Now let's do the same for capacitors
			for (indexer=0; indexer < num_caps; indexer++)
			{
				PrevCapState[indexer] = (capacitor::CAPCONTROL)pCapacitor_list[indexer]->control;	//Store the old
				pCapacitor_list[indexer]->control = capacitor::MANUAL;		//Put it into manual mode
			}
		}
		else	//Now in standby, implies were active before
		{
			//Put regulators back into previous mode
			for (indexer=0; indexer < num_regs; indexer++)
			{
				pRegulator_configs[indexer]->Control = PrevRegState[indexer];	//Put the regulator's mode back
			}

			//Put capacitors back in previous mode
			for (indexer=0; indexer < num_caps; indexer++)
			{
				pCapacitor_list[indexer]->control = PrevCapState[indexer];
			}
		}

		for (indexer=0; indexer<num_regs; indexer++)
		{
			TRegUpdate[indexer] = t0;				//Flag us to stay here.  This will force another iteration
		}

		TUpdateStatus = true;		//Flag the update
		prev_mode = (VOLTVARSTATE)control_method;	//Update the tracker
	}

	if (control_method == ACTIVE)	//are turned on?
	{
		for (reg_index=0; reg_index<num_regs; reg_index++)
		{
			if ((TRegUpdate[reg_index] <= t0) || (TRegUpdate[reg_index] == TS_NEVER))	//See if we're allowed to update
			{
				//Clear flag
				LimitExceed = 0x00;

				//Initialize "minimums" to something big
				vmin[0] = vmin[1] = vmin[2] = 999999999999.0;

				//Initialize VDrop and VSet value
				VDrop[0] = VDrop[1] = VDrop[2] = 0.0;
				VSet[0] = VSet[1] = VSet[2] = desired_voltage[reg_index];	//Default VSet to where we want

				//Initialize tap changes
				prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;

				//Parse through the measurement list - find the lowest voltage - do by PT_PHASE
				for (indexer=0; indexer<num_meas[reg_index]; indexer++)
				{
					//See if this node has phase A
					if ((pMeasurement_list[reg_index][indexer]->phases & PHASE_A) == PHASE_A)	//Has phase A
					{
						if (pMeasurement_list[reg_index][indexer]->voltage[0].Mag() < vmin[0])	//New minimum
						{
							vmin[0] = pMeasurement_list[reg_index][indexer]->voltage[0].Mag();
						}
					}

					//See if this node has phase B
					if ((pMeasurement_list[reg_index][indexer]->phases & PHASE_B) == PHASE_B)	//Has phase B
					{
						if (pMeasurement_list[reg_index][indexer]->voltage[1].Mag() < vmin[1])	//New minimum
						{
							vmin[1] = pMeasurement_list[reg_index][indexer]->voltage[1].Mag();
						}
					}

					//See if this node has phase C
					if ((pMeasurement_list[reg_index][indexer]->phases & PHASE_C) == PHASE_C)	//Has phase A
					{
						if (pMeasurement_list[reg_index][indexer]->voltage[2].Mag() < vmin[2])	//New minimum
						{
							vmin[2] = pMeasurement_list[reg_index][indexer]->voltage[2].Mag();
						}
					}
				}

				//Populate VRegTo (to end voltages)
				VRegTo[0] = RegToNodes[reg_index]->voltage[0].Mag();
				VRegTo[1] = RegToNodes[reg_index]->voltage[1].Mag();
				VRegTo[2] = RegToNodes[reg_index]->voltage[2].Mag();

				//Populate VDrop and VSet based on PT_PHASE
				if ((pRegulator_configs[reg_index]->PT_phase & PHASE_A) == PHASE_A)	//We have an A
				{
					VDrop[0] = VRegTo[0] - vmin[0];		//Calculate the drop
					VSet[0] = desired_voltage[reg_index] + VDrop[0];			//Calculate where we want to be

					//Make sure it isn't too big or small - otherwise toss a warning (it will cause problem)
					if (VSet[0] > maximum_voltage[reg_index])	//Will exceed
					{
						gl_warning("volt_var_control %s: The set point for phase A will exceed the maximum allowed voltage!",OBJECTHDR(this)->name);
						/*  TROUBLESHOOT
						The set point necessary to maintain the end point voltage exceeds the maximum voltage limit specified by the system.  Either
						increase this maximum_voltage limit, or configure your system differently.
						*/

						//See what region we are currently in
						if (pRegulator_list[reg_index]->tap[0] > 0)	//In raise region
						{
							if ((VRegTo[0] + reg_step_up[reg_index]) > maximum_voltage[reg_index])
							{
								LimitExceed |= 0x10;	//Flag A as above
							}
						}
						else	//Must be in lower region
						{
							if ((VRegTo[0] + reg_step_down[reg_index]) > maximum_voltage[reg_index])
							{
								LimitExceed |= 0x10;	//Flag A as above
							}
						}
					}
					else if (VSet[0] < minimum_voltage[reg_index])	//Will exceed
					{
						gl_warning("volt_var_control %s: The set point for phase A will exceed the minimum allowed voltage!",OBJECTHDR(this)->name);
						/*  TROUBLESHOOT
						The set point necessary to maintain the end point voltage exceeds the minimum voltage limit specified by the system.  Either
						decrease this minimum_voltage limit, or configure your system differently.
						*/

						//See what region we are currently in
						if (pRegulator_list[reg_index]->tap[0] > 0)	//In raise region
						{
							if ((VRegTo[0] - reg_step_up[reg_index]) < minimum_voltage[reg_index])
							{
								LimitExceed |= 0x01;	//Flag A as below
							}
						}
						else	//Must be in lower region
						{
							if ((VRegTo[0] - reg_step_down[reg_index]) < minimum_voltage[reg_index])
							{
								LimitExceed |= 0x01;	//Flag A as below
							}
						}
					}
				}

				if ((pRegulator_configs[reg_index]->PT_phase & PHASE_B) == PHASE_B)	//We have a B
				{
					VDrop[1] = VRegTo[1] - vmin[1];		//Calculate the drop
					VSet[1] = desired_voltage[reg_index] + VDrop[1];			//Calculate where we want to be

					//Make sure it isn't too big or small - otherwise toss a warning (it will cause problem)
					if (VSet[1] > maximum_voltage[reg_index])	//Will exceed
					{
						gl_warning("volt_var_control %s: The set point for phase B will exceed the maximum allowed voltage!",OBJECTHDR(this)->name);

						//See what region we are currently in
						if (pRegulator_list[reg_index]->tap[1] > 0)	//In raise region
						{
							if ((VRegTo[1] + reg_step_up[reg_index]) > maximum_voltage[reg_index])
							{
								LimitExceed |= 0x20;	//Flag B as above
							}
						}
						else	//Must be in lower region
						{
							if ((VRegTo[1] + reg_step_down[reg_index]) > maximum_voltage[reg_index])
							{
								LimitExceed |= 0x20;	//Flag B as above
							}
						}
					}
					else if (VSet[1] < minimum_voltage[reg_index])	//Will exceed
					{
						gl_warning("volt_var_control %s: The set point for phase B will exceed the minimum allowed voltage!",OBJECTHDR(this)->name);

						//See what region we are currently in
						if (pRegulator_list[reg_index]->tap[1] > 0)	//In raise region
						{
							if ((VRegTo[1] - reg_step_up[reg_index]) < minimum_voltage[reg_index])
							{
								LimitExceed |= 0x02;	//Flag B as below
							}
						}
						else	//Must be in lower region
						{
							if ((VRegTo[1] - reg_step_down[reg_index]) > minimum_voltage[reg_index])
							{
								LimitExceed |= 0x02;	//Flag B as below
							}
						}
					}
				}

				if ((pRegulator_configs[reg_index]->PT_phase & PHASE_C) == PHASE_C)	//We have a C
				{
					VDrop[2] = VRegTo[2] - vmin[2];		//Calculate the drop
					VSet[2] = desired_voltage[reg_index] + VDrop[2];			//Calculate where we want to be
					//Make sure it isn't too big or small - otherwise toss a warning (it will cause problem)

					if (VSet[2] > maximum_voltage[reg_index])	//Will exceed
					{
						gl_warning("volt_var_control %s: The set point for phase C will exceed the maximum allowed voltage!",OBJECTHDR(this)->name);

						//See what region we are currently in
						if (pRegulator_list[reg_index]->tap[2] > 0)	//In raise region
						{
							if ((VRegTo[2] + reg_step_up[reg_index]) > maximum_voltage[reg_index])
							{
								LimitExceed |= 0x40;	//Flag C as above
							}
						}
						else	//Must be in lower region
						{
							if ((VRegTo[2] + reg_step_down[reg_index]) > maximum_voltage[reg_index])
							{
								LimitExceed |= 0x40;	//Flag C as above
							}
						}
					}
					else if (VSet[2] < minimum_voltage[reg_index])	//Will exceed
					{
						gl_warning("volt_var_control %s: The set point for phase C will exceed the minimum allowed voltage!",OBJECTHDR(this)->name);

						//See what region we are currently in
						if (pRegulator_list[reg_index]->tap[2] > 0)	//In raise region
						{
							if ((VRegTo[2] - reg_step_up[reg_index]) < minimum_voltage[reg_index])
							{
								LimitExceed |= 0x04;	//Flag C as below
							}
						}
						else	//Must be in lower region
						{
							if ((VRegTo[2] - reg_step_down[reg_index]) < minimum_voltage[reg_index])
							{
								LimitExceed |= 0x04;	//Flag C as below
							}
						}
					}
				}

				//Now determine what kind of regulator we are
				if (pRegulator_configs[reg_index]->control_level == regulator_configuration::INDIVIDUAL)	//Individually handled
				{
					//Handle phases
					for (indexer=0; indexer<3; indexer++)	//Loop through phases
					{
						LimitExceed &= 0x7F;	//Use bit 8 as a validity flag (to save a variable)
						
						if ((indexer==0) && ((pRegulator_configs[reg_index]->PT_phase & PHASE_A) == PHASE_A))	//We have an A
						{
							temp_var_d = 0x01;		//A base lower "Limit" checker
							temp_var_u = 0x10;		//A base upper "Limit" checker
							LimitExceed |= 0x80;	//Valid phase
						}

						if ((indexer==1) && ((pRegulator_configs[reg_index]->PT_phase & PHASE_B) == PHASE_B))	//We have a B
						{
							temp_var_d = 0x02;		//B base lower "Limit" checker
							temp_var_u = 0x20;		//B base upper "Limit" checker
							LimitExceed |= 0x80;	//Valid phase
						}

						if ((indexer==2) && ((pRegulator_configs[reg_index]->PT_phase & PHASE_C) == PHASE_C))	//We have a C
						{
							temp_var_d = 0x04;		//C base lower "Limit" checker
							temp_var_u = 0x40;		//C base upper "Limit" checker
							LimitExceed |= 0x80;	//Valid phase
						}

						if ((LimitExceed & 0x80) == 0x80)	//Valid phase
						{
							//Make sure we aren't below the minimum or above the maximum first				  (***** This below here \/ \/ *************) - sub with step check! **************
							if (((vmin[indexer] > maximum_voltage[reg_index]) || (VRegTo[indexer] > maximum_voltage[reg_index])) && ((LimitExceed & temp_var_d) != temp_var_d))		//Above max, but won't go below
							{
								//Flag us for a down tap
								prop_tap_changes[indexer] = -1;
							}
							else if (((vmin[indexer] < minimum_voltage[reg_index]) || (VRegTo[indexer] < minimum_voltage[reg_index])) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below min, but won't exceed
							{
								//Flag us for an up tap
								prop_tap_changes[indexer] = 1;
							}
							else	//Normal operation
							{
								//See if we are in high load or low load conditions
								if (VDrop[indexer] > max_vdrop[reg_index])	//High loading
								{
									//See if we're outside our range
									if (((VSet[indexer] + vbw_high[reg_index]) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d)) 	//Above deadband, but can go down
									{
										//Check the theoretical change - make sure we won't exceed any limits
										if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
										{
											//Find out what a step down will get us theoretically
											if ((VRegTo[indexer] - reg_step_up[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
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
											if ((VRegTo[indexer] - reg_step_down[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
											{
												prop_tap_changes[indexer] = 0;	//No change allowed
											}
											else	//Change allowed
											{
												prop_tap_changes[indexer] = -1;  //Try to tap us down
											}
										}
									}//end above deadband
									else if (((VSet[indexer] - vbw_high[reg_index]) > VRegTo[indexer]) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below deadband, but can go up
									{
										//Check the theoretical change - make sure we won't exceed any limits
										if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
										{
											//Find out what a step up will get us theoretically
											if ((VRegTo[indexer] + reg_step_up[reg_index]) > maximum_voltage[reg_index])	//We'll exceed
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
											if ((VRegTo[indexer] + reg_step_down[reg_index]) > maximum_voltage[reg_index])	//We'll fall exceed
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
									if (((VSet[indexer] + vbw_low[reg_index]) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Above deadband, but can go down
									{
										//Check the theoretical change - make sure we won't exceed any limits
										if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
										{
											//Find out what a step down will get us theoretically
											if ((VRegTo[indexer] - reg_step_up[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
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
											if ((VRegTo[indexer] - reg_step_down[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
											{
												prop_tap_changes[indexer] = 0;	//No change allowed
											}
											else	//Change allowed
											{
												prop_tap_changes[indexer] = -1;  //Try to tap us down
											}
										}
									}//end above deadband
									else if (((VSet[indexer] - vbw_low[reg_index]) > VRegTo[indexer]) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below deadband, but can go up
									{
										//Check the theoretical change - make sure we won't exceed any limits
										if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
										{
											//Find out what a step up will get us theoretically
											if ((VRegTo[indexer] + reg_step_up[reg_index]) > maximum_voltage[reg_index])	//We'll exceed
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
											if ((VRegTo[indexer] + reg_step_down[reg_index]) > maximum_voltage[reg_index])	//We'll fall exceed
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
					TRegUpdate[reg_index] = TS_NEVER;

					for (indexer=0; indexer<3; indexer++)
					{
						if (prop_tap_changes[indexer] > 0)	//Want to tap up
						{
							//Make sure we aren't railed
							if (pRegulator_list[reg_index]->tap[indexer] >= pRegulator_configs[reg_index]->raise_taps)
							{
								//Just leave us railed - explicity done in case it somehow ends up over raise_taps (not sure how that would happen)
								pRegulator_list[reg_index]->tap[indexer] = pRegulator_configs[reg_index]->raise_taps;
							}
							else	//Must have room
							{
								pRegulator_list[reg_index]->tap[indexer]++;	//Increment

								//Flag as change
								Regulator_Change = true;
								TRegUpdate[reg_index] = t0 + (TIMESTAMP)RegUpdateTimes[reg_index];	//Set return time
							}
						}//end tap up
						else if (prop_tap_changes[indexer] < 0)	//Tap down
						{
							//Make sure we aren't railed
							if (pRegulator_list[reg_index]->tap[indexer] <= -pRegulator_configs[reg_index]->lower_taps)
							{
								//Just leave us railed - explicity done in case it somehow ends up under lower_taps (not sure how that would happen)
								pRegulator_list[reg_index]->tap[indexer] = -pRegulator_configs[reg_index]->lower_taps;
							}
							else	//Must have room
							{
								pRegulator_list[reg_index]->tap[indexer]--;	//Decrement

								//Flag the change
								Regulator_Change = true;
								TRegUpdate[reg_index] = t0 + (TIMESTAMP)RegUpdateTimes[reg_index];	//Set return time
							}
						}//end tap down
						//Defaulted else - no change
					}//end phase FOR
				}//end individual
				else	//Must be banked then
				{
					//Banked will take first PT_PHASE it matches.  If there's more than one, I don't want to know
					if ((pRegulator_configs[reg_index]->PT_phase & PHASE_A) == PHASE_A)	//We have an A
					{
						indexer = 0;		//Index for A-based voltages
						temp_var_d = 0x01;	//A base lower "Limit" checker
						temp_var_u = 0x10;	//A base upper "Limit" checker
					}
					else if ((pRegulator_configs[reg_index]->PT_phase & PHASE_B) == PHASE_B)	//We have a B
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
					if (((vmin[indexer] > maximum_voltage[reg_index]) || (VRegTo[indexer] > maximum_voltage[reg_index])) && ((LimitExceed & temp_var_d) != temp_var_d))		//Above max, but can go down
					{
						//Flag us for a down tap
						prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;
					}
					else if (((vmin[indexer] < minimum_voltage[reg_index]) || (VRegTo[indexer] < minimum_voltage[reg_index])) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below min, but can go up
					{
						//Flag us for an up tap
						prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 1;
					}
					else	//Normal operation
					{
						//See if we are in high load or low load conditions
						if (VDrop[indexer] > max_vdrop[reg_index])	//High loading
						{
							//See if we're outside our range
							if (((VSet[indexer] + vbw_high[reg_index]) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Above deadband, but can go down too
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step down will get us theoretically
									if ((VRegTo[indexer] - reg_step_up[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
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
									if ((VRegTo[indexer] - reg_step_down[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
									{
										prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;  //Try to tap us down
									}
								}
							}//end above deadband
							else if (((VSet[indexer] - vbw_high[reg_index]) > VRegTo[indexer]) && ((LimitExceed & temp_var_u) != temp_var_u))	//Below deadband, but can go up
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step up will get us theoretically
									if ((VRegTo[indexer] + reg_step_up[reg_index]) > maximum_voltage[reg_index])	//We'll exceed
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
									if ((VRegTo[indexer] + reg_step_down[reg_index]) > maximum_voltage[reg_index])	//We'll fall exceed
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
							if (((VSet[indexer] + vbw_low[reg_index]) < VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Above deadband, but can go down
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step down will get us theoretically
									if ((VRegTo[indexer] - reg_step_up[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
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
									if ((VRegTo[indexer] - reg_step_down[reg_index]) < minimum_voltage[reg_index])	//We'll fall below
									{
										prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = 0;	//No change allowed
									}
									else	//Change allowed
									{
										prop_tap_changes[0] = prop_tap_changes[1] = prop_tap_changes[2] = -1;  //Try to tap us down
									}
								}
							}//end above deadband
							else if (((VSet[indexer] - vbw_low[reg_index]) > VRegTo[indexer]) && ((LimitExceed & temp_var_d) != temp_var_d))	//Below deadband, but can go up
							{
								//Check the theoretical change - make sure we won't exceed any limits
								if (pRegulator_list[reg_index]->tap[indexer] > 0)	//Step up region
								{
									//Find out what a step up will get us theoretically
									if ((VRegTo[indexer] + reg_step_up[reg_index]) > maximum_voltage[reg_index])	//We'll exceed
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
									if ((VRegTo[indexer] + reg_step_down[reg_index]) > maximum_voltage[reg_index])	//We'll fall exceed
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
					TRegUpdate[reg_index] = TS_NEVER;
					if (prop_tap_changes[0] > 0)	//Want a tap up
					{
						//Check individually - set to rail if they are at or exceed - this may lose the offset, but I don't know how they'd ever exceed a limit anyways
						if (pRegulator_list[reg_index]->tap[0] >= pRegulator_configs[reg_index]->raise_taps)
						{
							pRegulator_list[reg_index]->tap[0] = pRegulator_configs[reg_index]->raise_taps;	//Set at limit
							limit_hit = true;											//Flag that a limit was hit
						}

						if (pRegulator_list[reg_index]->tap[1] >= pRegulator_configs[reg_index]->raise_taps)
						{
							pRegulator_list[reg_index]->tap[1] = pRegulator_configs[reg_index]->raise_taps;	//Set at limit
							limit_hit = true;											//Flag that a limit was hit
						}

						if (pRegulator_list[reg_index]->tap[2] >= pRegulator_configs[reg_index]->raise_taps)
						{
							pRegulator_list[reg_index]->tap[2] = pRegulator_configs[reg_index]->raise_taps;	//Set at limit
							limit_hit = true;											//Flag that a limit was hit
						}

						if (limit_hit == false)	//We can still proceed
						{
							pRegulator_list[reg_index]->tap[0]++;	//Increment them all
							pRegulator_list[reg_index]->tap[1]++;
							pRegulator_list[reg_index]->tap[2]++;

							Regulator_Change = true;					//Flag the change
							TRegUpdate[reg_index] = t0 + (TIMESTAMP)RegUpdateTimes[reg_index];	//Set return time
						}
						//Default else - limit hit, so "no change"
					}//end tap up
					else if (prop_tap_changes[0] < 0)	//Want a tap down
					{
						//Check individually - set to rail if they are at or exceed - this may lose the offset, but I don't know how they'd ever exceed a limit anyways
						if (pRegulator_list[reg_index]->tap[0] <= -pRegulator_configs[reg_index]->lower_taps)
						{
							pRegulator_list[reg_index]->tap[0] = -pRegulator_configs[reg_index]->lower_taps;	//Set at limit
							limit_hit = true;											//Flag that a limit was hit
						}

						if (pRegulator_list[reg_index]->tap[1] <= -pRegulator_configs[reg_index]->lower_taps)
						{
							pRegulator_list[reg_index]->tap[1] = -pRegulator_configs[reg_index]->lower_taps;	//Set at limit
							limit_hit = true;											//Flag that a limit was hit
						}

						if (pRegulator_list[reg_index]->tap[2] <= -pRegulator_configs[reg_index]->lower_taps)
						{
							pRegulator_list[reg_index]->tap[2] = -pRegulator_configs[reg_index]->lower_taps;	//Set at limit
							limit_hit = true;											//Flag that a limit was hit
						}

						if (limit_hit == false)	//We can still proceed
						{
							pRegulator_list[reg_index]->tap[0]--;	//Decrement them all
							pRegulator_list[reg_index]->tap[1]--;
							pRegulator_list[reg_index]->tap[2]--;

							Regulator_Change = true;					//Flag the change
							TRegUpdate[reg_index] = t0 + (TIMESTAMP)RegUpdateTimes[reg_index];	//Set return time
						}
						//Default else - limit hit, so "no change"
					}//end tap down
					//Defaulted else - no change requested
				}//End banked mode
			}//End allowed to update if
		}//End regulator traversion for
	}//end valid cycle

	//See how we need to proceed
	
	//Find the minimum update first
	treg_min = TS_NEVER;
	for (index=0; index<num_regs; index++)
	{
		if (TRegUpdate[index] < treg_min)
		{
			treg_min = TRegUpdate[index];
		}
	}

	if (tret < treg_min)	//Normal return is before the timer
		return tret;
	else	//Timer return is first - make this a "suggested" return
	{
		if (treg_min == TS_NEVER)	//Final check to make sure we don't return "-infinity"
			return TS_NEVER;
		else
			return -treg_min;
	}
}

TIMESTAMP volt_var_control::postsync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::postsync(t0);
	complex link_power_vals;
	int index;
	bool change_requested;
	bool allow_change;
	capacitor::CAPSWITCH bank_status;
	double temp_size;
	double curr_pf_temp, react_pwr_temp, des_react_pwr_temp;
	bool pf_add_capacitor, pf_check;	

	//Grab power values and all of those related calculations
	if ((control_method == ACTIVE) && (Regulator_Change == false))	//no regulator changes in progress and we're active
	{
		link_power_vals = complex(0.0,0.0);	//Zero the power

		if (solver_method == SM_NR)
		{
			//We're technically before the power calculations on link, so force them
			//not issue with FBS because we have to be below to get voltage updates 
			substation_link->calculate_power();
		}

		//Pull out phases as necessary
		if ((pf_phase & PHASE_A) == PHASE_A)
			link_power_vals += substation_link->indiv_power_in[0];

		if ((pf_phase & PHASE_B) == PHASE_B)
			link_power_vals += substation_link->indiv_power_in[1];

		if ((pf_phase & PHASE_C) == PHASE_C)
			link_power_vals += substation_link->indiv_power_in[2];

		//Populate the variables of interest
		react_pwr = link_power_vals.Im();						//Pull in reactive power

		if (pf_signed==true)
		{
			curr_pf_temp = fabs(link_power_vals.Re())/link_power_vals.Mag();	//Pull in power factor

			//"sign" it appropriately
			if (react_pwr<0)	//negative vars
				curr_pf = -curr_pf_temp;
			else				//positive (or somehow zero)
				curr_pf = curr_pf_temp;
		}
		else	//Otherwise, just pull in the value
		{
			curr_pf = fabs(link_power_vals.Re())/link_power_vals.Mag();	//Pull in power factor
		}

		//Update "proceeding" variable
		if (((solver_method == SM_NR) && (first_cycle==true)) || ((solver_method == SM_FBS) && (first_cycle == false)))
			allow_change = true;	//Intermediate assignment since FBS likes to mess up power calculations on the first cycle
		else
			allow_change = false;

		if (pf_signed==true)	//Consider "signing" on the power factor
		{
			//Figure out the reactive part "desired" for current load
			des_react_pwr_temp = fabs(link_power_vals.Re()*sqrt(1/(desired_pf*desired_pf)-1));

			//Formulate variables so signs matter now
			if (curr_pf < 0)	//Negative current value - implies capacitive loading
			{
				if (desired_pf > 0)	//Capacitve is desired, see if we can do something
				{
					if (-desired_pf > curr_pf)	//More capacitive desired - add one in
					{
						pf_add_capacitor = true;	//Add one

						//Determine size (3 curr, 5 desired, -5 - (-3) = 2 more allowed
						react_pwr_temp = -des_react_pwr_temp - react_pwr;
					}
					else	//Less capacitive - remove a capacitor
					{
						pf_add_capacitor = false;	//Remove one

						//Determine size (3 curr, -1 desired, 1 + (-3) = -2 more allowed
						react_pwr_temp = des_react_pwr_temp + react_pwr;
					}
				}//End desired is positive
				else	//Inductive PF desired - remove a capacitor
				{
					pf_add_capacitor = false;	//Remove one

					//Determine size (3 curr, -1 desired, -1  + (-3) = -4 more allowed
					react_pwr_temp = -des_react_pwr_temp + react_pwr;
				}//End inductive desired on capacitive system
			}//End negative current pf value
			else	//Positive or zero current value
			{
				if (desired_pf > 0)	//Capacitive is desired
				{
					pf_add_capacitor = true;	//Add a capacitor in

					//Determine size (-1 curr, 3 desired, 3 + (1) = 4 more allowed
					react_pwr_temp = des_react_pwr_temp + react_pwr;
				}
				else	//Inductive desired
				{
					if (-desired_pf > curr_pf)	//Less inductive wanted
					{
						pf_add_capacitor = true;	//Add a capacitor

						//Determine size (-3 curr, -1 desired, -1 + (3) = 2 more allowed
						react_pwr_temp = -des_react_pwr_temp + react_pwr;
					}
					else	//Must be less, so more inductive wanted
					{
						pf_add_capacitor = false;	//Remove a capacitor

						//Determine size (-3 curr, -4 desired, -4 + (3) = -1 more allowed
						react_pwr_temp = -des_react_pwr_temp + react_pwr;
					}//End more inductive wanted
				}//End inductive desired
			}//End positive current pf value

			//Remove the sign on the reactive power desired
			react_pwr_temp = fabs(react_pwr_temp);

			//Set flag for "traditional" method so we can get into the loop
			pf_check = true;
		}//End consider pf sign
		else	//Just consider it a deadband
		{
			if (curr_pf < desired_pf)
				pf_check = true;		//Outside the range, make a change
			else
				pf_check = false;		//Inside the deadband - don't care
		}

		//Now check to see if this is a first pass or not - always "attempt" a change for positive/negative pf - just may be no candidates
		if ((pf_check==true) && (allow_change==true) && (TCapUpdate <= t0))
		{
			change_requested = false;	//Start out assuming no change

			//Parse through the capacitor list - see where they sit in the categories - break after one switching operation
			for (index=0; index < num_caps; index++)
			{
				//Find the phases being watched, check their switch
				if ((pCapacitor_list[index]->pt_phase & PHASE_A) == PHASE_A)
					bank_status = (capacitor::CAPSWITCH)pCapacitor_list[index]->switchA_state;
				else if ((pCapacitor_list[index]->pt_phase & PHASE_B) == PHASE_B)
					bank_status = (capacitor::CAPSWITCH)pCapacitor_list[index]->switchB_state;
				else	//Must be C, right?
					bank_status = (capacitor::CAPSWITCH)pCapacitor_list[index]->switchC_state;

				if (pf_signed==true)	//Consider the sign in switching operations
				{
					//Now perform logic based on where it is
					if ((bank_status == capacitor::CLOSED) && (pf_add_capacitor==false))	//We are on and need to remove someone
					{
						temp_size = Capacitor_size[index] * d_max; //min;

						if (react_pwr_temp >= temp_size)
						{
							pCapacitor_list[index]->toggle_bank_status(false);	//Turn all off
							change_requested = true;							//Flag a change
							break;	//No more loop, only one control per loop
						}
					}//end cap was on
					else if ((bank_status == capacitor::OPEN) && (pf_add_capacitor==true))	//We're off and want to turn someone on
					{
						temp_size = Capacitor_size[index] * d_max;

						if (react_pwr_temp >= temp_size)
						{
							pCapacitor_list[index]->toggle_bank_status(true);	//Turn all on
							change_requested = true;							//Flag a change
							break;	//No more loop, only one control per loop
						}
					}//end cap was off and want to add
					//defaulted else - do nothing
				}//end consider pf sign
				else	//Don't consider the sign, just consider it a range
				{
					//Now perform logic based on where it is
					if (bank_status == capacitor::CLOSED)	//We are on
					{
						temp_size = Capacitor_size[index] * d_min;

						if (react_pwr < temp_size)
						{
							pCapacitor_list[index]->toggle_bank_status(false);	//Turn all off
							change_requested = true;							//Flag a change
							break;	//No more loop, only one control per loop
						}
					}//end cap was on
					else	//Must be false, so we're off
					{
						temp_size = Capacitor_size[index] * d_max;

						if (react_pwr > temp_size)
						{
							pCapacitor_list[index]->toggle_bank_status(true);	//Turn all on
							change_requested = true;							//Flag a change
							break;	//No more loop, only one control per loop
						}
					}//end cap was off
				}//End just consider pf range
			}//End capacitor list traversion

			//If we are outside our pf range and nothing went off, see if a smaller capacitor can be "prompted" to get us back in range
			if ((change_requested == false) && (pf_signed==true))
			{
				//See if we're needing a capacitor action to get back in range, or just looking for more adjustments
				if (((curr_pf > 0) && (curr_pf < -desired_pf)) || ((curr_pf < 0) && (curr_pf < desired_pf)))
				{
					//Parse through the capacitor list - go backwards (smallest first) - break after one operation (if any)
					for (index=(num_caps-1); index >= 0; index--)
					{
						//Find the phases being watched, check their switch
						if ((pCapacitor_list[index]->pt_phase & PHASE_A) == PHASE_A)
							bank_status = (capacitor::CAPSWITCH)pCapacitor_list[index]->switchA_state;
						else if ((pCapacitor_list[index]->pt_phase & PHASE_B) == PHASE_B)
							bank_status =(capacitor::CAPSWITCH) pCapacitor_list[index]->switchB_state;
						else	//Must be C, right?
							bank_status =(capacitor::CAPSWITCH) pCapacitor_list[index]->switchC_state;

						//Now perform logic based on where it is - if anything is found to "fit" the criterion, just enact it
						if ((bank_status == capacitor::CLOSED) && (pf_add_capacitor==false))	//We are on and need to remove someone
						{
							pCapacitor_list[index]->toggle_bank_status(false);	//Turn all off
							change_requested = true;							//Flag a change
							break;	//No more loop, only one control per loop
						}//end cap was on
						else if ((bank_status == capacitor::OPEN) && (pf_add_capacitor==true))	//We're off and want to turn someone on
						{
							pCapacitor_list[index]->toggle_bank_status(true);	//Turn all on
							change_requested = true;							//Flag a change
							break;	//No more loop, only one control per loop
						}//end cap was off and want to add
						//defaulted else - do nothing
					}//End capacitor list traversion
				}//End "outside acceptable range"
			}//end change still requested
			//Default else - change must have been requested

			if (change_requested == true)	//Something changed
			{
				TCapUpdate = t0 + (TIMESTAMP)CapUpdateTimes[index];	//Figure out where we want to go

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
		}//End TCap negative
	}//End Non-accumulation cycle
}

//Function to parse a comma-separated list to get the next double (or the last double)
char *volt_var_control::dbl_token(char *start_token, double *dbl_val)
{
	char workArray[64];	//If we ever need over 64, this will need changing
	char *outIndex, *workIndex, *end_token;
	char index;

	//Initialize work variable
	for (index=0; index<64; index++)
		workArray[index] = 0;

	//Link the output
	workIndex = workArray;

	//Look for a comma in the input value
	outIndex = strchr(start_token,',');	//Look for commas, or none

	if (outIndex == NULL)	//No commas found
	{
		while (*start_token != '\0')
		{
			*workIndex++ = *start_token++;
		}

		end_token = NULL;
	}
	else	//Comma found, but we only want to go just before it
	{
		while (start_token < outIndex)
		{
			*workIndex++ = *start_token++;
		}

		end_token = start_token+1;
	}

	//Point back to the beginning
	workIndex = workArray;

	//Convert it
	*dbl_val = strtod(workIndex,NULL);

	//Return the end pointer
	return end_token;
}

//Function to parse a comma-separated list to get the next object (or the last object)
char *volt_var_control::obj_token(char *start_token, OBJECT **obj_val)
{
	char workArray[64];	//Hopefully, names will never be over 64 characters - seems to get upset if we do more
	char *outIndex, *workIndex, *end_token;
	char index;

	//Initialize work variable
	for (index=0; index<64; index++)
		workArray[index] = 0;

	//Link the output
	workIndex = workArray;

	//Look for a comma in the input value
	outIndex = strchr(start_token,',');	//Look for commas, or none

	if (outIndex == NULL)	//No commas found
	{
		while (*start_token != '\0')
		{
			*workIndex++ = *start_token++;
		}

		end_token = NULL;
	}
	else	//Comma found, but we only want to go just before it
	{
		while (start_token < outIndex)
		{
			*workIndex++ = *start_token++;
		}

		end_token = start_token+1;
	}

	//Point back to the beginning
	workIndex = workArray;

	//Get the object
	*obj_val = gl_get_object(workIndex);

	//Return the end pointer
	return end_token;
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
	catch (const char *msg)
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
