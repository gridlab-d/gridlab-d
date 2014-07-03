/** $Id: emissions.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute

	@Warning: It is still under development.
	@file emissions.h

@{
**/

#ifndef _EMISSIONS_H
#define _EMISSIONS_H

#include "powerflow.h"
#include "powerflow_object.h"
#include "meter.h"

class emissions : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	//Maximum power outputs from different energy sources

	double Nuclear_Order;
	double Hydroelectric_Order;
	double Solarthermal_Order;
	double Biomass_Order;
	double Wind_Order;
	double Coal_Order;
	double Naturalgas_Order;
	double Geothermal_Order;
	double Petroleum_Order;



	double Naturalgas_Max_Out;
	double Coal_Max_Out;
	double Biomass_Max_Out;
	double Geothermal_Max_Out;
	double Hydroelectric_Max_Out;
	double Nuclear_Max_Out;
	double Wind_Max_Out;
	double Petroleum_Max_Out;
	double Solarthermal_Max_Out;

	//Conversion efficiences of different energy sources
	double Naturalgas_Conv_Eff;
	double Coal_Conv_Eff;
	double Biomass_Conv_Eff;
	double Geothermal_Conv_Eff;
	double Hydroelectric_Conv_Eff;
	double Nuclear_Conv_Eff;
	double Wind_Conv_Eff;
	double Petroleum_Conv_Eff;
	double Solarthermal_Conv_Eff;


	double Naturalgas_CO2;
	double Coal_CO2;
	double Biomass_CO2;
	double Geothermal_CO2;
	double Hydroelectric_CO2;
	double Nuclear_CO2;
	double Wind_CO2;
	double Petroleum_CO2;
	double Solarthermal_CO2;

	double Naturalgas_SO2;
	double Coal_SO2;
	double Biomass_SO2;
	double Geothermal_SO2;
	double Hydroelectric_SO2;
	double Nuclear_SO2;
	double Wind_SO2;
	double Petroleum_SO2;
	double Solarthermal_SO2;

	double Naturalgas_NOx;
	double Coal_NOx;
	double Biomass_NOx;
	double Geothermal_NOx;
	double Hydroelectric_NOx;
	double Nuclear_NOx;
	double Wind_NOx;
	double Petroleum_NOx;
	double Solarthermal_NOx;

	double Naturalgas_Out;
	double Coal_Out;
	double Biomass_Out;
	double Geothermal_Out;
	double Hydroelectric_Out;
	double Nuclear_Out;
	double Wind_Out;
	double Petroleum_Out;
	double Solarthermal_Out;

	
	double Naturalgas_emissions_CO2;
	double Naturalgas_emissions_SO2;
	double Naturalgas_emissions_NOx;

	double Coal_emissions_CO2;
	double Coal_emissions_SO2;
	double Coal_emissions_NOx;

	double Biomass_emissions_CO2;
	double Biomass_emissions_SO2;
	double Biomass_emissions_NOx;

	double Geothermal_emissions_CO2;
	double Geothermal_emissions_SO2;
	double Geothermal_emissions_NOx;

	double Hydroelectric_emissions_CO2;
	double Hydroelectric_emissions_SO2;
	double Hydroelectric_emissions_NOx;

	double Nuclear_emissions_CO2;
	double Nuclear_emissions_SO2;
	double Nuclear_emissions_NOx;

	double Wind_emissions_CO2;
	double Wind_emissions_SO2;
	double Wind_emissions_NOx;

	double Petroleum_emissions_CO2;
	double Petroleum_emissions_SO2;
	double Petroleum_emissions_NOx;

	double Solarthermal_emissions_CO2;
	double Solarthermal_emissions_SO2;
	double Solarthermal_emissions_NOx;

	double Total_emissions_CO2;
	double Total_emissions_SO2;
	double Total_emissions_NOx;
	double Total_energy_out;

	double cycle_interval;

	double Region;

	//Link to the parent meter
	meter *ParMeterObj;

	//Energy calculation
	double accumulated_energy;
	double cycle_power;

public:
	emissions(MODULE *mod);
	emissions(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	TIMESTAMP postsync(TIMESTAMP t0);

private:
	TIMESTAMP cycle_interval_TS;
	TIMESTAMP time_cycle_interval;
	TIMESTAMP prev_cycle_time;
	TIMESTAMP curr_cycle_time;
	double dt_val;

};


#endif // _EMISSIONS_H
