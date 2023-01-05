/** $Id: windturb_dg.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file windturb_dg.h
	@addtogroup windturb_dg
	@ingroup generators
 @{  
 **/
#ifndef _windturb_dg_H
#define _windturb_dg_H


#include <stdarg.h>
#include <vector>
#include <string>
#include <iostream>

#include "generators.h"

EXPORT STATUS windturb_dg_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure);

class windturb_dg : public gld_object
{
private:
	/* TODO: put private variables here */
	gld::complex AMx[3][3];			//Impedance matrix for Synchronous Generator
	gld::complex invAMx[3][3];		//Inverse of SG impedance matrix
	gld::complex IndTPMat[2][2];		//Induction Generator two port matrix
	gld::complex Vapu;				//Per unit voltage and current for Induction Generator at terminals
	gld::complex Vbpu;
	gld::complex Vcpu;
	gld::complex Iapu;
	gld::complex	Ibpu;
	gld::complex	Icpu;
	double air_dens;
	double Ridealgas;
	double Molar;
	double std_air_temp;
	double std_air_press;
	gld_property *pCircuit_V[3];		//< pointer to the three voltages on three lines
	gld_property *pLine_I[3];			//< pointer to the three current on three lines
	gld_property *pLine12;			    //< pointer to line current 12, used in triplex metering

	int number_of_phases_out;           //Used to flag three phase or triplex parent

	gld::complex value_Circuit_V[3];			//< value holder for voltage values
	gld::complex value_Line_I[3];			//< value holder for current values
	gld::complex value_Line12;               //< value holder for line current 12 in triplex metering


	bool parent_is_valid;				//< Flag to pointers
	bool parent_is_triplex;
	bool parent_is_inverter;
	
	double Power_Curve[2][100];  //Look-up table carrying power curve values. Maximum points limited to 100. Equals default (defined in .cpp) or user defined power curve 
	int number_of_points;

    gld_property *pPress;			
	gld_property *pTemp;			
	gld_property *pWS;				

    double value_Press;				
	double value_Temp;				
	double value_WS;				
	bool climate_is_valid;			//< Flag to pointer values
	
	//For current injection updates
	complex prev_current[3];
	bool NR_first_run;
	double internal_model_current_convergence;	//Variable to set convergence/reiteration context (for normal executions)

	complex prev_current12;
	
	//Inverter connections
	gld_property *inverter_power_property;
	gld_property *inverter_flag_property;
	
protected:
	/* TODO: put unpublished but inherited variables */

public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */

	gld::complex power_A;//power
	gld::complex power_B;
	gld::complex power_C;

	enum {OFFLINE=1, ONLINE};
	enumeration Gen_status;
	enum {INDUCTION=1, SYNCHRONOUS, USER_TYPE};
	enumeration Gen_type;
	enum {CONSTANTE=1, CONSTANTP, CONSTANTPQ};
	enumeration Gen_mode;
	enum {GENERIC_DEFAULT, GENERIC_SYNCH_SMALL, GENERIC_SYNCH_MID,GENERIC_SYNCH_LARGE, GENERIC_IND_SMALL, GENERIC_IND_MID, GENERIC_IND_LARGE, USER_DEFINED, VESTAS_V82, GE_25MW, BERGEY_10kW, GEN_TURB_POW_CURVE_2_4KW, GEN_TURB_POW_CURVE_10KW, GEN_TURB_POW_CURVE_100KW, GEN_TURB_POW_CURVE_1_5MW};
	enumeration Turbine_Model;
	enum {GENERAL_LARGE, GENERAL_MID,GENERAL_SMALL,MANUF_TABLE, CALCULATED, USER_SPECIFY};
	enumeration CP_Data;
	enum {POWER_CURVE=1, COEFF_OF_PERFORMANCE};
	enumeration Turbine_implementation;
	enum {DEFAULT=1, BUILT_IN, WIND_SPEED, CLIMATE_DATA};
	enumeration Wind_speed_source;

	double blade_diam;
	double turbine_height;
	double roughness_l;
	double ref_height;
	double Cp;

	int64 time_advance;

	double avg_ws;				//Default value for wind speed
	double cut_in_ws;			//Values are used to find GENERIC Cp
	double cut_out_ws;			// |
	double Cp_max;				// |
	double ws_maxcp;			// |
	double Cp_rated;			// |
	double ws_rated;			// |
	double wind_speed_hub_ht;

	double q;					//number of gearboxes

	double Pconv;				//Power converted from mechanical to electrical before elec losses
	double GenElecEff;			//Generator electrical efficiency used for testing

	unsigned int *n;

	gld::complex voltage_A;			//terminal voltage
	gld::complex voltage_B;
	gld::complex voltage_C;
	gld::complex current_A;			//terminal current
	gld::complex current_B;
	gld::complex current_C;

	double TotalRealPow;		//Real power supplied by generator - used for testing
	double TotalReacPow;		//Reactive power supplied - used for testing

	double Rated_VA;			// nominal capacity in VA
	double Rated_V;				// nominal line-line voltage in V
	double WSadj;				//Wind speed after all adjustments have been made (height, terrain, etc)
	double Wind_Speed;
	
	//Synchronous Generator
	gld::complex EfA;				// induced voltage on phase A in V
	gld::complex EfB;				// |
	gld::complex EfC;				// |
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
	gld::complex Vrotor_A;			// induced "rotor" voltage in pu
	gld::complex Vrotor_B;			// |
	gld::complex Vrotor_C;			// |
	gld::complex Irotor_A;			// "rotor" current generated in pu
	gld::complex Irotor_B;			// |
	gld::complex Irotor_C;			// |
	double Rst;					// stator internal impedance in p.u.
	double Xst;					// |
	double Rr;					// rotor internal impedance in p.u.
	double Xr;					// |
	double Rc;					// core/magnetization impedance in p.u.
	double Xm;					// |
	double Max_Vrotor;			// maximum induced voltage in p.u., e.g. 1.2
    double Min_Vrotor;			// minimum induced voltage in p.u., e.g. 0.8

	char power_curve_csv[1024]; // name of csv file containing the power curve
	bool power_curve_pu;		// Flag when set indicates that user provided power curve has power values in pu. Defaults to false in .cpp

public:
	/* required implementations */
	windturb_dg(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int init_climate(void);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	void compute_current_injection(void);
	void compute_current_injection_pc(void);
	void compute_power_injection_pc(void);
	STATUS updateCurrInjection(int64 iteration_count, bool *converged_failure);

	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
	void push_complex_powerflow_values(void);
	void push_complex_power_values(gld::complex inv_P);
	
	std::vector<std::string> readCSVRow(const std::string &row);
	std::vector<std::vector<std::string>> readCSV(std::istream &in);
	bool hasEnding(const std::string &fullString, const std::string &ending);

public:
	static CLASS *oclass;
	static windturb_dg *defaults;
	static CLASS *pclass;
};
#endif