/** $Id: load.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file load.cpp
	@addtogroup load
	@ingroup node
	
	Load objects represent static loads and export both voltages and current.  
		
	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "node.h"
#include "meter.h"

CLASS* load::oclass = NULL;
CLASS* load::pclass = NULL;

load::load(MODULE *mod) : node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"load",sizeof(load),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class load";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_enumeration, "load_class", PADDR(load_class),PT_DESCRIPTION,"Flag to track load type, not currently used for anything except sorting",
				PT_KEYWORD, "U", (enumeration)LC_UNKNOWN,
				PT_KEYWORD, "R", (enumeration)LC_RESIDENTIAL,
				PT_KEYWORD, "C", (enumeration)LC_COMMERCIAL,
				PT_KEYWORD, "I", (enumeration)LC_INDUSTRIAL,
				PT_KEYWORD, "A", (enumeration)LC_AGRICULTURAL,
			PT_complex, "constant_power_A[VA]", PADDR(constant_power[0]),PT_DESCRIPTION,"constant power load on phase A, specified as VA",
			PT_complex, "constant_power_B[VA]", PADDR(constant_power[1]),PT_DESCRIPTION,"constant power load on phase B, specified as VA",
			PT_complex, "constant_power_C[VA]", PADDR(constant_power[2]),PT_DESCRIPTION,"constant power load on phase C, specified as VA",
			PT_double, "constant_power_A_real[W]", PADDR(constant_power[0].Re()),PT_DESCRIPTION,"constant power load on phase A, real only, specified as W",
			PT_double, "constant_power_B_real[W]", PADDR(constant_power[1].Re()),PT_DESCRIPTION,"constant power load on phase B, real only, specified as W",
			PT_double, "constant_power_C_real[W]", PADDR(constant_power[2].Re()),PT_DESCRIPTION,"constant power load on phase C, real only, specified as W",
			PT_double, "constant_power_A_reac[VAr]", PADDR(constant_power[0].Im()),PT_DESCRIPTION,"constant power load on phase A, imaginary only, specified as VAr",
			PT_double, "constant_power_B_reac[VAr]", PADDR(constant_power[1].Im()),PT_DESCRIPTION,"constant power load on phase B, imaginary only, specified as VAr",
			PT_double, "constant_power_C_reac[VAr]", PADDR(constant_power[2].Im()),PT_DESCRIPTION,"constant power load on phase C, imaginary only, specified as VAr",
			PT_complex, "constant_current_A[A]", PADDR(constant_current[0]),PT_DESCRIPTION,"constant current load on phase A, specified as Amps",
			PT_complex, "constant_current_B[A]", PADDR(constant_current[1]),PT_DESCRIPTION,"constant current load on phase B, specified as Amps",
			PT_complex, "constant_current_C[A]", PADDR(constant_current[2]),PT_DESCRIPTION,"constant current load on phase C, specified as Amps",
			PT_double, "constant_current_A_real[A]", PADDR(constant_current[0].Re()),PT_DESCRIPTION,"constant current load on phase A, real only, specified as Amps",
			PT_double, "constant_current_B_real[A]", PADDR(constant_current[1].Re()),PT_DESCRIPTION,"constant current load on phase B, real only, specified as Amps",
			PT_double, "constant_current_C_real[A]", PADDR(constant_current[2].Re()),PT_DESCRIPTION,"constant current load on phase C, real only, specified as Amps",
			PT_double, "constant_current_A_reac[A]", PADDR(constant_current[0].Im()),PT_DESCRIPTION,"constant current load on phase A, imaginary only, specified as Amps",
			PT_double, "constant_current_B_reac[A]", PADDR(constant_current[1].Im()),PT_DESCRIPTION,"constant current load on phase B, imaginary only, specified as Amps",
			PT_double, "constant_current_C_reac[A]", PADDR(constant_current[2].Im()),PT_DESCRIPTION,"constant current load on phase C, imaginary only, specified as Amps",
			PT_complex, "constant_impedance_A[Ohm]", PADDR(constant_impedance[0]),PT_DESCRIPTION,"constant impedance load on phase A, specified as Ohms",
			PT_complex, "constant_impedance_B[Ohm]", PADDR(constant_impedance[1]),PT_DESCRIPTION,"constant impedance load on phase B, specified as Ohms",
			PT_complex, "constant_impedance_C[Ohm]", PADDR(constant_impedance[2]),PT_DESCRIPTION,"constant impedance load on phase C, specified as Ohms",
			PT_double, "constant_impedance_A_real[Ohm]", PADDR(constant_impedance[0].Re()),PT_DESCRIPTION,"constant impedance load on phase A, real only, specified as Ohms",
			PT_double, "constant_impedance_B_real[Ohm]", PADDR(constant_impedance[1].Re()),PT_DESCRIPTION,"constant impedance load on phase B, real only, specified as Ohms",
			PT_double, "constant_impedance_C_real[Ohm]", PADDR(constant_impedance[2].Re()),PT_DESCRIPTION,"constant impedance load on phase C, real only, specified as Ohms",
			PT_double, "constant_impedance_A_reac[Ohm]", PADDR(constant_impedance[0].Im()),PT_DESCRIPTION,"constant impedance load on phase A, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_B_reac[Ohm]", PADDR(constant_impedance[1].Im()),PT_DESCRIPTION,"constant impedance load on phase B, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_C_reac[Ohm]", PADDR(constant_impedance[2].Im()),PT_DESCRIPTION,"constant impedance load on phase C, imaginary only, specified as Ohms",

			PT_complex, "constant_power_AN[VA]", PADDR(constant_power_dy[3]),PT_DESCRIPTION,"constant power wye-connected load on phase A, specified as VA",
			PT_complex, "constant_power_BN[VA]", PADDR(constant_power_dy[4]),PT_DESCRIPTION,"constant power wye-connected load on phase B, specified as VA",
			PT_complex, "constant_power_CN[VA]", PADDR(constant_power_dy[5]),PT_DESCRIPTION,"constant power wye-connected load on phase C, specified as VA",
			PT_double, "constant_power_AN_real[W]", PADDR(constant_power_dy[3].Re()),PT_DESCRIPTION,"constant power wye-connected load on phase A, real only, specified as W",
			PT_double, "constant_power_BN_real[W]", PADDR(constant_power_dy[4].Re()),PT_DESCRIPTION,"constant power wye-connected load on phase B, real only, specified as W",
			PT_double, "constant_power_CN_real[W]", PADDR(constant_power_dy[5].Re()),PT_DESCRIPTION,"constant power wye-connected load on phase C, real only, specified as W",
			PT_double, "constant_power_AN_reac[VAr]", PADDR(constant_power_dy[3].Im()),PT_DESCRIPTION,"constant power wye-connected load on phase A, imaginary only, specified as VAr",
			PT_double, "constant_power_BN_reac[VAr]", PADDR(constant_power_dy[4].Im()),PT_DESCRIPTION,"constant power wye-connected load on phase B, imaginary only, specified as VAr",
			PT_double, "constant_power_CN_reac[VAr]", PADDR(constant_power_dy[5].Im()),PT_DESCRIPTION,"constant power wye-connected load on phase C, imaginary only, specified as VAr",
			PT_complex, "constant_current_AN[A]", PADDR(constant_current_dy[3]),PT_DESCRIPTION,"constant current wye-connected load on phase A, specified as Amps",
			PT_complex, "constant_current_BN[A]", PADDR(constant_current_dy[4]),PT_DESCRIPTION,"constant current wye-connected load on phase B, specified as Amps",
			PT_complex, "constant_current_CN[A]", PADDR(constant_current_dy[5]),PT_DESCRIPTION,"constant current wye-connected load on phase C, specified as Amps",
			PT_double, "constant_current_AN_real[A]", PADDR(constant_current_dy[3].Re()),PT_DESCRIPTION,"constant current wye-connected load on phase A, real only, specified as Amps",
			PT_double, "constant_current_BN_real[A]", PADDR(constant_current_dy[4].Re()),PT_DESCRIPTION,"constant current wye-connected load on phase B, real only, specified as Amps",
			PT_double, "constant_current_CN_real[A]", PADDR(constant_current_dy[5].Re()),PT_DESCRIPTION,"constant current wye-connected load on phase C, real only, specified as Amps",
			PT_double, "constant_current_AN_reac[A]", PADDR(constant_current_dy[3].Im()),PT_DESCRIPTION,"constant current wye-connected load on phase A, imaginary only, specified as Amps",
			PT_double, "constant_current_BN_reac[A]", PADDR(constant_current_dy[4].Im()),PT_DESCRIPTION,"constant current wye-connected load on phase B, imaginary only, specified as Amps",
			PT_double, "constant_current_CN_reac[A]", PADDR(constant_current_dy[5].Im()),PT_DESCRIPTION,"constant current wye-connected load on phase C, imaginary only, specified as Amps",
			PT_complex, "constant_impedance_AN[Ohm]", PADDR(constant_impedance_dy[3]),PT_DESCRIPTION,"constant impedance wye-connected load on phase A, specified as Ohms",
			PT_complex, "constant_impedance_BN[Ohm]", PADDR(constant_impedance_dy[4]),PT_DESCRIPTION,"constant impedance wye-connected load on phase B, specified as Ohms",
			PT_complex, "constant_impedance_CN[Ohm]", PADDR(constant_impedance_dy[5]),PT_DESCRIPTION,"constant impedance wye-connected load on phase C, specified as Ohms",
			PT_double, "constant_impedance_AN_real[Ohm]", PADDR(constant_impedance_dy[3].Re()),PT_DESCRIPTION,"constant impedance wye-connected load on phase A, real only, specified as Ohms",
			PT_double, "constant_impedance_BN_real[Ohm]", PADDR(constant_impedance_dy[4].Re()),PT_DESCRIPTION,"constant impedance wye-connected load on phase B, real only, specified as Ohms",
			PT_double, "constant_impedance_CN_real[Ohm]", PADDR(constant_impedance_dy[5].Re()),PT_DESCRIPTION,"constant impedance wye-connected load on phase C, real only, specified as Ohms",
			PT_double, "constant_impedance_AN_reac[Ohm]", PADDR(constant_impedance_dy[3].Im()),PT_DESCRIPTION,"constant impedance wye-connected load on phase A, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_BN_reac[Ohm]", PADDR(constant_impedance_dy[4].Im()),PT_DESCRIPTION,"constant impedance wye-connected load on phase B, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_CN_reac[Ohm]", PADDR(constant_impedance_dy[5].Im()),PT_DESCRIPTION,"constant impedance wye-connected load on phase C, imaginary only, specified as Ohms",

			PT_complex, "constant_power_AB[VA]", PADDR(constant_power_dy[0]),PT_DESCRIPTION,"constant power delta-connected load on phase A, specified as VA",
			PT_complex, "constant_power_BC[VA]", PADDR(constant_power_dy[1]),PT_DESCRIPTION,"constant power delta-connected load on phase B, specified as VA",
			PT_complex, "constant_power_CA[VA]", PADDR(constant_power_dy[2]),PT_DESCRIPTION,"constant power delta-connected load on phase C, specified as VA",
			PT_double, "constant_power_AB_real[W]", PADDR(constant_power_dy[0].Re()),PT_DESCRIPTION,"constant power delta-connected load on phase A, real only, specified as W",
			PT_double, "constant_power_BC_real[W]", PADDR(constant_power_dy[1].Re()),PT_DESCRIPTION,"constant power delta-connected load on phase B, real only, specified as W",
			PT_double, "constant_power_CA_real[W]", PADDR(constant_power_dy[2].Re()),PT_DESCRIPTION,"constant power delta-connected load on phase C, real only, specified as W",
			PT_double, "constant_power_AB_reac[VAr]", PADDR(constant_power_dy[0].Im()),PT_DESCRIPTION,"constant power delta-connected load on phase A, imaginary only, specified as VAr",
			PT_double, "constant_power_BC_reac[VAr]", PADDR(constant_power_dy[1].Im()),PT_DESCRIPTION,"constant power delta-connected load on phase B, imaginary only, specified as VAr",
			PT_double, "constant_power_CA_reac[VAr]", PADDR(constant_power_dy[2].Im()),PT_DESCRIPTION,"constant power delta-connected load on phase C, imaginary only, specified as VAr",
			PT_complex, "constant_current_AB[A]", PADDR(constant_current_dy[0]),PT_DESCRIPTION,"constant current delta-connected load on phase A, specified as Amps",
			PT_complex, "constant_current_BC[A]", PADDR(constant_current_dy[1]),PT_DESCRIPTION,"constant current delta-connected load on phase B, specified as Amps",
			PT_complex, "constant_current_CA[A]", PADDR(constant_current_dy[2]),PT_DESCRIPTION,"constant current delta-connected load on phase C, specified as Amps",
			PT_double, "constant_current_AB_real[A]", PADDR(constant_current_dy[0].Re()),PT_DESCRIPTION,"constant current delta-connected load on phase A, real only, specified as Amps",
			PT_double, "constant_current_BC_real[A]", PADDR(constant_current_dy[1].Re()),PT_DESCRIPTION,"constant current delta-connected load on phase B, real only, specified as Amps",
			PT_double, "constant_current_CA_real[A]", PADDR(constant_current_dy[2].Re()),PT_DESCRIPTION,"constant current delta-connected load on phase C, real only, specified as Amps",
			PT_double, "constant_current_AB_reac[A]", PADDR(constant_current_dy[0].Im()),PT_DESCRIPTION,"constant current delta-connected load on phase A, imaginary only, specified as Amps",
			PT_double, "constant_current_BC_reac[A]", PADDR(constant_current_dy[1].Im()),PT_DESCRIPTION,"constant current delta-connected load on phase B, imaginary only, specified as Amps",
			PT_double, "constant_current_CA_reac[A]", PADDR(constant_current_dy[2].Im()),PT_DESCRIPTION,"constant current delta-connected load on phase C, imaginary only, specified as Amps",
			PT_complex, "constant_impedance_AB[Ohm]", PADDR(constant_impedance_dy[0]),PT_DESCRIPTION,"constant impedance delta-connected load on phase A, specified as Ohms",
			PT_complex, "constant_impedance_BC[Ohm]", PADDR(constant_impedance_dy[1]),PT_DESCRIPTION,"constant impedance delta-connected load on phase B, specified as Ohms",
			PT_complex, "constant_impedance_CA[Ohm]", PADDR(constant_impedance_dy[2]),PT_DESCRIPTION,"constant impedance delta-connected load on phase C, specified as Ohms",
			PT_double, "constant_impedance_AB_real[Ohm]", PADDR(constant_impedance_dy[0].Re()),PT_DESCRIPTION,"constant impedance delta-connected load on phase A, real only, specified as Ohms",
			PT_double, "constant_impedance_BC_real[Ohm]", PADDR(constant_impedance_dy[1].Re()),PT_DESCRIPTION,"constant impedance delta-connected load on phase B, real only, specified as Ohms",
			PT_double, "constant_impedance_CA_real[Ohm]", PADDR(constant_impedance_dy[2].Re()),PT_DESCRIPTION,"constant impedance delta-connected load on phase C, real only, specified as Ohms",
			PT_double, "constant_impedance_AB_reac[Ohm]", PADDR(constant_impedance_dy[0].Im()),PT_DESCRIPTION,"constant impedance delta-connected load on phase A, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_BC_reac[Ohm]", PADDR(constant_impedance_dy[1].Im()),PT_DESCRIPTION,"constant impedance delta-connected load on phase B, imaginary only, specified as Ohms",
			PT_double, "constant_impedance_CA_reac[Ohm]", PADDR(constant_impedance_dy[2].Im()),PT_DESCRIPTION,"constant impedance delta-connected load on phase C, imaginary only, specified as Ohms",

			PT_complex,	"measured_voltage_A",PADDR(measured_voltage_A),PT_DESCRIPTION,"current measured voltage on phase A",
			PT_complex,	"measured_voltage_B",PADDR(measured_voltage_B),PT_DESCRIPTION,"current measured voltage on phase B",
			PT_complex,	"measured_voltage_C",PADDR(measured_voltage_C),PT_DESCRIPTION,"current measured voltage on phase C",
			PT_complex,	"measured_voltage_AB",PADDR(measured_voltage_AB),PT_DESCRIPTION,"current measured voltage on phases AB",
			PT_complex,	"measured_voltage_BC",PADDR(measured_voltage_BC),PT_DESCRIPTION,"current measured voltage on phases BC",
			PT_complex,	"measured_voltage_CA",PADDR(measured_voltage_CA),PT_DESCRIPTION,"current measured voltage on phases CA",
			PT_bool, "phase_loss_protection", PADDR(three_phase_protect), PT_DESCRIPTION, "Trip all three phases of the load if a fault occurs",

			// This allows the user to set a base power on each phase, and specify the power as a function
			// of ZIP and pf for each phase (similar to zipload).  This will override the constant values
			// above if specified, otherwise, constant values work as stated
			PT_double, "base_power_A[VA]",PADDR(base_power[0]),PT_DESCRIPTION,"in similar format as ZIPload, this represents the nominal power on phase A before applying ZIP fractions",
			PT_double, "base_power_B[VA]",PADDR(base_power[1]),PT_DESCRIPTION,"in similar format as ZIPload, this represents the nominal power on phase B before applying ZIP fractions",
			PT_double, "base_power_C[VA]",PADDR(base_power[2]),PT_DESCRIPTION,"in similar format as ZIPload, this represents the nominal power on phase C before applying ZIP fractions",
			PT_double, "power_pf_A[pu]",PADDR(power_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase A constant power portion of load",
			PT_double, "current_pf_A[pu]",PADDR(current_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase A constant current portion of load",
			PT_double, "impedance_pf_A[pu]",PADDR(impedance_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase A constant impedance portion of load",
			PT_double, "power_pf_B[pu]",PADDR(power_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase B constant power portion of load",
			PT_double, "current_pf_B[pu]",PADDR(current_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase B constant current portion of load",
			PT_double, "impedance_pf_B[pu]",PADDR(impedance_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase B constant impedance portion of load",
			PT_double, "power_pf_C[pu]",PADDR(power_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase C constant power portion of load",
			PT_double, "current_pf_C[pu]",PADDR(current_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase C constant current portion of load",
			PT_double, "impedance_pf_C[pu]",PADDR(impedance_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase C constant impedance portion of load",
			PT_double, "power_fraction_A[pu]",PADDR(power_fraction[0]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase A",
			PT_double, "current_fraction_A[pu]",PADDR(current_fraction[0]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase A",
			PT_double, "impedance_fraction_A[pu]",PADDR(impedance_fraction[0]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase A",
			PT_double, "power_fraction_B[pu]",PADDR(power_fraction[1]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase B",
			PT_double, "current_fraction_B[pu]",PADDR(current_fraction[1]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase B",
			PT_double, "impedance_fraction_B[pu]",PADDR(impedance_fraction[1]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase B",
			PT_double, "power_fraction_C[pu]",PADDR(power_fraction[2]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase C",
			PT_double, "current_fraction_C[pu]",PADDR(current_fraction[2]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase C",
			PT_double, "impedance_fraction_C[pu]",PADDR(impedance_fraction[2]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase C",

         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
				GL_THROW("Unable to publish load delta_linkage function");
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_load)==NULL)
				GL_THROW("Unable to publish load deltamode function");
			if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
				GL_THROW("Unable to publish load deltamode function");
    }
}

int load::isa(char *classname)
{
	return strcmp(classname,"load")==0 || node::isa(classname);
}

int load::create(void)
{
	int res = node::create();
        
	maximum_voltage_error = 0;
	base_power[0] = base_power[1] = base_power[2] = 0;
	power_fraction[0] = power_fraction[1] = power_fraction[2] = 0;
	current_fraction[0] = current_fraction[1] = current_fraction[2] = 0;
	impedance_fraction[0] = impedance_fraction[1] = impedance_fraction[2] = 0;
	power_pf[0] = power_pf[1] = power_pf[2] = 1;
	current_pf[0] = current_pf[1] = current_pf[2] = 1;
	impedance_pf[0] = impedance_pf[1] = impedance_pf[2] = 1;
	load_class = LC_UNKNOWN;
	three_phase_protect = false;	//By default, let all three phases go

	//Zero all loads (get the ones missed above)
	constant_power[0] = constant_power[1] = constant_power[2] = 0.0;
	constant_current[0] = constant_current[1] = constant_current[2] = 0.0;
	constant_impedance[0] = constant_impedance[1] = constant_impedance[2] = 0.0;

	constant_power_dy[0] = constant_power_dy[1] = constant_power_dy[2] = 0.0;
	constant_current_dy[0] = constant_current_dy[1] = constant_current_dy[2] = 0.0;
	constant_impedance_dy[0] = constant_impedance_dy[1] = constant_impedance_dy[2] = 0.0;

	constant_power_dy[3] = constant_power_dy[4] = constant_power_dy[5] = 0.0;
	constant_current_dy[3] = constant_current_dy[4] = constant_current_dy[5] = 0.0;
	constant_impedance_dy[3] = constant_impedance_dy[4] = constant_impedance_dy[5] = 0.0;

	//Flag us as a load
	node_type = LOAD_NODE;

    return res;
}

// Initialize, return 1 on success
int load::init(OBJECT *parent)
{
	char temp_buff[128];
	OBJECT *obj = OBJECTHDR(this);
	
	if (has_phase(PHASE_S))
	{
		GL_THROW("Load objects do not support triplex connections at this time!");
		/*  TROUBLESHOOT
		load objects are only designed to be added to the primary side of a distribution power flow.  To add loads
		to the triplex side, please use the triplex_node object.
		*/
	}

	//Update tracking flag
	//Get server mode variable
	gl_global_getvar("multirun_mode",temp_buff,sizeof(temp_buff));

	//See if we're not in standalone
	if (strcmp(temp_buff,"STANDALONE"))	//strcmp returns a 0 if they are the same
	{
		if (solver_method == SM_NR)
		{
			//Allocate the storage vector
			prev_power_value = (complex *)gl_malloc(3*sizeof(complex));

			//Check it
			if (prev_power_value==NULL)
			{
				GL_THROW("Failure to allocate memory for power tracking array");
				/*  TROUBLESHOOT
				While attempting to allocate memory for the power tracking array used
				by the master/slave functionality, an error occurred.  Please try again.
				If the error persists, please submit your code and a bug report via the trac
				website.
				*/
			}

			//Populate it with zeros for now, just cause - init sets voltages in node
			prev_power_value[0] = complex(0.0,0.0);
			prev_power_value[1] = complex(0.0,0.0);
			prev_power_value[2] = complex(0.0,0.0);
		}
	}

	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)	//Deltamode warning check
	{
		if ((constant_current[0] != 0.0) || (constant_current[1] != 0.0) || (constant_current[2] != 0.0))
		{
			gl_warning("load:%s - constant_current loads in deltamode are handled slightly different", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			Due to the potential for moving reference frame of deltamode systems, constant current loads are computed using a scaled
			per-unit approach, rather than the fixed constant_current value.  You may get results that differ from traditional GridLAB-D
			super-second or static powerflow results in this mode.
			*/
		}
	}

	return node::init(parent);
}

