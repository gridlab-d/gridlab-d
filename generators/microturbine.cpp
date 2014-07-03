/** $Id: microturbine.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file microturbine.cpp
	@defgroup microturbine
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"
#include "microturbine.h"
#include "timestamp.h"



#define HOUR 3600 * TS_SECOND

CLASS *microturbine::oclass = NULL;
microturbine *microturbine::defaults = NULL;

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
microturbine::microturbine(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"microturbine",sizeof(microturbine),passconfig|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class microturbine";
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

			PT_enumeration,"power_type",PADDR(power_type_v),
				PT_KEYWORD,"AC",(enumeration)AC,
				PT_KEYWORD,"DC",(enumeration)DC,


			PT_double, "Rinternal", PADDR(Rinternal),
			PT_double, "Rload", PADDR(Rload),
			PT_double, "V_Max[V]", PADDR(V_Max), 
			PT_complex, "I_Max[A]", PADDR(I_Max),
			
			PT_double, "frequency[Hz]", PADDR(frequency),
			PT_double, "Max_Frequency[Hz]", PADDR(Max_Frequency),
			PT_double, "Min_Frequency[Hz]", PADDR(Min_Frequency),
			PT_double, "Fuel_Used[kVA]", PADDR(Fuel_Used),
			PT_double, "Heat_Out[kVA]", PADDR(Heat_Out),
			PT_double, "KV", PADDR(KV), //voltage constant
			PT_double, "Power_Angle", PADDR(Power_Angle),


			PT_double, "Max_P[kW]", PADDR(Max_P), //< maximum real power capacity in kW
			PT_double, "Min_P[kW]", PADDR(Min_P), //< minimus real power capacity in kW
	
			//PT_double, "Max_Q[kVAR]", PADDR(Max_Q), //< maximum reactive power capacity in kVar
			//PT_double, "Min_Q[kVAR]", PADDR(Min_Q), //< minimus reactive power capacity in kVar

			PT_complex, "phaseA_V_Out[kV]", PADDR(phaseA_V_Out), //voltage
			PT_complex, "phaseB_V_Out[kV]", PADDR(phaseB_V_Out),
			PT_complex, "phaseC_V_Out[kV]", PADDR(phaseC_V_Out),
	
	//complex V_Out;
	
			PT_complex, "phaseA_I_Out[A]", PADDR(phaseA_I_Out),    // current
			PT_complex, "phaseB_I_Out[A]", PADDR(phaseB_I_Out),
			PT_complex, "phaseC_I_Out[A]", PADDR(phaseC_I_Out),

	//complex I_Out;

			PT_complex, "power_A_Out", PADDR(power_A_Out),     //power
			PT_complex, "power_B_Out", PADDR(power_B_Out),
			PT_complex, "power_C_Out", PADDR(power_C_Out),

			PT_complex, "VA_Out", PADDR(VA_Out),

			PT_double, "pf_Out", PADDR(pf_Out),
			PT_complex, "E_A_Internal", PADDR(E_A_Internal),
			PT_complex, "E_B_Internal", PADDR(E_B_Internal),
			PT_complex, "E_C_Internal", PADDR(E_C_Internal),

			PT_double, "efficiency", PADDR(efficiency),
			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			//PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
		
			//PT_complex, "VA_Out[VA]", PADDR(VA_Out),

			//resistances and max P, Q

			PT_set, "phases", PADDR(phases),

				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;

	


		memset(this,0,sizeof(microturbine));
		/* TODO: set the default values of all properties here */
	}
}


/* Object creation is called once for each object that is created by the core */
int microturbine::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}


double microturbine::determine_power_angle (complex power_out){ //could also pass in pf_Out as a parameter if needed
	Power_Angle = acos((double) (power_out.Re() / power_out.Mag()));
	return Power_Angle;

}

complex microturbine::determine_source_voltage(complex voltage_out, double r_internal, double r_load){
	//placeholder, probably need to replace with iterative function
	complex Vs = complex(((r_internal + r_load)/(r_load)) * voltage_out.Re(), ((r_internal + r_load)/(r_load)) * voltage_out.Im());
	return Vs;

}

double microturbine::determine_frequency(complex power_out){
	//double Kx = 3 * Rinternal / 3.14159;
	//used linear approximation which does better job than textbook equation...
	//assumes power in W
	double f = power_out.Mag() * 1.5 + 55000;\
	f = f/60;
	return f;
}

