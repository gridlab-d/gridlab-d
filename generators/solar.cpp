/** $Id: solar.cpp,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file solar.cpp
	@defgroup solar Diesel gensets
	@ingroup generators

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
			GL_THROW("unable to register object class implemented by %s", __FILE__); 

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
				PT_KEYWORD, "SINGLE_CRYSTAL_SILICON", SINGLE_CRYSTAL_SILICON, 
				PT_KEYWORD, "MULTI_CRYSTAL_SILICON", MULTI_CRYSTAL_SILICON,
				PT_KEYWORD, "AMORPHOUS_SILICON", AMORPHOUS_SILICON,
				PT_KEYWORD, "THIN_FILM_GA_AS", THIN_FILM_GA_AS,
				PT_KEYWORD, "CONCENTRATOR", CONCENTRATOR,

			PT_enumeration,"power_type",PADDR(power_type_v),
				PT_KEYWORD,"AC",AC,
				PT_KEYWORD,"DC",DC,


			PT_double, "noct", PADDR(NOCT),
			PT_double, "Tcell", PADDR(Tcell),
			PT_double, "Tambient", PADDR(Tambient),
			PT_double, "Insolation", PADDR(Insolation),
			PT_double, "Rinternal", PADDR(Rinternal),
			PT_double, "Rated_Insolation", PADDR(Rated_Insolation),
			PT_double, "V_Max[V]", PADDR(V_Max), 
			PT_complex, "Voc_Max", PADDR(Voc_Max),
			PT_complex, "Voc", PADDR(Voc),
			PT_double, "efficiency", PADDR(efficiency),
			PT_double, "area", PADDR(area),
			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			//PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
			PT_complex, "V_Out[V]", PADDR(V_Out),
			PT_complex, "I_Out[A]", PADDR(I_Out),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),


			//resistasnces and max P, Q

			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;


		


		memset(this,0,sizeof(solar));
		/* TODO: set the default values of all properties here */
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
			gl_verbose("solar init: solar data is %f", *pSolar);
		}
		else //climate data was found
		{
			gl_verbose("solar init: climate data was found!");
			// force rank of object w.r.t climate
			OBJECT *obj = gl_find_next(climates,NULL);
			if (obj->rank<=hdr->rank)
				gl_set_dependent(obj,hdr);
			pTout = (double*)GETADDR(obj,gl_get_property(obj,"temperature"));
			pRhout = (double*)GETADDR(obj,gl_get_property(obj,"humidity"));
			pSolar = (double*)GETADDR(obj,gl_get_property(obj,"solar_flux"));
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
	/* TODO: set the context-dependent initial value of properties */
	//double ZB, SB, EB;
	//complex tst;

	gl_verbose("solar init: started");

		gen_mode_v = SUPPLY_DRIVEN;
	
		gen_status_v = ONLINE;
		panel_type_v = SINGLE_CRYSTAL_SILICON;

		NOCT = 45;//celsius
		Tcell = 21; //celsius
		Tambient = 25; //celsius
		Insolation = 0;
		Rinternal = 0.05;
		Rated_Insolation = 1200;
		V_Max = complex(40);
		Voc = complex(40);
		Voc_Max = complex(40);
		area = 1000;
		
		efficiency = 0;
		
		gl_verbose("solar init: finished initializing variables");

	struct {
		complex **var;
		char *varname;
	} map[] = {
		// local object name,	meter object name
		{&pCircuit_V,			"V_In"}, // assumes 2 and 3 follow immediately in memory
		{&pLine_I,				"I_In"}, // assumes 2 and 3(N) follow immediately in memory
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	};
	 

	

	static complex default_line_voltage[1], default_line_current[1];
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL && strcmp(parent->oclass->name,"meter")==0)
	{

		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);

	 gl_verbose("solar init: mapped METER objects to internal variables");
	}

	else if (parent!=NULL && strcmp(parent->oclass->name,"inverter")==0){
		gl_verbose("solar init: parent WAS found, is an inverter!");
			// construct circuit variable map to meter
			/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
			
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++){
						*(map[i].var) = get_complex(parent,map[i].varname);
		}
		gl_verbose("solar init: mapped INVERTER objects to internal variables");
		
	}
	else{
		
			// construct circuit variable map to meter
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
		gl_verbose("solar init: mapped meter objects to internal variables");
	

		
		OBJECT *obj = OBJECTHDR(this);
		gl_verbose("solar init: no parent meter defined, parent is not a meter");
		gl_warning("solar:%d %s", obj->id, parent==NULL?"has no parent meter defined":"parent is not a meter");

		// attach meter variables to each circuit in the default_meter
			*(map[0].var) = &default_line_voltage[0];
			*(map[1].var) = &default_line_current[0];

		// provide initial values for voltages
			default_line_voltage[0] = complex(V_Max.Re()/sqrt(3.0),0);
			//default_line123_voltage[1] = complex(V_Max/sqrt(3.0)*cos(2*PI/3),V_Max/sqrt(3.0)*sin(2*PI/3));
			//default_line123_voltage[2] = complex(V_Max/sqrt(3.0)*cos(-2*PI/3),V_Max/sqrt(3.0)*sin(-2*PI/3));

	}

	gl_verbose("solar init: finished connecting with meter");



	if (gen_mode_v==UNKNOWN)
		{
			OBJECT *obj = OBJECTHDR(this);
			throw("Generator control mode is not specified");
		}
	if (gen_status_v==0)
		{
			//OBJECT *obj = OBJECTHDR(this);
			throw("Generator is out of service!");
		}
	
		
	switch(panel_type_v){
		case SINGLE_CRYSTAL_SILICON:
			efficiency = 0.2;
			gl_verbose("got here!");
			break;
		case MULTI_CRYSTAL_SILICON:
			efficiency = 0.15;
			break;
		case AMORPHOUS_SILICON:
			efficiency = 0.07;
			break;
		case THIN_FILM_GA_AS:
			efficiency = 0.3;
			break;
		case CONCENTRATOR:
			efficiency = 0.15;
			break;
		default:
			efficiency = 0.10;
			break;
	}
		//efficiency dictates how much of the rate insolation the panel can capture and
		//turn into electricity
		Max_P = Rated_Insolation * efficiency * area;
		gl_verbose("Max_P is : %f", Max_P);
		

	gl_verbose("solar init: about to init climate");
	solar::init_climate();


	gl_verbose("solar init: about to exit");



	return 1; /* return 1 on success, 0 on failure */
	

}
	void solar::derate_panel(double Tamb, double Insol){
		Tcell = Tamb + ((NOCT - 20)/0.8) * Insol/1000;
		Rinternal=.0001*Tcell*Tcell;
		gl_verbose("solar sync: panel temperature is : %f", Tcell);
		Voc = Voc_Max * (1 - (0.0037 * (Tcell - 25)));

		if(100.00 > Insol){
			VA_Out = 0;
		}else{
			VA_Out = complex(Max_P * (1 - (0.005 * (Tcell - 25))), 0);
		}
		gl_verbose("solar sync: VA_Out real component is: (%f , %f)", VA_Out.Re()), VA_Out.Im();
		VA_Out = complex(VA_Out.Re() * 0.97, VA_Out.Im()* 0.97); // mismatch derating
		VA_Out = complex(VA_Out.Re() * 0.96, VA_Out.Im()* 0.96);

		
	}
	
	
	
	void solar::calculate_IV(double Tamb, double Insol){
		derate_panel(Tamb, Insol);
		V_Out = V_Max * (Voc / Voc_Max);
		I_Out = (VA_Out / V_Out) * (Insol / Rated_Insolation); 
		gl_verbose("solar sync: VA_Out after set real component is: %f, %f", VA_Out.Re(), VA_Out.Im());
		gl_verbose("solar sync: I_Out real component is : (%f , %f)", I_Out.Re(), I_Out.Im());
	}



complex *solar::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}



/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP solar::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	I_Out = 0.0;
	gl_verbose("solar presync: about to exit presync");
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP solar::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	gl_verbose("solar sync: started");
	//gather insolation (tape)
	//gather ambient temperature (tape)

	//V_Out = pCircuit_V[0];	//Syncs the meter parent to the generator.
	
	//gl_verbose("solar sync: set V from meter");
	//need pSolar for this time step
	//FOR NOW

		if(0 == gl_convert("W/sf","W/m^2", (pSolar))){
					gl_error("climate::init unable to gl_convert() 'W/sf' to 'W/m^2'!");
					return 0;
				}

	Insolation = *pSolar;
	//Insolation = 1000.00;
	
	gl_verbose("solar sync: set insolation as: %f ", Insolation);
	Tambient = *pTout;
	gl_verbose("solar sync: set temperature as %f", Tambient);
	calculate_IV(Tambient, Insolation);
	gl_verbose("solar sync: calculated IV");
	pLine_I[0] = I_Out;
	pCircuit_V[0] = V_Out;
	gl_verbose("solar sync: sent I to the meter: (%f , %f)", I_Out.Re(), I_Out.Im());
	gl_verbose("solar sync: sent V to the meter: (%f , %f)", V_Out.Re(), V_Out.Im());
	gl_verbose("solar sync: sent the current to meter");
	//TIMESTAMP t2 = t1 + 10 * 60 * TS_SECOND;
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement bottom-up behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP solar::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
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
	} 
	catch (char *msg) 
	{
		gl_error("create_solar: %s", msg);
	}
	return 0;
}

EXPORT int init_solar(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,solar)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_solar(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
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
	catch (char *msg)
	{
		gl_error("sync_solar(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return t2;
}