TIMESTAMP load::presync(TIMESTAMP t0)
{
	if ((solver_method!=SM_FBS) && ((SubNode==PARENT) || (SubNode==DIFF_PARENT)))	//Need to do something slightly different with NR and parented node
	{
		if (SubNode == PARENT)
		{
			shunt[0] = shunt[1] = shunt[2] = 0.0;
			power[0] = power[1] = power[2] = 0.0;
			current[0] = current[1] = current[2] = 0.0;
		}

		//Explicitly specified load portions
		shunt_dy[0] = shunt_dy[1] = shunt_dy[2] = 0.0;
		shunt_dy[3] = shunt_dy[4] = shunt_dy[5] = 0.0;
		power_dy[0] = power_dy[1] = power_dy[2] = 0.0;
		power_dy[3] = power_dy[4] = power_dy[5] = 0.0;
		current_dy[0] = current_dy[1] = current_dy[2] = 0.0;
		current_dy[3] = current_dy[4] = current_dy[5] = 0.0;
	}

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::presync(t0);
	
	return result;
}

TIMESTAMP load::sync(TIMESTAMP t0)
{
	//bool all_three_phases;
	bool fault_mode;

	//See if we're reliability-enabled
	if (fault_check_object == NULL)
		fault_mode = false;
	else
		fault_mode = true;

	//Functionalized so deltamode can parttake
	load_update_fxn(fault_mode);

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::sync(t0);

	return result;
}

