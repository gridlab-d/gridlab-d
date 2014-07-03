/** $Id: rectifier.h,v 1.0 2008/07/17
	Copyright (C) 2008 Battelle Memorial Institute
	@file rectifier.h
	@addtogroup rectifier

 @{  
 **/

#ifndef _rectifier_H
#define _rectifier_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"
#include "gridlabd.h"
#include "power_electronics.h"


//rectifier extends power_electronics
class rectifier: public power_electronics
{
private:
	/* TODO: put private variables here */
	//complex AMx[3][3];//generator impedance matrix

protected:
	/* TODO: put unpublished but inherited variables */
public:
	enum RECTIFIER_TYPE {ONE_PULSE=0, TWO_PULSE=1, THREE_PULSE=2, SIX_PULSE=3, TWELVE_PULSE=4};
	enumeration rectifier_type_v;
	//RECTIFIER_TYPE rectifier_type_choice;
	//complex V_In; // V_in (DC)
	//complex I_In; // I_in (DC)
	complex VA_In; //power in (DC)
	complex VA_Out;  // power out (DC)
	double P_Out;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	double Q_Out;
	//double V_Rated_A; // irrelevant for DC
	//double V_Rated_B; //irrelevant for DC
	//double V_Rated_C; //irrelevant for DC
	complex V_In_Set_A;
	complex V_In_Set_B;
	complex V_In_Set_C; 
	double V_Rated;
	double margin;
	complex I_out_prev;
	complex I_step_max;
	double internal_losses;
	//double duty_ratio;
	double on_ratio;
	double input_frequency;
	double frequency_losses;

	double C_Storage_Out;

	//complex phaseA_I_Out_prev;      // current
	//complex phaseB_I_Out_prev;
	//complex phaseC_I_Out_prev;

	complex voltage_A; //voltage in
	complex voltage_B;
	complex voltage_C;

	complex V_Out; // only one voltage out for DC

	complex current_A;      // current in
	complex current_B;
	complex current_C;

	complex I_Out; //only one current out for DC

	complex power_A_In;//power
	complex power_B_In;
	complex power_C_In;
	//complex power_A_sch; // scheduled power
	//complex power_B_sch;
	//complex power_C_sch;    
	complex XphaseA;
	complex XphaseB;
	complex XphaseC;
	
	double *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines

public:
	/* required implementations */
	rectifier(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

	void iterative_IV(complex VA, char* phase_designation);

	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	complex *get_complex(OBJECT *obj, char *name);
	double *get_double(OBJECT *obj, char *name);
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

/**@}*/
/** $Id: rectifier.h,v 1.0 2008/07/17
	@file rectifier.h
	@addtogroup rectifier
	@ingroup MODULENAME

 @{  
 **/

#ifndef _rectifier_H
#define _rectifier_H

#include <stdarg.h>
#include "gridlabd.h"

class rectifier {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
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
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
