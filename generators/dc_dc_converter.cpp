/** $Id: dc_dc_converter.cpp,v 1.0 2008/07/15 
	Copyright (C) 2008 Battelle Memorial Institute
	@file dc_dc_converter.cpp
	@defgroup dc_dc_converter
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"

#include "power_electronics.h"
#include "dc_dc_converter.h"

#define DEFAULT 1.0;

//CLASS *dc_dc_converter::plcass = power_electronics;
CLASS *dc_dc_converter::oclass = NULL;
dc_dc_converter *dc_dc_converter::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
dc_dc_converter::dc_dc_converter(MODULE *module)
{	
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"dc_dc_converter",sizeof(dc_dc_converter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class dc_dc_converter";
		else
			oclass->trl = TRL_PROOF;
		
		if (gl_publish_variable(oclass,

			PT_enumeration,"dc_dc_converter_type",PADDR(dc_dc_converter_type_v),
				PT_KEYWORD,"BUCK",(enumeration)BUCK,
				PT_KEYWORD,"BOOST",(enumeration)BOOST,
				PT_KEYWORD,"BUCK_BOOST",(enumeration)BUCK_BOOST,

			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN,

			PT_complex, "V_Out[V]",PADDR(V_Out),
			PT_complex, "I_Out[A]",PADDR(I_Out),
			PT_complex, "Vdc[V]", PADDR(Vdc),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),
			PT_double, "P_Out", PADDR(P_Out),
			PT_double, "Q_Out", PADDR(Q_Out),

			PT_double, "service_ratio", PADDR(service_ratio),

			PT_complex, "V_In[V]",PADDR(V_In),
			PT_complex, "I_In[A]",PADDR(I_In),
			PT_complex, "VA_In[VA]", PADDR(VA_In),

			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;




		memset(this,0,sizeof(dc_dc_converter));
		/* TODO: set the default values of all properties here */
	}
}




