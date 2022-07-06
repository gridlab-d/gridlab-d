/** $Id: rectifier.h,v 1.0 2008/07/17
	Copyright (C) 2008 Battelle Memorial Institute
	@file rectifier.h
	@addtogroup rectifier

 @{  
 **/

#ifndef _rectifier_H
#define _rectifier_H

#include <stdarg.h>

#include "generators.h"

class rectifier: public gld_object
{

protected:
	/* TODO: put unpublished but inherited variables */
public:
	enum RECTIFIER_TYPE {ONE_PULSE=0, TWO_PULSE=1, THREE_PULSE=2, SIX_PULSE=3, TWELVE_PULSE=4};
	enumeration rectifier_type_v;

		//Comaptibility variables - used to be in power_electronics
	int number_of_phases_out;	//Count for number of phases

	//General status variables
	set phases;	/**< device phases (see PHASE codes) */
    enum GENERATOR_MODE {CONSTANT_V=1, CONSTANT_PQ=2, CONSTANT_PF=4, SUPPLY_DRIVEN=5};
    enumeration gen_mode_v;  //operating mode of the generator 

	double efficiency;

	double VA_In; //power in (DC)
	double VA_Out;  // power out (DC)
	double P_Out;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	double Q_Out;
	double V_Rated;

	double voltage_out[3]; //voltage in

	double V_Out; // only one voltage out for DC

	double current_out[3];      // current in

	double I_Out; //only one current out for DC

	double power_out[3];//power

private:
	gld_property *pCircuit_V;		//< pointer to the inverter voltage
	gld_property *pLine_I;			//< pointer to the inverter current

public:
	/* required implementations */
	rectifier(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static rectifier *defaults;
	static CLASS *plcass;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif
