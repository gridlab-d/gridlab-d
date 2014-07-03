/** $Id: power_electronics.cpp,v 1.0 2008/07/15 
	Copyright (C) 2008 Battelle Memorial Institute
	@file power_electronics.cpp
	@defgroup power_electronics
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"
#include "power_electronics.h"

#define DEFAULT 1.0;
#define S_DEFAULT 1.0;
#define G_DEFAULT 1000.0; 

CLASS *power_electronics::oclass = NULL;
power_electronics *power_electronics::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

power_electronics::power_electronics(){}

/* Class registration is only called once to register the class with the core */
power_electronics::power_electronics(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"power_electronics",sizeof(power_electronics),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class power_electronics";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			//should this be "GENERATOR_MODE" OR "gen_mode_v"?
			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN,

			PT_enumeration,"generator_status",PADDR(gen_status_v),
				PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE,
				PT_KEYWORD,"ONLINE",(enumeration)ONLINE,	
			
			PT_enumeration,"converter_type",PADDR(converter_type_v),
				PT_KEYWORD,"VOLTAGE_SOURCED",(enumeration)VOLTAGE_SOURCED,
				PT_KEYWORD,"CURRENT_SOURCED",(enumeration)CURRENT_SOURCED,

			PT_enumeration,"switch_type",PADDR(switch_type_v),
				PT_KEYWORD,"IDEAL_SWITCH",(enumeration)IDEAL_SWITCH,
				PT_KEYWORD,"BJT",(enumeration)BJT,
				PT_KEYWORD,"MOSFET",(enumeration)MOSFET,
				PT_KEYWORD,"SCR",(enumeration)SCR,
				PT_KEYWORD,"JFET",(enumeration)JFET,
				PT_KEYWORD,"IBJT",(enumeration)IBJT,
				PT_KEYWORD,"DARLINGTON",(enumeration)DARLINGTON,

			PT_enumeration,"filter_type",PADDR(filter_type_v),
				PT_KEYWORD,"LOW_PASS",(enumeration)LOW_PASS,
				PT_KEYWORD,"HIGH_PASS",(enumeration)HIGH_PASS,
				PT_KEYWORD,"BAND_STOP",(enumeration)BAND_STOP,
				PT_KEYWORD,"BAND_PASS",(enumeration)BAND_PASS,

			PT_enumeration,"filter_implementation",PADDR(filter_imp_v),
				PT_KEYWORD,"IDEAL_FILTER",(enumeration)IDEAL_FILTER,
				PT_KEYWORD,"CAPACITVE",(enumeration)CAPACITIVE,
				PT_KEYWORD,"INDUCTIVE",(enumeration)INDUCTIVE,
				PT_KEYWORD,"SERIES_RESONANT",(enumeration)SERIES_RESONANT,
				PT_KEYWORD,"PARALLEL_RESONANT",(enumeration)PARALLEL_RESONANT,

			PT_enumeration,"filter_frequency",PADDR(filter_freq_v),
				PT_KEYWORD,"F120HZ",(enumeration)F120HZ,
				PT_KEYWORD,"F180HZ",(enumeration)F180HZ,
				PT_KEYWORD,"F240HZ",(enumeration)F240HZ,

			
			PT_enumeration,"power_type",PADDR(power_type_v),
				PT_KEYWORD,"AC",(enumeration)AC,
				PT_KEYWORD,"DC",(enumeration)DC,

			PT_double, "Rated_kW[kW]", PADDR(Rated_kW), //< nominal power in kW
			PT_double, "Max_P[kW]", PADDR(Max_P),//< maximum real power capacity in kW
			PT_double, "Min_P[kW]", PADDR(Min_P),//< minimum real power capacity in kW
			//PT_double, "Max_Q[kVAr]", PADDR(Max_Q),//< maximum reactive power capacity in kVar
			//PT_double, "Min_Q[kVAr]", PADDR(Min_Q),//< minimum reactive power capacity in kVar
	
			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),
			
/*			
			PT_double, Rinternal;
			PT_double, Rload;
			PT_double, Rtotal;
			PT_double, Rphase;
			PT_double, Rground;
			PT_double, Rground_storage;
			PT_double[3], Rfilter;

			PT_double, Cinternal;
			PT_double, Cground;
			PT_double, Ctotal;
			PT_double[3], Cfilter;

			PT_double, Linternal;
			PT_double, Lground;
			PT_double, Ltotal;
			PT_double[3], Lfilter;

			PT_bool, filter_120HZ;
			PT_bool, filter_180HZ;
			PT_bool, filter_240HZ;
*/
			//PT_double, "pf_in[double]", PADDR(pf_in),
			//PT_double, "pf_out[double]", PADDR(pf_out),