/* Object creation is called once for each object that is created by the core */
int dc_dc_converter::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int dc_dc_converter::init(OBJECT *parent)
{

	gl_warning("DC_DC_CONVERTER IS AN EXPERIMENTAL MODEL. IT HAS NOT BEEN PROPERLY VALIDATED.");
	
				//initialize variables that are used internally
	//set_terminal_voltage = 240; //V
	//max_current_step_size = 100; //A
	Rated_kW = 1000; //< nominal power in kW
	Max_P = 1000;//< maximum real power capacity in kW
    Min_P = 0;//< minimum real power capacity in kW
	//Max_Q = 1000;//< maximum reactive power capacity in kVar
    //Min_Q = 1000;//< minimum reactive power capacity in kVar
	Rated_kVA = 2500; //< nominal capacity in kVA
	Rated_kV = 100; //< nominal line-line voltage in kV
	Rinternal = 0.035;
	Rload = 1;
	Rtotal = 0.05;
	//Rphase = 0.03;
	Rground = 0.03;
	Rground_storage = 0.05;

	Cinternal = 0;
	Cground = 0;
	Ctotal = 0;
	Linternal = 0;
	Lground = 0;
	Ltotal = 0;
	filter_120HZ = false;
	filter_180HZ = false;
	filter_240HZ = false;
	pf_in = 0;
	pf_out = 0;
	number_of_phases_in = 0;
	number_of_phases_out = 0;
	phaseAIn = false;
	phaseBIn = false;
	phaseCIn = false;
	phaseAOut = false;
	phaseBOut = false;
	phaseCOut = false;

	switch_type_choice = IDEAL_SWITCH;
	//generator_mode_choice = CONSTANT_PQ;
	gen_status_v = ONLINE;
	filter_type_v = BAND_PASS;
	filter_imp_v = IDEAL_FILTER;
	dc_dc_converter_type_v = BUCK_BOOST;
	power_in = DC;
	power_out = DC;


	P_Out = 500;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	Q_Out = 0;

	margin = 50;
	V_Set = 480;
	I_out_prev = 0;
	I_step_max = 50;
	internal_losses = 0;
	C_Storage_In = 0;
	C_Storage_Out = 0;
	losses = 0;
	efficiency = 0;

	//duty_ratio = 0;
	service_ratio = 2; //if greater than 1, output voltage > input voltage
	//if less than 1, output voltage < input voltage.
	//input_frequency = 2000;
	//frequency_losses = 0;
	
	/* TODO: set the context-dependent initial value of properties */
	

gl_verbose("dc_dc_converter init: initialized the variables");
	
	struct {
		complex **var;
		char *varname;
	} map[] = {
		// local object name,	meter object name
		{&pCircuit_V,			"V_In"},
		{&pLine_I,				"I_In"},
	};
	 

	static complex default_line_voltage[1], default_line_current[1];
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL && strcmp(parent->oclass->name,"meter")==0)
	{
		parent_string = "meter";
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);

		gl_verbose("dc_dc_converter init: mapped METER objects to internal variables");
	}
	else if (parent!=NULL && ((strcmp(parent->oclass->name,"inverter")==0))){
		gl_verbose("dc_dc_converter init: parent WAS found, is an inverter!");
		parent_string = "inverter";
			// construct circuit variable map to meter
			/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)

		for (i=0; i<sizeof(map)/sizeof(map[0]); i++){
			*(map[i].var) = get_complex(parent,map[i].varname);
		}
		gl_verbose("dc_dc_converter init: mapped INVERTER objects to local variables");
	}
	else if (parent!=NULL && strcmp(parent->oclass->name,"battery")==0){
		gl_verbose("dc_dc_converter init: parent WAS found, is an battery!");
		parent_string = "dc_dc_converter";
			// construct circuit variable map to meter
			/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)

		for (i=0; i<sizeof(map)/sizeof(map[0]); i++){
			*(map[i].var) = get_complex(parent,map[i].varname);
		}
		gl_verbose("dc_dc_converter init: mapped BATTERY objects to internal variables");
	}
	else{
		
			// construct circuit variable map to meter
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
		gl_verbose("dc_dc_converter init: mapped meter objects to internal variables");

		OBJECT *obj = OBJECTHDR(this);
		gl_verbose("dc_dc_converter init: no parent meter defined, parent is not a meter");
		gl_warning("dc_dc_converter:%d %s", obj->id, parent==NULL?"has no parent meter defined":"parent is not a meter");

		// attach meter variables to each circuit in the default_meter
			*(map[0].var) = &default_line_voltage[0];
			*(map[1].var) = &default_line_current[0];

		// provide initial values for voltages
			default_line_voltage[0] = complex(0,0);
			default_line_current[0] = complex(0,0);
			//default_line123_voltage[1] = complex(V_Max/sqrt(3.0)*cos(2*PI/3),V_Max/sqrt(3.0)*sin(2*PI/3));
			//default_line123_voltage[2] = complex(V_Max/sqrt(3.0)*cos(-2*PI/3),V_Max/sqrt(3.0)*sin(-2*PI/3));

	}

	gl_verbose("dc_dc_converter init: finished connecting with meter");




	if (gen_mode_v==UNKNOWN)
	{
		OBJECT *obj = OBJECTHDR(this);
		throw("Generator control mode is not specified");
	}
		if (gen_status_v== dc_dc_converter::OFFLINE)
	{
		//OBJECT *obj = OBJECTHDR(this);
		throw("Generator is out of service!");
	}else
		{

			//initialize variables that are used internally
			
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
			switch(dc_dc_converter_type_v){
				case BUCK:
					efficiency = 0.9;
					break;
				case BOOST:
					efficiency = 0.9;
					break;
				case BUCK_BOOST:
					efficiency = 0.8;
					break;
				default:
					efficiency = 0.8;
					break;
			}

			internal_switch_resistance(switch_type_choice);
			filter_circuit_impact((power_electronics::FILTER_TYPE)filter_type_v, 
					(power_electronics::FILTER_IMPLEMENTATION)filter_imp_v);

		}

	return 1;
}



complex *dc_dc_converter::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}