double microturbine::calculate_loss(double frequency_out){
	//placeholder	
	return 0;
}

double microturbine::determine_heat(complex power_out, double heat_loss){
	//placeholder;
	Heat_Out = power_out.Mag() * heat_loss;
	return Heat_Out;
}






/* Object initialization is called once after all object have been created */
int microturbine::init(OBJECT *parent)
{


	//generator_mode_choice = CONSTANT_PQ;
	gen_status_v = ONLINE;

	Rinternal = 0.05;
	Rload = 1;
	V_Max = complex(10000);
	I_Max = complex(1000);

	frequency = 0;
	Max_Frequency = 2000;
	Min_Frequency = 0;
	Fuel_Used = 0;
	Heat_Out = 0;
	KV = 1; //voltage constant
	Power_Angle = 1;
	
	Max_P = 100;//< maximum real power capacity in kW
    Min_P = 0;//< minimus real power capacity in kW
	
	Max_Q = 100;//< maximum reactive power capacity in kVar
    Min_Q = 0;//< minimus reactive power capacity in kVar
	Rated_kVA = 150; //< nominal capacity in kVA
	//double Rated_kV; //< nominal line-line voltage in kV
	
	efficiency = 0;
	pf_Out = 1;

	gl_verbose("microturbine init: finished initializing variables");

	struct {
		complex **var;
		char *varname;
	} map[] = {
		// local object name,	meter object name
		{&pCircuit_V_A,			"phaseA_V_In"}, 
		{&pCircuit_V_B,			"phaseB_V_In"}, 
		{&pCircuit_V_C,			"phaseC_V_In"}, 
		{&pLine_I_A,			"phaseA_I_In"}, 
		{&pLine_I_B,			"phaseB_I_In"}, 
		{&pLine_I_C,			"phaseC_I_In"}, 
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	};
	 

	

	static complex default_line_voltage[1], default_line_current[1];
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL && strcmp(parent->oclass->name,"meter")==0)
	{
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);

		gl_verbose("microturbine init: mapped METER objects to internal variables");
	}
	else if (parent!=NULL && strcmp(parent->oclass->name,"rectifier")==0){
		gl_verbose("microturbine init: parent WAS found, is an rectifier!");
			// construct circuit variable map to meter
			/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)

		for (i=0; i<sizeof(map)/sizeof(map[0]); i++){
			*(map[i].var) = get_complex(parent,map[i].varname);
		}
		gl_verbose("microturbine init: mapped RECTIFIER objects to internal variables");
	}
	else{
		
			// construct circuit variable map to meter
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
		gl_verbose("microturbine init: mapped meter objects to internal variables");

		OBJECT *obj = OBJECTHDR(this);
		gl_verbose("microturbine init: no parent meter defined, parent is not a meter");
		gl_warning("microturbine:%d %s", obj->id, parent==NULL?"has no parent meter defined":"parent is not a meter");

		// attach meter variables to each circuit in the default_meter
			*(map[0].var) = &default_line_voltage[0];
			*(map[1].var) = &default_line_current[0];

		// provide initial values for voltages
			default_line_voltage[0] = complex(V_Max.Re()/sqrt(3.0),0);
			default_line_current[0] = complex(I_Max.Re());
			//default_line123_voltage[1] = complex(V_Max/sqrt(3.0)*cos(2*PI/3),V_Max/sqrt(3.0)*sin(2*PI/3));
			//default_line123_voltage[2] = complex(V_Max/sqrt(3.0)*cos(-2*PI/3),V_Max/sqrt(3.0)*sin(-2*PI/3));

	}

	gl_verbose("microturbine init: finished connecting with meter");




	/* TODO: set the context-dependent initial value of properties */
	//double ZB, SB, EB;
	//complex tst;
		if (gen_mode_v==UNKNOWN)
	{
		OBJECT *obj = OBJECTHDR(this);
		throw("Generator control mode is not specified");
	}
		if (gen_status_v==0)
	{
		//OBJECT *obj = OBJECTHDR(this);
		throw("Generator is out of service!");
	}else
		{
			return 1;

		}
	return 1; /* return 1 on success, 0 on failure */
}





complex *microturbine::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}




