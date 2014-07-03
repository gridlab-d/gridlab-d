/** $Id: microturbine.h,v 1.0 2008/07/18
	Copyright (C) 2008 Battelle Memorial Institute
	@file microturbine.h
	@addtogroup microturbine

 @{  
 **/

#ifndef _microturbine_H
#define _microturbine_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"
#include "gridlabd.h"


class microturbine : public gld_object
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
	//note microturbine panel will always operate under the SUPPLY_DRIVEN generator mode
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2};
	enumeration gen_status_v;
	enum POWER_TYPE{DC=1, AC=2};
	enumeration power_type_v;
	


	//GENERATOR_MODE generator_mode_choice;
	//GENERATOR_STATUS generator_status;

	double Rinternal;
	double Rload;
	complex V_Max;
	complex I_Max;

	double frequency;
	double Max_Frequency;
	double Min_Frequency;
	double Fuel_Used;
	double Heat_Out;
	double KV; //voltage constant
	double Power_Angle;
	
	double Max_P;//< maximum real power capacity in kW
    double Min_P;//< minimus real power capacity in kW
	
	double Max_Q;//< maximum reactive power capacity in kVar
    double Min_Q;//< minimus reactive power capacity in kVar
	double Rated_kVA; //< nominal capacity in kVA
	//double Rated_kV; //< nominal line-line voltage in kV
	
	double efficiency;
	
	complex E_A_Internal;
	complex E_B_Internal;
	complex E_C_Internal;

	complex phaseA_V_Out;//voltage
	complex phaseB_V_Out;
	complex phaseC_V_Out;
	
	//complex V_Out;
	
	complex phaseA_I_Out;      // current
	complex phaseB_I_Out;
	complex phaseC_I_Out;

	//complex I_Out;

	complex power_A_Out;//power
	complex power_B_Out;
	complex power_C_Out;

	complex VA_Out;

	double pf_Out;

	complex *pCircuit_V_A;//< pointer to the three voltages on three lines
	complex *pCircuit_V_B;
	complex *pCircuit_V_C;
	
	complex *pLine_I_A;			//< pointer to the three current on three lines
	complex *pLine_I_B;
	complex *pLine_I_C;


public:
	/* required implementations */
	microturbine(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	double determine_power_angle (complex power_out);
	complex determine_source_voltage(complex voltage_out, double r_internal, double r_load);
	double determine_frequency(complex power_out);
	double calculate_loss(double frequency_out);
	double determine_heat(complex power_out, double heat_loss);

	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	complex *get_complex(OBJECT *obj, char *name);
public:
	static CLASS *oclass;
	static microturbine *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: microturbine.h,v 1.0 2008/07/18
	@file microturbine.h
	@addtogroup microturbine
	@ingroup MODULENAME

 @{  
 **/

#ifndef _microturbine_H
#define _microturbine_H

#include <stdarg.h>
#include "gridlabd.h"

class microturbine {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	microturbine(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static microturbine *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