/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP dc_dc_converter::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	I_Out = VA_Out = 0.0;

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP dc_dc_converter::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	
	V_Out = pCircuit_V[0];
	
	gl_verbose("dc_dc_c sync: got voltage from parent, is: %f %fj", V_Out.Re(),V_Out.Im());
	
	internal_losses = 1 - calculate_loss(Rtotal, Ltotal, Ctotal, DC, AC);
		
	//TODO: consider installing duty or on-ratio limits
	//switch(dc_dc_converter_type_v){
	//		case BUCK:
	//			if(V_Out.Re() > V_In.Re()){
	//				throw("Buck converters can not increase the voltage level!");
	//			}else{
	//				duty_ratio = V_Out.Re() / V_In.Re();
	//			}
	//			break;
	//		case BOOST:
	//			if(V_Out.Re() < V_In.Re()){
	//				throw("Boost converters can not decrease the voltage level!");
	//			}else{
	//				on_ratio = (V_Out.Re()/V_In.Re()) - 1;
	//			}
	//			break;
	//		case BUCK_BOOST:
	//			on_ratio = - V_Out.Re() / V_In.Re();
	//			break;
	//		default:
	//			break;
	//}


	switch(gen_mode_v){
		case SUPPLY_DRIVEN:
			gl_verbose("dc_dc_c sync: supply driven");
			{//TODO
			//set V_Out for each phase
			//set V_In from line
			//set I_In from line
			//set RLoad for this time step if it isn't constant
			
			VA_In = V_In * ~ I_In; //DC
		

			VA_Out = VA_In * efficiency * internal_losses;
			losses = VA_Out * Rtotal / (Rtotal + Rload);
			VA_Out = VA_Out * Rload / (Rtotal + Rload);
			/*
			if (number_of_phases_out == 3){
				power_A = power_B = power_C = VA_Out /3;
				phaseA_I_Out = (power_A / phaseA_V_Out)/sqrt(2.0);
				phaseB_I_Out = (power_B / phaseB_V_Out)/sqrt(2.0);
				phaseC_I_Out = (power_C / phaseC_V_Out)/sqrt(2.0);
			}else if(number_of_phases_out == 1){
				if(phaseA_V_Out != 0){
					power_A = VA_Out;
					phaseA_I_Out = (power_A / phaseA_V_Out) / sqrt(2.0);
				}else if(phaseB_V_Out != 0){
					power_B = VA_Out;
					phaseB_I_Out = (power_B / phaseB_V_Out) / sqrt(2.0);
				}else if(phaseC_V_Out != 0){
					power_C = VA_Out;
					phaseC_I_Out = (power_C / phaseC_V_Out) / sqrt(2.0);
				}else{
					throw ("none of the phases have voltages!");
				}
			}else{
				throw ("unsupported number of phases");
			}
			*/

			I_Out = VA_Out / V_Out;
			I_Out = ~ I_Out;

			pLine_I[0] = I_Out;

			return TS_NEVER;
			}
			break;
		case CONSTANT_PQ:
			gl_verbose("dc_dc_c sync: constant pq");
			{//TODO
			//gather V_Out for each phase
			//gather V_In (DC) from line
			//P_Out is either set or input from elsewhere
			//Q_Out is either set or input from elsewhere
			//Gather Rload

			//VA_Out = sqrt(pow(P_Out, 2) + pow(Q_Out, 2));
			//pf_out = P_Out/VA_Out.Mag();
			
			//VA_Out = VA_In * efficiency * internal_losses;
/*
			if (number_of_phases_out == 3){
				power_A = power_B = power_C = VA_Out /3;
				phaseA_I_Out = (power_A / phaseA_V_Out)/sqrt(2.0);
				phaseB_I_Out = (power_B / phaseB_V_Out)/sqrt(2.0);
				phaseC_I_Out = (power_C / phaseC_V_Out)/sqrt(2.0);
			}else if(number_of_phases_out == 1){
				if(phaseAOut){
					power_A = VA_Out;
					phaseA_I_Out = (power_A / phaseA_V_Out) / sqrt(2.0);
				}else if(phaseBOut){
					power_B = VA_Out;
					phaseB_I_Out = (power_B / phaseB_V_Out) / sqrt(2.0);
				}else if(phaseCOut){
					power_C = VA_Out;
					phaseC_I_Out = (power_C / phaseC_V_Out) / sqrt(2.0);
				}else{
					throw ("none of the phases have voltages!");
				}
			}else{
				throw ("unsupported number of phases");
			}
*/
			
			if(parent_string = "meter"){
				VA_Out = complex(P_Out,Q_Out);
				gl_verbose("dc_dc_c sync: VA_Out set is: (%f , %f)", VA_Out.Re(), VA_Out.Im());
			}else{
				I_Out = pLine_I[0];
				gl_verbose("dc_dc_c sync: V_In requested is: (%f , %f)", V_In.Re(), V_In.Im());
				VA_Out = V_Out * ~ I_Out;
				gl_verbose("dc_dc_c sync: VA_Out set is: (%f , %f)", VA_Out.Re(), VA_Out.Im());
			}

			V_In = V_Out / service_ratio;

			VA_In = VA_Out / (efficiency * internal_losses);

			gl_verbose("dc_dc_c sync: VA_In requested is: (%f , %f)", VA_In.Re(), VA_In.Im());

			losses = VA_Out * (1 - (efficiency * internal_losses));

			if(V_In == 0){
				I_In = 0;
			}else{
				I_In = VA_In / V_In;
				I_In = ~I_In;
			}

			gl_verbose("dc_dc_c sync: I_In requested is: (%f , %f)", I_In.Re(), I_In.Im());

			return TS_NEVER;
			}
			break;
		case CONSTANT_V:
			gl_verbose("dc_dc_c sync: constant v");
			{
			bool changed = false;
			
			//TODO
			//Gather V_Out
			//Gather VA_Out
			//Gather Rload
			I_Out = pLine_I[0];

			gl_verbose("dc_dc_c sync: I from parent is: (%f , %f)", I_Out.Re(), I_Out.Im());
			
			V_In = V_Out / service_ratio;


			gl_verbose("dc_dc_c sync: V_Out from parent is: (%f , %f)", V_Out.Re(), V_Out.Im());
			gl_verbose("dc_dc_c sync: V_In calculated: (%f , %f)", V_In.Re(), V_In.Im());
			
			if (V_Out < (V_Set - margin)){
				I_Out = I_out_prev + I_step_max / 2;
				changed = true;
			}else if (V_Out > (V_Set + margin)){
				I_Out = I_out_prev - I_step_max / 2;
				changed = true;
			}else{
				changed = false;
			}

			gl_verbose("dc_dc_c sync: I_In after adjustment is: (%f , %f)", I_Out.Re(), I_Out.Im());
	

			VA_Out = (~I_Out) * V_Out;

			gl_verbose("dc_dc_c sync: VA_Out pre-limits is: (%f , %f)", VA_Out.Re(), VA_Out.Im());

			if(VA_Out > Rated_kVA){
				VA_Out = Rated_kVA;
				I_Out = VA_Out / V_Out;
				I_Out = ~I_Out;
				changed = false;
			}
			
			if(VA_Out < 0){
				VA_Out = 0;
				I_Out = 0;
				changed = false;
			}



			/*
			
			if(phaseAOut){
				if (phaseA_V_Out < (V_Set_A - margin)){
					phaseA_I_Out = phaseA_I_Out_prev + I_step_max/2;
					changed = true;
				}else if (phaseA_V_Out > (V_Set_A + margin)){
					phaseA_I_Out = phaseA_I_Out_prev - I_step_max/2;
					changed = true;
				}else{changed = false;}
			}if (phaseBOut){
				if (phaseB_V_Out < (V_Set_B - margin)){
					phaseB_I_Out = phaseB_I_Out_prev + I_step_max/2;
					changed = true;
				}else if (phaseB_V_Out > (V_Set_B + margin)){
					phaseB_I_Out = phaseB_I_Out_prev - I_step_max/2;
					changed = true;
				}else{changed = false;}
			}if (phaseCOut){
				if (phaseC_V_Out < (V_Set_C - margin)){
					phaseC_I_Out = phaseC_I_Out_prev + I_step_max/2;
					changed = true;
				}else if (phaseC_V_Out > (V_Set_C + margin)){
					phaseC_I_Out = phaseC_I_Out_prev - I_step_max/2;
					changed = true;
				}else{changed = false;}
			}
			
			power_A = phaseA_I_Out * phaseA_V_Out;
			power_B = phaseB_I_Out * phaseB_V_Out;
			power_C = phaseC_I_Out * phaseC_V_Out;

			//check if dc_dc_converter is overloaded -- if so, cap at max power
			if (((power_A + power_B + power_C) > Rated_kVA) ||
				((power_A.Re() + power_B.Re() + power_C.Re()) > Max_P) ||
				((power_A.Im() + power_B.Im() + power_C.Im()) > Max_Q))
			{
				VA_Out = Rated_kVA / number_of_phases_out;
				if(phaseAOut){
					phaseA_I_Out = VA_Out / phaseA_V_Out;
				}
				if(phaseBOut){
					phaseB_I_Out = VA_Out / phaseB_V_Out;
				}
				if(phaseCOut){
					phaseC_I_Out = VA_Out / phaseC_V_Out;
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

			*/

			gl_verbose("dc_dc_c sync: VA_In requested pre-losses is: (%f , %f)", VA_Out.Re(), VA_Out.Im());

			VA_In = VA_Out / (efficiency * internal_losses);
			losses = VA_Out * (1 - (efficiency * internal_losses));

			gl_verbose("dc_dc_c sync: VA_In requested with losses is: (%f , %f)", VA_In.Re(), VA_In.Im());


			if(V_In == 0){
				I_In = 0;
			}else{
				I_In = VA_In / V_In;
				I_In = ~I_In;
			}

			gl_verbose("dc_dc_c sync: I_In requested is: (%f , %f)", I_In.Re(), I_In.Im());

			//TODO: check P and Q components to see if within bounds

			if(changed){
				TIMESTAMP t2 = t1 + 10 * 60 * TS_SECOND;
				return t2;
			}else{
				return TS_NEVER;
			}
			}
			break;
		default:
			break;

	return TS_NEVER;

	}
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
TIMESTAMP dc_dc_converter::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	///* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_dc_dc_converter(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(dc_dc_converter::oclass);
		if (*obj!=NULL)
		{
			dc_dc_converter *my = OBJECTDATA(*obj,dc_dc_converter);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	} 
	CREATE_CATCHALL(dc_dc_converter);
}

EXPORT int init_dc_dc_converter(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,dc_dc_converter)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(dc_dc_converter);
}

EXPORT TIMESTAMP sync_dc_dc_converter(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	dc_dc_converter *my = OBJECTDATA(obj,dc_dc_converter);
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
	SYNC_CATCHALL(dc_dc_converter);
	return t2;
}
