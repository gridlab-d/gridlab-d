/** $Id: solar.cpp,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file solar.cpp
	@defgroup solar Diesel gensets
	@ingroup generators
// Assumptions :
  1. All solar panels are tilted as per the site latitude to perform at their best efficiency
  2. All the solar cells are connected in series in a solar module
  3. 600Volts, 5/7.6 Amps, 200 Watts PV system is used for all residential , commercial and industrial applications. The number of modules will vary 
     based on the surface area
  4. A power derating of 10-15% is applied to take account of power losses and conversion in-efficiencies of the inverter.

// References
1. Photovoltaic Module Thermal/Wind performance: Long-term monitoring and Model development for energy rating , Solar program review meeting 2003, Govindswamy Tamizhmani et al
2. COMPARISON OF ENERGY PRODUCTION AND PERFORMANCE FROM FLAT-PLATE PHOTOVOLTAIC MODULE TECHNOLOGIES DEPLOYED AT FIXED TILT, J.A. del Cueto
3. Solar Collectors and Photovoltaic in energyPRO
4.Calculation of the polycrystalline PV module temperature using a simple method of energy balance 

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"
#include "solar.h"

CLASS *solar::oclass = NULL;
solar *solar::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
solar::solar(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"solar",sizeof(solar),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class solar";
		else
			oclass->trl = TRL_PROOF;


		if (gl_publish_variable(oclass,
			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN, //PV must operate in this mode

			PT_enumeration,"generator_status",PADDR(gen_status_v),
				PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE,
				PT_KEYWORD,"ONLINE",(enumeration)ONLINE,	

			PT_enumeration,"panel_type", PADDR(panel_type_v),
				PT_KEYWORD, "SINGLE_CRYSTAL_SILICON", (enumeration)SINGLE_CRYSTAL_SILICON, //Mono-crystalline in production and the most efficient, efficiency 0.15-0.17
				PT_KEYWORD, "MULTI_CRYSTAL_SILICON", (enumeration)MULTI_CRYSTAL_SILICON,
				PT_KEYWORD, "AMORPHOUS_SILICON", (enumeration)AMORPHOUS_SILICON,
				PT_KEYWORD, "THIN_FILM_GA_AS", (enumeration)THIN_FILM_GA_AS,
				PT_KEYWORD, "CONCENTRATOR", (enumeration)CONCENTRATOR,

			PT_enumeration,"power_type",PADDR(power_type_v),// this property is not used in the code. I recomend removing it from the code.
				PT_KEYWORD,"AC",(enumeration)AC,
				PT_KEYWORD,"DC",(enumeration)DC,

			PT_enumeration, "INSTALLATION_TYPE", PADDR(installation_type_v),// this property is not used in the code. I recomend removing it from the code.
			   PT_KEYWORD, "ROOF_MOUNTED", (enumeration)ROOF_MOUNTED,
               PT_KEYWORD, "GROUND_MOUNTED",(enumeration)GROUND_MOUNTED,

			PT_enumeration, "SOLAR_TILT_MODEL", PADDR(solar_model_tilt), PT_DESCRIPTION, "solar tilt model used to compute insolation values",
				PT_KEYWORD, "DEFAULT", (enumeration)LIUJORDAN,
				PT_KEYWORD, "SOLPOS", (enumeration)SOLPOS,
				PT_KEYWORD, "PLAYERVALUE", (enumeration)PLAYERVAL,

			PT_enumeration, "SOLAR_POWER_MODEL", PADDR(solar_power_model),
				PT_KEYWORD, "DEFAULT", (enumeration)BASEEFFICIENT,
				PT_KEYWORD, "FLATPLATE", (enumeration)FLATPLATE,

			PT_double, "a_coeff", PADDR(module_acoeff), PT_DESCRIPTION, "a coefficient for module temperature correction formula",
			PT_double, "b_coeff[s/m]", PADDR(module_bcoeff), PT_DESCRIPTION, "b coefficient for module temperature correction formula",
			PT_double, "dT_coeff[m*m*degC/kW]", PADDR(module_dTcoeff), PT_DESCRIPTION, "Temperature difference coefficient for module temperature correction formula",
			PT_double, "T_coeff[%/degC]", PADDR(module_Tcoeff), PT_DESCRIPTION, "Maximum power temperature coefficient for module temperature correction formula",

			PT_double, "NOCT[degF]", PADDR(NOCT), //Nominal operating cell temperature NOCT in deg F
            PT_double, "Tmodule[degF]", PADDR(Tmodule), //Temperature of PV module
			PT_double, "Tambient[degC]", PADDR(Tambient), //Ambient temperature for cell efficiency calculations
			PT_double, "wind_speed[mph]", PADDR(wind_speed), //Wind speed
			PT_double, "ambient_temperature[degF]", PADDR(Tamb), PT_DESCRIPTION, "Current ambient temperature of air",
			PT_double, "Insolation[W/sf]", PADDR(Insolation),
			PT_double, "Rinternal[Ohm]", PADDR(Rinternal),
			PT_double, "Rated_Insolation[W/sf]", PADDR(Rated_Insolation),
			PT_double, "Pmax_temp_coeff", PADDR(Pmax_temp_coeff),  //temp coefficient of rated Power in %/ deg C
            PT_double, "Voc_temp_coeff", PADDR(Voc_temp_coeff),
			PT_complex, "V_Max[V]", PADDR(V_Max), // Vmax of solar module found on specs
			PT_complex, "Voc_Max[V]", PADDR(Voc_Max),  //Voc max of solar module
			PT_complex, "Voc[V]", PADDR(Voc), 
			PT_double, "efficiency[unit]", PADDR(efficiency),
			PT_double, "area[sf]", PADDR(area),  //solar panel area
			PT_double, "soiling[pu]", PADDR(soiling_factor), PT_DESCRIPTION, "Soiling of array factor - representing dirt on array",
			PT_double, "derating[pu]", PADDR(derating_factor), PT_DESCRIPTION, "Panel derating to account for manufacturing variances",
			PT_double, "Tcell[degC]", PADDR(Tcell),

			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			PT_complex, "P_Out[kW]", PADDR(P_Out),
			PT_complex, "V_Out[V]", PADDR(V_Out),
			PT_complex, "I_Out[A]", PADDR(I_Out),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),
			PT_object, "weather", PADDR(weather),

			PT_double, "shading_factor[pu]", PADDR(shading_factor), PT_DESCRIPTION, "Shading factor for scaling solar power to the array",
			PT_double, "tilt_angle[deg]", PADDR(tilt_angle), PT_DESCRIPTION, "Tilt angle of PV array",
			PT_double, "orientation_azimuth[deg]", PADDR(orientation_azimuth), PT_DESCRIPTION, "Facing direction of the PV array",
			PT_bool, "latitude_angle_fix", PADDR(fix_angle_lat), PT_DESCRIPTION, "Fix tilt angle to installation latitude value",

			PT_enumeration, "orientation", PADDR(orientation_type),
				PT_KEYWORD, "DEFAULT", (enumeration)DEFAULT,
				PT_KEYWORD, "FIXED_AXIS", (enumeration)FIXED_AXIS,
				//PT_KEYWORD, "ONE_AXIS", ONE_AXIS,			//To be implemented later
				//PT_KEYWORD, "TWO_AXIS", TWO_AXIS,			//To be implemented later
				//PT_KEYWORD, "AZIMUTH_AXIS", AZIMUTH_AXIS,	//To be implemented later


			//resistances and max P, Q

			    PT_set, "phases", PADDR(phases),//Solar doesn't need phase information for anything.
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		defaults = this;
		memset(this,0,sizeof(solar));

	}
}

/* Object creation is called once for each object that is created by the core */
int solar::create(void) 
{
	//NOCT = 45; Tcell = 21; Tambient = 25;//degC ; Rated_Insolation = 1000;//rated insolation is 1000 W/m2 , taken from GE solar cell performance sheet
	// 1 sq m  = 10.764 sq ft.
	NOCT = 118.4; //degF
    Tcell = 21.0;  //degC
	Tambient = 25.0; //degC
	Tamb = 77;	//degF
	wind_speed = 0.0;
	Insolation = 0;
	Rinternal = 0.05;
	prevTemp = 15.0;	//Start at a reasonable ambient temp (degC) - default temp is 59 degF = 15 degC
	currTemp = 15.0;	//Start at a reasonable ambient temp (degC)
	prevTime = 0;
	Rated_Insolation = 92.902; //W/Sf for 1000 W/m2
    V_Max = complex (27.1,0);  // max. power voltage (Vmp) from GE solar cell performance charatcetristics
	Voc_Max = complex(34,0); //taken from GEPVp-200-M-Module performance characteristics
	Voc = complex (34,0);  //taken from GEPVp-200-M-Module performance characteristics
	P_Out = 0.0;

	area = 323; //sq f , 30m2
    
	//Defaults for flagging
	efficiency = 0;
	Pmax_temp_coeff = 0.0;
	Voc_temp_coeff  = 0.0;

	pSolarD = NULL;
	pSolarH = NULL;
	pSolarG = NULL;
	pAlbedo = NULL;
	pWindSpeed = NULL;

	module_acoeff = -2.81;		//Coefficients from Sandia database - represents 
	module_bcoeff = -0.0455;	//glass/cell/polymer sheet insulated back, raised structure mounting
	module_dTcoeff = 0.0;		
	module_Tcoeff = -0.5;		//%/C - default from SAM - appears to be a monocrystalline or polycrystalline silicon

	shading_factor = 1;					//By default, no shading
	tilt_angle = 45;					//45-deg angle by default
	orientation_azimuth = 180.0;		//Equator facing, by default - for LIUJORDAN model
	orientation_azimuth_corrected = 0;	//By default, still zero
	fix_angle_lat = false;				//By default, tilt angle fix not enabled (because ideal insolation, by default)

	soiling_factor = 0.95;				//Soiling assumed to block 5% solar irradiance
	derating_factor = 0.95;				//Manufacturing variations expected to remove 5% of energy

	orientation_type = DEFAULT;	//Default = ideal tracking
	solar_model_tilt = LIUJORDAN;	//"Classic" tilt model - from Duffie and Beckman (same as ETP inputs)
	solar_power_model = BASEEFFICIENT; //Use older power output calculation model - unsure where it came from

	//Null out the function pointers
	calc_solar_radiation = NULL;

	return 1; /* return 1 on success, 0 on failure */
}


