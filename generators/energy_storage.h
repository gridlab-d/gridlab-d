/** $Id: energy_storage.h,v 1.0 2008/07/18
	Copyright (C) 2008 Battelle Memorial Institute
	@file energy_storage.h
	@addtogroup energy_storage

 @{  
 **/

#ifndef _energy_storage_H
#define _energy_storage_H

#include <stdarg.h>

#include "generators.h"

class energy_storage : public gld_object
{
private:

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */
	enum GENERATOR_MODE {CONSTANT_V=1, CONSTANT_PQ, CONSTANT_PF, SUPPLY_DRIVEN};
	enumeration gen_mode_v;  //operating mode of the generator 
	//note energy_storage panel will always operate under the SUPPLY_DRIVEN generator mode
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2};
	enumeration gen_status_v;
	enum POWER_TYPE{DC=0, AC=1};
	enumeration power_type_v;

	gld::complex V_Max;
	gld::complex I_Max;
	double E_Max;
	double Energy;
	bool recalculate;
	double margin;
	
	double Max_P;//< maximum real power capacity in kW
    double Min_P;//< minimus real power capacity in kW
	
	double Rated_kVA; //< nominal capacity in kVA
	
	double efficiency;

	TIMESTAMP prev_time;
	double E_Next;
	
	
	gld::complex V_In;
	gld::complex I_In;
	gld::complex V_Internal;
	double Rinternal;
	gld::complex VA_Internal;
	gld::complex I_Prev;
	gld::complex I_Internal;
	double base_efficiency;

	gld::complex V_Out;
	
	gld::complex I_Out;

	gld::complex VA_Out;

	gld::complex *pCircuit_V;		//< pointer to the three voltages on three lines
	gld::complex *pLine_I;			//< pointer to the three current on three lines
	bool connected; // true if conencted to another item down the line, false if this is the only item on the bus

public:
	/* required implementations */
	energy_storage(void);
	energy_storage(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	
	//this should be overridden by a customized method in child classes
	double calculate_efficiency(gld::complex voltage, gld::complex current);

	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	gld::complex calculate_v_terminal(gld::complex v, gld::complex i);

public:
	static CLASS *oclass;
	static energy_storage *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: energy_storage.h,v 1.0 2008/07/18
	@file energy_storage.h
	@addtogroup energy_storage
	@ingroup MODULENAME

 @{  
 **/
