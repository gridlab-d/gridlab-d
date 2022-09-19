/** $Id: windturb_dg.cpp 4738 2014-07-03 00:55:39Z dchassin $
Copyright (C) 2008 Battelle Memorial Institute
@file windturb_dg.cpp
@defgroup windturb_dg Wind Turbine gensets
@ingroup generators
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <gld_complex.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>

#include "windturb_dg.h"

CLASS *windturb_dg::oclass = nullptr;
windturb_dg *windturb_dg::defaults = nullptr;

windturb_dg::windturb_dg(MODULE *module)
{
	if (oclass==nullptr)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"windturb_dg",sizeof(windturb_dg),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class windturb_dg";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration,"Gen_status",PADDR(Gen_status), PT_DESCRIPTION, "COP: Describes whether the generator is currently online or offline",
			PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE, PT_DESCRIPTION, "COP: Generator is currently not supplying power",
			PT_KEYWORD,"ONLINE",(enumeration)ONLINE, PT_DESCRIPTION, "COP: Generator is currently available to supply power",
			PT_enumeration,"Gen_type",PADDR(Gen_type), PT_DESCRIPTION, "COP: Type of generator",
			PT_KEYWORD,"INDUCTION",(enumeration)INDUCTION, PT_DESCRIPTION, "COP: Standard induction generator",
			PT_KEYWORD,"SYNCHRONOUS",(enumeration)SYNCHRONOUS, PT_DESCRIPTION, "COP: Standard synchronous generator; is also used to 'fake' a doubly-fed induction generator for now",
			PT_enumeration,"Gen_mode",PADDR(Gen_mode), PT_DESCRIPTION, "COP: Control mode that is used for the generator output",
			PT_KEYWORD,"CONSTANTE",(enumeration)CONSTANTE, PT_DESCRIPTION, "COP: Maintains the voltage at the terminals",
			PT_KEYWORD,"CONSTANTP",(enumeration)CONSTANTP, PT_DESCRIPTION, "COP: Maintains the real power output at the terminals",
			PT_KEYWORD,"CONSTANTPQ",(enumeration)CONSTANTPQ, PT_DESCRIPTION, "COP: Maintains the real and reactive output at the terminals - currently unsupported",
			PT_enumeration,"Turbine_Model",PADDR(Turbine_Model), PT_DESCRIPTION, "Type of turbine being represented; using any of these except USER_DEFINED also specifies a large number of defaults",
			PT_KEYWORD,"GENERIC_DEFAULT",(enumeration)GENERIC_DEFAULT, PT_DESCRIPTION, "Generic model used to set default parameters if user doesnot specify anything",
			PT_KEYWORD,"GENERIC_SYNCH_SMALL",(enumeration)GENERIC_SYNCH_SMALL, PT_DESCRIPTION, "COP: Generic model for a small, fixed pitch synchronous turbine",
			PT_KEYWORD,"GENERIC_SYNCH_MID",(enumeration)GENERIC_SYNCH_MID, PT_DESCRIPTION, "COP: Generic model for a mid-size, fixed pitch synchronous turbine",
			PT_KEYWORD,"GENERIC_SYNCH_LARGE",(enumeration)GENERIC_SYNCH_LARGE, PT_DESCRIPTION, "COP: Generic model for a large, fixed pitch synchronous turbine",
			PT_KEYWORD,"GENERIC_IND_SMALL",(enumeration)GENERIC_IND_SMALL, PT_DESCRIPTION, "COP: Generic model for a small induction, fixed pitch generator turbine",
			PT_KEYWORD,"GENERIC_IND_MID",(enumeration)GENERIC_IND_MID, PT_DESCRIPTION, "COP: Generic model for a mid-size induction, fixed pitch generator turbine",
			PT_KEYWORD,"GENERIC_IND_LARGE",(enumeration)GENERIC_IND_LARGE, PT_DESCRIPTION, "COP: Generic model for a large induction, fixed pitch generator turbine",
			PT_KEYWORD,"USER_DEFINED",(enumeration)USER_DEFINED, PT_DESCRIPTION, "COP: Allows user to specify all parameters - is not currently supported", // TODO: Doesnt work yet
			PT_KEYWORD,"VESTAS_V82",(enumeration)VESTAS_V82, PT_DESCRIPTION, "COP: Sets all defaults to represent the power output of a VESTAS V82 turbine",
			PT_KEYWORD,"GE_25MW",(enumeration)GE_25MW, PT_DESCRIPTION, "COP: Sets all defaults to represent the power output of a GE 2.5MW turbine",
			PT_KEYWORD,"BERGEY_10kW",(enumeration)BERGEY_10kW, PT_DESCRIPTION, "COP: Sets all defaults to represent the power output of a Bergey 10kW turbine",
			PT_KEYWORD,"GEN_TURB_POW_CURVE_2_4KW",(enumeration)GEN_TURB_POW_CURVE_2_4KW, PT_DESCRIPTION, "Generic model for a 2.4kW turbine, power curve implementation",
			PT_KEYWORD,"GEN_TURB_POW_CURVE_10KW",(enumeration)GEN_TURB_POW_CURVE_10KW, PT_DESCRIPTION, "Generic model for a 10kW turbine, power curve implementation",
			PT_KEYWORD,"GEN_TURB_POW_CURVE_100KW",(enumeration)GEN_TURB_POW_CURVE_100KW, PT_DESCRIPTION, "Generic model for a 100kW turbine, power curve implementation",
			PT_KEYWORD,"GEN_TURB_POW_CURVE_1_5MW",(enumeration)GEN_TURB_POW_CURVE_1_5MW, PT_DESCRIPTION, "Generic model for a 1.5MW turbine, power curve implementation",
			PT_enumeration,"Turbine_implementation",PADDR(Turbine_implementation), PT_DESCRIPTION, "Type of implementation for wind turbine model",
			PT_KEYWORD,"COEFF_OF_PERFORMANCE",(enumeration)COEFF_OF_PERFORMANCE, PT_DESCRIPTION, "Wind turbine generator implementation based on Coefficient of performance",
			PT_KEYWORD,"POWER_CURVE",(enumeration)POWER_CURVE, PT_DESCRIPTION, "Wind turbine implementation based on Generic Power Curve",

			PT_enumeration,"Wind_speed_source",PADDR(Wind_speed_source), PT_DESCRIPTION, "Specifies the source of wind speed",
			PT_KEYWORD,"DEFAULT",(enumeration)DEFAULT, PT_DESCRIPTION, "A defualt that equals either BUILT_IN or CLIMATE_DATA",
			PT_KEYWORD,"BUILT_IN",(enumeration)BUILT_IN, PT_DESCRIPTION, "The built-in source (avg_ws) specifying the wind speed",
			PT_KEYWORD,"WIND_SPEED",(enumeration)WIND_SPEED, PT_DESCRIPTION, "The wind speed at hub height specified by the user",
			PT_KEYWORD,"CLIMATE_DATA",(enumeration)CLIMATE_DATA, PT_DESCRIPTION, "The climate data specifying the wind speed",

			// TODO: There are a number of repeat variables due to poor variable name formatting.
			//       These need to be corrected through the deprecation process.
			PT_double, "turbine_height[m]", PADDR(turbine_height), PT_DESCRIPTION, "Describes the height of the wind turbine hub above the ground",
			PT_double, "roughness_length_factor", PADDR(roughness_l), PT_DESCRIPTION, "European Wind Atlas unitless correction factor for adjusting wind speed at various heights above ground and terrain types, default=0.055",
			PT_double, "blade_diam[m]", PADDR(blade_diam), PT_DESCRIPTION, "COP: Diameter of blades",
			PT_double, "blade_diameter[m]", PADDR(blade_diam), PT_DESCRIPTION, "COP: Diameter of blades",
			PT_double, "cut_in_ws[m/s]", PADDR(cut_in_ws), PT_DESCRIPTION, "COP: Minimum wind speed for generator operation",
			PT_double, "cut_out_ws[m/s]", PADDR(cut_out_ws), PT_DESCRIPTION, "COP: Maximum wind speed for generator operation",
			PT_double, "ws_rated[m/s]", PADDR(ws_rated), PT_DESCRIPTION, "COP: Rated wind speed for generator operation",
			PT_double, "ws_maxcp[m/s]", PADDR(ws_maxcp), PT_DESCRIPTION, "COP: Wind speed at which generator reaches maximum Cp",
			PT_double, "Cp_max[pu]", PADDR(Cp_max), PT_DESCRIPTION, "COP: Maximum coefficient of performance",
			PT_double, "Cp_rated[pu]", PADDR(Cp_rated), PT_DESCRIPTION, "COP: Rated coefficient of performance",
			PT_double, "Cp[pu]", PADDR(Cp), PT_DESCRIPTION, "COP: Calculated coefficient of performance",

			PT_double, "Rated_VA[VA]", PADDR(Rated_VA), PT_DESCRIPTION, "Rated generator power output",
			PT_double, "Rated_V[V]", PADDR(Rated_V), PT_DESCRIPTION, "COP: Rated generator terminal voltage",
			PT_double, "Pconv[W]", PADDR(Pconv), PT_DESCRIPTION, "COP: Amount of electrical power converted from mechanical power delivered",
			PT_double, "P_converted[W]", PADDR(Pconv), PT_DESCRIPTION, "COP: Amount of electrical power converted from mechanical power delivered",

			PT_double, "GenElecEff[%]", PADDR(GenElecEff), PT_DESCRIPTION, "COP: Calculated generator electrical efficiency",
			PT_double, "generator_efficiency[%]", PADDR(GenElecEff), PT_DESCRIPTION, "COP: Calculated generator electrical efficiency",
			PT_double, "TotalRealPow[W]", PADDR(TotalRealPow), PT_DESCRIPTION, "Total real power output",
			PT_double, "total_real_power[W]", PADDR(TotalRealPow), PT_DESCRIPTION, "Total real power output",
			PT_double, "TotalReacPow[VA]", PADDR(TotalReacPow), PT_DESCRIPTION, "Total reactive power output",
			PT_double, "total_reactive_power[VA]", PADDR(TotalReacPow), PT_DESCRIPTION, "Total reactive power output",

			PT_complex, "power_A[VA]", PADDR(power_A), PT_DESCRIPTION, "Total complex power injected on phase A",
			PT_complex, "power_B[VA]", PADDR(power_B), PT_DESCRIPTION, "Total complex power injected on phase B",
			PT_complex, "power_C[VA]", PADDR(power_C), PT_DESCRIPTION, "Total complex power injected on phase C",
			PT_complex, "power_12[VA]", PADDR(power_A), PT_DESCRIPTION, "Triplex mode - Total complex power on line 1 and 2",

			PT_double, "WSadj[m/s]", PADDR(WSadj), PT_DESCRIPTION, "Speed of wind at hub height",
			PT_double, "wind_speed_adjusted[m/s]", PADDR(WSadj), PT_DESCRIPTION, "Speed of wind at hub height",
			PT_double, "Wind_Speed[m/s]", PADDR(Wind_Speed),  PT_DESCRIPTION, "Wind speed at 5-15m level (typical measurement height)",
			PT_double, "wind_speed[m/s]", PADDR(Wind_Speed),  PT_DESCRIPTION, "Wind speed at 5-15m level (typical measurement height)",
			PT_double, "air_density[kg/m^3]", PADDR(air_dens), PT_DESCRIPTION, "COP: Estimated air density",
			PT_double, "wind_speed_hub_ht[m/s]", PADDR(wind_speed_hub_ht), PT_DESCRIPTION, "Wind speed at hub height. This is an input",

			PT_double, "R_stator[pu*Ohm]", PADDR(Rst), PT_DESCRIPTION, "COP: Induction generator primary stator resistance in p.u.",
			PT_double, "X_stator[pu*Ohm]", PADDR(Xst), PT_DESCRIPTION, "COP: Induction generator primary stator reactance in p.u.",
			PT_double, "R_rotor[pu*Ohm]", PADDR(Rr), PT_DESCRIPTION, "COP: Induction generator primary rotor resistance in p.u.",
			PT_double, "X_rotor[pu*Ohm]", PADDR(Xr), PT_DESCRIPTION, "COP: Induction generator primary rotor reactance in p.u.",
			PT_double, "R_core[pu*Ohm]", PADDR(Rc), PT_DESCRIPTION, "COP: Induction generator primary core resistance in p.u.",
			PT_double, "X_magnetic[pu*Ohm]", PADDR(Xm), PT_DESCRIPTION, "COP: Induction generator primary core reactance in p.u.",
			PT_double, "Max_Vrotor[pu*V]", PADDR(Max_Vrotor), PT_DESCRIPTION, "COP: Induction generator maximum induced rotor voltage in p.u., e.g. 1.2",
			PT_double, "Min_Vrotor[pu*V]", PADDR(Min_Vrotor), PT_DESCRIPTION, "COP: Induction generator minimum induced rotor voltage in p.u., e.g. 0.8",

			PT_double, "Rs[pu*Ohm]", PADDR(Rs), PT_DESCRIPTION, "COP: Synchronous generator primary stator resistance in p.u.",
			PT_double, "Xs[pu*Ohm]", PADDR(Xs), PT_DESCRIPTION, "COP: Synchronous generator primary stator reactance in p.u.",
			PT_double, "Rg[pu*Ohm]", PADDR(Rg), PT_DESCRIPTION, "COP: Synchronous generator grounding resistance in p.u.",
			PT_double, "Xg[pu*Ohm]", PADDR(Xg), PT_DESCRIPTION, "COP: Synchronous generator grounding reactance in p.u.",
			PT_double, "Max_Ef[pu*V]", PADDR(Max_Ef), PT_DESCRIPTION, "COP: Synchronous generator maximum induced rotor voltage in p.u., e.g. 0.8",
			PT_double, "Min_Ef[pu*V]", PADDR(Min_Ef), PT_DESCRIPTION, "COP: Synchronous generator minimum induced rotor voltage in p.u., e.g. 0.8",

			PT_double, "pf[pu]", PADDR(pf), PT_DESCRIPTION, "COP: Desired power factor in CONSTANTP mode (can be modified over time)",
			PT_double, "power_factor[pu]", PADDR(pf), PT_DESCRIPTION, "COP: Desired power factor in CONSTANTP mode (can be modified over time)",

			PT_char1024, "power_curve_csv", PADDR(power_curve_csv), PT_DESCRIPTION, "Name of .csv file containing user defined power curve",
			PT_bool, "power_curve_pu", PADDR(power_curve_pu), PT_DESCRIPTION, "Flag when set indicates that user provided power curve has power values in p.u. Defaults to false",

			PT_complex, "voltage_A[V]", PADDR(voltage_A),PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Terminal voltage on phase A",
			PT_complex, "voltage_B[V]", PADDR(voltage_B),PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Terminal voltage on phase B",
			PT_complex, "voltage_C[V]", PADDR(voltage_C),PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Terminal voltage on phase C",
			PT_complex, "voltage_12[V]", PADDR(voltage_A),PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Triplex mode - Voltage between line 1 and line 2",
			PT_complex, "voltage_1N[V]", PADDR(voltage_B),PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Triplex mode - Voltage between line 1 and the neutral N",
			PT_complex, "voltage_2N[V]", PADDR(voltage_C),PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Triplex mode - Voltage between line 2 and the neutral N",
			PT_complex, "current_A[A]", PADDR(current_A), PT_DESCRIPTION, "Calculated terminal current on phase A",
			PT_complex, "current_B[A]", PADDR(current_B), PT_DESCRIPTION, "Calculated terminal current on phase B",
			PT_complex, "current_C[A]", PADDR(current_C), PT_DESCRIPTION, "Calculated terminal current on phase C",
			PT_complex, "current_12[A]", PADDR(current_A), PT_DESCRIPTION, "Triplex mode - Calculated line current 12",
			PT_complex, "EfA[V]", PADDR(EfA), PT_DESCRIPTION, "COP: Synchronous generator induced voltage on phase A",
			PT_complex, "EfB[V]", PADDR(EfB), PT_DESCRIPTION, "COP: Synchronous generator induced voltage on phase B",
			PT_complex, "EfC[V]", PADDR(EfC), PT_DESCRIPTION, "COP: Synchronous generator induced voltage on phase C",
			PT_complex, "Vrotor_A[V]", PADDR(Vrotor_A), PT_DESCRIPTION, "COP: Induction generator induced voltage on phase A in p.u.",//Induction Generator
			PT_complex, "Vrotor_B[V]", PADDR(Vrotor_B), PT_DESCRIPTION, "COP: Induction generator induced voltage on phase B in p.u.",
			PT_complex, "Vrotor_C[V]", PADDR(Vrotor_C), PT_DESCRIPTION, "COP: Induction generator induced voltage on phase C in p.u.",
			PT_complex, "Irotor_A[V]", PADDR(Irotor_A), PT_DESCRIPTION, "COP: Induction generator induced current on phase A in p.u.",
			PT_complex, "Irotor_B[V]", PADDR(Irotor_B), PT_DESCRIPTION, "COP: Induction generator induced current on phase B in p.u.",
			PT_complex, "Irotor_C[V]", PADDR(Irotor_C), PT_DESCRIPTION, "COP: Induction generator induced current on phase C in p.u.",

			PT_double, "internal_model_current_convergence[pu]", PADDR(internal_model_current_convergence), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Convergence value for internal current calculations",

			PT_set, "phases", PADDR(phases), PT_DESCRIPTION, "Specifies which phases to connect to - currently triplex mode is only supported for power curve implementation",
			PT_KEYWORD, "A",(set)PHASE_A,
			PT_KEYWORD, "B",(set)PHASE_B,
			PT_KEYWORD, "C",(set)PHASE_C,
			PT_KEYWORD, "N",(set)PHASE_N,
			PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
            if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)windturb_dg_NR_current_injection_update) == nullptr)
				GL_THROW("Unable to publish wind turbine current injection update function");
	}
}

/* Object creation is called once for each object that is created by the core */
int windturb_dg::create(void) 
{

	// Defaults usually inside the create function

	roughness_l = .055;		//European wind atlas def for terrain roughness, values range from 0.0002 to 1.6
	ref_height = 10;		//height wind data was measured (Most meteorological data taken at 5-15 m)
	Max_Vrotor = 1.2;
	Min_Vrotor = 0.8;
	Max_Ef = 1.2;
	Min_Ef = 0.8;
	avg_ws = 8;				    //This is wind speed in m/s at reference height (ref_height=10m)
	wind_speed_hub_ht = 10;    //This is wind speed at 37 m hub height
	time_advance = 3600;	//amount of time to advance for WS data import in secs.

	/* set the default values of all properties here */
	Ridealgas = 8.31447;
	Molar = 0.0289644;
	//std_air_dens = 1.2754;	//dry air density at std pressure and temp in kg/m^3
	std_air_temp = 0;	//IUPAC std air temp in Celsius
	std_air_press = 100000;	//IUPAC std air pressure in Pascals
	Turbine_Model = GENERIC_DEFAULT;	//************** Defaults specified so it doesn't crash out

	Gen_mode = CONSTANTP;					//************** Default specified so values actually come out
	Gen_status = ONLINE;

	Turbine_implementation = POWER_CURVE;
	Wind_speed_source = DEFAULT;

	power_curve_pu = false;

	turbine_height = -9999;
	Rated_VA = -9999;
	blade_diam = -9999;
	cut_in_ws = -9999;
	cut_out_ws = -9999;
	Cp_max = -9999;
	Cp_rated =-9999;
	ws_maxcp = -9999;
	ws_rated = -9999;
	CP_Data = CALCULATED;

	strcpy(power_curve_csv,"");      //initializing string to null

	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = nullptr;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = nullptr;
	pLine12 = nullptr;
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0,0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = gld::complex(0.0,0.0);
	value_Line12 = gld::complex(0.0,0.0);

	parent_is_valid = false;
	parent_is_triplex = false;
	parent_is_inverter = false;
	
	inverter_power_property = nullptr;
	inverter_flag_property = nullptr;


    pPress = nullptr;
	pTemp = nullptr;
	pWS = nullptr;
    value_Press = 0.0;
	value_Temp = 0.0;
	value_WS = 0.0;
	climate_is_valid = false;

	//Current injection tracking variables
	prev_current[0] = prev_current[1] = prev_current[2] = complex(0.0,0.0);
	NR_first_run = false;

	prev_current12 = complex(0.0,0.0);

	//Set default convergence
	internal_model_current_convergence = 0.005;

	return 1; /* return 1 on success, 0 on failure */
}