TIMESTAMP load::postsync(TIMESTAMP t0)
{
	//Call node postsync first, otherwise the properties below end up 1 time out of sync (for PCed loads anyways)
	TIMESTAMP t1 = node::postsync(t0);

	//Moved from sync.  Hopefully it doesn't mess anything up, but these properties are 99% stupid and useless anyways, so meh
	measured_voltage_A.SetPolar(voltageA.Mag(),voltageA.Arg());  //Used for testing and xml output
	measured_voltage_B.SetPolar(voltageB.Mag(),voltageB.Arg());
	measured_voltage_C.SetPolar(voltageC.Mag(),voltageC.Arg());
	measured_voltage_AB = measured_voltage_A-measured_voltage_B;
	measured_voltage_BC = measured_voltage_B-measured_voltage_C;
	measured_voltage_CA = measured_voltage_C-measured_voltage_A;

	return t1;
}

//Functional call to sync-level load updates
//Here primarily so deltamode players can actually influence things
void load::load_update_fxn(bool fault_mode)
{
	bool all_three_phases;
	int index_var;

	for (int index=0; index<3; index++)
	{
		if (base_power[index] != 0.0)
		{
			// Put in the constant power portion
			if (power_fraction[index] != 0.0)
			{
				double real_power,imag_power;

				if (power_pf[index] == 0.0)
				{				
					real_power = 0.0;
					imag_power = base_power[index] * power_fraction[index];
				}
				else
				{
					real_power = base_power[index] * power_fraction[index] * fabs(power_pf[index]);
					imag_power = real_power * sqrt(1.0/(power_pf[index] * power_pf[index]) - 1.0);
				}

				if (power_pf[index] < 0)
				{
					imag_power *= -1.0;	//Adjust imaginary portion for negative PF
				}
				constant_power[index] = complex(real_power,imag_power);
			}
			else
			{
				constant_power[index] = complex(0,0);
			}
		
			// Put in the constant current portion and adjust the angle
			if (current_fraction[index] != 0.0)
			{
				double real_power,imag_power,temp_angle;
				complex temp_curr;

				if (current_pf[index] == 0.0)
				{				
					real_power = 0.0;
					imag_power = base_power[index] * current_fraction[index];
				}
				else
				{
					real_power = base_power[index] * current_fraction[index] * fabs(current_pf[index]);
					imag_power = real_power * sqrt(1.0/(current_pf[index] * current_pf[index]) - 1.0);
				}

				if (current_pf[index] < 0)
				{
					imag_power *= -1.0;	//Adjust imaginary portion for negative PF
				}
				
				// Calculate then shift the constant current to use the posted voltage as the reference angle
				temp_curr = ~complex(real_power,imag_power) / complex(nominal_voltage,0);
				temp_angle = temp_curr.Arg() + voltage[index].Arg();
				temp_curr.SetPolar(temp_curr.Mag(),temp_angle);

				constant_current[index] = temp_curr;
			}
			else
			{
				constant_current[index] = complex(0,0);
			}

			// Put in the constant impedance portion
			if (impedance_fraction[index] != 0.0)
			{
				double real_power,imag_power;

				if (impedance_pf[index] == 0.0)
				{				
					real_power = 0.0;
					imag_power = base_power[index] * impedance_fraction[index];
				}
				else
				{
					real_power = base_power[index] * impedance_fraction[index] * fabs(impedance_pf[index]);
					imag_power = real_power * sqrt(1.0/(impedance_pf[index] * impedance_pf[index]) - 1.0);
				}

				if (impedance_pf[index] < 0)
				{
					imag_power *= -1.0;	//Adjust imaginary portion for negative PF
				}

				constant_impedance[index] = ~( complex(nominal_voltage * nominal_voltage, 0) / complex(real_power,imag_power) );

			}
			else
			{
				constant_impedance[index] = complex(0,0);
			}
		}
	}

	if (fault_mode == false)	//Not reliability mode - normal mode
	{
		if ((solver_method!=SM_FBS) && ((SubNode==PARENT) || (SubNode==DIFF_PARENT)))	//Need to do something slightly different with GS/NR and parented load
		{													//associated with change due to player methods
			if (SubNode == PARENT)	//Normal parent gets one routine
			{
				if (!(constant_impedance[0].IsZero()))
					shunt[0] += complex(1.0)/constant_impedance[0];

				if (!(constant_impedance[1].IsZero()))
					shunt[1] += complex(1.0)/constant_impedance[1];
				
				if (!(constant_impedance[2].IsZero()))
					shunt[2] += complex(1.0)/constant_impedance[2];

				power[0] += constant_power[0];
				power[1] += constant_power[1];	
				power[2] += constant_power[2];

				current[0] += constant_current[0];
				current[1] += constant_current[1];
				current[2] += constant_current[2];
			}
			else //DIFF_PARENT
			{
				if(constant_impedance[0].IsZero())
					shunt[0] = 0.0;
				else
					shunt[0] = complex(1)/constant_impedance[0];

				if(constant_impedance[1].IsZero())
					shunt[1] = 0.0;
				else
					shunt[1] = complex(1)/constant_impedance[1];
				
				if(constant_impedance[2].IsZero())
					shunt[2] = 0.0;
				else
					shunt[2] = complex(1)/constant_impedance[2];
				
				power[0] = constant_power[0];
				power[1] = constant_power[1];	
				power[2] = constant_power[2];
				current[0] = constant_current[0];
				current[1] = constant_current[1];
				current[2] = constant_current[2];
			}

			//Do the same for explicit delta/wye connections -- both parent types get this
			for (index_var=0; index_var<6; index_var++)
			{
				if (!(constant_impedance_dy[index_var].IsZero()))
					shunt_dy[index_var] += complex(1.0)/constant_impedance_dy[index_var];

				power_dy[index_var] += constant_power_dy[index_var];

				current_dy[index_var] += constant_current_dy[index_var];
			}
		}
		else
		{
			if(constant_impedance[0].IsZero())
				shunt[0] = 0.0;
			else
				shunt[0] = complex(1)/constant_impedance[0];

			if(constant_impedance[1].IsZero())
				shunt[1] = 0.0;
			else
				shunt[1] = complex(1)/constant_impedance[1];
			
			if(constant_impedance[2].IsZero())
				shunt[2] = 0.0;
			else
				shunt[2] = complex(1)/constant_impedance[2];
			
			power[0] = constant_power[0];
			power[1] = constant_power[1];	
			power[2] = constant_power[2];

			current[0] = constant_current[0];
			current[1] = constant_current[1];
			current[2] = constant_current[2];

			//Do the same for explicit delta/wye connections - handle the same way (draconian overwrites!)
			for (index_var=0; index_var<6; index_var++)
			{
				if (constant_impedance_dy[index_var].IsZero())
					shunt_dy[index_var] = 0.0;
				else
					shunt_dy[index_var] = complex(1.0)/constant_impedance_dy[index_var];

				power_dy[index_var] = constant_power_dy[index_var];

				current_dy[index_var] = constant_current_dy[index_var];
			}
		}
	}//End normal mode
	else	//Reliability mode
	{
		all_three_phases = true;	//By default, handle all the phases

		//Check and see if we're a "protected" load
		if (three_phase_protect == true)
		{
			if ((SubNode!=CHILD) && (SubNode!=DIFF_CHILD))
			{
				//See if all phases are still in service
				if ((NR_busdata[NR_node_reference].origphases & NR_busdata[NR_node_reference].phases) != NR_busdata[NR_node_reference].origphases)
				{
					//Assume all three trip
					all_three_phases = false;
				}
			}
			else	//It is a child - look at parent
			{
				//See if all phases are still in service
				if ((NR_busdata[*NR_subnode_reference].origphases & NR_busdata[*NR_subnode_reference].phases) != NR_busdata[*NR_subnode_reference].origphases)
				{
					//Assume all three trip
					all_three_phases = false;
				}
			}
		}

		if (all_three_phases == true)	//Handle all phases correctly
		{
			if ((solver_method!=SM_FBS) && ((SubNode==PARENT) || (SubNode==DIFF_PARENT)))	//Need to do something slightly different with GS/NR and parented load
			{													//associated with change due to player methods

				if (SubNode == PARENT)	//Normal parents need this
				{
					if (!(constant_impedance[0].IsZero()))
						shunt[0] += complex(1.0)/constant_impedance[0];

					if (!(constant_impedance[1].IsZero()))
						shunt[1] += complex(1.0)/constant_impedance[1];
					
					if (!(constant_impedance[2].IsZero()))
						shunt[2] += complex(1.0)/constant_impedance[2];
					
					power[0] += constant_power[0];
					power[1] += constant_power[1];	
					power[2] += constant_power[2];

					current[0] += constant_current[0];
					current[1] += constant_current[1];
					current[2] += constant_current[2];
				}
				else //DIFF_PARENT
				{
					if(constant_impedance[0].IsZero())
						shunt[0] = 0.0;
					else
						shunt[0] = complex(1)/constant_impedance[0];

					if(constant_impedance[1].IsZero())
						shunt[1] = 0.0;
					else
						shunt[1] = complex(1)/constant_impedance[1];
					
					if(constant_impedance[2].IsZero())
						shunt[2] = 0.0;
					else
						shunt[2] = complex(1)/constant_impedance[2];
					
					power[0] = constant_power[0];
					power[1] = constant_power[1];	
					power[2] = constant_power[2];
					current[0] = constant_current[0];
					current[1] = constant_current[1];
					current[2] = constant_current[2];
				}

				//Do the same for explicit delta/wye connections -- both parent types handled the same
				for (index_var=0; index_var<6; index_var++)
				{
					if (!(constant_impedance_dy[index_var].IsZero()))
						shunt_dy[index_var] += complex(1.0)/constant_impedance_dy[index_var];

					power_dy[index_var] += constant_power_dy[index_var];

					current_dy[index_var] += constant_current_dy[index_var];
				}
			}
			else
			{
				if(constant_impedance[0].IsZero())
					shunt[0] = 0.0;
				else
					shunt[0] = complex(1)/constant_impedance[0];

				if(constant_impedance[1].IsZero())
					shunt[1] = 0.0;
				else
					shunt[1] = complex(1)/constant_impedance[1];
				
				if(constant_impedance[2].IsZero())
					shunt[2] = 0.0;
				else
					shunt[2] = complex(1)/constant_impedance[2];
				
				power[0] = constant_power[0];
				power[1] = constant_power[1];	
				power[2] = constant_power[2];

				current[0] = constant_current[0];
				current[1] = constant_current[1];
				current[2] = constant_current[2];

				//Do the same for explicit delta/wye connections - handle the same way (draconian overwrites!)
				for (index_var=0; index_var<6; index_var++)
				{
					if (constant_impedance_dy[index_var].IsZero())
						shunt_dy[index_var] = 0.0;
					else
						shunt_dy[index_var] = complex(1.0)/constant_impedance_dy[index_var];

					power_dy[index_var] = constant_power_dy[index_var];

					current_dy[index_var] = constant_current_dy[index_var];
				}
			}
		}//Handle all three
		else	//Zero all three - may cause issues with P/C loads, but why parent a load to a load??
		{
			power[0] = 0.0;
			power[1] = 0.0;
			power[2] = 0.0;
			shunt[0] = 0.0;
			shunt[1] = 0.0;
			shunt[2] = 0.0;
			current[0] = 0.0;
			current[1] = 0.0;
			current[2] = 0.0;

			//Zero out all of the explicit delta/wye connections as well
			//Do the same for explicit delta/wye connections - handle the same way (draconian overwrites!)
			for (index_var=0; index_var<6; index_var++)
			{
				shunt_dy[index_var] = complex(0.0,0.0);
				power_dy[index_var] = complex(0.0,0.0);
				current_dy[index_var] = complex(0.0,0.0);
			}
		}//End zero
	}//End reliability mode
}

