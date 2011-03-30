/** $Id: solar.h,v 1.0 2008/07/18
	Copyright (C) 2008 Battelle Memorial Institute
	@file solar.h
	@addtogroup solar

 @{  
 **/

#ifndef _solar_H
#define _solar_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"
#include "gridlabd.h"

class solar
{
private:
	/* TODO: put private variables here */
	//complex AMx[3][3];//generator impedance matrix

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */
	enum GENERATOR_MODE {CONSTANT_V=1, CONSTANT_PQ=2, CONSTANT_PF=4, SUPPLY_DRIVEN=5} gen_mode_v;  //operating mode of the generator 
	//note solar panel will always operate under the SUPPLY_DRIVEN generator mode
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2} gen_status_v;
	enum POWER_TYPE{DC=1, AC=2} power_type_v;
	enum PANEL_TYPE{SINGLE_CRYSTAL_SILICON=1, MULTI_CRYSTAL_SILICON=2, AMORPHOUS_SILICON=3, THIN_FILM_GA_AS=4, CONCENTRATOR=5} panel_type_v;
    enum INSTALLATION_TYPE {ROOF_MOUNTED=1, GROUND_MOUNTED=2} installation_type_v;

	//GENERATOR_MODE generator_mode_choice;
	//GENERATOR_STATUS generator_status;
	//PANEL_TYPE panel_type_choice;

	double NOCT;
	double Tcell;
	double Tmodule;
	double Tambient;
	double Insolation;
	double Rinternal;
	double Rated_Insolation;
	complex V_Max;
	complex Voc;
	complex Voc_Max;
	double area;
	double Tamb;
	double wind_speed;
	double Pmax_temp_coeff;
    double Voc_temp_coeff;
    double w1;
    double w2;
	double w3;
	double constant;
	complex P_Out;
	complex V_Out;
	complex I_Out;
	complex VA_Out;
	
	double efficiency;
	double *pTout;
	//double vTout;
	double *pRhout;
	double *pSolar;
	bool *NR_mode;			//Toggle for NR solving cycle.  If not NR, just goes to fals

//	double Rated_kW; //< nominal power in kW
//	double Rated_pf; //< power factor, P/(P^2+Q^2)^0.5
//	double Fuel_coefficient1;//< coefficient to calculate fuel cost
//	double Fuel_coefficient2;//< coefficient to calculate fuel cost
//	double Fuel_coefficient3;//< coefficient to calculate fuel cost
	//double Rs;//< internal transient resistance in p.u.
	//double Xs;//< internal transient impedance in p.u.
    //double Rg;//< grounding resistance in p.u.
	//double Xg;//< grounding impedance in p.u.
	//double Max_Ef;//< maximum induced voltage in p.u., e.g. 1.2
    //double Min_Ef;//< minimus induced voltage in p.u., e.g. 0.8
	double Max_P;//< maximum real power capacity in kW
    double Min_P;//< minimus real power capacity in kW
	//double Max_Q;//< maximum reactive power capacity in kVar
    //double Min_Q;//< minimus reactive power capacity in kVar
	double Rated_kVA; //< nominal capacity in kVA
	//double Rated_kV; //< nominal line-line voltage in kV
	//complex voltage_A;//voltage
	//complex voltage_B;
	//complex voltage_C;
	
	complex *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines


	//double V_Out;
	
	//complex current_A;      // current
	//complex current_B;
	//complex current_C;

	//double I_Out;

	//complex EfA;// induced voltage on phase A in Volt
	//complex EfB;
	//complex EfC;
	//complex power_A;//power
	//complex power_B;
	//complex power_C;

	//double VA_Out;

	//complex power_A_sch; // scheduled power
	//complex power_B_sch;
	//complex power_C_sch;    
	//complex EfA_sch;// scheduled electric potential
	//complex EfB_sch;
	//complex EfC_sch;


/*	double Real_Rs;
	double Real_Xs;
	double Real_Rg;
	double Real_Xg;
*/

public:
	/* required implementations */
	solar(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	void derate_panel(double Tamb, double Insol);
	void calculate_IV(double Tamb, double Insol);
	int init_climate(void);
	bool *get_bool(OBJECT *obj, char *name);

	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	complex *get_complex(OBJECT *obj, char *name);
public:
	static CLASS *oclass;
	static solar *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: solar.h,v 1.0 2008/07/18
	@file solar.h
	@addtogroup solar
	@ingroup MODULENAME

 @{  
 **/

#ifndef _solar_H
#define _solar_H

#include <stdarg.h>
#include "gridlabd.h"

class solar {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	solar(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static solar *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