/* Checks for climate object and maps the climate variables to the windturb object variables.  
If no climate object is linked, then default pressure, temperature, and wind speed will be used. */
int windturb_dg::init_climate()
{
	OBJECT *hdr = OBJECTHDR(this);

	// link to climate data
	FINDLIST *climates = nullptr;

	climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
	if (climates==nullptr)
	{
		//Flag as not found
		climate_is_valid = false;

		gl_warning("windturb_dg (id:%d)::init_climate(): no climate data found, using static data",hdr->id);

		//default to mock data
		value_WS = avg_ws;
		value_Press = std_air_press;
		value_Temp = std_air_temp;
	}
	else if (climates->hit_count>1)
	{
		gl_verbose("windturb_dg: %d climates found, using first one defined", climates->hit_count);
	}
	// }
	if (climates!=nullptr)
	{
		if (climates->hit_count==0)
		{
			//Flag as false
			climate_is_valid = false;

			//default to mock data
			gl_warning("windturb_dg (id:%d)::init_climate(): no climate data found, using static data",hdr->id);

			//default to mock data
			value_WS = avg_ws;
			value_Press = std_air_press;
			value_Temp = std_air_temp;
		}
		else //climate data was found
		{
			// force rank of object w.r.t climate
			OBJECT *obj = gl_find_next(climates,NULL);
			if (obj->rank<=hdr->rank)
				gl_set_dependent(obj,hdr);

			pWS = map_double_value(obj,"wind_speed");
			pPress = map_double_value(obj,"pressure");
			pTemp = map_double_value(obj,"temperature");

			//Flag properly
			climate_is_valid = true;
		}
	}
	if (Wind_speed_source == DEFAULT){
		if (climate_is_valid){
			Wind_speed_source = CLIMATE_DATA;
		}else{
			Wind_speed_source = BUILT_IN;
		}
	}
	return 1;
}

