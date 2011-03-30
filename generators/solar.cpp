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
		oclass = gl_register_class(module,"solar",sizeof(solar),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			throw "unable to register class solar";
		else
			oclass->trl = TRL_PROOF;


		if (gl_publish_variable(oclass,
			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",SUPPLY_DRIVEN, //PV must operate in this mode

			PT_enumeration,"generator_status",PADDR(gen_status_v),
				PT_KEYWORD,"OFFLINE",OFFLINE,
				PT_KEYWORD,"ONLINE",ONLINE,	

			PT_enumeration,"panel_type", PADDR(panel_type_v),
				PT_KEYWORD, "SINGLE_CRYSTAL_SILICON", SINGLE_CRYSTAL_SILICON, //Mono-crystalline in production and the most efficient, efficiency 0.15-0.17
				PT_KEYWORD, "MULTI_CRYSTAL_SILICON", MULTI_CRYSTAL_SILICON,
				PT_KEYWORD, "AMORPHOUS_SILICON", AMORPHOUS_SILICON,
				PT_KEYWORD, "THIN_FILM_GA_AS", THIN_FILM_GA_AS,
				PT_KEYWORD, "CONCENTRATOR", CONCENTRATOR,

			PT_enumeration,"power_type",PADDR(power_type_v),
				PT_KEYWORD,"AC",AC,
				PT_KEYWORD,"DC",DC,

			PT_enumeration, "INSTALLATION_TYPE", PADDR(installation_type_v),
			   PT_KEYWORD, "ROOF_MOUNTED", ROOF_MOUNTED,
               PT_KEYWORD, "GROUND_MOUNTED",GROUND_MOUNTED,


			PT_double, "NOCT[degF]", PADDR(NOCT), //Nominal operating cell temperature NOCT in deg C
			//PT_double, "Tcell[degC]", PADDR(Tcell), //Temperature of PV cell or module??
            PT_double, "Tmodule[degF]", PADDR(Tmodule), //Temperature of PV module
			PT_double, "Tambient[degF]", PADDR(Tambient), //Ambient temperature
			PT_double, "wind_speed[mph]", PADDR(wind_speed), //Wind speed
			//PT_double, "Insolation[W/m^2]", PADDR(Insolation),
			PT_double, "Insolation[W/sf]", PADDR(Insolation),
			PT_double, "Rinternal[Ohm]", PADDR(Rinternal),
			//PT_double, "Rated_Insolation[W/m^2]", PADDR(Rated_Insolation),
			PT_double, "Rated_Insolation[W/sf]", PADDR(Rated_Insolation),
			PT_double, "Pmax_temp_coeff", PADDR(Pmax_temp_coeff),  //temp coefficient of rated Power in %/ deg C
            PT_double, "Voc_temp_coeff", PADDR(Voc_temp_coeff),
			//PT_double, "V_Max[V]", PADDR(V_Max), // Vmax of solar module found on specs
			PT_complex, "V_Max[V]", PADDR(V_Max), // Vmax of solar module found on specs
			PT_complex, "Voc_Max[V]", PADDR(Voc_Max),  //Voc max of solar module
			PT_complex, "Voc[V]", PADDR(Voc), 
			PT_double, "efficiency[unit]", PADDR(efficiency),
			PT_double, "area[sf]", PADDR(area),  //solar panel area
			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			//PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
			PT_complex, "P_Out[kW]", PADDR(P_Out),
			PT_complex, "V_Out[V]", PADDR(V_Out),
			PT_complex, "I_Out[A]", PADDR(I_Out),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),

			//resistances and max P, Q

			    PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		defaults = this;
		memset(this,0,sizeof(solar));

		//NOCT = 45; Tcell = 21; Tambient = 25;//degC ; Rated_Insolation = 1000;//rated insolation is 1000 W/m2 , taken from GE solar cell performance sheet
		// 1 sq m  = 10.764 sq ft.
		NOCT = 118.4; //degF
        Tcell = 69.8;  //degF
		Tambient = 77; //degF
		Insolation = 0;
		Rinternal = 0.05;
		Rated_Insolation = 92.902; //W/Sf for 1000 W/m2
	    V_Max = complex (79.34,0);  // max. power voltage (Vmp) from GE solar cell performance charatcetristics
		Voc = complex (91.22,0);  //taken from GEPVp-200-M-Module performance characteristics, converted to degF
		Voc_Max = complex(91.22,0); //taken from GEPVp-200-M-Module performance characteristics,  converted to degF
		area = 323; //sq f , 30m2
		//Pmax_temp_coeff = -0.5 ;// taken from performance curve of GE solar modules 
        
		efficiency = 0;
	}
}

/* Object creation is called once for each object that is created by the core */
int solar::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}




/** Checks for climate object and maps the climate variables to the house object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 59 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
**/
int solar::init_climate()
{
	OBJECT *hdr = OBJECTHDR(this);

	// link to climate data
	static FINDLIST *climates = NULL;
	int not_found = 0;
	if (climates==NULL && not_found==0) 
	{
		climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		if (climates==NULL)
		{
			not_found = 1;
			gl_warning("solarpanel: no climate data found, using static data");

			//default to mock data
		static double tout=59.0, rhout=0.75, solar=1000;
		pTout = &tout;
		pRhout = &rhout;
		pSolar = &solar;
		}
		else if (climates->hit_count>1)
	{
			gl_warning("solarpanel: %d climates found, using first one defined", climates->hit_count);
		}
	}
	if (climates!=NULL)
	{
		if (climates->hit_count==0)
		{
			//default to mock data
		static double tout=59.0, rhout=0.75, solar=1000;
		pTout = &tout;
		pRhout = &rhout;
		pSolar = &solar;
			//gl_verbose("solar init: solar data is %f", *pSolar);
	}
		else //climate data was found
		{
			//gl_verbose("solar init: climate data was found!");
			// force rank of object w.r.t climate
			OBJECT *obj = gl_find_next(climates,NULL);
		if (obj->rank<=hdr->rank)
			gl_set_dependent(obj,hdr);
		   pTout = (double*)GETADDR(obj,gl_get_property(obj,"temperature"));
	       pRhout = (double*)GETADDR(obj,gl_get_property(obj,"humidity"));
		   pSolar = (double*)GETADDR(obj,gl_get_property(obj,"solar_flux"));
			//pSolar = (double*)GETADDR(obj,gl_get_property(obj,"solar_global"));
			//pSolar = (double*)malloc(50 * sizeof(double));

			//the climate object's solar data doesn't seem to work... so.... use a mock version?
			//int i = 0;
			//while (i < 50){
			//	if(i < 1){
			//		pSolar[i] = 1000.00;
			//	}else if(i < 2){
			//		pSolar[i] = 500.00;
			//	}else if (i < 3){
			//		pSolar[i] = 800.00;
			//	}else if (i < 4){
			//		pSolar[i] = 1000.00;
			//	}else if (i < 5 ){
			//		pSolar[i] = 800.00;
			//	}else if (i < 10){
			//		pSolar[i] = 500.00;
			//	}else{  // i < 50
			//		pSolar[i] = 0.00;
			//	}

			/*	gl_verbose("solar init: loop # is %i", i);
				gl_verbose("solar init: climate data is %f", pSolar[i]);
			i++;
			}*/


			//THIS WILL ONLY CONVERT ONE VALUE AT A TIME, NEED TO REVERT
			/*if(0 == gl_convert("W/sf","W/m^2", (pSolar))){
					gl_error("climate::init unable to gl_convert() 'W/sf' to 'W/m^2'!");
					return 0;
				}*/

			/*int i = 0;
			while (i < 100){
				gl_verbose("solar init: loop # is %i", i);
				gl_verbose("solar init: climate data is %f", pSolar[i]);
			i++;
			}*/


		}
	}
	return 1;
}

/* Object initialization is called once after all object have been created */
int solar::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	if (gen_mode_v == UNKNOWN)
	{
		gl_warning("Generator control mode is not specified! Using default: SUPPLY_DRIVEN");
		gen_mode_v = SUPPLY_DRIVEN;
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

	struct {
		complex **var;
		char *varname;
	} 
	
	map[] = {
		// map the V & I from the inverter
		{&pCircuit_V,			"V_In"}, // assumes 2 and 3 follow immediately in memory
		{&pLine_I,				"I_In"}, // assumes 2 and 3(N) follow immediately in memory
	};

	static complex default_line_voltage[1], default_line_current[1];
	int i;

	// find parent inverter, if not defined, use a default voltage
	if (parent != NULL && strcmp(parent->oclass->name,"inverter") == 0)
	{
		// construct circuit variable map to meter
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			*(map[i].var) = get_complex(parent,map[i].varname);
		}

		//NR_mode = get_bool(parent,"NR_mode");
	}
	else if	(parent != NULL && strcmp(parent->oclass->name,"inverter") != 0)
	{
		throw("Solar panel must have an inverter as it's parent");
	}
	else
	{	// default values of voltage
		// construct circuit variable map to meter
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
		gl_warning("solar panel:%d has no parent defined. Using static voltages.", obj->id);

		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line_voltage[0];
		*(map[1].var) = &default_line_current[0];

		// provide initial values for voltages
		default_line_voltage[0] = complex(V_Max.Re()/sqrt(3.0),0);
        	//default_line_voltage[0] = V_Max/sqrt(3.0);
     
		//NR_mode = false;
	}
	//1 m/s = 2.24 mph
		
	switch(panel_type_v)
	{
		case SINGLE_CRYSTAL_SILICON:
			efficiency = 0.2;
          //  w1 = 0.942; // w1 is coeff for Tamb in deg C// convert to fahrenheit degF =degC+9/5+32
           Pmax_temp_coeff = -0.00437/33.8 ;  // average values from ref 2 in per degF
		   Voc_temp_coeff  = -0.00393/33.8;
		   break;
		case MULTI_CRYSTAL_SILICON:
			efficiency = 0.15;
			Pmax_temp_coeff = -0.00416/33.8; // average values from ref 2
			Voc_temp_coeff  = -0.0039/33.8;
			break;
		case AMORPHOUS_SILICON:
			efficiency = 0.07;
		   Pmax_temp_coeff = 0.1745/33.8; // average values from ref 2
		   Voc_temp_coeff  = -0.00407/33.8;
		   break;
		case THIN_FILM_GA_AS:
			efficiency = 0.3;
			break;
		case CONCENTRATOR:
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
	Max_P = Rated_Insolation * efficiency * area; // We are calculating the module efficiency which should be less than cell efficiency. What about the sun hours??
	//gl_verbose("Max_P is : %f", Max_P);

	solar::init_climate();

	return 1; /* return 1 on success, 0 on failure */
}

TIMESTAMP solar::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	I_Out = complex(0,0);
	

	TIMESTAMP t2 = TS_NEVER;
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP solar::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	//gl_verbose("solar sync: started");

	//V_Out = pCircuit_V[0];	//Syncs the meter parent to the generator.
	if (1)//*NR_mode == false)
	{
		Insolation = pSolar[0];	// solar insolation read from player file 

		//Conversion of degree  centrigade to degree fahrenheit
		 // degree F = degree C* 1.8/32

		Tambient = *pTout;  //Tamb read from player file
        	
    //Tmodule = w1 *Tamb + w2 * Insolation + w3* wind_speed + constant;
		if (Insolation < 0.0){
			Insolation = 0.0;
		}
			
		Tmodule = Tambient+ (NOCT-68)/74.32 * Insolation;   //74.32 sf = 800 W/m2; 68 deg F = 20deg C
         
	     VA_Out = Max_P * Insolation/Rated_Insolation *(1+(Pmax_temp_coeff)*( Tmodule-77))*0.95*0.95; //derating due to manufacturing tolerance, derating sue to soiling both dimensionless
	
	     Voc = Voc_Max * (1+(Voc_temp_coeff)*(Tmodule-77));
				    
		 V_Out = V_Max * (Voc/Voc_Max);

	     I_Out = (VA_Out/V_Out);

		 *pLine_I =I_Out;
         *pCircuit_V =V_Out;

	}

	TIMESTAMP t2 = TS_NEVER;

	return t2; 
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

bool *solar::get_bool(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
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
