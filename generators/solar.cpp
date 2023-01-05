/**
 * // Assumptions:
1. All solar panels are tilted as per the site latitude to perform at their best efficiency
2. All the solar cells are connected in series in a solar module
3. 600Volts, 5/7.6 Amps, 200 Watts PV system is used for all residential , commercial and industrial applications. The number of modules will vary based on the surface area
4. A power derating of 10-15% is applied to take account of power losses and conversion in-efficiencies of the inverter.

// References:
1. Photovoltaic Module Thermal/Wind performance: Long-term monitoring and Model development for energy rating , Solar program review meeting 2003, Govindswamy Tamizhmani et al
2. COMPARISON OF ENERGY PRODUCTION AND PERFORMANCE FROM FLAT-PLATE PHOTOVOLTAIC MODULE TECHNOLOGIES DEPLOYED AT FIXED TILT, J.A. del Cueto
3. Solar Collectors and Photovoltaic in energyPRO
4. Calculation of the polycrystalline PV module temperature using a simple method of energy balance
**/

#include "solar.h"

#include <cassert>
#include <cmath>
#include <climits>
#include <iostream>

using namespace std;

#define DEG_TO_RAD(x) (x * M_PI) / 180.0
#define WPM2_TO_WPFT2(x) x / 10.7639
#define WPFT2_TO_WPM2(x) x * 10.7639
#define FAHR_TO_CELS(x) (x - 32.0) * 5.0 / 9.0
#define CELS_TO_FAHR(x) x * 9.0 / 5.0 + 32.0

#define PV_CURVE_PARAM_RATIO 1.2

/* Framework */
CLASS *solar::oclass = nullptr;
solar *solar::defaults = nullptr;

static PASSCONFIG passconfig = PC_BOTTOMUP | PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
solar::solar(MODULE *module)
{
	if (oclass == nullptr)
	{
		oclass = gl_register_class(module, "solar", sizeof(solar), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == nullptr)
			throw "unable to register class solar";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			// Flag for Mode Section of Calculating pvc_Pmax
			PT_bool, "pvc_Pmax_calc_simp_mode", PADDR(pvc_Pmax_calc_simp_mode), PT_DESCRIPTION, "If the simple mode is selected, the pvc_Pmax = pvc_U_m_V * pvc_I_m_A.",

			// Solar PV Panel (under PV_CURVE Mode)
			PT_double, "t_ref_cels[degC]", PADDR(pvc_t_ref_cels), PT_DESCRIPTION, "The referenced temperature",
			PT_double, "S_ref_wpm2[W/m^2]", PADDR(pvc_S_ref_wpm2), PT_DESCRIPTION, "The referenced insolation",

			PT_double, "pvc_a1_inv_cels", PADDR(pvc_a1), PT_DESCRIPTION, "In the calculation of dI, this is the coefficient that adjusts the difference between t (temperature) and pvc_t_ref_cels (referenced temperature)",
			PT_double, "pvc_b1_inv_cels", PADDR(pvc_b1), PT_DESCRIPTION, "In the calculation of dU, this is the coefficient that adjusts the difference between t (temperature) and pvc_t_ref_cels (referenced temperature)",

			PT_double, "pvc_U_oc_V[V]", PADDR(pvc_U_oc_V), PT_DESCRIPTION, "The open circuit voltage",
			PT_double, "pvc_I_sc_A[A]", PADDR(pvc_I_sc_A), PT_DESCRIPTION, "The short circuit current",
			PT_double, "pvc_U_m_V[V]", PADDR(pvc_U_m_V), PT_DESCRIPTION, "Voltage at maximum power point",
			PT_double, "pvc_I_m_A[A]", PADDR(pvc_I_m_A), PT_DESCRIPTION, "Current at maximum power point",

			// N-R Solver
			PT_int16, "MAX_NR_ITERATIONS", PADDR(max_nr_ite), PT_DESCRIPTION, "The allowed maximum number of newton-raphson itrations",
			PT_double, "x0_root_rt", PADDR(x0_root_rt), PT_DESCRIPTION, "Set the initial guess at this extra percentage of the absolute x value at the extreme point",
			PT_double, "DOA_NR_ITERATIONS", PADDR(eps_nr_ite), PT_DESCRIPTION, "Set the degree of accuracy of newton-raphson method",

			PT_enumeration, "panel_type", PADDR(panel_type_v),
			PT_KEYWORD, "SINGLE_CRYSTAL_SILICON", (enumeration)SINGLE_CRYSTAL_SILICON, //Mono-crystalline in production and the most efficient, efficiency 0.15-0.17
			PT_KEYWORD, "MULTI_CRYSTAL_SILICON", (enumeration)MULTI_CRYSTAL_SILICON,
			PT_KEYWORD, "AMORPHOUS_SILICON", (enumeration)AMORPHOUS_SILICON,
			PT_KEYWORD, "THIN_FILM_GA_AS", (enumeration)THIN_FILM_GA_AS,
			PT_KEYWORD, "CONCENTRATOR", (enumeration)CONCENTRATOR,

			PT_enumeration, "SOLAR_TILT_MODEL", PADDR(solar_model_tilt), PT_DESCRIPTION, "solar tilt model used to compute insolation values",
			PT_KEYWORD, "DEFAULT", (enumeration)LIUJORDAN,
			PT_KEYWORD, "SOLPOS", (enumeration)SOLPOS,
			PT_KEYWORD, "PLAYERVALUE", (enumeration)PLAYERVAL,

			PT_enumeration, "SOLAR_POWER_MODEL", PADDR(solar_power_model),
			PT_KEYWORD, "DEFAULT", (enumeration)BASEEFFICIENT,
			PT_KEYWORD, "FLATPLATE", (enumeration)FLATPLATE,
			PT_KEYWORD, "PV_CURVE", (enumeration)PV_CURVE,

			PT_double, "a_coeff", PADDR(module_acoeff), PT_DESCRIPTION, "a coefficient for module temperature correction formula",
			PT_double, "b_coeff[s/m]", PADDR(module_bcoeff), PT_DESCRIPTION, "b coefficient for module temperature correction formula",
			PT_double, "dT_coeff[m*m*degC/kW]", PADDR(module_dTcoeff), PT_DESCRIPTION, "Temperature difference coefficient for module temperature correction formula",
			PT_double, "T_coeff[%/degC]", PADDR(module_Tcoeff), PT_DESCRIPTION, "Maximum power temperature coefficient for module temperature correction formula",

			PT_double, "NOCT[degF]", PADDR(NOCT),			 //Nominal operating cell temperature NOCT in deg F
			PT_double, "Tmodule[degF]", PADDR(Tmodule),		 //Temperature of PV module
			PT_double, "Tambient[degC]", PADDR(Tambient),	 //Ambient temperature for cell efficiency calculations
			PT_double, "wind_speed[mph]", PADDR(wind_speed), //Wind speed
			PT_double, "ambient_temperature[degF]", PADDR(Tamb), PT_DESCRIPTION, "Current ambient temperature of air",
			PT_double, "Insolation[W/sf]", PADDR(Insolation),
			PT_double, "Rinternal[Ohm]", PADDR(Rinternal),
			PT_double, "Rated_Insolation[W/sf]", PADDR(Rated_Insolation),
			PT_double, "Pmax_temp_coeff", PADDR(Pmax_temp_coeff), //temp coefficient of rated Power in %/ deg C
			PT_double, "Voc_temp_coeff", PADDR(Voc_temp_coeff),
			PT_double, "V_Max[V]", PADDR(V_Max),	 // Vmax of solar module found on specs
			PT_double, "Voc_Max[V]", PADDR(Voc_Max), //Voc max of solar module
			PT_double, "Voc[V]", PADDR(Voc),
			PT_double, "efficiency[unit]", PADDR(efficiency),
			PT_double, "area[sf]", PADDR(area), //solar panel area
			PT_double, "soiling[pu]", PADDR(soiling_factor), PT_DESCRIPTION, "Soiling of array factor - representing dirt on array",
			PT_double, "derating[pu]", PADDR(derating_factor), PT_DESCRIPTION, "Panel derating to account for manufacturing variances",
			PT_double, "Tcell[degC]", PADDR(Tcell),

			PT_double, "rated_power[W]", PADDR(Max_P), PT_DESCRIPTION, "Used to define the size of the solar panel in power rather than square footage.",
			PT_double, "P_Out[kW]", PADDR(P_Out),
			PT_double, "V_Out[V]", PADDR(V_Out),
			PT_double, "I_Out[A]", PADDR(I_Out),
			PT_object, "weather", PADDR(weather),

			PT_double, "shading_factor[pu]", PADDR(shading_factor), PT_DESCRIPTION, "Shading factor for scaling solar power to the array",
			PT_double, "tilt_angle[deg]", PADDR(tilt_angle), PT_DESCRIPTION, "Tilt angle of PV array",
			PT_double, "orientation_azimuth[deg]", PADDR(orientation_azimuth), PT_DESCRIPTION, "Facing direction of the PV array",
			PT_bool, "latitude_angle_fix", PADDR(fix_angle_lat), PT_DESCRIPTION, "Fix tilt angle to installation latitude value",

			PT_double, "default_voltage_variable", PADDR(default_voltage_array), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Accumulator/placeholder for default voltage value, when solar is run without an inverter",
			PT_double, "default_current_variable", PADDR(default_current_array), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Accumulator/placeholder for default current value, when solar is run without an inverter",
			PT_double, "default_power_variable", PADDR(default_power_array), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Accumulator/placeholder for default power value, when solar is run without an inverter",

			PT_enumeration, "orientation", PADDR(orientation_type),
			PT_KEYWORD, "DEFAULT", (enumeration)DEFAULT,
			PT_KEYWORD, "FIXED_AXIS", (enumeration)FIXED_AXIS,
			//PT_KEYWORD, "ONE_AXIS", ONE_AXIS,			//To be implemented later
			//PT_KEYWORD, "TWO_AXIS", TWO_AXIS,			//To be implemented later
			//PT_KEYWORD, "AZIMUTH_AXIS", AZIMUTH_AXIS,	//To be implemented later
			NULL) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		//Deltamode linkage
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_solar) == nullptr)
			GL_THROW("Unable to publish solar deltamode function");
		if (gl_publish_function(oclass, "DC_gen_object_update", (FUNCTIONADDR)dc_object_update_solar) == nullptr)
			GL_THROW("Unable to publish solar DC object update function");

		defaults = this;
		memset(this, 0, sizeof(solar));
	}
}

/* Object creation is called once for each object that is created by the core */
int solar::create(void)
{
	pvc_Pmax_calc_simp_mode = true;

	NOCT = 118.4;	 //degF
	Tcell = 21.0;	 //degC
	Tambient = 25.0; //degC
	Tamb = 77;		 //degF
	wind_speed = 0.0;
	Insolation = 0;
	Rinternal = 0.05;
	prevTemp = 15.0; //Start at a reasonable ambient temp (degC) - default temp is 59 degF = 15 degC
	currTemp = 15.0; //Start at a reasonable ambient temp (degC)
	prevTime = 0;
	Rated_Insolation = 92.902; //W/Sf for 1000 W/m2
	V_Max = 27.1;			   // max. power voltage (Vmp) from GE solar cell performance charatcetristics
	Voc_Max = 34.0;			   //taken from GEPVp-200-M-Module performance characteristics
	Voc = 34.0;				   //taken from GEPVp-200-M-Module performance characteristics
	P_Out = 0.0;

	area = 0; //sq f , 30m2

	//Defaults for flagging
	efficiency = 0;
	Pmax_temp_coeff = 0.0;
	Voc_temp_coeff = 0.0;

	//Property values - NULL out
	pTout = nullptr;
	pWindSpeed = nullptr;

	module_acoeff = -2.81;	 //Coefficients from Sandia database - represents
	module_bcoeff = -0.0455; //glass/cell/polymer sheet insulated back, raised structure mounting
	module_dTcoeff = 0.0;
	module_Tcoeff = -0.5; //%/C - default from SAM - appears to be a monocrystalline or polycrystalline silicon

	shading_factor = 1;				   //By default, no shading
	tilt_angle = 45;				   //45-deg angle by default
	orientation_azimuth = 180.0;	   //Equator facing, by default - for LIUJORDAN model
	orientation_azimuth_corrected = 0; //By default, still zero
	fix_angle_lat = false;			   //By default, tilt angle fix not enabled (because ideal insolation, by default)

	soiling_factor = 0.95;	//Soiling assumed to block 5% solar irradiance
	derating_factor = 0.95; //Manufacturing variations expected to remove 5% of energy

	orientation_type = DEFAULT;		   //Default = ideal tracking
	solar_model_tilt = LIUJORDAN;	   //"Classic" tilt model - from Duffie and Beckman (same as ETP inputs)
	solar_power_model = BASEEFFICIENT; //Use older power output calculation model - unsure where it came from

	//Null out the function pointers
	calc_solar_radiation = nullptr;

	//Deltamode flags
	deltamode_inclusive = false;
	first_sync_delta_enabled = false;

	//Inverter pointers
	inverter_voltage_property = nullptr;
	inverter_current_property = nullptr;
	inverter_power_property = nullptr;
	inverter_rated_power_va_property = nullptr;
	inverter_rated_dc_voltage = nullptr;

	//Default versions
	default_voltage_array = 0.0;
	default_current_array = 0.0;
	default_power_array = 0.0;

	//Initialize DC trackng variables
	last_DC_current = 0.0;
	last_DC_power = 0.0;

	//Initialize PV DC model parameters - cuts down on warning messages
	max_nr_ite = SHRT_MAX;
	x0_root_rt = 0.15; //Set the initial guess at 15% extra of the absolute value of the extreme point
	eps_nr_ite = 1e-5;
	pvc_t_ref_cels = 25; //Unit: Celsius
	pvc_S_ref_wpm2 = 1e3; //Unit: w/m^2
	pvc_a1 = 0;
	pvc_b1 = 0;
	pvc_U_oc_V = 0; //1005;
	pvc_I_sc_A = 0; //1e2;
	pvc_U_m_V = 0; //750;
	pvc_I_m_A = 0; //84;

	//Init rated_power
	Max_P = 0;

	return 1; /* return 1 on success, 0 on failure */
}

/** Checks for climate object and maps the climate variables to the house object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 59 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
**/
int solar::init_climate()
{
	OBJECT *hdr = OBJECTHDR(this);
	OBJECT *obj = nullptr;

	// link to climate data
	FINDLIST *climates = nullptr;

	if (solar_model_tilt != PLAYERVAL)
	{
		if (weather != nullptr)
		{
			if (!gl_object_isa(weather, "climate"))
			{
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
		else //No weather specified, search
		{
			climates = gl_find_objects(FL_NEW, FT_CLASS, SAME, "climate", FT_END);
			if (climates == nullptr)
			{
				//Ensure weather is set to NULL - catch below
				weather = nullptr;
			}
			else if (climates->hit_count == 0)
			{
				//Ensure weather is set to NULL - catch below
				weather = nullptr;
			}
			else //climate data must have been found
			{
				if (climates->hit_count > 1)
				{
					gl_warning("solarpanel: %d climates found, using first one defined", climates->hit_count);
					/*  TROUBLESHOOT
					More than one climate object was found, so only the first one will be used by the solar array object
					*/
				}

				gl_verbose("solar init: climate data was found!");
				// force rank of object w.r.t climate
				obj = gl_find_next(climates, NULL);
				weather = obj;
			}

			//Free up the list
			gl_free(climates);
		}

		//Make sure it actually found one
		if (weather == nullptr)
		{
			//Replicate above warning
			gl_warning("solarpanel: no climate data found, using static data");
			/*  TROUBLESHOOT
			No climate object was found and player mode was not enabled, so the solar array object
			is utilizing default values for all relevant weather variables.
			*/

			//default to mock data - for the two fields that exist (temperature and windspeed)
			//All others just put in the one equation that uses them
			Tamb = 59.0;
			wind_speed = 0.0;

			if (orientation_type == FIXED_AXIS)
			{
				GL_THROW("FIXED_AXIS requires a climate file!");
				/*  TROUBLESHOOT
				The FIXED_AXIS model for the PV array requires climate data to properly function.
				Please specify such data, or consider using a different tilt model.
				*/
			}
		}
		else if (!gl_object_isa(weather, "climate")) //Semi redundant for "weather"
		{
			GL_THROW("weather object is not a climate object!");
			/*  TROUBLESHOOT
			The object specified for the weather property is not a climate object and will not work
			with the solar object.  Please specify a valid climate object, or let the solar object
			automatically connect.
			*/
		}
		else //Must be a proper object
		{
			//Deferred init used to be here - handled in mainline init now

			//Check our rank
			if (obj->rank <= hdr->rank)
				gl_set_dependent(obj, hdr);

			//Check and see if we have a lat/long set -- if not, pull the one from the climate
			if (isnan(hdr->longitude))
			{
				//Pull the value from the climate
				hdr->longitude = obj->longitude;
			}

			//Do the same for latitude
			if (isnan(hdr->latitude))
			{
				//Pull the value from the climate
				hdr->latitude = obj->latitude;
			}

			//Map the properties - temperature
			pTout = new gld_property(obj, "temperature");

			//Check it
			if (!pTout->is_valid() || !pTout->is_double())
			{
				GL_THROW("solar:%d - %s - Failed to map outside temperature", hdr->id, (hdr->name ? hdr->name : "Unnamed"));
				/*  TROUBLESHOOT
				The solar PV array failed to map the outside air temperature.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			//Map the wind speed
			pWindSpeed = new gld_property(obj, "wind_speed");

			//Check tit
			if (!pWindSpeed->is_valid() || !pWindSpeed->is_double())
			{
				GL_THROW("solar:%d - %s - Failed to map wind speed", hdr->id, (hdr->name ? hdr->name : "Unnamed"));
				/*  TROUBLESHOOT
				The solar PV array failed to map the wind speed.  Ensure this is
				properly specified in your climate data and try again.
				*/
			}

			//If climate data was found, check other related variables
			if (fix_angle_lat)
			{
				if (hdr->latitude < 0) //Southern hemisphere
				{
					//Get the latitude from the climate file
					tilt_angle = -hdr->latitude;
				}
				else //Northern
				{
					//Get the latitude from the climate file
					tilt_angle = hdr->latitude;
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

			// Check the solar method
			// CDC: This method of determining solar model based on tracking type seemed flawed. They should be independent of each other.
			if (orientation_type == DEFAULT)
			{
				calc_solar_radiation = (FUNCTIONADDR)(gl_get_function(obj, "calc_solar_ideal_shading_position_radians"));
			}
			else if (orientation_type == FIXED_AXIS)
			{
				//See which function we want to use
				if (solar_model_tilt == LIUJORDAN)
				{
					//Map up the "classic" function
					calc_solar_radiation = (FUNCTIONADDR)(gl_get_function(obj, "calculate_solar_radiation_shading_position_radians"));
				}
				else if (solar_model_tilt == SOLPOS) //Use the solpos/Perez tilt model
				{
					//Map up the updated function
					calc_solar_radiation = (FUNCTIONADDR)(gl_get_function(obj, "calculate_solpos_radiation_shading_position_radians"));
				}

				//Make sure it was found
				if (calc_solar_radiation == nullptr)
				{
					GL_THROW("Unable to map solar radiation function on %s in %s", obj->name, hdr->name);
					/*  TROUBLESHOOT
					While attempting to initialize the photovoltaic array mapping of the solar radiation function.
					Please try again.  If the bug persists, please submit your GLM and a bug report via the trac website.
					*/
				}

				//Check azimuth for absurdity as well
				if ((orientation_azimuth < 0.0) || (orientation_azimuth > 360.0))
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
					if (obj->latitude > 0.0) //North - "south" is equatorial facing
					{
						orientation_azimuth_corrected = 180.0 - orientation_azimuth;
					}
					else if (obj->latitude < 0.0) //South - "north" is equatorial facing
					{
						gl_warning("solar:%s - Default solar position model is not recommended for southern hemisphere!", hdr->name);
						/*  TROUBLESHOOT
						The Liu-Jordan (default) solar position and tilt model was built around the northern
						hemisphere.  As such, operating in the southern hemisphere does not provide completely accurate
						results.  They are close, but tilted surfaces are not properly accounted for.  It is recommended
						that the SOLAR_TILT_MODEL SOLPOS be used for southern hemisphere operations.
						*/

						if ((orientation_azimuth >= 0.0) && (orientation_azimuth <= 180.0))
						{
							orientation_azimuth_corrected = orientation_azimuth; //East positive
						}
						else if (orientation_azimuth == 360.0) //Special case for those who like 360 as North
						{
							orientation_azimuth_corrected = 0.0;
						}
						else //Must be west
						{
							orientation_azimuth_corrected = orientation_azimuth - 360.0;
						}
					}
					else //Equator - erm....
					{
						GL_THROW("Exact equator location of array detected - unknown how to handle orientation");
						/*  TROUBLESHOOT
						The solar orientation algorithm implemented inside GridLAB-D does not understand how to orient
						itself for an array exactly on the equator.  Shift it north or south a little bit to get valid results.
						*/
					}
				}
				else
				{ //Right now only SOLPOS, which is "correct" - if another is implemented, may need another case
					orientation_azimuth_corrected = orientation_azimuth;
				}
			}
			//Defaulted else for now - don't do anything
		} //End valid weather - mapping check
	}
	else //Player mode, just drop a message
	{
		gl_warning("Solar object:%s is in player mode - be sure to specify relevant values", hdr->name);
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
	gld_property *temp_property_pointer = nullptr;
	gld_wlock *test_rlock = nullptr;
	bool temp_bool_val;
	double temp_double_val, temp_inv_p_rated, temp_p_efficiency, temp_p_eta;
	double temp_double_div_value_eta, temp_double_div_value_eff;
	enumeration temp_enum_val;
	int32 temp_phase_count_val;
	FUNCTIONADDR temp_fxn = nullptr;
	STATUS fxn_return_status;

	if (parent != nullptr)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("solar::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	if (panel_type_v == UNKNOWN)
	{
		gl_warning("Solar panel type is unknown! Using default: SINGLE_CRYSTAL_SILICON");
		panel_type_v = SINGLE_CRYSTAL_SILICON;
	}
	switch (panel_type_v)
	{
	case SINGLE_CRYSTAL_SILICON:
		if (efficiency == 0.0)
			efficiency = 0.2;
		if (Pmax_temp_coeff == 0.0)
			Pmax_temp_coeff = -0.00437 / 33.8; // average values from ref 2 in per degF
		if (Voc_temp_coeff == 0.0)
			Voc_temp_coeff = -0.00393 / 33.8;
		break;
	case MULTI_CRYSTAL_SILICON:
		if (efficiency == 0.0)
			efficiency = 0.15;
		if (Pmax_temp_coeff == 0.0)
			Pmax_temp_coeff = -0.00416 / 33.8; // average values from ref 2
		if (Voc_temp_coeff == 0.0)
			Voc_temp_coeff = -0.0039 / 33.8;
		break;
	case AMORPHOUS_SILICON:
		if (efficiency == 0.0)
			efficiency = 0.07;
		if (Pmax_temp_coeff == 0.0)
			Pmax_temp_coeff = 0.1745 / 33.8; // average values from ref 2
		if (Voc_temp_coeff == 0.0)
			Voc_temp_coeff = -0.00407 / 33.8;
		break;
	case THIN_FILM_GA_AS:
		if (efficiency == 0.0)
			efficiency = 0.3;
		break;
	case CONCENTRATOR:
		if (efficiency == 0.0)
			efficiency = 0.15;
		break;
	default:
		if (efficiency == 0)
			efficiency = 0.10;
		break;
	}

	//efficiency dictates how much of the rate insolation the panel can capture and
	//turn into electricity
	//Rated power output
	if (solar_power_model != PV_CURVE) //Get rid of the warning when PV_CURVE mode is used
	{
		if (Max_P == 0)
		{
			Max_P = Rated_Insolation * efficiency * area; // We are calculating the module efficiency which should be less than cell efficiency. What about the sun hours??
			gl_verbose("solar:%d %s - Max_P was not specified.  Calculating from other defaults.",obj->id,(obj->name?obj->name:"Unnamed"));
			/* TROUBLESHOOT
			The relationship between power output and other physical variables is described by Max_P = Rated_Insolation * efficiency * area. Since Max_P
			was not set, using this equation to calculate it.
			*/

			if (Max_P == 0)
			{
				gl_warning("solar:%d %s - Max_P and {area or Rated_Insolation or effiency} were not specified or specified as zero.  Leads to maximum power output of 0.",obj->id,(obj->name?obj->name:"Unnamed"));
				/* TROUBLESHOOT
				The relationship between power output and other physical variables is described by Max_P = Rated_Insolation * efficiency * area. Since Max_P
				was not specified and this equation leads to a value of zero, the output of the model is likely to be no power at all times.
				*/
			}
		}
		else
	{
		if (efficiency != 0 && area != 0 && Rated_Insolation != 0)
		{
			double temp = Rated_Insolation * efficiency * area;

			if (temp < Max_P * .99 || temp > Max_P * 1.01)
			{
				Max_P = temp;
				gl_warning("solar:%d %s - Max_P is not within 99% to 101% of Rated_Insolation * efficiency * area.  Ignoring Max_P.",obj->id,(obj->name?obj->name:"Unnamed"));
				/* TROUBLESHOOT
				The relationship between power output and other physical variables is described by Max_P = Rated_Insolation * efficiency * area. However, the model
				can be overspecified. In the case that it is, we have defaulted to old versions of GridLAB-D and ignored the Max_P and re-calculated it using
				the other values.  If you would like to use Max_P, please only specify Rated_Insolation and Max_P.
				*/
			}
		}
	}
	}

	if (Rated_Insolation == 0)
	{
		if (area != 0 && efficiency != 0)
			Rated_Insolation = Max_P / area / efficiency;
		else
		{
			gl_error("solar:%d %s - Rated Insolation was not specified (or zero).  Power outputs cannot be calculated without a rated insolation value.",obj->id,(obj->name?obj->name:"Unnamed"));
			/* TROUBLESHOOT
			The relationship is described by Rated_Insolation = Max_P / area / efficiency.  Nonezero values of area
			and efficiency are required for this calculation.  Please specify both.
			*/
		}
	}

	// find parent inverter, if not defined, use a default voltage
	if (parent != nullptr)
	{
		if (gl_object_isa(parent, "inverter", "generators")) // SOLAR has a PARENT and PARENT is an INVERTER - old-school inverter
		{
			//Map the inverter voltage
			inverter_voltage_property = new gld_property(parent, "V_In");

			//Check it
			if (!inverter_voltage_property->is_valid() || !inverter_voltage_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map to one of the inverter interface variables, an error occurred.  Please try again.
				If the error persists, please submit a bug report and your model file via the issue tracking system.
				*/
			}

			//Map the inverter current
			inverter_current_property = new gld_property(parent, "I_In");

			//Check it
			if (!inverter_current_property->is_valid() || !inverter_current_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the inverter power value
			inverter_power_property = new gld_property(parent, "P_In");

			//Check it
			if (!inverter_power_property->is_valid() || !inverter_power_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Find multipoint efficiency property to check
			temp_property_pointer = new gld_property(parent, "use_multipoint_efficiency");

			//Check and make sure it is valid
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While mapping a property of the inverter parented to a solar object, an error occurred.  Please
				try again.
				*/
			}

			//Pull the property
			temp_property_pointer->getp<bool>(temp_bool_val, *test_rlock);

			//Remove the property
			delete temp_property_pointer;

			//Pull the maximum_dc_power property
			temp_property_pointer = new gld_property(parent, "maximum_dc_power");

			//Check it
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Pull the value
			temp_double_val = temp_property_pointer->get_double();

			//Remove it
			delete temp_property_pointer;

			//Pull the inverter_type property
			temp_property_pointer = new gld_property(parent, "inverter_type");

			//Check it
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Pull the value
			temp_enum_val = temp_property_pointer->get_enumeration();

			//Remove the property
			delete temp_property_pointer;

			//Pull the number of phases
			temp_property_pointer = new gld_property(parent, "number_of_phases_out");

			//Check it
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_integer())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Pull the value
			temp_phase_count_val = temp_property_pointer->get_integer();

			//Remove the property
			delete temp_property_pointer;

			//Retrieve the rated_power property
			temp_property_pointer = new gld_property(parent, "rated_power");

			//Check it
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Pull the value
			temp_inv_p_rated = temp_property_pointer->get_double();

			//Remove it
			delete temp_property_pointer;

			//Pull efficiency - efficiency_value
			temp_property_pointer = new gld_property(parent, "efficiency_value");

			//Check it
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Pull the value
			temp_p_efficiency = temp_property_pointer->get_double();

			//Remove it
			delete temp_property_pointer;

			//Pull inv_eta - "inverter_efficiency"
			temp_property_pointer = new gld_property(parent, "inverter_efficiency");

			//Check it
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter property", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Pull the value
			temp_p_eta = temp_property_pointer->get_double();

			//Remove it
			delete temp_property_pointer;

			//Create some intermediate values
			temp_double_div_value_eta = temp_inv_p_rated / temp_p_eta;
			temp_double_div_value_eff = temp_inv_p_rated / temp_p_efficiency;

			if (temp_bool_val)
			{
				if (Max_P > temp_double_val)
				{
					gl_warning("The PV is over rated for its parent inverter.");
					/*  TROUBLESHOOT
					The maximum output for the PV array is larger than the inverter rating.  Ensure this 
					was done intentionally.  If not, please correct your values and try again.
					*/
				}
			}
			else if (temp_enum_val == 4)
			{ //four quadrant inverter
				if (temp_phase_count_val == 4)
				{
					if (Max_P > temp_double_div_value_eta)
					{
						gl_warning("The PV is over rated for its parent inverter.");
						//Defined above
					}
				}
				else
				{
					if (Max_P > temp_phase_count_val * temp_double_div_value_eta)
					{
						gl_warning("The PV is over rated for its parent inverter.");
						//Defined above
					}
				}
			}
			else
			{
				if (temp_phase_count_val == 4)
				{
					if (Max_P > temp_double_div_value_eff)
					{
						gl_warning("The PV is over rated for its parent inverter.");
						//Defined above
					}
				}
				else
				{
					if (Max_P > temp_phase_count_val * temp_double_div_value_eff)
					{
						gl_warning("The PV is over rated for its parent inverter.");
						//Defined above
					}
				}
			}
			//gl_verbose("Max_P is : %f", Max_P);
		}
		else if (gl_object_isa(parent, "inverter_dyn", "generators")) // SOLAR has a PARENT and PARENT is an inverter_dyn object
		{
			//Map the inverter voltage
			inverter_voltage_property = new gld_property(parent, "V_In");

			//Check it
			if (!inverter_voltage_property->is_valid() || !inverter_voltage_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the inverter property pvc_pmax, which indicates the maximum power avaialbe from the PV now
			const char *cur_prop_name = "pvc_Pmax";
			inverter_pvc_Pmax_property = new gld_property(parent, cur_prop_name);

			//Check it
			if (!inverter_pvc_Pmax_property->is_valid() || !inverter_pvc_Pmax_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map a default power interface field '%s'", obj->id, (obj->name ? obj->name : "Unnamed"), cur_prop_name);
				/*  TROUBLESHOOT
			While attempting to map to one of the default power interface variables, an error occurred.  Please try again.
			If the error persists, please submit a bug report and your model file via the issue tracking system.
			*/
			}

			//Map the inverter current
			inverter_current_property = new gld_property(parent, "I_In");

			//Check it
			if (!inverter_current_property->is_valid() || !inverter_current_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the inverter power value
			inverter_power_property = new gld_property(parent, "P_In");

			//Check it
			if (!inverter_power_property->is_valid() || !inverter_power_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the inverter rated power value
			inverter_rated_power_va_property = new gld_property(parent, "rated_power");

			//Check it
			if (!inverter_rated_power_va_property->is_valid() || !inverter_rated_power_va_property->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter rated power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the inverter dc voltage
			inverter_rated_dc_voltage = new gld_property(parent, "rated_DC_Voltage");

			//Check it
			if (!inverter_rated_dc_voltage->is_valid() || !inverter_rated_dc_voltage->is_double())
			{
				GL_THROW("solar:%d - %s - Unable to map inverter rated DC voltage interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Other variables mapped to normal inverter above -- if these end up being needed for inverter_dyn implementations, map them here
			//See if we're the PV-DC model - if so, register us with the inverter
			if (solar_power_model != PV_CURVE)
			{
				gl_warning("solar:%d - %s - inverter_dyn object only supports PV_CURVE model - forcing that", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				At this time, the inverter_dyn and solar interactions only support the PV_CURVE model.  This may change in the future.
				*/

				//Force it
				solar_power_model = PV_CURVE;
			}

			//"Register" us with the inverter

			//Map the function
			temp_fxn = (FUNCTIONADDR)(gl_get_function(parent, "register_gen_DC_object"));

			//See if it was located
			if (temp_fxn == nullptr)
			{
				GL_THROW("solar:%d - %s - failed to map additional current injection mapping for inverter_dyn:%d - %s", obj->id, (obj->name ? obj->name : "unnamed"), parent->id, (parent->name ? parent->name : "unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the DC interfacing function for the solar object, an error was encountered.
				Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
				*/
			}

			//Call the mapping function
			fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *))(*temp_fxn))(obj->parent, obj);

			//Make sure it worked
			if (fxn_return_status != SUCCESS)
			{
				GL_THROW("solar:%d - %s - failed to map additional current injection mapping for inverter_dyn:%d - %s", obj->id, (obj->name ? obj->name : "unnamed"), parent->id, (parent->name ? parent->name : "unnamed"));
				//Defined above
			}
		}	 //End inverter_dyn
		else //It's not an inverter - fail it.
		{
			GL_THROW("Solar panel can only have an inverter as its parent.");
			/* TROUBLESHOOT
			The solar panel can only have an INVERTER as parent, and no other object. Or it can be all by itself, without a parent.
			*/
		}
	}
	else //No parent
	{	 // default values of voltage
		gl_warning("solar panel:%d has no parent defined. Using static voltages.", obj->id);

		//Map the values to ourself

		//Map the inverter voltage
		inverter_voltage_property = new gld_property(obj, "default_voltage_variable");

		//Check it
		if (!inverter_voltage_property->is_valid() || !inverter_voltage_property->is_double())
		{
			GL_THROW("solar:%d - %s - Unable to map a default power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map to one of the default power interface variables, an error occurred.  Please try again.
			If the error persists, please submit a bug report and your model file via the issue tracking system.
			*/
		}

		//Map the inverter current
		inverter_current_property = new gld_property(obj, "default_current_variable");

		//Check it
		if (!inverter_current_property->is_valid() || !inverter_current_property->is_double())
		{
			GL_THROW("solar:%d - %s - Unable to map a default power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		//Map the inverter power
		inverter_power_property = new gld_property(obj, "default_power_variable");

		//Check it
		if (!inverter_power_property->is_valid() || !inverter_power_property->is_double())
		{
			GL_THROW("solar:%d - %s - Unable to map a default power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		//Set the local voltage value
		default_voltage_array = V_Max / sqrt(3.0);
	}

	climate_result = init_climate();

	//Check factors
	if ((soiling_factor < 0) || (soiling_factor > 1.0))
	{
		soiling_factor = 0.95;
		gl_warning("Invalid soiling factor specified, defaulting to 95%%");
		/*  TROUBLESHOOT
		A soiling factor less than zero or greater than 1.0 was specified.  This is not within the valid
		range, so a default of 0.95 was selected.
		*/
	}

	if ((derating_factor < 0) || (derating_factor > 1.0))
	{
		derating_factor = 0.95;
		gl_warning("Invalid derating factor specified, defaulting to 95%%");
		/*  TROUBLESHOOT
		A derating factor less than zero or greater than 1.0 was specified.  This is not within the valid
		range, so a default of 0.95 was selected.
		*/
	}

	//See if the global flag is set - if so, add the object flag
	if (all_generator_delta)
	{
		obj->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true; //Set the flag and off we go
	}

	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (!enable_subsecond_models)
		{
			gl_warning("solar:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The solar object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			gen_object_count++; //Increment the counter
			first_sync_delta_enabled = true;
		}

		//Make sure our parent is an inverter and deltamode enabled (otherwise this is dumb)
		if (gl_object_isa(parent, "inverter", "generators") || gl_object_isa(parent, "inverter_dyn", "generators"))
		{
			//Make sure our parent has the flag set
			if ((parent->flags & OF_DELTAMODE) != OF_DELTAMODE)
			{
				GL_THROW("solar:%d %s is attached to an inverter, but that inverter is not set up for deltamode", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				The solar object is not parented to a deltamode-enabled inverter.  There is really no reason to have the solar object deltamode-enabled,
				so this should be fixed.
				*/
			}
			//Default else, all is well
		} //Not a proper parent
		else
		{
			GL_THROW("solar:%d %s is not parented to an inverter -- deltamode operations do not support this", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The solar object is not parented to an inverter.  Deltamode only supports it being parented to a deltamode-enabled inverter.
			*/
		}
	}	 //End deltamode inclusive
	else //This particular model isn't enabled
	{
		if (enable_subsecond_models)
		{
			gl_warning("solar:%d %s - Deltamode is enabled for the module, but not this solar array!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The solar array is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this array may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
	}

	//Initialize PV DC model, if desired
	if (solar_power_model == PV_CURVE)
	{
		init_pub_vars_pvcurve_mode();
	}

	return climate_result; /* return 1 on success, 0 on failure */
}

TIMESTAMP solar::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	I_Out = 0.0;

	TIMESTAMP t2 = TS_NEVER;
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP solar::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	int64 ret_value;
	OBJECT *obj = OBJECTHDR(this);
	double insolwmsq, corrwindspeed, Tback, Ftempcorr;
	gld_wlock *test_rlock = nullptr;

	if (first_sync_delta_enabled) //Deltamode first pass
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models) //We want deltamode - see if it's populated yet
		{
			if ((gen_object_current == -1) || (delta_objects == nullptr))
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (gen_object_current >= gen_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the generators module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the generator
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Add us into the list
			delta_objects[gen_object_current] = obj;

			//Map up the function for interupdate
			delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "interupdate_gen_object"));

			//Make sure it worked
			if (delta_functions[gen_object_current] == nullptr)
			{
				GL_THROW("Failure to map deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Map up the function for postupdate
			post_delta_functions[gen_object_current] = nullptr; //No post-update function for us

			//Map up the function for preupdate
			delta_preupdate_functions[gen_object_current] = nullptr;

			//Update pointer
			gen_object_current++;

			//Flag us as complete
			first_sync_delta_enabled = false;
		}	 //End deltamode specials - first pass
		else //Somehow, we got here and deltamode isn't properly enabled...odd, just deflag us
		{
			first_sync_delta_enabled = false;
		}
	} //End first delta timestep
	//default else - either not deltamode, or not the first timestep

	//Check the shading factor
	if ((shading_factor < 0) || (shading_factor > 1))
	{
		GL_THROW("Shading factor outside [0 1] limits in %s", obj->name);
		/*  TROUBLESHOOT
		The shading factor for the solar device is set outside the limited range
		of 0 to 1.  Please set it back within this range and try again.
		*/
	}

	if (solar_model_tilt != PLAYERVAL)
	{
		//Update windspeed - since it's being read and has a variable
		if (weather != nullptr)
		{
			wind_speed = pWindSpeed->get_double();
			Tamb = pTout->get_double();
		}
		//Default else -- just keep the original "static" values

		//Check our mode
		switch (orientation_type)
		{
		case DEFAULT:
		{
			if (weather == nullptr)
			{
				//If no weather, just assume this is the Rated_Insolation (defaults to 1000 W/m^2) by the shading factor.
				Insolation = Rated_Insolation * shading_factor;
			}
			else
			{
				ret_value = ((int64(*)(OBJECT *, double, double, double, double, double *))(*calc_solar_radiation))(weather, DEG_TO_RAD(tilt_angle), obj->latitude, obj->longitude, shading_factor, &Insolation);
			}
			break;
		}
		case FIXED_AXIS: // NOTE that this means FIXED, stationary. There is no AXIS at all. FIXED_AXIS is known as Single Axis Tracking by some, so the term is misleading.
		{
			//Snag solar insolation - prorate by shading (direct axis) - uses model selected earlier
			ret_value = ((int64(*)(OBJECT *, double, double, double, double, double, double *))(*calc_solar_radiation))(weather, DEG_TO_RAD(tilt_angle), DEG_TO_RAD(orientation_azimuth_corrected), obj->latitude, obj->longitude, shading_factor, &Insolation);
			//ret_value = ((int64 (*)(OBJECT *, double, double, double, double *))(*calc_solar_radiation))(weather,tilt_angle,orientation_azimuth_corrected,shading_factor,&Insolation);

			//Make sure it worked
			if (ret_value == 0)
			{
				GL_THROW("Calculation of solar radiation failed in %s", obj->name);
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
	if (Insolation < 0.0)
	{
		Insolation = 0.0;
	}

	if (solar_power_model == BASEEFFICIENT)
	{
		Tambient = FAHR_TO_CELS(Tamb);					   //Read Tamb into the variable - convert to degC for consistency (with below - where the algorithm is more complex and I'm too lazy to convert it to degF in MANY places)
		Tmodule = Tamb + (NOCT - 68) / 74.32 * Insolation; //74.32 sf = 800 W/m2; 68 deg F = 20deg C
		P_Out = Max_P * Insolation / Rated_Insolation *
				(1 + (Pmax_temp_coeff) * (Tmodule - 77)) * derating_factor * soiling_factor; //derating due to manufacturing tolerance, derating sue to soiling both dimensionless
	}
	else if (solar_power_model == FLATPLATE) //Flat plate efficiency
	{
		//Approach taken from NREL SAM documentation for flat plate efficiency model - unclear on its origins
		//Cycle temperature differences through if using flat efficiency model
		if (prevTime != t0)
		{
			prevTemp = currTemp; //Current becomes previous

			currTemp = FAHR_TO_CELS(Tamb); //Convert current temperature back to metric

			prevTime = t0; //Record current timestep
		}

		//Get the "ambient" temperature of the array - by SAM algorithm, taken as
		//halfway between last intervals (linear interpolation)
		Tambient = prevTemp + (currTemp - prevTemp) / 2.0;

		//Impose numerical error by converting things back into metric
		//First put insolation back into W/m^2 - factor in soiling at this point
		insolwmsq = WPFT2_TO_WPM2(Insolation) * soiling_factor;

		//Convert wind speed from mph to m/s
		corrwindspeed = wind_speed * 0.44704;

		//Calculate the "back" temperature of the array
		Tback = (insolwmsq * exp(module_acoeff + module_bcoeff * corrwindspeed) + Tambient);

		//Now compute the cell temperature, based on the back temperature
		Tcell = Tback + insolwmsq / 1000.0 * module_dTcoeff;

		//TCell is assumed to be Tmodule from old calculations (used in voltage below) - convert back to Fahrenheit
		Tmodule = CELS_TO_FAHR(Tcell);

		//Calculate temperature correction value
		Ftempcorr = 1.0 + module_Tcoeff * (Tcell - 25.0) / 100.0; //@Frank, here the 25 and 100 may be the rated values. Not sure if they need to be replaced by those published variables.

		//Place into the DC value - factor in area, derating, and all of the related items
		//P_Out = Insolation*soiling_factor*derating_factor*area*efficiency*Ftempcorr;
		P_Out = Insolation * soiling_factor * derating_factor * (Max_P / Rated_Insolation) * Ftempcorr;
	}
	else if (solar_power_model == PV_CURVE)
	{
		//[Deltamode]: Solar is slaved in this mode - inverter calls the update, so all "property updates" will be in the function below

		//[For steady-state]: Solar proactively tells its parent inverter the max P in the PV_CURVE mode
		update_cur_t_and_S();
		double pvc_Pmax;
		if (pvc_Pmax_calc_simp_mode)
		{
			pvc_Pmax = pvc_U_m_V * pvc_I_m_A;
		}
		else
		{
			pvc_Pmax = get_p_max(pvc_U_m_V);
		}

		inverter_pvc_Pmax_property->setp<double>(pvc_Pmax, *test_rlock);

		if (pvc_Pmax > Max_P)
		{
			gl_warning("solar:%d %s - may be overloaded.",obj->id,(obj->name ? obj->name : "unnamed"));
		}

		return TS_NEVER;
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

	Voc = Voc_Max * (1 + (Voc_temp_coeff) * (Tmodule - 77));
	V_Out = V_Max * (Voc / Voc_Max);
	I_Out = (P_Out / V_Out);

	//Export the values
	inverter_voltage_property->setp<double>(V_Out, *test_rlock);
	inverter_current_property->setp<double>(I_Out, *test_rlock);

	return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP solar::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//Deltamode interupdate function -- basically call sync
//Module-level call
SIMULATIONMODE solar::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltatimedbl, currentDBLtime;
	TIMESTAMP time_passin_value, ret_value;

	//Get timestep value
	deltat = (double)dt / (double)DT_SECOND;

	if (iteration_count_val == 0) //Only update timestamp tracker on first iteration
	{
		//Get decimal timestamp value
		deltatimedbl = (double)delta_time / (double)DT_SECOND;

		//Update tracking variable
		currentDBLtime = (double)gl_globalclock + deltatimedbl;

		//Cast it back
		time_passin_value = (TIMESTAMP)currentDBLtime;

		//Just call sync with a nonsense secondary variable -- doesn't hurt anything in this case
		//Don't care about the return value either
		ret_value = sync(time_passin_value, TS_NEVER);
	}
	//PV_CURVE Model is called by the inverter, so it doesn't need to do anything in here - it's all in the sub-function

	//Solar object never drives anything, so it's always ready to leave
	return SM_EVENT;
}

//DC update function
STATUS solar::solar_dc_update(OBJECT *calling_obj, bool init_mode)
{
	gld_wlock *test_rlock = nullptr;
	STATUS temp_status = SUCCESS;
	double inv_I, inv_P;

	//General updates - theoretically happen for all runs
	//Tamb and Insolation were theoretically updated in the sync call (either as exec or in interupdate)
	update_cur_t_and_S();

	if (init_mode) //Initialization - first runs
	{
		//Pull P_In from the inverter - for now, this is singular (may need to be adjusted when multiple objects exist)
		inv_P = inverter_power_property->get_double();

		//Built on the assumption that the inverter will have a P_In set, and we'l compute the voltage from this
		double cur_p_max = get_p_max(pvc_U_m_V);
		if (cur_p_max < inv_P) //Data sanity check
		{
			GL_THROW("The inverter is requiring %f [W],"
					 " which is larger than the maximum power avialble (%f [W]) of the connected PV(s).",
					 inv_P, cur_p_max);
		}
		V_Out = get_u_from_p(pvc_U_m_V, eps_nr_ite, inv_P);

		//Push the voltage back out to the inverter - this may need different logic when there are multiple objects
		inverter_voltage_property->setp<double>(V_Out, *test_rlock);

		//Initialize our tracking variables - since we're likely to go to normal code after this
		last_DC_current = 0.0;
		last_DC_power = 0.0;
	}
	else //Standard runs
	{
		//Pull the values from the inverter
		V_Out = inverter_voltage_property->get_double();
		inv_I = inverter_current_property->get_double();
		inv_P = inverter_power_property->get_double();

		//Built on assumption that in this portion, inverter is providing just V_In and we need to compute the P and I from that
		P_Out = get_p_from_u(V_Out);
		I_Out = get_i_from_u(V_Out);

		//Already written for assumption that multiple DC objects will be on the inverter
		inv_I += I_Out - last_DC_current;
		inv_P += P_Out - last_DC_power;

		//Push the changes
		inverter_current_property->setp<double>(inv_I, *test_rlock);
		inverter_power_property->setp<double>(inv_P, *test_rlock);

		//Update trackers
		last_DC_current = I_Out;
		last_DC_power = P_Out;
	}

	//Return our status
	return temp_status;
}

/* Utility Funcs */
void solar::init_pub_vars_pvcurve_mode()
{
	OBJECT *obj = OBJECTHDR(this);

	// Init with Reference Temperature & Insolation
	pvc_cur_S_wpm2 = pvc_S_ref_wpm2;
	pvc_cur_t_cels = pvc_t_ref_cels;

	// N-R Solver
	if (max_nr_ite <= 0)
	{
		max_nr_ite = SHRT_MAX;
		gl_warning("solar:%d %s - max_nr_ite was not a valid value, set to %d.",obj->id,(obj->name?obj->name:"Unnamed"),SHRT_MAX);
	}

	if (x0_root_rt <= 0)
	{
		x0_root_rt = 0.15; //Set the initial guess at 15% extra of the absolute value of the extreme point
		gl_warning("solar:%d %s - x0_root_rt was not a valid value, set to 0.15.",obj->id,(obj->name?obj->name:"Unnamed"));
	}

	if (eps_nr_ite <= 0)
	{
		eps_nr_ite = 1e-5;
		gl_warning("solar:%d %s - eps_nr_ite was not a valid value, set to 1e-5.",obj->id,(obj->name?obj->name:"Unnamed"));
	}

	// Solar PV
	if (pvc_t_ref_cels <= 0)
	{
		pvc_t_ref_cels = 25; //Unit: Celsius
		gl_warning("solar:%d %s  -pvc_t_ref_cels was not a valid value, set to 25 degC.",obj->id,(obj->name?obj->name:"Unnamed"));
	}

	if (pvc_S_ref_wpm2 <= 0)
	{
		pvc_S_ref_wpm2 = 1e3; //Unit: w/m^2
		gl_warning("solar:%d %s - pvc_S_ref_wpm2 was not a valid value, set to 1e3 (w/m^2).",obj->id,(obj->name?obj->name:"Unnamed"));
	}

	if (pvc_a1 < 0)
	{
		pvc_a1 = 0;
		gl_warning("solar:%d %s - pvc_a1 was specified as a negative value - set to 0 (1/Celsius).",obj->id,(obj->name?obj->name:"Unnamed"));
	}

	if (pvc_b1 < 0)
	{
		pvc_b1 = 0;
		gl_warning("solar:%d %s - pvc_b1 was specified as a negative value - set to 0 (1/Celsius).",obj->id,(obj->name?obj->name:"Unnamed"));
	}

	// Data Sanity Check
	if (Max_P <= 0)
	{
		if ((pvc_U_m_V > 0) && (pvc_I_m_A > 0))
		{
			Max_P = pvc_U_m_V * pvc_I_m_A;

			if(pvc_I_sc_A <= 0)
			{
				pvc_I_sc_A = pvc_I_m_A * PV_CURVE_PARAM_RATIO;
			}
			if(pvc_U_oc_V <= 0)
			{
				pvc_U_oc_V = pvc_U_m_V * PV_CURVE_PARAM_RATIO;
			}
		}
		else
		{
			double inv_rated_power_va = inverter_rated_power_va_property->get_double();
			Max_P = inv_rated_power_va;

			double inv_rated_dc_volt_v = inverter_rated_dc_voltage->get_double();
			pvc_U_m_V = inv_rated_dc_volt_v;

			pvc_I_m_A = Max_P / pvc_U_m_V;

			pvc_I_sc_A = pvc_I_m_A * PV_CURVE_PARAM_RATIO;
			pvc_U_oc_V = pvc_U_m_V * PV_CURVE_PARAM_RATIO;
		}

		gl_warning("solar:%d %s - Max power not set. Using inverter defaults for pv curve parameters",obj->id,(obj->name?obj->name:"Unnamed"));
		/*  TROUBLESHOOT
		The rated_power as not set for the solar object.  The rated_power for the inverter is taken instead, and all characteristics of the
		PV curve are computed from assumed defaults.  If this is not desired, direclty populate the values.
		*/
	}
	else
	{
		if (pvc_U_m_V <= 0)
		{
			double inv_rated_dc_volt_v = inverter_rated_dc_voltage->get_double();
			pvc_U_m_V = inv_rated_dc_volt_v;
		}
		if (pvc_I_m_A <= 0)
		{
			pvc_I_m_A = Max_P / pvc_U_m_V;
		}
		if (pvc_I_sc_A <= 0)
		{
			pvc_I_sc_A = pvc_I_m_A * PV_CURVE_PARAM_RATIO;
		}
		if (pvc_U_oc_V <= 0)
		{
			pvc_U_oc_V = pvc_U_m_V * PV_CURVE_PARAM_RATIO;
		}

		gl_warning("solar:%d %s - PV curve specification incomplete - defaults calcualted",obj->id,(obj->name?obj->name:"Unnamed"));
		/*  TROUBLESHOOT
		One or more of the four PV curve characteristics were not specified, so they were computed from an assumed default.  If this is
		undesired, explicitly specify the parameters.
		*/
	}

	// Calc C1 & C2 using other PVC params
	pvc_C2 = (pvc_U_m_V / pvc_U_oc_V - 1) / log(1 - pvc_I_m_A / pvc_I_sc_A);
	pvc_C1 = (1 - pvc_I_m_A / pvc_I_sc_A) * exp(-pvc_U_m_V / pvc_C2 / pvc_U_oc_V);
}

void solar::update_cur_t_and_S()
{
	pvc_cur_t_cels = FAHR_TO_CELS(Tamb);
	pvc_cur_S_wpm2 = WPFT2_TO_WPM2(Insolation);
}

/* N-R Solver */
// Funcs added for the N-R solver
double solar::nr_ep_rt(double x)
{
	return hf_dfdU(x, pvc_cur_t_cels, pvc_cur_S_wpm2) / hf_d2fdU2(x, pvc_cur_t_cels);
}

double solar::nr_root_rt(double x, double P)
{
	return hf_f(x, pvc_cur_t_cels, pvc_cur_S_wpm2, P) / hf_dfdU(x, pvc_cur_t_cels, pvc_cur_S_wpm2);
}

double solar::nr_root_search(double x, double doa, double P)
{
	double xn_ep = newton_raphson_nr_ep_rt(x, eps_nr_ite);
	double x0_root = xn_ep + x0_root_rt * fabs(xn_ep);
	double xn_root = newton_raphson_nr_root_rt(x0_root, eps_nr_ite, P);
	return xn_root;
}

double solar::newton_raphson_nr_ep_rt(double x, double doa)
{
	int num_nr_ite = 0;
	double h = nr_ep_rt(x);
	while (fabs(h) >= doa)
	{
		x = x - h; // x(n+1) = x(n) - f{x(n)} / f'{x(n)}
		h = nr_ep_rt(x);
		num_nr_ite++;
		assert(num_nr_ite < max_nr_ite);
	}

	//cout << "The number of iterations is: " << num_nr_ite << "\n";
	return x;
}

double solar::newton_raphson_nr_root_rt(double x, double doa, double P)
{
	int num_nr_ite = 0;
	double h = nr_root_rt(x, P);
	while (fabs(h) >= doa)
	{
		x = x - h; // x(n+1) = x(n) - f{x(n)} / f'{x(n)}
		h = nr_root_rt(x, P);
		num_nr_ite++;
		assert(num_nr_ite < max_nr_ite);
	}

	//cout << "The number of iterations is: " << num_nr_ite << "\n";
	return x;
}

double solar::get_i_from_u(double u)
{
	double i = hf_I(u, pvc_cur_t_cels, pvc_cur_S_wpm2);
	return i;
}

double solar::get_p_from_u(double u)
{
	double p = u * hf_I(u, pvc_cur_t_cels, pvc_cur_S_wpm2);
	return p;
}

double solar::get_u_of_p_max(double x0)
{
	double temp_xn = newton_raphson_nr_ep_rt(x0, eps_nr_ite);
	return temp_xn;
}

double solar::get_p_max(double x0)
{
	double temp_xn = get_u_of_p_max(x0);
	return get_p_from_u(temp_xn);
}

double solar::get_u_from_p(double x, double doa, double P)
{
	return nr_root_search(x, doa, P);
}

/* Solar PV Panel Part*/
// Funcs added for the solar pv model
double solar::hf_dU(double t)
{
	return -pvc_b1 * pvc_U_oc_V * (t - pvc_t_ref_cels);
}

double solar::hf_dI(double t, double S)
{
	return pvc_I_sc_A * (pvc_a1 * S / pvc_S_ref_wpm2 * (t - pvc_t_ref_cels) + S / pvc_S_ref_wpm2 - 1);
}

double solar::hf_I(double U, double t, double S)
{
	return pvc_I_sc_A * (1 - pvc_C1 * (exp((U - hf_dU(t)) / pvc_C2 / pvc_U_oc_V) - 1)) + hf_dI(t, S);
}

double solar::hf_P(double U, double t, double S)
{
	return U * hf_I(U, t, S);
}

double solar::hf_f(double U, double t, double S, double P)
{
	return hf_P(U, t, S) - P;
}

double solar::hf_dIdU(double U, double t)
{
	return pvc_I_sc_A * (-pvc_C1) / pvc_C2 / pvc_U_oc_V * (exp((U - hf_dU(t)) / pvc_C2 / pvc_U_oc_V) - 0);
}

double solar::hf_d2IdU2(double U, double t)
{
	return pvc_I_sc_A * (-pvc_C1) / pvc_C2 / pvc_U_oc_V / pvc_C2 / pvc_U_oc_V * exp((U - hf_dU(t)) / pvc_C2 / pvc_U_oc_V);
}

double solar::hf_dfdU(double U, double t, double S)
{
	return hf_I(U, t, S) + U * hf_dIdU(U, t);
}

double solar::hf_d2fdU2(double U, double t)
{
	return hf_dIdU(U, t) + hf_dIdU(U, t) + U * hf_d2IdU2(U, t);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_solar(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(solar::oclass);
		if (*obj != nullptr)
		{
			solar *my = OBJECTDATA(*obj, solar);
			gl_set_parent(*obj, parent);
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
		if (obj != nullptr)
			return OBJECTDATA(obj, solar)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(solar);
}

EXPORT TIMESTAMP sync_solar(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	solar *my = OBJECTDATA(obj, solar);
	try
	{
		switch (pass)
		{
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock, t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock, t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass == clockpass)
			obj->clock = t1;
	}
	SYNC_CATCHALL(solar);
	return t2;
}

//DELTAMODE Linkage
EXPORT SIMULATIONMODE interupdate_solar(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	solar *my = OBJECTDATA(obj, solar);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_solar(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

//DC Object calls from inverter linkage
EXPORT STATUS dc_object_update_solar(OBJECT *us_obj, OBJECT *calling_obj, bool init_mode)
{
	solar *me_solar = OBJECTDATA(us_obj, solar);
	STATUS temp_status;

	//Call our update function
	temp_status = me_solar->solar_dc_update(calling_obj, init_mode);

	//Return this value
	return temp_status;
}