/*
			PT_int, number_of_phases_in;
			PT_int, number_of_phases_out;

			SWITCH_TYPE switch_type_choice;
			SWITCH_IMPLEMENTATION switch_implementation_choice;
			GENERATOR_MODE generator_mode_choice;
			POWER_TYPE power_in;
			POWER_TYPE power_out;
*/
			//PT_double, "efficiency[double]", PADDR(efficiency),


/*
			PT_complex, "voltage_A[V]", PADDR(voltage_A),
			PT_complex, "voltage_B[V]", PADDR(voltage_B),
			PT_complex, "voltage_C[V]", PADDR(voltage_C),
			PT_complex, "current_A[A]", PADDR(current_A),
			PT_complex, "current_B[A]", PADDR(current_B),
			PT_complex, "current_C[A]", PADDR(current_C),
			PT_complex, "EfA[V]", PADDR(EfA),
			PT_complex, "EfB[V]", PADDR(EfB),
			PT_complex, "EfC[V]", PADDR(EfC),
			PT_complex, "power_A[VA]", PADDR(power_A),
			PT_complex, "power_B[VA]", PADDR(power_B),
			PT_complex, "power_C[VA]", PADDR(power_C),
			PT_complex, "power_A_sch[VA]", PADDR(power_A_sch),
			PT_complex, "power_B_sch[VA]", PADDR(power_B_sch),
			PT_complex, "power_C_sch[VA]", PADDR(power_C_sch),
			PT_complex, "EfA_sch[V]", PADDR(EfA_sch),
			PT_complex, "EfB_sch[V]", PADDR(EfB_sch),
			PT_complex, "EfC_sch[V]", PADDR(EfC_sch),
*/			
			
			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(power_electronics));
		/* TODO: set the default values of all properties here */
	}
}



int power_electronics::filter_circuit_impact(power_electronics::FILTER_TYPE filter_type_choice, power_electronics::FILTER_IMPLEMENTATION filter_implementation_choice){
	switch(filter_type_choice){
		case BAND_PASS:
			switch(filter_implementation_choice){
				case IDEAL_FILTER:
					if(filter_120HZ){
					Rsfilter[0] = S_DEFAULT;
					Rgfilter[0] = G_DEFAULT;
					Xsfilter[0] = S_DEFAULT;
					Xgfilter[0] = G_DEFAULT;
					}
					if(filter_180HZ){
					Rsfilter[1] = S_DEFAULT;
					Rgfilter[1] = G_DEFAULT;
					Xsfilter[1] = S_DEFAULT;
					Xgfilter[1] = G_DEFAULT;
					}
					if(filter_240HZ){
					Rsfilter[2] = S_DEFAULT;
					Rgfilter[2] = G_DEFAULT;
					Xsfilter[2] = S_DEFAULT;
					Xgfilter[2] = G_DEFAULT;
					}
					break;
				case CAPACITIVE:
					if(filter_120HZ){
					Rsfilter[0] = S_DEFAULT;
					Rgfilter[0] = G_DEFAULT;
					Xsfilter[0] = S_DEFAULT;
					Xgfilter[0] = G_DEFAULT;
					}
					if(filter_180HZ){
					Rsfilter[1] = S_DEFAULT;
					Rgfilter[1] = G_DEFAULT;
					Xsfilter[1] = S_DEFAULT;
					Xgfilter[1] = G_DEFAULT;
					}
					if(filter_240HZ){
					Rsfilter[2] = S_DEFAULT;
					Rgfilter[2] = G_DEFAULT;
					Xsfilter[2] = S_DEFAULT;
					Xgfilter[2] = G_DEFAULT;
					}
					break;
				case INDUCTIVE:
					if(filter_120HZ){
					Rsfilter[0] = S_DEFAULT;
					Rgfilter[0] = G_DEFAULT;
					Xsfilter[0] = S_DEFAULT;
					Xgfilter[0] = G_DEFAULT;
					}
					if(filter_180HZ){
					Rsfilter[1] = S_DEFAULT;
					Rgfilter[1] = G_DEFAULT;
					Xsfilter[1] = S_DEFAULT;
					Xgfilter[1] = G_DEFAULT;
					}
					if(filter_240HZ){
					Rsfilter[2] = S_DEFAULT;
					Rgfilter[2] = G_DEFAULT;
					Xsfilter[2] = S_DEFAULT;
					Xgfilter[2] = G_DEFAULT;
					}
					break;
				case SERIES_RESONANT:
					if(filter_120HZ){
					Rsfilter[0] = S_DEFAULT;
					Rgfilter[0] = G_DEFAULT;
					Xsfilter[0] = S_DEFAULT;
					Xgfilter[0] = G_DEFAULT;
					}
					if(filter_180HZ){
					Rsfilter[1] = S_DEFAULT;
					Rgfilter[1] = G_DEFAULT;
					Xsfilter[1] = S_DEFAULT;
					Xgfilter[1] = G_DEFAULT;
					}
					if(filter_240HZ){
					Rsfilter[2] = S_DEFAULT;
					Rgfilter[2] = G_DEFAULT;
					Xsfilter[2] = S_DEFAULT;
					Xgfilter[2] = G_DEFAULT;
					}
					break;
				case PARALLEL_RESONANT:
					if(filter_120HZ){
					Rsfilter[0] = S_DEFAULT;
					Rgfilter[0] = G_DEFAULT;
					Xsfilter[0] = S_DEFAULT;
					Xgfilter[0] = G_DEFAULT;
					}
					if(filter_180HZ){
					Rsfilter[1] = S_DEFAULT;
					Rgfilter[1] = G_DEFAULT;
					Xsfilter[1] = S_DEFAULT;
					Xgfilter[1] = G_DEFAULT;
					}
					if(filter_240HZ){
					Rsfilter[2] = S_DEFAULT;
					Rgfilter[2] = G_DEFAULT;
					Xsfilter[2] = S_DEFAULT;
					Xgfilter[2] = G_DEFAULT;
					}
					break;
				default:
					break;
			}
		break;

		default:
		break;
			
	}
	Rsfilter_total = Rsfilter[0] + Rsfilter[1] + Rsfilter[2];
	Rgfilter_total = Rgfilter[0] + Rgfilter[1] + Rgfilter[2];
	Xsfilter_total = Xsfilter[0] + Xsfilter[1] + Xsfilter[2];
	Xgfilter_total = Xgfilter[0] + Xgfilter[1] + Xgfilter[2];
	return 1;
}

//given output voltage, solves source voltage

complex power_electronics::filter_voltage_impact_source(complex desired_I_out, complex desired_V_out){
	if(desired_V_out == 0){
		return complex(0,0);
	}else{
	complex Is = filter_current_impact_source(desired_I_out, desired_V_out);
	complex Vs = desired_V_out + (Is * complex(Rsfilter_total,Xsfilter_total));
	return Vs;
	}
}

//given output current, solves source current

complex power_electronics::filter_current_impact_source(complex desired_I_out, complex desired_V_out){
	if(desired_I_out == 0){
		return complex(0,0);
	}else{
	complex Ig = desired_V_out / complex(Rgfilter_total,Xgfilter_total);
	complex Is = desired_I_out + Ig;
	return Is;
	}
}


//given the input source and voltage, compute output voltage
complex power_electronics::filter_voltage_impact_out(complex source_I_in, complex source_V_in){
	if(source_V_in == 0){
		return complex(0,0);
	}else{
	complex Vo = source_V_in - (source_I_in * complex(Rsfilter_total,Xsfilter_total));
	return Vo;
	}
}

//given the input source and voltage, compute output current
complex power_electronics::filter_current_impact_out(complex source_I_in, complex source_V_in){
	if(source_I_in == 0){
		return complex(0,0);
	}else{
	complex Vo = filter_voltage_impact_out(source_I_in, source_V_in);
	complex Ig = Vo / complex(Rgfilter_total,Xgfilter_total);
	complex Io = source_I_in - Ig;
	return Io;
	}
}



double power_electronics::internal_switch_resistance(power_electronics::SWITCH_TYPE switch_type_choice){
	switch(switch_type_choice){
		case IDEAL_SWITCH:
			Rinternal += 0;
			break;
		case BJT:
			Rinternal += DEFAULT;
			break;
		case MOSFET:
			Rinternal += DEFAULT;
			break;
		case SCR:
			Rinternal += DEFAULT;
			break;	
		case JFET:
			Rinternal += DEFAULT;
			break;	
		case IBJT:
			Rinternal += DEFAULT;
			break;
		case DARLINGTON:
			Rinternal += DEFAULT;
			break;
		default:
			break;
	}
	return 1;
}

double power_electronics::calculate_loss(double Rtotal, double LTotal, double Ctotal, POWER_TYPE power_in, POWER_TYPE power_out){
	//placeholder
	return 0;
}

double power_electronics::calculate_frequency_loss(double frequency, double Rtotal, double Ltotal, double Ctotal){
	//placeholder
	return 0;
}




/* Object creation is called once for each object that is created by the core */
int power_electronics::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int power_electronics::init(OBJECT *parent)
{
	
	/* TODO: set the context-dependent initial value of properties */
	/** The Power Electronics module is not meant to be operational, it is just
	a parent class for modules that implement it.
	double ZB, SB, EB;
	complex tst;
		if (Gen_mode==UNKNOWN)
	{
		OBJECT *obj = OBJECTHDR(this);
		throw("Generator control mode is not specified");
	}
		if (Gen_status==0)
	{
		//OBJECT *obj = OBJECTHDR(this);
		throw("Generator is out of service!");
	}else
		{
			if (Rated_kW!=0.0)  SB = Rated_kW/sqrt(1-Rated_pf*Rated_pf);
			if (Rated_kVA!=0.0)  SB = Rated_kVA/3;
			if (Rated_kV!=0.0)  EB = Rated_kV/sqrt(3.0);
			if (SB!=0.0)  ZB = EB*EB/SB;
			else throw("Generator power capacity not specified!");
			double Real_Rinternal = Rinternal * ZB; 
			double Real_Rload = Rload * ZB;
			double Real_Rtotal = Rtotal * ZB;
			double Real_Rphase = Rphase * ZB;
			double Real_Rground = Rground * ZB;
			double Real_Rground_storage = Rground_storage * ZB;
			double[3] Real_Rfilter = Rfilter * ZB;

			double Real_Cinternal = Cinternal * ZB;
			double Real_Cground = Cground * ZB;
			double Real_Ctotal = Ctotal * ZB;
			double[3] Real_Cfilter = Cfilter * ZB;

			double Real_Linternal = Linternal * ZB;
			double Real_Lground = Lground * ZB;
			double Real_Ltotal = Ltotal * ZB;
			double[3] Real_Lfilter = Lfilter * ZB;

			tst = complex(Real_Rground,Real_Lground);
			AMx[0][0] = complex(Real_Rinternal,Real_Linternal) + tst;
			AMx[1][1] = complex(Real_Rinternal,Real_Linternal) + tst;
			AMx[2][2] = complex(Real_Rinternal,Real_Linternal) + tst;
		//	AMx[0][0] = AMx[1][1] = AMx[2][2] = complex(Real_Rs+Real_Rg,Real_Xs+Real_Xg);
			AMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst;
		}


	return 1; /* return 1 on success, 0 on failure */

	return 1;
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP power_electronics::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	//current_A = current_B = current_C = 0.0;

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP power_electronics::sync(TIMESTAMP t0, TIMESTAMP t1) 
{

//	complex PA,QA,PB,QB,PC,QC;
//	double Rated_kVar;
//	if (Gen_mode==CONSTANTEf)
//	{
//		current_A += AMx[0][0]*(voltage_A-EfA) + AMx[0][1]*(voltage_B-EfB) + AMx[0][2]*(voltage_C-EfC);
//		current_B += AMx[1][0]*(voltage_A-EfA) + AMx[1][1]*(voltage_B-EfB) + AMx[1][2]*(voltage_C-EfC);
//		current_C += AMx[2][0]*(voltage_A-EfA) + AMx[2][1]*(voltage_B-EfB) + AMx[2][2]*(voltage_C-EfC);
// //       PA = Re(voltage_A * phaseA_I_in); 
////		QA = Im(voltage_A * phaseA_I_in); 
//	
//	}else if (Gen_mode==CONSTANTPQ){
///*		if ((Rated_kW!=0.0)&&(Rated_pf!=0.0)){
//			Rated_kVar = Rated_kW *sqrt(1-Rated_pf^2)/Rated_pf;
//		}
//*/
//		current_A = - (~(complex(power_A_sch)/voltage_A/3));
//		current_B = - (~(complex(power_B_sch)/voltage_B/3));
//		current_C = - (~(complex(power_C_sch)/voltage_C/3));
//	}
//
//
TIMESTAMP t2 = TS_NEVER;
//	/* TODO: implement bottom-up behavior */
return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP power_electronics::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	///* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_power_electronics(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(power_electronics::oclass);
		if (*obj!=NULL)
		{
			power_electronics *my = OBJECTDATA(*obj,power_electronics);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(power_electronics);
}

EXPORT int init_power_electronics(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,power_electronics)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(power_electronics);
}

EXPORT TIMESTAMP sync_power_electronics(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	power_electronics *my = OBJECTDATA(obj,power_electronics);
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
	SYNC_CATCHALL(power_electronics);
	return t2;
}
