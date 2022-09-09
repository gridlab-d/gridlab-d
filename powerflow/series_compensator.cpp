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

CLASS* series_compensator::oclass = nullptr;
CLASS* series_compensator::pclass = nullptr;

series_compensator::series_compensator(MODULE *mod) : link_object(mod)
{
	if (oclass==nullptr)
	{
		// link to parent class (used by isa)
		pclass = link_object::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"series_compensator",sizeof(series_compensator),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class series_compensator";
		else
			oclass->trl = TRL_DEMONSTRATED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "link",

			PT_double, "vset_A[pu]", PADDR(vset_value[0]), PT_DESCRIPTION, "Voltage magnitude reference for phase A",
			PT_double, "vset_B[pu]", PADDR(vset_value[1]), PT_DESCRIPTION, "Voltage magnitude reference for phase B",
			PT_double, "vset_C[pu]", PADDR(vset_value[2]), PT_DESCRIPTION, "Voltage magnitude reference for phase C",

			PT_double, "vset_A_0[pu]", PADDR(vset_value_0[0]), PT_DESCRIPTION, "Voltage magnitude set point for phase A, changed by the player",
			PT_double, "vset_B_0[pu]", PADDR(vset_value_0[1]), PT_DESCRIPTION, "Voltage magnitude set point for phase B, changed by the player",
			PT_double, "vset_C_0[pu]", PADDR(vset_value_0[2]), PT_DESCRIPTION, "Voltage magnitude set point for phase C, changed by the player",

			PT_double, "vset_1[pu]", PADDR(vset_value[0]), PT_DESCRIPTION, "Voltage magnitude reference for phase 1 of a triplex system",
			PT_double, "vset_2[pu]", PADDR(vset_value[1]), PT_DESCRIPTION, "Voltage magnitude reference for phase 2 of a tryplex system",

			PT_double, "vset_1_0[pu]", PADDR(vset_value_0[0]), PT_DESCRIPTION, "Voltage magnitude reference for phase 1 of a triplex system",
			PT_double, "vset_2_0[pu]", PADDR(vset_value_0[1]), PT_DESCRIPTION, "Voltage magnitude reference for phase 2 of a tryplex system",

			PT_bool, "frequency_regulation", PADDR(frequency_regulation),  PT_DESCRIPTION, "DELTAMODE: Boolean value indicating whether the frequency regulation of the series compensator is enabled or not",
			PT_bool, "frequency_open_loop_control", PADDR(frequency_open_loop_control),  PT_DESCRIPTION, "DELTAMODE: Boolean value indicating whether the frequency open loop control of the series compensator is enabled or not",


			PT_double, "t_delay", PADDR(t_delay), PT_DESCRIPTION, "the controller will wait for t_delay to take actions",
			PT_double, "t_hold", PADDR(t_hold), PT_DESCRIPTION, "Once the controller changes the voltage set point, it will stay there for t_hold time",
			PT_double, "recover_rate", PADDR(recover_rate), PT_DESCRIPTION, "The rate that the voltage goes back to nominal, unit: pu/s",
			PT_double, "frequency_low", PADDR(frequency_low), PT_DESCRIPTION, "The low frequency that activates the controller",
			PT_double, "frequency_high", PADDR(frequency_high), PT_DESCRIPTION, "The high frequency that activates the controller",
			PT_double, "V_error", PADDR(V_error), PT_DESCRIPTION, "Make sure the voltage can go back to nominal",


			PT_double, "voltage_update_tolerance[pu]", PADDR(voltage_iter_tolerance), PT_DESCRIPTION, "Largest absolute between vset_X and measured voltage that won't force a reiteration",

			PT_double, "turns_ratio_A", PADDR(turns_ratio[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase A series compensator equivalent",
			PT_double, "turns_ratio_B", PADDR(turns_ratio[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase B series compensator equivalent",
			PT_double, "turns_ratio_C", PADDR(turns_ratio[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase C series compensator equivalent",

			PT_double, "turns_ratio_1", PADDR(turns_ratio[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase 1 (triplex) series compensator equivalent",
			PT_double, "turns_ratio_2", PADDR(turns_ratio[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Debug variable - Turns ratio for phase 2 (triplex) series compensator equivalent",

			PT_double, "n_max_ext_A", PADDR(n_max_ext[0]), PT_DESCRIPTION, "maximum Turn ratio for phase A",
			PT_double, "n_max_ext_B", PADDR(n_max_ext[1]), PT_DESCRIPTION, "maximum Turn ratio for phase B",
			PT_double, "n_max_ext_C", PADDR(n_max_ext[2]), PT_DESCRIPTION, "maximum Turn ratio for phase C",
			PT_double, "n_min_ext_A", PADDR(n_min_ext[0]), PT_DESCRIPTION, "minimum Turn ratio for phase A",
			PT_double, "n_min_ext_B", PADDR(n_min_ext[1]), PT_DESCRIPTION, "minimum Turn ratio for phase B",
			PT_double, "n_min_ext_C", PADDR(n_min_ext[2]), PT_DESCRIPTION, "minimum Turn ratio for phase C",

			PT_double, "n_max_ext_1", PADDR(n_max_ext[0]), PT_DESCRIPTION, "maximum Turn ratio for phase 1 (triplex)",
			PT_double, "n_max_ext_2", PADDR(n_max_ext[1]), PT_DESCRIPTION, "maximum Turn ratio for phase 2 (triplex)",
			PT_double, "n_min_ext_1", PADDR(n_min_ext[0]), PT_DESCRIPTION, "minimum Turn ratio for phase 1 (triplex)",
			PT_double, "n_min_ext_2", PADDR(n_min_ext[1]), PT_DESCRIPTION, "minimum Turn ratio for phase 2 (triplex)",

			PT_double, "kp", PADDR(kp), PT_DESCRIPTION,  "proportional gain",
			PT_double, "ki", PADDR(ki), PT_DESCRIPTION,  "integrator gain",
			PT_double, "kpf", PADDR(kpf), PT_DESCRIPTION,  "proportional gain of frequency regulation",
			PT_double, "f_db_max", PADDR(f_db_max), PT_DESCRIPTION,  "frequency dead band max",
			PT_double, "f_db_min", PADDR(f_db_min), PT_DESCRIPTION,  "frequency dead band max",
			PT_double, "delta_Vmax", PADDR(delta_Vmax), PT_DESCRIPTION,  "upper limit of the frequency regulation output",
			PT_double, "delta_Vmin", PADDR(delta_Vmin), PT_DESCRIPTION,  "lower limit of the frequency regulation output",
			PT_double, "delta_V", PADDR(delta_V), PT_DESCRIPTION,  "frequency regulation output",


			PT_double, "V_bypass_max_pu", PADDR(V_bypass_max_pu), PT_DESCRIPTION,  "the upper limit voltage to bypass compensator",
			PT_double, "V_bypass_min_pu", PADDR(V_bypass_min_pu), PT_DESCRIPTION,  "the lower limit voltage to bypass compensator",

			PT_enumeration, "phase_A_state", PADDR(phase_states[0]),PT_DESCRIPTION,"Defines if phase A is in bypass or not",
				PT_KEYWORD, "NORMAL", (enumeration)ST_NORMAL,
				PT_KEYWORD, "BYPASS", (enumeration)ST_BYPASS,
			PT_enumeration, "phase_B_state", PADDR(phase_states[1]),PT_DESCRIPTION,"Defines if phase B is in bypass or not",
				PT_KEYWORD, "NORMAL", (enumeration)ST_NORMAL,
				PT_KEYWORD, "BYPASS", (enumeration)ST_BYPASS,
			PT_enumeration, "phase_C_state", PADDR(phase_states[2]),PT_DESCRIPTION,"Defines if phase C is in bypass or not",
				PT_KEYWORD, "NORMAL", (enumeration)ST_NORMAL,
				PT_KEYWORD, "BYPASS", (enumeration)ST_BYPASS,
			PT_enumeration, "phase_1_state", PADDR(phase_states[0]),PT_DESCRIPTION,"Defines if phase 1 is in bypass or not",
				PT_KEYWORD, "NORMAL", (enumeration)ST_NORMAL,
				PT_KEYWORD, "BYPASS", (enumeration)ST_BYPASS,
			PT_enumeration, "phase_2_state", PADDR(phase_states[1]),PT_DESCRIPTION,"Defines if phase 2 is in bypass or not",
				PT_KEYWORD, "NORMAL", (enumeration)ST_NORMAL,
				PT_KEYWORD, "BYPASS", (enumeration)ST_BYPASS,

			PT_double, "series_compensator_resistance[Ohm]", PADDR(series_compensator_resistance), PT_DESCRIPTION, "Baseline resistance for the series compensator device - needed for NR",
			nullptr)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_series_compensator)==nullptr)
			GL_THROW("Unable to publish series_compensator deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==nullptr)
			GL_THROW("Unable to publish series_compensator external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==nullptr)
			GL_THROW("Unable to publish series_compensator external power limit calculation function");
		if (gl_publish_function(oclass,	"perform_current_calculation_pwr_link", (FUNCTIONADDR)currentcalculation_link)==nullptr)
			GL_THROW("Unable to publish series_compensator external current calculation function");
	}
}

int series_compensator::isa(char *classname)
{
	return strcmp(classname,"series_compensator")==0 || link_object::isa(classname);
}

int series_compensator::create()
{
	int result = link_object::create();
	
	series_compensator_start_time = TS_INVALID;
	series_compensator_first_step = true;


	frequency_regulation = false;
	frequency_open_loop_control = false;


	// frequency open loop control
	t_delay_low_flag = false;
	t_hold_low_flag = false;
	t_recover_low_flag = false;
	t_delay_high_flag = false;
	t_hold_high_flag = false;
	t_recover_high_flag = false;

	t_count_low_delay = 0;
	t_count_low_hold = 0;
	t_count_high_delay = 0;
	t_count_high_hold = 0;

	t_delay = 0.05; // 0.05 seconds
	t_hold =  20; // 20 seconds
	recover_rate = 0.005; // 0.01 pu/s
	frequency_low = 59; //59 Hz
	frequency_high = 61; //61 Hz
	V_error = 1e-6;

	kp = 0;
	ki = 10;
    n_max[0] = n_max[1] = n_max[2] = 1.13;
    n_min[0] = n_min[1] = n_min[2] = 0.87;
    n_max_ext[0] = n_max_ext[1] = n_max_ext[2] = 1.13;
    n_min_ext[0] = n_min_ext[1] = n_min_ext[2] = 0.87;
    V_bypass_max_pu = 1.25;
    V_bypass_min_pu = 0.667;

    // frequency regulator
    kpf = 0.01;
    f_db_max = 0.3;
    f_db_min = -0.3;
    delta_Vmax = 0.05;
    delta_Vmin = -0.05;

	pred_state.dn_ini_StateVal[0] = pred_state.dn_ini_StateVal[1]=pred_state.dn_ini_StateVal[2] = 0;
	pred_state.n_ini_StateVal[0] = pred_state.n_ini_StateVal[1] = pred_state.n_ini_StateVal[2] = 1;
	pred_state.n_StateVal[0] = pred_state.n_StateVal[1] = pred_state.n_StateVal[2]= 1;

	next_state.dn_ini_StateVal[0] = next_state.dn_ini_StateVal[1]=next_state.dn_ini_StateVal[2] = 0;
	next_state.n_ini_StateVal[0] = next_state.n_ini_StateVal[1] = next_state.n_ini_StateVal[2] = 1;
	next_state.n_StateVal[0] = next_state.n_StateVal[1] = next_state.n_StateVal[2]= 1;

	curr_state.dn_ini_StateVal[0] = curr_state.dn_ini_StateVal[1]=curr_state.dn_ini_StateVal[2] = 0;
	curr_state.n_ini_StateVal[0] = curr_state.n_ini_StateVal[1] = curr_state.n_ini_StateVal[2] = 1;
	curr_state.n_StateVal[0] = curr_state.n_StateVal[1] = curr_state.n_StateVal[2]= 1;

	//Working variables - set initial ratio to unity
	turns_ratio[0] = turns_ratio[1] = turns_ratio[2] = 1.0;
	prev_turns_ratio[0] = prev_turns_ratio[1] = prev_turns_ratio[2] = 0.0;	//Set different, so it forces an update

	//Null the voltage pointers
	ToNode_voltage[0] = ToNode_voltage[1] = ToNode_voltage[2] = nullptr;
	FromNode_voltage[0]=FromNode_voltage[1]=FromNode_voltage[2]=nullptr;
	FromNode_frequency = nullptr;

	//Zero the values
	val_ToNode_voltage[0] = val_ToNode_voltage[1] = val_ToNode_voltage[2] = gld::complex(0.0,0.0);
	val_ToNode_voltage_pu[0] = val_ToNode_voltage_pu[1] = val_ToNode_voltage_pu[2] = 0.0;
	val_FromNode_voltage[0] = val_FromNode_voltage[1] = val_FromNode_voltage[2] = gld::complex(0.0,0.0);
	val_FromNode_voltage_pu[0] = val_FromNode_voltage_pu[1] = val_FromNode_voltage_pu[2] = 0.0;

	vset_value[0] = vset_value[1] = vset_value[2] = 1.0;	//Start out set to unity voltage
	vset_value_0[0] = vset_value_0[1] = vset_value_0[2] = 1.0;	//Start out set to unity voltage

	val_FromNode_nominal_voltage = 0.0;
	val_FromNode_frequency = nominal_frequency;
	voltage_iter_tolerance = 1e-4;	//1% PU default

	//Operating in normal, by default
	phase_states[0] = phase_states[1] = phase_states[2] = ST_NORMAL;

	//Arbirtrary value for deltamode return
	deltamode_return_val = -1;

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
			a_mat[iindex][jindex] = gld::complex(0.0,0.0);
			b_mat[iindex][jindex] = gld::complex(0.0,0.0);
			c_mat[iindex][jindex] = gld::complex(0.0,0.0);
			d_mat[iindex][jindex] = gld::complex(0.0,0.0);
			A_mat[iindex][jindex] = gld::complex(0.0,0.0);
			B_mat[iindex][jindex] = gld::complex(0.0,0.0);
			base_admittance_mat[iindex][jindex] = gld::complex(0.0,0.0);
		}
	}

	FromNode_frequency = new gld_property(from,"measured_frequency");

	series_compensator_start_time = gl_globalclock;

	//Make sure it worked
	if (!FromNode_frequency->is_valid() || !FromNode_frequency->is_double())
	{
		GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined elsewhere
	}

	val_FromNode_frequency = FromNode_frequency->get_double(); //get the frequency

	//See what kind of connection we are
	if (has_phase(PHASE_S))	//Triplex-enabled
	{
		//Do phase 1
		//Map to the property of interest - voltage_1
		ToNode_voltage[0] = new gld_property(to,"voltage_1");
		FromNode_voltage[0] = new gld_property(from,"voltage_1");



		//Make sure it worked
		if (!ToNode_voltage[0]->is_valid() || !ToNode_voltage[0]->is_complex())
		{
			GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined elsewhere
		}

		//Make sure it worked
		if (!FromNode_voltage[0]->is_valid() || !FromNode_voltage[0]->is_complex())
		{
			GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined elsewhere
		}

		//Set the appropriate turns ratio matrices
		d_mat[0][0] = gld::complex(turns_ratio[0],0.0);
		a_mat[0][0] = gld::complex(1.0/turns_ratio[0],0.0);
		A_mat[0][0] = gld::complex(turns_ratio[0],0.0);

		//Now phase 2
		//Map to the property of interest - voltage_2
		ToNode_voltage[1] = new gld_property(to,"voltage_2");
		FromNode_voltage[1] = new gld_property(from,"voltage_2");

		//Make sure it worked
		if (!ToNode_voltage[1]->is_valid() || !ToNode_voltage[1]->is_complex())
		{
			GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined elsewhere
		}

		//Make sure it worked
		if (!FromNode_voltage[1]->is_valid() || !FromNode_voltage[1]->is_complex())
		{
			GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined elsewhere
		}

		//Set the appropriate turns ratio matrices
		d_mat[1][1] = gld::complex(turns_ratio[1],0.0);
		a_mat[1][1] = gld::complex(1.0/turns_ratio[1],0.0);
		A_mat[1][1] = gld::complex(turns_ratio[1],0.0);

	}//End triplex
	else	//Must be some variant of three-phase
	{
		//Map the to-node connections
		if (has_phase(PHASE_A))
		{
			//Map to the property of interest - voltage_A
			ToNode_voltage[0] = new gld_property(to,"voltage_A");
			FromNode_voltage[0] = new gld_property(from,"voltage_A");

			//Make sure it worked
			if (!ToNode_voltage[0]->is_valid() || !ToNode_voltage[0]->is_complex())
			{
				GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
				/* TROUBLESHOOT
				While attempting to map the voltage for the to node, a property could not be properly mapped.
				Please try again.  If the error persists, please submit an issue in the ticketing system.
				*/
			}

			//Make sure it worked
			if (!FromNode_voltage[0]->is_valid() || !FromNode_voltage[0]->is_complex())
			{
				GL_THROW("series_compensator:%d - %s - Unable to map property for remote object",obj->id,(obj->name ? obj->name : "Unnamed"));
				/* TROUBLESHOOT
				While attempting to map the voltage for the to node, a property could not be properly mapped.
				Please try again.  If the error persists, please submit an issue in the ticketing system.
				*/
			}


			//Set the appropriate turns ratio matrices
			d_mat[0][0] = gld::complex(turns_ratio[0],0.0);
			a_mat[0][0] = gld::complex(1.0/turns_ratio[0],0.0);
			A_mat[0][0] = gld::complex(turns_ratio[0],0.0);
		}
		else	//Nope
		{
			ToNode_voltage[0] = nullptr;	//Null it -- should already be done, but be paranoid

			//Set the per-unit setpoint too -- otherwise we'll have issues later
			vset_value[0] = 0.0;
			vset_value_0[0] = 0.0;
		}

		//Check for B
		if (has_phase(PHASE_B))
		{
			//Map to the property of interest - voltage_B
			ToNode_voltage[1] = new gld_property(to,"voltage_B");
			FromNode_voltage[1] = new gld_property(from,"voltage_B");

			//Make sure it worked
			if (!ToNode_voltage[1]->is_valid() || !ToNode_voltage[1]->is_complex())
			{
				GL_THROW("series_compensator:%d - %s - Unable to map property for TO node",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Make sure it worked
			if (!FromNode_voltage[1]->is_valid() || !FromNode_voltage[1]->is_complex())
			{
				GL_THROW("series_compensator:%d - %s - Unable to map property for TO node",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Set the appropriate turns ratio matrices
			d_mat[1][1] = gld::complex(turns_ratio[1],0.0);
			a_mat[1][1] = gld::complex(1.0/turns_ratio[1],0.0);
			A_mat[1][1] = gld::complex(turns_ratio[1],0.0);
		}
		else	//Not here
		{
			ToNode_voltage[1] = nullptr;	//Null for paranoia

			//Set the per-unit setpoint too -- otherwise we'll have issues later
			vset_value[1] = 0.0;
			vset_value_0[1] = 0.0;
		}

		//Check for C
		if (has_phase(PHASE_C))
		{
			//Map to the property of interest - voltage_C
			ToNode_voltage[2] = new gld_property(to,"voltage_C");
			FromNode_voltage[2] = new gld_property(from,"voltage_C");

			//Make sure it worked
			if (!ToNode_voltage[2]->is_valid() || !ToNode_voltage[2]->is_complex())
			{
				GL_THROW("series_compensator:%d - %s - Unable to map property for TO node",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Make sure it worked
			if (!FromNode_voltage[2]->is_valid() || !FromNode_voltage[2]->is_complex())
			{
				GL_THROW("series_compensator:%d - %s - Unable to map property for TO node",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Set the appropriate turns ratio matrices
			d_mat[2][2] = gld::complex(turns_ratio[2],0.0);
			a_mat[2][2] = gld::complex(1.0/turns_ratio[2],0.0);
			A_mat[2][2] = gld::complex(turns_ratio[2],0.0);
		}
		else	//Nope
		{
			ToNode_voltage[2] = nullptr;

			//Set the per-unit setpoint too -- otherwise we'll have issues later
			vset_value[2] = 0.0;
			vset_value_0[2] = 0.0;
		}
	}//End three-phase variants

	//Pull the nominal voltage from our from node
	temp_volt_prop = new gld_property(from,"nominal_voltage");

	//Make sure it worked
	if (!temp_volt_prop->is_valid() || !temp_volt_prop->is_double())
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

		//See if we're triplex or not
		if (has_phase(PHASE_S))
		{
			//Phase 1 parts
			base_admittance_mat[0][0] = gld::complex(1.0/series_compensator_resistance,0.0);
			b_mat[0][0] = series_compensator_resistance;

			//Phase 2 parts
			base_admittance_mat[1][1] = gld::complex(1.0/series_compensator_resistance,0.0);
			b_mat[1][1] = series_compensator_resistance;
		}//end triplex
		else	//Three-phase of some sort
		{
			//Set the baseline impedance values
			if (has_phase(PHASE_A))
			{
				base_admittance_mat[0][0] = gld::complex(1.0/series_compensator_resistance,0.0);
				b_mat[0][0] = series_compensator_resistance;
			}
			if (has_phase(PHASE_B))
			{
				base_admittance_mat[1][1] = gld::complex(1.0/series_compensator_resistance,0.0);
				b_mat[1][1] = series_compensator_resistance;
			}
			if (has_phase(PHASE_C))
			{
				base_admittance_mat[2][2] = gld::complex(1.0/series_compensator_resistance,0.0);
				b_mat[2][2] = series_compensator_resistance;
			}
		}//End three-phase
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

	if (t1 != TS_NEVER_DBL)
		return -t1; 	//Soft return
	else
		return TS_NEVER;
}

//Postsync
TIMESTAMP series_compensator::postsync(TIMESTAMP t0)
{
	int function_return_val;

	double voltage_difference[3];

	bool iterate = false;


	TIMESTAMP t1 = link_object::postsync(t0);


	if (series_compensator_start_time != t0)
	{
		series_compensator_first_step = false;
	}

	if (series_compensator_first_step)
	{

		if (has_phase(PHASE_S))
		{
			//Get raw value
			val_ToNode_voltage[0] = ToNode_voltage[0]->get_complex();
			val_FromNode_voltage[0] = FromNode_voltage[0]->get_complex();
			val_ToNode_voltage[1] = ToNode_voltage[1]->get_complex();
			val_FromNode_voltage[1] = FromNode_voltage[1]->get_complex();

			//Do the per-unit conversion
			val_ToNode_voltage_pu[0] = val_ToNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
			val_FromNode_voltage_pu[0] = val_FromNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
			val_ToNode_voltage_pu[1] = val_ToNode_voltage[1].Mag() / val_FromNode_nominal_voltage;
			val_FromNode_voltage_pu[1] = val_FromNode_voltage[1].Mag() / val_FromNode_nominal_voltage;

			voltage_difference[0] = val_ToNode_voltage_pu[0] - vset_value[0];
			voltage_difference[1] = val_ToNode_voltage_pu[1] - vset_value[1];

			if ( abs(voltage_difference[0]) > voltage_iter_tolerance)
			{
				iterate = true;
				curr_state.n_ini_StateVal[0] = turns_ratio[0] = vset_value[0]/val_FromNode_voltage_pu[0];
			}

			if ( abs(voltage_difference[1]) > voltage_iter_tolerance)
			{
				iterate = true;
				curr_state.n_ini_StateVal[1] = turns_ratio[1] = vset_value[1]/val_FromNode_voltage_pu[1];
			}

		}//end triplex
		else	//Three-phase variant
		{
			//Pull the voltages
			if (has_phase(PHASE_A))
			{
				//Get raw value
				val_ToNode_voltage[0] = ToNode_voltage[0]->get_complex();
				val_FromNode_voltage[0] = FromNode_voltage[0]->get_complex();

				//Do the per-unit conversion
				val_ToNode_voltage_pu[0] = val_ToNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
				val_FromNode_voltage_pu[0] = val_FromNode_voltage[0].Mag() / val_FromNode_nominal_voltage;

				voltage_difference[0] = val_ToNode_voltage_pu[0] - vset_value[0];

				if ( abs(voltage_difference[0]) > voltage_iter_tolerance)
				{
					iterate = true;
					curr_state.n_ini_StateVal[0] = turns_ratio[0] = vset_value[0]/val_FromNode_voltage_pu[0];
				}

			}
			else
			{
				//Zero everything
				val_ToNode_voltage[0] = gld::complex(0.0,0.0);
				val_ToNode_voltage_pu[0] = 0.0;
				val_FromNode_voltage[0] = gld::complex(0.0,0.0);
				val_FromNode_voltage_pu[0] = 0.0;
			}

			//Pull the voltages
			if (has_phase(PHASE_B))
			{
				//Get raw value
				val_ToNode_voltage[1] = ToNode_voltage[1]->get_complex();
				val_FromNode_voltage[1] = FromNode_voltage[1]->get_complex();

				//Do the per-unit conversion
				val_ToNode_voltage_pu[1] = val_ToNode_voltage[1].Mag() / val_FromNode_nominal_voltage;
				val_FromNode_voltage_pu[1] = val_FromNode_voltage[1].Mag() / val_FromNode_nominal_voltage;

				voltage_difference[1] = val_ToNode_voltage_pu[1] - vset_value[1];

				if ( abs(voltage_difference[1]) > voltage_iter_tolerance)
				{
					iterate = true;
					curr_state.n_ini_StateVal[1] = turns_ratio[1] = vset_value[1]/val_FromNode_voltage_pu[1];
				}

			}
			else
			{
				//Zero everything
				val_ToNode_voltage[1] = gld::complex(0.0,0.0);
				val_ToNode_voltage_pu[1] = 0.0;
				val_FromNode_voltage[1] = gld::complex(0.0,0.0);
				val_FromNode_voltage_pu[1] = 0.0;

			}

			//Pull the voltages
			if (has_phase(PHASE_C))
			{
				//Get raw value
				val_ToNode_voltage[2] = ToNode_voltage[2]->get_complex();
				val_FromNode_voltage[2] = FromNode_voltage[2]->get_complex();

				//Do the per-unit conversion
				val_ToNode_voltage_pu[2] = val_ToNode_voltage[2].Mag() / val_FromNode_nominal_voltage;
				val_FromNode_voltage_pu[2] = val_FromNode_voltage[2].Mag() / val_FromNode_nominal_voltage;

				voltage_difference[2] = val_ToNode_voltage_pu[2] - vset_value[2];

				if ( abs(voltage_difference[2]) > voltage_iter_tolerance)
				{
					iterate = true;
					curr_state.n_ini_StateVal[2] = turns_ratio[2] = vset_value[2]/val_FromNode_voltage_pu[2];
				}
			}
			else
			{
				//Zero everything
				val_ToNode_voltage[2] = gld::complex(0.0,0.0);
				val_ToNode_voltage_pu[2] = 0.0;
				val_FromNode_voltage[2] = gld::complex(0.0,0.0);
				val_FromNode_voltage_pu[2] = 0.0;

			}
		}//End three-phase



	}

	function_return_val = sercom_postPost_fxn(0,0);	//Just give it an arbitary value -- used mostly for delta flagging

	if (iterate)
	{
		//If this changes, re-evaluate this code
		return t0;
	}

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
	//See if we're triplex or not
	if (has_phase(PHASE_S))
	{
		//Update turns ratio for various phases
		//Phase 1
		d_mat[0][0] = gld::complex(turns_ratio[0],0.0);
		a_mat[0][0] = gld::complex(1.0/turns_ratio[0],0.0);
		A_mat[0][0] = gld::complex(turns_ratio[0],0.0);

		//Phase 2
		d_mat[1][1] = gld::complex(turns_ratio[1],0.0);
		a_mat[1][1] = gld::complex(1.0/turns_ratio[1],0.0);
		A_mat[1][1] = gld::complex(turns_ratio[1],0.0);
	}
	else	//Nope, normal three-phase
	{
		//Update turns ratio for various phases
		if (has_phase(PHASE_A))
		{
			d_mat[0][0] = gld::complex(turns_ratio[0],0.0);
			a_mat[0][0] = gld::complex(1.0/turns_ratio[0],0.0);
			A_mat[0][0] = gld::complex(turns_ratio[0],0.0);
		}

		if (has_phase(PHASE_B))
		{
			d_mat[1][1] = gld::complex(turns_ratio[1],0.0);
			a_mat[1][1] = gld::complex(1.0/turns_ratio[1],0.0);
			A_mat[1][1] = gld::complex(turns_ratio[1],0.0);
		}

		if (has_phase(PHASE_C))
		{
			d_mat[2][2] = gld::complex(turns_ratio[2],0.0);
			a_mat[2][2] = gld::complex(1.0/turns_ratio[2],0.0);
			A_mat[2][2] = gld::complex(turns_ratio[2],0.0);
		}
	}
}

//Functionalized version of the code for deltamode - "post-link::presync" portions
void series_compensator::sercom_postPre_fxn(void)
{
	char phaseWarn;
	int jindex,kindex;
	gld::complex Ylefttemp[3][3];
	gld::complex Yto[3][3];
	gld::complex Yfrom[3][3];
	gld::complex invratio[3];

	if (solver_method == SM_NR)
	{
		//See if we even need to update
		//Only perform update if a turns_ratio has changed.  Since the link calculations are replicated here and it directly
		//accesses the NR memory space, this won't cause any issues.
		if ((prev_turns_ratio[0] != turns_ratio[0]) || (prev_turns_ratio[1] != turns_ratio[1]) || (prev_turns_ratio[2] != turns_ratio[2]))	//Change has occurred
		{
			//See if we're triplex or not
			if ((NR_branchdata[NR_branch_reference].origphases & 0x80) == 0x80)
			{
				//Now see if we're active
				if ((NR_branchdata[NR_branch_reference].phases & 0x80) == 0x80)
				{
					invratio[0] = gld::complex(1.0,0.0) / a_mat[0][0];
					invratio[1] = gld::complex(1.0,0.0) / a_mat[1][1];
				}
				else
				{
					invratio[0] = gld::complex(0.0,0.0);
					invratio[1] = gld::complex(0.0,0.0);
				}

				//Zero the third one, just because
				invratio[2] = gld::complex(0.0,0.0);
			}
			else	//Do a three-phase check, though it may be disconnected completely
			{
				//Compute the inverse ratio - A
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)
				{
					invratio[0] = gld::complex(1.0,0.0) / a_mat[0][0];
				}
				else
				{
					invratio[0] = gld::complex(0.0,0.0);
				}

				//Compute the inverse ratio - B
				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)
				{
					invratio[1] = gld::complex(1.0,0.0) / a_mat[1][1];
				}
				else
				{
					invratio[1] = gld::complex(0.0,0.0);
				}

				//Compute the inverse ratio - C
				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)
				{
					invratio[2] = gld::complex(1.0,0.0) / a_mat[2][2];
				}
				else
				{
					invratio[2] = gld::complex(0.0,0.0);
				}
			}//End three-phase

			//Get matrices for NR (should be the same for triplex vs normal, with this implementation)

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
int series_compensator::sercom_postPost_fxn(unsigned char pass_value, double deltat)
{
	char index_val;
	unsigned char phase_mask;
	double temp_diff_val[3];
	int return_val;
	bool bypass_initiated;

	//By default, assume we want to exit normal
	return_val = 0;

	val_FromNode_frequency = FromNode_frequency->get_double(); //get the frequency

	//See if we're triplex
	if (has_phase(PHASE_S))
	{
		//Get raw value
		val_ToNode_voltage[0] = ToNode_voltage[0]->get_complex();
		val_FromNode_voltage[0] = FromNode_voltage[0]->get_complex();
		val_ToNode_voltage[1] = ToNode_voltage[1]->get_complex();
		val_FromNode_voltage[1] = FromNode_voltage[1]->get_complex();

		//Do the per-unit conversion
		val_ToNode_voltage_pu[0] = val_ToNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
		val_FromNode_voltage_pu[0] = val_FromNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
		val_ToNode_voltage_pu[1] = val_ToNode_voltage[1].Mag() / val_FromNode_nominal_voltage;
		val_FromNode_voltage_pu[1] = val_FromNode_voltage[1].Mag() / val_FromNode_nominal_voltage;

		//Just set the third to zero, for paranoia sake
		val_ToNode_voltage[2] = gld::complex(0.0,0.0);
		val_ToNode_voltage_pu[2] = 0.0;
		val_FromNode_voltage[2] = gld::complex(0.0,0.0);
		val_FromNode_voltage_pu[2] = 0.0;

	}//end triplex
	else	//Three-phase variant
	{
		//Pull the voltages
		if (has_phase(PHASE_A))
		{
			//Get raw value
			val_ToNode_voltage[0] = ToNode_voltage[0]->get_complex();
			val_FromNode_voltage[0] = FromNode_voltage[0]->get_complex();

			//Do the per-unit conversion
			val_ToNode_voltage_pu[0] = val_ToNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
			val_FromNode_voltage_pu[0] = val_FromNode_voltage[0].Mag() / val_FromNode_nominal_voltage;
		}
		else
		{
			//Zero everything
			val_ToNode_voltage[0] = gld::complex(0.0,0.0);
			val_ToNode_voltage_pu[0] = 0.0;
			val_FromNode_voltage[0] = gld::complex(0.0,0.0);
			val_FromNode_voltage_pu[0] = 0.0;
		}

		//Pull the voltages
		if (has_phase(PHASE_B))
		{
			//Get raw value
			val_ToNode_voltage[1] = ToNode_voltage[1]->get_complex();
			val_FromNode_voltage[1] = FromNode_voltage[1]->get_complex();

			//Do the per-unit conversion
			val_ToNode_voltage_pu[1] = val_ToNode_voltage[1].Mag() / val_FromNode_nominal_voltage;
			val_FromNode_voltage_pu[1] = val_FromNode_voltage[1].Mag() / val_FromNode_nominal_voltage;
		}
		else
		{
			//Zero everything
			val_ToNode_voltage[1] = gld::complex(0.0,0.0);
			val_ToNode_voltage_pu[1] = 0.0;
			val_FromNode_voltage[1] = gld::complex(0.0,0.0);
			val_FromNode_voltage_pu[1] = 0.0;

		}

		//Pull the voltages
		if (has_phase(PHASE_C))
		{
			//Get raw value
			val_ToNode_voltage[2] = ToNode_voltage[2]->get_complex();
			val_FromNode_voltage[2] = FromNode_voltage[2]->get_complex();

			//Do the per-unit conversion
			val_ToNode_voltage_pu[2] = val_ToNode_voltage[2].Mag() / val_FromNode_nominal_voltage;
			val_FromNode_voltage_pu[2] = val_FromNode_voltage[2].Mag() / val_FromNode_nominal_voltage;
		}
		else
		{
			//Zero everything
			val_ToNode_voltage[2] = gld::complex(0.0,0.0);
			val_ToNode_voltage_pu[2] = 0.0;
			val_FromNode_voltage[2] = gld::complex(0.0,0.0);
			val_FromNode_voltage_pu[2] = 0.0;

		}
	}//End three-phase

	if (deltatimestep_running > 0)
	{
		//******************* Code injection point ******************//
		//This may be a place to do the turns ratio update, or in the above logic (with the return value)
		//Probably where the integration-method code would go
		//Need to flag if deltamode or not -- use "" to do that (possibly inside the function)

		// predictor pass

		if (pass_value==0)
		{

			if(frequency_regulation)
			{
				delta_f = val_FromNode_frequency - nominal_frequency;

				if(delta_f > f_db_max)
				{
					delta_V = delta_f * kpf;

					if(delta_V > delta_Vmax)
					{
						delta_V = delta_Vmax;
					}

				}
				else if(delta_f < f_db_min)
				{
					delta_V = delta_f * kpf;

					if(delta_V < delta_Vmin)
					{
						delta_V = delta_Vmin;
					}

				}
				else
				{
					delta_V = 0;
				}
			}
			else if (frequency_open_loop_control)
			{

				// Under-frequency event
				if (t_recover_low_flag) // voltage gradually goes back to nominal value
				{
					t_hold_low_flag = false;
					t_delay_low_flag = false;
					delta_V += recover_rate * deltat;

					if(abs(delta_V) <= V_error)
					{
						t_recover_low_flag = false;
					}
				}

				else if (t_hold_low_flag)  // voltage stays on hold for t_hold time
				{
					delta_V = delta_Vmin;
					t_count_low_hold += deltat;
					t_delay_low_flag = false;
					t_recover_low_flag = false;

					if(t_count_low_hold >= t_hold)
					{
						t_hold_low_flag = false;
						t_recover_low_flag = true;
						t_delay_low_flag = false;
						t_count_low_hold = 0;
					}
				}

				else if ((val_FromNode_frequency < frequency_low) && !t_hold_low_flag && !t_recover_low_flag && !t_delay_high_flag && !t_hold_high_flag && !t_recover_high_flag)
				{
					t_delay_low_flag = true;
					t_count_low_delay += deltat;
					t_hold_low_flag = false;
					t_recover_low_flag = false;

					if (t_count_low_delay >= t_delay)
					{
						delta_V = delta_Vmin;
						t_hold_low_flag = true;
						t_delay_low_flag = false;
						t_recover_low_flag = false;
						t_count_low_delay = 0;
						t_count_low_hold = 0;
					}

				}
				else
				{
					t_delay_low_flag = false;
					t_count_low_delay = 0;
				}

				// Over-frequency event
				if (t_recover_high_flag) // voltage gradually goes back to nominal value
				{
					t_hold_high_flag = false;
					t_delay_high_flag = false;
					delta_V -= recover_rate * deltat;

					if(abs(delta_V) <= V_error)
					{
						t_recover_high_flag = false;
					}
				}

				else if (t_hold_high_flag)  // voltage stays on hold for t_hold time
				{
					delta_V = delta_Vmax;
					t_count_high_hold += deltat;
					t_delay_high_flag = false;
					t_recover_high_flag = false;

					if(t_count_high_hold >= t_hold)
					{
						t_hold_high_flag = false;
						t_recover_high_flag = true;
						t_delay_high_flag = false;
						t_count_high_hold = 0;
					}
				}

				else if ((val_FromNode_frequency > frequency_high) && !t_hold_high_flag && !t_recover_high_flag && !t_delay_low_flag && !t_hold_low_flag && !t_recover_low_flag) // t_delay status
				{
					t_delay_high_flag = true;
					t_count_high_delay += deltat;
					t_hold_high_flag = false;
					t_recover_high_flag = false;

					if (t_count_high_delay >= t_delay)
					{
						delta_V = delta_Vmax;
						t_hold_high_flag = true;
						t_delay_high_flag = false;
						t_recover_high_flag = false;
						t_count_high_delay = 0;
						t_count_high_hold = 0;
					}

				}
				else
				{
					t_delay_high_flag = false;
					t_count_high_delay = 0;
				}

			}
			else
			{
				delta_V = 0;
			}


			//See if we should be triplex
			if ((NR_branchdata[NR_branch_reference].origphases & 0x80) == 0x80)
			{
				//Now see if we're actually active
				if ((NR_branchdata[NR_branch_reference].phases & 0x80) == 0x80)
				{

					//Loop it
					for (index_val=0; index_val<2; index_val++)
					{
						vset_value[index_val] = delta_V + vset_value_0[index_val]; //get the voltage set point according to the frequency regulation requirement

						if (val_FromNode_voltage_pu[index_val] > V_bypass_max_pu)
						{
							n_min[index_val] = 1;  //bypass the compensator
						}
						else
						{
							n_min[index_val] = n_min_ext[index_val];
						}

						if (val_FromNode_voltage_pu[index_val] < V_bypass_min_pu)
						{
							n_max[index_val] = 1; //bypass the compensator
						}
						else
						{
							n_max[index_val] = n_max_ext[index_val];
						}

						//integrator
						pred_state.dn_ini_StateVal[index_val] = (vset_value[index_val] - val_ToNode_voltage_pu[index_val])*ki;
						pred_state.n_ini_StateVal[index_val] = curr_state.n_ini_StateVal[index_val] + pred_state.dn_ini_StateVal[index_val] * deltat;

						//n_max and n_min for integrator output
						if (pred_state.n_ini_StateVal[index_val] > n_max[index_val])
						{
							pred_state.n_ini_StateVal[index_val] = n_max[index_val];
						}

						if (pred_state.n_ini_StateVal[index_val] < n_min[index_val])
						{
							pred_state.n_ini_StateVal[index_val] = n_min[index_val];
						}

						// PI controller output
						pred_state.n_StateVal[index_val] = pred_state.n_ini_StateVal[index_val] + pred_state.dn_ini_StateVal[index_val]/ki*kp;

						//n_max and n_min for PI controller output
						if (pred_state.n_StateVal[index_val] > n_max[index_val])
						{
							pred_state.n_StateVal[index_val] = n_max[index_val];
						}

						if (pred_state.n_StateVal[index_val] < n_min[index_val])
						{
							pred_state.n_StateVal[index_val] = n_min[index_val];
						}

						turns_ratio[index_val] = pred_state.n_StateVal[index_val];
					}

				}//End valid triplex
				//Default else - don't do anything -- does this maybe need to be zeroed?

			}//End triplex
			else	//Some form of three-phase
			{
				//Do it in a loop, because I got sick of copy pasting
				for (index_val=0; index_val<3; index_val++)
				{
					//Get the phase mask
					phase_mask = (1 << (2 - index_val));

					//Check if it is valid
					if ((NR_branchdata[NR_branch_reference].phases & phase_mask) == phase_mask)
					{

						vset_value[index_val] = delta_V + vset_value_0[index_val]; //get the voltage set point according to the frequency regulation requirement

						if (val_FromNode_voltage_pu[index_val] > V_bypass_max_pu)
						{
							n_min[index_val] = 1;  //bypass the compensator
						}
						else
						{
							n_min[index_val] = n_min_ext[index_val];
						}

						if (val_FromNode_voltage_pu[index_val] < V_bypass_min_pu)
						{
							n_max[index_val] = 1; //bypass the compensator
						}
						else
						{
							n_max[index_val] = n_max_ext[index_val];
						}

						//integrator
						pred_state.dn_ini_StateVal[index_val] = (vset_value[index_val] - val_ToNode_voltage_pu[index_val])*ki;
						pred_state.n_ini_StateVal[index_val] = curr_state.n_ini_StateVal[index_val] + pred_state.dn_ini_StateVal[index_val] * deltat;


						//n_max and n_min for integrator output
						if (pred_state.n_ini_StateVal[index_val] > n_max[index_val])
						{
							pred_state.n_ini_StateVal[index_val] = n_max[index_val];
						}

						if (pred_state.n_ini_StateVal[index_val] < n_min[index_val])
						{
							pred_state.n_ini_StateVal[index_val] = n_min[index_val];
						}

						// PI controller output
						pred_state.n_StateVal[index_val] = pred_state.n_ini_StateVal[index_val] + pred_state.dn_ini_StateVal[index_val]/ki*kp;

						//n_max and n_min for PI controller output
						if (pred_state.n_StateVal[index_val] > n_max[index_val])
						{
							pred_state.n_StateVal[index_val] = n_max[index_val];
						}

						if (pred_state.n_StateVal[index_val] < n_min[index_val])
						{
							pred_state.n_StateVal[index_val] = n_min[index_val];
						}

						turns_ratio[index_val] = pred_state.n_StateVal[index_val];
					}//End valid phase
				}//End three-phase loop
			}//End three-phase

			return_val = 1;	//Reiterate - to get us to corrector pass

			//Store return value, for sake of doing so (semi-redundant here)
			deltamode_return_val = return_val;
		}
		else if  (pass_value==1) //corrector pass
		{

			if(frequency_regulation)
			{
				delta_f = val_FromNode_frequency - nominal_frequency;

				if(delta_f > f_db_max)
				{
					delta_V = delta_f * kpf;

					if(delta_V > delta_Vmax)
					{
						delta_V = delta_Vmax;
					}

				}
				else if(delta_f < f_db_min)
				{
					delta_V = delta_f * kpf;

					if(delta_V < delta_Vmin)
					{
						delta_V = delta_Vmin;
					}

				}
				else
				{
					delta_V = 0;
				}
			}
			else if (frequency_open_loop_control)
			{

			}
			else
			{
				delta_V = 0;
			}

			//Check and see if we're triplex
			if ((NR_branchdata[NR_branch_reference].origphases & 0x80) == 0x80)
			{
				//See if we're actually valid
				if ((NR_branchdata[NR_branch_reference].phases & 0x80) == 0x80)
				{
					//Loop the phases
					for (index_val=0; index_val<2; index_val++)
					{

						vset_value[index_val] = delta_V + vset_value_0[index_val]; //get the voltage set point according to the frequency regulation requirement

						//Reset the flag
						bypass_initiated = false;

						if (val_FromNode_voltage_pu[index_val] > V_bypass_max_pu)
						{
							n_min[index_val] = 1;  //bypass the compensator
							bypass_initiated = true;
						}
						else
						{
							n_min[index_val] = n_min_ext[index_val];
						}

						if (val_FromNode_voltage_pu[index_val] < V_bypass_min_pu)
						{
							n_max[index_val] = 1; //bypass the compensator
							bypass_initiated = true;
						}
						else
						{
							n_max[index_val] = n_max_ext[0];
						}

						//Update the state
						if (bypass_initiated)
						{
							phase_states[index_val] = ST_BYPASS;
						}
						else
						{
							phase_states[index_val] = ST_NORMAL;
						}

						next_state.dn_ini_StateVal[index_val] = (vset_value[index_val] - val_ToNode_voltage_pu[index_val])*ki;
						next_state.n_ini_StateVal[index_val] = curr_state.n_ini_StateVal[index_val] + (pred_state.dn_ini_StateVal[index_val] + next_state.dn_ini_StateVal[index_val]) * deltat/2;


						//n_max and n_min for integrator output
						if (next_state.n_ini_StateVal[index_val] > n_max[index_val])
						{
							next_state.n_ini_StateVal[index_val] = n_max[index_val];
						}

						if (next_state.n_ini_StateVal[index_val] < n_min[index_val])
						{
							next_state.n_ini_StateVal[index_val] = n_min[index_val];
						}


						// PI controller output
						next_state.n_StateVal[index_val] = next_state.n_ini_StateVal[index_val] + next_state.dn_ini_StateVal[index_val]/ki*kp;

						//n_max and n_min for PI controller output
						if (next_state.n_StateVal[index_val] > n_max[index_val])
						{
							next_state.n_StateVal[index_val] = n_max[index_val];
						}

						if (next_state.n_StateVal[index_val] < n_min[index_val])
						{
							next_state.n_StateVal[index_val] = n_min[index_val];
						}

						turns_ratio[index_val] = next_state.n_StateVal[index_val];

					}

				}//End valid triplex
				//Default else - disconnected - should we update something?
			}
			else	//Three-phase, of some sort
			{
				//Loop it again, due to laziness/portability
				for (index_val=0; index_val<3; index_val++)
				{
					//Get the phase mask
					phase_mask = (1 << (2 - index_val));

					//Reset the tracking flag
					bypass_initiated = false;

					if ((NR_branchdata[NR_branch_reference].phases & phase_mask) == phase_mask)
					{
						vset_value[index_val] = delta_V + vset_value_0[index_val]; //get the voltage set point according to the frequency regulation requirement

						if (val_FromNode_voltage_pu[index_val] > V_bypass_max_pu)
						{
							n_min[index_val] = 1;  //bypass the compensator
							bypass_initiated = true;
						}
						else
						{
							n_min[index_val] = n_min_ext[0];
						}

						if (val_FromNode_voltage_pu[index_val] < V_bypass_min_pu)
						{
							n_max[index_val] = 1; //bypass the compensator
							bypass_initiated = true;
						}
						else
						{
							n_max[index_val] = n_max_ext[index_val];
						}

						next_state.dn_ini_StateVal[index_val] = (vset_value[index_val] - val_ToNode_voltage_pu[index_val])*ki;
						next_state.n_ini_StateVal[index_val] = curr_state.n_ini_StateVal[index_val] + (pred_state.dn_ini_StateVal[index_val] + next_state.dn_ini_StateVal[index_val]) * deltat/2;

						//Update the state
						if (bypass_initiated)
						{
							phase_states[index_val] = ST_BYPASS;
						}
						else
						{
							phase_states[index_val] = ST_NORMAL;
						}

						//n_max and n_min for integrator output
						if (next_state.n_ini_StateVal[index_val] > n_max[index_val])
						{
							next_state.n_ini_StateVal[index_val] = n_max[index_val];
						}

						if (next_state.n_ini_StateVal[index_val] < n_min[index_val])
						{
							next_state.n_ini_StateVal[index_val] = n_min[index_val];
						}


						// PI controller output
						next_state.n_StateVal[index_val] = next_state.n_ini_StateVal[index_val] + next_state.dn_ini_StateVal[index_val]/ki*kp;

						//n_max and n_min for PI controller output
						if (next_state.n_StateVal[index_val] > n_max[index_val])
						{
							next_state.n_StateVal[index_val] = n_max[index_val];
						}

						if (next_state.n_StateVal[index_val] < n_min[index_val])
						{
							next_state.n_StateVal[index_val] = n_min[index_val];
						}

						turns_ratio[index_val] = next_state.n_StateVal[index_val];

					}//End valid phase
				}//End loop
			}//End three-phase

			return_val =  2;

			//Store return value, for sake of doing so (semi-redundant here)
			deltamode_return_val = return_val;

			memcpy(&curr_state,&next_state,sizeof(SERIES_STATE));
		}
		else	//Other iterations (if more than predictor/corrector - e.g., HELICS)
		{
			//Just read the last value
			return_val = deltamode_return_val;
		}
	}
	else
	{
		//See if we're triplex
		if ((NR_branchdata[NR_branch_reference].origphases & 0x80) == 0x80)
		{
			//See if we're valid triplex
			if ((NR_branchdata[NR_branch_reference].phases & 0x80) == 0x80)
			{
				//Loop it, just because
				for (index_val=0; index_val<2; index_val++)
				{
					temp_diff_val[index_val] = fabs(vset_value[index_val] - val_ToNode_voltage_pu[index_val]);

					//Check it
					if (temp_diff_val[index_val] > voltage_iter_tolerance)
					{
						return_val = 1;	//Arbitrary flag - could be used for other things in the future
					}
				}
			}
			//Default else -- should we do anything?
		}
		else	//Some type of three-phase
		{
			//Loop the phases, for convenience
			for (index_val=0; index_val<3; index_val++)
			{
				//Get the phase mask
				phase_mask = (1 << (2 - index_val));

				if ((NR_branchdata[NR_branch_reference].phases & phase_mask) == phase_mask)
				{
					temp_diff_val[index_val] = fabs(vset_value[index_val] - val_ToNode_voltage_pu[index_val]);

					//Check it
					if (temp_diff_val[index_val] > voltage_iter_tolerance)
					{
						return_val = 1;	//Arbitrary flag - could be used for other things in the future
					}
				}
			}//End loop
		}
	}

	//Return
	return return_val;
}

//Module-level deltamode call
SIMULATIONMODE series_compensator::inter_deltaupdate_series_compensator(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//OBJECT *hdr = OBJECTHDR(this);
	double curr_time_value;	//Current time of simulation
	int temp_return_val;
	double deltat;

	deltat = (double)dt/(double)DT_SECOND;

	//Get the current time
	curr_time_value = gl_globaldeltaclock;

	if (!interupdate_pos)	//Before powerflow call
	{
		//Replicate presync behavior

		//Call the pre-presync regulator code
		sercom_prePre_fxn();

		//Link presync stuff
		NR_link_sync_fxn();
		
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
		temp_return_val = sercom_postPost_fxn(iteration_count_val,deltat);

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
		if (*obj!=nullptr)
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
