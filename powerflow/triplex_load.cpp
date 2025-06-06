/** $Id: triplex_load.cpp 1182 2012-7-1 2:13 PDT fish334 $
	Copyright (C) 2013 Battelle Memorial Institute
	@file triplex_load.cpp
	@addtogroup triplex_load
	@ingroup triplex_node
	
	Triplex_load objects represent static loads and export both voltages and current.  
		
	@{
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "triplex_load.h"

CLASS* triplex_load::oclass = nullptr;
CLASS* triplex_load::pclass = nullptr;

triplex_load::triplex_load(MODULE *mod) : triplex_node(mod)
{
	if(oclass == nullptr)
	{
		pclass = triplex_node::oclass;
		
		oclass = gl_register_class(mod,"triplex_load",sizeof(triplex_load),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class triplex_load";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "triplex_node",
			PT_enumeration, "load_class", PADDR(load_class),PT_DESCRIPTION,"Flag to track load type, not currently used for anything except sorting",
				PT_KEYWORD, "U", (enumeration)LC_UNKNOWN,
				PT_KEYWORD, "R", (enumeration)LC_RESIDENTIAL,
				PT_KEYWORD, "C", (enumeration)LC_COMMERCIAL,
				PT_KEYWORD, "I", (enumeration)LC_INDUSTRIAL,
				PT_KEYWORD, "A", (enumeration)LC_AGRICULTURAL,
			PT_enumeration, "load_priority", PADDR(load_priority),PT_DESCRIPTION,"Load classification based on priority",
				PT_KEYWORD, "DISCRETIONARY", (enumeration)DISCRETIONARY,
				PT_KEYWORD, "PRIORITY", (enumeration)PRIORITY,
				PT_KEYWORD, "CRITICAL", (enumeration)CRITICAL,
			PT_complex, "constant_power_1[VA]", PADDR(constant_power[0]),PT_DESCRIPTION,"constant power load on split phase 1, specified as VA",
			PT_complex, "constant_power_2[VA]", PADDR(constant_power[1]),PT_DESCRIPTION,"constant power load on split phase 2, specified as VA",
			PT_complex, "constant_power_12[VA]", PADDR(constant_power[2]),PT_DESCRIPTION,"constant power load on split phase 12, specified as VA",
			PT_double, "constant_power_1_real[W]", PADDR(constant_power[0].Re()),PT_DESCRIPTION,"constant power load on spit phase 1, real only, specified as W",
			PT_double, "constant_power_2_real[W]", PADDR(constant_power[1].Re()),PT_DESCRIPTION,"constant power load on phase 2, real only, specified as W",
			PT_double, "constant_power_12_real[W]", PADDR(constant_power[2].Re()),PT_DESCRIPTION,"constant power load on phase 12, real only, specified as W",
			PT_double, "constant_power_1_reac[VAr]", PADDR(constant_power[0].Im()),PT_DESCRIPTION,"constant power load on phase 1, imaginary only, specified as VAr",
			PT_double, "constant_power_2_reac[VAr]", PADDR(constant_power[1].Im()),PT_DESCRIPTION,"constant power load on phase 2, imaginary only, specified as VAr",
			PT_double, "constant_power_12_reac[VAr]", PADDR(constant_power[2].Im()),PT_DESCRIPTION,"constant power load on phase 12, imaginary only, specified as VAr",
			PT_complex, "constant_current_1[A]", PADDR(constant_current[0]),PT_DESCRIPTION,"constant current load on phase 1, specified as Amps",
			PT_complex, "constant_current_2[A]", PADDR(constant_current[1]),PT_DESCRIPTION,"constant current load on phase 2, specified as Amps",
			PT_complex, "constant_current_12[A]", PADDR(constant_current[2]),PT_DESCRIPTION,"constant current load on phase 12, specified as Amps",
			PT_double, "constant_current_1_real[A]", PADDR(constant_current[0].Re()),PT_DESCRIPTION,"constant current load on phase 1, real only, specified as Amps",
			PT_double, "constant_current_2_real[A]", PADDR(constant_current[1].Re()),PT_DESCRIPTION,"constant current load on phase 2, real only, specified as Amps",
			PT_double, "constant_current_12_real[A]", PADDR(constant_current[2].Re()),PT_DESCRIPTION,"constant current load on phase 12, real only, specified as Amps",
			PT_double, "constant_current_1_reac[A]", PADDR(constant_current[0].Im()),PT_DESCRIPTION,"constant current load on phase 1, imaginary only, specified as Amps",
			PT_double, "constant_current_2_reac[A]", PADDR(constant_current[1].Im()),PT_DESCRIPTION,"constant current load on phase 2, imaginary only, specified as Amps",
			PT_double, "constant_current_12_reac[A]", PADDR(constant_current[2].Im()),PT_DESCRIPTION,"constant current load on phase 12, imaginary only, specified as Amps",
			PT_complex, "constant_impedance_1[Ohm]", PADDR(constant_impedance[0]),PT_DESCRIPTION,"constant impedance load on phase 1, specified as Ohms",
			PT_complex, "constant_impedance_2[Ohm]", PADDR(constant_impedance[1]),PT_DESCRIPTION,"constant impedance load on phase 2, specified as Ohms",
			PT_complex, "constant_impedance_12[Ohm]", PADDR(constant_impedance[2]),PT_DESCRIPTION,"constant impedance load on phase 12, specified as Ohms",
			PT_double, "constant_impedance_1_real[Ohm]", PADDR(constant_impedance[0].Re()),PT_DESCRIPTION,"constant impedance load on phase 1, real only, specified as Ohms",
			PT_double, "constant_impedance_2_real[Ohm]", PADDR(constant_impedance[1].Re()),PT_DESCRIPTION,"constant impedance load on phase 2, real only, specified as Ohms",
			PT_double, "constant_impedance_12_real[Ohm]", PADDR(constant_impedance[2].Re()),PT_DESCRIPTION,"constant impedance load on phase 12, real only, specified as Ohms",
			PT_double, "constant_impedance_1_reac[Ohm]", PADDR(constant_impedance[0].Im()),PT_DESCRIPTION,"constant impedance load on phase 1, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_2_reac[Ohm]", PADDR(constant_impedance[1].Im()),PT_DESCRIPTION,"constant impedance load on phase 2, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_12_reac[Ohm]", PADDR(constant_impedance[2].Im()),PT_DESCRIPTION,"constant impedance load on phase 12, imaginary only, specified as Ohms",
			PT_complex,	"measured_voltage_1[V]",PADDR(measured_voltage_1),PT_DESCRIPTION,"measured voltage on phase 1",
			PT_complex,	"measured_voltage_2[V]",PADDR(measured_voltage_2),PT_DESCRIPTION,"measured voltage on phase 2",
			PT_complex,	"measured_voltage_12[V]",PADDR(measured_voltage_12),PT_DESCRIPTION,"measured voltage on phase 12",
			PT_complex, "indiv_measured_power_1[VA]",PADDR(measured_power[0]),PT_DESCRIPTION,"current measured power on phase 1",
			PT_complex, "indiv_measured_power_2[VA]",PADDR(measured_power[1]),PT_DESCRIPTION,"current measured power on phase 2",
			PT_complex, "indiv_measured_power_12[VA]",PADDR(measured_power[2]),PT_DESCRIPTION,"current measured power on phase 12",
			PT_complex, "measured_power[VA]",PADDR(measured_total_power),PT_DESCRIPTION,"current total measured power",

			// This allows the user to set a base power on each phase, and specify the power as a function
			// of ZIP and pf for each phase (similar to zipload).  This will override the constant values
			// above if specified, otherwise, constant values work as stated
			PT_double, "base_power_1[VA]",PADDR(base_power[0]),PT_DESCRIPTION,"in similar format as ZIPload, this represents the nominal power on phase 1 before applying ZIP fractions",
			PT_double, "base_power_2[VA]",PADDR(base_power[1]),PT_DESCRIPTION,"in similar format as ZIPload, this represents the nominal power on phase 2 before applying ZIP fractions",
			PT_double, "base_power_12[VA]",PADDR(base_power[2]),PT_DESCRIPTION,"in similar format as ZIPload, this represents the nominal power on phase 12 before applying ZIP fractions",
			PT_double, "power_pf_1[pu]",PADDR(power_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 1 constant power portion of load",
			PT_double, "current_pf_1[pu]",PADDR(current_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 1 constant current portion of load",
			PT_double, "impedance_pf_1[pu]",PADDR(impedance_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 1 constant impedance portion of load",
			PT_double, "power_pf_2[pu]",PADDR(power_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 2 constant power portion of load",
			PT_double, "current_pf_2[pu]",PADDR(current_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 2 constant current portion of load",
			PT_double, "impedance_pf_2[pu]",PADDR(impedance_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 2 constant impedance portion of load",
			PT_double, "power_pf_12[pu]",PADDR(power_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 12 constant power portion of load",
			PT_double, "current_pf_12[pu]",PADDR(current_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 12 constant current portion of load",
			PT_double, "impedance_pf_12[pu]",PADDR(impedance_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase 12 constant impedance portion of load",
			PT_double, "power_fraction_1[pu]",PADDR(power_fraction[0]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase 1",
			PT_double, "current_fraction_1[pu]",PADDR(current_fraction[0]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase 1",
			PT_double, "impedance_fraction_1[pu]",PADDR(impedance_fraction[0]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase 1",
			PT_double, "power_fraction_2[pu]",PADDR(power_fraction[1]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase 2",
			PT_double, "current_fraction_2[pu]",PADDR(current_fraction[1]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase 2",
			PT_double, "impedance_fraction_2[pu]",PADDR(impedance_fraction[1]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase 2",
			PT_double, "power_fraction_12[pu]",PADDR(power_fraction[2]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase 12",
			PT_double, "current_fraction_12[pu]",PADDR(current_fraction[2]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase 12",
			PT_double, "impedance_fraction_12[pu]",PADDR(impedance_fraction[2]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase 12",

         	nullptr) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_triplex_load)==nullptr)
				GL_THROW("Unable to publish triplex_load deltamode function");
			if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==nullptr)
				GL_THROW("Unable to publish triplex_load swing-swapping function");
			if (gl_publish_function(oclass,	"pwr_current_injection_update_map", (FUNCTIONADDR)node_map_current_update_function)==nullptr)
				GL_THROW("Unable to publish triplex_load current injection update mapping function");
			if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==nullptr)
				GL_THROW("Unable to publish triplex_load VFD attachment function");
			if (gl_publish_function(oclass, "pwr_object_reset_disabled_status", (FUNCTIONADDR)node_reset_disabled_status) == nullptr)
				GL_THROW("Unable to publish triplex_load island-status-reset function");
			if (gl_publish_function(oclass, "pwr_object_swing_status_check", (FUNCTIONADDR)node_swing_status) == nullptr)
				GL_THROW("Unable to publish triplex_load swing-status check function");
			if (gl_publish_function(oclass, "pwr_object_load_update", (FUNCTIONADDR)update_triplex_load_values) == nullptr)
				GL_THROW("Unable to publish triplex_load impedance-conversion/update function");
			if (gl_publish_function(oclass, "pwr_object_shunt_update", (FUNCTIONADDR)node_update_shunt_values) == nullptr)
				GL_THROW("Unable to publish triplex_load shunt update function");
    }
}

int triplex_load::isa(char *classname)
{
	return strcmp(classname,"triplex_load")==0 || triplex_node::isa(classname);
}

int triplex_load::create(void)
{
	int res = triplex_node::create();
        
	maximum_voltage_error = 0;
	base_power[0] = base_power[1] = base_power[2] = 0;
	power_fraction[0] = power_fraction[1] = power_fraction[2] = 0;
	current_fraction[0] = current_fraction[1] = current_fraction[2] = 0;
	impedance_fraction[0] = impedance_fraction[1] = impedance_fraction[2] = 0;
	power_pf[0] = power_pf[1] = power_pf[2] = 1;
	current_pf[0] = current_pf[1] = current_pf[2] = 1;
	impedance_pf[0] = impedance_pf[1] = impedance_pf[2] = 1;
	load_class = LC_UNKNOWN;

	base_load_val_was_nonzero[0] = base_load_val_was_nonzero[1] = base_load_val_was_nonzero[2] = false;	//Start deflagged
	ZIP_constant_current[0] = ZIP_constant_current[1] = ZIP_constant_current[2] = false;				//Start assuming not touched

	prev_load_values[0][0] = prev_load_values[0][1] = prev_load_values[0][2] = gld::complex(0.0,0.0);
	prev_load_values[1][0] = prev_load_values[1][1] = prev_load_values[1][2] = gld::complex(0.0,0.0);
	prev_load_values[2][0] = prev_load_values[2][1] = prev_load_values[2][2] = gld::complex(0.0,0.0);

    return res;
}

// Initialize, return 1 on success
int triplex_load::init(OBJECT *parent)
{
	int ret_value;
	OBJECT *obj = OBJECTHDR(this);

	ret_value = triplex_node::init(parent);

	if (((constant_current[0] != 0.0) && (base_power[0] != 0.0)) ||
		((constant_current[1] != 0.0) && (base_power[1] != 0.0)) ||
		((constant_current[2] != 0.0) && (base_power[2] != 0.0)))
	{
		gl_warning("triplex_load:%s - ZIP fractions will override constant_current_X values", obj->name ? obj->name : "unnamed");
		/*  TROUBLESHOOT
		In triplex_load objects, if constant_current_X is specified and a base_power_X has a constant current portion, the base_power_X-based
		values will override constant_current_X and inputs to constant_current_X will be ignored for the remainder of the simulation.
		*/
	}

	return ret_value;
}

TIMESTAMP triplex_load::presync(TIMESTAMP t0)
{
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = triplex_node::presync(t0);
	
	return result;
}

TIMESTAMP triplex_load::sync(TIMESTAMP t0)
{
	//Functionalized load update - so deltamode can parttake
	triplex_load_update_fxn();

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = triplex_node::sync(t0);

	return result;
}

TIMESTAMP triplex_load::postsync(TIMESTAMP t0)
{
	//Call node postsync first, otherwise the properties below end up 1 time out of sync (for PCed loads anyways)
	TIMESTAMP t1 = triplex_node::postsync(t0);

	//Moved from sync.  Hopefully it doesn't mess anything up, but these properties are 99% stupid and useless anyways, so meh
	measured_voltage_1.SetPolar(voltage1.Mag(),voltage1.Arg());  //Used for testing and xml output
	measured_voltage_2.SetPolar(voltage2.Mag(),voltage2.Arg());
	measured_voltage_12.SetPolar(voltage12.Mag(),voltage12.Arg());

	measured_power[0] = voltage[0]*(~current_inj[0]);
	measured_power[1] = -(voltage[1]*(~current_inj[1]));
	measured_power[2] = voltage[2]*(~(-(current_inj[1]+current_inj[0])));	//Voltage_N
	measured_total_power = measured_power[0] + measured_power[1] + measured_power[2];

	return t1;
}

//Functional call to sync-level load updates
//Here primarily so deltamode players can actually influence things, as well as constant current rotations
void triplex_load::triplex_load_update_fxn(void)
{
	gld::complex intermed_impedance[3];
	int index_var;
	gld::complex adjusted_current[3];
	gld::complex adjust_temp_nominal_voltage[3];
	double adjust_temp_voltage_mag[3];
	double adjust_nominal_voltage_val[3];
	gld::complex adjust_voltage_val[3];
	gld::complex working_impedance_value;

	//Roll GFA check into here, so current loads updates are handled properly
	//See if GFA functionality is enabled
	if (GFA_enable)
	{
		//See if we're enabled - just skipping the load update should be enough, if we are not
		if (!GFA_status)
		{
			//Remove any load contributions
			triplex_load_delete_update_fxn();

			//Exit
			return;
		}
	}
	//Default elses - just continue like normal

	//Remove prior contributions and zero the accumulators
	shunt[0] -= prev_load_values[0][0];
	shunt[1] -= prev_load_values[0][1];
	shunt[2] -= prev_load_values[0][2];

	current[0] -= prev_load_values[1][0];
	current[1] -= prev_load_values[1][1];
	current12 -= prev_load_values[1][2];	//12 is a separate variable - [2] is N

	power[0] -= prev_load_values[2][0];
	power[1] -= prev_load_values[2][1];
	power[2] -= prev_load_values[2][2];

	//Update voltage12, since that wouldn't happen yet
	voltage12 = voltage1 + voltage2;

	//Zero the accumulators
	for (index_var=0; index_var<3; index_var++)
	{
		prev_load_values[0][index_var] = gld::complex(0.0,0.0);
		prev_load_values[1][index_var] = gld::complex(0.0,0.0);
		prev_load_values[2][index_var] = gld::complex(0.0,0.0);
	}

	if(base_power[0] != 0.0){// Phase 1

		//Set the flag
		base_load_val_was_nonzero[0] = true;

		if (power_fraction[0] + current_fraction[0] + impedance_fraction[0] != 1.0)
		{	
			power_fraction[0] = 1 - current_fraction[0] - impedance_fraction[0];

			OBJECT *obj = OBJECTHDR(this);

			gl_warning("load:%s - ZIP components on phase 1 did not sum to 1. Setting power_fraction to %.2f", obj->name ? obj->name : "unnamed", power_fraction[0]);
			/*  TROUBLESHOOT
			ZIP load fractions must sum to 1.  The values (power_fraction_1, impedance_fraction_1, and current_fraction_1) did not sum to 1. To 
			ensure that constraint (Z+I+P=1), power_fraction is being calculated and overwritten.
			*/
		}


		// Put in the constant power portion
		if (power_fraction[0] != 0.0){
			double real_power, imag_power;
			if(power_pf[0] == 0.0){
				real_power = 0.0;
				imag_power = base_power[0]*power_fraction[0];
			} else {
				real_power = base_power[0]*power_fraction[0]*fabs(power_pf[0]);
				imag_power = real_power*sqrt(1.0/(power_pf[0]*power_pf[0]) - 1.0);
			}

			if(power_pf[0] < 0.0){
				imag_power *= -1.0;
			}

			constant_power[0] = gld::complex(real_power,imag_power);
		} else {
			constant_power[0] = gld::complex(0, 0);
		}
		// Put in the constant current portion and adjust the angle
		if(current_fraction[0] != 0.0){
			double real_power, imag_power, temp_angle;
			gld::complex temp_curr;

			if(current_pf[0] == 0.0){				
				real_power = 0.0;
				imag_power = base_power[0]*current_fraction[0];
			} else {
				real_power = base_power[0]*current_fraction[0]*fabs(current_pf[0]);
				imag_power = real_power*sqrt(1.0/(current_pf[0]*current_pf[0]) - 1.0);
			}

			if(current_pf[0] < 0){
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}
			
			// Calculate then shift the constant current to use the posted voltage as the reference angle
			temp_curr = ~gld::complex(real_power, imag_power)/gld::complex(nominal_voltage, 0);
			temp_angle = temp_curr.Arg() + voltage1.Arg();
			temp_curr.SetPolar(temp_curr.Mag(), temp_angle);

			constant_current[0] = temp_curr;

			//Set the flag
			ZIP_constant_current[0] = true;
		} else {
			constant_current[0] = gld::complex(0, 0);
		}
		// Put in the constant impedance portion
		if(impedance_fraction[0] != 0.0){
			double real_power, imag_power;

			if (impedance_pf[0] == 0.0)
			{				
				real_power = 0.0;
				imag_power = base_power[0]*impedance_fraction[0];
			}
			else
			{
				real_power = base_power[0]*impedance_fraction[0]*fabs(impedance_pf[0]);
				imag_power = real_power*sqrt(1.0/(impedance_pf[0]*impedance_pf[0]) - 1.0);
			}

			if (impedance_pf[0] < 0)
			{
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}

			constant_impedance[0] = ~(gld::complex(nominal_voltage*nominal_voltage, 0)/gld::complex(real_power, imag_power));
		} else {
			constant_impedance[0] = gld::complex(0, 0);
		}
	}
	else if (base_load_val_was_nonzero[0])	//Zero case, be sure to re-zero it
	{
		//zero all components
		constant_power[0] = gld::complex(0.0,0.0);
		constant_current[0] = gld::complex(0.0,0.0);
		constant_impedance[0] = gld::complex(0.0,0.0);

		//Deflag us
		base_load_val_was_nonzero[0] = false;
	}

	if(base_power[1] != 0.0){// Phase 2

		//Set the flag
		base_load_val_was_nonzero[1] = true;

		if (power_fraction[1] + current_fraction[1] + impedance_fraction[1] != 1.0)
		{	
			power_fraction[1] = 1 - current_fraction[1] - impedance_fraction[1];

			OBJECT *obj = OBJECTHDR(this);

			gl_warning("load:%s - ZIP components on phase 2 did not sum to 1. Setting power_fraction to %.2f", obj->name ? obj->name : "unnamed", power_fraction[1]);
			/*  TROUBLESHOOT
			ZIP load fractions must sum to 1.  The values (power_fraction_2, impedance_fraction_2, and current_fraction_2) did not sum to 1. To 
			ensure that constraint (Z+I+P=1), power_fraction is being calculated and overwritten.
			*/
		}
		// Put in the constant power portion
		if (power_fraction[1] != 0.0){
			double real_power, imag_power;
			if(power_pf[1] == 0.0){
				real_power = 0.0;
				imag_power = base_power[1]*power_fraction[1];
			} else {
				real_power = base_power[1]*power_fraction[1]*fabs(power_pf[1]);
				imag_power = real_power*sqrt(1.0/(power_pf[1]*power_pf[1]) - 1.0);
			}

			if(power_pf[1] < 0.0){
				imag_power *= -1.0;
			}

			constant_power[1] = gld::complex(real_power,imag_power);
		} else {
			constant_power[1] = gld::complex(0, 0);
		}
		// Put in the constant current portion and adjust the angle
		if(current_fraction[1] != 0.0){
			double real_power, imag_power, temp_angle;
			gld::complex temp_curr;

			if(current_pf[1] == 0.0){				
				real_power = 0.0;
				imag_power = base_power[1]*current_fraction[1];
			} else {
				real_power = base_power[1]*current_fraction[1]*fabs(current_pf[1]);
				imag_power = real_power*sqrt(1.0/(current_pf[1]*current_pf[1]) - 1.0);
			}

			if(current_pf[1] < 0){
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}
			
			// Calculate then shift the constant current to use the posted voltage as the reference angle
			temp_curr = ~gld::complex(real_power, imag_power)/gld::complex(nominal_voltage, 0);
			temp_angle = temp_curr.Arg() + voltage2.Arg();
			temp_curr.SetPolar(temp_curr.Mag(), temp_angle);

			constant_current[1] = temp_curr;

			//Set the flag
			ZIP_constant_current[1] = true;
		} else {
			constant_current[1] = gld::complex(0, 0);
		}
		// Put in the constant impedance portion
		if(impedance_fraction[1] != 0.0){
			double real_power, imag_power;

			if (impedance_pf[1] == 0.0)
			{				
				real_power = 0.0;
				imag_power = base_power[1]*impedance_fraction[1];
			}
			else
			{
				real_power = base_power[1]*impedance_fraction[1]*fabs(impedance_pf[1]);
				imag_power = real_power*sqrt(1.0/(impedance_pf[1]*impedance_pf[1]) - 1.0);
			}

			if (impedance_pf[1] < 0)
			{
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}

			constant_impedance[1] = ~(gld::complex(nominal_voltage*nominal_voltage, 0)/gld::complex(real_power, imag_power));
		} else {
			constant_impedance[1] = gld::complex(0, 0);
		}
	}
	else if (base_load_val_was_nonzero[1])	//Zero case, be sure to re-zero it
	{
		//zero all components
		constant_power[1] = gld::complex(0.0,0.0);
		constant_current[1] = gld::complex(0.0,0.0);
		constant_impedance[1] = gld::complex(0.0,0.0);

		//Deflag us
		base_load_val_was_nonzero[1] = false;
	}

	if(base_power[2] != 0.0){// Phase 12

		//Set the flag
		base_load_val_was_nonzero[2] = true;

		if (power_fraction[2] + current_fraction[2] + impedance_fraction[2] != 1.0)
		{	
			power_fraction[2] = 1 - current_fraction[2] - impedance_fraction[2];

			OBJECT *obj = OBJECTHDR(this);

			gl_warning("load:%s - ZIP components on phase 12 did not sum to 1. Setting power_fraction to %.2f", obj->name ? obj->name : "unnamed", power_fraction[2]);
			/*  TROUBLESHOOT
			ZIP load fractions must sum to 1.  The values (power_fraction_12, impedance_fraction_12, and current_fraction_12) did not sum to 1. To 
			ensure that constraint (Z+I+P=1), power_fraction is being calculated and overwritten.
			*/
		}
		// Put in the constant power portion
		if (power_fraction[2] != 0.0){
			double real_power, imag_power;
			if(power_pf[2] == 0.0){
				real_power = 0.0;
				imag_power = base_power[2]*power_fraction[2];
			} else {
				real_power = base_power[2]*power_fraction[2]*fabs(power_pf[2]);
				imag_power = real_power*sqrt(1.0/(power_pf[2]*power_pf[2]) - 1.0);
			}

			if(power_pf[2] < 0.0){
				imag_power *= -1.0;
			}

			constant_power[2] = gld::complex(real_power,imag_power);
		} else {
			constant_power[2] = gld::complex(0, 0);
		}
		// Put in the constant current portion and adjust the angle
		if(current_fraction[2] != 0.0){
			double real_power, imag_power, temp_angle;
			gld::complex temp_curr;

			if(current_pf[2] == 0.0){				
				real_power = 0.0;
				imag_power = base_power[2]*current_fraction[2];
			} else {
				real_power = base_power[2]*current_fraction[2]*fabs(current_pf[2]);
				imag_power = real_power*sqrt(1.0/(current_pf[2]*current_pf[2]) - 1.0);
			}

			if(current_pf[2] < 0){
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}
			
			// Calculate then shift the constant current to use the posted voltage as the reference angle
			//Note that the 2*nominal_voltage is to account for the 240 V connection
			temp_curr = ~gld::complex(real_power, imag_power)/gld::complex(2.0*nominal_voltage, 0);
			temp_angle = temp_curr.Arg() + voltage12.Arg();
			temp_curr.SetPolar(temp_curr.Mag(), temp_angle);

			constant_current[2] = temp_curr;

			//Set the flag
			ZIP_constant_current[2] = true;
		} else {
			constant_current[2] = gld::complex(0, 0);
		}
		// Put in the constant impedance portion
		if(impedance_fraction[2] != 0.0){
			double real_power, imag_power;

			if (impedance_pf[2] == 0.0)
			{				
				real_power = 0.0;
				imag_power = base_power[2]*impedance_fraction[2];
			}
			else
			{
				real_power = base_power[2]*impedance_fraction[2]*fabs(impedance_pf[2]);
				imag_power = real_power*sqrt(1.0/(impedance_pf[2]*impedance_pf[2]) - 1.0);
			}

			if (impedance_pf[2] < 0)
			{
				imag_power *= -1.0;	//Adjust imaginary portion for negative PF
			}

			//Note that nominal_voltage is 120, so need to double for 240, the 4 is 2^2
			constant_impedance[2] = ~(gld::complex(4*nominal_voltage*nominal_voltage, 0)/gld::complex(real_power, imag_power));
		} else {
			constant_impedance[2] = gld::complex(0, 0);
		}
	}
	else if (base_load_val_was_nonzero[2])	//Zero case, be sure to re-zero it
	{
		//zero all components
		constant_power[2] = gld::complex(0.0,0.0);
		constant_current[2] = gld::complex(0.0,0.0);
		constant_impedance[2] = gld::complex(0.0,0.0);

		//Deflag us
		base_load_val_was_nonzero[2] = false;
	}

	//Apply any frequency dependencies, if relevant
	//Perform the intermediate impedance calculations, if necessary
	if (enable_frequency_dependence)
	{
		//Check impedance values for normal connections - 1,2,12
		for (index_var=0; index_var<3; index_var++)
		{
			if (!constant_impedance[index_var].IsZero())
			{
				//Assign real part
				intermed_impedance[index_var].SetReal(constant_impedance[index_var].Re());
				
				//Assign reactive part -- apply fix based on inductance/capacitance
				if (constant_impedance[index_var].Im()<0)	//Capacitive
				{
					intermed_impedance[index_var].SetImag(constant_impedance[index_var].Im()/current_frequency*nominal_frequency);
				}
				else	//Inductive
				{
					intermed_impedance[index_var].SetImag(constant_impedance[index_var].Im()/nominal_frequency*current_frequency);
				}
			}
			else
			{
				intermed_impedance[index_var] = 0.0;
			}
		}
	}
	else //No frequency dependence
	{
		//Just copy in
		intermed_impedance[0] = constant_impedance[0];
		intermed_impedance[1] = constant_impedance[1];
		intermed_impedance[2] = constant_impedance[2];
	}
	
	//See how to accumulate the various values
	if(!intermed_impedance[0].IsZero())
	{
		//Accumulator
		shunt[0] += gld::complex(1.0)/intermed_impedance[0];

		//Tracker
		prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
	}

	if(!intermed_impedance[1].IsZero())
	{
		//Accumulator
		shunt[1] += gld::complex(1.0)/intermed_impedance[1];

		//Tracker
		prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
	}
	
	if(!intermed_impedance[2].IsZero())
	{
		//Accumulator
		shunt[2] += gld::complex(1.0)/intermed_impedance[2];

		//Tracker
		prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
	}
	
	//Power and current accumulators
	power[0] += constant_power[0];
	power[1] += constant_power[1];	
	power[2] += constant_power[2];

	//Adjust values for both - FBS/NR are identical here, due to triplex nature

	//Compute constants (just do it once)
	//Set nominal voltage values - set up for explicit down below
	adjust_nominal_voltage_val[0] = nominal_voltage;		//1N
	adjust_nominal_voltage_val[1] = nominal_voltage;		//2N
	adjust_nominal_voltage_val[2] = nominal_voltage * 2.0;	//12

	//Just copy in the voltages (and compute delta) - mostly for explicit calculation, so the loop can just use it
	adjust_voltage_val[0] = voltage[0];
	adjust_voltage_val[1] = voltage[1];
	adjust_voltage_val[2] = voltage1 + voltage2;

	//Create the nominal voltage vectors - delta first
	adjust_temp_nominal_voltage[0].SetPolar(adjust_nominal_voltage_val[0],0.0);
	adjust_temp_nominal_voltage[1].SetPolar(adjust_nominal_voltage_val[1],0.0);
	adjust_temp_nominal_voltage[2].SetPolar(adjust_nominal_voltage_val[2],0.0);

	//Get magnitudes of all
	adjust_temp_voltage_mag[0] = adjust_voltage_val[0].Mag();
	adjust_temp_voltage_mag[1] = adjust_voltage_val[1].Mag();
	adjust_temp_voltage_mag[2] = adjust_voltage_val[2].Mag();

	//Start adjustments - loop them
	for (index_var=0; index_var<3; index_var++)
	{
		if ((constant_current[index_var] != 0.0) && (adjust_temp_voltage_mag[index_var] != 0.0) && !ZIP_constant_current[index_var])
		{
			//calculate new value
			adjusted_current[index_var] = ~(adjust_temp_nominal_voltage[index_var] * ~constant_current[index_var] * adjust_temp_voltage_mag[index_var] / (adjust_voltage_val[index_var] * adjust_nominal_voltage_val[index_var]));
		}
		else
		{
			if (!ZIP_constant_current[index_var])
			{
				adjusted_current[index_var] = gld::complex(0.0,0.0);
			}
			else
			{
				adjusted_current[index_var] = constant_current[index_var];
			}
		}
	}

	//Update the accumulator
	current[0] += adjusted_current[0];
	current[1] += adjusted_current[1];
	current12 += adjusted_current[2];

	//Current trackers
	prev_load_values[1][0] += adjusted_current[0];
	prev_load_values[1][1] += adjusted_current[1];
	prev_load_values[1][2] += adjusted_current[2];

	//Power trackers
	prev_load_values[2][0] += constant_power[0];
	prev_load_values[2][1] += constant_power[1];
	prev_load_values[2][2] += constant_power[2];
}

