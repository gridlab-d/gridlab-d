/** $Id: rectifier.cpp,v 1.0 2008/07/15 
Copyright (C) 2008 Battelle Memorial Institute
@file rectifier.cpp
@defgroup rectifier
@ingroup generators

@{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"
#include "power_electronics.h"
#include "rectifier.h"

#define DEFAULT 1.0;

//CLASS *rectifier::plcass = power_electronics;
CLASS *rectifier::oclass = NULL;
rectifier *rectifier::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
rectifier::rectifier(MODULE *module)
{	
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"rectifier",sizeof(rectifier),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class rectifier";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,

			PT_enumeration,"rectifier_type",PADDR(rectifier_type_v),
			PT_KEYWORD,"ONE_PULSE",(enumeration)ONE_PULSE,
			PT_KEYWORD,"TWO_PULSE",(enumeration)TWO_PULSE,
			PT_KEYWORD,"THREE_PULSE",(enumeration)THREE_PULSE,
			PT_KEYWORD,"SIX_PULSE",(enumeration)SIX_PULSE,
			PT_KEYWORD,"TWELVE_PULSE",(enumeration)TWELVE_PULSE,

			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
			PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
			PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
			PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
			PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
			PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN,

			PT_complex, "V_Out[V]",PADDR(V_Out),
			PT_double, "V_Rated[V]",PADDR(V_Rated),
			PT_complex, "I_Out[A]",PADDR(I_Out),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),
			PT_double, "P_Out", PADDR(P_Out),
			PT_double, "Q_Out", PADDR(Q_Out),
			PT_complex, "Vdc[V]", PADDR(Vdc),
			PT_complex, "voltage_A[V]", PADDR(voltage_A),
			PT_complex, "voltage_B[V]", PADDR(voltage_B),
			PT_complex, "voltage_C[V]", PADDR(voltage_C),
			PT_complex, "current_A[V]", PADDR(current_A),
			PT_complex, "current_B[V]", PADDR(current_B),
			PT_complex, "current_C[V]", PADDR(current_C),
			PT_complex, "power_A_In[VA]", PADDR(power_A_In),
			PT_complex, "power_B_In[VA]", PADDR(power_B_In),
			PT_complex, "power_C_In[VA]", PADDR(power_C_In),

			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			PT_set, "phases", PADDR(phases),
			PT_KEYWORD, "A",(set)PHASE_A,
			PT_KEYWORD, "B",(set)PHASE_B,
			PT_KEYWORD, "C",(set)PHASE_C,
			PT_KEYWORD, "N",(set)PHASE_N,
			PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;






		memset(this,0,sizeof(rectifier));
		/* TODO: set the default values of all properties here */
	}
}




