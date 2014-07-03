/** $Id: power_electronics.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file power_electronics.h
	@addtogroup power_electronics

 @{  
 **/


#ifndef _power_electronics_H
#define _power_electronics_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"
#include "gridlabd.h"

/**
	The power_electronics class is meant as a power device interface, to be used specifically with
	distributed generation modules.  The class serves as a parent class to a variety of power_electronics
	implementation classes, the inverter (dc to ac), dc converter (dc to dc), and rectifier (ac to dc).
*/

class power_electronics : public gld_object
{
private:
	/* TODO: put private variables here */
	//complex AMx[3][3];//generator impedance matrix

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */
    enum GENERATOR_MODE {CONSTANT_V=1, CONSTANT_PQ=2, CONSTANT_PF=4, SUPPLY_DRIVEN=5};
    enumeration gen_mode_v;  //operating mode of the generator 
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2};
    enumeration gen_status_v;
	enum CONVERTER_TYPE{VOLTAGE_SOURCED=1, CURRENT_SOURCED=2};
    enumeration converter_type_v; //current sourced not implemented
	enum SWITCH_TYPE {IDEAL_SWITCH=1, BJT=2, MOSFET=3, SCR=4, JFET=5, IBJT=6, DARLINGTON=7};
    enumeration switch_type_v;
	enum FILTER_TYPE {LOW_PASS=1, HIGH_PASS=2, BAND_STOP=3, BAND_PASS=4};
    enumeration filter_type_v; //only band_pass implemented
	enum FILTER_IMPLEMENTATION {IDEAL_FILTER=1, CAPACITIVE=2, INDUCTIVE=3, SERIES_RESONANT=4, PARALLEL_RESONANT=5};
    enumeration filter_imp_v;
	
	enum FILTER_FREQUENCY{F120HZ=1, F180HZ=2, F240HZ=3};
    enumeration filter_freq_v;
	enum POWER_TYPE{DC=1, AC=2};
	enumeration power_type_v;
	
	double set_terminal_voltage;
	double max_current_step_size;
	double Rated_kW; //< nominal power in kW
	double Max_P;//< maximum real power capacity in kW
    double Min_P;//< minimum real power capacity in kW
	double Max_Q;//< maximum reactive power capacity in kVar
    double Min_Q;//< minimum reactive power capacity in kVar
	double Rated_kVA; //< nominal capacity in kVA
	double Rated_kV; //< nominal line-line voltage in kV
	double Rinternal;
	double Rload;
	double Rtotal;
	double Rphase;
	complex Xphase;
	double Rground;
	double Rground_storage;
	double Rsfilter[3];
	double Rgfilter[3];
	double Xsfilter[3];
	double Xgfilter[3];

	double Rsfilter_total;
	double Rgfilter_total;
	double Xsfilter_total;
	double Xgfilter_total;

	char* parent_string;


	double Cinternal;
	double Cground;
	double Ctotal;
	//double Cfilter[3];
	double Linternal;
	double Lground;
	double Ltotal;
	
	double Vdc;
	//double Lfilter[3];
	bool filter_120HZ;
	bool filter_180HZ;
	bool filter_240HZ;
	double pf_in;
	double pf_out;
	int number_of_phases_in;
	int number_of_phases_out;
	bool phaseAIn;
	bool phaseBIn;
	bool phaseCIn;
	bool phaseAOut;
	bool phaseBOut;
	bool phaseCOut;
/*
	SWITCH_TYPE switch_type_choice;
	SWITCH_IMPLEMENTATION switch_implementation_choice;
	GENERATOR_MODE generator_mode_choice;
	POWER_TYPE power_in;
	POWER_TYPE power_out;
*/

public:
	SWITCH_TYPE switch_type_choice;
	//GENERATOR_MODE generator_mode_choice;
	//GENERATOR_STATUS generator_status;
	//FILTER_TYPE filter_type_choice;
	//FILTER_IMPLEMENTATION filter_implementation_choice;
	POWER_TYPE power_in;
	POWER_TYPE power_out;
	double efficiency;
	complex losses;

/*	
	double Rs;//< internal transient resistance in p.u.
	double Xs;//< internal transient impedance in p.u.
    double Rg;//< grounding resistance in p.u.
	double Xg;//< grounding impedance in p.u.
	double Max_Ef;//< maximum induced voltage in p.u., e.g. 1.2
    double Min_Ef;//< minimum induced voltage in p.u., e.g. 0.8
*/

	//no power dealt with in parent class, only in child classes because AC/DC in/out needs to be
	//detailed before calculations can be performed
	/*
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

*/

/*	double Real_Rs;
	double Real_Xs;
	double Real_Rg;
	double Real_Xg;
*/

public:
	/* required implementations */
	power_electronics(void);
	power_electronics(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

	//helper functions
	int filter_circuit_impact(FILTER_TYPE filter_type_choice, FILTER_IMPLEMENTATION filter_implementation_choice);
	complex filter_voltage_impact_source(complex desired_I_out, complex desired_V_out);
	complex filter_current_impact_source(complex desired_I_out, complex desired_V_out);
	complex filter_voltage_impact_out(complex source_I_in, complex source_V_in);
	complex filter_current_impact_out(complex source_I_in, complex source_V_in);
	double internal_switch_resistance(SWITCH_TYPE switch_type_choice);
	double calculate_loss(double Rtotal, double LTotal, double Ctotal, POWER_TYPE power_in, POWER_TYPE power_out);
	double calculate_frequency_loss(double frequency, double Rtotal, double Ltotal, double Ctotal);

	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static power_electronics *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif
