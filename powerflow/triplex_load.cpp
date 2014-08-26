/** $Id: triplex_load.cpp 1182 2012-7-1 2:13 PDT fish334 $
	Copyright (C) 2013 Battelle Memorial Institute
	@file triplex_load.cpp
	@addtogroup triplex_load
	@ingroup triplex_node
	
	Triplex_load objects represent static loads and export both voltages and current.  
		
	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "triplex_node.h"
#include "triplex_meter.h"
#include "triplex_load.h"

CLASS* triplex_load::oclass = NULL;
CLASS* triplex_load::pclass = NULL;

triplex_load::triplex_load(MODULE *mod) : triplex_node(mod)
{
	if(oclass == NULL)
	{
		pclass = triplex_node::oclass;
		
		oclass = gl_register_class(mod,"triplex_load",sizeof(triplex_load),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
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
			PT_complex,	"measured_voltage_1",PADDR(measured_voltage_1),PT_DESCRIPTION,"current measured voltage on phase 1",
			PT_complex,	"measured_voltage_2",PADDR(measured_voltage_2),PT_DESCRIPTION,"current measured voltage on phase 2",
			PT_complex,	"measured_voltage_12",PADDR(measured_voltage_12),PT_DESCRIPTION,"current measured voltage on phase 12",

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

         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
				GL_THROW("Unable to publish triplex_load delta_linkage function");
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_triplex_load)==NULL)
				GL_THROW("Unable to publish triplex_load deltamode function");
			if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
				GL_THROW("Unable to publish triplex_load deltamode function");
    }
}

int triplex_load::isa(char *classname)
{
	return strcmp(classname,"triplex_load")==0 || node::isa(classname);
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

    return res;
}

// Initialize, return 1 on success
int triplex_load::init(OBJECT *parent)
{
	return triplex_node::init(parent);
}

TIMESTAMP triplex_load::presync(TIMESTAMP t0)
{
	if ((solver_method!=SM_FBS) && (SubNode==PARENT))	//Need to do something slightly different with GS and parented node
	{
		shunt[0] = shunt[1] = shunt[2] = 0.0;
		power[0] = power[1] = power[2] = 0.0;
		current[0] = current[1] = current[2] = 0.0;
	}
	
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = triplex_node::presync(t0);
	
	return result;
}

TIMESTAMP triplex_load::sync(TIMESTAMP t0)
{
	//bool all_three_phases;
	bool fault_mode;

	//See if we're reliability-enabled
	if (fault_check_object == NULL)
		fault_mode = false;
	else
		fault_mode = true;

	//Functionalized so deltamode can parttake
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

	return t1;
}

//Functional call to sync-level load updates
//Here primarily so deltamode players can actually influence things
void triplex_load::triplex_load_update_fxn()
{
	complex volt = complex(0,0);

	if(base_power[0] != 0.0){// Phase 1
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

			constant_power[0] = complex(real_power,imag_power);
		} else {
			constant_power[0] = complex(0, 0);
		}
		// Put in the constant current portion and adjust the angle
		if(current_fraction[0] != 0.0){
			double real_power, imag_power, temp_angle;
			complex temp_curr;

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
			temp_curr = ~complex(real_power, imag_power)/complex(nominal_voltage, 0);
			temp_angle = temp_curr.Arg() + voltage1.Arg();
			temp_curr.SetPolar(temp_curr.Mag(), temp_angle);

			constant_current[0] = temp_curr;
		} else {
			constant_current[0] = complex(0, 0);
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

			constant_impedance[0] = ~(complex(nominal_voltage*nominal_voltage, 0)/complex(real_power, imag_power));
		} else {
			constant_impedance[0] = complex(0, 0);
		}
	}

	if(base_power[1] != 0.0){// Phase 2
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

			constant_power[1] = complex(real_power,imag_power);
		} else {
			constant_power[1] = complex(0, 0);
		}
		// Put in the constant current portion and adjust the angle
		if(current_fraction[1] != 0.0){
			double real_power, imag_power, temp_angle;
			complex temp_curr;

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
			temp_curr = ~complex(real_power, imag_power)/complex(nominal_voltage, 0);
			temp_angle = temp_curr.Arg() + voltage1.Arg();
			temp_curr.SetPolar(temp_curr.Mag(), temp_angle);

			constant_current[1] = temp_curr;
		} else {
			constant_current[1] = complex(0, 0);
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

			constant_impedance[1] = ~(complex(nominal_voltage*nominal_voltage, 0)/complex(real_power, imag_power));
		} else {
			constant_impedance[1] = complex(0, 0);
		}
	}

	if(base_power[2] != 0.0){// Phase 12
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

			constant_power[2] = complex(real_power,imag_power);
		} else {
			constant_power[2] = complex(0, 0);
		}
		// Put in the constant current portion and adjust the angle
		if(current_fraction[2] != 0.0){
			double real_power, imag_power, temp_angle;
			complex temp_curr;

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
			temp_curr = ~complex(real_power, imag_power)/complex(2*nominal_voltage, 0);
			temp_angle = temp_curr.Arg() + voltage12.Arg();
			temp_curr.SetPolar(temp_curr.Mag(), temp_angle);

			constant_current[2] = temp_curr;
		} else {
			constant_current[2] = complex(0, 0);
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

			constant_impedance[2] = ~(complex(4*nominal_voltage*nominal_voltage, 0)/complex(real_power, imag_power));
		} else {
			constant_impedance[2] = complex(0, 0);
		}
	}

	if ((solver_method!=SM_FBS) && (SubNode==PARENT))	//Need to do something slightly different with GS/NR and parented load
	{													//associated with change due to player methods

		if (!(constant_impedance[0].IsZero()))
			pub_shunt[0] += complex(1.0)/constant_impedance[0];

		if (!(constant_impedance[1].IsZero()))
			pub_shunt[1] += complex(1.0)/constant_impedance[1];
		
		if (!(constant_impedance[2].IsZero()))
			pub_shunt[2] += complex(1.0)/constant_impedance[2];
		
		power1 += constant_power[0];
		power2 += constant_power[1];	
		power12 += constant_power[2];
		current1 += constant_current[0];
		current2 += constant_current[1];
		current12 += constant_current[2];
	}
	else
	{
		if(constant_impedance[0].IsZero())
			pub_shunt[0] = 0.0;
		else
			pub_shunt[0] = complex(1)/constant_impedance[0];

		if(constant_impedance[1].IsZero())
			pub_shunt[1] = 0.0;
		else
			pub_shunt[1] = complex(1)/constant_impedance[1];
		
		if(constant_impedance[2].IsZero())
			pub_shunt[2] = 0.0;
		else
			pub_shunt[2] = complex(1)/constant_impedance[2];
		
		power1 = constant_power[0];
		power2 = constant_power[1];	
		power12 = constant_power[2];
		current1 = constant_current[0];
		current2 = constant_current[1];
		current12 = constant_current[2];
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE triplex_load::inter_deltaupdate_triplex_load(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//unsigned char pass_mod;
	OBJECT *hdr = OBJECTHDR(this);
	bool fault_mode;

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Triplex_load presync items
			if ((solver_method!=SM_FBS) && (SubNode==PARENT))	//Need to do something slightly different with GS and parented node
			{
				shunt[0] = shunt[1] = shunt[2] = 0.0;
				power[0] = power[1] = power[2] = 0.0;
				current[0] = current[1] = current[2] = 0.0;
			}

		//Call triplex-specific call
		BOTH_triplex_node_presync_fxn();

		//Call node presync-equivalent items
		NR_node_presync_fxn();

		//Triplex_load-specific sync calls
			//See if we're reliability-enabled
			if (fault_check_object == NULL)
				fault_mode = false;
			else
				fault_mode = true;

			//Functionalized so deltamode can parttake
			triplex_load_update_fxn();

		//Call sync-equivalent of triplex portion first
		BOTH_triplex_node_sync_fxn();

		//Call node sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Perform node postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

		//Triplex_load-specific postsync items
			measured_voltage_1.SetPolar(voltage1.Mag(),voltage1.Arg());  //Used for testing and xml output
			measured_voltage_2.SetPolar(voltage2.Mag(),voltage2.Arg());
			measured_voltage_12.SetPolar(voltage12.Mag(),voltage12.Arg());

		//No control required at this time - powerflow defers to the whims of other modules
		//Code below implements predictor/corrector-type logic, even though it effectively does nothing
		return SM_EVENT;

		////Do deltamode-related logic
		//if (bustype==SWING)	//We're the SWING bus, control our destiny (which is really controlled elsewhere)
		//{
		//	//See what we're on
		//	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

		//	//Check pass
		//	if (pass_mod==0)	//Predictor pass
		//	{
		//		return SM_DELTA_ITER;	//Reiterate - to get us to corrector pass
		//	}
		//	else	//Corrector pass
		//	{
		//		//As of right now, we're always ready to leave
		//		//Other objects will dictate if we stay (powerflow is indifferent)
		//		return SM_EVENT;
		//	}//End corrector pass
		//}//End SWING bus handling
		//else	//Normal bus
		//{
		//	return SM_EVENT;	//Normal nodes want event mode all the time here - SWING bus will
		//						//control the reiteration process for pred/corr steps
		//}
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
		if (*obj!=NULL)
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
/**@}*/
