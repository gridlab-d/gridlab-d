/** $Id: emissions.cpp, V 2.2 2011-2-11 17:00:00Z Bharat Exp $
	Copyright (C) 2011 Battelle Memorial Institute

	@Warning: It is still under development.

	@file emissions.cpp
	@addtogroup powerflow recloser
	@ingroup powerflow

	Implements a emissions object.

@{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>

using namespace std;

#include "emissions.h"

//////////////////////////////////////////////////////////////////////////
// fault_check CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* emissions::oclass = NULL;
CLASS* emissions::pclass = NULL;

emissions::emissions(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"emissions",sizeof(emissions),PC_POSTTOPDOWN);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		
		if(gl_publish_variable(oclass,
			PT_complex, "X1[kWh]", PADDR(Naturalgas_Max_Out),
			PT_complex, "X2[kWh]", PADDR(Coal_Max_Out),
			PT_complex, "X3[kWh]", PADDR(Biomass_Max_Out),
			PT_complex, "X4[kWh]", PADDR(Geothermal_Max_Out),
			PT_complex, "X5[kWh]", PADDR(Hydroelectric_Max_Out),
			PT_complex, "X6[kWh]", PADDR(Nuclear_Max_Out),
			PT_complex, "X7[kWh]", PADDR(Wind_Max_Out),
			PT_complex, "X8[kWh]", PADDR(Petroleum_Max_Out),
			PT_complex, "X9[kWh]", PADDR(Solarthermal_Max_Out),

			PT_double, "X1C[Btu/kWh]", PADDR(Naturalgas_Conv_Eff),
			PT_double, "X2C[Btu/kWh]", PADDR(Coal_Conv_Eff),
			PT_double, "X3C[Btu/kWh]", PADDR(Biomass_Conv_Eff),
			PT_double, "X4C[Btu/kWh]", PADDR(Geothermal_Conv_Eff),
			PT_double, "X5C[Btu/kWh]", PADDR(Hydroelectric_Conv_Eff),
			PT_double, "X6C[Btu/kWh]", PADDR(Nuclear_Conv_Eff),
			PT_double, "X7C[Btu/kWh]", PADDR(Wind_Conv_Eff),
			PT_double, "X8C[Btu/kWh]", PADDR(Petroleum_Conv_Eff),
			PT_double, "X9C[Btu/kWh]", PADDR(Solarthermal_Conv_Eff),

			PT_double, "X1_CO2[lb/Btu]", PADDR(Naturalgas_CO2),
			PT_double, "X2_CO2[lb/Btu]", PADDR(Coal_CO2),
			PT_double, "X3_CO2[lb/Btu]", PADDR(Biomass_CO2),
			PT_double, "X4_CO2[lb/Btu]", PADDR(Geothermal_CO2),
			PT_double, "X5_CO2[lb/Btu]", PADDR(Hydroelectric_CO2),
			PT_double, "X6_CO2[lb/Btu]", PADDR(Nuclear_CO2),
			PT_double, "X7_CO2[lb/Btu]", PADDR(Wind_CO2),
			PT_double, "X8_CO2[lb/Btu]", PADDR(Petroleum_CO2),
			PT_double, "X9_CO2[lb/Btu]", PADDR(Solarthermal_CO2),

			PT_double, "X1_SO2[lb/Btu]", PADDR(Naturalgas_SO2),
			PT_double, "X2_SO2[lb/Btu]", PADDR(Coal_SO2),
			PT_double, "X3_SO2[lb/Btu]", PADDR(Biomass_SO2),
			PT_double, "X4_SO2[lb/Btu]", PADDR(Geothermal_SO2),
			PT_double, "X5_SO2[lb/Btu]", PADDR(Hydroelectric_SO2),
			PT_double, "X6_SO2[lb/Btu]", PADDR(Nuclear_SO2),
			PT_double, "X7_SO2[lb/Btu]", PADDR(Wind_SO2),
			PT_double, "X8_SO2[lb/Btu]", PADDR(Petroleum_SO2),
			PT_double, "X9_SO2[lb/Btu]", PADDR(Solarthermal_SO2),

			PT_double, "X1_NOx[lb/Btu]", PADDR(Naturalgas_NOx),
			PT_double, "X2_NOx[lb/Btu]", PADDR(Coal_NOx),
			PT_double, "X3_NOx[lb/Btu]", PADDR(Biomass_NOx),
			PT_double, "X4_NOx[lb/Btu]", PADDR(Geothermal_NOx),
			PT_double, "X5_NOx[lb/Btu]", PADDR(Hydroelectric_NOx),
			PT_double, "X6_NOx[lb/Btu]", PADDR(Nuclear_NOx),
			PT_double, "X7_NOx[lb/Btu]", PADDR(Wind_NOx),
			PT_double, "X8_NOx[lb/Btu]", PADDR(Petroleum_NOx),
			PT_double, "X9_NOx[lb/Btu]", PADDR(Solarthermal_NOx),

			PT_complex, "Naturalgas_Out[kWh]", PADDR(Naturalgas_Out),
			PT_complex, "Coal_Out[kWh]", PADDR(Coal_Out),
			PT_complex, "Biomass_Out[kWh]", PADDR(Biomass_Out),
			PT_complex, "Geothermal_Out[kWh]", PADDR(Geothermal_Out),
			PT_complex, "Hydroelectric_Out[kWh]", PADDR(Hydroelectric_Out),
			PT_complex, "Nuclear_Out[kWh]", PADDR(Nuclear_Out),
			PT_complex, "Wind_Out[kWh]", PADDR(Wind_Out),
			PT_complex, "Petroleum_Out[kWh]", PADDR(Petroleum_Out),
			PT_complex, "Solarthermal_Out[kWh]", PADDR(Solarthermal_Out),


			PT_double, "Naturalgas_emissions_CO2", PADDR(Naturalgas_emissions_CO2),
			PT_double, "Naturalgas_emissions_SO2", PADDR(Naturalgas_emissions_SO2),
			PT_double, "Naturalgas_emissions_NOx", PADDR(Naturalgas_emissions_NOx),

			PT_double, "Coal_emissions_CO2", PADDR(Coal_emissions_CO2),
			PT_double, "Coal_emissions_SO2", PADDR(Coal_emissions_SO2),
			PT_double, "Coal_emissions_NOx", PADDR(Coal_emissions_NOx),

			PT_double, "Biomass_emissions_CO2", PADDR(Biomass_emissions_CO2),
			PT_double, "Biomass_emissions_SO2", PADDR(Biomass_emissions_SO2),
			PT_double, "Biomass_emissions_NOx", PADDR(Biomass_emissions_NOx),

			PT_double, "Geothermal_emissions_CO2", PADDR(Geothermal_emissions_CO2),
			PT_double, "Geothermal_emissions_SO2", PADDR(Geothermal_emissions_SO2),
			PT_double, "Geothermal_emissions_NOx", PADDR(Geothermal_emissions_NOx),

			PT_double, "Hydroelectric_emissions_CO2", PADDR(Hydroelectric_emissions_CO2),
			PT_double, "Hydroelectric_emissions_SO2", PADDR(Hydroelectric_emissions_SO2),
			PT_double, "Hydroelectric_emissions_NOx", PADDR(Hydroelectric_emissions_NOx),

			PT_double, "Nuclear_emissions_CO2", PADDR(Nuclear_emissions_CO2),
			PT_double, "Nuclear_emissions_SO2", PADDR(Nuclear_emissions_SO2),
			PT_double, "Nuclear_emissions_NOx", PADDR(Nuclear_emissions_NOx),

			PT_double, "Wind_emissions_CO2", PADDR(Wind_emissions_CO2),
			PT_double, "Wind_emissions_SO2", PADDR(Wind_emissions_SO2),
			PT_double, "Wind_emissions_NOx", PADDR(Wind_emissions_NOx),

			PT_double, "Petroleum_emissions_CO2", PADDR(Petroleum_emissions_CO2),
			PT_double, "Petroleum_emissions_SO2", PADDR(Petroleum_emissions_SO2),
			PT_double, "Petroleum_emissions_NOx", PADDR(Petroleum_emissions_NOx),

			PT_double, "Solarthermal_emissions_CO2", PADDR(Solarthermal_emissions_CO2),
			PT_double, "Solarthermal_emissions_SO2", PADDR(Solarthermal_emissions_SO2),
			PT_double, "Solarthermal_emissions_NOx", PADDR(Solarthermal_emissions_NOx),

			PT_double, "Total_emissions_CO2", PADDR(Total_emissions_CO2),
			PT_double, "Total_emissions_SO2", PADDR(Total_emissions_SO2),
			PT_double, "Total_emissions_NOx", PADDR(Total_emissions_NOx),

			PT_complex, "Total_energy_out", PADDR(Total_emissions_CO2),

			PT_double,"cycle_interval[s]", PADDR(cycle_interval),
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int emissions::isa(char *classname)
{
	return strcmp(classname,"emissions")==0;
}

int emissions::create(void)
{
	int rval;

	rval = powerflow_object::create();
	
	phases = PHASE_A;	//Populate a phase to make powerflow happy

	//Initialize tracking variables
	cycle_interval_TS = 0;
	time_cycle_interval = 0;
	prev_cycle_time = 0;
	curr_cycle_time = 0;

	//Energy variables
	accumulated_energy = 0.0;
	cycle_power = 0.0;

	//Initialize all of the emissions variables
	Naturalgas_Max_Out = 0.0;
	Coal_Max_Out = 0.0;
	Biomass_Max_Out = 0.0;
	Geothermal_Max_Out = 0.0;
	Hydroelectric_Max_Out = 0.0;
	Nuclear_Max_Out = 0.0;
	Wind_Max_Out = 0.0;
	Petroleum_Max_Out = 0.0;
	Solarthermal_Max_Out = 0.0;
	Naturalgas_Conv_Eff = 0.0;
	Coal_Conv_Eff = 0.0;
	Biomass_Conv_Eff = 0.0;
	Geothermal_Conv_Eff = 0.0;
	Hydroelectric_Conv_Eff = 0.0;
	Nuclear_Conv_Eff = 0.0;
	Wind_Conv_Eff = 0.0;
	Petroleum_Conv_Eff = 0.0;
	Solarthermal_Conv_Eff = 0.0;
	Naturalgas_CO2 = 0.0;
	Coal_CO2 = 0.0;
	Biomass_CO2 = 0.0;
	Geothermal_CO2 = 0.0;
	Hydroelectric_CO2 = 0.0;
	Nuclear_CO2 = 0.0;
	Wind_CO2 = 0.0;
	Petroleum_CO2 = 0.0;
	Solarthermal_CO2 = 0.0;
	Naturalgas_SO2 = 0.0;
	Coal_SO2 = 0.0;
	Biomass_SO2 = 0.0;
	Geothermal_SO2 = 0.0;
	Hydroelectric_SO2 = 0.0;
	Nuclear_SO2 = 0.0;
	Wind_SO2 = 0.0;
	Petroleum_SO2 = 0.0;
	Solarthermal_SO2 = 0.0;
	Naturalgas_NOx = 0.0;
	Coal_NOx = 0.0;
	Biomass_NOx = 0.0;
	Geothermal_NOx = 0.0;
	Hydroelectric_NOx = 0.0;
	Nuclear_NOx = 0.0;
	Wind_NOx = 0.0;
	Petroleum_NOx = 0.0;
	Solarthermal_NOx = 0.0;
	Naturalgas_Out = 0.0;
	Coal_Out = 0.0;
	Biomass_Out = 0.0;
	Geothermal_Out = 0.0;
	Hydroelectric_Out = 0.0;
	Nuclear_Out = 0.0;
	Wind_Out = 0.0;
	Petroleum_Out = 0.0;
	Solarthermal_Out = 0.0;
	Naturalgas_emissions_CO2 = 0.0;
	Naturalgas_emissions_SO2 = 0.0;
	Naturalgas_emissions_NOx = 0.0;
	Coal_emissions_CO2 = 0.0;
	Coal_emissions_SO2 = 0.0;
	Coal_emissions_NOx = 0.0;
	Biomass_emissions_CO2 = 0.0;
	Biomass_emissions_SO2 = 0.0;
	Biomass_emissions_NOx = 0.0;
	Geothermal_emissions_CO2 = 0.0;
	Geothermal_emissions_SO2 = 0.0;
	Geothermal_emissions_NOx = 0.0;
	Hydroelectric_emissions_CO2 = 0.0;
	Hydroelectric_emissions_SO2 = 0.0;
	Hydroelectric_emissions_NOx = 0.0;
	Nuclear_emissions_CO2 = 0.0;
	Nuclear_emissions_SO2 = 0.0;
	Nuclear_emissions_NOx = 0.0;
	Wind_emissions_CO2 = 0.0;
	Wind_emissions_SO2 = 0.0;
	Wind_emissions_NOx = 0.0;
	Petroleum_emissions_CO2 = 0.0;
	Petroleum_emissions_SO2 = 0.0;
	Petroleum_emissions_NOx = 0.0;
	Solarthermal_emissions_CO2 = 0.0;
	Solarthermal_emissions_SO2 = 0.0;
	Solarthermal_emissions_NOx = 0.0;
	Total_emissions_CO2 = 0.0;
	Total_emissions_SO2 = 0.0;
	Total_emissions_NOx = 0.0;
	Total_energy_out = 0.0;

	return rval;
}

int emissions::init(OBJECT *parent)
{
	int rval;
	OBJECT *obj = OBJECTHDR(this);

	rval = powerflow_object::init(parent);
 
	//Make sure we have a meter
	if (parent!=NULL)
	{
		//Make sure our parent is a meter
		if (gl_object_isa(parent,"meter","powerflow"))
		{
			//Try to map it
			ParMeterObj = OBJECTDATA(parent,meter);
		}
		else
		{
			GL_THROW("emissions:%s must be parented to a three-phase meter to work!",obj->name);
			/*  TROUBLESHOOT
			The emissions object requires a three-phase meter object to properly work.  Please parent
			the emissions object to such a meter and try again.
			*/
		}
	}
	else
	{
		GL_THROW("emissions:%s must be parented to a three-phase meter to work!",obj->name);
		//Use same error as above, still relevant
	}

	//Check the cycle interval - if it isn't populated, make it 15 mintues
	if (cycle_interval == 0.0)
		cycle_interval = 900.0;

	//Make sure it is valid
	if (cycle_interval <= 0)
	{
		GL_THROW("emissions:%s must have a positive cycle_interval time",obj->name);
		/*  TROUBLESHOOT
		The calculation cycle interval for the emissions object must be a non-zero, positive
		value.  Please specify one and try again.
		*/
	}

	//Convert it to a TIMESTAMP
	cycle_interval_TS = (TIMESTAMP)(cycle_interval);

	//Other initialization values (I assume these will eventually be populated, since they effectively do nothing right now)
	if(Naturalgas_Out==0) Naturalgas_Out = 0;

	if(Coal_Out == 0) Coal_Out = 0;
	if(Biomass_Out == 0) Biomass_Out = 0;
	if(Geothermal_Out == 0) Geothermal_Out = 0;
	if(Hydroelectric_Out == 0) Hydroelectric_Out = 0;
	if(Nuclear_Out == 0) Nuclear_Out = 0;
	if (Wind_Out == 0) Wind_Out =0;
	if (Petroleum_Out == 0) Petroleum_Out = 0;
	if(Solarthermal_Out == 0) Solarthermal_Out = 0;


	if(Naturalgas_CO2 == 0) Naturalgas_CO2 = 0;
	if(Coal_CO2 == 0) Coal_CO2 = 0;
	if(Biomass_CO2 == 0) Biomass_CO2 = 0;
	if(Geothermal_CO2 == 0) Geothermal_CO2 = 0;
	if(Hydroelectric_CO2 == 0) Hydroelectric_CO2 = 0;
	if(Nuclear_CO2 == 0) Nuclear_CO2 = 0;
	if(Wind_CO2 == 0) Wind_CO2 = 0;
	if(Petroleum_CO2 == 0) Petroleum_CO2 = 0;
	if(Solarthermal_CO2 == 0) Solarthermal_CO2 = 0;

	if(Naturalgas_SO2 == 0) Naturalgas_SO2 = 0;
	if(Coal_SO2 == 0) Coal_SO2 = 0;
	if(Biomass_SO2 == 0) Biomass_SO2 = 0;
	if(Geothermal_SO2 == 0) Geothermal_SO2 = 0;
	if(Hydroelectric_SO2 == 0) Hydroelectric_SO2 = 0;
	if(Nuclear_SO2 == 0) Nuclear_SO2 = 0;
	if(Wind_SO2 == 0) Wind_SO2 = 0;
	if(Petroleum_SO2 == 0) Petroleum_SO2 = 0;
	if(Solarthermal_SO2 == 0) Solarthermal_SO2 = 0;

	if(Naturalgas_NOx == 0) Naturalgas_NOx = 0;
	if(Coal_NOx == 0) Coal_NOx = 0;
	if(Biomass_NOx == 0) Biomass_NOx = 0;
	if(Geothermal_NOx == 0) Geothermal_NOx = 0;
	if(Hydroelectric_NOx == 0) Hydroelectric_NOx = 0;
	if(Nuclear_NOx == 0) Nuclear_NOx = 0;
	if(Wind_NOx == 0) Wind_NOx = 0;
	if(Petroleum_NOx == 0) Petroleum_NOx = 0;
	if(Solarthermal_NOx == 0) Solarthermal_NOx = 0;		

	return rval;
}

