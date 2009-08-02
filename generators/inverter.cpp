/** $Id: inverter.cpp,v 1.0 2008/07/15 
	Copyright (C) 2008 Battelle Memorial Institute
	@file inverter.cpp
	@defgroup inverter
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"

#include "power_electronics.h"
#include "inverter.h"

#define DEFAULT 1.0;

//CLASS *inverter::plcass = power_electronics;
CLASS *inverter::oclass = NULL;
inverter *inverter::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
inverter::inverter(MODULE *module)
{	
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"inverter",sizeof(inverter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__); 
		
		if (gl_publish_variable(oclass,

			PT_enumeration,"inverter_type",PADDR(inverter_type_v),
				PT_KEYWORD,"TWO_PULSE",TWO_PULSE,
				PT_KEYWORD,"SIX_PULSE",SIX_PULSE,
				PT_KEYWORD,"TWELVE_PULSE",TWELVE_PULSE,
				PT_KEYWORD,"PWM",PWM,

			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",SUPPLY_DRIVEN,

			
			PT_complex, "V_In[V]",PADDR(V_In),
			PT_complex, "I_In[A]",PADDR(I_In),
			PT_complex, "VA_In[VA]", PADDR(VA_In),
			PT_complex, "Vdc[V]", PADDR(Vdc),
			PT_complex, "phaseA_V_Out[V]", PADDR(phaseA_V_Out),
			PT_complex, "phaseB_V_Out[V]", PADDR(phaseB_V_Out),
			PT_complex, "phaseC_V_Out[V]", PADDR(phaseC_V_Out),
			PT_complex, "phaseA_I_Out[V]", PADDR(phaseA_I_Out),
			PT_complex, "phaseB_I_Out[V]", PADDR(phaseB_I_Out),
			PT_complex, "phaseC_I_Out[V]", PADDR(phaseC_I_Out),
			PT_complex, "power_A[VA]", PADDR(power_A),
			PT_complex, "power_B[VA]", PADDR(power_B),
			PT_complex, "power_C[VA]", PADDR(power_C),

			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),



			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		

	

		memset(this,0,sizeof(inverter));
		/* TODO: set the default values of all properties here */
	}
}