/* Object creation is called once for each object that is created by the core */
int rectifier::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int rectifier::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	//initialize variables that are used internally
	//set_terminal_voltage = 240; //V
	//max_current_step_size = 100; //A
	Rated_kW = 1000; //< nominal power in kW
	Max_P = 1000;//< maximum real power capacity in kW
	Min_P = 0;//< minimum real power capacity in kW
	//Max_Q = 1000;//< maximum reactive power capacity in kVar
	//Min_Q = 1000;//< minimum reactive power capacity in kVar
	Rated_kVA = 1500; //< nominal capacity in kVA
	Rated_kV = 10; //< nominal line-line voltage in kV
	Rinternal = 0.035;
	Rload = 1;
	Rtotal = 0.05;
	//XphaseA = complex(1 * 1,0);
	//XphaseB = complex(1 * -0.5, 1 * 0.866);
	//XphaseC = complex(1 * -0.5, 1 * -0.866);
	XphaseA = complex(5 * 1,0);
	XphaseB = complex(5 * 1,0);
	XphaseC = complex(5 * 1,0);
	Rground = 0.03;
	Rground_storage = 0.05;
	Vdc = 480;

	Cinternal = 0;
	Cground = 0;
	Ctotal = 0;
	Linternal = 0;
	Lground = 0;
	Ltotal = 0;
	filter_120HZ = false;
	filter_180HZ = false;
	filter_240HZ = false;
	pf_in = 1;
	pf_out = 0;
	number_of_phases_in = 3;
	number_of_phases_out = 0;
	phaseAIn = true;
	phaseBIn = true;
	phaseCIn = true;
	phaseAOut = false;
	phaseBOut = false;
	phaseCOut = false;

	V_In_Set_A = complex(360,0);
	V_In_Set_B = complex(-180, 311.769);
	V_In_Set_C = complex(-180,-311.769);

	switch_type_choice = IDEAL_SWITCH;
	//generator_mode_choice = CONSTANT_PQ;
	gen_status_v = ONLINE;
	filter_type_v = BAND_PASS;
	filter_imp_v = IDEAL_FILTER;
	rectifier_type_v = SIX_PULSE;
	power_in = AC;
	power_out = DC;


	P_Out = 500;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	Q_Out = 0;



	margin = 50;
	V_Rated = 360;
	I_out_prev = 0;
	I_step_max = 100;
	internal_losses = 0;
	C_Storage_Out = 0;
	efficiency = 0;
	losses = 0;

	on_ratio = 0;
	input_frequency = 2000;
	frequency_losses = 0;


	gl_verbose("rectifier init: initialized the variables");

	int i;

	if (parent!=NULL && gl_object_isa(parent,"meter"))
	{
		// TO DO: Figure out how to connect a DC device to meter.
	}

	if (parent!=NULL && gl_object_isa(parent,"inverter"))
	{
		parent_string = "inverter";

		/*struct {
			complex **var;
			char *varname;
		}
		map[] = {
			// local object name,	meter object name
			{&pCircuit_V,			"Vdc"}, 
			{&pLine_I,				"I_In"}
		};*/

		// attach meter variables to each circuit
			i=0;
			if ((*(&pCircuit_V) = get_double(parent,"Vdc"))==NULL)
			{
				GL_THROW("%s (%s:%d) does not implement inverter variable %s for %s (inverter:%d)", 
					/*	TROUBLESHOOT
					The rectifier requires that the inverter contains certain published properties in order to properly connect. If you encounter this error, please report it to the developers, along with
					the version of GridLAB-D that raised this error.
					*/
					parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, "Vdc", obj->name?obj->name:"unnamed", obj->id);
			}
			i=1;
			if ((*(&pLine_I) = get_complex(parent,"I_In"))==NULL)
			{
				GL_THROW("%s (%s:%d) does not implement inverter variable %s for %s (inverter:%d)", 
					/*	TROUBLESHOOT
					The rectifier requires that the inverter contains certain published properties in order to properly connect. If you encounter this error, please report it to the developers, along with
					the version of GridLAB-D that raised this error.
					*/
					parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, "I_In", obj->name?obj->name:"unnamed", obj->id);
			}
		
	}

	if (parent_string == NULL){
		GL_THROW("Rectifier's parent is not an inverter. The rectifier object's parent must be an inverter.");
	}

	/* TODO: set the context-dependent initial value of properties */
	if (gen_mode_v==UNKNOWN)
	{
		GL_THROW("Generator control mode is not specified.");
	}
	else if(gen_mode_v == CONSTANT_V)
	{
		GL_THROW("Generator mode CONSTANT_V is not implemented yet.");
	}
	else if(gen_mode_v == CONSTANT_PQ)
	{
		GL_THROW("Generator mode CONSTANT_PQ is not implemented yet.");
	}
	else if(gen_mode_v == CONSTANT_PF)
	{
		GL_THROW("Generator mode CONSTANT_PF is not implemented yet.");
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
	double Real_Xphase = Xphase * ZB;
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
	switch(rectifier_type_v){
		case ONE_PULSE:
			efficiency = 0.5;
			break;
		case TWO_PULSE:
			efficiency = 0.7;
			break;
		case THREE_PULSE:
			efficiency = 0.7;
			break;
		case SIX_PULSE:
			efficiency = 0.8;
			break;
		case TWELVE_PULSE:
			efficiency = 0.9;
			break;
		default:
			efficiency = 0.8;
			break;
	}

	internal_switch_resistance(switch_type_choice);
	filter_circuit_impact((power_electronics::FILTER_TYPE)filter_type_v, (power_electronics::FILTER_IMPLEMENTATION)filter_imp_v);

	gl_verbose("rectifier init: about to exit");
	return 1;
}


complex *rectifier::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}
double *rectifier::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}



void rectifier::iterative_IV(complex VA, char* phase_designation){
	/*complex Vdet = complex(0,0);
	complex increment = complex(0,0);
	complex Idet = complex(0,0);
	complex Vs = complex(0,0);
	complex s = complex(0,0);
	complex incrementA;
	complex incrementB;
	complex incrementC;
	complex VdetA = V_In_Set_A;
	complex VdetB = V_In_Set_B;
	complex VdetC = V_In_Set_C;
	complex IdetA = complex(0,0);
	complex IdetB = complex(0,0);
	complex IdetC = complex(0,0);
	int i = 0;

	if(VA.Re() > 3000){
	incrementA = complex(1 * 1,0);
	incrementB = complex(1 * -0.5, 1 * 0.866);
	incrementC = complex(1 * -0.5, 1 * -0.866);
	}else if (VA.Re() > 1000){
	incrementA = complex(0.5 * 1,0);
	incrementB = complex(0.5 * -0.5, 0.5 * 0.866);
	incrementC = complex(0.5 * -0.5, 0.5 * -0.866);
	}else{
	incrementA = complex(0.25 * 1,0);
	incrementB = complex(0.25 * -0.5, 0.25 * 0.866);
	incrementC = complex(0.25 * -0.5, 0.25 * -0.866);
	}


	if(phase_designation == "D"){
	while(true){
	if((abs(VA.Re() - s.Re()) < margin)){
	if(true){//abs(VA.Im() - s.Im()) < margin){
	break;
	}
	}

	if(i > 150){
	throw("rectifier sync: rectifier failed to converge!  The power asked for was too high or too low!");
	break;
	}
	VdetA = VdetA + incrementA;
	VdetB = VdetB + incrementB;
	VdetC = VdetC + incrementC;
	IdetA = (VdetA - V_In_Set_A)/XphaseA;
	IdetB = (VdetB - V_In_Set_B)/XphaseB;
	IdetC = (VdetC - V_In_Set_C)/XphaseC;
	s = ((~IdetA) * VdetA) + ((~IdetB) * VdetB) + ((~IdetC) * VdetC);
	i++;
	}

	voltage_A = VdetA;
	current_A = IdetA;
	power_A_In = (~IdetA) * VdetA;
	voltage_B = VdetB;
	current_B = IdetB;
	power_B_In = (~IdetB) * VdetB;
	voltage_C = VdetC;
	current_C = IdetC;
	power_C_In = (~IdetC) * VdetC;


	gl_verbose("rectifier sync: iterative solver: Total VA delivered is: (%f , %f)", s.Re(), s.Im());

	gl_verbose("rectifier sync: iterative solver: power_A_In delivered is: (%f , %f)", power_A_In.Re(), power_A_In.Im());
	gl_verbose("rectifier sync: iterative solver: voltage_A is: (%f , %f)", voltage_A.Re(), voltage_A.Im());
	gl_verbose("rectifier sync: iterative solver: current_A is: (%f , %f)", current_A.Re(), current_A.Im());

	gl_verbose("rectifier sync: iterative solver: power_B_In delivered is: (%f , %f)", power_B_In.Re(), power_B_In.Im());
	gl_verbose("rectifier sync: iterative solver: voltage_B is: (%f , %f)", voltage_B.Re(), voltage_B.Im());
	gl_verbose("rectifier sync: iterative solver: current_B is: (%f , %f)", current_B.Re(), current_B.Im());

	gl_verbose("rectifier sync: iterative solver: power_C_In delivered is: (%f , %f)", power_C_In.Re(), power_C_In.Im());
	gl_verbose("rectifier sync: iterative solver: voltage_C is: (%f , %f)", voltage_C.Re(), voltage_C.Im());
	gl_verbose("rectifier sync: iterative solver: current_C is: (%f , %f)", current_C.Re(), current_C.Im());

	return;
	}

	complex Xphase = complex(0,0);


	if (phase_designation == "A"){
	Vdet = V_In_Set_A;
	Vs = V_In_Set_A;
	increment = complex(0.5 * 1,0);
	Xphase = XphaseA;
	}
	else if(phase_designation == "B"){
	Vdet = V_In_Set_B;
	Vs = V_In_Set_B;
	increment = complex(0.5 * -0.5, 0.5 * 0.866);
	Xphase = XphaseB;
	}
	else if(phase_designation == "C"){
	Vdet = V_In_Set_C;
	Vs = V_In_Set_C;
	increment = complex(0.5 * -0.5, 0.5 * -0.866);
	Xphase = XphaseC;
	}else{
	throw("no phase designated!");
	}

	Idet = (Vdet - Vs)/Xphase;
	s = (~Idet) * Vdet;
	if(s.Mag() >= VA.Mag()){
	if(phase_designation == "A"){
	voltage_A = Vdet;
	current_A = Idet;
	}
	else if(phase_designation == "B"){
	voltage_B = Vdet;
	current_B = Idet;
	}
	else if(phase_designation == "C"){
	voltage_C = Vdet;
	current_C = Idet;
	}
	return;
	}


	while(((abs(VA.Re() - s.Re()) > margin) || (abs(VA.Im() - s.Im()) > margin))&& i < 50){
	Vdet = Vdet + increment; 
	Idet = (Vdet - Vs)/Xphase;
	s = Vdet * ~ Idet;
	i++;
	}

	if(phase_designation == "A"){
	voltage_A = Vdet;
	current_A = Idet;
	gl_verbose("rectifier sync: iterative solver: power_A_In asked for was: (%f , %f)", VA.Re(), VA.Im());
	gl_verbose("rectifier sync: iterative solver: power_A_In delivered is: (%f , %f)", s.Re(), s.Im());
	gl_verbose("rectifier sync: iterative solver: voltage_A is: (%f , %f)", voltage_A.Re(), voltage_A.Im());
	gl_verbose("rectifier sync: iterative solver: current_A is: (%f , %f)", current_A.Re(), current_A.Im());

	}
	else if(phase_designation == "B"){
	voltage_B = Vdet;
	current_B = Idet;
	gl_verbose("rectifier sync: iterative solver: power_B_In asked for was: (%f , %f)", VA.Re(), VA.Im());
	gl_verbose("rectifier sync: iterative solver: power_B_In delivered is: (%f , %f)", s.Re(), s.Im());
	gl_verbose("rectifier sync: iterative solver: voltage_B is: (%f , %f)", voltage_B.Re(), voltage_B.Im());
	gl_verbose("rectifier sync: iterative solver: current_B is: (%f , %f)", current_B.Re(), current_B.Im());
	}
	else if(phase_designation == "C"){
	voltage_C = Vdet;
	current_C = Idet;
	gl_verbose("rectifier sync: iterative solver: power_C_In asked for was: (%f , %f)", VA.Re(), VA.Im());
	gl_verbose("rectifier sync: iterative solver: power_C_In delivered is: (%f , %f)", s.Re(), s.Im());
	gl_verbose("rectifier sync: iterative solver: voltage_C is: (%f , %f)", voltage_C.Re(), voltage_C.Im());
	gl_verbose("rectifier sync: iterative solver: current_C is: (%f , %f)", current_C.Re(), current_C.Im());
	}


	return;
	*/
}



/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP rectifier::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	I_Out = VA_Out = 0.0;

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP rectifier::sync(TIMESTAMP t0, TIMESTAMP t1) 
{

	V_Out = *pCircuit_V;
	I_Out = (*pLine_I).Re();

	gl_verbose("rectifier sync: V_Out from parent is: (%f , %f)", V_Out.Re(), V_Out.Im());
	gl_verbose("rectifier sync: I_Out from parent is: (%f , %f)", I_Out.Re(), I_Out.Im());

	internal_losses = 1 - calculate_loss(Rtotal, Ltotal, Ctotal, DC, AC);
	frequency_losses = 1 - calculate_frequency_loss(input_frequency, Rtotal,Ltotal, Ctotal);

	//TODO: consider installing duty or on-ratio limits
	//AC-DC voltage magnitude ratio rule
	double VInMag = V_Out.Mag() * PI / (3 * sqrt(6.0));
	voltage_A = complex(VInMag,0);
	voltage_B = complex(-VInMag/2, VInMag * sqrt(3.0) / 2);
	voltage_C = complex(-VInMag/2, -VInMag * sqrt(3.0) / 2);


	switch(gen_mode_v){
		case SUPPLY_DRIVEN:
			{

				complex S_A_In, S_B_In, S_C_In;

				//DC Voltage, controlled by parent object determines DC Voltage.
				S_A_In = voltage_A*(~(current_A));
				S_B_In = voltage_B*(~(current_B));
				S_C_In = voltage_C*(~(current_C));

				power_A_In = S_A_In.Re();
				power_B_In = S_B_In.Re();
				power_C_In = S_C_In.Re();
				/*
				power_A_In = voltage_A.Mag() * current_A.Mag(); //AC
				power_B_In = voltage_B.Mag() * current_B.Mag();
				power_C_In = voltage_C.Mag() * current_C.Mag();
				*/
				VA_In = power_A_In + power_B_In + power_C_In;



				VA_Out = VA_In * efficiency * internal_losses * frequency_losses;
				losses = VA_Out * Rtotal / (Rtotal + Rload);

				I_Out = ~(VA_Out / V_Out); //These values are completely real, but since parent object uses complex, use here as well and follow rule for complex conjugate
				*pLine_I=I_Out;

				gl_verbose("rectifier sync: VA_Out is: (%f , %f)", VA_Out.Re(), VA_Out.Im());
				gl_verbose("rectifier sync: V_Out is: (%f , %f)", V_Out.Re(), V_Out.Im());
				gl_verbose("rectifier sync: I_Out is: (%f , %f)", I_Out.Re(), I_Out.Im());
				gl_verbose("rectifier sync: losses is: (%f , %f)", losses.Re(), losses.Im());


				gl_verbose("rectifier sync: supply driven about to exit");
				return TS_NEVER;
			}
			break;
			/*
			case CONSTANT_PQ:
			{//TODO
			gl_verbose("rectifier sync: constant pq rectifier");
			//P_Out is either set or input from elsewhere
			//Q_Out is either set or input from elsewhere

			//if the device is connected directly to the meter, then it is the lead node and must set the
			//power output for the rest of the branches
			if(parent_string == "meter"){
			VA_Out = complex(P_Out,Q_Out);
			}else{
			VA_Out = V_Out * (~I_Out);
			}

			gl_verbose("rectifier sync: VA_Out calculated from parent is: (%f , %f)", VA_Out.Re(), VA_Out.Im());

			VA_In = VA_Out / (efficiency * internal_losses);
			losses = VA_Out * (1 - (efficiency * internal_losses));

			//I_In = VA_In / V_In;

			gl_verbose("rectifier sync: VA_In calculated after losses is: (%f , %f)", VA_In.Re(), VA_In.Im());

			if(number_of_phases_in == 3){
			iterative_IV(VA_In, "D");
			}

			if(number_of_phases_in == 1){
			//divide up power by the number of phases, then assign that power to each phase
			VA_In = VA_In / number_of_phases_in;

			//assume balanced system:


			//TODO: I'm not sure if the sqrt(2) is necessary... it is supposed to account for RMS current but 
			//it may already be in RMS current after dividing by voltage
			if(phaseAIn){
			power_A_In = VA_In;
			iterative_IV(power_A_In,"A");

			//	voltage_A = ((power_A_In * Xphase) + V_In_Set_A)^0.5;
			//	current_A = (power_A_In / voltage_A); 
			//	
			}

			if(phaseBIn){
			power_B_In = VA_In;
			iterative_IV(power_B_In,"B");
			//		voltage_B = ((power_B_In * Xphase) + V_In_Set_B)^0.5;
			//		current_B = (power_B_In / voltage_B);
			//	
			}

			if(phaseCIn){
			power_C_In = VA_In;
			iterative_IV(power_C_In,"C");

			//	voltage_C = ((power_C_In * Xphase) + V_In_Set_C)^0.5;
			//	current_C = (power_C_In / voltage_C);
			//	
			}

			}


			gl_verbose("rectifier sync: VA_In is: (%f , %f)", VA_In.Re(), VA_In.Im());
			gl_verbose("rectifier sync: power_A_In is: (%f , %f)", power_A_In.Re(), power_A_In.Im());
			gl_verbose("rectifier sync: power_B_In is: (%f , %f)", power_B_In.Re(), power_B_In.Im());
			gl_verbose("rectifier sync: power_C_In is: (%f , %f)", power_C_In.Re(), power_C_In.Im());
			gl_verbose("rectifier sync: voltage_A is: (%f , %f)", voltage_A.Re(), voltage_A.Im());
			gl_verbose("rectifier sync: voltage_B is: (%f , %f)", voltage_B.Re(), voltage_B.Im());
			gl_verbose("rectifier sync: voltage_C is: (%f , %f)", voltage_C.Re(), voltage_C.Im());
			gl_verbose("rectifier sync: current_A is: (%f , %f)", current_A.Re(), current_A.Im());
			gl_verbose("rectifier sync: current_B is: (%f , %f)", current_B.Re(), current_B.Im());
			gl_verbose("rectifier sync: current_C is: (%f , %f)", current_C.Re(), current_C.Im());

			gl_verbose("rectifier sync: constant pq about to exit");
			return TS_NEVER;
			}
			break;
			case CONSTANT_V:
			{
			gl_verbose("rectifier sync: constant v rectifier");
			bool changed = false;


			//voltage_A = V_Out;
			//voltage_B = V_Out;
			//voltage_C = V_Out;


			//TODO
			//Gather V_Out
			//Gather VA_Out
			//Gather Rload


			if (V_Out < (V_Rated - margin)){
			I_Out = I_out_prev + I_step_max / 2;
			changed = true;
			}else if (V_Out > (V_Rated + margin)){
			I_Out = I_out_prev - I_step_max / 2;
			changed = true;
			}else{
			changed = false;
			}

			VA_Out = (~I_Out) * V_Out;

			if(VA_Out > Rated_kVA){
			VA_Out = Rated_kVA;
			I_Out = VA_Out / V_Out;
			changed = false;
			}

			if(VA_Out < 0){
			VA_Out = 0;
			I_Out = 0;
			changed = false;
			}





			if(phaseAOut){
			if (phaseA_V_Out < (V_Rated_A - margin)){
			phaseA_I_Out = phaseA_I_Out_prev + I_step_max/2;
			changed = true;
			}else if (phaseA_V_Out > (V_Rated_A + margin)){
			phaseA_I_Out = phaseA_I_Out_prev - I_step_max/2;
			changed = true;
			}else{changed = false;}
			}if (phaseBOut){
			if (phaseB_V_Out < (V_Rated_B - margin)){
			phaseB_I_Out = phaseB_I_Out_prev + I_step_max/2;
			changed = true;
			}else if (phaseB_V_Out > (V_Rated_B + margin)){
			phaseB_I_Out = phaseB_I_Out_prev - I_step_max/2;
			changed = true;
			}else{changed = false;}
			}if (phaseCOut){
			if (phaseC_V_Out < (V_Rated_C - margin)){
			phaseC_I_Out = phaseC_I_Out_prev + I_step_max/2;
			changed = true;
			}else if (phaseC_V_Out > (V_Rated_C + margin)){
			phaseC_I_Out = phaseC_I_Out_prev - I_step_max/2;
			changed = true;
			}else{changed = false;}
			}

			power_A = phaseA_I_Out * phaseA_V_Out;
			power_B = phaseB_I_Out * phaseB_V_Out;
			power_C = phaseC_I_Out * phaseC_V_Out;

			//check if rectifier is overloaded -- if so, cap at max power
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



			gl_verbose("rectifier sync: VA_Out requested from parent is: (%f , %f)", VA_Out.Re(), VA_Out.Im());


			VA_In = VA_Out / (efficiency * internal_losses);
			losses = VA_Out * (1 - (efficiency * internal_losses));

			gl_verbose("rectifier sync: VA_In after losses is: (%f , %f)", VA_In.Re(), VA_In.Im());

			//after we export this current to the generator that is hooked up on the input, 
			//we will expect that during the next timestep, the generator will give us a
			//new frequency of input power that it has adopted in order to meet the 
			//requested current

			if(number_of_phases_in == 3){
			iterative_IV(VA_In, "D");
			}

			if(number_of_phases_in == 1){
			VA_In = VA_In / number_of_phases_in;

			if(phaseAIn){
			power_A_In = VA_In;
			iterative_IV(power_A_In,"A");
			}

			if(phaseBIn){
			power_B_In = VA_In;
			iterative_IV(power_B_In,"B");
			}

			if(phaseCIn){
			power_C_In = VA_In;
			iterative_IV(power_C_In,"C");
			}

			}
			//TODO: check P and Q components to see if within bounds


			gl_verbose("rectifier sync: VA_In is: (%f , %f)", VA_In.Re(), VA_In.Im());
			gl_verbose("rectifier sync: power_A_In is: (%f , %f)", power_A_In.Re(), power_A_In.Im());
			gl_verbose("rectifier sync: power_B_In is: (%f , %f)", power_B_In.Re(), power_B_In.Im());
			gl_verbose("rectifier sync: power_C_In is: (%f , %f)", power_C_In.Re(), power_C_In.Im());
			gl_verbose("rectifier sync: voltage_A is: (%f , %f)", voltage_A.Re(), voltage_A.Im());
			gl_verbose("rectifier sync: voltage_B is: (%f , %f)", voltage_B.Re(), voltage_B.Im());
			gl_verbose("rectifier sync: voltage_C is: (%f , %f)", voltage_C.Re(), voltage_C.Im());
			gl_verbose("rectifier sync: current_A is: (%f , %f)", current_A.Re(), current_A.Im());
			gl_verbose("rectifier sync: current_B is: (%f , %f)", current_B.Re(), current_B.Im());
			gl_verbose("rectifier sync: current_C is: (%f , %f)", current_C.Re(), current_C.Im());


			gl_verbose("rectifier sync: constant v rectifier about to exit");
			if(changed){
			TIMESTAMP t2 = TS_NEVER;
			return t2;
			}else{
			return TS_NEVER;
			}
			}
			break;*/
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
// //       PA = Re(voltage_A * current_A); 
////		QA = Im(voltage_A * current_A); 
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
TIMESTAMP rectifier::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	///* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_rectifier(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(rectifier::oclass);
		if (*obj!=NULL)
		{
			rectifier *my = OBJECTDATA(*obj,rectifier);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(rectifier);
}

EXPORT int init_rectifier(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,rectifier)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(rectifier);
}

EXPORT TIMESTAMP sync_rectifier(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	rectifier *my = OBJECTDATA(obj,rectifier);
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
	SYNC_CATCHALL(rectifier);
	return t2;
}
