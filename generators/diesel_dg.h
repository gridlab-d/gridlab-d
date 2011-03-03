/** $Id: diesel_dg.h,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file diesel_dg.h
	@addtogroup diesel_dg

 @{  
 **/

#ifndef _diesel_dg_H
#define _diesel_dg_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
#include "../powerflow/node.h"
#include "gridlabd.h"

class diesel_dg
{
private:
	/* TODO: put private variables here */
	
	complex *pCircuit_V; ///< pointer to the three voltages on three lines
	complex *pLine_I; ///< pointer to the three current on three lines

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */

	enum {OFFLINE=1, ONLINE} Gen_status;
	enum {INDUCTION=1, SYNCHRONOUS} Gen_type;
	enum {CONSTANTE=1, CONSTANTP, CONSTANTPQ} Gen_mode;

   // enum {CONSTANTE=1, CONSTANTPQ} Gen_mode;  //< meter type 
	//enum {OFFLINE=0, ONLINE=1} Gen_status;
//	double Rated_kW; //< nominal power in kW
//	double Rated_pf; //< power factor, P/(P^2+Q^2)^0.5
//	double Fuel_coefficient1;//< coefficient to calculate fuel cost
//	double Fuel_coefficient2;//< coefficient to calculate fuel cost
//	double Fuel_coefficient3;//< coefficient to calculate fuel cost

	//Diesel engine inputs

	complex AMx[3][3];			//Impedance matrix for Synchronous Generator
	complex invAMx[3][3];		//Inverse of SG impedance matrix

	double speed; // speed of an engine
	double cylinders;//Total number of cylinders in a diesel engine
	double stroke; //category of internal combustion engines
	double torque;//Net brake load
	double pressure;//Mean effective pressure
	double time_operation; 
	double fuel;//fuel consumption
	double w_coolingwater;//weight of cooling water supplied per minute
	double inlet_temperature; //Inlet temperature of cooling water in degC
	double outlet_temperature;//outlet temperature of cooling water in degC
	double air_fuel; //Air used per kg fuel
	double room_temperature; //Room temperature in degC
	double exhaust_temperature;//exhaust gas temperature in degC
	double cylinder_length;
	double cylinder_radius;//
	double brake_diameter;//
	double calotific_fuel;//calorific value of fuel
	double steam_exhaust;//steam formed per kg of fuel in the exhaust
	double specific_heat_steam;//specific heat of steam in exhaust
	double specific_heat_dry;//specific heat of dry exhaust gases
	double indicated_hp; //Indicated horse power is the power developed inside the cylinder
	double brake_hp; //brake horse power is the output of the engine at the shaft measured by a dynamometer.
	double thermal_efficiency;//thermal efficiency or mechanical efiiciency of the engine is efined as bp/ip
	double energy_supplied; //energy supplied during the trail
	double heat_equivalent_ip;//heat equivalent of IP in a given time of operation
	double energy_coolingwater;//energy carried away by cooling water
	double mass_exhaustgas; //mass of dry exhaust gas
	double energy_exhaustgas;//energy carried away by dry exhaust gases
	double energy_steam;//energy carried away by steam
	double total_energy_exhaustgas;//total energy carried away by dry exhaust gases is the sum of energy carried away bt steam and energy carried away by dry exhaust gases
	double unaccounted_energyloss;//unaccounted for energy loss

	double Rated_V;
	double Rated_VA;

	//end of diesel engine inputs
	
	//Synchronous gen inputs
	
	double f;//system frquency
	double poles;//Number of poles in the synchronous generator
	double wm;//angular velocity in rad/sec
	double Pconv;//Converted power = Mechanical input - (F & W loasses + Stray losses + Core losses)
	double Tind;//Induced torque
	double EA;//Internal generated voltage produced in one phase of the synchronous generator
	double Vo;//Terminal voltage of the synchronous generator
	double Rs1;//per phase resistance of the synchronous generator
	double Xs1;//per phase synchronous reactance of the synchronous generator
	double delta;//delta is the angle between EA and Vo. It is called torque angle
	double IA;//Armature current
    double Ploss;//Copper losses
	double Pout;//The real electric output power of the synchronous generator
	double effe;//electrical efficiency
	double effo;//Overall efficiency

	double pf;

	double GenElecEff;
	double TotalRealPow;
	double TotalReacPow;

	int64 time_advance;


	//end of synchronous generator inputs

	double Rs;//< internal transient resistance in p.u.
	double Xs;//< internal transient impedance in p.u.
    double Rg;//< grounding resistance in p.u.
	double Xg;//< grounding impedance in p.u.
	double Max_Ef;//< maximum induced voltage in p.u., e.g. 1.2
    double Min_Ef;//< minimus induced voltage in p.u., e.g. 0.8
	double Max_P;//< maximum real power capacity in kW
    double Min_P;//< minimus real power capacity in kW
	double Max_Q;//< maximum reactive power capacity in kVar
    double Min_Q;//< minimus reactive power capacity in kVar
	double Rated_kVA; //< nominal capacity in kVA
	double Rated_kV; //< nominal line-line voltage in kV
	complex voltage_A;//voltage
	complex voltage_B;
	complex voltage_C;
	complex current_A;      // current
	complex current_B;
	complex current_C;
	complex EfA;// induced voltage on phase A in Volt
	complex EfB;
	complex EfC;
	complex power_A;//power
	complex power_B;
	complex power_C;
	complex power_A_sch; // scheduled power
	complex power_B_sch;
	complex power_C_sch;    
	complex EfA_sch;// scheduled electric potential
	complex EfB_sch;
	complex EfC_sch;
	int SlackBus; //indicate whether slack bus


/*	double Real_Rs;
	double Real_Xs;
	double Real_Rg;
	double Real_Xg;
*/

public:
	/* required implementations */
	diesel_dg(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static diesel_dg *defaults;
	complex *get_complex(OBJECT *obj, char *name);

#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: diesel_dg.h,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	@file diesel_dg.h
	@addtogroup diesel_dg
	@ingroup MODULENAME

 @{  
 **/

#ifndef _diesel_dg_H
#define _diesel_dg_H

#include <stdarg.h>
#include "gridlabd.h"

class diesel_dg {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	diesel_dg(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static diesel_dg *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
