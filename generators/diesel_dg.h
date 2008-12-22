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
	complex AMx[3][3];//generator impedance matrix
	complex *pCircuit_V; ///< pointer to the three voltages on three lines
	complex *pLine_I; ///< pointer to the three current on three lines

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */
    enum {CONSTANTE=1, CONSTANTPQ} Gen_mode;  //< meter type 
	enum {OFFLINE=0, ONLINE=1} Gen_status;
//	double Rated_kW; //< nominal power in kW
//	double Rated_pf; //< power factor, P/(P^2+Q^2)^0.5
//	double Fuel_coefficient1;//< coefficient to calculate fuel cost
//	double Fuel_coefficient2;//< coefficient to calculate fuel cost
//	double Fuel_coefficient3;//< coefficient to calculate fuel cost
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
//	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
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
