/** $Id: battery.h,v 1.0 2008/07/18
	Copyright (C) 2008 Battelle Memorial Institute
	@file battery.h
	@addtogroup battery

 @{  
 **/

#ifndef _battery_H
#define _battery_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"

#include "gridlabd.h"
#include "energy_storage.h"

class battery : public energy_storage
{
private:
	/* TODO: put private variables here */
	//complex AMx[3][3];//generator impedance matrix

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */
	enum GENERATOR_MODE {CONSTANT_V=1, CONSTANT_PQ, CONSTANT_PF, SUPPLY_DRIVEN} gen_mode_v;  //operating mode of the generator 
	//note battery panel will always operate under the SUPPLY_DRIVEN generator mode
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2} gen_status_v;
	enum POWER_TYPE{DC1, AC=2} power_type_v;
	enum RFB_SIZE{SMALL=1, MED_COMMERCIAL, MED_HIGH_ENERGY, LARGE} rfb_size_v;


	//GENERATOR_MODE generator_mode_choice;
	//GENERATOR_STATUS generator_status;
	//RFB_SIZE rfb_size_choice;

	
	//complex V_Max;
	//complex I_Max;
	//double E_Max;
	//double Energy;
	//bool recalculate;
	//double margin;
	//
	//double Max_P;//< maximum real power capacity in kW
 //   double Min_P;//< minimus real power capacity in kW
	//
	////double Max_Q;//< maximum reactive power capacity in kVar
 //   //double Min_Q;//< minimus reactive power capacity in kVar
	//double Rated_kVA; //< nominal capacity in kVA
	////double Rated_kV; //< nominal line-line voltage in kV
	//
	//double efficiency;

	//TIMESTAMP prev_time;
	//double E_Next;
	
	
	//complex V_In;
	//complex I_In;
	//complex V_Internal;
	//double Rinternal;
	//complex VA_Internal;
	//complex I_Prev;

	//complex V_Out;

	//complex I_Out;

	//complex VA_Out;

	//
	//complex *pCircuit_V;		//< pointer to the three voltages on three lines
	//complex *pLine_I;			//< pointer to the three current on three lines
	//bool connected; // true if conencted to another item down the line, false if this is the only item on the bus

public:
	/* required implementations */
	battery(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	//double timestamp_to_hours(TIMESTAMP t);
	TIMESTAMP rfb_event_time(TIMESTAMP t0, complex power, double e);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	double calculate_efficiency(complex voltage, complex current);
	complex *get_complex(OBJECT *obj, char *name);
	complex calculate_v_terminal(complex v, complex i);

public:
	static CLASS *oclass;
	static battery *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: battery.h,v 1.0 2008/07/18
	@file battery.h
	@addtogroup battery
	@ingroup MODULENAME

 @{  
 **/

#ifndef _battery_H
#define _battery_H

#include <stdarg.h>
#include "gridlabd.h"

class battery {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	battery(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static battery *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