//Function to appropriately zero load - make sure we don't get too heavy handed
void triplex_load::triplex_load_delete_update_fxn(void)
{
	int index_var, extract_node_ref;

	//If FPI, remove the shunt portions
	if (NR_solver_algorithm == NRM_FPI)
	{
		//See if we're a parented load or not - not sure how we'd ever be a "different child", but copy paste!
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
		{
			extract_node_ref = NR_node_reference;
		}
		else	//It is a child - look at parent
		{
			extract_node_ref = *NR_subnode_reference;
		}

		//Remove the components
		NR_busdata[extract_node_ref].full_Y_load[0] -= prev_load_values[0][0];
		NR_busdata[extract_node_ref].full_Y_load[4] -= prev_load_values[0][1];
		NR_busdata[extract_node_ref].full_Y_load[8] -= prev_load_values[0][2];

		//Flag the FPI change
		NR_FPI_imp_load_change = true;
	}//End FPI

	//Loop and clear
	for (index_var=0; index_var<3; index_var++)
	{
		//Remove contributions - power and shunt
		shunt[index_var] -= prev_load_values[0][index_var];
		power[index_var] -= prev_load_values[2][index_var];
	}

	//Do current independently, since it is "odd"
	current[0] -= prev_load_values[1][0];
	current[1] -= prev_load_values[1][1];
	current12 -= prev_load_values[1][2];

	//Zero the accumulators
	for (index_var=0; index_var<3; index_var++)
	{
		prev_load_values[0][index_var] = gld::complex(0.0,0.0);
		prev_load_values[1][index_var] = gld::complex(0.0,0.0);
		prev_load_values[2][index_var] = gld::complex(0.0,0.0);
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE triplex_load::inter_deltaupdate_triplex_load(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	OBJECT *hdr = OBJECTHDR(this);
	double deltat;
	STATUS return_status_val;

	//Create delta_t variable
	deltat = (double)dt/(double)DT_SECOND;

	//Update time tracking variable - mostly for GFA functionality calls
	if ((iteration_count_val==0) && !interupdate_pos) //Only update timestamp tracker on first iteration
	{
		//Update tracking variable
		prev_time_dbl = gl_globaldeltaclock;

		//Update frequency calculation values (if needed)
		if (fmeas_type != FM_NONE)
		{
			//See which pass
			if (delta_time == 0)
			{
				//Initialize dynamics - first run of new delta call
				init_freq_dynamics(deltat);
			}
			else
			{
				//Copy the tracker value
				memcpy(&prev_freq_state,&curr_freq_state,sizeof(FREQM_STATES));
			}
		}
	}

	//Perform the GFA update, if enabled
	if (GFA_enable && (iteration_count_val == 0) && !interupdate_pos)	//Always just do on the first pass
	{
		//Do the checks
		GFA_Update_time = perform_GFA_checks(deltat);
	}

	if (!interupdate_pos)	//Before powerflow call
	{
		//Call node presync-equivalent items
		NR_node_presync_fxn(0);

		//Functionalized load updates - so deltamode can parttake
		triplex_load_update_fxn();

		//Call node sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call

	}//End Before NR solver (or inclusive)
	else	//After the call
	{
		//Perform node postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

		//Triplex_load-specific postsync items
		measured_voltage_1.SetPolar(voltage1.Mag(),voltage1.Arg());  //Used for testing and xml output
		measured_voltage_2.SetPolar(voltage2.Mag(),voltage2.Arg());
		measured_voltage_12.SetPolar(voltage12.Mag(),voltage12.Arg());

		measured_power[0] = voltage[0]*(~current_inj[0]);
		measured_power[1] = -(voltage[1]*(~current_inj[1]));
		measured_power[2] = voltage[2]*(~(-(current_inj[1]+current_inj[0])));	//Voltage_N
		measured_total_power = measured_power[0] + measured_power[1] + measured_power[2];

		//Frequency measurement stuff
		if (fmeas_type != FM_NONE)
		{
			return_status_val = calc_freq_dynamics(deltat);

			//Check it
			if (return_status_val == FAILED)
			{
				return SM_ERROR;
			}
		}//End frequency measurement desired
		//Default else -- don't calculate it

		//See if GFA functionality is required, since it may require iterations or "continance"
		if (GFA_enable)
		{
			//See if our return is value
			if ((GFA_Update_time > 0.0) && (GFA_Update_time < 1.7))
			{
				//Force us to stay
				return SM_DELTA;
			}
			else	//Just return whatever we were going to do
			{
				return SM_EVENT;
			}
		}
		else	//Normal mode
		{
			return SM_EVENT;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: triplex_load
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_triplex_load(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(triplex_load::oclass);
		if (*obj!=nullptr)
		{
			triplex_load *my = OBJECTDATA(*obj,triplex_load);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(triplex_load);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to the object to be initialized
* @return 1 on success, 0 on error
*/
EXPORT int init_triplex_load(OBJECT *obj)
{
	try {
		triplex_load *my = OBJECTDATA(obj,triplex_load);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(triplex_load);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_triplex_load(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		triplex_load *pObj = OBJECTDATA(obj,triplex_load);
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
	SYNC_CATCHALL(triplex_load);
}

EXPORT int isa_triplex_load(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_load)->isa(classname);
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_triplex_load(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	triplex_load *my = OBJECTDATA(obj,triplex_load);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_triplex_load(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_triplex_load(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//Exposed function to do load update - primarily to get impedance conversion into solver_nr directly
EXPORT STATUS update_triplex_load_values(OBJECT *obj)
{
	triplex_load *my = OBJECTDATA(obj,triplex_load);

	//Call the update
	my->triplex_load_update_fxn();

	//Return us - always succeed, for now
	return SUCCESS;
}
/**@}*/
