/** $Id: solar.h,v 1.0 2008/07/18
	Copyright (C) 2008 Battelle Memorial Institute
	@file solar.h
	@addtogroup solar

 @{  
 **/

#ifndef _solar_H
#define _solar_H

#include <stdarg.h>
#include "gridlabd.h"

class solar : public gld_object
{
private:

protected:

public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */
	enum GENERATOR_MODE {CONSTANT_V=1, CONSTANT_PQ=2, CONSTANT_PF=4, SUPPLY_DRIVEN=5};
	enumeration gen_mode_v;  //operating mode of the generator 
	//note solar panel will always operate under the SUPPLY_DRIVEN generator mode
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2};
	enumeration gen_status_v;
	enum POWER_TYPE{DC=1, AC=2};
	enumeration power_type_v;
	enum PANEL_TYPE{SINGLE_CRYSTAL_SILICON=1, MULTI_CRYSTAL_SILICON=2, AMORPHOUS_SILICON=3, THIN_FILM_GA_AS=4, CONCENTRATOR=5};
	enumeration panel_type_v;
    enum INSTALLATION_TYPE {ROOF_MOUNTED=1, GROUND_MOUNTED=2};
	enumeration installation_type_v;
	enum SOLAR_TILT_MODEL {LIUJORDAN=0, SOLPOS=1, PLAYERVAL=2};
	enumeration solar_model_tilt;
	enum SOLAR_POWER_MODEL {BASEEFFICIENT=0, FLATPLATE=1};
	enumeration solar_power_model;

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

	//Variables for temperature correction - obtained from Sandia database for module types
	double module_acoeff;			//Temperature correction coefficient a
	double module_bcoeff;			//Temperature correction coefficient b
	double module_dTcoeff;			//Temperature difference coefficient associated with insolation heating
	double module_Tcoeff;			//Maximum power temperature coefficient

	double shading_factor;			//Shading factor
	double tilt_angle;				//Installation tilt angle
	double orientation_azimuth;		//published Orientation of the array
	bool fix_angle_lat;				//Fix tilt angle to latitude (replicates NREL SAM function)
	double soiling_factor;			//Soiling factor to be applied - makes user specifiable
	double derating_factor;			//Inverter derating factor - makes user specifiable

	enum ORIENTATION {DEFAULT=0, FIXED_AXIS=1, ONE_AXIS=2, TWO_AXIS=3, AZIMUTH_AXIS=4};
	enumeration orientation_type;	//Describes orientation features of PV

	FUNCTIONADDR calc_solar_radiation;	//Function pointer to climate's calculate solar radiation in degrees
		
	OBJECT *weather;
	double efficiency;
	double *pTout;
	double prevTemp, currTemp;
	TIMESTAMP prevTime;
	double *pRhout;
	double *pSolarD;		//Direct solar radiation
	double *pSolarH;		//Horizontal solar radiation
	double *pSolarG;		//Global horizontal
	double *pAlbedo;		//Ground reflectance
	double *pWindSpeed;

	double Max_P;//< maximum real power capacity in kW
    double Min_P;//< minimus real power capacity in kW
	//double Max_Q;//< maximum reactive power capacity in kVar
    //double Min_Q;//< minimus reactive power capacity in kVar
	double Rated_kVA; //< nominal capacity in kVA
	
	complex *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines

private:
	double orientation_azimuth_corrected;	//Corrected azimuth, for crazy "0=equator" referenced models

public:
	/* required implementations */
	solar(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	void derate_panel(double Tamb, double Insol);
	void calculate_IV(double Tamb, double Insol);
	int init_climate(void);

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
