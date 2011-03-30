/** $Id: diesel_dg.cpp,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file diesel_dg.cpp
	@defgroup diesel_dg Diesel gensets
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
//#include "C:\_FY08\Projects-SVN\source\residential\solvers.h"

#include "diesel_dg.h"
#include "lock.h"

CLASS *diesel_dg::oclass = NULL;
diesel_dg *diesel_dg::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
diesel_dg::diesel_dg(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"diesel_dg",sizeof(diesel_dg),passconfig);
		if (oclass==NULL)
			throw "unable to register class diesel_dg";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,

			PT_enumeration,"Gen_mode",PADDR(Gen_mode),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"CONSTANTE",CONSTANTE,
				PT_KEYWORD,"CONSTANTPQ",CONSTANTPQ,
				PT_KEYWORD,"CONSTANTP",CONSTANTP,

			PT_enumeration,"Gen_status",PADDR(Gen_status),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"OFFLINE",OFFLINE,
				PT_KEYWORD,"ONLINE",ONLINE,	

			PT_enumeration,"Gen_type",PADDR(Gen_type),
				PT_KEYWORD,"INDUCTION",INDUCTION,
				PT_KEYWORD,"SYNCHRONOUS",SYNCHRONOUS,

			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
			
			PT_double, "pf", PADDR(pf),

			PT_double, "GenElecEff", PADDR(GenElecEff),
			PT_double, "TotalRealPow[W]", PADDR(TotalRealPow),
			PT_double, "TotalReacPow[VAr]", PADDR(TotalReacPow),

			// Diesel engine power plant inputs
			PT_double, "speed[1/min]", PADDR(speed),// speed of an engine
			PT_double, "cylinders", PADDR(cylinders),//Total number of cylinders in a diesel engine
			PT_double, "stroke", PADDR(stroke),//category of internal combustion engines
			PT_double, "torque[N]", PADDR(torque),//Net brake load
			PT_double, "pressure[N/m^2]", PADDR(pressure),
			PT_double, "time_operation[min]", PADDR(time_operation),
			PT_double, "fuel[kg]", PADDR(fuel),//fuel consumption
			PT_double, "w_coolingwater[kg]", PADDR(w_coolingwater),//weight of cooling water supplied per minute
			PT_double, "inlet_temperature[degC]", PADDR(inlet_temperature),//Inlet temperature of cooling water in degC
			PT_double, "outlet_temperature[degC]", PADDR(outlet_temperature),//outlet temperature of cooling water in degC
			PT_double, "air_fuel[kg]", PADDR(air_fuel),//Air used per kg fuel
			PT_double, "room_temperature[degC]", PADDR(room_temperature),//Room temperature in degC
			PT_double, "exhaust_temperature[degC]", PADDR(exhaust_temperature),//exhaust gas temperature in degC
			PT_double, "cylinder_length[m]", PADDR(cylinder_length),//
			PT_double, "cylinder_radius[m]", PADDR(cylinder_radius),//
			PT_double, "brake_diameter[m]", PADDR(brake_diameter),//
			PT_double, "calotific_fuel[kJ/kg]", PADDR(calotific_fuel),//calorific value of fuel
			PT_double, "steam_exhaust[kg]", PADDR(steam_exhaust),//steam formed per kg of fuel in the exhaust
			PT_double, "specific_heat_steam[kJ/kg/K]", PADDR(specific_heat_steam),//specific heat of steam in exhaust
			PT_double, "specific_heat_dry[kJ/kg/K]", PADDR(specific_heat_dry),//specific heat of dry exhaust gases

			PT_double, "indicated_hp[W]", PADDR(indicated_hp),//Indicated horse power is the power developed inside the cylinder
			PT_double, "brake_hp[W]", PADDR(brake_hp),//brake horse power is the output of the engine at the shaft measured by a dynamometer
			PT_double, "thermal_efficiency", PADDR(thermal_efficiency),//thermal efficiency or mechanical efiiciency of the engine is efined as bp/ip
			PT_double, "energy_supplied[kJ]", PADDR(energy_supplied),//energy supplied during the trail
			PT_double, "heat_equivalent_ip[kJ]", PADDR(heat_equivalent_ip),//heat equivalent of IP in a given time of operation
			PT_double, "energy_coolingwater[kJ]", PADDR(energy_coolingwater),//energy carried away by cooling water
			PT_double, "mass_exhaustgas[kg]", PADDR(mass_exhaustgas),//mass of dry exhaust gas
			PT_double, "energy_exhaustgas[kJ]", PADDR(energy_exhaustgas),//energy carried away by dry exhaust gases
			PT_double, "energy_steam[kJ]", PADDR(energy_steam),//energy carried away by steam
			PT_double, "total_energy_exhaustgas[kJ]", PADDR(total_energy_exhaustgas),//total energy carried away by dry exhaust gases is the sum of energy carried away bt steam and energy carried away by dry exhaust gases
			PT_double, "unaccounted_energyloss[kJ]", PADDR(unaccounted_energyloss),//unaccounted for energy loss

			//end of diesel engine inputs

			//Synchronous generator inputs
             
			PT_double, "f[Hz]", PADDR(f),//system frquency
			PT_double, "poles", PADDR(poles),//Number of poles in the synchronous generator
			PT_double, "wm[rad/s]", PADDR(wm),//angular velocity in rad/sec
			PT_double, "Pconv[kW]", PADDR(Pconv),//Converted power = Mechanical input - (F & W loasses + Stray losses + Core losses)
			PT_double, "Tind[kW]", PADDR(Tind),//Induced torque
			PT_double, "EA[V]", PADDR(EA),//Internal generated voltage produced in one phase of the synchronous generator
			PT_double, "Vo[V]", PADDR(EA),//Terminal voltage of the synchronous generator
			PT_double, "Rs1[Ohm]", PADDR(Rs1),//per phase resistance of the synchronous generator
			PT_double, "Xs1[Ohm]", PADDR(Xs1),//per phase synchronous reactance of the synchronous generator
			PT_double, "delta[deg]", PADDR(delta),//delta is the angle between EA and Vo. It is called torque angle
			PT_double, "IA[A]", PADDR(IA),//Armature current
            PT_double, "Ploss[kW]", PADDR(Ploss),//Copper losses
			PT_double, "Pout[kW]", PADDR(Pout),//The real electric output power of the synchronous generator
			PT_double, "effe[%]", PADDR(effe),//electrical efficiency
			PT_double, "effo[%]", PADDR(effo),//Overall efficiency


			//End of synchronous generator inputs
			PT_double, "Rated_V[V]", PADDR(Rated_V),
			PT_double, "Rated_VA[VA]", PADDR(Rated_VA),

			PT_double, "Rs", PADDR(Rs),
			PT_double, "Xs", PADDR(Xs),
			PT_double, "Rg", PADDR(Rg),
			PT_double, "Xg", PADDR(Xg),
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

			//PT_double, "Pconv[W]", PADDR(Pconv),

			PT_int16,"SlackBus",PADDR(SlackBus),
			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;

		memset(this,0,sizeof(diesel_dg));
		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int diesel_dg::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

////Initialize tracking variables

			Rated_kVA = 0.0;
			Rated_kV = 0.0;
			
			pf = 0.0;

			GenElecEff = 0.0;
			TotalRealPow = 0.0;
			TotalReacPow = 0.0;
			
			// Diesel engine power plant inputs
			speed = 0.0;
			cylinders = 0.0;
			stroke = 0.0;
			torque = 0.0;
			pressure =0.0;
			time_operation = 0.0;
			fuel = 0.0;
			w_coolingwater = 0.0;
			inlet_temperature = 0.0;
			outlet_temperature = 0.0;
			air_fuel = 0.0;
			room_temperature = 0.0;
			exhaust_temperature = 0.0;
			cylinder_length = 0.0;
			cylinder_radius = 0.0;
			brake_diameter = 0.0;
			calotific_fuel = 0.0;
			steam_exhaust = 0.0;
			specific_heat_steam = 0.0;
			specific_heat_dry = 0.0;

			indicated_hp = 0.0;
			brake_hp = 0.0;
			thermal_efficiency = 0.0;
			energy_supplied = 0.0;

			heat_equivalent_ip = 0.0;
			energy_coolingwater = 0.0;
			mass_exhaustgas = 0.0;
			energy_exhaustgas = 0.0;

			energy_steam = 0.0;
			total_energy_exhaustgas = 0.0;
			unaccounted_energyloss = 0.0;

			//end of diesel engine inputs

			//Synchronous generator inputs
             
			f = 60;
			poles = 2;
			wm = 2*3.14*f;
			Pconv = 0.0;
			Tind = 0.0;
			EA = 0.0;

			Vo = 0.0;
			Rs1 = 0.0;
			Xs1 = 0.0;
			delta = 0.0;
			IA = 0.0;
            Ploss = 0.0;
			Pout = 0.0;
			effe = 0.0;
			effo = 0.0;


			//End of synchronous generator inputs
			Rated_V = 480;
			Rated_VA = 12000;

				Rs = 0.025;				//Estimated values for synch representation.
				Xs = 0.200;
				Rg = 0.000;
				Xg = 0.000;
			voltage_A = 0.0;
			voltage_B = 0.0;
			voltage_C = 0.0;
			current_A = 0.0;
			current_B = 0.0;
			current_C = 0.0;
			EfA = 0.0;
			EfB = 0.0;
			EfC = 0.0;
			power_A = 0.0;
			power_B = 0.0;
			power_C = 0.0;
			power_A_sch = 0.0;
			power_B_sch = 0.0;
			power_C_sch = 0.0;
			EfA_sch = 0.0;
			EfB_sch = 0.0;
			EfC_sch = 0.0;
			Max_P = 11000;
			Max_Q = 6633.25;



	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int diesel_dg::init(OBJECT *parent)
{

		double ZB, SB, EB;
	complex tst, tst2, tst3, tst4;
		 current_A = current_B = current_C = 0.0;

	// construct circuit variable map to meter -- copied from 'House' module
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
		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);
	}
	else
	{
		OBJECT *obj = OBJECTHDR(this);
		gl_warning("house:%d %s", obj->id, parent==NULL?"has no parent meter defined":"parent is not a meter");

		// attach meter variables to each circuit in the default_meter
			*(map[0].var) = &default_line123_voltage[0];
			*(map[1].var) = &default_line1_current[0];

		// provide initial values for voltages
			default_line123_voltage[0] = complex(Rated_V/sqrt(3.0),0);
			default_line123_voltage[1] = complex(Rated_V/sqrt(3.0)*cos(2*PI/3),Rated_V/sqrt(3.0)*sin(2*PI/3));
			default_line123_voltage[2] = complex(Rated_V/sqrt(3.0)*cos(-2*PI/3),Rated_V/sqrt(3.0)*sin(-2*PI/3));

	}

	if (Gen_mode==UNKNOWN)
	{
		OBJECT *obj = OBJECTHDR(this);
		throw("Generator control mode is not specified");
	}

	if (Gen_status==0)
	{
		throw("Generator is out of service!"); 	}
	
	else
	{	//TO DO: what to do if not defined...break out?
		if (Rated_VA!=0.0)  SB = Rated_VA/3;
		if (Rated_V!=0.0)  EB = Rated_V/sqrt(3.0);
		if (SB!=0.0)  ZB = EB*EB/SB;
		else throw("Generator power capacity not specified!");
	}
		

	if (Gen_type == SYNCHRONOUS)  
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

//	init_climate();

return 1;
}//init ends here

/* Presync is called when the clock needs to advance on the first top-down pass */
/*TIMESTAMP diesel_dg::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	current_A = current_B = current_C = 0.0;

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	//return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
//}**/

