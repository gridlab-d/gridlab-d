/** $Id: inverter.h,v 1.0 2008/07/16
	Copyright (C) 2008 Battelle Memorial Institute
	@file inverter.h
	@addtogroup inverter

 @{  
 **/

#ifndef _inverter_H
#define _inverter_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"
#include "gridlabd.h"
#include "power_electronics.h"


//inverter extends power_electronics
class inverter: public power_electronics
{
private:
	/* TODO: put private variables here */

protected:
	/* TODO: put unpublished but inherited variables */
public:
	enum INVERTER_TYPE {TWO_PULSE=0, SIX_PULSE=1, TWELVE_PULSE=2, PWM=3}inverter_type_v;
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2} gen_status_v;
	//INVERTER_TYPE inverter_type_choice;
	complex V_In; // V_in (DC)
	complex I_In; // I_in (DC)
	complex VA_In; //power in (DC)

	
	complex VA_Out;
	double P_Out;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	double Q_Out;
	double V_Set_A;
	double V_Set_B;
	double V_Set_C;
	double margin;
	complex I_out_prev;
	complex I_step_max;
	double internal_losses;
	double C_Storage_In;
	double power_factor;

	complex V_In_Set_A;
	complex V_In_Set_B;
	complex V_In_Set_C; 

	complex *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines
	complex *pLine12;			//< used in triplex metering

	double output_frequency;
	double frequency_losses;

	complex phaseA_I_Out_prev;      // current
	complex phaseB_I_Out_prev;
	complex phaseC_I_Out_prev;

	complex phaseA_V_Out;//voltage
	complex phaseB_V_Out;
	complex phaseC_V_Out;
	complex phaseA_I_Out;      // current
	complex phaseB_I_Out;
	complex phaseC_I_Out;
	complex power_A;//power
	complex power_B;
	complex power_C;
	complex last_current[4];	//Previously applied power output (used to remove from parent so XML files look proper)
	//complex power_A_sch; // scheduled power
	//complex power_B_sch;
	//complex power_C_sch;  
	bool *NR_mode;			//Toggle for NR solving cycle.  If not NR, just goes to fals

public:
	/* required implementations */
	bool *get_bool(OBJECT *obj, char *name);
	inverter(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	complex *get_complex(OBJECT *obj, char *name);
public:
	static CLASS *oclass;
	static inverter *defaults;
	static CLASS *plcass;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: inverter.h,v 1.0 2008/06/16
	@file inverter.h
	@addtogroup inverter
	@ingroup MODULENAME

 @{  
 **/

#ifndef _inverter_H
#define _inverter_H

#include <stdarg.h>
#include "gridlabd.h"

class inverter {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	inverter(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static inverter *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