/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP microturbine::presync(TIMESTAMP t0, TIMESTAMP t1)
{

	TIMESTAMP t2 = TS_NEVER;
	Heat_Out = Fuel_Used = frequency = 0.0;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP microturbine::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	//gather V_Out for each phase
	//gather I_Out for each phase
	//gather VA_Out for each phase
	//gather Q_Out
	//gather S_Out
	//gather Pf_Out

	phaseA_V_Out = pCircuit_V_A[0];
	phaseB_V_Out = pCircuit_V_B[0];
	phaseC_V_Out = pCircuit_V_C[0];

	phaseA_I_Out = pLine_I_A[0];
	phaseB_I_Out = pLine_I_B[0];
	phaseC_I_Out = pLine_I_C[0];

	gl_verbose("microturbine sync: phaseA_V_Out from parent is: (%f , %f)", phaseA_V_Out.Re(), phaseA_V_Out.Im());
	gl_verbose("microturbine sync: phaseB_V_Out from parent is: (%f , %f)", phaseB_V_Out.Re(), phaseB_V_Out.Im());
	gl_verbose("microturbine sync: phaseC_V_Out from parent is: (%f , %f)", phaseC_V_Out.Re(), phaseC_V_Out.Im());

	gl_verbose("microturbine sync: phaseA_I_Out from parent is: (%f , %f)", phaseA_I_Out.Re(), phaseA_I_Out.Im());
	gl_verbose("microturbine sync: phaseB_I_Out from parent is: (%f , %f)", phaseB_I_Out.Re(), phaseB_I_Out.Im());
	gl_verbose("microturbine sync: phaseC_I_Out from parent is: (%f , %f)", phaseC_I_Out.Re(), phaseC_I_Out.Im());

	power_A_Out = (~phaseA_I_Out) * phaseA_V_Out;
	power_B_Out = (~phaseB_I_Out) * phaseB_V_Out;
	power_C_Out = (~phaseC_I_Out) * phaseC_V_Out;


	gl_verbose("microturbine sync: power_A_Out from parent is: (%f , %f)", power_A_Out.Re(), power_A_Out.Im());
	gl_verbose("microturbine sync: power_B_Out from parent is: (%f , %f)", power_B_Out.Re(), power_B_Out.Im());
	gl_verbose("microturbine sync: power_C_Out from parent is: (%f , %f)", power_C_Out.Re(), power_C_Out.Im());


	VA_Out = power_A_Out + power_B_Out + power_C_Out;

	E_A_Internal = determine_source_voltage(phaseA_V_Out, Rinternal, Rload);
	E_B_Internal = determine_source_voltage(phaseB_V_Out, Rinternal, Rload);
	E_C_Internal = determine_source_voltage(phaseC_V_Out, Rinternal, Rload);


	
	gl_verbose("microturbine sync: E_A_Internal calc is: (%f , %f)", E_A_Internal.Re(), E_A_Internal.Im());
	gl_verbose("microturbine sync: E_B_Internal calc is: (%f , %f)", E_B_Internal.Re(), E_B_Internal.Im());
	gl_verbose("microturbine sync: E_C_Internal calc is: (%f , %f)", E_C_Internal.Re(), E_C_Internal.Im());


	frequency = determine_frequency(VA_Out);

	gl_verbose("microturbine sync: determined frequency is: %f", frequency);

	if(frequency > Max_Frequency){
		throw ("the frequency asked for from the microturbine is too high!");
	}
	if(frequency < Min_Frequency){
		throw ("the frequency asked for from the microturbine is too low!");
	}


	double loss = calculate_loss(frequency);
	efficiency = 1 - loss;
	Heat_Out = determine_heat(VA_Out, loss);
	Fuel_Used = Heat_Out + VA_Out.Mag();
	
	gl_verbose("microturbine sync: about to exit");

return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP microturbine::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;

	Heat_Out = Fuel_Used = frequency = 0.0;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_microturbine(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(microturbine::oclass);
		if (*obj!=NULL)
		{
			microturbine *my = OBJECTDATA(*obj,microturbine);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	} 
	CREATE_CATCHALL(microturbine);
}

EXPORT int init_microturbine(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,microturbine)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(microturbine);
}

EXPORT TIMESTAMP sync_microturbine(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	microturbine *my = OBJECTDATA(obj,microturbine);
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
	SYNC_CATCHALL(microturbine);
	return t2;
}
