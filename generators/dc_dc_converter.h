/** $Id: dc_dc_converter.h,v 1.0 2008/07/17
	Copyright (C) 2008 Battelle Memorial Institute
	@file dc_dc_converter.h
	@addtogroup dc_dc_converter

 @{  
 **/

#ifndef _dc_dc_converter_H
#define _dc_dc_converter_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"
#include "gridlabd.h"
#include "power_electronics.h"


//dc_dc_converter extends power_electronics
class dc_dc_converter: public power_electronics
{
private:
	/* TODO: put private variables here */
	//complex AMx[3][3];//generator impedance matrix

protected:
	/* TODO: put unpublished but inherited variables */
public:
	enum DC_DC_CONVERTER_TYPE {BUCK=0, BOOST=1, BUCK_BOOST=2};
	enumeration dc_dc_converter_type_v;
	//DC_DC_CONVERTER_TYPE dc_dc_converter_type_choice;
	complex V_In; // V_in (DC)
	complex I_In; // I_in (DC)
	complex VA_In; //power in (DC)
	complex VA_Out;  // power out (DC)
	double P_Out;	//assumed to be equal to VA_Out because DC Power does not have a reactive component
	double Q_Out;	//irrelevant for DC
	//double V_Set_A; // irrelevant for DC
	//double V_Set_B; //irrelevant for DC
	//double V_Set_C; //irrelevant for DC
	double V_Set;
	double margin;
	complex I_out_prev;
	complex I_step_max;
	double internal_losses;
	//double duty_ratio;
	//double on_ratio;
	double service_ratio;
	double C_Storage_In;
	double C_Storage_Out;

	//complex phaseA_I_Out_prev;      // current
	//complex phaseB_I_Out_prev;
	//complex phaseC_I_Out_prev;

	//complex phaseA_V_Out;//voltage
	//complex phaseB_V_Out;
	//complex phaseC_V_Out;
	complex V_Out; // only one voltage out for DC

	//complex phaseA_I_Out;      // current
	//complex phaseB_I_Out;
	//complex phaseC_I_Out;
	complex I_Out; //only one current out for DC

	//complex power_A;//power
	//complex power_B;
	//complex power_C;
	//complex power_A_sch; // scheduled power
	//complex power_B_sch;
	//complex power_C_sch;    

	
	complex *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines


public:
	/* required implementations */
	dc_dc_converter(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	complex *get_complex(OBJECT *obj, char *name);
public:
	static CLASS *oclass;
	static dc_dc_converter *defaults;
	static CLASS *plcass;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: dc_dc_converter.h,v 1.0 2008/07/17
	@file dc_dc_converter.h
	@addtogroup dc_dc_converter
	@ingroup MODULENAME

 @{  
 **/

#ifndef _dc_dc_converter_H
#define _dc_dc_converter_H

#include <stdarg.h>
#include "gridlabd.h"

class dc_dc_converter {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	dc_dc_converter(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static dc_dc_converter *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
