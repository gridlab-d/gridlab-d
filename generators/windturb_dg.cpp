/** $Id: windturb_dg.cpp 4738 2014-07-03 00:55:39Z dchassin $
Copyright (C) 2008 Battelle Memorial Institute
@file windturb_dg.cpp
@defgroup windturb_dg Wind Turbine gensets
@ingroup generators

**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "windturb_dg.h"

CLASS *windturb_dg::oclass = NULL;
windturb_dg *windturb_dg::defaults = NULL;

windturb_dg::windturb_dg(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"windturb_dg",sizeof(windturb_dg),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class windturb_dg";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration,"Gen_status",PADDR(Gen_status), PT_DESCRIPTION, "Describes whether the generator is currently online or offline",
			PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE, PT_DESCRIPTION, "Generator is currently not supplying power",
			PT_KEYWORD,"ONLINE",(enumeration)ONLINE, PT_DESCRIPTION, "Generator is currently available to supply power",
			PT_enumeration,"Gen_type",PADDR(Gen_type), PT_DESCRIPTION, "Type of generator",
			PT_KEYWORD,"INDUCTION",(enumeration)INDUCTION, PT_DESCRIPTION, "Standard induction generator",
			PT_KEYWORD,"SYNCHRONOUS",(enumeration)SYNCHRONOUS, PT_DESCRIPTION, "Standard synchronous generator; is also used to 'fake' a doubly-fed induction generator for now",
			PT_enumeration,"Gen_mode",PADDR(Gen_mode), PT_DESCRIPTION, "Control mode that is used for the generator output",
			PT_KEYWORD,"CONSTANTE",(enumeration)CONSTANTE, PT_DESCRIPTION, "Maintains the voltage at the terminals",
			PT_KEYWORD,"CONSTANTP",(enumeration)CONSTANTP, PT_DESCRIPTION, "Maintains the real power output at the terminals",
			PT_KEYWORD,"CONSTANTPQ",(enumeration)CONSTANTPQ, PT_DESCRIPTION, "Maintains the real and reactive output at the terminals - currently unsupported",
			PT_enumeration,"Turbine_Model",PADDR(Turbine_Model), PT_DESCRIPTION, "Type of turbine being represented; using any of these except USER_DEFINED also specifies a large number of defaults",
			PT_KEYWORD,"GENERIC_SYNCH_SMALL",(enumeration)GENERIC_SYNCH_SMALL, PT_DESCRIPTION, "Generic model for a small, fixed pitch synchronous turbine",
			PT_KEYWORD,"GENERIC_SYNCH_MID",(enumeration)GENERIC_SYNCH_MID, PT_DESCRIPTION, "Generic model for a mid-size, fixed pitch synchronous turbine",
			PT_KEYWORD,"GENERIC_SYNCH_LARGE",(enumeration)GENERIC_SYNCH_LARGE, PT_DESCRIPTION, "Generic model for a large, fixed pitch synchronous turbine",
			PT_KEYWORD,"GENERIC_IND_SMALL",(enumeration)GENERIC_IND_SMALL, PT_DESCRIPTION, "Generic model for a small induction, fixed pitch generator turbine",
			PT_KEYWORD,"GENERIC_IND_MID",(enumeration)GENERIC_IND_MID, PT_DESCRIPTION, "Generic model for a mid-size induction, fixed pitch generator turbine",
			PT_KEYWORD,"GENERIC_IND_LARGE",(enumeration)GENERIC_IND_LARGE, PT_DESCRIPTION, "Generic model for a large induction, fixed pitch generator turbine",
			PT_KEYWORD,"USER_DEFINED",(enumeration)USER_DEFINED, PT_DESCRIPTION, "Allows user to specify all parameters - is not currently supported", // TODO: Doesnt work yet
			PT_KEYWORD,"VESTAS_V82",(enumeration)VESTAS_V82, PT_DESCRIPTION, "Sets all defaults to represent the power output of a VESTAS V82 turbine",
			PT_KEYWORD,"GE_25MW",(enumeration)GE_25MW, PT_DESCRIPTION, "Sets all defaults to represent the power output of a GE 2.5MW turbine",
			PT_KEYWORD,"BERGEY_10kW",(enumeration)BERGEY_10kW, PT_DESCRIPTION, "Sets all defaults to represent the power output of a Bergey 10kW turbine",

			// TODO: There are a number of repeat variables due to poor variable name formatting.
			//       These need to be corrected through the deprecation process.
			PT_double, "turbine_height[m]", PADDR(turbine_height), PT_DESCRIPTION, "Describes the height of the wind turbine hub above the ground",
			PT_double, "roughness_length_factor", PADDR(roughness_l), PT_DESCRIPTION, "European Wind Atlas unitless correction factor for adjusting wind speed at various heights above ground and terrain types, default=0.055",
			PT_double, "blade_diam[m]", PADDR(blade_diam), PT_DESCRIPTION, "Diameter of blades",
			PT_double, "blade_diameter[m]", PADDR(blade_diam), PT_DESCRIPTION, "Diameter of blades",
			PT_double, "cut_in_ws[m/s]", PADDR(cut_in_ws), PT_DESCRIPTION, "Minimum wind speed for generator operation",
			PT_double, "cut_out_ws[m/s]", PADDR(cut_out_ws), PT_DESCRIPTION, "Maximum wind speed for generator operation",
			PT_double, "ws_rated[m/s]", PADDR(ws_rated), PT_DESCRIPTION, "Rated wind speed for generator operation",
			PT_double, "ws_maxcp[m/s]", PADDR(ws_maxcp), PT_DESCRIPTION, "Wind speed at which generator reaches maximum Cp",
			PT_double, "Cp_max[pu]", PADDR(Cp_max), PT_DESCRIPTION, "Maximum coefficient of performance",
			PT_double, "Cp_rated[pu]", PADDR(Cp_rated), PT_DESCRIPTION, "Rated coefficient of performance",
			PT_double, "Cp[pu]", PADDR(Cp), PT_DESCRIPTION, "Calculated coefficient of performance",

			PT_double, "Rated_VA[VA]", PADDR(Rated_VA), PT_DESCRIPTION, "Rated generator power output",
			PT_double, "Rated_V[V]", PADDR(Rated_V), PT_DESCRIPTION, "Rated generator terminal voltage",
			PT_double, "Pconv[W]", PADDR(Pconv), PT_DESCRIPTION, "Amount of electrical power converted from mechanical power delivered",
			PT_double, "P_converted[W]", PADDR(Pconv), PT_DESCRIPTION, "Amount of electrical power converted from mechanical power delivered",

			PT_double, "GenElecEff[%]", PADDR(GenElecEff), PT_DESCRIPTION, "Calculated generator electrical efficiency",
			PT_double, "generator_efficiency[%]", PADDR(GenElecEff), PT_DESCRIPTION, "Calculated generator electrical efficiency",
			PT_double, "TotalRealPow[W]", PADDR(TotalRealPow), PT_DESCRIPTION, "Total real power output",
			PT_double, "total_real_power[W]", PADDR(TotalRealPow), PT_DESCRIPTION, "Total real power output",
			PT_double, "TotalReacPow[VA]", PADDR(TotalReacPow), PT_DESCRIPTION, "Total reactive power output",
			PT_double, "total_reactive_power[VA]", PADDR(TotalReacPow), PT_DESCRIPTION, "Total reactive power output",

			PT_complex, "power_A[VA]", PADDR(power_A), PT_DESCRIPTION, "Total complex power injected on phase A",
			PT_complex, "power_B[VA]", PADDR(power_B), PT_DESCRIPTION, "Total complex power injected on phase B",
			PT_complex, "power_C[VA]", PADDR(power_C), PT_DESCRIPTION, "Total complex power injected on phase C",

			PT_double, "WSadj[m/s]", PADDR(WSadj), PT_DESCRIPTION, "Speed of wind at hub height",
			PT_double, "wind_speed_adjusted[m/s]", PADDR(WSadj), PT_DESCRIPTION, "Speed of wind at hub height",
			PT_double, "Wind_Speed[m/s]", PADDR(Wind_Speed),  PT_DESCRIPTION, "Wind speed at 5-15m level (typical measurement height)",
			PT_double, "wind_speed[m/s]", PADDR(Wind_Speed),  PT_DESCRIPTION, "Wind speed at 5-15m level (typical measurement height)",
			PT_double, "air_density[kg/m^3]", PADDR(air_dens), PT_DESCRIPTION, "Estimated air density",

			PT_double, "R_stator[pu*Ohm]", PADDR(Rst), PT_DESCRIPTION, "Induction generator primary stator resistance in p.u.",
			PT_double, "X_stator[pu*Ohm]", PADDR(Xst), PT_DESCRIPTION, "Induction generator primary stator reactance in p.u.",
			PT_double, "R_rotor[pu*Ohm]", PADDR(Rr), PT_DESCRIPTION, "Induction generator primary rotor resistance in p.u.",
			PT_double, "X_rotor[pu*Ohm]", PADDR(Xr), PT_DESCRIPTION, "Induction generator primary rotor reactance in p.u.",
			PT_double, "R_core[pu*Ohm]", PADDR(Rc), PT_DESCRIPTION, "Induction generator primary core resistance in p.u.",
			PT_double, "X_magnetic[pu*Ohm]", PADDR(Xm), PT_DESCRIPTION, "Induction generator primary core reactance in p.u.",
			PT_double, "Max_Vrotor[pu*V]", PADDR(Max_Vrotor), PT_DESCRIPTION, "Induction generator maximum induced rotor voltage in p.u., e.g. 1.2",
			PT_double, "Min_Vrotor[pu*V]", PADDR(Min_Vrotor), PT_DESCRIPTION, "Induction generator minimum induced rotor voltage in p.u., e.g. 0.8",

			PT_double, "Rs[pu*Ohm]", PADDR(Rs), PT_DESCRIPTION, "Synchronous generator primary stator resistance in p.u.",
			PT_double, "Xs[pu*Ohm]", PADDR(Xs), PT_DESCRIPTION, "Synchronous generator primary stator reactance in p.u.",
			PT_double, "Rg[pu*Ohm]", PADDR(Rg), PT_DESCRIPTION, "Synchronous generator grounding resistance in p.u.",
			PT_double, "Xg[pu*Ohm]", PADDR(Xg), PT_DESCRIPTION, "Synchronous generator grounding reactance in p.u.",
			PT_double, "Max_Ef[pu*V]", PADDR(Max_Ef), PT_DESCRIPTION, "Synchronous generator maximum induced rotor voltage in p.u., e.g. 0.8",
			PT_double, "Min_Ef[pu*V]", PADDR(Min_Ef), PT_DESCRIPTION, "Synchronous generator minimum induced rotor voltage in p.u., e.g. 0.8",

			PT_double, "pf[pu]", PADDR(pf), PT_DESCRIPTION, "Desired power factor in CONSTANTP mode (can be modified over time)",
			PT_double, "power_factor[pu]", PADDR(pf), PT_DESCRIPTION, "Desired power factor in CONSTANTP mode (can be modified over time)",

			PT_complex, "voltage_A[V]", PADDR(voltage_A), PT_DESCRIPTION, "Terminal voltage on phase A",
			PT_complex, "voltage_B[V]", PADDR(voltage_B), PT_DESCRIPTION, "Terminal voltage on phase B",
			PT_complex, "voltage_C[V]", PADDR(voltage_C), PT_DESCRIPTION, "Terminal voltage on phase C",
			PT_complex, "current_A[A]", PADDR(current_A), PT_DESCRIPTION, "Calculated terminal current on phase A",
			PT_complex, "current_B[A]", PADDR(current_B), PT_DESCRIPTION, "Calculated terminal current on phase B",
			PT_complex, "current_C[A]", PADDR(current_C), PT_DESCRIPTION, "Calculated terminal current on phase C",
			PT_complex, "EfA[V]", PADDR(EfA), PT_DESCRIPTION, "Synchronous generator induced voltage on phase A",
			PT_complex, "EfB[V]", PADDR(EfB), PT_DESCRIPTION, "Synchronous generator induced voltage on phase B",
			PT_complex, "EfC[V]", PADDR(EfC), PT_DESCRIPTION, "Synchronous generator induced voltage on phase C",
			PT_complex, "Vrotor_A[V]", PADDR(Vrotor_A), PT_DESCRIPTION, "Induction generator induced voltage on phase A in p.u.",//Induction Generator
			PT_complex, "Vrotor_B[V]", PADDR(Vrotor_B), PT_DESCRIPTION, "Induction generator induced voltage on phase B in p.u.",
			PT_complex, "Vrotor_C[V]", PADDR(Vrotor_C), PT_DESCRIPTION, "Induction generator induced voltage on phase C in p.u.",
			PT_complex, "Irotor_A[V]", PADDR(Irotor_A), PT_DESCRIPTION, "Induction generator induced current on phase A in p.u.",
			PT_complex, "Irotor_B[V]", PADDR(Irotor_B), PT_DESCRIPTION, "Induction generator induced current on phase B in p.u.",
			PT_complex, "Irotor_C[V]", PADDR(Irotor_C), PT_DESCRIPTION, "Induction generator induced current on phase C in p.u.",

			PT_set, "phases", PADDR(phases), PT_DESCRIPTION, "Specifies which phases to connect to - currently not supported and assumes three-phase connection",
			PT_KEYWORD, "A",(set)PHASE_A,
			PT_KEYWORD, "B",(set)PHASE_B,
			PT_KEYWORD, "C",(set)PHASE_C,
			PT_KEYWORD, "N",(set)PHASE_N,
			PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
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
	avg_ws = 8;				//wind speed in m/s

	time_advance = 3600;	//amount of time to advance for WS data import in secs.

	/* set the default values of all properties here */
	Ridealgas = 8.31447;
	Molar = 0.0289644;
	//std_air_dens = 1.2754;	//dry air density at std pressure and temp in kg/m^3
	std_air_temp = 0;	//IUPAC std air temp in Celsius
	std_air_press = 100000;	//IUPAC std air pressure in Pascals
	Turbine_Model = GENERIC_SYNCH_LARGE;	//************** Default specified so it doesn't crash out
	Gen_mode = CONSTANTP;					//************** Default specified so values actually come out
	Gen_status = ONLINE;

	turbine_height = -9999;
	blade_diam = -9999;
	cut_in_ws = -9999;
	cut_out_ws = -9999;
	Cp_max = -9999;
	Cp_rated =-9999;
	ws_maxcp = -9999;
	ws_rated = -9999;
	CP_Data = CALCULATED;

	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = NULL;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = NULL;
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = complex(0.0,0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = complex(0.0,0.0);
	parent_is_valid = false;

    pPress = NULL;
	pTemp = NULL;
	pWS = NULL;
    value_Press = 0.0;
	value_Temp = 0.0;
	value_WS = 0.0;
	climate_is_valid = false;

	return 1; /* return 1 on success, 0 on failure */
}

