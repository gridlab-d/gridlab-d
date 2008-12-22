/** $Id: windturb_dg.h,v 0.1 2008/06/06 12:00 d3x289 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file windturb_dg.h
	@addtogroup windturb_dg
	@ingroup generators

 @{  
 **/
#ifndef _windturb_dg_H
#define _windturb_dg_H


#include <stdarg.h>
#include "../powerflow/powerflow_object.h"   
#include "../powerflow/node.h"
#include "gridlabd.h"

/*
#ifdef _WINDTURB_DG_CPP
#define GLOBAL
#define INIT(A) = (A)
#else
#define GLOBAL extern
#define INIT(A)
#endif
*/

	
class windturb_dg
{
private:
	/* TODO: put private variables here */
	complex AMx[3][3];			//Impedance matrix for Synchronous Generator
	complex invAMx[3][3];		//Inverse of SG impedance matrix
	complex IndTPMat[2][2];		//Induction Generator two port matrix
	complex Vapu;				//Per unit voltage and current for Induction Generator at terminals
	complex Vbpu;
	complex Vcpu;
	complex Iapu;
	complex	Ibpu;
	complex	Icpu;
	double Ridealgas;
	double Molar;
	double std_air_dens;
	double std_air_temp;
	double std_air_press;
	complex *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines
	
protected:
	/* TODO: put unpublished but inherited variables */

public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */

	enum {OFFLINE=1, ONLINE} Gen_status;
	enum {INDUCTION=1, SYNCHRONOUS} Gen_type;
	enum {CONSTANTE=1, CONSTANTP, CONSTANTPQ} Gen_mode;
	enum {GENERIC_SYNCH=1, GENERIC_IND, VESTAS_V82, GE_25MW} Turbine_Model;
	enum {GENERAL=1, MANUF_TABLE} CP_Data;

	double blade_diam;
	double turbine_height;
	double roughness_l;
	double ref_height;

	int64 time_advance;

	double avg_ws;				//Default value for wind speed
	double cut_in_ws;			//Values are used to find GENERIC Cp
	double cut_out_ws;			// |
	double Cp_max;				// |
	double ws_maxcp;			// |
	double Cp_rated;			// |
	double ws_rated;			// |

	double q;					//number of gearboxes

    double * pPress;			//Used to find air density
	double * pTemp;				// |
	double * pWS;				// |
	double * pair_density;		// |
	double Pconv;				//Power converted from mechanical to electrical before elec losses
	double GenElecEff;			//Generator electrical efficiency used for testing

	unsigned int *n;

	complex voltage_A;			//terminal voltage
	complex voltage_B;
	complex voltage_C;
	complex current_A;			//terminal current
	complex current_B;
	complex current_C;

	double TotalRealPow;		//Real power supplied by generator - used for testing
	double TotalReacPow;		//Reactive power supplied - used for testing

	double Rated_VA;			// nominal capacity in VA
	double Rated_V;				// nominal line-line voltage in V
	double WSadj;				//Wind speed after all adjustments have been made (height, terrain, etc)
	double Wind_Speed;
	
	//Synchronous Generator
	complex EfA;				// induced voltage on phase A in V
	complex EfB;				// |
	complex EfC;				// |
	double Rs;					// internal transient resistance in p.u.
	double Xs;					// internal transient impedance in p.u.
    double Rg;					// grounding resistance in p.u.
	double Xg;					// grounding impedance in p.u.
	double Max_Ef;				// maximum induced voltage in p.u., e.g. 1.2
    double Min_Ef;				// minimum induced voltage in p.u., e.g. 0.8
	double Max_P;				// maximum real power capacity in kW
    double Min_P;				// minimum real power capacity in kW
	double Max_Q;				// maximum reactive power capacity in kVar
    double Min_Q;				// minimum reactive power capacity in kVar
	double pf;					// desired power factor - TO DO: implement later use with controller

	//Induction Generator
	complex Vrotor_A;			// induced "rotor" voltage in pu
	complex Vrotor_B;			// |
	complex Vrotor_C;			// |
	complex Irotor_A;			// "rotor" current generated in pu
	complex Irotor_B;			// |
	complex Irotor_C;			// |
	double Rst;					// stator internal impedance in p.u.
	double Xst;					// |
	double Rr;					// rotor internal impedance in p.u.
	double Xr;					// |
	double Rc;					// core/magnetization impedance in p.u.
	double Xm;					// |
	double Max_Vrotor;			// maximum induced voltage in p.u., e.g. 1.2
    double Min_Vrotor;			// minimum induced voltage in p.u., e.g. 0.8




public:
	/* required implementations */
	windturb_dg(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int init_climate(void);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);


public:
	static CLASS *oclass;
	static windturb_dg *defaults;
	complex *get_complex(OBJECT *obj, char *name);

#endif
};