TIMESTAMP emissions::postsync(TIMESTAMP t0)
{
	double temp_energy;
	complex temp_power;
	complex energy_for_calc;
	bool energy_requirement;
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::postsync(t0);

	//First cycle, set up the interval
	if ((prev_cycle_time == 0) && (t0 != 0))
	{
		time_cycle_interval = t0 + cycle_interval_TS;
	}

	if (prev_cycle_time != t0)	//New timestamp - accumulate
	{
		//Store current cycle
		curr_cycle_time = t0;

		//Find dt from last interval
		dt_val = (double)(curr_cycle_time - prev_cycle_time);

		//Update tracking variable
		prev_cycle_time = t0;

		//Accumulate the energy value
		temp_energy = cycle_power* dt_val/3600;
		accumulated_energy += temp_energy;
	}

	//Power determination code
	if ((curr_cycle_time == t0) && (NR_cycle == true))	//Accumulation cycle
	{
		//Grab the current power value - put in kVA
		temp_power = (ParMeterObj->indiv_measured_power[0] + ParMeterObj->indiv_measured_power[1] + ParMeterObj->indiv_measured_power[2]);
		cycle_power = (temp_power.Re()) / 1000.0;
	}

	//See if it is time for a computation!
	if (curr_cycle_time >= time_cycle_interval)	//Update values
	{
		//Set temp variable
		energy_requirement = true;

		//Store the value in - cast it as a complex
		energy_for_calc = accumulated_energy;

		//Proceed
		if (Naturalgas_Max_Out.Mag() < energy_for_calc.Mag() && (energy_requirement == true))
		{
			Naturalgas_Out = Naturalgas_Max_Out;

			Naturalgas_emissions_CO2 = Naturalgas_Out.Mag() * Naturalgas_Conv_Eff * Naturalgas_CO2;
			Naturalgas_emissions_SO2 = Naturalgas_Out.Mag() * Naturalgas_Conv_Eff * Naturalgas_SO2;
			Naturalgas_emissions_NOx = Naturalgas_Out.Mag() * Naturalgas_Conv_Eff * Naturalgas_NOx;

			energy_for_calc = energy_for_calc - Naturalgas_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Naturalgas_Out = energy_for_calc;

			Naturalgas_emissions_CO2 = Naturalgas_Out.Mag() * Naturalgas_Conv_Eff * Naturalgas_CO2;
			Naturalgas_emissions_SO2 = Naturalgas_Out.Mag() * Naturalgas_Conv_Eff * Naturalgas_SO2;
			Naturalgas_emissions_NOx = Naturalgas_Out.Mag() * Naturalgas_Conv_Eff* Naturalgas_NOx;	

			energy_requirement = false;

			energy_for_calc = energy_for_calc - Naturalgas_Out;

		}

		if (Coal_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Coal_Out = Coal_Max_Out;

			Coal_emissions_CO2 = Coal_Out.Mag() * Coal_Conv_Eff * Coal_CO2;
			Coal_emissions_SO2 = Coal_Out.Mag() * Coal_Conv_Eff * Coal_SO2;
			Coal_emissions_NOx = Coal_Out.Mag() * Coal_Conv_Eff * Coal_NOx;

			energy_for_calc = energy_for_calc - Coal_Max_Out; 
		}
		else if (energy_requirement == true)
		{
			Coal_Out = energy_for_calc;

			Coal_emissions_CO2 = Coal_Out.Mag() * Coal_Conv_Eff * Coal_CO2;
			Coal_emissions_SO2 = Coal_Out.Mag() * Coal_Conv_Eff * Coal_SO2;
			Coal_emissions_NOx = Coal_Out.Mag() * Coal_Conv_Eff * Coal_NOx;	

			energy_requirement = false;

			energy_for_calc = energy_for_calc - Coal_Out; 

		}
		if (Biomass_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Biomass_Out = Biomass_Max_Out ;

			Biomass_emissions_CO2 = Biomass_Out.Mag() * Biomass_Conv_Eff * Biomass_CO2;
			Biomass_emissions_SO2 = Biomass_Out.Mag() * Biomass_Conv_Eff * Biomass_SO2;
			Biomass_emissions_NOx = Biomass_Out.Mag() * Biomass_Conv_Eff * Biomass_NOx;

			energy_for_calc = energy_for_calc - Biomass_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Biomass_Out = energy_for_calc;

			Biomass_emissions_CO2 = Biomass_Out.Mag() * Biomass_Conv_Eff * Biomass_CO2;
			Biomass_emissions_SO2 = Biomass_Out.Mag() * Biomass_Conv_Eff * Biomass_SO2;
			Biomass_emissions_NOx = Biomass_Out.Mag() * Biomass_Conv_Eff * Biomass_NOx;

			energy_requirement = false;

			energy_for_calc = energy_for_calc - Biomass_Out;
	
		}
		if (Geothermal_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Geothermal_Out = Geothermal_Max_Out;

			Geothermal_emissions_CO2 = Geothermal_Out.Mag() * Geothermal_Conv_Eff * Geothermal_CO2;
			Geothermal_emissions_SO2 = Geothermal_Out.Mag() * Geothermal_Conv_Eff * Geothermal_SO2;
			Geothermal_emissions_NOx = Geothermal_Out.Mag() * Geothermal_Conv_Eff * Geothermal_NOx;

			energy_for_calc = energy_for_calc - Geothermal_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Geothermal_Out = energy_for_calc;

			Geothermal_emissions_CO2 = Geothermal_Out.Mag() * Geothermal_Conv_Eff * Geothermal_CO2;
			Geothermal_emissions_SO2 = Geothermal_Out.Mag() * Geothermal_Conv_Eff * Geothermal_SO2;
			Geothermal_emissions_NOx = Geothermal_Out.Mag() * Geothermal_Conv_Eff * Geothermal_NOx;

			energy_requirement = false;	

			energy_for_calc = energy_for_calc - Geothermal_Out;

		}
		if (Hydroelectric_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Hydroelectric_Out = Hydroelectric_Max_Out;

			Hydroelectric_emissions_CO2 = Hydroelectric_Out.Mag() * Hydroelectric_Conv_Eff * Hydroelectric_CO2;
			Hydroelectric_emissions_SO2 = Hydroelectric_Out.Mag() * Hydroelectric_Conv_Eff * Hydroelectric_SO2;
			Hydroelectric_emissions_NOx = Hydroelectric_Out.Mag() * Hydroelectric_Conv_Eff * Hydroelectric_NOx;

			energy_for_calc = energy_for_calc - Hydroelectric_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Hydroelectric_Out = energy_for_calc;

			
			Hydroelectric_emissions_CO2 = Hydroelectric_Out.Mag() * Hydroelectric_Conv_Eff * Hydroelectric_CO2;
			Hydroelectric_emissions_SO2 = Hydroelectric_Out.Mag() * Hydroelectric_Conv_Eff * Hydroelectric_SO2;
			Hydroelectric_emissions_NOx = Hydroelectric_Out.Mag() * Hydroelectric_Conv_Eff * Hydroelectric_NOx;

			energy_requirement = false;

			energy_for_calc = energy_for_calc - Hydroelectric_Out;
		

		}
		if (Nuclear_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Nuclear_Out = Nuclear_Max_Out;


			Nuclear_emissions_CO2 = Nuclear_Out.Mag() * Nuclear_Conv_Eff * Nuclear_CO2;
			Nuclear_emissions_SO2 = Nuclear_Out.Mag() * Nuclear_Conv_Eff * Nuclear_SO2;
			Nuclear_emissions_NOx = Nuclear_Out.Mag() * Nuclear_Conv_Eff * Nuclear_NOx;

			energy_for_calc = energy_for_calc - Nuclear_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Nuclear_Out = energy_for_calc;

			Nuclear_emissions_CO2 = Nuclear_Out.Mag() * Nuclear_Conv_Eff * Nuclear_CO2;
			Nuclear_emissions_SO2 = Nuclear_Out.Mag() * Nuclear_Conv_Eff * Nuclear_SO2;
			Nuclear_emissions_NOx = Nuclear_Out.Mag() * Nuclear_Conv_Eff * Nuclear_NOx;

			energy_requirement = false;

			energy_for_calc = energy_for_calc - Nuclear_Out;

		}
		if (Wind_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Wind_Out = Wind_Max_Out;

			Wind_emissions_CO2 = Wind_Out.Mag() * Wind_Conv_Eff * Wind_CO2;
			Wind_emissions_SO2 = Wind_Out.Mag() * Wind_Conv_Eff * Wind_SO2;
			Wind_emissions_NOx = Wind_Out.Mag() * Wind_Conv_Eff * Wind_NOx;

			energy_for_calc = energy_for_calc - Wind_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Wind_Out = energy_for_calc;

			Wind_emissions_CO2 = Wind_Out.Mag() * Wind_Conv_Eff * Wind_CO2;
			Wind_emissions_SO2 = Wind_Out.Mag() * Wind_Conv_Eff * Wind_SO2;
			Wind_emissions_NOx = Wind_Out.Mag() * Wind_Conv_Eff * Wind_NOx;	

			energy_requirement = false;

			energy_for_calc = energy_for_calc - Wind_Out;


		}
		if (Petroleum_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Petroleum_Out = Petroleum_Max_Out;

			Petroleum_emissions_CO2 = Petroleum_Out.Mag() * Petroleum_Conv_Eff * Petroleum_CO2;
			Petroleum_emissions_SO2 = Petroleum_Out.Mag() * Petroleum_Conv_Eff * Petroleum_SO2;
			Petroleum_emissions_NOx = Petroleum_Out.Mag() * Petroleum_Conv_Eff * Petroleum_NOx;

			energy_for_calc = energy_for_calc - Petroleum_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Petroleum_Out = energy_for_calc;

			Petroleum_emissions_CO2 = Petroleum_Out.Mag() * Petroleum_Conv_Eff * Petroleum_CO2;
			Petroleum_emissions_SO2 = Petroleum_Out.Mag() * Petroleum_Conv_Eff * Petroleum_SO2;
			Petroleum_emissions_NOx = Petroleum_Out.Mag() * Petroleum_Conv_Eff * Petroleum_NOx;	
			
			energy_requirement = false;

			energy_for_calc = energy_for_calc - Petroleum_Out;

		}

		if (Solarthermal_Max_Out.Mag() < energy_for_calc.Mag() && energy_requirement == true)
		{
			Solarthermal_Out = Solarthermal_Max_Out;

			Solarthermal_emissions_CO2 = Solarthermal_Out.Mag() * Solarthermal_Conv_Eff * Solarthermal_CO2;
			Solarthermal_emissions_SO2 = Solarthermal_Out.Mag() * Solarthermal_Conv_Eff * Solarthermal_SO2;
			Solarthermal_emissions_NOx = Solarthermal_Out.Mag() * Solarthermal_Conv_Eff * Solarthermal_NOx;

			energy_for_calc = energy_for_calc - Solarthermal_Max_Out;
		}
		else if (energy_requirement == true)
		{
			Solarthermal_Out = energy_for_calc;

			Solarthermal_emissions_CO2 = Solarthermal_Out.Mag() * Solarthermal_Conv_Eff * Solarthermal_CO2;
			Solarthermal_emissions_SO2 = Solarthermal_Out.Mag() * Solarthermal_Conv_Eff * Solarthermal_SO2;
			Solarthermal_emissions_NOx = Solarthermal_Out.Mag() * Solarthermal_Conv_Eff * Solarthermal_NOx;
			
			energy_for_calc = energy_for_calc - Solarthermal_Out;

			
		}
		
		Total_energy_out = Naturalgas_Out + Coal_Out + Biomass_Out + Geothermal_Out + Hydroelectric_Out + Nuclear_Out + Wind_Out + Petroleum_Out + Solarthermal_Out;

		Total_emissions_CO2 = Naturalgas_emissions_CO2 + Coal_emissions_CO2 + Biomass_emissions_CO2 + Geothermal_emissions_CO2 + Hydroelectric_emissions_CO2 + Nuclear_emissions_CO2 + Wind_emissions_CO2 + Petroleum_emissions_CO2 + Solarthermal_emissions_CO2;

		Total_emissions_SO2 = Naturalgas_emissions_SO2 + Coal_emissions_SO2 + Biomass_emissions_SO2 + Geothermal_emissions_SO2 + Hydroelectric_emissions_SO2 + Nuclear_emissions_SO2 + Wind_emissions_SO2 + Petroleum_emissions_SO2 + Solarthermal_emissions_SO2;

		Total_emissions_NOx = Naturalgas_emissions_NOx + Coal_emissions_NOx + Biomass_emissions_NOx + Geothermal_emissions_NOx + Hydroelectric_emissions_NOx + Nuclear_emissions_NOx + Wind_emissions_NOx + Petroleum_emissions_NOx + Solarthermal_emissions_NOx;

		
		//Zero the energy accumulator
		accumulated_energy = 0.0;
		
		//Update the cycle interval
		time_cycle_interval += cycle_interval_TS;

	}//End of calculation phase

	//Determine what to return
	if (tret > time_cycle_interval)
	{
		tret = time_cycle_interval;	//Next output cycle is sooner
	}

	//Make sure we don't send out a negative TS_NEVER
	if (tret == TS_NEVER)
		return TS_NEVER;
	else
		return -tret;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: fault_check
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_emissions(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(emissions::oclass);
		if (*obj!=NULL)
		{
			emissions *my = OBJECTDATA(*obj,emissions);
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

EXPORT int init_emissions(OBJECT *obj, OBJECT *parent)
{
	try {
			return OBJECTDATA(obj,emissions)->init(parent);
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
EXPORT TIMESTAMP sync_emissions(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	emissions *pObj = OBJECTDATA(obj,emissions);
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
		gl_error("emissions %s (%s:%d): %s", obj->name, obj->oclass->name, obj->id, msg);
	}
	catch (...)
	{
		gl_error("emissions %s (%s:%d): unknown exception", obj->name, obj->oclass->name, obj->id);
	}
	return TS_INVALID; /* stop the clock */
}

EXPORT int isa_emissions(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,emissions)->isa(classname);
}

/**@}**/
