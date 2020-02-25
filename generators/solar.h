#ifndef GLD_GENERATORS_SOLAR_H_
#define GLD_GENERATORS_SOLAR_H_

#include <stdarg.h>

#include "generators.h"

//#define SOLAR_NR_EPSILON 1e-5

EXPORT SIMULATIONMODE interupdate_solar(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);

class solar : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;

protected:
public: /* Published Variables & Other Funcs For 'PV_CURVE' Mode */
	// Published Variables for N-R Solver (under the mode 'PV_CURVE')
	int16 max_nr_ite;
	double x0_root_rt;
	double eps_nr_ite;

	// Published Variables for Solar PV Panel (under the mode 'PV_CURVE')
	double pvc_t_ref_cels;
	double pvc_S_ref_wpm2;

	double pvc_a1;
	double pvc_b1;

	double pvc_U_oc_V;
	double pvc_I_sc_A;
	double pvc_U_m_V;
	double pvc_I_m_A;

	// Test & Init Funcs
	void print_init_pub_vars();
	void init_pub_vars_pvcurve_mode();

private: /* For N-R Solver & P-V Curve */
	// For and From Weather
	double pvc_cur_t_cels;
	double pvc_cur_S_wpm2;

	// N-R Sovler Part
	double nr_ep_rt(double);
	double nr_root_rt(double, double);

	using tpd_hf_ptr = double (solar::*)(double, double);
	double newton_raphson(double, tpd_hf_ptr, double, double = 0);
	double nr_root_search(double, double, double);

	double get_i_from_u(double);
	double get_p_from_u(double);
	double get_p_max(double = 0);
	double get_u_of_p_max(double = 0);

	void test_nr_solver(); // Test func

	// Solar PV Panel Part (P-V Curve)
	double pvc_C1;
	double pvc_C2;

	double hf_dU(double t);
	double hf_dI(double t, double S);
	double hf_I(double U, double t, double S);
	double hf_P(double U, double t, double S);
	double hf_f(double U, double t, double S, double P);
	double hf_dIdU(double U, double t);
	double hf_d2IdU2(double U, double t);
	double hf_dfdU(double U, double t, double S);
	double hf_d2fdU2(double U, double t);

	void display_params(); // Test func

public:
	/* Published Variables of Two Modes (BASEEFFICIENT = 0, FLATPLATE = 1) */
	enum GENERATOR_MODE
	{
		CONSTANT_V = 1,
		CONSTANT_PQ = 2,
		CONSTANT_PF = 4,
		SUPPLY_DRIVEN = 5
	};
	enumeration gen_mode_v; //operating mode of the generator
	//note solar panel will always operate under the SUPPLY_DRIVEN generator mode
	enum POWER_TYPE
	{
		DC = 1,
		AC = 2
	};
	enumeration power_type_v;
	enum PANEL_TYPE
	{
		SINGLE_CRYSTAL_SILICON = 1,
		MULTI_CRYSTAL_SILICON = 2,
		AMORPHOUS_SILICON = 3,
		THIN_FILM_GA_AS = 4,
		CONCENTRATOR = 5
	};
	enumeration panel_type_v;
	enum INSTALLATION_TYPE
	{
		ROOF_MOUNTED = 1,
		GROUND_MOUNTED = 2
	};
	enumeration installation_type_v;
	enum SOLAR_TILT_MODEL
	{
		LIUJORDAN = 0,
		SOLPOS = 1,
		PLAYERVAL = 2
	};
	enumeration solar_model_tilt;
	enum SOLAR_POWER_MODEL
	{
		BASEEFFICIENT = 0,
		FLATPLATE = 1,
		PV_CURVE = 2
	};
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
	double module_acoeff;  //Temperature correction coefficient a
	double module_bcoeff;  //Temperature correction coefficient b
	double module_dTcoeff; //Temperature difference coefficient associated with insolation heating
	double module_Tcoeff;  //Maximum power temperature coefficient

	double shading_factor;		//Shading factor
	double tilt_angle;			//Installation tilt angle
	double orientation_azimuth; //published Orientation of the array
	bool fix_angle_lat;			//Fix tilt angle to latitude (replicates NREL SAM function)
	double soiling_factor;		//Soiling factor to be applied - makes user specifiable
	double derating_factor;		//Inverter derating factor - makes user specifiable

	enum ORIENTATION
	{
		DEFAULT = 0,
		FIXED_AXIS = 1,
		ONE_AXIS = 2,
		TWO_AXIS = 3,
		AZIMUTH_AXIS = 4
	};
	enumeration orientation_type; //Describes orientation features of PV

	FUNCTIONADDR calc_solar_radiation; //Function pointer to climate's calculate solar radiation in degrees

	OBJECT *weather;
	double efficiency;
	double prevTemp, currTemp;
	TIMESTAMP prevTime;

	double Max_P; //< maximum real power capacity in kW
	double Min_P; //< minimus real power capacity in kW

private:
	double orientation_azimuth_corrected; //Corrected azimuth, for crazy "0=equator" referenced models

	//Pointers to properties
	gld_property *pTout;
	gld_property *pWindSpeed;

	//Inverter connections
	gld_property *inverter_voltage_property;
	gld_property *inverter_current_property;

	//Default voltage and current values, if ran "headless"
	complex default_voltage_array;
	complex default_current_array;

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

	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);

public:
	static CLASS *oclass;
	static solar *defaults;
#ifdef OPTIONAL
	static CLASS *pclass;						/**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif // GLD_GENERATORS_SOLAR_H_