/* Object creation is called once for each object that is created by the core */
int inverter::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int inverter::init(OBJECT *parent)
{
	
	gl_verbose("inverter init: started");
	/* TODO: set the context-dependent initial value of properties */

//initialize variables that are used internally
	//set_terminal_voltage = 240; //V
	//max_current_step_size = 100; //A
	Rated_kW = 1000; //< nominal power in kW
	Max_P = 1000;//< maximum real power capacity in kW
    Min_P = 0;//< minimum real power capacity in kW
	Max_Q = 1000;//< maximum reactive power capacity in kVar
    Min_Q = 1000;//< minimum reactive power capacity in kVar
	Rated_kVA = 1500; //< nominal capacity in kVA
	Rated_kV = 10; //< nominal line-line voltage in kV
	Rinternal = 0.035;
	Rload = 1;
	Rtotal = 0.05;
	//Rphase = 0.03;
	Rground = 0.03;
	Rground_storage = 0.05;
	Vdc = 480;

	Cinternal = 0;
	Cground = 0;
	Ctotal = 0;
	Linternal = 0;
	Lground = 0;
	Ltotal = 0;
	filter_120HZ = true;
	filter_180HZ = true;
	filter_240HZ = true;
	pf_in = 0;
	pf_out = 1;
	number_of_phases_in = 0;
	number_of_phases_out = 3;
	phaseAIn = false;
	phaseBIn = false;
	phaseCIn = false;
	phaseAOut = true;
	phaseBOut = true;
	phaseCOut = true;

	switch_type_choice = IDEAL_SWITCH;
	//generator_mode_choice = CONSTANT_PQ;
	gen_status_v = ONLINE;
	filter_type_v = BAND_PASS;
	filter_imp_v = IDEAL_FILTER;
	inverter_type_v = PWM;
	power_in = DC;
	power_out = AC;

	
	V_In_Set_A = complex(480,0);
	V_In_Set_B = complex(-240, 415.69);
	V_In_Set_C = complex(-240,-415.69);


	P_Out = 1200;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	Q_Out = 500;


	V_Set_A = 240;
	V_Set_B = 240;
	V_Set_C = 240;
	margin = 10;
	I_out_prev = 0;
	I_step_max = 100;
	internal_losses = 0;
	C_Storage_In = 0;
	efficiency = 0;

	gl_verbose("inverter init: finished initializing variables");

// construct circuit variable map to meter
	struct {
		complex **var;
		char *varname;
	} map[] = {
		// local object name,	meter object name
		{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
		{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	};

	static complex default_line123_voltage[3], default_line1_current[3];
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL && strcmp(parent->oclass->name,"meter")==0)
	{
		gl_verbose("inverter init: parent found and it is a METER");
		// attach meter variables to each circuit
		parent_string = "meter";
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);
	}
	else
	{
		OBJECT *obj = OBJECTHDR(this);
		parent_string = "none";
		gl_verbose("inverter init: error, no parent defined!");
		gl_warning("inverter:%d %s", obj->id, parent==NULL?"has no parent meter defined":"parent is not a meter");

		// attach meter variables to each circuit in the default_meter
			*(map[0].var) = &default_line123_voltage[0];
			*(map[1].var) = &default_line1_current[0];

		// provide initial values for voltages
			default_line123_voltage[0] = complex(Rated_kV*1000/sqrt(3.0),0);
			default_line123_voltage[1] = complex(Rated_kV*1000/sqrt(3.0)*cos(2*PI/3),Rated_kV*1000/sqrt(3.0)*sin(2*PI/3));
			default_line123_voltage[2] = complex(Rated_kV*1000/sqrt(3.0)*cos(-2*PI/3),Rated_kV*1000/sqrt(3.0)*sin(-2*PI/3));

	}

	gl_verbose("inverter init: connected to meter");

	if (gen_mode_v==UNKNOWN)
	{
		OBJECT *obj = OBJECTHDR(this);
		throw("Generator control mode is not specified");
	}
		if (gen_status_v== inverter::OFFLINE)
	{
		//OBJECT *obj = OBJECTHDR(this);
		throw("Generator is out of service!");
	}


			
			//need to check for parameters SWITCH_TYPE, FILTER_TYPE, FILTER_IMPLEMENTATION, GENERATOR_MODE
	/*
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

			*/

			//all other variables set in input file through public parameters
			switch(inverter_type_v){
				case TWO_PULSE:
					efficiency = 0.8;
					break;
				case SIX_PULSE:
					efficiency = 0.8;
					break;
				case TWELVE_PULSE:
					efficiency = 0.8;
					break;
				case PWM:
					efficiency = 0.9;
					break;
				default:
					efficiency = 0.8;
					break;
			}

			gl_verbose("inverter init: set efficiencies");
			internal_switch_resistance(switch_type_choice);
			filter_circuit_impact(filter_type_v, filter_imp_v);

		
	gl_verbose("inverter init: about to exit");

	return 1;
}


complex * inverter::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}



