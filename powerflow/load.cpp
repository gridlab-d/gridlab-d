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
		
		oclass = gl_register_class(mod,"load",sizeof(load),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class load";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_enumeration, "load_class", PADDR(load_class),
				PT_KEYWORD, "U", LC_UNKNOWN,
				PT_KEYWORD, "R", LC_RESIDENTIAL,
				PT_KEYWORD, "C", LC_COMMERCIAL,
				PT_KEYWORD, "I", LC_INDUSTRIAL,
				PT_KEYWORD, "A", LC_AGRICULTURAL,
			PT_complex, "constant_power_A[VA]", PADDR(constant_power[0]),
			PT_complex, "constant_power_B[VA]", PADDR(constant_power[1]),
			PT_complex, "constant_power_C[VA]", PADDR(constant_power[2]),
			PT_double, "constant_power_A_real[W]", PADDR(constant_power[0].Re()),
			PT_double, "constant_power_B_real[W]", PADDR(constant_power[1].Re()),
			PT_double, "constant_power_C_real[W]", PADDR(constant_power[2].Re()),
			PT_double, "constant_power_A_reac[VAr]", PADDR(constant_power[0].Im()),
			PT_double, "constant_power_B_reac[VAr]", PADDR(constant_power[1].Im()),
			PT_double, "constant_power_C_reac[VAr]", PADDR(constant_power[2].Im()),
			PT_complex, "constant_current_A[A]", PADDR(constant_current[0]),
			PT_complex, "constant_current_B[A]", PADDR(constant_current[1]),
			PT_complex, "constant_current_C[A]", PADDR(constant_current[2]),
			PT_double, "constant_current_A_real[A]", PADDR(constant_current[0].Re()),
			PT_double, "constant_current_B_real[A]", PADDR(constant_current[1].Re()),
			PT_double, "constant_current_C_real[A]", PADDR(constant_current[2].Re()),
			PT_double, "constant_current_A_reac[A]", PADDR(constant_current[0].Im()),
			PT_double, "constant_current_B_reac[A]", PADDR(constant_current[1].Im()),
			PT_double, "constant_current_C_reac[A]", PADDR(constant_current[2].Im()),
			PT_complex, "constant_impedance_A[Ohm]", PADDR(constant_impedance[0]),
			PT_complex, "constant_impedance_B[Ohm]", PADDR(constant_impedance[1]),
			PT_complex, "constant_impedance_C[Ohm]", PADDR(constant_impedance[2]),
			PT_double, "constant_impedance_A_real[Ohm]", PADDR(constant_impedance[0].Re()),
			PT_double, "constant_impedance_B_real[Ohm]", PADDR(constant_impedance[1].Re()),
			PT_double, "constant_impedance_C_real[Ohm]", PADDR(constant_impedance[2].Re()),
			PT_double, "constant_impedance_A_reac[Ohm]", PADDR(constant_impedance[0].Im()),
			PT_double, "constant_impedance_B_reac[Ohm]", PADDR(constant_impedance[1].Im()),
			PT_double, "constant_impedance_C_reac[Ohm]", PADDR(constant_impedance[2].Im()),
			PT_complex,	"measured_voltage_A",PADDR(measured_voltage_A),
			PT_complex,	"measured_voltage_B",PADDR(measured_voltage_B),
			PT_complex,	"measured_voltage_C",PADDR(measured_voltage_C),
			PT_complex,	"measured_voltage_AB",PADDR(measured_voltage_AB),
			PT_complex,	"measured_voltage_BC",PADDR(measured_voltage_BC),
			PT_complex,	"measured_voltage_CA",PADDR(measured_voltage_CA),
			PT_bool, "phase_loss_protection", PADDR(three_phase_protect), PT_DESCRIPTION, "Trip all three phases of the load if a fault occurs",

			// This allows the user to set a base power on each phase, and specify the power as a function
			// of ZIP and pf for each phase (similar to zipload).  This will override the constant values
			// above if specified, otherwise, constant values work as stated
			PT_double, "base_power_A[VA]",PADDR(base_power[0]),
			PT_double, "base_power_B[VA]",PADDR(base_power[1]),
			PT_double, "base_power_C[VA]",PADDR(base_power[2]),
			PT_double, "power_pf_A[pu]",PADDR(power_pf[0]),
			PT_double, "current_pf_A[pu]",PADDR(current_pf[0]),
			PT_double, "impedance_pf_A[pu]",PADDR(impedance_pf[0]),
			PT_double, "power_pf_B[pu]",PADDR(power_pf[1]),
			PT_double, "current_pf_B[pu]",PADDR(current_pf[1]),
			PT_double, "impedance_pf_B[pu]",PADDR(impedance_pf[1]),
			PT_double, "power_pf_C[pu]",PADDR(power_pf[2]),
			PT_double, "current_pf_C[pu]",PADDR(current_pf[2]),
			PT_double, "impedance_pf_C[pu]",PADDR(impedance_pf[2]),
			PT_double, "power_fraction_A[pu]",PADDR(power_fraction[0]),
			PT_double, "current_fraction_A[pu]",PADDR(current_fraction[0]),
			PT_double, "impedance_fraction_A[pu]",PADDR(impedance_fraction[0]),
			PT_double, "power_fraction_B[pu]",PADDR(power_fraction[1]),
			PT_double, "current_fraction_B[pu]",PADDR(current_fraction[1]),
			PT_double, "impedance_fraction_B[pu]",PADDR(impedance_fraction[1]),
			PT_double, "power_fraction_C[pu]",PADDR(power_fraction[2]),
			PT_double, "current_fraction_C[pu]",PADDR(current_fraction[2]),
			PT_double, "impedance_fraction_C[pu]",PADDR(impedance_fraction[2]),

         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
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

    return res;
}

TIMESTAMP load::presync(TIMESTAMP t0)
{
	if ((solver_method!=SM_FBS) && (SubNode==PARENT))	//Need to do something slightly different with GS and parented node
	{
		shunt[0] = shunt[1] = shunt[2] = 0.0;
		power[0] = power[1] = power[2] = 0.0;
		current[0] = current[1] = current[2] = 0.0;
	}
	
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::presync(t0);
	
	return result;
}

TIMESTAMP load::sync(TIMESTAMP t0)
{
	bool all_three_phases;

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
					real_power = base_power[index] * power_fraction[index] * power_pf[index];
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
					real_power = base_power[index] * current_fraction[index] * current_pf[index];
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
					real_power = base_power[index] * impedance_fraction[index] * impedance_pf[index];
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

	if (fault_check_object == NULL)	//Not reliability mode - normal mode
	{
		if ((solver_method!=SM_FBS) && (SubNode==PARENT))	//Need to do something slightly different with GS/NR and parented load
		{													//associated with change due to player methods

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
		}
	}
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
			if ((solver_method!=SM_FBS) && (SubNode==PARENT))	//Need to do something slightly different with GS/NR and parented load
			{													//associated with change due to player methods

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
		}
	}

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
		return my->init();
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

/**@}*/
