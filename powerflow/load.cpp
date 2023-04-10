/** $Id: load.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file load.cpp
	@addtogroup load
	@ingroup node
	
	Load objects represent static loads and export both voltages and current.  
		
	@{
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "load.h"

CLASS* load::oclass = nullptr;
CLASS* load::pclass = nullptr;

load::load(MODULE *mod) : node(mod)
{
	if(oclass == nullptr)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"load",sizeof(load),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
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
			PT_enumeration, "load_priority", PADDR(load_priority),PT_DESCRIPTION,"Load classification based on priority",
				PT_KEYWORD, "DISCRETIONARY", (enumeration)DISCRETIONARY,
				PT_KEYWORD, "PRIORITY", (enumeration)PRIORITY,
				PT_KEYWORD, "CRITICAL", (enumeration)CRITICAL,
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

			PT_complex,	"measured_voltage_A[V]",PADDR(measured_voltage_A),PT_DESCRIPTION,"current measured voltage on phase A",
			PT_complex,	"measured_voltage_B[V]",PADDR(measured_voltage_B),PT_DESCRIPTION,"current measured voltage on phase B",
			PT_complex,	"measured_voltage_C[V]",PADDR(measured_voltage_C),PT_DESCRIPTION,"current measured voltage on phase C",
			PT_complex,	"measured_voltage_AB[V]",PADDR(measured_voltage_AB),PT_DESCRIPTION,"current measured voltage on phases AB",
			PT_complex,	"measured_voltage_BC[V]",PADDR(measured_voltage_BC),PT_DESCRIPTION,"current measured voltage on phases BC",
			PT_complex,	"measured_voltage_CA[V]",PADDR(measured_voltage_CA),PT_DESCRIPTION,"current measured voltage on phases CA",
			PT_bool, "phase_loss_protection", PADDR(three_phase_protect), PT_DESCRIPTION, "Trip all three phases of the load if a fault occurs",
			PT_complex, "measured_power_A[VA]",PADDR(measured_power[0]),PT_DESCRIPTION,"current measured power on phase A",
			PT_complex, "measured_power_B[VA]",PADDR(measured_power[1]),PT_DESCRIPTION,"current measured power on phase B",
			PT_complex, "measured_power_C[VA]",PADDR(measured_power[2]),PT_DESCRIPTION,"current measured power on phase C",
			PT_complex, "measured_power[VA]",PADDR(measured_total_power),PT_DESCRIPTION,"current total power",

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

			PT_enumeration, "inrush_integration_method_capacitance",PADDR(inrush_int_method_capacitance),PT_DESCRIPTION,"Selected integration method to use for capacitive elements of the load",
				PT_KEYWORD,"NONE",(enumeration)IRM_NONE,
				PT_KEYWORD,"UNDEFINED",(enumeration)IRM_UNDEFINED,
				PT_KEYWORD,"TRAPEZOIDAL",(enumeration)IRM_TRAPEZOIDAL,
				PT_KEYWORD,"BACKWARD_EULER",(enumeration)IRM_BACKEULER,

			PT_enumeration, "inrush_integration_method_inductance",PADDR(inrush_int_method_inductance),PT_DESCRIPTION,"Selected integration method to use for inductive elements of the load",
				PT_KEYWORD,"NONE",(enumeration)IRM_NONE,
				PT_KEYWORD,"UNDEFINED",(enumeration)IRM_UNDEFINED,
				PT_KEYWORD,"TRAPEZOIDAL",(enumeration)IRM_TRAPEZOIDAL,
				PT_KEYWORD,"BACKWARD_EULER",(enumeration)IRM_BACKEULER,

         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_load)==nullptr)
			GL_THROW("Unable to publish load deltamode function");
		if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==nullptr)
			GL_THROW("Unable to publish load swing-swapping function");
		if (gl_publish_function(oclass,	"pwr_current_injection_update_map", (FUNCTIONADDR)node_map_current_update_function)==nullptr)
			GL_THROW("Unable to publish load current injection update mapping function");
		if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==nullptr)
			GL_THROW("Unable to publish load VFD attachment function");
		if (gl_publish_function(oclass, "pwr_object_reset_disabled_status", (FUNCTIONADDR)node_reset_disabled_status) == nullptr)
			GL_THROW("Unable to publish load island-status-reset function");
		if (gl_publish_function(oclass, "pwr_object_load_update", (FUNCTIONADDR)update_load_values) == nullptr)
			GL_THROW("Unable to publish load impedance-conversion/update function");
		if (gl_publish_function(oclass, "pwr_object_swing_status_check", (FUNCTIONADDR)node_swing_status) == nullptr)
			GL_THROW("Unable to publish load swing-status check function");
		if (gl_publish_function(oclass, "pwr_object_shunt_update", (FUNCTIONADDR)node_update_shunt_values) == nullptr)
			GL_THROW("Unable to publish load shunt update function");
		if (gl_publish_function(oclass, "pwr_object_kmldata", (FUNCTIONADDR)load_kmldata) == nullptr)
			GL_THROW("Unable to publish load kmldata function");
    }
}

int load::isa(char *classname)
{
	return strcmp(classname,"load")==0 || node::isa(classname);
}

int load::create(void)
{
	int res = node::create();
	char index_x,index_y;
        
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
	constant_power[0] = constant_power[1] = constant_power[2] = gld::complex(0.0,0.0);
	constant_current[0] = constant_current[1] = constant_current[2] = gld::complex(0.0,0.0);
	constant_impedance[0] = constant_impedance[1] = constant_impedance[2] = gld::complex(0.0,0.0);

	constant_power_dy[0] = constant_power_dy[1] = constant_power_dy[2] = gld::complex(0.0,0.0);
	constant_current_dy[0] = constant_current_dy[1] = constant_current_dy[2] = gld::complex(0.0,0.0);
	constant_impedance_dy[0] = constant_impedance_dy[1] = constant_impedance_dy[2] = gld::complex(0.0,0.0);

	constant_power_dy[3] = constant_power_dy[4] = constant_power_dy[5] = gld::complex(0.0,0.0);
	constant_current_dy[3] = constant_current_dy[4] = constant_current_dy[5] = gld::complex(0.0,0.0);
	constant_impedance_dy[3] = constant_impedance_dy[4] = constant_impedance_dy[5] = gld::complex(0.0,0.0);

	//Initialize the tracking variables - do it as a loop (complex mucks things up)
	for (index_x=0; index_x<3; index_x++)
	{
		for (index_y=0; index_y<3; index_y++)
		{
			prev_load_values[index_x][index_y] = gld::complex(0.0,0.0);

			prev_load_values_dy[index_x][index_y] = gld::complex(0.0,0.0);
			prev_load_values_dy[index_x][index_y+3] = gld::complex(0.0,0.0);
		}
	}

	//Flag us as a load
	node_type = LOAD_NODE;

	//in-rush related zeroing
	prev_shunt[0] = prev_shunt[1] = prev_shunt[2] = gld::complex(0.0,0.0);

	//ZIP fraction tracking
	base_load_val_was_nonzero[0] = base_load_val_was_nonzero[1] = base_load_val_was_nonzero[2] = false;

	//Set defaults to see if anyone changes the integration methods
	inrush_int_method_inductance = IRM_UNDEFINED;
	inrush_int_method_capacitance = IRM_UNDEFINED;

    return res;
}

// Initialize, return 1 on success
int load::init(OBJECT *parent)
{
	char temp_buff[128];
	OBJECT *obj = OBJECTHDR(this);
	int ret_value;
	
	if (has_phase(PHASE_S))
	{
		GL_THROW("Load objects do not support triplex connections at this time!");
		/*  TROUBLESHOOT
		load objects are only designed to be added to the primary side of a distribution power flow.  To add loads
		to the triplex side, please use the triplex_load object.
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
			prev_power_value = (gld::complex *)gl_malloc(3*sizeof(gld::complex));

			//Check it
			if (prev_power_value==nullptr)
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
			prev_power_value[0] = gld::complex(0.0,0.0);
			prev_power_value[1] = gld::complex(0.0,0.0);
			prev_power_value[2] = gld::complex(0.0,0.0);
		}
	}

	//Initialize the node prior to the next step, otherwise the "all flagger" hasn't occurred and the warning won't pop
	ret_value = node::init(parent);

	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)	//Deltamode warning check
	{
		//See if we're going to be "in-rushy" or not
		if (enable_inrush_calculations)
		{
			//Figure out if we need our integration method updated - inductance
			if (inrush_int_method_inductance == IRM_UNDEFINED)
			{
				//Pull the main object-level one
				inrush_int_method_inductance = inrush_integration_method;
			}

			//Figure out if we need our integration method updated - capacitance
			if (inrush_int_method_capacitance == IRM_UNDEFINED)
			{
				//Pull the main object-level one
				inrush_int_method_capacitance = inrush_integration_method;
			}

			//Allocate all of the terms - inductive and capacitive, just in case
			//Allocate ahrl - inductive term
			ahrlloadstore = (gld::complex *)gl_malloc(3*sizeof(gld::complex));

			//Check it
			if (ahrlloadstore == nullptr)
			{
				GL_THROW("load:%d-%s - failed to allocate memory for in-rush calculations",obj->id, obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				While allocating memory for the in-rush calculations, an error was encountering.  Please try again.  If the error persists,
				please post a bug report to the forums or ticketing website, along with your GLM code.
				*/
			}

			//bhrl inductive term
			bhrlloadstore = (gld::complex *)gl_malloc(3*sizeof(gld::complex));

			//Check it
			if (bhrlloadstore == nullptr)
			{
				GL_THROW("load:%d-%s - failed to allocate memory for in-rush calculations",obj->id, obj->name ? obj->name : "unnamed");
				//Defined above
			}

			//chrc - capacitive term
			chrcloadstore = (gld::complex *)gl_malloc(3*sizeof(gld::complex));

			//Check it
			if (chrcloadstore == nullptr)
			{
				GL_THROW("load:%d-%s - failed to allocate memory for in-rush calculations",obj->id, obj->name ? obj->name : "unnamed");
				//Defined above
			}

			//LoadHistTermL - history term for inductive
			LoadHistTermL = (gld::complex *)gl_malloc(6*sizeof(gld::complex));

			//Check it
			if (LoadHistTermL == nullptr)
			{
				GL_THROW("load:%d-%s - failed to allocate memory for in-rush calculations",obj->id, obj->name ? obj->name : "unnamed");
				//Defined above
			}

			//LoadHistTermC - history term for capacitive
			LoadHistTermC = (gld::complex *)gl_malloc(6*sizeof(gld::complex));

			//Check it
			if (LoadHistTermC == nullptr)
			{
				GL_THROW("load:%d-%s - failed to allocate memory for in-rush calculations",obj->id, obj->name ? obj->name : "unnamed");
				//Defined above
			}

			//Now zero them all, for good measure
			ahrlloadstore[0] = ahrlloadstore[1] = ahrlloadstore[2] = gld::complex(0.0,0.0);
			bhrlloadstore[0] = bhrlloadstore[1] = bhrlloadstore[2] = gld::complex(0.0,0.0);
			chrcloadstore[0] = chrcloadstore[1] = chrcloadstore[2] = gld::complex(0.0,0.0);

			LoadHistTermL[0] = LoadHistTermL[1] = LoadHistTermL[2] = gld::complex(0.0,0.0);
			LoadHistTermL[3] = LoadHistTermL[4] = LoadHistTermL[5] = gld::complex(0.0,0.0);

			LoadHistTermC[0] = LoadHistTermC[1] = LoadHistTermC[2] = gld::complex(0.0,0.0);
			LoadHistTermC[3] = LoadHistTermC[4] = LoadHistTermC[5] = gld::complex(0.0,0.0);

			//full_Y_load inited in node
		}
		//Default else - not needed
	}

	//Simple Check to see if phases are there - only works for GLM-specified values (instead of players)
	if (has_phase(PHASE_D))
	{
		//Crude phase checks
		if (((constant_power[0].Mag() > 0.0) || (constant_current[0].Mag() > 0.0) || (constant_impedance[0].Mag() > 0.0) || (fabs(base_power[0]) > 0.0)) && !has_phase(PHASE_A|PHASE_B))
		{
			gl_warning("load:%d %s - delta-connected load components for xxx_A need phases AB to be present",obj->id,(obj->name?obj->name:"Unnamed"));
			/*   TROUBLESHOOT
			A load object has a delta connection implied in the phase, but the load specified requires more phases than are present on the object.
			e.g., constant_power_A for a delta connection implies phases must include AB, but one of these is missing.  This load value will not
			be calculated correctly and/or excluded from the powerflow.
			*/
		}

		if (((constant_power[1].Mag() > 0.0) || (constant_current[1].Mag() > 0.0) || (constant_impedance[1].Mag() > 0.0) || (fabs(base_power[1]) > 0.0)) && !has_phase(PHASE_B|PHASE_C))
		{
			gl_warning("load:%d %s - delta-connected load components for xxx_B need phases BC to be present",obj->id,(obj->name?obj->name:"Unnamed"));
			/*   TROUBLESHOOT
			A load object has a delta connection implied in the phase, but the load specified requires more phases than are present on the object.
			e.g., constant_power_B for a delta connection implies phases must include BC, but one of these is missing.  This load value will not
			be calculated correctly and/or excluded from the powerflow.
			*/
		}

		if (((constant_power[2].Mag() > 0.0) || (constant_current[2].Mag() > 0.0) || (constant_impedance[2].Mag() > 0.0) || (fabs(base_power[2]) > 0.0)) && !has_phase(PHASE_C|PHASE_A))
		{
			gl_warning("load:%d %s - delta-connected load components for xxx_C need phases CA to be present",obj->id,(obj->name?obj->name:"Unnamed"));
			/*   TROUBLESHOOT
			A load object has a delta connection implied in the phase, but the load specified requires more phases than are present on the object.
			e.g., constant_power_C for a delta connection implies phases must include CA, but one of these is missing.  This load value will not
			be calculated correctly and/or excluded from the powerflow.
			*/
		}
	}

	return ret_value;
}

TIMESTAMP load::presync(TIMESTAMP t0)
{
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::presync(t0);
	
	return result;
}

TIMESTAMP load::sync(TIMESTAMP t0)
{
	//bool all_three_phases;
	TIMESTAMP tresults_val, result;
	OBJECT *obj = OBJECTHDR(this);

	//Check for straggler nodes - fix so segfaults don't occur
	if ((prev_NTime==0) && (solver_method == SM_NR))
	{
		//See if we've been initialized yet - links typically do this, but single buses get missed
		if (NR_node_reference == -1)
		{
			//Call the populate routine
			NR_populate();
		}
		else if (NR_node_reference == -99)	//Child check us, since that can break things too
		{
			//See if our parent was initialized
			if (*NR_subnode_reference == -1)
			{
				//Try to initialize it, for giggles
				node *Temp_Node = OBJECTDATA(SubNodeParent,node);

				//Call the initialization
				Temp_Node->NR_populate();

				//Check it again
				if (*NR_subnode_reference == -1)
				{
					GL_THROW("load:%d - %s - Uninitialized parent is causing odd issues - fix your model!",obj->id,(obj->name?obj->name:"Unnamed"));
					/*  TROUBLESHOOT
					A childed load object is having issues mapping to its parent node - this typically happens in very odd cases of islanded/orphaned
					topologies that only contain nodes (no links).  Adjust your GLM to work around this issue.
					*/
				}
			}
		}
	}

	//Initialize time
	tresults_val = TS_NEVER;

	//Functionalized load updates - so deltamode can parttake
	load_update_fxn();

	//Must be at the bottom, or the new values will be calculated after the fact
	result = node::sync(t0);

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

	measured_power[0] = voltageA*(~current_inj[0]);
	measured_power[1] = voltageB*(~current_inj[1]);
	measured_power[2] = voltageC*(~current_inj[2]);
	measured_total_power = measured_power[0] + measured_power[1] + measured_power[2];

	return t1;
}