int windturb_dg::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	double temp_double_value;
	gld_property *temp_property_pointer;
	enumeration temp_enum;
	set temp_phases_set;
	int temp_phases=0;
		
	double ZB, SB, EB;
	gld::complex tst, tst2, tst3, tst4;

	switch (Turbine_Model)	{
		case GENERIC_IND_LARGE:
		case GENERIC_SYNCH_LARGE:	//Creates generic 1.5 MW wind turbine.
			blade_diam = 82.5;
			turbine_height = 90;
			q = 3;						//number of gearbox stages
			Rated_VA = 1635000;
			Max_P = 1500000;
			Max_Q = 650000;
			Rated_V = 600;
			pf = 0.95;
			CP_Data = GENERAL_LARGE;
			cut_in_ws = 4;			//lowest wind speed 
			cut_out_ws = 25;		//highest wind speed 
			Cp_max = 0.302;			//rotor specifications for power curve
			ws_maxcp = 7;			
			Cp_rated = Cp_max-.05;	
			ws_rated = 12.5;
			if (Turbine_Model == GENERIC_IND_LARGE) {
				Gen_type = INDUCTION;
				Rst = 0.12;					
				Xst = 0.17;					
				Rr = 0.12;				
				Xr = 0.15;			
				Rc = 999999;		
				Xm = 9.0;	
			}
			else if (Turbine_Model == GENERIC_SYNCH_LARGE) {
				Gen_type = SYNCHRONOUS;
				Rs = 0.05;
				Xs = 0.200;
				Rg = 0.000;
				Xg = 0.000;
			}
			if (Turbine_implementation == POWER_CURVE){
				Turbine_implementation = COEFF_OF_PERFORMANCE;
				gl_warning("windturb_dg (id:%d, name:%s): GENERIC_SYNCH_LARGE/GENERIC_IND_LARGE is not compatible with the power curve implementation. Switching to legacy coefficient of performance implementation.", obj->id,obj->name);
			}
			break;
		case GENERIC_IND_MID:
		case GENERIC_SYNCH_MID:	//Creates generic 100kW wind turbine, northwind 100
			blade_diam = 23.2;   //in m
			turbine_height = 30;   //in m
			q = 0;						//number of gearbox stages, no gear box 
			Rated_VA = 156604;
			Max_P = 150000;
			Max_Q = 45000;
			Rated_V = 480;  
			pf = 0.9;     ///lag and lead of 0.9
			CP_Data = GENERAL_MID;
			cut_in_ws = 3.5;			//lowest wind speed in m/s
			cut_out_ws = 25;		   //highest wind speed in m/s
			Cp_max = 0.302;			  //rotor specifications for power curve
			ws_maxcp = 7;			
			Cp_rated = Cp_max-.05;	
			ws_rated = 14.5;		//	in m/s
			if (Turbine_Model == GENERIC_IND_MID) {     // need to check the machine parameters
				Gen_type = INDUCTION;
				Rst = 0.12;					
				Xst = 0.17;					
				Rr = 0.12;				
				Xr = 0.15;			
				Rc = 999999;		
				Xm = 9.0;	
			}
			else if (Turbine_Model == GENERIC_SYNCH_MID) {
				Gen_type = SYNCHRONOUS;
				Rs = 0.05;
				Xs = 0.200;
				Rg = 0.000;
				Xg = 0.000;
			}
			if (Turbine_implementation == POWER_CURVE){
				Turbine_implementation = COEFF_OF_PERFORMANCE;
				gl_warning("windturb_dg (id:%d, name:%s): GENERIC_SYNCH_MID/GENERIC_IND_MID is not compatible with the power curve implementation. Switching to legacy coefficient of performance implementation.", obj->id,obj->name);
			}
			break;
		case GENERIC_IND_SMALL:					
		case GENERIC_SYNCH_SMALL:	//Creates generic 5 kW wind turbine, Fortis Montana 5 kW wind turbine
			blade_diam = 5;      // in m
			turbine_height = 16;   //in m
			q = 0;						//number of gearbox stages, no gear box
			Rated_VA = 6315;               // calculate from P & Q
			Max_P = 5800; 
			Max_Q = 2500;
			Rated_V = 600;
			pf = 0.95;
			CP_Data = GENERAL_SMALL;
			cut_in_ws = 2.5;			//lowest wind speed 
			cut_out_ws = 25;		//highest wind speed 
			Cp_max = 0.302;			//rotor specifications for power curve
			ws_maxcp = 7;			//	|
			Cp_rated = Cp_max-.05;	//	|
			ws_rated = 17;		//	|
			if (Turbine_Model == GENERIC_IND_SMALL) {
				Gen_type = INDUCTION;
				Rst = 0.12;					
				Xst = 0.17;					
				Rr = 0.12;				
				Xr = 0.15;			
				Rc = 999999;		
				Xm = 9.0;	
			}
			else if (Turbine_Model == GENERIC_SYNCH_SMALL) {
				Gen_type = SYNCHRONOUS;
				Rs = 0.05;
				Xs = 0.200;
				Rg = 0.000;
				Xg = 0.000;
			}
			if (Turbine_implementation == POWER_CURVE){
				Turbine_implementation = COEFF_OF_PERFORMANCE;
				gl_warning("windturb_dg (id:%d, name:%s): GENERIC_SYNCH_SMALL/GENERIC_IND_SMALL is not compatible with the power curve implementation. Switching to legacy coefficient of performance implementation.", obj->id,obj->name);
			}
			break;
		case VESTAS_V82:	//Include manufacturer's data - cases can be added to call other wind turbines
			turbine_height = 78;
			blade_diam = 82;
			Rated_VA = 1808000;
			Rated_V = 600;
			Max_P = 1650000;
			Max_Q = 740000;
			pf = 0.91;		//Can range between 0.65-1.00 depending on controllers and Pout.
			CP_Data = MANUF_TABLE;		
			cut_in_ws = 3.5;
			cut_out_ws = 20;
			q = 2;
			Gen_type = SYNCHRONOUS;	//V82 actually uses a DFIG, but will use synch representation for now
			Rs = 0.025;				//Estimated values for synch representation.
			Xs = 0.200;
			Rg = 0.000;
			Xg = 0.000;
			if (Turbine_implementation == POWER_CURVE){
				Turbine_implementation = COEFF_OF_PERFORMANCE;
				gl_warning("windturb_dg (id:%d, name:%s): VESTAS_V82 is not compatible with the power curve implementation. Switching to legacy coefficient of performance implementation.", obj->id,obj->name);
			}
			break;
		case GE_25MW:
			turbine_height = 100;
			blade_diam = 100;
			Rated_VA = 2727000;
			Rated_V = 690;
			Max_P = 2500000;
			Max_Q = 1090000;
			pf = 0.95;		//ranges between -0.9 -> 0.9;
			q = 3;
			CP_Data = GENERAL_LARGE;
			cut_in_ws = 3.5;
			cut_out_ws = 25;
			Cp_max = 0.28;
			Cp_rated = 0.275;
			ws_maxcp = 8.2;
			ws_rated = 12.5;
			Gen_type = SYNCHRONOUS;
			Rs = 0.035;
			Xs = 0.200;
			Rg = 0.000;
			Xg = 0.000;
			if (Turbine_implementation == POWER_CURVE){
				Turbine_implementation = COEFF_OF_PERFORMANCE;
				gl_warning("windturb_dg (id:%d, name:%s): GE_25MW is not compatible with the power curve implementation. Switching to legacy coefficient of performance implementation.", obj->id,obj->name);
			}
			break;
		case BERGEY_10kW:
			turbine_height = 24;
			blade_diam = 7;
			Rated_VA = 10000;
			Rated_V = 360;
			Max_P = 15000;
			Max_Q = 4000;
			pf = 0.95;		//ranges between -0.9 -> 0.9;
			q = 0;
			CP_Data = GENERAL_SMALL;
			cut_in_ws = 2;
			cut_out_ws = 20;
			Cp_max = 0.28;
			Cp_rated = 0.275;
			ws_maxcp = 8.2;
			ws_rated = 12.5;
			Gen_type = SYNCHRONOUS;
			Rs = 0.05;
			Xs = 0.200;
			Rg = 0.000;
			Xg = 0.000;
			if (Turbine_implementation == POWER_CURVE){
				Turbine_implementation = COEFF_OF_PERFORMANCE;
				gl_warning("windturb_dg (id:%d, name:%s): BERGEY_10kW is not compatible with the power curve implementation. Switching to legacy coefficient of performance implementation.", obj->id,obj->name);
			}
			break;
		case USER_DEFINED:

			CP_Data = USER_SPECIFY;	

			Gen_type = USER_TYPE;
			Rs = 0.2;
			Xs = 0.2;
			Rg = 0.1;
			Xg = 0;

			if (turbine_height <=0)
				GL_THROW ("turbine height cannot have a negative or zero value.");
			/*  TROUBLESHOOT
			Turbine height must be specified as a value greater than or equal to zero.
			*/
			if (blade_diam <=0)
				GL_THROW ("blade diameter cannot have a negative or zero value.");
			/*  TROUBLESHOOT
			Blade diameter must be specified as a value greater than or equal to zero.
			*/
			if (cut_in_ws <=0)
				GL_THROW ("cut in wind speed cannot have a negative or zero value.");
			/*  TROUBLESHOOT
			Cut in wind speed must be specified as a value greater than or equal to zero.
			*/
			if (cut_out_ws <=0)
				GL_THROW ("cut out wind speed cannot have a negative or zero value.");
			/*  TROUBLESHOOT
			Cut out wind speed must be specified as a value greater than or equal to zero.
			*/
			if (ws_rated <=0)
				GL_THROW ("rated wind speed cannot have a negative or zero value.");
			/*  TROUBLESHOOT
			Rated wind speed must be specified as a value greater than or equal to zero.
			*/
			if (ws_maxcp <=0)
				GL_THROW ("max cp cannot have a negative or zero value.");
			/*  TROUBLESHOOT
			Maximum coefficient of performance must be specified as a value greater than or equal to zero.
			*/
			break;
		case GEN_TURB_POW_CURVE_2_4KW:             //2.4 kW generic turbine
			turbine_height = 21;
			Rated_VA = 2400;

			break;
		case GEN_TURB_POW_CURVE_10KW:              //10 kW generic turbine
			turbine_height = 37;
			Rated_VA = 10000;

			break;
		case GEN_TURB_POW_CURVE_100KW:             //100 kW generic turbine
			turbine_height = 37;
			Rated_VA = 100000;

			break;
		case GEN_TURB_POW_CURVE_1_5MW:             //1.5 MW generic turbine
			turbine_height = 80;
			Rated_VA = 1500000;

			break;
		case GENERIC_DEFAULT:
			//Default turbine parameters used when user specifies no generic turbine or turbine parameters
			blade_diam = 22;
			q = 3;						//number of gearbox stages
			Max_P = 90000;
			Max_Q = 40000;
			Rated_V = 600;
			pf = 0.9;
			CP_Data = GENERAL_MID;
			cut_in_ws = 3.5;			//lowest wind speed
			cut_out_ws = 25;		//highest wind speed
			Cp_max = 0.302;			//rotor specifications for power curve
			ws_maxcp = 7;
			Cp_rated = Cp_max-.05;
			ws_rated = 12.5;
			Gen_type = SYNCHRONOUS;
			Rs = 0.05;
			Xs = 0.200;
			Rg = 0.000;
			Xg = 0.000;                           //*******************Defaults specified


			//Basic checks on height and capacity here
			if (turbine_height > 400){
				gl_warning("windturb_dg (id:%d, name:%s): Specified turbine height of %.1f m is greater than 400 m and may be unrealistically tall.", obj->id,obj->name, turbine_height);
			}
			if (Rated_VA > 25000000){
				gl_warning("windturb_dg (id:%d, name:%s): Specified turbine capacity of %.1f VA is greater than 25 MW and may be unrealistically large.", obj->id,obj->name, Rated_VA);
			}

			if ((turbine_height <= 0) && (turbine_height != -9999)) {      //Checks for user-specified turbine height
				GL_THROW ("Turbine height must have a positive value.");
			}
			if ((Rated_VA <= 0) && (Rated_VA != -9999)){                   //Checks for user-specified turbine capacity
				GL_THROW ("Turbine capacity must have a positive value.");
			}

			//Estimating turbine capacity/height if only turbine height/capacity is specified. Uses capacity-height clusters. While estimating, upper limit of height is 400 m, upper limit of capacity is 25 MW
			if ((turbine_height != -9999) && (Rated_VA == -9999)){                 // Case when only turbine height is specified by the user
				gl_warning("windturb_dg (id:%d, name:%s): Turbine capacity is unspecified. Approximating turbine capacity from the height provided.", obj->id,obj->name);
				if ((turbine_height > 0) && (turbine_height < 25)){
					Rated_VA = 2400;
				} else if ((turbine_height >= 25) && (turbine_height < 45)){
					Rated_VA = 10000;
				} else if ((turbine_height >= 45)){
					Rated_VA = (exp((turbine_height + 104.402)/24.8885))*1000;
					if (Rated_VA > 25000000){
						Rated_VA = 25000000;
						gl_warning("windturb_dg (id:%d, name:%s): Turbine capacity is set to the inferred-capacity limit.", obj->id,obj->name);
					}
				} else {
					GL_THROW("windturb_dg Invalid input %f",turbine_height);
				}
				gl_warning("windturb_dg (id:%d, name:%s): Approximated turbine capacity is: %.1f VA.", obj->id,obj->name, Rated_VA);

			} else if ((turbine_height == -9999) && (Rated_VA != -9999)){          // Case when only turbine capacity is specified by the user
				gl_warning("windturb_dg (id:%d, name:%s): Turbine height is unspecified. Approximating Turbine height from the capacity provided.", obj->id,obj->name);
				if ((Rated_VA > 0) && (Rated_VA < 4500)){
					turbine_height = 21;
				} else if ((Rated_VA >= 4500) && (Rated_VA < 300000)){
					turbine_height = 37;
				} else if ((Rated_VA >= 300000)){
					turbine_height = 24.8885*log(Rated_VA/1000) - 104.402;
					if (turbine_height > 400){
						turbine_height = 400;
						gl_warning("windturb_dg (id:%d, name:%s): Turbine height is set to the inferred-height limit.", obj->id,obj->name);
					}
				} else {
					GL_THROW("windturb_dg Invalid input %f",Rated_VA);
				}
				gl_warning("windturb_dg (id:%d, name:%s): Approximated turbine height is: %.1f m.", obj->id,obj->name, turbine_height);

			} else if ((turbine_height == -9999) && (Rated_VA == -9999)) {        // Case when none of capacity or height is specified by the user
				turbine_height = 37;    //Setting to defaults i.e. 100 kW turbine capacity and height
				Rated_VA = 100000;
				gl_warning("windturb_dg (id:%d, name:%s): Turbine capacity and height not specified. Using default turbine capacity of %.1f VA and default turbine height of %.1f m.", obj->id,obj->name, Rated_VA, turbine_height);
			} else {                                                              //Case when both capacity and height are specified by user. Check for incompatibility and issue warnings.
				if ((Rated_VA > 0) && (Rated_VA < 4500)){
					if ((turbine_height < 4) || (turbine_height > 41)){
						gl_warning("windturb_dg (id:%d, name:%s): Potential mismatch of turbine capacity %.1f VA and turbine height %.1f m.", obj->id,obj->name, Rated_VA, turbine_height);
					}
				} else if ((Rated_VA >= 4500) && (Rated_VA < 20000)){
					if ((turbine_height < 12) || (turbine_height > 49)){
						gl_warning("windturb_dg (id:%d, name:%s): Potential mismatch of turbine capacity %.1f VA and turbine height %.1f m.", obj->id,obj->name, Rated_VA, turbine_height);
					}
				} else if ((Rated_VA >= 20000) && (Rated_VA < 300000)){
					if ((turbine_height < 18) || (turbine_height > 55)){
						gl_warning("windturb_dg (id:%d, name:%s): Potential mismatch of turbine capacity %.1f VA and turbine height %.1f m.", obj->id,obj->name, Rated_VA, turbine_height);
					}
				} else {
					if ((turbine_height < ((24.8885*log(Rated_VA/1000) - 104.402) - 20)) || (turbine_height > ((24.8885*log(Rated_VA/1000) - 104.402) + 11))){
						gl_warning("windturb_dg (id:%d, name:%s): Potential mismatch of turbine capacity %.1f VA and turbine height %.1f m.", obj->id,obj->name, Rated_VA, turbine_height);
					}
				}
			}
			break;

		default:
			GL_THROW("Unknown turbine model was specified");
			/*  TROUBLESHOOT
			An unknown wind turbine model was selected.  Please select a Turbine_Model from the available list.
			*/
	}


	for (int i=0;i < (sizeof (Power_Curve[0]) /sizeof (double));i++) {
		Power_Curve[0][i] = -1;
		Power_Curve[1][i] = -1;
	}

	double Power_Curve_default[2][21] = {                           //Default power curve
		{2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5, 6.0, 6.5, 7.0, 7.5, 8.0, 8.5, 9.0, 9.5, 10.0, 10.5, 11.0, 11.5, 12.0, 25.0},
		{0.0, 0.0084725, 0.026315, 0.049903, 0.083323, 0.12564, 0.17541, 0.23191, 0.30286, 0.38807, 0.49134, 0.58717, 0.66827, 0.75071, 0.82431, 0.89813, 0.95368, 1.0013, 1.0331, 1.0602, 1.0602}
	};

	if (Turbine_implementation == POWER_CURVE){

        //read power curve .csv file if provided. Doing sanity checks on data format
		if (!(hasEnding(power_curve_csv, ".csv"))){
			// No valid .csv file name - Using default power curve
			if (strcmp(power_curve_csv,"")==0) {
				// .csv file name not specified - Issue warning
				gl_warning("windturb_dg (id:%d, name:%s): No user defined power curve provided. Resorting to default power curve.", obj->id,obj->name);
			} else{
				// Invalid .csv file name specified - Issue warning
				gl_warning("windturb_dg (id:%d, name:%s): windturb_dg: Unrecognized file type. Resorting to default power curve.", obj->id,obj->name);
			}

			for (int i=0; i<sizeof(Power_Curve_default[0])/sizeof(double); i++){
				Power_Curve[0][i] = Power_Curve_default[0][i];
				Power_Curve[1][i] = Power_Curve_default[1][i];
			}
		} else {
			// Valid .csv file name found - Attempting to load power curve data from .csv file
			std::ifstream file(power_curve_csv);

			std::vector<std::vector<std::string>> csvData;

			csvData = readCSV(file);

			if (csvData.size() == 0){
				// Empty .csv file - Issue an error
				GL_THROW("Unable to read power curve data from .csv file.");
			}

			if (csvData.size() < 3){
				// Too few points in .csv file - Issue an error
				GL_THROW("Invalid data format. Provide at least 3 data points.");
			}

			if (csvData.size() > (sizeof(Power_Curve[0])/sizeof(double))){
				// Too many points in .csv file - Issue an error
				GL_THROW ("Invalid data format. Provide up to %lu points.", (sizeof(Power_Curve[0])/sizeof(double)));
			}
			for ( size_t i = 0; i < csvData.size(); i++ )
			{
				if (csvData[i].size() != 2){
					// Too few cloumns in .csv file row - Issue an error
					GL_THROW ("Invalid data format. More than 2 columns in row %i of the .csv file", i+1);
				}

				// Copying wind speed data from data object to power curve object
				try
				{
					Power_Curve[0][i] = std::stod(csvData[i][0]);
				}
				catch(...)
				{
					GL_THROW("Invalid entry in row %i and column %i of the .csv file. Please check .csv file.", i+1, 1);
				}

				// Copying turbine power data from data object to power curve object
				try
				{
					Power_Curve[1][i] = std::stod(csvData[i][1]);
				}
				catch(...)
				{
					GL_THROW("Invalid entry in row %i and column %i of the .csv file. Please check .csv file.", i+1, 2);
				}
			}

		}

		int valid_entries = 0;
		for (int i=0;i < (sizeof (Power_Curve[0]) /sizeof (double));i++) {
			if ((Power_Curve[0][i] != -1) && (Power_Curve[1][i] != -1)){
				valid_entries ++;
			}
		}
		number_of_points = valid_entries;
	}

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=nullptr)
	{
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("windturb_dg::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
		if (gl_object_isa(parent,"meter","powerflow"))	//Attach to meter
		{
			//Flag properly
			parent_is_valid = true;
			parent_is_triplex = false;
			parent_is_inverter = false;

			//Map the solver method and see if we're NR-oriented
			temp_property_pointer = new gld_property("powerflow::solver_method");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("windturb_dg:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the powerflow:solver_method property, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			//Must be valid, read it
			temp_enum = temp_property_pointer->get_enumeration();

			//Remove the link
			delete temp_property_pointer;

			//Check which method we are - NR=2
			if (temp_enum == 2)
			{
				//Set NR first run flag
				NR_first_run = true;
			}
			else	//Other methods - should already be set, but be explicit
			{
				NR_first_run = false;
			}

			//Map phases
			//Map and pull the phases
			temp_property_pointer = new gld_property(parent,"phases");

			//Make sure ti worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_set())
			{
				GL_THROW("Unable to map phases property - ensure the parent is a meter or a node or a load");
				/*  TROUBLESHOOT
				While attempting to map the phases property from the parent object, an error was encountered.
				Please check and make sure your parent object is a meter or triplex_meter inside the powerflow module and try
				again.  If the error persists, please submit your code and a bug report via the Trac website.
				*/
			}

			//Pull the phase information
			temp_phases_set = temp_property_pointer->get_set();

			//Clear the temporary pointer
			delete temp_property_pointer;

			// Currently only supports 3-phase connection, so check number of phases of parent
			if ((temp_phases_set & PHASE_A) == PHASE_A)
				temp_phases += 1;
			if ((temp_phases_set & PHASE_B) == PHASE_B)
				temp_phases += 1;
			if ((temp_phases_set & PHASE_C) == PHASE_C)
				temp_phases += 1;

			if (temp_phases < 3){
				GL_THROW("The wind turbine model currently only supports a 3-phase connection, please check meter connection");
				/*  TROUBLESHOOT
				Currently the wind turbine model only supports 3-phase connnections. Please attach to 3-phase meter.
				*/
			} else {        // three phase
				number_of_phases_out = 3;
			}

			//Map the voltages
			temp_property_pointer = new gld_property(parent,"nominal_voltage");

			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("Unable to map nominal_voltage property - ensure the parent is a powerflow:meter");
				/*  TROUBLESHOOT
				While attempting to map the nominal_voltage property from the parent object, an error was encountered.
				Please check and make sure your parent object is a meter inside the powerflow module and try
				again.  If the error persists, please submit your code and a bug report via the Trac website.
				*/
			}

			//Pull value
			temp_double_value = temp_property_pointer->get_double();

			//Remove the pointer
			delete temp_property_pointer;

			// check nominal voltage against rated voltage
			if (( fabs(1 - (temp_double_value * sqrt(3.0) / Rated_V) ) > 0.1 ) && (Turbine_implementation != POWER_CURVE))
				gl_warning("windturb_dg (id:%d, name:%s): Rated generator voltage (LL: %.1f) and nominal voltage (LL: %.1f) of meter parent are different by greater than 10 percent. Odd behavior may occur.",obj->id,obj->name,Rated_V,temp_double_value * sqrt(3.0));
			/* TROUBLESHOOT
			Currently, the model allows you to attach the turbine to a voltage that is quite different than the rated terminal
			voltage of the generator.  However, this may cause odd behavior, as the solved powerflow voltage is used to calculate
			the generator induced voltages and conversion from mechanical power.  It is recommended that the nominal
			voltages of the parent meter be within ~10% of the rated voltage.
			*/

			//Map the values of the parent
			pCircuit_V[0] = map_complex_value(parent,"voltage_A");
			pCircuit_V[1] = map_complex_value(parent,"voltage_B");
			pCircuit_V[2] = map_complex_value(parent,"voltage_C");

			//If we're NR, map a different field to avoid the rotation
			if (NR_first_run)
			{
				pLine_I[0] = map_complex_value(parent,"prerotated_current_A");
				pLine_I[1] = map_complex_value(parent,"prerotated_current_B");
				pLine_I[2] = map_complex_value(parent,"prerotated_current_C");
			}
			else
			{
				pLine_I[0] = map_complex_value(parent,"current_A");
				pLine_I[1] = map_complex_value(parent,"current_B");
				pLine_I[2] = map_complex_value(parent,"current_C");
			}

			//Null the triplex interface point, just in case
			pLine12 = nullptr;
		}
		else if (gl_object_isa(parent,"triplex_meter","powerflow"))
		{
			//Map the solver method and see if we're NR-oriented
			temp_property_pointer = new gld_property("powerflow::solver_method");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("windturb_dg:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the powerflow:solver_method property, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			//Must be valid, read it
			temp_enum = temp_property_pointer->get_enumeration();

			//Remove the link
			delete temp_property_pointer;

			//Check which method we are - NR=2
			if (temp_enum == 2)
			{
				//Set NR first run flag
				NR_first_run = true;
			}
			else	//Other methods - should already be set, but be explicit
			{
				NR_first_run = false;
			}

			if (Turbine_implementation == COEFF_OF_PERFORMANCE){
				GL_THROW("windturb_dg (id:%d): 3-phase coefficient of performance implementation uses a 3-phase generator and cannot be connected to a triplex meter",obj->id);
			}

			//Set flags
			parent_is_valid = true;
			parent_is_triplex = true;
			parent_is_inverter = false;

			//Map the variables
			pCircuit_V[0] = map_complex_value(parent,"voltage_12");
			pCircuit_V[1] = map_complex_value(parent,"voltage_1N");
			pCircuit_V[2] = map_complex_value(parent,"voltage_2N");

			//If we're NR, map a different field to avoid the rotation
			if (NR_first_run)
			{
				pLine_I[0] = map_complex_value(parent,"prerotated_current_1");
				pLine_I[1] = map_complex_value(parent,"prerotated_current_2");
				pLine_I[2] = map_complex_value(parent,"current_N");

				pLine12 = map_complex_value(parent,"prerotated_current_12");
			}
			else
			{
				pLine_I[0] = map_complex_value(parent,"current_1");
				pLine_I[1] = map_complex_value(parent,"current_2");
				pLine_I[2] = map_complex_value(parent,"current_N");
				pLine12 = map_complex_value(parent,"current_12");
			}

			//pPower = map_complex_value(parent,"measured_power");

			//Map and pull the phases
			temp_property_pointer = new gld_property(parent,"phases");

			//Make sure ti worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_set())
			{
				GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
				//Defined above
			}

			//Pull the phase information
			temp_phases_set = temp_property_pointer->get_set();

			//Clear the temporary pointer
			delete temp_property_pointer;

			number_of_phases_out = 1;

		}
		else if (gl_object_isa(parent,"rectifier","generators"))
		{
			//Set the "pull flag"
			parent_is_valid = true;
			parent_is_triplex = false;
			parent_is_inverter = false;
			
			number_of_phases_out = 3;

			//Map the voltages
			temp_property_pointer = new gld_property(parent,"V_Rated");

			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("Unable to map V_Rated property - ensure the parent is a powerflow:meter");
				/*  TROUBLESHOOT
				While attempting to map the nominal_voltage property from the parent object, an error was encountered.
				Please check and make sure your parent object is a meter inside the powerflow module and try
				again.  If the error persists, please submit your code and a bug report via the Trac website.
				*/
			}

			//Pull the value
			temp_double_value = temp_property_pointer->get_double();

			//Remove the pointer
			delete temp_property_pointer;

			// check nominal voltage against rated voltage
			if (( fabs(1 - (temp_double_value / Rated_V) ) > 0.1 ) && (Turbine_implementation != POWER_CURVE))
				gl_warning("windturb_dg (id:%d, name:%s): Rated generator voltage (LL: %.1f) and nominal voltage (LL: %.1f) of meter parent are different by greater than 10 percent. Odd behavior may occur.",obj->id,obj->name,Rated_V,temp_double_value * sqrt(3.0));
			/* TROUBLESHOOT
			Currently, the model allows you to attach the turbine to a voltage that is quite different than the rated terminal
			voltage of the generator.  However, this may cause odd behavior, as the solved powerflow voltage is used to calculate
			the generator induced voltages and conversion from mechanical power.  It is recommended that the nominal
			voltages of the parent meter be within ~10% of the rated voltage.
			*/

			//Map the values of the parent
			pCircuit_V[0] = map_complex_value(parent,"voltage_A");
			pCircuit_V[1] = map_complex_value(parent,"voltage_B");
			pCircuit_V[2] = map_complex_value(parent,"voltage_C");

			pLine_I[0] = map_complex_value(parent,"current_A");
			pLine_I[1] = map_complex_value(parent,"current_B");
			pLine_I[2] = map_complex_value(parent,"current_C");
		}
		else if (gl_object_isa(parent,"inverter","generators"))
		{
			//if wind turbine implementation is not power curve-based, throw an error
			if (Turbine_implementation != POWER_CURVE){
				GL_THROW("windturb_dg (id:%d): Inverter parent is only supported for power curve-based wind turbine implementation",obj->id);
			}
			//Set flags
			parent_is_valid = true;
			parent_is_triplex = false;
			parent_is_inverter = true;
			
			//Map the solver method and see if we're NR-oriented
			temp_property_pointer = new gld_property("powerflow::solver_method");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("windturb_dg:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the powerflow:solver_method property, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			//Must be valid, read it
			temp_enum = temp_property_pointer->get_enumeration();

			//Remove the link
			delete temp_property_pointer;

			//Check which method we are - NR=2
			if (temp_enum == 2)
			{
				//Set NR first run flag
				NR_first_run = true;
			}
			else	//Other methods - should already be set, but be explicit
			{
				NR_first_run = false;
			}
			
			//Map the flag that indicates that this wind turbine is the child of the upstream inverter
			inverter_flag_property = new gld_property(parent, "WT_is_connected");

			//Check it
			if (!inverter_flag_property->is_valid() || !inverter_flag_property->is_bool())
			{
				GL_THROW("wind turbine:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
			
			//Map the inverter power value
			inverter_power_property = new gld_property(parent, "VA_Out");

			//Check it
			if (!inverter_power_property->is_valid() || !inverter_power_property->is_complex())
			{
				GL_THROW("wind turbine:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
		}
		else
		{
			GL_THROW("windturb_dg (id:%d): Invalid parent object",obj->id);
			/* TROUBLESHOOT 
			The wind turbine object must be attached a 3-phase meter object, triplex meter or a rectifier.  Please check parent of object.
			*/
		}
	}
	else
	{
		gl_warning("windturb_dg:%d %s", obj->id, parent==nullptr?"has no parent meter defined":"parent is not a meter");	

		//Set the flag
		parent_is_valid = false;

		//Set the default values for the voltage

		//Get default magnitude
		temp_double_value = Rated_V/sqrt(3.0);

		//Set the values
		value_Circuit_V[0].SetPolar(temp_double_value,0.0);
		value_Circuit_V[1].SetPolar(temp_double_value,-2.0/3.0*PI);
		value_Circuit_V[2].SetPolar(temp_double_value,2.0/3.0*PI);
	}

	if (Gen_status==OFFLINE)
	{
		gl_warning("init_windturb_dg (id:%d,name:%s): Generator is out of service!", obj->id,obj->name); 	
	}	

	if (Gen_type == SYNCHRONOUS || Gen_type == INDUCTION)
	{
		if (Gen_mode == CONSTANTE)
		{
			gl_warning("init_windturb_dg (id:%d,name:%s): Synchronous and induction generators in constant voltage mode has not been fully tested and my not work properly.", obj->id,obj->name);
		}
	}

	if (Rated_VA!=0.0)  
		SB = Rated_VA/3;
	if (Rated_V!=0.0)  
		EB = Rated_V/sqrt(3.0);
	if (SB!=0.0)  
		ZB = EB*EB/SB;
	else 
		GL_THROW("Generator power capacity not specified!");
	/* TROUBLESHOOT 
	Rated_VA of generator must be specified so that per unit values can be calculated
	*/

	if (Turbine_implementation != POWER_CURVE){
		if (Gen_type == INDUCTION)
		{
			gld::complex Zrotor(Rr,Xr);
			gld::complex Zmag = gld::complex(Rc*Xm*Xm/(Rc*Rc + Xm*Xm),Rc*Rc*Xm/(Rc*Rc + Xm*Xm));
			gld::complex Zstator(Rst,Xst);

			//Induction machine two-port matrix.
			IndTPMat[0][0] = (Zmag + Zstator)/Zmag;
			IndTPMat[0][1] = Zrotor + Zstator + Zrotor*Zstator/Zmag;
			IndTPMat[1][0] = gld::complex(1,0) / Zmag;
			IndTPMat[1][1] = (Zmag + Zrotor) / Zmag;
		}

		else if (Gen_type == SYNCHRONOUS)
		{
			double Real_Rs = Rs * ZB;
			double Real_Xs = Xs * ZB;
			double Real_Rg = Rg * ZB;
			double Real_Xg = Xg * ZB;
			tst = gld::complex(Real_Rg,Real_Xg);
			tst2 = gld::complex(Real_Rs,Real_Xs);
			AMx[0][0] = tst2 + tst;			//Impedance matrix
			AMx[1][1] = tst2 + tst;
			AMx[2][2] = tst2 + tst;
			AMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst;
			tst3 = (gld::complex(1,0) + gld::complex(2,0)*tst/tst2)/(tst2 + gld::complex(3,0)*tst);
			tst4 = (-tst/tst2)/(tst2 + tst);
			invAMx[0][0] = tst3;			//Admittance matrix (inverse of Impedance matrix)
			invAMx[1][1] = tst3;
			invAMx[2][2] = tst3;
			invAMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst4;
		}
		else
			GL_THROW("Unknown generator type specified");
		/* TROUBLESHOOT
		Shouldn't have been able to specify an unknown generator type.  Please report this error to GridLAB-D support.
		*/
	}

	init_climate();

	//Check the convergence criterion
	if (internal_model_current_convergence <= 0.0)
	{
		gl_error("windturb_dg:%d - %s: invalid convergence criterion specified!", obj->id,(obj->name ? obj->name:"Unnamed"));
		/*  TROUBLESHOOT
		The internal model convergence criterion was set to zero or negative.  This invalid.  Fix it to continue.
		*/

		return 0;
	}

	return 1;
}

// Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP windturb_dg::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	//current_A = current_B = current_C = 0.0;
	return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP windturb_dg::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP t2 = TS_NEVER;
	double Pwind, Pmech, detCp, F, G, gearbox_eff;
	double matCp[2][3];
	double store_current_current, store_last_current;

	OBJECT *obj = OBJECTHDR(this);
	FUNCTIONADDR test_fxn;
	STATUS fxn_return_status;

	//Update tracker
	store_last_current = current_A.Mag() + current_B.Mag() + current_C.Mag();

	//Map the current injection routine to our parent bus, if needed
	if (NR_first_run)
	{
		if (parent_is_valid && !parent_is_inverter)
		{
			//Map the current injection function
			test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_current_injection_update_map"));

			//See if it was located
			if (test_fxn == nullptr)
			{
				GL_THROW("windturb_dg:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the additional current injection function, an error was encountered.
				Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
				*/
			}

			//Call the mapping function
			fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *))(*test_fxn))(obj->parent, obj);

			//Make sure it worked
			if (fxn_return_status != SUCCESS)
			{
				GL_THROW("windturb_dg:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
				//Defined above
			}
		}

		//Force a reiteration on the first run.  Current convergence check should get it, but just to be safe
		t2 = t0;

		//Deflag us
		NR_first_run = false;
	}
	//Default else - FBS or not first run

	//Pull the updates, if needed
	if (climate_is_valid)
	{
		value_Press = pPress->get_double() * 100;       // in Pa; climate is in mbar
		value_Temp = (pTemp->get_double() - 32) * 5/9;  // in degC; climate is in degF
	}


	// press in Pascals and temp in Kelvins
	air_dens = (value_Press) * Molar / (Ridealgas * ( value_Temp + 273.15) );

	//wind speed at height of hub - uses European Wind Atlas method
	if(ref_height == roughness_l){
		ref_height = roughness_l+0.001;
	}

	//WSadj = value_WS * log(turbine_height/roughness_l)/log(ref_height/roughness_l); log(37/.055)/log(10/0.055) = 2.828/2.26 = 1.25

	switch (Wind_speed_source){
		case WIND_SPEED:
			//value_WS = wind_speed_hub_ht;       //new var. should also have a default vale
			WSadj = wind_speed_hub_ht;
			break;

		case BUILT_IN:
			//value_WS = avg_ws;          //
			WSadj = avg_ws * log(turbine_height/roughness_l)/log(ref_height/roughness_l);
			break;

		case CLIMATE_DATA:
			double climate_ws=0;
			if (climate_is_valid){
				climate_ws = pWS->get_double() * 0.44704;       //mph to m/s conversion
			}else{
				climate_ws = avg_ws;
				gl_warning("windturb_dg (id:%d)::sync(): no climate data found, using built in source",obj->id);
			}
			WSadj = climate_ws * log(turbine_height/roughness_l)/log(ref_height/roughness_l);
			break;
	}
	/* TODO:  import previous and future wind data 
	and then pseudo-randomize the wind speed values beween 1st and 2nd
	WSadj = gl_pseudorandomvalue(RT_RAYLEIGH,&c,(WS1/sqrt(PI/2)));*/
	switch (Turbine_implementation){
		case COEFF_OF_PERFORMANCE:
			// It looks cleaner to switch to a density adjustment on an interpolated power curve
			//    Pwind error is on the order of Cp error cubed
			Pwind = 0.5 * (air_dens) * PI * pow(blade_diam/2,2) * pow(WSadj,3);

			if (CP_Data == GENERAL_LARGE || CP_Data == GENERAL_MID || CP_Data == GENERAL_SMALL)
			{
				if (WSadj <= cut_in_ws)
				{
					Cp = 0;
				}

				else if (WSadj > cut_out_ws)
				{
					Cp = 0;
				}

				else if(WSadj > ws_rated)
				{
					Cp = Cp_rated * pow(ws_rated,3) / pow(WSadj,3);
				}
				else
				{
					if (WSadj == 0 || WSadj <= cut_in_ws || WSadj >= cut_out_ws)
					{
						Cp = 0;
					}
					else {
						matCp[0][0] = pow((ws_maxcp/cut_in_ws - 1),2);   //Coeff of Performance found
						matCp[0][1] = pow((ws_maxcp/cut_in_ws - 1),3);	 //by using method described in [1]
						matCp[0][2] = 1;
						matCp[1][0] = pow((ws_maxcp/ws_rated - 1),2);
						matCp[1][1] = pow((ws_maxcp/ws_rated - 1),3);
						matCp[1][2] = 1 - Cp_rated/Cp_max;
						detCp = matCp[0][0]*matCp[1][1] - matCp[0][1]*matCp[1][0];

						F = (matCp[0][2]*matCp[1][1] - matCp[0][1]*matCp[1][2])/detCp;
						G = (matCp[0][0]*matCp[1][2] - matCp[0][2]*matCp[1][0])/detCp;

						Cp = Cp_max*(1 - F*pow((ws_maxcp/WSadj - 1),2) - G*pow((ws_maxcp/WSadj - 1),3));
					}
				}
			}
			else if (CP_Data == MANUF_TABLE)	//Coefficient of Perfomance generated from Manufacturer's table
			{
				switch (Turbine_Model)	{
					case VESTAS_V82:
						if (WSadj <= cut_in_ws || WSadj >= cut_out_ws)
						{
							Cp = 0;
						}
						else
						{
							//Original data [0 0 0 0 0.135 0.356 0.442 0.461 .458 .431 .397 .349 .293 .232 .186 .151 .125 .104 .087 .074 .064] from 4-20 m/s
							double Cp_tab[21] = {0, 0, 0, 0, 0.135, 0.356, 0.442, 0.461, 0.458, 0.431, 0.397, 0.349, 0.293, 0.232, 0.186, 0.151, 0.125, 0.104, 0.087, 0.074, 0.064};

							// Linear interpolation on the table
							Cp = Cp_tab[(int)WSadj] + (WSadj - (int)WSadj) * (Cp_tab[(int)WSadj+1] - Cp_tab[(int)WSadj]);
						}
						break;
					default:
						GL_THROW("Coefficient of Performance model not determined.");
				}

			}
			else if (CP_Data == CALCULATED)
			{
				matCp[0][0] = pow((ws_maxcp/cut_in_ws - 1),2);   //Coeff of Performance found
				matCp[0][1] = pow((ws_maxcp/cut_in_ws - 1),3);	 //by using method described in [1]
				matCp[0][2] = 1;
				matCp[1][0] = pow((ws_maxcp/ws_rated - 1),2);
				matCp[1][1] = pow((ws_maxcp/ws_rated - 1),3);
				matCp[1][2] = 1 - Cp_rated/Cp_max;
				detCp = matCp[0][0]*matCp[1][1] - matCp[0][1]*matCp[1][0];

				F = (matCp[0][2]*matCp[1][1] - matCp[0][1]*matCp[1][2])/detCp;
				G = (matCp[0][0]*matCp[1][2] - matCp[0][2]*matCp[1][0])/detCp;

				Cp = Cp_max*(1 - F*pow((ws_maxcp/WSadj - 1),2) - G*pow((ws_maxcp/WSadj - 1),3));
			}
			else
			{
				GL_THROW("CP_Data not defined.");
			}

			/// define user defined data , also catch for future cases

			Pmech = Pwind * Cp;

			if (Pmech != 0)
			{
				gearbox_eff = 1 - (q*.01*Rated_VA / Pmech);	 //Method described in [2].

				if (gearbox_eff < .1)
				{
					gearbox_eff = .1;	//Prevents efficiency from becoming negative at low power.
				}

				Pmech = Pwind * Cp * gearbox_eff;
			}

			Pconv = 1 * Pmech;  //TODO: Assuming 0% losses due to friction and miscellaneous losses


			compute_current_injection();

			break;
		case POWER_CURVE:
			if (!parent_is_inverter){
				compute_current_injection_pc();
			}else{
				compute_power_injection_pc();
			}
			
			break;
	}

	// Double check to make sure it is actually converged to a steady answer
	// Initial comment had this flagged for NR, but FBS is actually the one most sensitive to this
	store_current_current = current_A.Mag() + current_B.Mag() + current_C.Mag();
	if ( fabs((store_current_current - store_last_current) / store_last_current) > internal_model_current_convergence )
		t2 = t1;

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}
/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP windturb_dg::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	// Handling for NR || FBS

	if (!parent_is_triplex){
		//Remove our parent contributions (so XMLs look proper)
		value_Line_I[0] = -prev_current[0];
		value_Line_I[1] = -prev_current[1];
		value_Line_I[2] = -prev_current[2];
	} else {
		value_Line12 = -prev_current12;
	}

	//Zero the tracker, otherwise this won't work right next round
	prev_current[0] = prev_current[1] = prev_current[2] = complex(0.0,0.0);

	prev_current12 = complex(0.0,0.0);

	//Push up the powerflow interface
	if (parent_is_valid)
	{
		if (!parent_is_inverter){
			push_complex_powerflow_values();
		}
	}

	//Re-zero the tracker, or the next iteration will have issues
	prev_current[0] = prev_current[1] = prev_current[2] = complex(0.0,0.0);

	prev_current12 = complex(0.0,0.0);

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//Functionalized current calculation - basically so NR can call it on iterations
void windturb_dg::compute_current_injection(void)
{
	if (Gen_status==ONLINE)
	{
		int k;

		//Pull the voltage values
		if (parent_is_valid)
		{
			value_Circuit_V[0] = pCircuit_V[0]->get_complex();
			value_Circuit_V[1] = pCircuit_V[1]->get_complex();
			value_Circuit_V[2] = pCircuit_V[2]->get_complex();
		}

		voltage_A = value_Circuit_V[0];	//Syncs the meter parent to the generator.
		voltage_B = value_Circuit_V[1];
		voltage_C = value_Circuit_V[2];
		double Pconva = 0.0;
		double Pconvb = 0.0;
		double Pconvc = 0.0;
		if((voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()) > 1e-9){
			Pconva = (voltage_A.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;
			Pconvb = (voltage_B.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;
			Pconvc = (voltage_C.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;
		}

		if (Gen_type == INDUCTION)	//TO DO:  Induction gen. Ef not working correctly yet.
		{
			Pconva = Pconva/Rated_VA;					//induction generator solved in pu
			Pconvb = Pconvb/Rated_VA;
			Pconvc = Pconvc/Rated_VA;

			Vapu = voltage_A/(Rated_V/sqrt(3.0));	
			Vbpu = voltage_B/(Rated_V/sqrt(3.0));
			Vcpu = voltage_C/(Rated_V/sqrt(3.0));

			Vrotor_A = Vapu;  
			Vrotor_B = Vbpu;
			Vrotor_C = Vcpu;

			gld::complex detTPMat = IndTPMat[1][1]*IndTPMat[0][0] - IndTPMat[1][0]*IndTPMat[0][1];

			if (Pconv > 0)			
			{
				switch (Gen_mode)	
				{
				case CONSTANTE:
					for(k = 0; k < 6; k++) //TODO: convert to a convergence
					{
						Irotor_A = (~((gld::complex(Pconva,0)/Vrotor_A)));
						Irotor_B = (~((gld::complex(Pconvb,0)/Vrotor_B)));
						Irotor_C = (~((gld::complex(Pconvc,0)/Vrotor_C)));

						Iapu = IndTPMat[1][0]*Vrotor_A + IndTPMat[1][1]*Irotor_A;
						Ibpu = IndTPMat[1][0]*Vrotor_B + IndTPMat[1][1]*Irotor_B;
						Icpu = IndTPMat[1][0]*Vrotor_C + IndTPMat[1][1]*Irotor_C;

						Vrotor_A = gld::complex(1,0)/detTPMat * (IndTPMat[1][1]*Vapu - IndTPMat[0][1]*Iapu);
						Vrotor_B = gld::complex(1,0)/detTPMat * (IndTPMat[1][1]*Vbpu - IndTPMat[0][1]*Ibpu);
						Vrotor_C = gld::complex(1,0)/detTPMat * (IndTPMat[1][1]*Vcpu - IndTPMat[0][1]*Icpu);

						Vrotor_A = Vrotor_A * Max_Vrotor / Vrotor_A.Mag();
						Vrotor_B = Vrotor_B * Max_Vrotor / Vrotor_B.Mag();
						Vrotor_C = Vrotor_C * Max_Vrotor / Vrotor_C.Mag();
					}
					break;
				case CONSTANTPQ:
					double last_Ipu = 0;
					double current_Ipu = 1;
					unsigned int temp_ind = 1;

					while ( fabs( (last_Ipu-current_Ipu)/current_Ipu) > 0.005 )
					{
						last_Ipu = current_Ipu;

						Irotor_A = -(~gld::complex(-Pconva/Vrotor_A.Mag()*cos(Vrotor_A.Arg()),Pconva/Vrotor_A.Mag()*sin(Vrotor_A.Arg())));
						Irotor_B = -(~gld::complex(-Pconvb/Vrotor_B.Mag()*cos(Vrotor_B.Arg()),Pconvb/Vrotor_B.Mag()*sin(Vrotor_B.Arg())));
						Irotor_C = -(~gld::complex(-Pconvc/Vrotor_C.Mag()*cos(Vrotor_C.Arg()),Pconvc/Vrotor_C.Mag()*sin(Vrotor_C.Arg())));

						Iapu = IndTPMat[1][0]*Vrotor_A - IndTPMat[1][1]*Irotor_A;
						Ibpu = IndTPMat[1][0]*Vrotor_B - IndTPMat[1][1]*Irotor_B;
						Icpu = IndTPMat[1][0]*Vrotor_C - IndTPMat[1][1]*Irotor_C;

						Vrotor_A = gld::complex(1,0)/detTPMat * (IndTPMat[1][1]*Vapu - IndTPMat[0][1]*Iapu);
						Vrotor_B = gld::complex(1,0)/detTPMat * (IndTPMat[1][1]*Vbpu - IndTPMat[0][1]*Ibpu);
						Vrotor_C = gld::complex(1,0)/detTPMat * (IndTPMat[1][1]*Vcpu - IndTPMat[0][1]*Icpu);

						current_Ipu = Iapu.Mag() + Ibpu.Mag() + Icpu.Mag();

						temp_ind += 1;
						if (temp_ind > 100)
						{
							OBJECT *obj = OBJECTHDR(this);

							gl_warning("windturb_dg (id:%d,name:%s): internal iteration limit reached, breaking out of loop.  Injected current may not be solved sufficiently.",obj->id,obj->name);
							/* TROUBLESHOOT
							This may need some work.  The generator models are solved iteratively by using the system voltage
							as the boundary condition.  The current model iterates on solving the current injection, but then
							breaks out if not solved within 100 iterations.  May indicate some issues with the model (i.e.,
							voltage is incorrectly set on the connection node) or it may indicate poor programming.  Please report
							if you see this message.
							*/
							break;
						}
					}
					break;
				}

				// convert current back out of p.u.
				current_A = Iapu * Rated_VA/(Rated_V/sqrt(3.0));	
				current_B = Ibpu * Rated_VA/(Rated_V/sqrt(3.0));	
				current_C = Icpu * Rated_VA/(Rated_V/sqrt(3.0));
			}
			else // Generator is offline
			{
				current_A = 0;	
				current_B = 0;	
				current_C = 0;
			}
		}

		else if (Gen_type == SYNCHRONOUS)			//synch gen is NOT solved in pu
		{											//sg ef mode is not working yet
			double Mxef, Mnef, PoutA, PoutB, PoutC, QoutA, QoutB, QoutC;
			gld::complex SoutA, SoutB, SoutC;
			gld::complex lossesA, lossesB, lossesC;

			Mxef = Max_Ef * Rated_V/sqrt(3.0);
			Mnef = Min_Ef * Rated_V/sqrt(3.0);

			//TODO: convert to a convergence
			if (Gen_mode == CONSTANTE)	//Ef is controllable to give a needed power output.
			{
				current_A = invAMx[0][0]*(voltage_A - EfA) + invAMx[0][1]*(voltage_B - EfB) + invAMx[0][2]*(voltage_C - EfC);
				current_B = invAMx[1][0]*(voltage_A - EfA) + invAMx[1][1]*(voltage_B - EfB) + invAMx[1][2]*(voltage_C - EfC);
				current_C = invAMx[2][0]*(voltage_A - EfA) + invAMx[2][1]*(voltage_B - EfB) + invAMx[2][2]*(voltage_C - EfC);

				SoutA = -voltage_A * (~(current_A));  //TO DO:  unbalanced
				SoutB = -voltage_B * (~(current_B));
				SoutC = -voltage_C * (~(current_C));

			}
			//Gives a constant output power of real power converted Pout,then Qout is found through a controllable power factor.
			else if (Gen_mode == CONSTANTP)	
			{	
				//If air density increases, power extracted can be much greater than the default specifications - cap it.								
				if (Pconva > 1.025*Max_P/3) {
					Pconva = 1.025*Max_P/3;		
				}								
				if (Pconvb > 1.025*Max_P/3) {
					Pconvb = 1.025*Max_P/3;
				}
				if (Pconvc > 1.025*Max_P/3) {
					Pconvc = 1.025*Max_P/3;
				}
				if(voltage_A.Mag() > 0.0){
					current_A = -(~(gld::complex(Pconva,Pconva*tan(acos(pf)))/voltage_A));
				} else {
					current_A = gld::complex(0.0,0.0);
				}
				if(voltage_B.Mag() > 0.0){
					current_B = -(~(gld::complex(Pconvb,Pconvb*tan(acos(pf)))/voltage_B));
				} else {
					current_B = gld::complex(0.0,0.0);
				}
				if(voltage_B.Mag() > 0.0){
					current_C = -(~(gld::complex(Pconvc,Pconvc*tan(acos(pf)))/voltage_C));
				} else {
					current_C = gld::complex(0.0,0.0);
				}

				if (Pconv > 0)
				{
					double last_current = 0;
					double current_current = current_A.Mag() + current_B.Mag() + current_C.Mag();
					unsigned int temp_count = 1;
					
					while ( fabs( (last_current-current_current)/current_current) > 0.005 )
					{
						last_current = current_current;

						PoutA = Pconva - current_A.Mag()*current_A.Mag()*(AMx[0][0] - AMx[0][1]).Re();
						PoutB = Pconvb - current_B.Mag()*current_B.Mag()*(AMx[1][1] - AMx[0][1]).Re();
						PoutC = Pconvc - current_C.Mag()*current_C.Mag()*(AMx[2][2] - AMx[0][1]).Re();

						QoutA = pf/fabs(pf)*PoutA*sin(acos(pf));
						QoutB = pf/fabs(pf)*PoutB*sin(acos(pf));
						QoutC = pf/fabs(pf)*PoutC*sin(acos(pf));
						if(voltage_A.Mag() > 0.0){
							current_A = -(~(gld::complex(PoutA,QoutA)/voltage_A));
						} else {
							current_A = gld::complex(0.0,0.0);
						}
						if(voltage_B.Mag() > 0.0){
						current_B = -(~(gld::complex(PoutB,QoutB)/voltage_B));
						} else {
							current_B = gld::complex(0.0,0.0);
						}
						if(voltage_C.Mag() > 0.0){
						current_C = -(~(gld::complex(PoutC,QoutC)/voltage_C));
						} else {
							current_C = gld::complex(0.0,0.0);
						}

						current_current = current_A.Mag() + current_B.Mag() + current_C.Mag();

						temp_count += 1;

						if ( temp_count > 100 )
						{
							OBJECT *obj = OBJECTHDR(this);

							gl_warning("windturb_dg (id:%d,name:%s): internal iteration limit reached, breaking out of loop.  Injected current may not be solved sufficiently.",obj->id,obj->name);
							/* TROUBLESHOOT
							This may need some work.  The generator models are solved iteratively by using the system voltage
							as the boundary condition.  The current model iterates on solving the current injection, but then
							breaks out if not solved within 100 iterations.  May indicate some issues with the model (i.e.,
							voltage is incorrectly set on the connection node) or it may indicate poor programming.  Please report
							if you see this message.
							*/
							break;
						}
					}
					gl_debug("windturb_dg iteration count = %d",temp_count);
				}
				else
				{
					current_A = 0;
					current_B = 0;
					current_C = 0;
				}

				EfA = voltage_A - (AMx[0][0] - AMx[0][1])*current_A - AMx[0][2]*(current_A + current_B + current_C);
				EfB = voltage_B - (AMx[1][1] - AMx[1][0])*current_A - AMx[1][2]*(current_A + current_B + current_C);
				EfC = voltage_C - (AMx[2][2] - AMx[2][0])*current_A - AMx[2][1]*(current_A + current_B + current_C);
			}
			else
				GL_THROW("Unknown generator mode");
		}

		//sum up and finalize everything for output
		double PowerA, PowerB, PowerC, QA, QB, QC;

		PowerA = -voltage_A.Mag()*current_A.Mag()*cos(voltage_A.Arg() - current_A.Arg());
		PowerB = -voltage_B.Mag()*current_B.Mag()*cos(voltage_B.Arg() - current_B.Arg());
		PowerC = -voltage_C.Mag()*current_C.Mag()*cos(voltage_C.Arg() - current_C.Arg());

		QA = -voltage_A.Mag()*current_A.Mag()*sin(voltage_A.Arg() - current_A.Arg());
		QB = -voltage_B.Mag()*current_B.Mag()*sin(voltage_B.Arg() - current_B.Arg());
		QC = -voltage_C.Mag()*current_C.Mag()*sin(voltage_C.Arg() - current_C.Arg());

		power_A = gld::complex(PowerA,QA);
		power_B = gld::complex(PowerB,QB);
		power_C = gld::complex(PowerC,QC);

		TotalRealPow = PowerA + PowerB + PowerC;
		TotalReacPow = QA + QB + QC;

		GenElecEff = TotalRealPow/Pconv * 100;

		Wind_Speed = WSadj;
	}
	// Generator is offline
	else 
	{
		current_A = 0;
		current_B = 0;
		current_C = 0;

		power_A = gld::complex(0,0);
		power_B = gld::complex(0,0);
		power_C = gld::complex(0,0);
	}

	//Update values
	value_Line_I[0] = current_A;
	value_Line_I[1] = current_B;
	value_Line_I[2] = current_C;

	//Powerflow post-it
	if (parent_is_valid)
	{
		push_complex_powerflow_values();
	}

}

void windturb_dg::compute_current_injection_pc(void){
	double Power_calc;

	if (parent_is_valid)
	{
		value_Circuit_V[0] = pCircuit_V[0]->get_complex();
		value_Circuit_V[1] = pCircuit_V[1]->get_complex();
		value_Circuit_V[2] = pCircuit_V[2]->get_complex();
	}

	voltage_A = value_Circuit_V[0];	//Syncs the meter parent to the wind turbine
	voltage_B = value_Circuit_V[1];
	voltage_C = value_Circuit_V[2];

	if (WSadj <= Power_Curve[0][0]) {
		Power_calc = 0;
	} else if (WSadj >= Power_Curve[0][number_of_points-1]){
		Power_calc = 0;
	} else {
		for (int i=0; i<(number_of_points-1); i++){
			if (WSadj >= Power_Curve[0][i] && WSadj <= Power_Curve[0][i+1]){

				Power_calc = Power_Curve[1][i] + ((Power_Curve[1][i+1] - Power_Curve[1][i]) * ((WSadj - Power_Curve[0][i]) / (Power_Curve[0][i+1] - Power_Curve[0][i])));
			}
		}
	}
	if (number_of_phases_out == 3)//has all three phases
	{
		double PowerA, PowerB, PowerC;
		double QA, QB, QC;   					//reactive power assumed to be zero.
		QA = 0;
		QB = 0;
		QC = 0;

		if(strstr(power_curve_csv, ".csv")){  //user provided power curve .csv file
			if (power_curve_pu){      //p.u. power curve provided, uses turbine capacity
				power_A = complex((Power_calc*Rated_VA)/3,QA);
				power_B = complex((Power_calc*Rated_VA)/3,QB);
				power_C = complex((Power_calc*Rated_VA)/3,QC);
			} else {                          //power curve in watts provided, ignores capacity even if it is specified
				power_A = complex((Power_calc)/3,QA);
				power_B = complex((Power_calc)/3,QB);
				power_C = complex((Power_calc)/3,QC);
			}
		} else {                              //case with Default power curve (p.u.), uses capacity
			power_A = complex((Power_calc*Rated_VA)/3,QA);
			power_B = complex((Power_calc*Rated_VA)/3,QB);
			power_C = complex((Power_calc*Rated_VA)/3,QC);
		}

		Wind_Speed = WSadj;

		if(voltage_A.Mag() > 0.0){
			current_A = -(~(power_A/voltage_A));
		} else {
			current_A = complex(0.0,0.0);
		}
		if(voltage_B.Mag() > 0.0){
			current_B = -(~(power_B/voltage_B));
		} else {
			current_B = complex(0.0,0.0);
		}
		if(voltage_C.Mag() > 0.0){
			current_C = -(~(power_C/voltage_C));
		} else {
			current_C = complex(0.0,0.0);
		}

		PowerA = -voltage_A.Mag()*current_A.Mag()*cos(voltage_A.Arg() - current_A.Arg());
		PowerB = -voltage_B.Mag()*current_B.Mag()*cos(voltage_B.Arg() - current_B.Arg());
		PowerC = -voltage_C.Mag()*current_C.Mag()*cos(voltage_C.Arg() - current_C.Arg());

		TotalRealPow = PowerA + PowerB + PowerC;
		TotalReacPow = QA + QB + QC;

		//Update values
		value_Line_I[0] = current_A;
		value_Line_I[1] = current_B;
		value_Line_I[2] = current_C;

		//Powerflow post-it
		if (parent_is_valid)
		{
			push_complex_powerflow_values();
		}
	} else if ( ((phases & PHASE_S1) == PHASE_S1) && (number_of_phases_out == 1) ) { // Split-phase

		double QA;  					//reactive power assumed to be zero.
		QA = 0;

		if(strstr(power_curve_csv, ".csv")){       //user provided power curve .csv file
			if (power_curve_pu){           //p.u. power curve provided, uses turbine capacity
				power_A = complex((Power_calc*Rated_VA),QA);
			} else {                               //power curve in watts provided, ignores capacity even if it is specified
				power_A = complex((Power_calc),QA);
			}
		} else {                                   //case with Default power curve (p.u.), uses capacity
			power_A = complex((Power_calc*Rated_VA),QA);
		}

		Wind_Speed = WSadj;

		if(voltage_A.Mag() > 0.0){
			current_A = -(~(power_A/voltage_A));
		} else {
			current_A = complex(0.0,0.0);
		}

		TotalRealPow = -voltage_A.Mag()*current_A.Mag()*cos(voltage_A.Arg() - current_A.Arg());
		TotalReacPow = QA;

		//Update value
		value_Line12 = current_A;

		//Powerflow post-it
		if (parent_is_valid)
		{
			push_complex_powerflow_values();
		}
	} else if ( (((phases & PHASE_ABCN) == PHASE_ABCN) && (number_of_phases_out == 1)) || (((phases & PHASE_ABC) == PHASE_ABC) && (number_of_phases_out == 1)) ) {    //case where a three-phase wind turbine is connected to triplex meter
		GL_THROW ("Phase mismatch between the wind turbine and the triplex parent");
	} else {
		//Should not get here
		GL_THROW ("Invalid phase configuration");
	}
}

void windturb_dg::compute_power_injection_pc(void){
	double Power_calc;
	double Q;  					//reactive power assumed to be zero.
	Q = 0;
	
	if (WSadj <= Power_Curve[0][0]) {	
		Power_calc = 0;	
	} else if (WSadj >= Power_Curve[0][number_of_points-1]){
		Power_calc = 0;
	} else {	  
		for (int i=0; i<(number_of_points-1); i++){
			if (WSadj >= Power_Curve[0][i] && WSadj <= Power_Curve[0][i+1]){

				Power_calc = Power_Curve[1][i] + ((Power_Curve[1][i+1] - Power_Curve[1][i]) * ((WSadj - Power_Curve[0][i]) / (Power_Curve[0][i+1] - Power_Curve[0][i])));
			}
		}
	}
	//now publish Power_calc to inverter parent
	if (parent_is_valid){
		push_complex_power_values(complex((Power_calc*Rated_VA),Q));
	}
}

//Map Complex value
gld_property *windturb_dg::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("windturb_dg:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in windturb_dg.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *windturb_dg::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("windturb_dg:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in windturb_dg.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to push up all changes of complex properties to powerflow from local variables
void windturb_dg::push_complex_powerflow_values(void)
{
	gld::complex temp_complex_val;
	gld_wlock *test_rlock = nullptr;
	int indexval;

	if (parent_is_triplex)
	{
		//Shouldn't need to NULL check this one, but do it anyways, for consistency
		if (pLine12 != nullptr)
		{
			//**** Current value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine12->get_complex();

			//Add the difference
			temp_complex_val += value_Line12 - prev_current12;

			//Push it back up
			pLine12->setp<complex>(temp_complex_val,*test_rlock);

			//Store the update value
			prev_current12 = value_Line12;
		}//End pLine_I valid
		//Default else -- it's null, so skip it
	} else //End is triplex, //Loop through the three-phases/accumulators
	{
		for (indexval=0; indexval<3; indexval++)
		{
			//**** Current value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_I[indexval]->get_complex();

			//Add the difference
			//temp_complex_val += value_Line_I[indexval];
			temp_complex_val += value_Line_I[indexval] - prev_current[indexval];

			//Push it back up
			pLine_I[indexval]->setp<gld::complex>(temp_complex_val,*test_rlock);

			//Store the update value
			prev_current[indexval] = value_Line_I[indexval];
		}
	}
}

void windturb_dg::push_complex_power_values(complex inv_P)
{
	//complex temp_complex_val;
	gld_wlock *test_rlock = nullptr;
	bool WT_conn_flag = true;
	//int indexval;
	
	if (parent_is_inverter) {
		//Push the changes
		inverter_power_property->setp<complex>(inv_P, *test_rlock);
		inverter_flag_property->setp<bool>(WT_conn_flag, *test_rlock);
	}
}

// Function to update current injection value
STATUS windturb_dg::updateCurrInjection(int64 iteration_count,bool *converged_failure)
{
	//Assume no convergence failure - mostly for deltamode/Norton-equivalent initialization
	*converged_failure = false;
	
	//Call the current updating function
	switch (Turbine_implementation){
		case COEFF_OF_PERFORMANCE:
			compute_current_injection();
			break;

		case POWER_CURVE:
			if (!parent_is_inverter){
				compute_current_injection_pc();
			}else{
				compute_power_injection_pc();
			}
			break;
	}

	//Always succeed for now
	return SUCCESS;
}

//Generic Functions to read .csv files
std::vector<std::string> windturb_dg::readCSVRow(const std::string &row) {
    std::vector<std::string> fields {""};
    size_t i = 0; // index of the current field
    for (char c : row) {
		if (c == ','){
			fields.push_back("");
			i++;
		} else {
			fields[i].push_back(c);
		}
    }
    return fields;
}

std::vector<std::vector<std::string>> windturb_dg::readCSV(std::istream &in) {
    std::vector<std::vector<std::string>> table;
    std::string row;
    while (!in.eof()) {
        std::getline(in, row);
        if (in.bad() || in.fail()) {
            break;
        }
        auto fields = readCSVRow(row);
        table.push_back(fields);
    }
    return table;
}

bool windturb_dg::hasEnding(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: windturb_dg
//////////////////////////////////////////////////////////////////////////

EXPORT int create_windturb_dg(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(windturb_dg::oclass);
		if (*obj!=nullptr)
		{
			windturb_dg *my = OBJECTDATA(*obj,windturb_dg);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else 
			return 0;
	}
	CREATE_CATCHALL(windturb_dg);
}



EXPORT int init_windturb_dg(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=nullptr)
			return OBJECTDATA(obj,windturb_dg)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(windturb_dg);
}

EXPORT TIMESTAMP sync_windturb_dg(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = TS_NEVER;
	windturb_dg *my = OBJECTDATA(obj,windturb_dg);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t1 = my->presync(obj->clock,t0);
			break;
		case PC_BOTTOMUP:
			t1 = my->sync(obj->clock,t0);
			break;
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock,t0);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
	}
	SYNC_CATCHALL(windturb_dg);
	return t1;
}

//Current injection exposed function
EXPORT STATUS windturb_dg_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure)
{
	STATUS temp_status;

	//Map the node
	windturb_dg *my = OBJECTDATA(obj, windturb_dg);

	//Call the function, where we can update the current injection
	temp_status = my->updateCurrInjection(iteration_count,converged_failure);

	//Return what the sub function said we were
	return temp_status;
}

/*
[1]	Malinga, B., Sneckenberger, J., and Feliachi, A.; "Modeling and Control of a Wind Turbine as a Distributed Resource", 
Proceedings of the 35th Southeastern Symposium on System Theory, Mar 16-18, 2003, pp. 108-112.
[2]	Cotrell J.R., "A preliminary evaluation of a multiple-generator drivetrain configuration for wind turbines,"
in Proc. 21st ASME Wind Energy Symposium, 2002, pp. 345-352.
*/