//Notify function
//NOTE: The NR-based notify stuff may no longer be needed after NR is "flattened", since it will
//      effectively be like FBS at that point.
//Second NOTE: This function doesn't incorporate the newer explicit delta/wye connected load structure (TODO)
int load::notify(int update_mode, PROPERTY *prop, char *value)
{
	complex diff_val;
	double power_tolerance;

	if (solver_method == SM_NR)
	{
		//See if it was a power update - if it was populated
		if (prev_power_value != NULL)
		{
			//See if there is a power update - phase A
			if (strcmp(prop->name,"constant_power_A")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_power_value[0] = constant_power[0];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Calculate the "power tolerance"
					power_tolerance = constant_power[0].Mag() * default_maximum_power_error;

					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = constant_power[0] - prev_power_value[0];

					if (diff_val.Mag() >= power_tolerance)
					{
						NR_retval = gl_globalclock;	//Force a reiteration
					}
				}
			}

			//See if there is a power update - phase B
			if (strcmp(prop->name,"constant_power_B")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_power_value[1] = constant_power[1];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Calculate the "power tolerance"
					power_tolerance = constant_power[1].Mag() * default_maximum_power_error;

					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = constant_power[1] - prev_power_value[1];

					if (diff_val.Mag() >= power_tolerance)
					{
						NR_retval = gl_globalclock;	//Force a reiteration
					}
				}
			}

			//See if there is a power update - phase C
			if (strcmp(prop->name,"constant_power_C")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_power_value[2] = constant_power[2];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Calculate the "power tolerance"
					power_tolerance = constant_power[2].Mag() * default_maximum_power_error;

					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = constant_power[2] - prev_power_value[2];

					if (diff_val.Mag() >= power_tolerance)
					{
						NR_retval = gl_globalclock;	//Force a reiteration
					}
				}
			}
		}
	}//End NR
	//Default else - FBS doesn't really need any special code for it's handling

	return 1;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE load::inter_deltaupdate_load(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//unsigned char pass_mod;
	OBJECT *hdr = OBJECTHDR(this);
	bool fault_mode;

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Load-specific presync items
		if (SubNode==PARENT)	//Need to do something slightly different with GS and parented node
		{
			shunt[0] = shunt[1] = shunt[2] = 0.0;
			power[0] = power[1] = power[2] = 0.0;
			current[0] = current[1] = current[2] = 0.0;
		}

		//Call presync-equivalent items
		NR_node_presync_fxn();

		//See if we're reliability-enabled
		if (fault_check_object == NULL)
			fault_mode = false;
		else
			fault_mode = true;

		//Functionalized so deltamode can parttake
		load_update_fxn(fault_mode);

		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Perform postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

		//Postsync load items - measurement population
		measured_voltage_A.SetPolar(voltageA.Mag(),voltageA.Arg());  //Used for testing and xml output
		measured_voltage_B.SetPolar(voltageB.Mag(),voltageB.Arg());
		measured_voltage_C.SetPolar(voltageC.Mag(),voltageC.Arg());
		measured_voltage_AB = measured_voltage_A-measured_voltage_B;
		measured_voltage_BC = measured_voltage_B-measured_voltage_C;
		measured_voltage_CA = measured_voltage_C-measured_voltage_A;

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
// IMPLEMENTATION OF CORE LINKAGE: load
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_load(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(load::oclass);
		if (*obj!=NULL)
		{
			load *my = OBJECTDATA(*obj,load);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(load);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to the object to be initialized
* @return 1 on success, 0 on error
*/
EXPORT int init_load(OBJECT *obj)
{
	try {
		load *my = OBJECTDATA(obj,load);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(load);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_load(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		load *pObj = OBJECTDATA(obj,load);
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
	SYNC_CATCHALL(load);
}

EXPORT int isa_load(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,load)->isa(classname);
}

EXPORT int notify_load(OBJECT *obj, int update_mode, PROPERTY *prop, char *value){
	load *n = OBJECTDATA(obj, load);
	int rv = 1;

	rv = n->notify(update_mode, prop, value);

	return rv;
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_load(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	load *my = OBJECTDATA(obj,load);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_load(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_load(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}*/