/* Checks for climate object and maps the climate variables to the windturb object variables.  
If no climate object is linked, then default pressure, temperature, and wind speed will be used. */
int windturb_dg::init_climate()
{
	OBJECT *hdr = OBJECTHDR(this);

	// link to climate data
	FINDLIST *climates = NULL;

	climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
	if (climates==NULL)
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
	if (climates!=NULL)
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
	return 1;
}

int windturb_dg::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	double temp_double_value;
	gld_property *temp_property_pointer;
	set temp_phases_set;
	int temp_phases=0;
		
	double ZB, SB, EB;
	complex tst, tst2, tst3, tst4;

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
		default:
			GL_THROW("Unknown turbine model was specified");
			/*  TROUBLESHOOT
			An unknown wind turbine model was selected.  Please select a Turbine_Model from the available list.
			*/
	}

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL)
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

			//Map phases
			//Map and pull the phases
			temp_property_pointer = new gld_property(parent,"phases");

			//Make sure ti worked
			if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_set() != true))
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

			if (temp_phases < 3)
				GL_THROW("The wind turbine model currently only supports a 3-phase connection, please check meter connection");
			/*  TROUBLESHOOT
			Currently the wind turbine model only supports 3-phase connnections. Please attach to 3-phase meter.
			*/

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
			if ( fabs(1 - (temp_double_value * sqrt(3.0) / Rated_V) ) > 0.1 )
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
		else if (gl_object_isa(parent,"triplex_meter","powerflow"))
		{
			GL_THROW("The wind turbine model does currently support direct connection to single phase or triplex meters. Connect through a rectifier-inverter combination.");
			/*  TROUBLESHOOT
			This model does not currently support connection to a triplex system.  Please connect to a 3-phase meter.
			*/
		}
		else if (gl_object_isa(parent,"rectifier","generators"))
		{
			//Set the "pull flag"
			parent_is_valid = true;

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
			if ( fabs(1 - (temp_double_value / Rated_V) ) > 0.1 )
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
		else
		{
			GL_THROW("windturb_dg (id:%d): Invalid parent object",obj->id);
			/* TROUBLESHOOT 
			The wind turbine object must be attached a 3-phase meter object.  Please check parent of object.
			*/
		}
	}
	else
	{
		gl_warning("windturb_dg:%d %s", obj->id, parent==NULL?"has no parent meter defined":"parent is not a meter");	

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

	if (Gen_type == INDUCTION)  
	{
		complex Zrotor(Rr,Xr);
		complex Zmag = complex(Rc*Xm*Xm/(Rc*Rc + Xm*Xm),Rc*Rc*Xm/(Rc*Rc + Xm*Xm));
		complex Zstator(Rst,Xst);

		//Induction machine two-port matrix.
		IndTPMat[0][0] = (Zmag + Zstator)/Zmag;
		IndTPMat[0][1] = Zrotor + Zstator + Zrotor*Zstator/Zmag;
		IndTPMat[1][0] = complex(1,0) / Zmag;
		IndTPMat[1][1] = (Zmag + Zrotor) / Zmag;
	}

	else if (Gen_type == SYNCHRONOUS)  
	{
		double Real_Rs = Rs * ZB; 
		double Real_Xs = Xs * ZB;
		double Real_Rg = Rg * ZB; 
		double Real_Xg = Xg * ZB;
		tst = complex(Real_Rg,Real_Xg);
		tst2 = complex(Real_Rs,Real_Xs);
		AMx[0][0] = tst2 + tst;			//Impedance matrix
		AMx[1][1] = tst2 + tst;
		AMx[2][2] = tst2 + tst;
		AMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst;
		tst3 = (complex(1,0) + complex(2,0)*tst/tst2)/(tst2 + complex(3,0)*tst);
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

	init_climate();

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

	store_last_current = current_A.Mag() + current_B.Mag() + current_C.Mag();

	//Pull the updates, if needed
	if (climate_is_valid == true)
	{
		value_Press = pPress->get_double();
		value_Temp = pTemp->get_double();
		value_WS = pWS->get_double();
	}

	// convert press to Pascals and temp to Kelvins
	// TODO: convert this to using gl_convert
	air_dens = (value_Press*100) * Molar / (Ridealgas * ( (value_Temp - 32)*5/9 + 273.15));

	//wind speed at height of hub - uses European Wind Atlas method
	if(ref_height == roughness_l){
		ref_height = roughness_l+0.001;
	}
	WSadj = value_WS * log(turbine_height/roughness_l)/log(ref_height/roughness_l); 

	/* TODO:  import previous and future wind data 
	and then pseudo-randomize the wind speed values beween 1st and 2nd
	WSadj = gl_pseudorandomvalue(RT_RAYLEIGH,&c,(WS1/sqrt(PI/2)));*/

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
				else  //TO DO:  possibly replace polynomial with spline library function interpolation
				{	  
					//Uses a centered, 10th-degree polynomial Matlab interpolation of original Manuf. data
					double z = (WSadj - 10.5)/5.9161;

					//Original data [0 0 0 0 0.135 0.356 0.442 0.461 .458 .431 .397 .349 .293 .232 .186 .151 .125 .104 .087 .074 .064] from 4-20 m/s
					Cp = -0.08609*pow(z,10) + 0.078599*pow(z,9) + 0.50509*pow(z,8) - 0.45466*pow(z,7) - 0.94154*pow(z,6) + 0.77922*pow(z,5) + 0.59082*pow(z,4) - 0.23196*pow(z,3) - 0.25009*pow(z,2) - 0.24282*z + 0.37502;
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

	if (Gen_status==ONLINE)
	{
		int k;

		//Pull the voltage values
		if (parent_is_valid == true)
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

			complex detTPMat = IndTPMat[1][1]*IndTPMat[0][0] - IndTPMat[1][0]*IndTPMat[0][1];

			if (Pconv > 0)			
			{
				switch (Gen_mode)	
				{
				case CONSTANTE:
					for(k = 0; k < 6; k++) //TODO: convert to a convergence
					{
						Irotor_A = (~((complex(Pconva,0)/Vrotor_A)));
						Irotor_B = (~((complex(Pconvb,0)/Vrotor_B)));
						Irotor_C = (~((complex(Pconvc,0)/Vrotor_C)));

						Iapu = IndTPMat[1][0]*Vrotor_A + IndTPMat[1][1]*Irotor_A;
						Ibpu = IndTPMat[1][0]*Vrotor_B + IndTPMat[1][1]*Irotor_B;
						Icpu = IndTPMat[1][0]*Vrotor_C + IndTPMat[1][1]*Irotor_C;

						Vrotor_A = complex(1,0)/detTPMat * (IndTPMat[1][1]*Vapu - IndTPMat[0][1]*Iapu);
						Vrotor_B = complex(1,0)/detTPMat * (IndTPMat[1][1]*Vbpu - IndTPMat[0][1]*Ibpu);
						Vrotor_C = complex(1,0)/detTPMat * (IndTPMat[1][1]*Vcpu - IndTPMat[0][1]*Icpu);

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

						Irotor_A = -(~complex(-Pconva/Vrotor_A.Mag()*cos(Vrotor_A.Arg()),Pconva/Vrotor_A.Mag()*sin(Vrotor_A.Arg())));
						Irotor_B = -(~complex(-Pconvb/Vrotor_B.Mag()*cos(Vrotor_B.Arg()),Pconvb/Vrotor_B.Mag()*sin(Vrotor_B.Arg())));
						Irotor_C = -(~complex(-Pconvc/Vrotor_C.Mag()*cos(Vrotor_C.Arg()),Pconvc/Vrotor_C.Mag()*sin(Vrotor_C.Arg())));

						Iapu = IndTPMat[1][0]*Vrotor_A - IndTPMat[1][1]*Irotor_A;
						Ibpu = IndTPMat[1][0]*Vrotor_B - IndTPMat[1][1]*Irotor_B;
						Icpu = IndTPMat[1][0]*Vrotor_C - IndTPMat[1][1]*Irotor_C;

						Vrotor_A = complex(1,0)/detTPMat * (IndTPMat[1][1]*Vapu - IndTPMat[0][1]*Iapu);
						Vrotor_B = complex(1,0)/detTPMat * (IndTPMat[1][1]*Vbpu - IndTPMat[0][1]*Ibpu);
						Vrotor_C = complex(1,0)/detTPMat * (IndTPMat[1][1]*Vcpu - IndTPMat[0][1]*Icpu);

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
			complex SoutA, SoutB, SoutC;
			complex lossesA, lossesB, lossesC;

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
					current_A = -(~(complex(Pconva,Pconva*tan(acos(pf)))/voltage_A));
				} else {
					current_A = complex(0.0,0.0);
				}
				if(voltage_B.Mag() > 0.0){
					current_B = -(~(complex(Pconvb,Pconvb*tan(acos(pf)))/voltage_B));
				} else {
					current_B = complex(0.0,0.0);
				}
				if(voltage_B.Mag() > 0.0){
					current_C = -(~(complex(Pconvc,Pconvc*tan(acos(pf)))/voltage_C));
				} else {
					current_C = complex(0.0,0.0);
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
							current_A = -(~(complex(PoutA,QoutA)/voltage_A));
						} else {
							current_A = complex(0.0,0.0);
						}
						if(voltage_B.Mag() > 0.0){
						current_B = -(~(complex(PoutB,QoutB)/voltage_B));
						} else {
							current_B = complex(0.0,0.0);
						}
						if(voltage_C.Mag() > 0.0){
						current_C = -(~(complex(PoutC,QoutC)/voltage_C));
						} else {
							current_C = complex(0.0,0.0);
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

		power_A = complex(PowerA,QA);
		power_B = complex(PowerB,QB);
		power_C = complex(PowerC,QC);

		TotalRealPow = PowerA + PowerB + PowerC;
		TotalReacPow = QA + QB + QC;

		GenElecEff = TotalRealPow/Pconv * 100;

		Wind_Speed = WSadj;
		
		//Update values
		value_Line_I[0] = current_A;
		value_Line_I[1] = current_B;
		value_Line_I[2] = current_C;

		//Powerflow post-it
		if (parent_is_valid == true)
		{
			push_complex_powerflow_values();
		}
	}
	// Generator is offline
	else 
	{
		current_A = 0;
		current_B = 0;
		current_C = 0;

		power_A = complex(0,0);
		power_B = complex(0,0);
		power_C = complex(0,0);
	}

	// Double check to make sure it is actually converged to a steady answer
	//   Mostly applies to NR mode to make sure terminal voltage has converged enough
	//   in NR to give a good boundary condition for solving the generator circuit
	double store_current_current = current_A.Mag() + current_B.Mag() + current_C.Mag();
	if ( fabs((store_current_current - store_last_current) / store_last_current) > 0.005 )
		t2 = t1;

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */	
}
/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP windturb_dg::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	// Handling for NR || FBS

	//Remove our parent contributions (so XMLs look proper)
	value_Line_I[0] = -current_A;
	value_Line_I[1] = -current_B;
	value_Line_I[2] = -current_C;

	//Push up the powerflow interface
	if (parent_is_valid)
	{
		push_complex_powerflow_values();
	}

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
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
	if ((pQuantity->is_valid() != true) || (pQuantity->is_double() != true))
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
	complex temp_complex_val;
	gld_wlock *test_rlock;
	int indexval;

	//Loop through the three-phases/accumulators
	for (indexval=0; indexval<3; indexval++)
	{
		//**** Current value ***/
		//Pull current value again, just in case
		temp_complex_val = pLine_I[indexval]->get_complex();

		//Add the difference
		temp_complex_val += value_Line_I[indexval];

		//Push it back up
		pLine_I[indexval]->setp<complex>(temp_complex_val,*test_rlock);
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
		if (*obj!=NULL)
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
		if (obj!=NULL)
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


/*
[1]	Malinga, B., Sneckenberger, J., and Feliachi, A.; "Modeling and Control of a Wind Turbine as a Distributed Resource", 
Proceedings of the 35th Southeastern Symposium on System Theory, Mar 16-18, 2003, pp. 108-112.

[2]	Cotrell J.R., "A preliminary evaluation of a multiple-generator drivetrain configuration for wind turbines,"
in Proc. 21st ASME Wind Energy Symposium, 2002, pp. 345-352.
*/