/* Sync is called when the clock needs to advance on the bottom-up pass */


// Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP diesel_dg::presync(TIMESTAMP t0, TIMESTAMP t1)

{
	   return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP diesel_dg::sync(TIMESTAMP t0, TIMESTAMP t1) 
{

		double Pmech,

		indicated_hp = (100000/60)*(pressure*cylinder_length*(3.1416/4)*cylinder_radius*cylinder_radius*speed*cylinders);
		
		brake_hp = 2*3.1416*(speed/stroke)*(torque/cylinders)/60;

		thermal_efficiency = (brake_hp/indicated_hp)*100;

		energy_supplied = fuel * calotific_fuel;

		heat_equivalent_ip = indicated_hp * time_operation * 60/1000;

		energy_coolingwater = w_coolingwater * 4.187 * (outlet_temperature - inlet_temperature);

		mass_exhaustgas = (fuel * air_fuel) - (steam_exhaust * fuel);

		energy_exhaustgas = mass_exhaustgas * specific_heat_dry * (exhaust_temperature - room_temperature);

		energy_steam = (steam_exhaust * fuel) * (4.187 * (100 - room_temperature) + 2257.9 + specific_heat_steam * (exhaust_temperature - 100));

		total_energy_exhaustgas = energy_exhaustgas + energy_steam;

		unaccounted_energyloss = energy_supplied - heat_equivalent_ip - energy_coolingwater - total_energy_exhaustgas;	

		Pmech = brake_hp;

		Pconv = 1 * Pmech;  //TO DO:  friction and windage loss, misc. loss model

		//current_A = current_B = current_C = 0.0;

	int k;
	voltage_A = pCircuit_V[0];	//Syncs the meter parent to the generator.
	voltage_B = pCircuit_V[1];
	voltage_C = pCircuit_V[2];

	double Pconva = (voltage_A.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;
	double Pconvb = (voltage_B.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;
	double Pconvc = (voltage_C.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;

	if (Gen_type == SYNCHRONOUS)	
	{											//sg ef mode is not working yet
		double Mxef, Mnef, PoutA, PoutB, PoutC, QoutA, QoutB, QoutC;
		complex SoutA, SoutB, SoutC;
		complex lossesA, lossesB, lossesC;

		Mxef = Max_Ef * Rated_V/sqrt(3.0);
		Mnef = Min_Ef * Rated_V/sqrt(3.0);

		
		if (Gen_mode == CONSTANTE)	//Ef is controllable to give a needed power output.
		{
			current_A = invAMx[0][0]*(voltage_A - EfA) + invAMx[0][1]*(voltage_B - EfB) + invAMx[0][2]*(voltage_C - EfC);
			current_B = invAMx[1][0]*(voltage_A - EfA) + invAMx[1][1]*(voltage_B - EfB) + invAMx[1][2]*(voltage_C - EfC);
			current_C = invAMx[2][0]*(voltage_A - EfA) + invAMx[2][1]*(voltage_B - EfB) + invAMx[2][2]*(voltage_C - EfC);

			SoutA = -voltage_A * (~(current_A));  //TO DO:  unbalanced
			SoutB = -voltage_B * (~(current_B));
			SoutC = -voltage_C * (~(current_C));

			
		}

		else if (Gen_mode == CONSTANTPQ)	//Gives a constant output power of real power converted Pout,  
		{									//then Qout is found through a controllable power factor.
			if (Pconva > 1*Max_P/3) {
				Pconva = 1*Max_P/3;		//If air density increases, power extracted can be much greater
			}								//than amount the generator can handle.  This limits to 5% overpower.
			if (Pconvb > 1*Max_P/3) {
				Pconvb = 1*Max_P/3;
			}
			if (Pconvc > 1*Max_P/3) {
				Pconvc = 1*Max_P/3;
			}
			
			current_A = -(~(complex(Pconva,(Pconva/pf)*sin(acos(pf)))/voltage_A));
			current_B = -(~(complex(Pconvb,(Pconvb/pf)*sin(acos(pf)))/voltage_B));
			current_C = -(~(complex(Pconvc,(Pconvc/pf)*sin(acos(pf)))/voltage_C));

			for (k = 0; k < 100; k++)
			{
				PoutA = Pconva - current_A.Mag()*current_A.Mag()*(AMx[0][0] - AMx[0][1]).Re();
				PoutB = Pconvb - current_B.Mag()*current_B.Mag()*(AMx[1][1] - AMx[0][1]).Re();
				PoutC = Pconvc - current_C.Mag()*current_C.Mag()*(AMx[2][2] - AMx[0][1]).Re();

				QoutA = pf/abs(pf)*PoutA*sin(acos(pf));
				QoutB = pf/abs(pf)*PoutB*sin(acos(pf));
				QoutC = pf/abs(pf)*PoutC*sin(acos(pf));

				current_A = -(~(complex(PoutA,QoutA)/voltage_A));
				current_B = -(~(complex(PoutB,QoutB)/voltage_B));
				current_C = -(~(complex(PoutC,QoutC)/voltage_C));
			}

			EfA = voltage_A - (AMx[0][0] - AMx[0][1])*current_A - AMx[0][2]*(current_A + current_B + current_C);
			EfB = voltage_B - (AMx[1][1] - AMx[1][0])*current_A - AMx[1][2]*(current_A + current_B + current_C);
			EfC = voltage_C - (AMx[2][2] - AMx[2][0])*current_A - AMx[2][1]*(current_A + current_B + current_C);

			//if (EfA.Mag() > Mxef || EfA.Mag() > Mxef || EfA.Mag() > Mxef)
			//{
			//	Gen_mode = CONSTANTEf;
			//}TO DO:  loop back to Ef if true?
		}
	}//Synchronous generator ends here
	
	//test functions

	double PowerA, PowerB, PowerC, QA, QB, QC;

	PowerA = -voltage_A.Mag()*current_A.Mag()*cos(voltage_A.Arg() - current_A.Arg());
	PowerB = -voltage_B.Mag()*current_B.Mag()*cos(voltage_B.Arg() - current_B.Arg());
	PowerC = -voltage_C.Mag()*current_C.Mag()*cos(voltage_C.Arg() - current_C.Arg());

	QA = -voltage_A.Mag()*current_A.Mag()*sin(voltage_A.Arg() - current_A.Arg());
	QB = -voltage_B.Mag()*current_B.Mag()*sin(voltage_B.Arg() - current_B.Arg());
	QC = -voltage_C.Mag()*current_C.Mag()*sin(voltage_C.Arg() - current_C.Arg());


	TotalRealPow = PowerA + PowerB + PowerC;
	TotalReacPow = QA + QB + QC;

	GenElecEff = TotalRealPow/Pconv * 100;

//	Wind_Speed = WSadj;
	
	pLine_I[0] = current_A;
	pLine_I[1] = current_B;
	pLine_I[2] = current_C;
	
	TIMESTAMP t2 = TS_NEVER;
	return t2;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP diesel_dg::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	//TIMESTAMP t2 = t1 + time_advance;
	TIMESTAMP t2 = TS_NEVER;

	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

complex *diesel_dg::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_diesel_dg(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(diesel_dg::oclass);
		if (*obj!=NULL)
		{
			diesel_dg *my = OBJECTDATA(*obj,diesel_dg);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	} 
	CREATE_CATCHALL(diesel_dg);
}

EXPORT int init_diesel_dg(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,diesel_dg)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(diesel_dg);
}

EXPORT TIMESTAMP sync_diesel_dg(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = TS_NEVER;
	diesel_dg *my = OBJECTDATA(obj,diesel_dg);
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
		if (pass==clockpass)
			obj->clock = t1;		
	}
	SYNC_CATCHALL(diesel_dg);
	return t1;
}