//Functional call to sync-level load updates
//Here primarily so deltamode players can actually influence things
void load::load_update_fxn(void)
{
	bool fault_mode;
	bool all_three_phases, transf_from_stdy_state;
	gld::complex intermed_impedance[3];
	gld::complex intermed_impedance_dy[6];
	int index_var, extract_node_ref;
	bool volt_below_thresh;
	double voltage_base_val;
	double voltage_pu_vals[3];
	gld::complex nominal_voltage_value;
	int node_reference_value;
	double curr_delta_time;
	bool require_inrush_update, local_shunt_update;
	gld::complex working_impedance_value, working_data_value, working_admittance_value;
	double workingvalue;
	OBJECT *obj = nullptr;
	double baseangles[3];
	node *temp_par_node = nullptr;
	gld::complex adjusted_FBS_current[6];
	gld::complex adjust_FBS_temp_nominal_voltage[6];
	double adjust_FBS_temp_voltage_mag[6];
	double adjust_FBS_nominal_voltage_val[6];
	gld::complex adjust_FBS_voltage_val[6];

	//See if we're reliability-enabled
	if (fault_check_object == nullptr)
		fault_mode = false;
	else
		fault_mode = true;

	//Roll GFA check into here, so current loads updates are handled properly
	//See if GFA functionality is enabled
	if (GFA_enable)
	{
		//See if we're enabled - just skipping the load update should be enough, if we are not
		if (!GFA_status)
		{
			//Remove any load contributions
			load_delete_update_fxn();

			//Exit
			return;
		}
	}
	//Default elses - just continue like normal

	//Remove prior contributions - do this first so everything else can just accumulate to the trackers and accumalators below
	for (index_var=0; index_var<3; index_var++)
	{
		//Remove last values - stops massive accumulations
		shunt[index_var] -= prev_load_values[0][index_var];
		current[index_var] -= prev_load_values[1][index_var];
		power[index_var] -= prev_load_values[2][index_var];

		//Remove last values - explicit connections
		shunt_dy[index_var] -= prev_load_values_dy[0][index_var];
		current_dy[index_var] -= prev_load_values_dy[1][index_var];
		power_dy[index_var] -= prev_load_values_dy[2][index_var];

		shunt_dy[index_var+3] -= prev_load_values_dy[0][index_var+3];
		current_dy[index_var+3] -= prev_load_values_dy[1][index_var+3];
		power_dy[index_var+3] -= prev_load_values_dy[2][index_var+3];

		//Zero the trackers too
		prev_load_values[0][index_var] = gld::complex(0.0,0.0);
		prev_load_values[1][index_var] = gld::complex(0.0,0.0);
		prev_load_values[2][index_var] = gld::complex(0.0,0.0);

		//Zero the explicit trackers
		prev_load_values_dy[0][index_var] = gld::complex(0.0,0.0);
		prev_load_values_dy[1][index_var] = gld::complex(0.0,0.0);
		prev_load_values_dy[2][index_var] = gld::complex(0.0,0.0);
		prev_load_values_dy[0][index_var+3] = gld::complex(0.0,0.0);
		prev_load_values_dy[1][index_var+3] = gld::complex(0.0,0.0);
		prev_load_values_dy[2][index_var+3] = gld::complex(0.0,0.0);
	}

	//Set base angles for ZIP calculations below
	//Note this assumption HAS to be done this way -- while the ZIP calculation below
	//correctly rotates for deltamode, no other constant current values do, which means this one
	//"correct" implementation breaks all the other assumptions.  Therefore, it has to be "stupidified"
	//so it works with the auto-rotation and impedance-conversion code below
	baseangles[0] = 0.0;
	baseangles[1] = -2.0*PI/3.0;
	baseangles[2] = 2.0*PI/3.0;

	for (int index=0; index<3; index++)
	{
		if (base_power[index] != 0.0)
		{
			//Set the flag
			base_load_val_was_nonzero[index] = true;

			if (power_fraction[index] + current_fraction[index] + impedance_fraction[index] != 1.0)
			{	
				power_fraction[index] = 1 - current_fraction[index] - impedance_fraction[index];
				
				char temp[3] = {'A','B','C'};

				OBJECT *obj = OBJECTHDR(this);

				gl_warning("load:%s - ZIP components on phase %c did not sum to 1. Setting power_fraction to %.2f", obj->name ? obj->name : "unnamed", temp[index], power_fraction[index]);
				/*  TROUBLESHOOT
				ZIP load fractions must sum to 1.  The values (power_fraction_X, impedance_fraction_X, and current_fraction_X) on the given phase did not sum to 1. To 
				ensure that constraint (Z+I+P=1), power_fraction is being calculated and overwritten.
				*/
			}

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
				constant_power[index] = gld::complex(real_power,imag_power);
			}
			else
			{
				constant_power[index] = gld::complex(0,0);
			}
		
			// Put in the constant current portion and adjust the angle
			if (current_fraction[index] != 0.0)
			{
				double real_power,imag_power,temp_angle;
				gld::complex temp_curr;

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
				if (has_phase(PHASE_D)) {
				  temp_curr = ~gld::complex(real_power,imag_power) / gld::complex(sqrt(3.0)*nominal_voltage,0);
				  temp_angle = temp_curr.Arg() + baseangles[index] + PI/6.0;
				} else {
				  temp_curr = ~gld::complex(real_power,imag_power) / gld::complex(nominal_voltage,0);
				  temp_angle = temp_curr.Arg() + baseangles[index];
				}

				//Create the corrected value
				temp_curr.SetPolar(temp_curr.Mag(),temp_angle);

				constant_current[index] = temp_curr;
			}
			else
			{
				constant_current[index] = gld::complex(0,0);
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

				//Compute the impedance value - adjust for delta-connection, as necessary
				if (has_phase(PHASE_D)) {
				  constant_impedance[index] = ~( gld::complex(3.0 * nominal_voltage * nominal_voltage, 0) / gld::complex(real_power,imag_power) );
				} else {
				  constant_impedance[index] = ~( gld::complex(nominal_voltage * nominal_voltage, 0) / gld::complex(real_power,imag_power) );
				}
			}
			else
			{
				constant_impedance[index] = gld::complex(0,0);
			}
		}
		else if (base_load_val_was_nonzero[index])	//Zero case, be sure to re-zero it
		{
			//zero all components
			constant_power[index] = gld::complex(0.0,0.0);
			constant_current[index] = gld::complex(0.0,0.0);
			constant_impedance[index] = gld::complex(0.0,0.0);

			//Deflag us
			base_load_val_was_nonzero[index] = false;
		}
	}

	//Perform the intermediate impedance calculations, if necessary
	if (enable_frequency_dependence)
	{
		//Check impedance values for normal connections
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

		//Check impedance values for explicit delta/wye connections
		for (index_var=0; index_var<6; index_var++)
		{
			if (!constant_impedance_dy[index_var].IsZero())
			{
				//Assign real part
				intermed_impedance_dy[index_var].SetReal(constant_impedance_dy[index_var].Re());
				
				//Assign reactive part -- apply fix based on inductance/capacitance
				if (constant_impedance_dy[index_var].Im()<0)	//Capacitive
				{
					intermed_impedance_dy[index_var].SetImag(constant_impedance_dy[index_var].Im()/current_frequency*nominal_frequency);
				}
				else	//Inductive
				{
					intermed_impedance_dy[index_var].SetImag(constant_impedance_dy[index_var].Im()/nominal_frequency*current_frequency);
				}
			}
			else
			{
				intermed_impedance_dy[index_var] = 0.0;
			}
		}
	}
	else //normal
	{
		intermed_impedance[0] = constant_impedance[0];
		intermed_impedance[1] = constant_impedance[1];
		intermed_impedance[2] = constant_impedance[2];

		//Do the same for explicit delta/wye connections -- both parent types get this
		for (index_var=0; index_var<6; index_var++)
		{
			intermed_impedance_dy[index_var] = constant_impedance_dy[index_var];
		}
	}

	if (!fault_mode && !enable_impedance_conversion)	//Not reliability mode - normal mode
	{
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

		//Adjustments for FBS - NR does them in solver_NR (and stays there, due to phasing for reliability)
		if (solver_method == SM_FBS)
		{
			//Compute constants (just do it once)
			//Set nominal voltage values - set up for explicit down below
			adjust_FBS_nominal_voltage_val[0] = nominal_voltage * sqrt(3.0);	//Delta
			adjust_FBS_nominal_voltage_val[1] = nominal_voltage * sqrt(3.0);
			adjust_FBS_nominal_voltage_val[2] = nominal_voltage * sqrt(3.0);
			adjust_FBS_nominal_voltage_val[3] = nominal_voltage;				//Wye
			adjust_FBS_nominal_voltage_val[4] = nominal_voltage;
			adjust_FBS_nominal_voltage_val[5] = nominal_voltage;

			//Just copy in the voltages (and compute delta) - mostly for explicit calculation, so the loop can just use it
			adjust_FBS_voltage_val[0] = voltage[0] - voltage[1];
			adjust_FBS_voltage_val[1] = voltage[1] - voltage[2];
			adjust_FBS_voltage_val[2] = voltage[2] - voltage[0];
			adjust_FBS_voltage_val[3] = voltage[0];
			adjust_FBS_voltage_val[4] = voltage[1];
			adjust_FBS_voltage_val[5] = voltage[2];

			//Create the nominal voltage vectors - delta first
			adjust_FBS_temp_nominal_voltage[0].SetPolar(adjust_FBS_nominal_voltage_val[0],PI/6.0);
			adjust_FBS_temp_nominal_voltage[1].SetPolar(adjust_FBS_nominal_voltage_val[1],-1.0*PI/2.0);
			adjust_FBS_temp_nominal_voltage[2].SetPolar(adjust_FBS_nominal_voltage_val[2],5.0*PI/6.0);
			adjust_FBS_temp_nominal_voltage[3].SetPolar(adjust_FBS_nominal_voltage_val[3],0.0);
			adjust_FBS_temp_nominal_voltage[4].SetPolar(adjust_FBS_nominal_voltage_val[4],-2.0*PI/3.0);
			adjust_FBS_temp_nominal_voltage[5].SetPolar(adjust_FBS_nominal_voltage_val[5],2.0*PI/3.0);

			//Get magnitudes of all
			adjust_FBS_temp_voltage_mag[0] = adjust_FBS_voltage_val[0].Mag();
			adjust_FBS_temp_voltage_mag[1] = adjust_FBS_voltage_val[1].Mag();
			adjust_FBS_temp_voltage_mag[2] = adjust_FBS_voltage_val[2].Mag();
			adjust_FBS_temp_voltage_mag[3] = adjust_FBS_voltage_val[3].Mag();
			adjust_FBS_temp_voltage_mag[4] = adjust_FBS_voltage_val[4].Mag();
			adjust_FBS_temp_voltage_mag[5] = adjust_FBS_voltage_val[5].Mag();

			//See if we're delta or not
			if (has_phase(PHASE_D))
			{
				//Start adjustments - AB
				if ((constant_current[0] != 0.0) && (adjust_FBS_temp_voltage_mag[0] != 0.0))
				{
					//calculate new value
					adjusted_FBS_current[0] = ~(adjust_FBS_temp_nominal_voltage[0] * ~constant_current[0] * adjust_FBS_temp_voltage_mag[0] / (adjust_FBS_voltage_val[0] * adjust_FBS_nominal_voltage_val[0]));
				}
				else
				{
					adjusted_FBS_current[0] = gld::complex(0.0,0.0);
				}

				//Start adjustments - BC
				if ((constant_current[1] != 0.0) && (adjust_FBS_temp_voltage_mag[1] != 0.0))
				{
					//calculate new value
					adjusted_FBS_current[1] = ~(adjust_FBS_temp_nominal_voltage[1] * ~constant_current[1] * adjust_FBS_temp_voltage_mag[1] / (adjust_FBS_voltage_val[1] * adjust_FBS_nominal_voltage_val[1]));
				}
				else
				{
					adjusted_FBS_current[1] = gld::complex(0.0,0.0);
				}

				//Start adjustments - CA
				if ((constant_current[2] != 0.0) && (adjust_FBS_temp_voltage_mag[2] != 0.0))
				{
					//calculate new value
					adjusted_FBS_current[2] = ~(adjust_FBS_temp_nominal_voltage[2] * ~constant_current[2] * adjust_FBS_temp_voltage_mag[2] / (adjust_FBS_voltage_val[2] * adjust_FBS_nominal_voltage_val[2]));
				}
				else
				{
					adjusted_FBS_current[2] = gld::complex(0.0,0.0);
				}
			}
			else	//Wye
			{
				//Start adjustments - A
				if ((constant_current[0] != 0.0) && (adjust_FBS_temp_voltage_mag[3] != 0.0))
				{
					//calculate new value
					adjusted_FBS_current[0] = ~(adjust_FBS_temp_nominal_voltage[3] * ~constant_current[0] * adjust_FBS_temp_voltage_mag[3] / (adjust_FBS_voltage_val[3] * adjust_FBS_nominal_voltage_val[3]));
				}
				else
				{
					adjusted_FBS_current[0] = gld::complex(0.0,0.0);
				}

				//Start adjustments - B
				if ((constant_current[1] != 0.0) && (adjust_FBS_temp_voltage_mag[4] != 0.0))
				{
					//calculate new value
					adjusted_FBS_current[1] = ~(adjust_FBS_temp_nominal_voltage[4] * ~constant_current[1] * adjust_FBS_temp_voltage_mag[4] / (adjust_FBS_voltage_val[4] * adjust_FBS_nominal_voltage_val[4]));
				}
				else
				{
					adjusted_FBS_current[1] = gld::complex(0.0,0.0);
				}

				//Start adjustments - C
				if ((constant_current[2] != 0.0) && (adjust_FBS_temp_voltage_mag[5] != 0.0))
				{
					//calculate new value
					adjusted_FBS_current[2] = ~(adjust_FBS_temp_nominal_voltage[5] * ~constant_current[2] * adjust_FBS_temp_voltage_mag[5] / (adjust_FBS_voltage_val[5] * adjust_FBS_nominal_voltage_val[5]));
				}
				else
				{
					adjusted_FBS_current[2] = gld::complex(0.0,0.0);
				}
			}

			//Accumulate, same as below
			current[0] += adjusted_FBS_current[0];
			current[1] += adjusted_FBS_current[1];
			current[2] += adjusted_FBS_current[2];

			//Current trackers
			prev_load_values[1][0] += adjusted_FBS_current[0];
			prev_load_values[1][1] += adjusted_FBS_current[1];
			prev_load_values[1][2] += adjusted_FBS_current[2];
		}
		else
		{
			current[0] += constant_current[0];
			current[1] += constant_current[1];
			current[2] += constant_current[2];

			//Ccurrent trackers
			prev_load_values[1][0] += constant_current[0];
			prev_load_values[1][1] += constant_current[1];
			prev_load_values[1][2] += constant_current[2];
		}

		//Power trackers
		prev_load_values[2][0] += constant_power[0];
		prev_load_values[2][1] += constant_power[1];
		prev_load_values[2][2] += constant_power[2];

		//Do the same for explicit delta/wye connections
		for (index_var=0; index_var<6; index_var++)
		{
			if (!intermed_impedance_dy[index_var].IsZero())
			{
				//Accumulator
				shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

				//Tracker
				prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
			}

			power_dy[index_var] += constant_power_dy[index_var];

			//Adjustments for FBS - NR does them in solver_NR (and stays there, due to phasing for reliability)
			if (solver_method == SM_FBS)
			{
				//Start adjustments - per-index
				if ((constant_current_dy[index_var] != 0.0) && (adjust_FBS_temp_voltage_mag[index_var] != 0.0))
				{
					//calculate new value
					adjusted_FBS_current[index_var] = ~(adjust_FBS_temp_nominal_voltage[index_var] * ~constant_current_dy[index_var] * adjust_FBS_temp_voltage_mag[index_var] / (adjust_FBS_voltage_val[index_var] * adjust_FBS_nominal_voltage_val[index_var]));
				}
				else
				{
					adjusted_FBS_current[index_var] = gld::complex(0.0,0.0);
				}

				//Accumulate into the variable
				current_dy[index_var] += adjusted_FBS_current[index_var];

				//Update current trackers
				prev_load_values_dy[1][index_var] += adjusted_FBS_current[index_var];
			}
			else
			{
				//Accumulate into the variable
				current_dy[index_var] += constant_current_dy[index_var];

				//Update current trackers
				prev_load_values_dy[1][index_var] += constant_current_dy[index_var];
			}

			//Update power trackers
			prev_load_values_dy[2][index_var] += constant_power_dy[index_var];
		}
	}//End normal mode
	else	//Reliability mode
	{
		all_three_phases = true;	//By default, handle all the phases

		//Check and see if we're a "protected" load
		if (three_phase_protect)
		{
			if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
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

		if (all_three_phases)	//Handle all phases correctly
		{
			if ((solver_method!=SM_FBS) && ((SubNode & (SNT_PARENT | SNT_DIFF_PARENT)) != 0))	//Need to do something slightly different with GS/NR and parented load
			{													//associated with change due to player methods
				if ((SubNode & SNT_PARENT) == SNT_PARENT)	//Normal parents need this
				{
					//See if in-rush is enabled - only makes sense in reliability situations
					if (enable_inrush_calculations || enable_impedance_conversion)
					{
						//Reset variable
						volt_below_thresh = false;

						//Check to see if we're under the voltage pu limit or not -- any version under will trip it
						if (has_phase(PHASE_D))
						{
							voltage_base_val = nominal_voltage * sqrt(3.0);

							//Compute per-unit values
							voltage_pu_vals[0] = voltaged[0].Mag()/voltage_base_val;
							voltage_pu_vals[1] = voltaged[1].Mag()/voltage_base_val;
							voltage_pu_vals[2] = voltaged[2].Mag()/voltage_base_val;

							//Check them - Phase AB
							if (((NR_busdata[NR_node_reference].phases & 0x06) == 0x06) && (voltage_pu_vals[0] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase BC
							if (((NR_busdata[NR_node_reference].phases & 0x03) == 0x03) && (voltage_pu_vals[1] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase CA
							if (((NR_busdata[NR_node_reference].phases & 0x05) == 0x05) && (voltage_pu_vals[2] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}
						}
						else	//Must be Wye-connected
						{
							//Set nominal voltage
							voltage_base_val = nominal_voltage;

							//Compute per-unit values
							voltage_pu_vals[0] = voltage[0].Mag()/voltage_base_val;
							voltage_pu_vals[1] = voltage[1].Mag()/voltage_base_val;
							voltage_pu_vals[2] = voltage[2].Mag()/voltage_base_val;

							//Check them - Phase A
							if (((NR_busdata[NR_node_reference].phases & 0x04) == 0x04) && (voltage_pu_vals[0] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase B
							if (((NR_busdata[NR_node_reference].phases & 0x02) == 0x02) && (voltage_pu_vals[1] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase C
							if (((NR_busdata[NR_node_reference].phases & 0x01) == 0x01) && (voltage_pu_vals[2] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}
						}

						//See if tripped
						if (volt_below_thresh)
						{
							//Convert all loads to an impedance-equivalent
							if (has_phase(PHASE_D))	//Delta-connected
							{
								//Reset - out of paranoia
								voltage_base_val = nominal_voltage * sqrt(3.0);

								//Check phases - AB
								if ((NR_busdata[NR_node_reference].phases & 0x06) == 0x06)
								{
									//Check power value
									if (!(constant_power[0].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[0] += ~(nominal_voltage_value/constant_power[0]);
									}

									//Check current
									if (!(constant_current[0].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,PI/6);

										//Compute the value
										intermed_impedance[0] += nominal_voltage_value/constant_current[0];
									}
								}//End AB check

								//Check phases - BC
								if ((NR_busdata[NR_node_reference].phases & 0x03) == 0x03)
								{
									//Check power value
									if (!(constant_power[1].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[1] += ~(nominal_voltage_value/constant_power[1]);
									}

									//Check current
									if (!(constant_current[1].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,-1.0*PI/2.0);

										//Compute the value
										intermed_impedance[1] += nominal_voltage_value/constant_current[1];
									}
								}//End BC check

								//Check phases - CA
								if ((NR_busdata[NR_node_reference].phases & 0x05) == 0x05)
								{
									//Check power value
									if (!(constant_power[2].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[2] += ~(nominal_voltage_value/constant_power[2]);
									}

									//Check current
									if (!(constant_current[2].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,5.0*PI/6.0);

										//Compute the value
										intermed_impedance[2] += nominal_voltage_value/constant_current[2];
									}
								}//End CA check
							}//End Delta connection
							else	//Wye-connected
							{
								//Reset - out of paranoia
								voltage_base_val = nominal_voltage;

								//Check phases - A
								if ((NR_busdata[NR_node_reference].phases & 0x04) == 0x04)
								{
									//Check power value
									if (!(constant_power[0].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[0] += ~(nominal_voltage_value/constant_power[0]);
									}

									//Check current
									if (!(constant_current[0].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value = gld::complex(voltage_base_val,0.0);

										//Compute the value
										intermed_impedance[0] += nominal_voltage_value/constant_current[0];
									}
								}//End A check

								//Check phases - B
								if ((NR_busdata[NR_node_reference].phases & 0x02) == 0x02)
								{
									//Check power value
									if (!(constant_power[1].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[1] += ~(nominal_voltage_value/constant_power[1]);
									}

									//Check current
									if (!(constant_current[1].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,-2.0*PI/3.0);

										//Compute the value
										intermed_impedance[1] += nominal_voltage_value/constant_current[1];
									}
								}//End B check

								//Check phases - C
								if ((NR_busdata[NR_node_reference].phases & 0x01) == 0x01)
								{
									//Check power value
									if (!(constant_power[2].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[2] += ~(nominal_voltage_value/constant_power[2]);
									}

									//Check current
									if (!(constant_current[2].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,2.0*PI/3.0);

										//Compute the value
										intermed_impedance[2] += nominal_voltage_value/constant_current[2];
									}
								}//End C check
							}//End wye connection

							//Add the values in
							if (!(intermed_impedance[0].IsZero()))
							{
								//Accumulator
								shunt[0] += gld::complex(1.0)/intermed_impedance[0];

								//Tracker
								prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
							}

							if (!(intermed_impedance[1].IsZero()))
							{
								//Accumulator
								shunt[1] += gld::complex(1.0)/intermed_impedance[1];

								//Tracker
								prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
							}
							
							if (!(intermed_impedance[2].IsZero()))
							{
								//Accumulator
								shunt[2] += gld::complex(1.0)/intermed_impedance[2];

								//Tracker
								prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
							}

							//Power and current remain zero

							//Combination load section

							// *********** DELTA Portions *************
							//Set new base value
							voltage_base_val = nominal_voltage * sqrt(3.0);

							//Check phases - AB
							if ((NR_busdata[NR_node_reference].phases & 0x06) == 0x06)
							{
								//Check power value
								if (!(constant_power_dy[0].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[0] += ~(nominal_voltage_value/constant_power_dy[0]);
								}

								//Check current
								if (!(constant_current_dy[0].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,PI/6);

									//Compute the value
									intermed_impedance_dy[0] += nominal_voltage_value/constant_current_dy[0];
								}
							}//End AB check

							//Check phases - BC
							if ((NR_busdata[NR_node_reference].phases & 0x03) == 0x03)
							{
								//Check power value
								if (!(constant_power_dy[1].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[1] += ~(nominal_voltage_value/constant_power_dy[1]);
								}

								//Check current
								if (!(constant_current_dy[1].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,-1.0*PI/2.0);

									//Compute the value
									intermed_impedance_dy[1] += nominal_voltage_value/constant_current_dy[1];
								}
							}//End BC check

							//Check phases - CA
							if ((NR_busdata[NR_node_reference].phases & 0x05) == 0x05)
							{
								//Check power value
								if (!(constant_power_dy[2].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[2] += ~(nominal_voltage_value/constant_power_dy[2]);
								}

								//Check current
								if (!(constant_current_dy[2].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,5.0*PI/6.0);

									//Compute the value
									intermed_impedance_dy[2] += nominal_voltage_value/constant_current_dy[2];
								}
							}//End CA check

							// ********** WYE portions *****************
							//Set new base value
							voltage_base_val = nominal_voltage;

							//Check phases - A
							if ((NR_busdata[NR_node_reference].phases & 0x04) == 0x04)
							{
								//Check power value
								if (!(constant_power_dy[3].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[3] += ~(nominal_voltage_value/constant_power_dy[3]);
								}

								//Check current
								if (!(constant_current_dy[3].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value = gld::complex(voltage_base_val,0.0);

									//Compute the value
									intermed_impedance_dy[3] += nominal_voltage_value/constant_current_dy[3];
								}
							}//End A check

							//Check phases - B
							if ((NR_busdata[NR_node_reference].phases & 0x02) == 0x02)
							{
								//Check power value
								if (!(constant_power_dy[4].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[4] += ~(nominal_voltage_value/constant_power_dy[4]);
								}

								//Check current
								if (!(constant_current_dy[4].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,-2.0*PI/3.0);

									//Compute the value
									intermed_impedance_dy[4] += nominal_voltage_value/constant_current_dy[4];
								}
							}//End B check

							//Check phases - C
							if ((NR_busdata[NR_node_reference].phases & 0x01) == 0x01)
							{
								//Check power value
								if (!(constant_power_dy[5].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[5] += ~(nominal_voltage_value/constant_power_dy[5]);
								}

								//Check current
								if (!(constant_current_dy[5].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,2.0*PI/3.0);

									//Compute the value
									intermed_impedance_dy[5] += nominal_voltage_value/constant_current_dy[5];
								}
							}//End C check

							//Do the updated portion
							for (index_var=0; index_var<6; index_var++)
							{
								if (!intermed_impedance_dy[index_var].IsZero())
								{
									//Accumulator
									shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

									//Tracker
									prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
								}
							}
						}//End below thresh - convert
						else	//Must be above, be "normal"
						{
							if (!(intermed_impedance[0].IsZero()))
							{
								//Accumulator
								shunt[0] += gld::complex(1.0)/intermed_impedance[0];

								//Tracker
								prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
							}

							if (!(intermed_impedance[1].IsZero()))
							{
								//Accumulator
								shunt[1] += gld::complex(1.0)/intermed_impedance[1];

								//Tracker
								prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
							}
							
							if (!(intermed_impedance[2].IsZero()))
							{
								//Accumulator
								shunt[2] += gld::complex(1.0)/intermed_impedance[2];

								//Tracker
								prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
							}
							
							//Current and power accumulators
							power[0] += constant_power[0];
							power[1] += constant_power[1];	
							power[2] += constant_power[2];

							current[0] += constant_current[0];
							current[1] += constant_current[1];
							current[2] += constant_current[2];

							//Power and current trackers
							prev_load_values[1][0] += constant_current[0];
							prev_load_values[1][1] += constant_current[1];
							prev_load_values[1][2] += constant_current[2];
							prev_load_values[2][0] += constant_power[0];
							prev_load_values[2][1] += constant_power[1];
							prev_load_values[2][2] += constant_power[2];

							//Combination portion
							//Do the same for explicit delta/wye connections
							for (index_var=0; index_var<6; index_var++)
							{
								if (!intermed_impedance_dy[index_var].IsZero())
								{
									//Accumulator
									shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

									//Tracker
									prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
								}

								power_dy[index_var] += constant_power_dy[index_var];
								current_dy[index_var] += constant_current_dy[index_var];

								//Update power/current trackers
								prev_load_values_dy[1][index_var] += constant_current_dy[index_var];
								prev_load_values_dy[2][index_var] += constant_power_dy[index_var];
							}
						}//End "perform normally"
					}//end in-rush
					else	//False, so go normal
					{
						if (!(intermed_impedance[0].IsZero()))
						{
							//Accumulator
							shunt[0] += gld::complex(1.0)/intermed_impedance[0];

							//Tracker
							prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
						}

						if (!(intermed_impedance[1].IsZero()))
						{
							//Accumulator
							shunt[1] += gld::complex(1.0)/intermed_impedance[1];

							//Tracker
							prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
						}
						
						if (!(intermed_impedance[2].IsZero()))
						{
							//Accumulator
							shunt[2] += gld::complex(1.0)/intermed_impedance[2];

							//Tracker
							prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
						}
						
						//Current and power accumulators
						power[0] += constant_power[0];
						power[1] += constant_power[1];	
						power[2] += constant_power[2];

						current[0] += constant_current[0];
						current[1] += constant_current[1];
						current[2] += constant_current[2];

						//Power and current trackers
						prev_load_values[1][0] += constant_current[0];
						prev_load_values[1][1] += constant_current[1];
						prev_load_values[1][2] += constant_current[2];
						prev_load_values[2][0] += constant_power[0];
						prev_load_values[2][1] += constant_power[1];
						prev_load_values[2][2] += constant_power[2];

						//Combination Portion
						//Do the same for explicit delta/wye connections
						for (index_var=0; index_var<6; index_var++)
						{
							if (!intermed_impedance_dy[index_var].IsZero())
							{
								//Accumulator
								shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

								//Tracker
								prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
							}

							power_dy[index_var] += constant_power_dy[index_var];
							current_dy[index_var] += constant_current_dy[index_var];

							//Update power/current trackers
							prev_load_values_dy[1][index_var] += constant_current_dy[index_var];
							prev_load_values_dy[2][index_var] += constant_power_dy[index_var];
						}
					}//End non-inrush
				}//End normal parent
				else //DIFF_PARENT
				{
					//See if in-rush is enabled - only makes sense in reliability situations
					if (enable_inrush_calculations || enable_impedance_conversion)
					{
						//Reset variable
						volt_below_thresh = false;

						//Check to see if we're under the voltage pu limit or not -- any version under will trip it
						if (has_phase(PHASE_D))
						{
							voltage_base_val = nominal_voltage * sqrt(3.0);

							//Compute per-unit values
							voltage_pu_vals[0] = voltaged[0].Mag()/voltage_base_val;
							voltage_pu_vals[1] = voltaged[1].Mag()/voltage_base_val;
							voltage_pu_vals[2] = voltaged[2].Mag()/voltage_base_val;

							//Check them - Phase AB
							if (((NR_busdata[NR_node_reference].phases & 0x06) == 0x06) && (voltage_pu_vals[0] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase BC
							if (((NR_busdata[NR_node_reference].phases & 0x03) == 0x03) && (voltage_pu_vals[1] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase CA
							if (((NR_busdata[NR_node_reference].phases & 0x05) == 0x05) && (voltage_pu_vals[2] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}
						}
						else	//Must be Wye-connected
						{
							//Set nominal voltage
							voltage_base_val = nominal_voltage;

							//Compute per-unit values
							voltage_pu_vals[0] = voltage[0].Mag()/voltage_base_val;
							voltage_pu_vals[1] = voltage[1].Mag()/voltage_base_val;
							voltage_pu_vals[2] = voltage[2].Mag()/voltage_base_val;

							//Check them - Phase A
							if (((NR_busdata[NR_node_reference].phases & 0x04) == 0x04) && (voltage_pu_vals[0] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase B
							if (((NR_busdata[NR_node_reference].phases & 0x02) == 0x02) && (voltage_pu_vals[1] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}

							//Check them - Phase C
							if (((NR_busdata[NR_node_reference].phases & 0x01) == 0x01) && (voltage_pu_vals[2] < impedance_conversion_low_pu))
							{
								volt_below_thresh = true;
							}
						}

						//See if tripped
						if (volt_below_thresh)
						{
							//Convert all loads to an impedance-equivalent
							if (has_phase(PHASE_D))	//Delta-connected
							{
								//Reset - out of paranoia
								voltage_base_val = nominal_voltage * sqrt(3.0);

								//Check phases - AB
								if ((NR_busdata[NR_node_reference].phases & 0x06) == 0x06)
								{
									//Check power value
									if (!(constant_power[0].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[0] += ~(nominal_voltage_value/constant_power[0]);
									}

									//Check current
									if (!(constant_current[0].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,PI/6);

										//Compute the value
										intermed_impedance[0] += nominal_voltage_value/constant_current[0];
									}
								}//End AB check

								//Check phases - BC
								if ((NR_busdata[NR_node_reference].phases & 0x03) == 0x03)
								{
									//Check power value
									if (!(constant_power[1].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[1] += ~(nominal_voltage_value/constant_power[1]);
									}

									//Check current
									if (!(constant_current[1].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,-1.0*PI/2.0);

										//Compute the value
										intermed_impedance[1] += nominal_voltage_value/constant_current[1];
									}
								}//End BC check

								//Check phases - CA
								if ((NR_busdata[NR_node_reference].phases & 0x05) == 0x05)
								{
									//Check power value
									if (!(constant_power[2].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[2] += ~(nominal_voltage_value/constant_power[2]);
									}

									//Check current
									if (!(constant_current[2].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,5.0*PI/6.0);

										//Compute the value
										intermed_impedance[2] += nominal_voltage_value/constant_current[2];
									}
								}//End CA check
							}//End Delta connection
							else	//Wye-connected
							{
								//Reset - out of paranoia
								voltage_base_val = nominal_voltage;

								//Check phases - A
								if ((NR_busdata[NR_node_reference].phases & 0x04) == 0x04)
								{
									//Check power value
									if (!(constant_power[0].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[0] += ~(nominal_voltage_value/constant_power[0]);
									}

									//Check current
									if (!(constant_current[0].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value = gld::complex(voltage_base_val,0.0);

										//Compute the value
										intermed_impedance[0] += nominal_voltage_value/constant_current[0];
									}
								}//End A check

								//Check phases - B
								if ((NR_busdata[NR_node_reference].phases & 0x02) == 0x02)
								{
									//Check power value
									if (!(constant_power[1].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[1] += ~(nominal_voltage_value/constant_power[1]);
									}

									//Check current
									if (!(constant_current[1].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,-2.0*PI/3.0);

										//Compute the value
										intermed_impedance[1] += nominal_voltage_value/constant_current[1];
									}
								}//End B check

								//Check phases - C
								if ((NR_busdata[NR_node_reference].phases & 0x01) == 0x01)
								{
									//Check power value
									if (!(constant_power[2].IsZero()))
									{
										//Set nominal voltage - |V|^2
										nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

										//Compute the value
										intermed_impedance[2] += ~(nominal_voltage_value/constant_power[2]);
									}

									//Check current
									if (!(constant_current[2].IsZero()))
									{
										//Set nominal voltage - actual angle
										nominal_voltage_value.SetPolar(voltage_base_val,2.0*PI/3.0);

										//Compute the value
										intermed_impedance[2] += nominal_voltage_value/constant_current[2];
									}
								}//End C check
							}//End wye connection

							//Add the values in
							if (!(intermed_impedance[0].IsZero()))
							{
								//Accumulator
								shunt[0] += gld::complex(1.0)/intermed_impedance[0];

								//Tracker
								prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
							}

							if (!(intermed_impedance[1].IsZero()))
							{
								//Accumulator
								shunt[1] += gld::complex(1.0)/intermed_impedance[1];

								//Tracker
								prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
							}
							
							if (!(intermed_impedance[2].IsZero()))
							{
								//Accumulator
								shunt[2] += gld::complex(1.0)/intermed_impedance[2];

								//Tracker
								prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
							}

							//Power and current remain zero
							
							//Combination load section

							// *********** DELTA Portions *************
							//Set new base value
							voltage_base_val = nominal_voltage * sqrt(3.0);

							//Check phases - AB
							if ((NR_busdata[NR_node_reference].phases & 0x06) == 0x06)
							{
								//Check power value
								if (!(constant_power_dy[0].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[0] += ~(nominal_voltage_value/constant_power_dy[0]);
								}

								//Check current
								if (!(constant_current_dy[0].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,PI/6);

									//Compute the value
									intermed_impedance_dy[0] += nominal_voltage_value/constant_current_dy[0];
								}
							}//End AB check

							//Check phases - BC
							if ((NR_busdata[NR_node_reference].phases & 0x03) == 0x03)
							{
								//Check power value
								if (!(constant_power_dy[1].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[1] += ~(nominal_voltage_value/constant_power_dy[1]);
								}

								//Check current
								if (!(constant_current_dy[1].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,-1.0*PI/2.0);

									//Compute the value
									intermed_impedance_dy[1] += nominal_voltage_value/constant_current_dy[1];
								}
							}//End BC check

							//Check phases - CA
							if ((NR_busdata[NR_node_reference].phases & 0x05) == 0x05)
							{
								//Check power value
								if (!(constant_power_dy[2].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[2] += ~(nominal_voltage_value/constant_power_dy[2]);
								}

								//Check current
								if (!(constant_current_dy[2].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,5.0*PI/6.0);

									//Compute the value
									intermed_impedance_dy[2] += nominal_voltage_value/constant_current_dy[2];
								}
							}//End CA check

							// ********** WYE portions *****************
							//Set new base value
							voltage_base_val = nominal_voltage;

							//Check phases - A
							if ((NR_busdata[NR_node_reference].phases & 0x04) == 0x04)
							{
								//Check power value
								if (!(constant_power_dy[3].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[3] += ~(nominal_voltage_value/constant_power_dy[3]);
								}

								//Check current
								if (!(constant_current_dy[3].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value = gld::complex(voltage_base_val,0.0);

									//Compute the value
									intermed_impedance_dy[3] += nominal_voltage_value/constant_current_dy[3];
								}
							}//End A check

							//Check phases - B
							if ((NR_busdata[NR_node_reference].phases & 0x02) == 0x02)
							{
								//Check power value
								if (!(constant_power_dy[4].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[4] += ~(nominal_voltage_value/constant_power_dy[4]);
								}

								//Check current
								if (!(constant_current_dy[4].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,-2.0*PI/3.0);

									//Compute the value
									intermed_impedance_dy[4] += nominal_voltage_value/constant_current_dy[4];
								}
							}//End B check

							//Check phases - C
							if ((NR_busdata[NR_node_reference].phases & 0x01) == 0x01)
							{
								//Check power value
								if (!(constant_power_dy[5].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance_dy[5] += ~(nominal_voltage_value/constant_power_dy[5]);
								}

								//Check current
								if (!(constant_current_dy[5].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,2.0*PI/3.0);

									//Compute the value
									intermed_impedance_dy[5] += nominal_voltage_value/constant_current_dy[5];
								}
							}//End C check

							//Do the updated portion
							for (index_var=0; index_var<6; index_var++)
							{
								if (!intermed_impedance_dy[index_var].IsZero())
								{
									//Accumulator
									shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

									//Tracker
									prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
								}
							}
						}//End below thresh - convert
						else	//Must be above, be "normal"
						{
							//Apply the values
							if (!(intermed_impedance[0].IsZero()))
							{
								//Accumulator
								shunt[0] += gld::complex(1.0)/intermed_impedance[0];

								//Tracker
								prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
							}

							if (!(intermed_impedance[1].IsZero()))
							{
								//Accumulator
								shunt[1] += gld::complex(1.0)/intermed_impedance[1];

								//Tracker
								prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
							}
							
							if (!(intermed_impedance[2].IsZero()))
							{
								//Accumulator
								shunt[2] += gld::complex(1.0)/intermed_impedance[2];

								//Tracker
								prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
							}
							
							//Current and power accumulators
							power[0] += constant_power[0];
							power[1] += constant_power[1];	
							power[2] += constant_power[2];

							current[0] += constant_current[0];
							current[1] += constant_current[1];
							current[2] += constant_current[2];

							//Power and current trackers
							prev_load_values[1][0] += constant_current[0];
							prev_load_values[1][1] += constant_current[1];
							prev_load_values[1][2] += constant_current[2];
							prev_load_values[2][0] += constant_power[0];
							prev_load_values[2][1] += constant_power[1];
							prev_load_values[2][2] += constant_power[2];

							//Combination Portion
							//Do the same for explicit delta/wye connections
							for (index_var=0; index_var<6; index_var++)
							{
								if (!intermed_impedance_dy[index_var].IsZero())
								{
									//Accumulator
									shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

									//Tracker
									prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
								}

								power_dy[index_var] += constant_power_dy[index_var];
								current_dy[index_var] += constant_current_dy[index_var];

								//Update power/current trackers
								prev_load_values_dy[1][index_var] += constant_current_dy[index_var];
								prev_load_values_dy[2][index_var] += constant_power_dy[index_var];
							}
						}//End normal operations
					}//End in-rush enabled
					else	//No in-rush, normal operations
					{
						//Apply the updates
						if (!(intermed_impedance[0].IsZero()))
						{
							//Accumulator
							shunt[0] += gld::complex(1.0)/intermed_impedance[0];

							//Tracker
							prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
						}

						if (!(intermed_impedance[1].IsZero()))
						{
							//Accumulator
							shunt[1] += gld::complex(1.0)/intermed_impedance[1];

							//Tracker
							prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
						}
						
						if (!(intermed_impedance[2].IsZero()))
						{
							//Accumulator
							shunt[2] += gld::complex(1.0)/intermed_impedance[2];

							//Tracker
							prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
						}
						
						//Current and power accumulators
						power[0] += constant_power[0];
						power[1] += constant_power[1];	
						power[2] += constant_power[2];

						current[0] += constant_current[0];
						current[1] += constant_current[1];
						current[2] += constant_current[2];

						//Power and current trackers
						prev_load_values[1][0] += constant_current[0];
						prev_load_values[1][1] += constant_current[1];
						prev_load_values[1][2] += constant_current[2];
						prev_load_values[2][0] += constant_power[0];
						prev_load_values[2][1] += constant_power[1];
						prev_load_values[2][2] += constant_power[2];

						//Combination Portion
						//Do the same for explicit delta/wye connections
						for (index_var=0; index_var<6; index_var++)
						{
							if (!intermed_impedance_dy[index_var].IsZero())
							{
								//Accumulator
								shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

								//Tracker
								prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
							}

							power_dy[index_var] += constant_power_dy[index_var];
							current_dy[index_var] += constant_current_dy[index_var];

							//Update power/current trackers
							prev_load_values_dy[1][index_var] += constant_current_dy[index_var];
							prev_load_values_dy[2][index_var] += constant_power_dy[index_var];
						}
					}//End normal operations
				}//End differently connected parent
			}//Some form of NR parent
			else	//Basically, not a NR parent
			{
				//See if in-rush is enabled - only makes sense in reliability situations
				if (enable_inrush_calculations || enable_impedance_conversion)
				{
					//See if we're a child or not
					if (NR_node_reference == -99)	//Child or other special object
					{
						node_reference_value = *NR_subnode_reference;	//Grab out parent

						//Check it, for giggles/thoroughness
						if (node_reference_value < 0)
						{
							//Get header information
							obj = OBJECTHDR(this);

							GL_THROW("node:%d -- %s tried to perform an impedance conversion with an uninitialzed child node!",obj->id, obj->name?obj->name:"unnamed");
							/*  TROUBLESHOOT
							While attempting to convert a load to a constant impedance value for in-rush modeling, a problem occurred mapping a parent node.
							Please try again.  If the error persists, please submit your code and a bug report via the ticketing system.
							*/
						}
						//Defaulted else -- it's theoretically a good value
					}//end child or other
					else //Normal node
					{
						node_reference_value = NR_node_reference;
					}

					//Reset variable
					volt_below_thresh = false;

					//Check to see if we're under the voltage pu limit or not -- any version under will trip it
					if (has_phase(PHASE_D))
					{
						voltage_base_val = nominal_voltage * sqrt(3.0);

						//Compute per-unit values
						voltage_pu_vals[0] = voltaged[0].Mag()/voltage_base_val;
						voltage_pu_vals[1] = voltaged[1].Mag()/voltage_base_val;
						voltage_pu_vals[2] = voltaged[2].Mag()/voltage_base_val;

						//Check them - Phase AB
						if (((NR_busdata[node_reference_value].phases & 0x06) == 0x06) && (voltage_pu_vals[0] < impedance_conversion_low_pu))
						{
							volt_below_thresh = true;
						}

						//Check them - Phase BC
						if (((NR_busdata[node_reference_value].phases & 0x03) == 0x03) && (voltage_pu_vals[1] < impedance_conversion_low_pu))
						{
							volt_below_thresh = true;
						}

						//Check them - Phase CA
						if (((NR_busdata[node_reference_value].phases & 0x05) == 0x05) && (voltage_pu_vals[2] < impedance_conversion_low_pu))
						{
							volt_below_thresh = true;
						}
					}
					else	//Must be Wye-connected
					{
						//Set nominal voltage
						voltage_base_val = nominal_voltage;

						//Compute per-unit values
						voltage_pu_vals[0] = voltage[0].Mag()/voltage_base_val;
						voltage_pu_vals[1] = voltage[1].Mag()/voltage_base_val;
						voltage_pu_vals[2] = voltage[2].Mag()/voltage_base_val;

						//Check them - Phase A
						if (((NR_busdata[node_reference_value].phases & 0x04) == 0x04) && (voltage_pu_vals[0] < impedance_conversion_low_pu))
						{
							volt_below_thresh = true;
						}

						//Check them - Phase B
						if (((NR_busdata[node_reference_value].phases & 0x02) == 0x02) && (voltage_pu_vals[1] < impedance_conversion_low_pu))
						{
							volt_below_thresh = true;
						}

						//Check them - Phase C
						if (((NR_busdata[node_reference_value].phases & 0x01) == 0x01) && (voltage_pu_vals[2] < impedance_conversion_low_pu))
						{
							volt_below_thresh = true;
						}
					}

					//See if tripped
					if (volt_below_thresh)
					{
						//Current and power would be zeroed here - basically, we'll ignore them later

						//Convert all loads to an impedance-equivalent
						if (has_phase(PHASE_D))	//Delta-connected
						{
							//Reset - out of paranoia
							voltage_base_val = nominal_voltage * sqrt(3.0);

							//Check phases - AB
							if ((NR_busdata[node_reference_value].phases & 0x06) == 0x06)
							{
								//Check power value
								if (!(constant_power[0].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance[0] += ~(nominal_voltage_value/constant_power[0]);
								}

								//Check current
								if (!(constant_current[0].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,PI/6);

									//Compute the value
									intermed_impedance[0] += nominal_voltage_value/constant_current[0];
								}
							}//End AB check

							//Check phases - BC
							if ((NR_busdata[node_reference_value].phases & 0x03) == 0x03)
							{
								//Check power value
								if (!(constant_power[1].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance[1] += ~(nominal_voltage_value/constant_power[1]);
								}

								//Check current
								if (!(constant_current[1].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,-1.0*PI/2.0);

									//Compute the value
									intermed_impedance[1] += nominal_voltage_value/constant_current[1];
								}
							}//End BC check

							//Check phases - CA
							if ((NR_busdata[node_reference_value].phases & 0x05) == 0x05)
							{
								//Check power value
								if (!(constant_power[2].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance[2] += ~(nominal_voltage_value/constant_power[2]);
								}

								//Check current
								if (!(constant_current[2].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,5.0*PI/6.0);

									//Compute the value
									intermed_impedance[2] += nominal_voltage_value/constant_current[2];
								}
							}//End CA check
						}//End Delta connection
						else	//Wye-connected
						{
							//Reset - out of paranoia
							voltage_base_val = nominal_voltage;

							//Check phases - A
							if ((NR_busdata[node_reference_value].phases & 0x04) == 0x04)
							{
								//Check power value
								if (!(constant_power[0].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance[0] += ~(nominal_voltage_value/constant_power[0]);
								}

								//Check current
								if (!(constant_current[0].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value = gld::complex(voltage_base_val,0.0);

									//Compute the value
									intermed_impedance[0] += nominal_voltage_value/constant_current[0];
								}
							}//End A check

							//Check phases - B
							if ((NR_busdata[node_reference_value].phases & 0x02) == 0x02)
							{
								//Check power value
								if (!(constant_power[1].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance[1] += ~(nominal_voltage_value/constant_power[1]);
								}

								//Check current
								if (!(constant_current[1].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,-2.0*PI/3.0);

									//Compute the value
									intermed_impedance[1] += nominal_voltage_value/constant_current[1];
								}
							}//End B check

							//Check phases - C
							if ((NR_busdata[node_reference_value].phases & 0x01) == 0x01)
							{
								//Check power value
								if (!(constant_power[2].IsZero()))
								{
									//Set nominal voltage - |V|^2
									nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

									//Compute the value
									intermed_impedance[2] += ~(nominal_voltage_value/constant_power[2]);
								}

								//Check current
								if (!(constant_current[2].IsZero()))
								{
									//Set nominal voltage - actual angle
									nominal_voltage_value.SetPolar(voltage_base_val,2.0*PI/3.0);

									//Compute the value
									intermed_impedance[2] += nominal_voltage_value/constant_current[2];
								}
							}//End C check
						}//End Wye connection

						//Add the values in
						if (!(intermed_impedance[0].IsZero()))
						{
							//Accumulator
							shunt[0] += gld::complex(1.0)/intermed_impedance[0];

							//Tracker
							prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
						}

						if (!(intermed_impedance[1].IsZero()))
						{
							//Accumulator
							shunt[1] += gld::complex(1.0)/intermed_impedance[1];

							//Tracker
							prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
						}
						
						if (!(intermed_impedance[2].IsZero()))
						{
							//Accumulator
							shunt[2] += gld::complex(1.0)/intermed_impedance[2];

							//Tracker
							prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
						}

						//Power and current remain zero
						
						//Combination load section

						// *********** DELTA Portions *************
						//Set new base value
						voltage_base_val = nominal_voltage * sqrt(3.0);

						//Check phases - AB
						if ((NR_busdata[node_reference_value].phases & 0x06) == 0x06)
						{
							//Check power value
							if (!(constant_power_dy[0].IsZero()))
							{
								//Set nominal voltage - |V|^2
								nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

								//Compute the value
								intermed_impedance_dy[0] += ~(nominal_voltage_value/constant_power_dy[0]);
							}

							//Check current
							if (!(constant_current_dy[0].IsZero()))
							{
								//Set nominal voltage - actual angle
								nominal_voltage_value.SetPolar(voltage_base_val,PI/6);

								//Compute the value
								intermed_impedance_dy[0] += nominal_voltage_value/constant_current_dy[0];
							}
						}//End AB check

						//Check phases - BC
						if ((NR_busdata[node_reference_value].phases & 0x03) == 0x03)
						{
							//Check power value
							if (!(constant_power_dy[1].IsZero()))
							{
								//Set nominal voltage - |V|^2
								nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

								//Compute the value
								intermed_impedance_dy[1] += ~(nominal_voltage_value/constant_power_dy[1]);
							}

							//Check current
							if (!(constant_current_dy[1].IsZero()))
							{
								//Set nominal voltage - actual angle
								nominal_voltage_value.SetPolar(voltage_base_val,-1.0*PI/2.0);

								//Compute the value
								intermed_impedance_dy[1] += nominal_voltage_value/constant_current_dy[1];
							}
						}//End BC check

						//Check phases - CA
						if ((NR_busdata[node_reference_value].phases & 0x05) == 0x05)
						{
							//Check power value
							if (!(constant_power_dy[2].IsZero()))
							{
								//Set nominal voltage - |V|^2
								nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

								//Compute the value
								intermed_impedance_dy[2] += ~(nominal_voltage_value/constant_power_dy[2]);
							}

							//Check current
							if (!(constant_current_dy[2].IsZero()))
							{
								//Set nominal voltage - actual angle
								nominal_voltage_value.SetPolar(voltage_base_val,5.0*PI/6.0);

								//Compute the value
								intermed_impedance_dy[2] += nominal_voltage_value/constant_current_dy[2];
							}
						}//End CA check

						// ********** WYE portions *****************
						//Set new base value
						voltage_base_val = nominal_voltage;

						//Check phases - A
						if ((NR_busdata[node_reference_value].phases & 0x04) == 0x04)
						{
							//Check power value
							if (!(constant_power_dy[3].IsZero()))
							{
								//Set nominal voltage - |V|^2
								nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

								//Compute the value
								intermed_impedance_dy[3] += ~(nominal_voltage_value/constant_power_dy[3]);
							}

							//Check current
							if (!(constant_current_dy[3].IsZero()))
							{
								//Set nominal voltage - actual angle
								nominal_voltage_value = gld::complex(voltage_base_val,0.0);

								//Compute the value
								intermed_impedance_dy[3] += nominal_voltage_value/constant_current_dy[3];
							}
						}//End A check

						//Check phases - B
						if ((NR_busdata[node_reference_value].phases & 0x02) == 0x02)
						{
							//Check power value
							if (!(constant_power_dy[4].IsZero()))
							{
								//Set nominal voltage - |V|^2
								nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

								//Compute the value
								intermed_impedance_dy[4] += ~(nominal_voltage_value/constant_power_dy[4]);
							}

							//Check current
							if (!(constant_current_dy[4].IsZero()))
							{
								//Set nominal voltage - actual angle
								nominal_voltage_value.SetPolar(voltage_base_val,-2.0*PI/3.0);

								//Compute the value
								intermed_impedance_dy[4] += nominal_voltage_value/constant_current_dy[4];
							}
						}//End B check

						//Check phases - C
						if ((NR_busdata[node_reference_value].phases & 0x01) == 0x01)
						{
							//Check power value
							if (!(constant_power_dy[5].IsZero()))
							{
								//Set nominal voltage - |V|^2
								nominal_voltage_value = gld::complex(voltage_base_val*voltage_base_val,0);

								//Compute the value
								intermed_impedance_dy[5] += ~(nominal_voltage_value/constant_power_dy[5]);
							}

							//Check current
							if (!(constant_current_dy[5].IsZero()))
							{
								//Set nominal voltage - actual angle
								nominal_voltage_value.SetPolar(voltage_base_val,2.0*PI/3.0);

								//Compute the value
								intermed_impedance_dy[5] += nominal_voltage_value/constant_current_dy[5];
							}
						}//End C check

						//Do the updated portion
						for (index_var=0; index_var<6; index_var++)
						{
							if (!intermed_impedance_dy[index_var].IsZero())
							{
								//Accumulator
								shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

								//Tracker
								prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
							}
						}
					}//End below thresh - convert
					else	//Must be above, be "normal"
					{
						//Add the values in
						if (!(intermed_impedance[0].IsZero()))
						{
							//Accumulator
							shunt[0] += gld::complex(1.0)/intermed_impedance[0];

							//Tracker
							prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
						}

						if (!(intermed_impedance[1].IsZero()))
						{
							//Accumulator
							shunt[1] += gld::complex(1.0)/intermed_impedance[1];

							//Tracker
							prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
						}
						
						if (!(intermed_impedance[2].IsZero()))
						{
							//Accumulator
							shunt[2] += gld::complex(1.0)/intermed_impedance[2];

							//Tracker
							prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
						}
						
						//Current and power accumulators
						power[0] += constant_power[0];
						power[1] += constant_power[1];	
						power[2] += constant_power[2];

						current[0] += constant_current[0];
						current[1] += constant_current[1];
						current[2] += constant_current[2];

						//Power and current trackers
						prev_load_values[1][0] += constant_current[0];
						prev_load_values[1][1] += constant_current[1];
						prev_load_values[1][2] += constant_current[2];
						prev_load_values[2][0] += constant_power[0];
						prev_load_values[2][1] += constant_power[1];
						prev_load_values[2][2] += constant_power[2];

						//Do the same for explicit delta/wye connections
						for (index_var=0; index_var<6; index_var++)
						{
							if (!intermed_impedance_dy[index_var].IsZero())
							{
								//Accumulator
								shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

								//Tracker
								prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
							}

							power_dy[index_var] += constant_power_dy[index_var];
							current_dy[index_var] += constant_current_dy[index_var];

							//Update power/current trackers
							prev_load_values_dy[1][index_var] += constant_current_dy[index_var];
							prev_load_values_dy[2][index_var] += constant_power_dy[index_var];
						}
					}//End above voltage threshold
				}//End in-rush enabled
				else	//Not in-rush, just normal operations
				{
					//Add the values in
					if (!(intermed_impedance[0].IsZero()))
					{
						//Accumulator
						shunt[0] += gld::complex(1.0)/intermed_impedance[0];

						//Tracker
						prev_load_values[0][0] += gld::complex(1.0,0.0)/intermed_impedance[0];
					}

					if (!(intermed_impedance[1].IsZero()))
					{
						//Accumulator
						shunt[1] += gld::complex(1.0)/intermed_impedance[1];

						//Tracker
						prev_load_values[0][1] += gld::complex(1.0,0.0)/intermed_impedance[1];
					}
					
					if (!(intermed_impedance[2].IsZero()))
					{
						//Accumulator
						shunt[2] += gld::complex(1.0)/intermed_impedance[2];

						//Tracker
						prev_load_values[0][2] += gld::complex(1.0,0.0)/intermed_impedance[2];
					}
					
					//Current and power accumulators
					power[0] += constant_power[0];
					power[1] += constant_power[1];	
					power[2] += constant_power[2];

					current[0] += constant_current[0];
					current[1] += constant_current[1];
					current[2] += constant_current[2];

					//Power and current trackers
					prev_load_values[1][0] += constant_current[0];
					prev_load_values[1][1] += constant_current[1];
					prev_load_values[1][2] += constant_current[2];
					prev_load_values[2][0] += constant_power[0];
					prev_load_values[2][1] += constant_power[1];
					prev_load_values[2][2] += constant_power[2];

					//Do the same for explicit delta/wye connections
					for (index_var=0; index_var<6; index_var++)
					{
						if (!intermed_impedance_dy[index_var].IsZero())
						{
							//Accumulator
							shunt_dy[index_var] += gld::complex(1.0)/intermed_impedance_dy[index_var];

							//Tracker
							prev_load_values_dy[0][index_var] += gld::complex(1.0,0.0)/intermed_impedance_dy[index_var];
						}

						power_dy[index_var] += constant_power_dy[index_var];
						current_dy[index_var] += constant_current_dy[index_var];

						//Update power/current trackers
						prev_load_values_dy[1][index_var] += constant_current_dy[index_var];
						prev_load_values_dy[2][index_var] += constant_power_dy[index_var];
					}
				}//End normal operations
			}//End not a parented NR
		}//Handle all three
		//Default else - everything would stay zero
	}//End reliability mode

	//In-rush update information - incorporate the latest "impedance" values
	//Put at bottom to ensure it gets the "converted impedance final result"
	//Deltamode catch and check
	if (enable_inrush_calculations)
	{
		if (deltatimestep_running > 0)	//In deltamode
		{
			//Get current deltamode timestep
			curr_delta_time = gl_globaldeltaclock;

			//Set flag
			require_inrush_update = true;

			//See if we're a different timestep (update is in node.cpp postsync function)
			if (curr_delta_time != prev_delta_time)
			{
				//Loop through the phases for the updates
				for (index_var=0; index_var<3; index_var++)
				{
					//Start, assuming "normal"
					transf_from_stdy_state = false;

					//See which mode we should go into
					if (prev_delta_time < 0)	//Came from steady state, let's see our status
					{
						//Check phase voltage value - if it is non-zero, assume we were running
						//Adjust for childed nodes
						if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
						{
							if (NR_busdata[NR_node_reference].V[index_var].Mag() > 0.0)
							{
								transf_from_stdy_state = true;
							}
							//Default else, leave false
						}
						else	//Childed - get parent values
						{
							if (NR_busdata[*NR_subnode_reference].V[index_var].Mag() > 0.0)
							{
								transf_from_stdy_state = true;
							}
							//Default else, leave false
						}
					}
					//Default else, already were running, so unneeded

					//See if we came from steady state
					if (transf_from_stdy_state)
					{
						//Check and see what type of load this is -- Phase A
						//Use intermediate variable to only get our load parts (not any underlying contributions)
						if (prev_load_values[0][index_var].Im()>0.0)	//Capacitve
						{
							//Zero all inductive terms, just because
							LoadHistTermL[index_var] = gld::complex(0.0,0.0);
							LoadHistTermL[index_var+3] = gld::complex(0.0,0.0);

							//Zero the two inductive constant terms
							ahrlloadstore[index_var] = gld::complex(0.0,0.0);
							bhrlloadstore[index_var] = gld::complex(0.0,0.0);

							//Update capacitive history terms
							LoadHistTermC[index_var+3] = LoadHistTermC[index_var];

							//Code from below -- transition in needs an update first
							if (inrush_int_method_capacitance == IRM_TRAPEZOIDAL)
							{
								//Extract the imaginary part (should be only part) and de-phasor it - Yshunt/(2*pi*f)*2/dt
								workingvalue = prev_load_values[0][index_var].Im() / (PI * current_frequency * deltatimestep_running);

								//Create chrcstore while we're in here
								chrcloadstore[index_var] = 2.0 * workingvalue;

								//Calculate the updated history terms - hrcf = chrc*vfromprev/2.0
								//Do a parent check
								if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
								{
									LoadHistTermC[index_var] = NR_busdata[NR_node_reference].V[index_var] * chrcloadstore[index_var] / gld::complex(2.0,0.0);
								}
								else	//Childed - get parent values
								{
									LoadHistTermC[index_var] = NR_busdata[*NR_subnode_reference].V[index_var] * chrcloadstore[index_var] / gld::complex(2.0,0.0);
								}
							}
							else if (inrush_int_method_capacitance == IRM_BACKEULER)
							{
								//Extract the imaginary part (should be only part) and de-phasor it - Yshunt/(2*pi*f)/dt
								workingvalue = prev_load_values[0][index_var].Im() / (2.0 * PI * current_frequency * deltatimestep_running);

								//Create chrcstore while we're in here
								chrcloadstore[index_var] = workingvalue;

								//Calculate the updated history terms - hrcf = chrc*vfromprev
								//Do a parent check
								if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
								{
									LoadHistTermC[index_var] = NR_busdata[NR_node_reference].V[index_var] * chrcloadstore[index_var];
								}
								else	//Childed - get parent values
								{
									LoadHistTermC[index_var] = NR_busdata[*NR_subnode_reference].V[index_var] * chrcloadstore[index_var];
								}
							}
							//future else

							//End of copy from below

						}
						else if (prev_load_values[0][index_var].Im()<0.0) //Inductive - again use tracking variable
						{
							//Zero all capacitive terms, just because
							LoadHistTermC[index_var] = gld::complex(0.0,0.0);
							LoadHistTermC[index_var+3] = gld::complex(0.0,0.0);

							//Zero the capacitive term
							chrcloadstore[index_var] = gld::complex(0.0,0.0);

							//Update inductive history terms
							LoadHistTermL[index_var+3] = LoadHistTermL[index_var];

							//Copied from below - update
							//Form the equivalent impedance value
							working_impedance_value = gld::complex(1.0,0.0) / prev_load_values[0][index_var];

							if (inrush_int_method_inductance == IRM_TRAPEZOIDAL)
							{
								//Extract the imaginary part (should be only part) and de-phasor it - Yimped/(2*pi*f)*2/dt
								workingvalue = working_impedance_value.Im() / (PI * current_frequency * deltatimestep_running);

								//Put into the other working matrix (zh)
								working_data_value = working_impedance_value - gld::complex(workingvalue,0.0);
							}
							else if (inrush_int_method_inductance == IRM_BACKEULER)
							{
								//Extract the imaginary part (should be only part) and de-phasor it - Yimped/(2*pi*f)/dt
								workingvalue = working_impedance_value.Im() / (2.0 * PI * current_frequency * deltatimestep_running);

								//Put into the other working matrix (zh)
								working_data_value = gld::complex(-1.0 * workingvalue,0.0);
							}
							//Default else -- should never get here

							//Put back into "real" impedance value to make Y (Znew)
							working_impedance_value += gld::complex(workingvalue,0.0);

							//Make it an admittance again, for the update - Y
							if (!working_impedance_value.IsZero())
							{
								working_admittance_value = gld::complex(1.0,0.0) / working_impedance_value;
							}
							else
							{
								working_admittance_value = gld::complex(0.0,0.0);
							}

							//Form the bhrl term - Y*Zh = bhrl
							bhrlloadstore[index_var] = working_admittance_value * working_data_value;

							if (inrush_int_method_inductance == IRM_TRAPEZOIDAL)
							{
								//Compute the ahrl term - Y(Zh*Y - I)
								ahrlloadstore[index_var] = working_admittance_value*(working_data_value * working_admittance_value - gld::complex(1.0,0.0));
							}
							else if (inrush_int_method_inductance == IRM_BACKEULER)
							{
								//Compute the ahrl term - Y*Zh*Y
								ahrlloadstore[index_var] = working_admittance_value * working_data_value * working_admittance_value;
							}
							//else should never happen

							//End copy of above

							//Calculate the updated history terms - hrl = inv((1+bhrl))*ahrl*vprev
							//Cheating references to voltages
							//Do a parent check
							if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
							{
								LoadHistTermL[index_var] = NR_busdata[NR_node_reference].V[index_var] * ahrlloadstore[index_var] /
														   (gld::complex(1.0,0.0) + bhrlloadstore[index_var]);
							}
							else	//Childed, steal our parent's values
							{
								LoadHistTermL[index_var] = NR_busdata[*NR_subnode_reference].V[index_var] * ahrlloadstore[index_var] /
														   (gld::complex(1.0,0.0) + bhrlloadstore[index_var]);
							}
						}
						else	//Must be zero -- purely resistive, or something
						{
							//Zero both sets of terms
							ahrlloadstore[index_var] = gld::complex(0.0,0.0);
							bhrlloadstore[index_var] = gld::complex(0.0,0.0);

							chrcloadstore[index_var] = gld::complex(0.0,0.0);

							//Zero everything on principle
							LoadHistTermL[index_var] = gld::complex(0.0,0.0);
							LoadHistTermL[index_var+3] = gld::complex(0.0,0.0);

							LoadHistTermC[index_var] = gld::complex(0.0,0.0);
							LoadHistTermC[index_var+3] = gld::complex(0.0,0.0);
						}
					}//End transitioned from steady state
					else	//Normal update
					{
						//Check and see what type of load this is -- Phase A
						if (prev_load_values[0][index_var].Im()>0.0)	//Capacitve - use intermediate variable
						{
							//Zero all inductive terms, just because
							LoadHistTermL[index_var] = gld::complex(0.0,0.0);
							LoadHistTermL[index_var+3] = gld::complex(0.0,0.0);

							//Update capacitive history terms
							LoadHistTermC[index_var+3] = LoadHistTermC[index_var];

							//Figure out which method we're using
							if (inrush_int_method_capacitance == IRM_TRAPEZOIDAL)
							{
								//Calculate the updated history terms - hrcf = chrc*vfromprev - hrcfprev
								//Do a parent check
								if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
								{
									LoadHistTermC[index_var] = NR_busdata[NR_node_reference].V[index_var] * chrcloadstore[index_var] -
															LoadHistTermC[index_var+3];
								}
								else	//Childed - get parent values
								{
									LoadHistTermC[index_var] = NR_busdata[*NR_subnode_reference].V[index_var] * chrcloadstore[index_var] -
															LoadHistTermC[index_var+3];
								}
							}
							else if (inrush_int_method_capacitance == IRM_BACKEULER)
							{
								//Calculate the updated history terms - hrcf = chrc*vfromprev
								//Do a parent check
								if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
								{
									LoadHistTermC[index_var] = NR_busdata[NR_node_reference].V[index_var] * chrcloadstore[index_var];
								}
								else	//Childed - get parent values
								{
									LoadHistTermC[index_var] = NR_busdata[*NR_subnode_reference].V[index_var] * chrcloadstore[index_var];
								}
							}
							//Default else -- some other method
						}
						else if (prev_load_values[0][index_var].Im()<0.0) //Inductive - use intermediate variable
						{
							//Zero all capacitive terms, just because
							LoadHistTermC[index_var] = gld::complex(0.0,0.0);
							LoadHistTermC[index_var+3] = gld::complex(0.0,0.0);

							//Update inductive history terms
							LoadHistTermL[index_var+3] = LoadHistTermL[index_var];

							//Calculate the updated history terms - hrl = ahrl*vprev-bhrl*hrlprev
							//Cheating references to voltages
							//Do a parent check
							if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
							{
								LoadHistTermL[index_var] = NR_busdata[NR_node_reference].V[index_var] * ahrlloadstore[index_var] -
														   bhrlloadstore[index_var] * LoadHistTermL[index_var+3];
							}
							else	//Childed, steal our parent's values
							{
								LoadHistTermL[index_var] = NR_busdata[*NR_subnode_reference].V[index_var] * ahrlloadstore[index_var] -
														   bhrlloadstore[index_var] * LoadHistTermL[index_var+3];
							}
						}
						else	//Must be zero -- purely resistive, or something
						{
							//Zero everything on principle
							LoadHistTermL[index_var] = gld::complex(0.0,0.0);
							LoadHistTermL[index_var+3] = gld::complex(0.0,0.0);

							LoadHistTermC[index_var] = gld::complex(0.0,0.0);
							LoadHistTermC[index_var+3] = gld::complex(0.0,0.0);
						}
					}//End normal update (deltamode running or we started "off"

					//See if we're a parented load or not
					if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
					{
						//Compute the values and post them to our bus term
						NR_busdata[NR_node_reference].BusHistTerm[index_var] += LoadHistTermL[index_var] + LoadHistTermC[index_var];
					}
					else	//It is a child - look at parent
					{
						//Compute the values and post them to our bus term
						NR_busdata[*NR_subnode_reference].BusHistTerm[index_var] += LoadHistTermL[index_var] + LoadHistTermC[index_var];
					}
				}//End Phase loop
			}//End new delta timestep
			//Defaulted else -- not a new time, so don't update it
		}//End deltamode running
		else	//Inrush, but not delta mode
		{
			//******************* TODO -- See if this can basically be incorporated into postupdate, since it should only need to be done once ********************/
			//Deflag
			require_inrush_update = false;

			//Zero everything on principle
			LoadHistTermL[0] = gld::complex(0.0,0.0);
			LoadHistTermL[1] = gld::complex(0.0,0.0);
			LoadHistTermL[2] = gld::complex(0.0,0.0);
			LoadHistTermL[3] = gld::complex(0.0,0.0);
			LoadHistTermL[4] = gld::complex(0.0,0.0);
			LoadHistTermL[5] = gld::complex(0.0,0.0);

			LoadHistTermC[0] = gld::complex(0.0,0.0);
			LoadHistTermC[1] = gld::complex(0.0,0.0);
			LoadHistTermC[2] = gld::complex(0.0,0.0);
			LoadHistTermC[3] = gld::complex(0.0,0.0);
			LoadHistTermC[4] = gld::complex(0.0,0.0);
			LoadHistTermC[5] = gld::complex(0.0,0.0);

			//Zero our full load component too
			//See if we're a parented load or not -- zero us as well (assumes nothing is in delta)
			if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
			{
				//Compute the values and post them to our bus term
				NR_busdata[NR_node_reference].BusHistTerm[0] = gld::complex(0.0,0.0);
				NR_busdata[NR_node_reference].BusHistTerm[1] = gld::complex(0.0,0.0);
				NR_busdata[NR_node_reference].BusHistTerm[2] = gld::complex(0.0,0.0);

				//@FPIM - see if this needs revisiting - may not even need to be zeroed for TCIM
				if (NR_solver_algorithm == NRM_TCIM)
				{
					//Add this into the main admittance matrix (handled directly)
					NR_busdata[NR_node_reference].full_Y_load[0] = gld::complex(0.0,0.0);
					NR_busdata[NR_node_reference].full_Y_load[4] = gld::complex(0.0,0.0);
					NR_busdata[NR_node_reference].full_Y_load[8] = gld::complex(0.0,0.0);
				}
			}
			else	//It is a child - look at parent
			{
				//Compute the values and post them to our bus term
				NR_busdata[*NR_subnode_reference].BusHistTerm[0] = gld::complex(0.0,0.0);
				NR_busdata[*NR_subnode_reference].BusHistTerm[1] = gld::complex(0.0,0.0);
				NR_busdata[*NR_subnode_reference].BusHistTerm[2] = gld::complex(0.0,0.0);

				//@FPIM - see if this needs revisiting - may not even need to be zeroed for TCIM
				if (NR_solver_algorithm == NRM_TCIM)
				{
					NR_busdata[*NR_subnode_reference].full_Y_load[0] = gld::complex(0.0,0.0);
					NR_busdata[*NR_subnode_reference].full_Y_load[4] = gld::complex(0.0,0.0);
					NR_busdata[*NR_subnode_reference].full_Y_load[8] = gld::complex(0.0,0.0);
				}
			}

			//Zero previous shunt values, just because
			prev_shunt[0] = gld::complex(0.0,0.0);
			prev_shunt[1] = gld::complex(0.0,0.0);
			prev_shunt[2] = gld::complex(0.0,0.0);
		}
	}
	else	//Not in in-rush or deltamode - flag to not
	{
		require_inrush_update = false;
	}

	//Update the impedance terms
	if (require_inrush_update)
	{
		//Loop the whole set - all phases
		for (index_var=0; index_var<3; index_var++)
		{
			//Figure out what type of load it is
			if (prev_load_values[0][index_var].Im()>0.0)	//Capacitve - use intermediate variable
			{
				//Zero the two inductive terms
				ahrlloadstore[index_var] = gld::complex(0.0,0.0);
				bhrlloadstore[index_var] = gld::complex(0.0,0.0);

				if (inrush_int_method_capacitance == IRM_TRAPEZOIDAL)
				{
					//Extract the imaginary part (should be only part) and de-phasor it - Yshunt/(2*pi*f)*2/dt
					workingvalue = prev_load_values[0][index_var].Im() / (PI * current_frequency * deltatimestep_running);

					//Create chrcstore while we're in here
					chrcloadstore[index_var] = 2.0 * workingvalue;
				}
				else if (inrush_int_method_capacitance == IRM_BACKEULER)
				{
					//Extract the imaginary part (should be only part) and de-phasor it - Yshunt/(2*pi*f)/dt
					workingvalue = prev_load_values[0][index_var].Im() / (2.0 * PI * current_frequency * deltatimestep_running);

					//Create chrcstore while we're in here
					chrcloadstore[index_var] = workingvalue;
				}
				//Future else

				//Put into the "shunt" admittance value
				shunt[index_var] += gld::complex(workingvalue,0.0);

				//Update the tracker
				prev_load_values[0][index_var] += gld::complex(workingvalue,0.0);
			}//End capacitve term update
			else if (prev_load_values[0][index_var].Im()<0.0) //Inductive
			{
				//Zero the capacitive term
				chrcloadstore[index_var] = gld::complex(0.0,0.0);

				//Form the equivalent impedance value
				working_impedance_value = gld::complex(1.0,0.0) / prev_load_values[0][index_var];

				if (inrush_int_method_inductance == IRM_TRAPEZOIDAL)
				{
					//Extract the imaginary part (should be only part) and de-phasor it - Yimped/(2*pi*f)*2/dt
					workingvalue = working_impedance_value.Im() / (PI * current_frequency * deltatimestep_running);

					//Put into the other working matrix (zh)
					working_data_value = working_impedance_value - gld::complex(workingvalue,0.0);
				}
				else if (inrush_int_method_inductance == IRM_BACKEULER)
				{
					//Extract the imaginary part (should be only part) and de-phasor it - Yimped/(2*pi*f)/dt
					workingvalue = working_impedance_value.Im() / (2.0 * PI * current_frequency * deltatimestep_running);

					//Put into the other working matrix (zh)
					working_data_value = gld::complex(-1.0 * workingvalue,0.0);
				}
				//future ones

				//Put back into "real" impedance value to make Y (Znew)
				working_impedance_value += gld::complex(workingvalue,0.0);

				//Make it an admittance again, for the update - Y
				if (!working_impedance_value.IsZero())
				{
					working_admittance_value = gld::complex(1.0,0.0) / working_impedance_value;
				}
				else
				{
					working_admittance_value = gld::complex(0.0,0.0);
				}

				//Form the bhrl term - Y*Zh = bhrl
				bhrlloadstore[index_var] = working_admittance_value * working_data_value;

				if (inrush_int_method_inductance == IRM_TRAPEZOIDAL)
				{
					//Compute the ahrl term - Y(Zh*Y - I)
					ahrlloadstore[index_var] = working_admittance_value*(working_data_value * working_admittance_value - gld::complex(1.0,0.0));
				}
				else if (inrush_int_method_inductance == IRM_BACKEULER)
				{
					//Compute the ahrl term - Y*Zh*Y
					ahrlloadstore[index_var] = working_admittance_value * working_data_value * working_admittance_value;
				}
				//Future else

				//Make sure we store the "new Y" so things get updated right - remove partial contribution as well
				shunt[index_var] += working_admittance_value - prev_load_values[0][index_var];

				//Update the tracker - replacement value for above (was an equal before, so it overrides)
				prev_load_values[0][index_var] = working_admittance_value;
			}//end inductive term update
			else	//Must be zero -- purely resistive, or something
			{
				//Zero both sets of terms
				ahrlloadstore[index_var] = gld::complex(0.0,0.0);
				bhrlloadstore[index_var] = gld::complex(0.0,0.0);

				chrcloadstore[index_var] = gld::complex(0.0,0.0);
			}//End something else term update

			//FPI already dumps straight ot full_Y_load, so let it handle that
			if (NR_solver_algorithm == NRM_TCIM)
			{
				//See if we're a parented load or not, to make sure we update right
				if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
				{
					//Add this into the main admittance matrix (handled directly)
					NR_busdata[NR_node_reference].full_Y_load[(index_var*3+index_var)] += shunt[index_var]-prev_shunt[index_var];
				}
				else	//It is a child - look at parent
				{
					//Add this into the main admittance matrix (handled directly)
					NR_busdata[*NR_subnode_reference].full_Y_load[(index_var*3+index_var)] += shunt[index_var]-prev_shunt[index_var];
				}

				//Update tracker
				prev_shunt[index_var] = shunt[index_var];

				//Remove our "tracked" contribution
				shunt[index_var] -= prev_load_values[0][index_var];
				
				//Zero the tracker
				prev_load_values[0][index_var] = gld::complex(0.0,0.0);
			}
			else
			{
				//Update tracker
				prev_load_values[0][index_var] = shunt[index_var];
			}
			//Default else - FPI
		}//End phase looping for in-rush terms

		//TODO:
		/**************************************************** Combination loads still need to be done *********************/
		//Good chance those will require full 3x3 implementations -- may end up being a 4.0 implementation
		//

	}//End in-rush term updat needed
	//Default else -- no update needed
}

//Function to appropriately zero load - make sure we don't get too heavy handed
void load::load_delete_update_fxn(void)
{
	int index_var, extract_node_ref;

	//If FPI - remove the impedance portions from the tracked matrix
	if (NR_solver_algorithm == NRM_FPI)
	{
		//Determine our reference
		//See if we're a parented load or not
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
		{
			extract_node_ref = NR_node_reference;
		}
		else	//It is a child - look at parent
		{
			extract_node_ref = *NR_subnode_reference;
		}

		//Remove the explicit Delta-Wye componenents
		NR_busdata[extract_node_ref].full_Y_load[0] -= prev_load_values_dy[0][0] + prev_load_values_dy[0][2] + prev_load_values_dy[0][3];
		NR_busdata[extract_node_ref].full_Y_load[1] += prev_load_values_dy[0][0];
		NR_busdata[extract_node_ref].full_Y_load[2] += prev_load_values_dy[0][2];
		NR_busdata[extract_node_ref].full_Y_load[3] += prev_load_values_dy[0][0];
		NR_busdata[extract_node_ref].full_Y_load[4] -= prev_load_values_dy[0][1] + prev_load_values_dy[0][0] + prev_load_values_dy[0][4];
		NR_busdata[extract_node_ref].full_Y_load[5] += prev_load_values_dy[0][1];
		NR_busdata[extract_node_ref].full_Y_load[6] += prev_load_values_dy[0][2];
		NR_busdata[extract_node_ref].full_Y_load[7] += prev_load_values_dy[0][1];
		NR_busdata[extract_node_ref].full_Y_load[8] -= prev_load_values_dy[0][2] + prev_load_values_dy[0][1] + prev_load_values_dy[0][5];

		//Do the "traditional" load removal
		if (has_phase(PHASE_D))
		{
			//Update the matrix
			NR_busdata[extract_node_ref].full_Y_load[0] -= prev_load_values[0][0] + prev_load_values[0][2];
			NR_busdata[extract_node_ref].full_Y_load[1] += prev_load_values[0][0];
			NR_busdata[extract_node_ref].full_Y_load[2] += prev_load_values[0][2];
			NR_busdata[extract_node_ref].full_Y_load[3] += prev_load_values[0][0];
			NR_busdata[extract_node_ref].full_Y_load[4] -= prev_load_values[0][1] + prev_load_values[0][0];
			NR_busdata[extract_node_ref].full_Y_load[5] += prev_load_values[0][1];
			NR_busdata[extract_node_ref].full_Y_load[6] += prev_load_values[0][2];
			NR_busdata[extract_node_ref].full_Y_load[7] += prev_load_values[0][1];
			NR_busdata[extract_node_ref].full_Y_load[8] -= prev_load_values[0][2] + prev_load_values[0][1];
		}
		else
		{
			//Update the matrix
			NR_busdata[extract_node_ref].full_Y_load[0] -= prev_load_values[0][0];
			NR_busdata[extract_node_ref].full_Y_load[4] -= prev_load_values[0][1];
			NR_busdata[extract_node_ref].full_Y_load[8] -= prev_load_values[0][2];
		}

		//Flag an update, because we just changed things
		NR_FPI_imp_load_change = true;
	}//End FPI Impedance updates

	//Loop and clear
	for (index_var=0; index_var<3; index_var++)
	{
		//Remove contributions
		shunt[index_var] -= prev_load_values[0][index_var];
		current[index_var] -= prev_load_values[1][index_var];
		power[index_var] -= prev_load_values[2][index_var];

		//Clear the trackers
		prev_load_values[0][index_var] = gld::complex(0.0,0.0);
		prev_load_values[1][index_var] = gld::complex(0.0,0.0);
		prev_load_values[2][index_var] = gld::complex(0.0,0.0);
	}

	//Now do again for the explicit connections
	for (index_var=0; index_var<6; index_var++)
	{
		//Remove contributions
		shunt_dy[index_var] -= prev_load_values_dy[0][index_var];
		current_dy[index_var] -= prev_load_values_dy[1][index_var];
		power_dy[index_var] -= prev_load_values_dy[2][index_var];

		//Clear the trackers
		prev_load_values_dy[0][index_var] = gld::complex(0.0,0.0);
		prev_load_values_dy[1][index_var] = gld::complex(0.0,0.0);
		prev_load_values_dy[2][index_var] = gld::complex(0.0,0.0);
	}
}

//Notify function
//NOTE: The NR-based notify stuff may no longer be needed after NR is "flattened", since it will
//      effectively be like FBS at that point.
//Second NOTE: This function doesn't incorporate the newer explicit delta/wye connected load structure (TODO)
int load::notify(int update_mode, PROPERTY *prop, char *value)
{
	gld::complex diff_val;
	double power_tolerance;

	if (solver_method == SM_NR)
	{
		//See if it was a power update - if it was populated
		if (prev_power_value != nullptr)
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
		//Call presync-equivalent items
		NR_node_presync_fxn(0);

		//Functionalized load updates - so deltamode can parttake
		load_update_fxn();

		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call

	}//End Before NR solver (or inclusive)
	else	//After the call
	{
		//Perform postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

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

		//Postsync load items - measurement population
		measured_voltage_A.SetPolar(voltageA.Mag(),voltageA.Arg());  //Used for testing and xml output
		measured_voltage_B.SetPolar(voltageB.Mag(),voltageB.Arg());
		measured_voltage_C.SetPolar(voltageC.Mag(),voltageC.Arg());
		measured_voltage_AB = measured_voltage_A-measured_voltage_B;
		measured_voltage_BC = measured_voltage_B-measured_voltage_C;
		measured_voltage_CA = measured_voltage_C-measured_voltage_A;

		measured_power[0] = voltageA*(~current_inj[0]);
		measured_power[1] = voltageB*(~current_inj[1]);
		measured_power[2] = voltageC*(~current_inj[2]);
		measured_total_power = measured_power[0] + measured_power[1] + measured_power[2];

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
	}//End "After NR solver" branch
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
		if (*obj!=nullptr)
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

//Exposed function to do load update - primarily to get impedance conversion into solver_nr directly
EXPORT STATUS update_load_values(OBJECT *obj)
{
	load *my = OBJECTDATA(obj,load);

	//Call the update
	my->load_update_fxn();

	//Return us - always succeed, for now
	return SUCCESS;
}

//KML Export
EXPORT int load_kmldata(OBJECT *obj,int (*stream)(const char*,...))
{
	load *n = OBJECTDATA(obj, load);
	int rv = 1;

	rv = n->kmldata(stream);

	return rv;
}

int load::kmldata(int (*stream)(const char*,...))
{
	int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};

	// impedance demand
	stream("<TR><TH ALIGN=LEFT>Z</TH>");
	for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
	{
		if ( phase[i] )
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kVA</TD>", constant_impedance[i].Mag()/1000);
		else
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
	}
	stream("</TR>\n");

	// current demand
	stream("<TR><TH ALIGN=LEFT>I</TH>");
	for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
	{
		if ( phase[i] )
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kVA</TD>", constant_current[i].Mag()/1000);
		else
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
	}
	stream("</TR>\n");

	// power demand
	stream("<TR><TH ALIGN=LEFT>P</TH>");
	for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
	{
		if ( phase[i] )
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kVA</TD>", constant_power[i].Mag()/1000);
		else
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
	}
	stream("</TR>\n");

	return 0;
}

/**@}*/