/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP inverter::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	phaseA_I_Out = phaseB_I_Out = phaseC_I_Out = 0.0;
	//power_A = power_B = power_C = 0.0;

	gl_verbose("inverter presync: about to exit");

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP inverter::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	gl_verbose("inverter sync: started");
	phaseA_V_Out = pCircuit_V[0];	//Syncs the meter parent to the generator.
	phaseB_V_Out = pCircuit_V[1];
	phaseC_V_Out = pCircuit_V[2];

	gl_verbose("inverter sync: Phase A Voltage from parent (meter) is: (%f , %f)", phaseA_V_Out.Re(), phaseA_V_Out.Im());
	gl_verbose("inverter sync: Phase B Voltage from parent (meter) is: (%f , %f)", phaseB_V_Out.Re(), phaseB_V_Out.Im());
	gl_verbose("inverter sync: Phase C Voltage from parent (meter) is: (%f , %f)", phaseC_V_Out.Re(), phaseC_V_Out.Im());

	
	internal_losses = 1 - calculate_loss(Rtotal, Ltotal, Ctotal, DC, AC);
	gl_verbose("inverter sync: internal losses are: %f", 1 - internal_losses);
	frequency_losses = 1 - calculate_frequency_loss(output_frequency, Rtotal,Ltotal, Ctotal);
	gl_verbose("inverter sync: frequency losses are: %f", 1 - frequency_losses);

	switch(gen_mode_v){
		case SUPPLY_DRIVEN:
			{
				gl_verbose("inverter sync: supply driven");
				//TODO
			//set V_Out for each phase
			//set V_In from line
			//set I_In from line
			//set RLoad for this time step if it isn't constant
			
			VA_In = V_In * ~ I_In; //DC

			// need to differentiate between different pulses...
		
			VA_Out = VA_In * efficiency * internal_losses * frequency_losses;
			gl_verbose("inverter sync: VA Out before Rtotal is: (%f , %f)", VA_Out.Re(), VA_Out.Im());
			losses = VA_Out * Rtotal / (Rtotal + Rload);
			VA_Out = VA_Out * Rload / (Rtotal + Rload);
			gl_verbose("inverter sync: VA Out final is: (%f , %f)", VA_Out.Re(), VA_Out.Im());

			
			gl_verbose("inverter sync: got power");

			if (number_of_phases_out == 3){
				power_A = power_B = power_C = VA_Out /3;
				phaseA_I_Out = (power_A / phaseA_V_Out); // /sqrt(2.0);
				phaseB_I_Out = (power_B / phaseB_V_Out); // /sqrt(2.0);
				phaseC_I_Out = (power_C / phaseC_V_Out); // /sqrt(2.0);

				phaseA_I_Out = ~ phaseA_I_Out;
				phaseB_I_Out = ~ phaseB_I_Out;
				phaseC_I_Out = ~ phaseC_I_Out;
				
				gl_verbose("inverter sync: phaseA_I_Out pre-filter is (%f , %f)", phaseA_I_Out.Re(), phaseA_I_Out.Im());
				gl_verbose("inverter sync: phaseB_I_Out pre-filter is (%f , %f)", phaseB_I_Out.Re(), phaseB_I_Out.Im());
				gl_verbose("inverter sync: phaseC_I_Out pre-filter is (%f , %f)", phaseC_I_Out.Re(), phaseC_I_Out.Im());

				gl_verbose("inverter sync: phaseA_V_Out pre-filter is (%f , %f)", phaseA_V_Out.Re(), phaseA_V_Out.Im());
				gl_verbose("inverter sync: phaseB_V_Out pre-filter is (%f , %f)", phaseB_V_Out.Re(), phaseB_V_Out.Im());
				gl_verbose("inverter sync: phaseC_V_Out pre-filter is (%f , %f)", phaseC_V_Out.Re(), phaseC_V_Out.Im());

				complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
				complex phaseB_V_Internal = filter_voltage_impact_source(phaseB_I_Out, phaseB_V_Out);
				complex phaseC_V_Internal = filter_voltage_impact_source(phaseC_I_Out, phaseC_V_Out);

				
				gl_verbose("inverter sync: phaseA_V_Internal is (%f , %f)", phaseA_V_Internal.Re(), phaseA_V_Internal.Im());
				gl_verbose("inverter sync: phaseB_V_Internal is (%f , %f)", phaseB_V_Internal.Re(), phaseB_V_Internal.Im());
				gl_verbose("inverter sync: phaseC_V_Internal is (%f , %f)", phaseC_V_Internal.Re(), phaseC_V_Internal.Im());

				phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
				phaseB_I_Out = filter_current_impact_out(phaseB_I_Out, phaseB_V_Internal);
				phaseC_I_Out = filter_current_impact_out(phaseC_I_Out, phaseC_V_Internal);


			}else if(number_of_phases_out == 1){
				if(phaseA_V_Out != 0){
					power_A = VA_Out;
					phaseA_I_Out = (power_A / phaseA_V_Out); 
					phaseA_I_Out = ~ phaseA_I_Out;
					complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
					phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
				}else if(phaseB_V_Out != 0){
					power_B = VA_Out;
					phaseB_I_Out = (power_B / phaseB_V_Out); 
					phaseB_I_Out = ~ phaseB_I_Out;
					complex phaseB_V_Internal = filter_voltage_impact_source(phaseB_I_Out, phaseB_V_Out);
					phaseB_I_Out = filter_current_impact_out(phaseB_I_Out, phaseB_V_Internal);
				}else if(phaseC_V_Out != 0){
					power_C = VA_Out;
					phaseC_I_Out = (power_C / phaseC_V_Out); 
					phaseC_I_Out = ~ phaseC_I_Out;
					complex phaseC_V_Internal = filter_voltage_impact_source(phaseC_I_Out, phaseC_V_Out);
					phaseC_I_Out = filter_current_impact_out(phaseC_I_Out, phaseC_V_Internal);
				}else{
					throw ("none of the phases have voltages!");
				}
			}else{
				throw ("unsupported number of phases");
			}

			pLine_I[0] = phaseA_I_Out;
			pLine_I[1] = phaseB_I_Out;
			pLine_I[2] = phaseC_I_Out;

			gl_verbose("inverter sync: phaseA_I_Out is (%f , %f)", phaseA_I_Out.Re(), phaseA_I_Out.Im());
			gl_verbose("inverter sync: phaseB_I_Out is (%f , %f)", phaseB_I_Out.Re(), phaseB_I_Out.Im());
			gl_verbose("inverter sync: phaseC_I_Out is (%f , %f)", phaseC_I_Out.Re(), phaseC_I_Out.Im());

			gl_verbose("inverter sync: power_A_Out is (%f , %f)", power_A.Re(), power_A.Im());
			gl_verbose("inverter sync: power_B_Out is (%f , %f)", power_B.Re(), power_B.Im());
			gl_verbose("inverter sync: power_C_Out is (%f , %f)", power_C.Re(), power_C.Im());

			return TS_NEVER;
			}
			break;
		case CONSTANT_PQ:
			{
				gl_verbose("inverter sync: constant pq");
				//TODO
			//gather V_Out for each phase
			//gather V_In (DC) from line -- can not gather V_In, for now set equal to V_Out
			//P_Out is either set or input from elsewhere
			//Q_Out is either set or input from elsewhere
			//Gather Rload

			if(parent_string = "meter"){
				VA_Out = complex(P_Out,Q_Out);
			}else{
				phaseA_I_Out = pLine_I[0];
				phaseB_I_Out = pLine_I[1];
				phaseC_I_Out = pLine_I[2];

				VA_Out = phaseA_V_Out * ~ phaseA_I_Out + phaseB_V_Out * ~ phaseB_I_Out + phaseC_V_Out * ~ phaseC_I_Out;
			}

			pf_out = P_Out/VA_Out.Mag();
			
			//VA_Out = VA_In * efficiency * internal_losses;

			

			if (number_of_phases_out == 3){
				power_A = power_B = power_C = VA_Out /3;
				phaseA_I_Out = (power_A / phaseA_V_Out); // /sqrt(2.0);
				phaseB_I_Out = (power_B / phaseB_V_Out); // /sqrt(2.0);
				phaseC_I_Out = (power_C / phaseC_V_Out); // /sqrt(2.0);

				phaseA_I_Out = ~ phaseA_I_Out;
				phaseB_I_Out = ~ phaseB_I_Out;
				phaseC_I_Out = ~ phaseC_I_Out;

			}else if(number_of_phases_out == 1){
				if(phaseAOut){
					power_A = VA_Out;
					phaseA_I_Out = (power_A / phaseA_V_Out); // /sqrt(2);
					phaseA_I_Out = ~ phaseA_I_Out;
				}else if(phaseBOut){
					power_B = VA_Out;
					phaseB_I_Out = (power_B / phaseB_V_Out);  // /sqrt(2);
					phaseB_I_Out = ~ phaseB_I_Out;
				}else if(phaseCOut){
					power_C = VA_Out;
					phaseC_I_Out = (power_C / phaseC_V_Out); // /sqrt(2);
					phaseC_I_Out = ~ phaseC_I_Out;
				}else{
					throw ("none of the phases have voltages!");
				}
			}else{
				throw ("unsupported number of phases");
			}

			VA_In = VA_Out / (efficiency * internal_losses * frequency_losses);
			losses = VA_Out * (1 - (efficiency * internal_losses * frequency_losses));

			//V_In = complex(0,0);
			//
			////is there a better way to do this?
			//if(phaseAOut){
			//	V_In += abs(phaseA_V_Out.Re());
			//}
			//if(phaseBOut){
			//	V_In += abs(phaseB_V_Out.Re());
			//}
			//if(phaseCOut){
			//	V_In += abs(phaseC_V_Out.Re());
			//}else{
			//	throw ("none of the phases have voltages!");
			//}

			V_In = Vdc;



			I_In = VA_In / V_In;
			I_In = ~I_In;

			V_In = filter_voltage_impact_source(I_In, V_In);
			I_In = filter_current_impact_source(I_In, V_In);

			gl_verbose("Inverter sync: V_In asked for by inverter is: (%f , %f)", V_In.Re(), V_In.Im());
			gl_verbose("Inverter sync: I_In asked for by inverter is: (%f , %f)", I_In.Re(), I_In.Im());


			pLine_I[0] = phaseA_I_Out;
			pLine_I[1] = phaseB_I_Out;
			pLine_I[2] = phaseC_I_Out;

			return TS_NEVER;
			}
			break;
		case CONSTANT_V:
			{
				gl_verbose("inverter sync: constant v");
			bool changed = false;
			
			//TODO
			//Gather V_Out
			//Gather VA_Out
			//Gather Rload
			if(phaseAOut){
				if (phaseA_V_Out.Re() < (V_Set_A - margin)){
					phaseA_I_Out = phaseA_I_Out_prev + I_step_max/2;
					changed = true;
				}else if (phaseA_V_Out.Re() > (V_Set_A + margin)){
					phaseA_I_Out = phaseA_I_Out_prev - I_step_max/2;
					changed = true;
				}else{changed = false;}
			}if (phaseBOut){
				if (phaseB_V_Out.Re() < (V_Set_B - margin)){
					phaseB_I_Out = phaseB_I_Out_prev + I_step_max/2;
					changed = true;
				}else if (phaseB_V_Out.Re() > (V_Set_B + margin)){
					phaseB_I_Out = phaseB_I_Out_prev - I_step_max/2;
					changed = true;
				}else{changed = false;}
			}if (phaseCOut){
				if (phaseC_V_Out.Re() < (V_Set_C - margin)){
					phaseC_I_Out = phaseC_I_Out_prev + I_step_max/2;
					changed = true;
				}else if (phaseC_V_Out.Re() > (V_Set_C + margin)){
					phaseC_I_Out = phaseC_I_Out_prev - I_step_max/2;
					changed = true;
				}else{changed = false;}
			}
			
			power_A = (~phaseA_I_Out) * phaseA_V_Out;
			power_B = (~phaseB_I_Out) * phaseB_V_Out;
			power_C = (~phaseC_I_Out) * phaseC_V_Out;

			//check if inverter is overloaded -- if so, cap at max power
			if (((power_A + power_B + power_C) > Rated_kVA) ||
				((power_A.Re() + power_B.Re() + power_C.Re()) > Max_P) ||
				((power_A.Im() + power_B.Im() + power_C.Im()) > Max_Q))
			{
				VA_Out = Rated_kVA / number_of_phases_out;
				//if it's maxed out, don't ask for the simulator to re-call
				changed = false;
				if(phaseAOut){
					phaseA_I_Out = VA_Out / phaseA_V_Out;
					phaseA_I_Out = (~phaseA_I_Out);
				}
				if(phaseBOut){
					phaseB_I_Out = VA_Out / phaseB_V_Out;
					phaseB_I_Out = (~phaseB_I_Out);
				}
				if(phaseCOut){
					phaseC_I_Out = VA_Out / phaseC_V_Out;
					phaseC_I_Out = (~phaseC_I_Out);
				}
			}
			
			//check if power is negative for some reason, should never be
			if(power_A < 0){
				power_A = 0;
				phaseA_I_Out = 0;
				throw("phaseA power is negative!");
			}
			if(power_B < 0){
				power_B = 0;
				phaseB_I_Out = 0;
				throw("phaseB power is negative!");
			}
			if(power_C < 0){
				power_C = 0;
				phaseC_I_Out = 0;
				throw("phaseC power is negative!");

			}
			VA_In = VA_Out / (efficiency * internal_losses * frequency_losses);
			losses = VA_Out * (1 - (efficiency * internal_losses * frequency_losses));

			//V_In = complex(0,0);
			//
			////is there a better way to do this?
			//if(phaseAOut){
			//	V_In += abs(phaseA_V_Out.Re());
			//}
			//if(phaseBOut){
			//	V_In += abs(phaseB_V_Out.Re());
			//}
			//if(phaseCOut){
			//	V_In += abs(phaseC_V_Out.Re());
			//}else{
			//	throw ("none of the phases have voltages!");
			//}

			V_In = Vdc;

			I_In = VA_In / V_In;
			I_In  = ~I_In;
			
			gl_verbose("Inverter sync: I_In asked for by inverter is: (%f , %f)", I_In.Re(), I_In.Im());

			V_In = filter_voltage_impact_source(I_In, V_In);
			I_In = filter_current_impact_source(I_In, V_In);


			//TODO: check P and Q components to see if within bounds

			if(changed){
				pLine_I[0] = phaseA_I_Out;
				pLine_I[1] = phaseB_I_Out;
				pLine_I[2] = phaseC_I_Out;
				
				TIMESTAMP t2 = t1 + 10 * 60 * TS_SECOND;
				return t2;
			}else{
				pLine_I[0] = phaseA_I_Out;
				pLine_I[1] = phaseB_I_Out;
				pLine_I[2] = phaseC_I_Out;
				return TS_NEVER;
			}
			}
			break;
		default:
			break;
	
	pLine_I[0] = phaseA_I_Out;
	pLine_I[1] = phaseB_I_Out;
	pLine_I[2] = phaseC_I_Out;
	gl_verbose("inverter sync: about to exit1!");
	gl_verbose("inverter sync: phaseA_I_Out is (%f , %f)", phaseA_I_Out.Re(), phaseA_I_Out.Im());
	gl_verbose("inverter sync: phaseB_I_Out is (%f , %f)", phaseB_I_Out.Re(), phaseB_I_Out.Im());
	gl_verbose("inverter sync: phaseC_I_Out is (%f , %f)", phaseC_I_Out.Re(), phaseC_I_Out.Im());


	return TS_NEVER;

	}
	
	pLine_I[0] = phaseA_I_Out;
	pLine_I[1] = phaseB_I_Out;
	pLine_I[2] = phaseC_I_Out;

	gl_verbose("inverter sync: about to exit2!");
	gl_verbose("inverter sync: phaseA_I_Out is (%f , %f)", phaseA_I_Out.Re(), phaseA_I_Out.Im());
	gl_verbose("inverter sync: phaseB_I_Out is (%f , %f)", phaseB_I_Out.Re(), phaseB_I_Out.Im());
	gl_verbose("inverter sync: phaseC_I_Out is (%f , %f)", phaseC_I_Out.Re(), phaseC_I_Out.Im());
	return TS_NEVER;
}

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
//TIMESTAMP t2 = TS_NEVER;
//	/* TODO: implement bottom-up behavior */
//return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
//}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP inverter::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	///* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_inverter(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(inverter::oclass);
		if (*obj!=NULL)
		{
			inverter *my = OBJECTDATA(*obj,inverter);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	} 
	catch (char *msg) 
	{
		gl_error("create_inverter: %s", msg);
	}
	
	return 0;
}

EXPORT int init_inverter(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,inverter)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_inverter(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	
	return 0;
}

EXPORT TIMESTAMP sync_inverter(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	inverter *my = OBJECTDATA(obj,inverter);
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
		gl_error("sync_inverter(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return t2;
}