/** Checks for climate object and maps the climate variables to the house object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 59 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
**/
int solar::init_climate()
{
	OBJECT *hdr = OBJECTHDR(this);
	OBJECT *obj = NULL;

	// link to climate data
	FINDLIST *climates = NULL;
	
	if (solar_model_tilt != PLAYERVAL)
	{
		if (weather!=NULL)
		{
			if(!gl_object_isa(weather, "climate")){
				// strcmp failure
				gl_error("weather property refers to a(n) \"%s\" object and not a climate object", weather->oclass->name);
				/*  TROUBLESHOOT
				While attempting to map a climate property, the solar array encountered an object that is not a climate object.
				Please check to make sure a proper climate object is present, and/or specified.  If the bug persists, please
				submit your code and a bug report via the trac website.
				*/
				return 0;
			}
			obj = weather;
		}
		else	//No weather specified, search
		{
			climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
			if (climates==NULL)
			{
				//Ensure weather is set to NULL - catch below
				weather = NULL;
			}
			else if (climates->hit_count==0)
			{
				//Ensure weather is set to NULL - catch below
				weather = NULL;
			}
			else //climate data must have been found
			{
				if (climates->hit_count>1)
				{
					gl_warning("solarpanel: %d climates found, using first one defined", climates->hit_count);
					/*  TROUBLESHOOT
					More than one climate object was found, so only the first one will be used by the solar array object
					*/
				}


				gl_verbose("solar init: climate data was found!");
				// force rank of object w.r.t climate
				obj = gl_find_next(climates,NULL);
				weather = obj;
			}
		}

		//Make sure it actually found one
		if (weather == NULL)
		{
			//Replicate above warning
			gl_warning("solarpanel: no climate data found, using static data");
			/*  TROUBLESHOOT
			No climate object was found and player mode was not enabled, so the solar array object
			is utilizing default values for all relevant weather variables.
			*/

			//default to mock data
			static double tout=59.0, rhout=0.75, solar=92.902, wsout=0.0, albdefault=0.2;
			pTout = &tout;
			pRhout = &rhout;
			pSolarD = &solar;	//Default all solar values to normal "optimal" 1000 W/m^2
			pSolarH = &solar;
			pSolarG = &solar;
			pAlbedo = &albdefault;
			pWindSpeed = &wsout;

			if (orientation_type==FIXED_AXIS)
			{
				GL_THROW("FIXED_AXIS requires a climate file!");
				/*  TROUBLESHOOT
				The FIXED_AXIS model for the PV array requires climate data to properly function.
				Please specify such data, or consider using a different tilt model.
				*/
			}
		}
		else if (!gl_object_isa(weather,"climate"))	//Semi redundant for "weather"
		{
			GL_THROW("weather object is not a climate object!");
			/*  TROUBLESHOOT
			The object specified for the weather property is not a climate object and will not work
			with the solar object.  Please specify a valid climate object, or let the solar object
			automatically connect.
			*/
		}
		else	//Must be a proper object
		{
			if((obj->flags & OF_INIT) != OF_INIT){
				char objname[256];
				gl_verbose("solar::init(): deferring initialization on %s", gl_name(obj, objname, 255));
				return 2; // defer
			}
			if (obj->rank<=hdr->rank)
				gl_set_dependent(obj,hdr);
		   
			pTout = (double*)GETADDR(obj,gl_get_property(obj,"temperature"));
//			pRhout = (double*)GETADDR(obj,gl_get_property(obj,"humidity"));	<---- Not used anywhere yet
			pSolarD = (double*)GETADDR(obj,gl_get_property(obj,"solar_direct"));
			pSolarH = (double*)GETADDR(obj,gl_get_property(obj,"solar_diffuse"));
			pSolarG = (double*)GETADDR(obj,gl_get_property(obj,"solar_global"));
			pAlbedo = (double*)GETADDR(obj,gl_get_property(obj,"ground_reflectivity"));
			pWindSpeed = (double*)GETADDR(obj,gl_get_property(obj,"wind_speed"));

			//Should probably check these
			if (pTout==NULL)
			{
				GL_THROW("Failed to map outside temperature");
				/*  TROUBLESHOOT
				The solar PV array failed to map the outside air temperature.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			//No need to error check - doesn't exist in any formulations yet
			//if (pRhout==NULL)
			//{
			//	GL_THROW("Failed to map outside relative humidity");
			//	/*  TROUBLESHOOT
			//	The solar PV array failed to map the outside relative humidity.  Ensure this is
			//	properly specified in your climate data and try again.
			//	*/
			//}

			if (pSolarD==NULL)
			{
				GL_THROW("Failed to map direct normal solar radiation");
				/*  TROUBLESHOOT
				The solar PV array failed to map the solar direct normal radiation.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			if (pSolarH==NULL)
			{
				GL_THROW("Failed to map diffuse horizontal solar radiation");
				/*  TROUBLESHOOT
				The solar PV array failed to map the solar diffuse horizontal radiation.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			if (pSolarG==NULL)
			{
				GL_THROW("Failed to map global horizontal solar radiation");
				/*  TROUBLESHOOT
				The solar PV array failed to map the solar global horizontal radiation.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			if (pAlbedo==NULL)
			{
				GL_THROW("Failed to map albedo/ground reflectance");
				/*  TROUBLESHOOT
				The solar PV array failed to map the ground reflectance.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			if (pWindSpeed==NULL)
			{
				GL_THROW("Failed to map wind speed");
				/*  TROUBLESHOOT
				The solar PV array failed to map the wind speed.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			//If climate data was found, check other related variables
			if (fix_angle_lat==true)
			{
				if (obj->latitude < 0)	//Southern hemisphere
				{
					//Get the latitude from the climate file
					tilt_angle = -obj->latitude;
				}
				else	//Northern
				{
					//Get the latitude from the climate file
					tilt_angle = obj->latitude;
				}
			}

			//Check the tilt angle for absurdity
			if (tilt_angle < 0)
			{
				GL_THROW("Invalid tilt_angle - tilt must be between 0 and 90 degrees");
				/*  TROUBLESHOOT
				A negative tilt angle was specified.  This implies the array is under the ground and will
				not receive any meaningful solar irradiation.  Please correct the tilt angle and try again.
				*/
			}
			else if (tilt_angle > 90.0)
			{
				GL_THROW("Invalid tilt angle - values above 90 degrees are unsupported!");
				/*  TROUBLESHOOT
				An tilt angle over 90 degrees (straight up and down) was specified.  Beyond this angle, the
				tilt algorithm does not function properly.  Please specific the tilt angle between 0 and 90 degrees
				and try again.
				*/
			}

			//Check the solar method
			if (orientation_type == FIXED_AXIS)
			{
				//See which function we want to use
				if (solar_model_tilt==LIUJORDAN)
				{
					//Map up the "classic" function
					calc_solar_radiation = (FUNCTIONADDR)(gl_get_function(obj,"calculate_solar_radiation_shading_degrees"));
				}
				else if (solar_model_tilt==SOLPOS)	//Use the solpos/Perez tilt model
				{
					//Map up the "classic" function
					calc_solar_radiation = (FUNCTIONADDR)(gl_get_function(obj,"calc_solpos_radiation_shading_degrees"));
				}
								
				//Make sure it was found
				if (calc_solar_radiation == NULL)
				{
					GL_THROW("Unable to map solar radiation function on %s in %s",obj->name,hdr->name);
					/*  TROUBLESHOOT
					While attempting to initialize the photovoltaic array mapping of the solar radiation function.
					Please try again.  If the bug persists, please submit your GLM and a bug report via the trac website.
					*/
				}

				//Check azimuth for absurdity as well
				if ((orientation_azimuth<0.0) || (orientation_azimuth > 360.0))
				{
					GL_THROW("orientation_azimuth must be a value representing a valid cardinal direction of 0 to 360 degrees!");
					/*  TROUBLESHOOT
					The orientation_azimuth property is expected values on the cardinal points degree system.  For this convention, 0 or
					360 is north, 90 is east, 180 is south, and 270 is west.  Please specify a direction within the 0 to 360 degree bound and try again.
					*/
				}

				//Map up our azimuth now too, if needed - Liu & Jordan model assumes 0 = equator facing
				if (solar_model_tilt == LIUJORDAN)
				{
					if (obj->latitude>0.0)	//North - "south" is equatorial facing
					{
						orientation_azimuth_corrected =  180.0 - orientation_azimuth;
					}
					else if (obj->latitude<0.0) //South - "north" is equatorial facing
					{
						gl_warning("solar:%s - Default solar position model is not recommended for southern hemisphere!",hdr->name);
						/*  TROUBLESHOOT
						The Liu-Jordan (default) solar position and tilt model was built around the northern
						hemisphere.  As such, operating in the southern hemisphere does not provide completely accurate
						results.  They are close, but tilted surfaces are not properly accounted for.  It is recommended
						that the SOLAR_TILT_MODEL SOLPOS be used for southern hemisphere operations.
						*/

						if ((orientation_azimuth >= 0.0) && (orientation_azimuth <= 180.0))
						{
							orientation_azimuth_corrected =  orientation_azimuth;	//East positive
						}
						else if (orientation_azimuth == 360.0) //Special case for those who like 360 as North
						{
							orientation_azimuth_corrected = 0.0;
						}
						else	//Must be west
						{
							orientation_azimuth_corrected = orientation_azimuth - 360.0;
						}
					}
					else	//Equator - erm....
					{
						GL_THROW("Exact equator location of array detected - unknown how to handle orientation");
						/*  TROUBLESHOOT
						The solar orientation algorithm implemented inside GridLAB-D does not understand how to orient
						itself for an array exactly on the equator.  Shift it north or south a little bit to get valid results.
						*/
					}
				}
				else	//Right now only SOLPOS, which is "correct" - if another is implemented, may need another case
					orientation_azimuth_corrected = orientation_azimuth;
			}
			//Defaulted else for now - don't do anything
		}//End valid weather - mapping check
	}
	else	//Player mode, just drop a message
	{
		gl_warning("Solar object:%s is in player mode - be sure to specify relevant values",hdr->name);
		/*  TROUBLESHOOT
		The solar array object is in player mode.  It will not take values from climate files or objects.
		Be sure to specify the Insolation, ambient_temperature, and wind_speed values as necessary.  It also
		will not incorporate any tilt functionality, since the Insolation value is expected to already include
		this adjustment.
		*/
	}

	return 1;
}

/* Object initialization is called once after all object have been created */
int solar::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	int climate_result;

	if (gen_mode_v == UNKNOWN)
	{
		gl_warning("Generator control mode is not specified! Using default: SUPPLY_DRIVEN");
		gen_mode_v = SUPPLY_DRIVEN;
	} else if(gen_mode_v == CONSTANT_V){
		gl_error("Generator control mode is CONSTANT_V. The Solar object only operates in SUPPLY_DRIVEN generator control mode.");
		return 0;
	} else if(gen_mode_v == CONSTANT_PQ){
		gl_error("Generator control mode is CONSTANT_PQ. The Solar object only operates in SUPPLY_DRIVEN generator control mode.");
		return 0;
	} else if(gen_mode_v == CONSTANT_PF){
		gl_error("Generator control mode is CONSTANT_PF. The Solar object only operates in SUPPLY_DRIVEN generator control mode.");
		return 0;
	}
	if (gen_status_v == UNKNOWN)
	{
		gl_warning("Solar panel status is unknown! Using default: ONLINE");
		gen_status_v = ONLINE;
	}
	if (panel_type_v == UNKNOWN)
	{
		gl_warning("Solar panel type is unknown! Using default: SINGLE_CRYSTAL_SILICON");
		panel_type_v = SINGLE_CRYSTAL_SILICON;
	}
	switch(panel_type_v)
	{
		case SINGLE_CRYSTAL_SILICON:
			if (efficiency==0.0)
				efficiency = 0.2;
          //  w1 = 0.942; // w1 is coeff for Tamb in deg C// convert to fahrenheit degF =degC+9/5+32
			if (Pmax_temp_coeff==0.0)
				Pmax_temp_coeff = -0.00437/33.8 ;  // average values from ref 2 in per degF
			if (Voc_temp_coeff==0.0)
			   Voc_temp_coeff  = -0.00393/33.8;
		   break;
		case MULTI_CRYSTAL_SILICON:
			if (efficiency==0.0)
				efficiency = 0.15;
			if (Pmax_temp_coeff==0.0)
				Pmax_temp_coeff = -0.00416/33.8; // average values from ref 2
			if (Voc_temp_coeff==0.0)
				Voc_temp_coeff  = -0.0039/33.8;
			break;
		case AMORPHOUS_SILICON:
			if (efficiency==0.0)
				efficiency = 0.07;
			if (Pmax_temp_coeff==0.0)
			   Pmax_temp_coeff = 0.1745/33.8; // average values from ref 2
			if (Voc_temp_coeff==0.0)
			   Voc_temp_coeff  = -0.00407/33.8;
		   break;
		case THIN_FILM_GA_AS:
			if (efficiency==0.0)
				efficiency = 0.3;
			break;
		case CONCENTRATOR:
			if (efficiency==0.0)
				efficiency = 0.15;
			break;
		default:
			if (efficiency == 0)
				efficiency = 0.10;
			break;
	}

	static complex default_line_voltage[1], default_line_current[1];
	int i;

	//efficiency dictates how much of the rate insolation the panel can capture and
	//turn into electricity
	//Rated power output
	Max_P = Rated_Insolation * efficiency * area; // We are calculating the module efficiency which should be less than cell efficiency. What about the sun hours??

	// find parent inverter, if not defined, use a default voltage
	if (parent != NULL && strcmp(parent->oclass->name,"inverter") == 0) // SOLAR has a PARENT and PARENT is an INVERTER
	{
		struct {
			complex **var;
			char *varname;
		} map[] = {
			// map the V & I from the inverter
			{&pCircuit_V,			"V_In"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"I_In"}, // assumes 2 and 3(N) follow immediately in memory
		};
		// construct circuit variable map to meter
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			*(map[i].var) = get_complex(parent,map[i].varname);
		}

		inverter *par = OBJECTDATA(obj->parent, inverter);

		if((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("solar::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}

		if(par->use_multipoint_efficiency == TRUE)
		{
			if(Max_P > par->p_dco){
				gl_warning("The PV is over rated for its parent inverter.");
				/*  TROUBLESHOOT
				The maximum output for the PV array is larger than the inverter rating.  Ensure this 
				was done intentionally.  If not, please correct your values and try again.
				*/
			}
		}
		else if(par->inverter_type_v == 4)
		{//four quadrant inverter
			if(par->number_of_phases_out == 4){
				if(Max_P > par->p_rated/par->inv_eta){
					gl_warning("The PV is over rated for its parent inverter.");
					//Defined above
				}
			} else {
				if(Max_P > par->number_of_phases_out*par->p_rated/par->inv_eta){
					gl_warning("The PV is over rated for its parent inverter.");
					//Defined above
				}
			}
		} else {
			if(par->number_of_phases_out == 4){
				if(Max_P > par->p_rated/par->efficiency){
					gl_warning("The PV is over rated for its parent inverter.");
					//Defined above
				}
			} else {
				if(Max_P > par->number_of_phases_out*par->p_rated/par->efficiency){
					gl_warning("The PV is over rated for its parent inverter.");
					//Defined above
				}
			}
		}
		//gl_verbose("Max_P is : %f", Max_P);
	}
	else if	(parent != NULL && strcmp(parent->oclass->name,"inverter") != 0)
	{
		GL_THROW("Solar panel can only have an inverter as its parent.");
		/* TROUBLESHOOT
		   The solar panel can only have an INVERTER as parent, and no other object. Or it can be all by itself, without a parent.
		*/
	}
	else
	{	// default values of voltage
		gl_warning("solar panel:%d has no parent defined. Using static voltages.", obj->id);
		struct {
			complex **var;
			char *varname;
		} map[] = {
			// map the V & I from the inverter
			{&pCircuit_V,			"V_In"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"I_In"}, // assumes 2 and 3(N) follow immediately in memory
		};
		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line_voltage[0];
		*(map[1].var) = &default_line_current[0];

		// provide initial values for voltages
		default_line_voltage[0] = complex(V_Max.Re()/sqrt(3.0),0);
        	//default_line_voltage[0] = V_Max/sqrt(3.0);
     
	}

	climate_result=init_climate();

	//Check factors
	if ((soiling_factor<0) || (soiling_factor>1.0))
	{
		soiling_factor = 0.95;
		gl_warning("Invalid soiling factor specified, defaulting to 95%");
		/*  TROUBLESHOOT
		A soiling factor less than zero or greater than 1.0 was specified.  This is not within the valid
		range, so a default of 0.95 was selected.
		*/
	}

	if ((derating_factor<0) || (derating_factor>1.0))
	{
		derating_factor = 0.95;
		gl_warning("Invalid derating factor specified, defaulting to 95%");
		/*  TROUBLESHOOT
		A derating factor less than zero or greater than 1.0 was specified.  This is not within the valid
		range, so a default of 0.95 was selected.
		*/
	}

	return climate_result; /* return 1 on success, 0 on failure */
}

TIMESTAMP solar::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	I_Out = complex(0,0);
	

	TIMESTAMP t2 = TS_NEVER;
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP solar::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	int64 ret_value;
	OBJECT *obj = OBJECTHDR(this);
	double insolwmsq, corrwindspeed, Tback, Ftempcorr;

	//Check the shading factor
	if ((shading_factor <0) || (shading_factor > 1))
	{
		GL_THROW("Shading factor outside [0 1] limits in %s",obj->name);
		/*  TROUBLESHOOT
		The shading factor for the solar device is set outside the limited range
		of 0 to 1.  Please set it back within this range and try again.
		*/
	}

	if (solar_model_tilt != PLAYERVAL)
	{
		//Update windspeed - since it's being read and has a variable
		wind_speed = *pWindSpeed;
		Tamb = *pTout;

		//Check our mode
		switch (orientation_type) {
			case DEFAULT:
				{
					Insolation = shading_factor*(*pSolarD) + *pSolarH + *pSolarG*(1 - cos(tilt_angle))*(*pAlbedo)/2.0;
					break;
				}
			case FIXED_AXIS:
				{
					//Snag solar insolation - prorate by shading (direct axis) - uses model selected earlier
					ret_value = ((int64 (*)(OBJECT *, double, double, double, double *))(*calc_solar_radiation))(weather,tilt_angle,orientation_azimuth_corrected,shading_factor,&Insolation);

					//Make sure it worked
					if (ret_value == 0)
					{
						GL_THROW("Calculation of solar radiation failed in %s",obj->name);
						/*  TROUBLESHOOT
						While calculating solar radiation in the photovoltaic object, an error
						occurred.  Please try again.  If the error persists, please submit your
						code and a bug report via the trac website.
						*/
					}

					break;
				}
			case ONE_AXIS:
			case TWO_AXIS:
			case AZIMUTH_AXIS:
			default:
				{
					GL_THROW("Unknown or unsupported orientation detected");
					/*  TROUBLESHOOT
					While attempting to calculate solar output, an unknown, or currently unimplemented,
					orientation was detected.  Please try again.
					*/
				}
		}
	}
	//Default else - pull in from published values (player driven)

	//Tmodule = w1 *Tamb + w2 * Insolation + w3* wind_speed + constant;
	if (Insolation < 0.0){
		Insolation = 0.0;
	}

	if (solar_power_model==BASEEFFICIENT)
	{
		Tambient = (Tamb-32.0)*5.0/9.0;  //Read Tamb into the variable - convert to degC for consistency (with below - where the algorithm is more complex and I'm too lazy to convert it to degF in MANY places)

		Tmodule = Tamb + (NOCT-68)/74.32 * Insolation;   //74.32 sf = 800 W/m2; 68 deg F = 20deg C
	     
		 VA_Out = Max_P * Insolation/Rated_Insolation *(1+(Pmax_temp_coeff)*( Tmodule-77))*derating_factor*soiling_factor; //derating due to manufacturing tolerance, derating sue to soiling both dimensionless
	}
	else if (solar_power_model==FLATPLATE)	//Flat plate efficiency
	{
		//Approach taken from NREL SAM documentation for flat plate efficiency model - unclear on its origins

		//Cycle temperature differences through if using flat efficiency model
		if (prevTime != t0)
		{
			prevTemp = currTemp;	//Current becomes previous

			currTemp = (Tamb-32.0)*5.0/9.0;  //Convert current temperature back to metric

			prevTime = t0;			//Record current timestep
		}

		//Get the "ambient" temperature of the array - by SAM algorithm, taken as
		//halfway between last intervals (linear interpolation)
		Tambient = prevTemp + (currTemp-prevTemp)/2.0;

		//Impose numerical error by converting things back into metric
		//First put insolation back into W/m^2 - factor in soiling at this point
		insolwmsq = Insolation*10.7639104*soiling_factor;	//Convert to W/m^2
		
		//Convert wind speed from mph to m/s
		corrwindspeed = wind_speed*0.44704;
		
		//Calculate the "back" temperature of the array
		Tback = (insolwmsq*exp(module_acoeff + module_bcoeff*corrwindspeed) + Tambient);

		//Now compute the cell temperature, based on the back temperature
		Tcell = Tback + insolwmsq/1000.0*module_dTcoeff;

		//TCell is assumed to be Tmodule from old calculations (used in voltage below) - convert back to Fahrenheit
		Tmodule=Tcell*9.0/5.0 + 32.0;

		//Calculate temperature correction value
		Ftempcorr = 1.0 + module_Tcoeff*(Tcell-25.0)/100.0;

		//Place into the DC value - factor in area, derating, and all of the related items
		P_Out = Insolation*soiling_factor*derating_factor*area*efficiency*Ftempcorr;
		
		//Populate VA, just because it seems to be used below
		VA_Out = P_Out;
	}
	else
	{
		GL_THROW("Unknown solar power output model selected!");
		/*  TROUBLESHOOT
		An unknown value was put in for the solar power output model.  Please
		choose a correct value.  If the value is correct and the error persists, please
		submit your code and a bug report via the trac website.
		*/
	}

	 Voc = Voc_Max * (1+(Voc_temp_coeff)*(Tmodule-77));
			    
	 V_Out = V_Max * (Voc/Voc_Max);

     I_Out = (VA_Out/V_Out);

	 *pLine_I =I_Out;
     *pCircuit_V =V_Out;

	return TS_NEVER; 
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP solar::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	

return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

complex *solar::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_solar(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(solar::oclass);
		if (*obj!=NULL)
		{
			solar *my = OBJECTDATA(*obj,solar);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	} 
	CREATE_CATCHALL(solar);
}

EXPORT int init_solar(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,solar)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(solar);
}

EXPORT TIMESTAMP sync_solar(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	solar *my = OBJECTDATA(obj,solar);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
	}
	SYNC_CATCHALL(solar);
	return t2;
}
