/** $Id: house_e.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house_e.h
	@addtogroup house_e
	@ingroup residential

 @{
 **/

#ifndef _HOUSE_E_H
#define _HOUSE_E_H

#include "residential.h"
#include "complex.h"
#include "enduse.h"
#include "loadshape.h"
#include "residential_enduse.h"

typedef struct s_implicit_enduse {
	enduse load;
	double amps;
	int is220;
	struct s_implicit_enduse *next;
} IMPLICITENDUSE;

typedef enum {
	// analog loads (continuous load) - bits 0-15
	IEU_LIGHTS			= 0x00000001, ///< implicit lights load
	IEU_PLUGS			= 0x00000002, ///< implicit plugs load
	IEU_OCCUPANCY		= 0x00000004, ///< implicit occupancy load
	
	// pulsed loads (unqueued aperiodic loads) - bits 16-31
	IEU_DISHWASHER		= 0x00000100, ///< implicit dishwasher load
	IEU_MICROWAVE		= 0x00000200, ///< implicit microwave load

	// modulated loads (periodic loads) - bits 32-47
	IEU_FREEZER			= 0x00010000, ///< implicit freezer load
	IEU_REFRIGERATOR	= 0x00020000, ///< implicit refrigerator load
	IEU_RANGE			= 0x00040000, ///< implicit cooking load
	IEU_EVCHARGER		= 0x00080000, ///< implicit evcharger load
	IEU_WATERHEATER		= 0x00100000, ///< implicit waterheater load

	// queued loads (queued aperiodic loads) - bits 48-63
	IEU_CLOTHESWASHER	= 0x01000000, ///< implicit clotheswasher load
	IEU_DRYER			= 0x02000000, ///< implicit dryer load

	/// @todo add other implicit enduse flags as they are defined
	IEU_ALL				= 0x03170303, ///< all (needed to filter)
} IMPLICITENDUSEFLAGS;

//Solar quadrant definitions
#define NO_SOLAR	0x0000
#define HORIZONTAL	0x0001
#define NORTH		0x0002
#define EAST		0x0004
#define SOUTH		0x0008
#define WEST		0x0010

class house_e : public residential_enduse { /*inherits due to HVAC being a load */
public:
	object weather; ///< reference to the climate
	PANEL panel; ///< main house_e panel
	/// Get voltage on a circuit
	/// @return voltage (or 0 if breaker is open)
	inline complex V(CIRCUIT *c)			///< pointer to circuit 
	{ return c->status==BRK_CLOSED ? *(c->pV) : complex(0,0);};
	complex *pCircuit_V;					///< pointer to the three voltages on three lines
	complex *pLine_I;						///< pointer to the three current on three lines
	complex *pShunt;						///< pointer to shunt value on triplex parent
	complex *pPower;						///< pointer to power value on triplex parent
	bool *pHouseConn;						///< Pointer to house_present variable on triplex parent
	int *pMeterStatus;						///< Pointer to service_status variable on triplex parent
	IMPLICITENDUSE *implicit_enduse_list;	///< implicit enduses
	static set implicit_enduses_active;		///< implicit enduses that are to be activated
public:
	// building design variables
	double floor_area;							///< house_e floor area (ft^2)
	double envelope_UA;							///< envelope UA (BTU.sq.ft/hr.ft2)
	double number_of_doors;						///< door-wall ratio
	double area_per_door;						///< area per exterior door
	double total_door_area;						///< exterior door area
	double total_wall_area;						///< exterior opaque wall area
	double gross_wall_area;						///< gross wall area (sq.ft)
	double ceiling_height;						///< ceiling height
	double interior_exterior_wall_ratio;		///< ratio of internal to external wall area
	double exterior_wall_fraction;				///< exterior-wall ratio
	double exterior_ceiling_fraction;			///< ratio of external ceiling sf to floor area
	double exterior_floor_fraction;
	double aspect_ratio;						///< building footprint aspect ratio
	double solar_aperture[N_SOLAR_SURFACES];	///< Future: Solar aperture(WWR) by orientation
	double house_content_heat_transfer_coeff;	///< mass UA
	double air_density;							///< air density
	double air_heat_capacity;					///< heat capacity of air
	double house_content_thermal_mass;			///< house thermal mass (BTU/F)
	double total_thermal_mass_per_floor_area;	///<Total thermal mass per unit of floor area (Rob's rule of thumb is 2 for wood frame)
	double interior_surface_heat_transfer_coeff;///< Rob's rule of thumb is 1
	double air_heat_fraction;					///< fraction of gains that go to air - this variable is not needed consider deleting?
	double mass_solar_gain_fraction;			///< fraction of solar gains that goes to the mass
	double mass_internal_gain_fraction;		///< fraction of the internal gains that goes to the mass
	double number_of_stories;
	
	// additional reverse etp parameters
	set include_solar_quadrant;
	typedef enum {HC_DEFAULT=0, HC_FLAT=1, HC_LINEAR=2, HC_CURVED=3};
	enumeration heating_cop_curve;
	typedef enum {HP_DEFAULT=0, HP_FLAT=1, HP_LINEAR=2, HP_CURVED=3};
	enumeration heating_cap_curve;
	typedef enum {CC_DEFAULT=0, CC_FLAT=1, CC_LINEAR=2, CC_CURVED=3};
	enumeration cooling_cop_curve;
	typedef enum {CP_DEFAULT=0, CP_FLAT=1, CP_LINEAR=2, CP_CURVED=3};
	enumeration cooling_cap_curve;
	bool use_latent_heat;
	bool include_fan_heatgain;
	double fan_heatgain_fraction;
	double horizontal_diffuse_solar_radiation;
	double north_incident_solar_radiation;
	double north_west_incident_solar_radiation;
	double west_incident_solar_radiation;
	double south_west_incident_solar_radiation;
	double south_incident_solar_radiation;
	double south_east_incident_solar_radiation;
	double east_incident_solar_radiation;
	double north_east_incident_solar_radiation;

	// window variables
	double glazing_shgc;						///< glazing SHGC
	double window_wall_ratio;					///< window-wall ratio
	double window_roof_ratio;					///< window-roof ratio (skylights)
	double total_window_area;
	double window_exterior_transmission_coefficient; ///< fraction of energy that transmits through windows
	double solar_heatgain_factor;				///< product of the window solar heatgain coefficient and the exterior transmission coefficient

	// system design variables
	// thermostat
	double thermostat_deadband;		///< thermostat deadband (degF)
	int16 thermostat_cycle_time;	///< thermostat minimum cycle time (seconds)
	int16 thermostat_off_cycle_time; ///< the minimum amount of time the thermostat cycle must stay in the off state
	int16 thermostat_on_cycle_time; ///< the minimum amount of time the thermostat cycle must stay in the on state
	TIMESTAMP thermostat_last_cycle_time;
	TIMESTAMP thermostat_last_off_cycle_time;
	TIMESTAMP thermostat_last_on_cycle_time;
	double heating_setpoint;		///< heating setpoint (degF)
	double cooling_setpoint;		///< cooling setpoint (degF)
	double dlc_offset;

	// hvac control variable
	typedef enum {TC_FULL=0, TC_BAND=1, TC_NONE=2};
	enumeration thermostat_control;	///< method of HVAC control
	double TcoolOn;		///< temperature above which cooling turns on
	double TcoolOff;	///< temperature below which cooling turns off
	double TheatOn;		///< temperature below which heating turns on
	double TheatOff;	///< temperature above which heating turns off
	double TauxOn;		///< temperature below which aux heating turns on

	// hvac characteristics
	double design_heating_setpoint;	///< design heating setpoint (degF)
	double design_cooling_setpoint;	///< design cooling setpoint (degF)
	double design_heating_capacity;	///< space heating capacity (BTUh/sf)
	double design_cooling_capacity;	///< space cooling capacity (BTUh/sf)
	double heating_COP;				///< space heating COP
	double cooling_COP;				///< space cooling COP

	double over_sizing_factor;		///< Future: equipment over sizing factor
	double rated_heating_capacity;	///< rated heating capacity of the system (BTUh/sf; varies w.r.t Tout),
	double rated_cooling_capacity;	///< rated cooling capacity of the system (BTUh/sf; varies w.r.t Tout)
	double hvac_breaker_rating;		///< HVAC current limit on the breaker
	double hvac_power_factor;		///< HVAC power factor
	// auxiliary heat characteristics
	double aux_heat_capacity;		///< auxiliary (resistive) heat is COP 1 and has no adjustment
	double aux_heat_deadband;		///< additional deadband before auxiliary heat engages
	double aux_heat_temp_lockout;	///< outdoor temperature at which the auxiliary heat will automatically engage
	double aux_heat_time_delay;		///< minimum time the heat pump must run until the auxiliary heating engages
	// fan characteristics
	double fan_design_power;		///< designed maximum power draw of the ventilation fan
	double fan_low_power_fraction;	///< fraction of ventilation fan power draw during low-power mode (two-speed only)
	double fan_power;				///< current power being fed to the ventilation fan
	double fan_design_airflow;		///< designed airflow for the ventilation system (cf/min)

	///These are only used when fan is alone (resistive or gas heating)
	double fan_impedance_fraction;  ///< ZIP fraction for fan - impedance
	double fan_current_fraction;    ///< ZIP fraction for fan - current
	double fan_power_fraction;      ///< ZIP fraction for fan - power
	double fan_power_factor;		///< power factor of fan
	///
	double duct_pressure_drop;		///< pressure drop across the ventilation ducting, in inches of water pressure
	double cooling_supply_air_temp;
	double heating_supply_air_temp;
	// building HVAC effects
	double airchange_per_hour;		///< house_e air changes per hour (ach)
	double airchange_UA;			///< additional UA due to air changes per hour
	double UA;						///< the total UA of the house
	double latent_load_fraction;	///< fractional increase in cooling load due to latent heat
	
	// current hvac properties
	double system_rated_power;		///< rated power of the system
	double system_rated_capacity;	///< rated capacity of the system
	complex hvac_power;				///< actual power draw of the hvac system (includes fan and motor where applicable)

	/* inherited res_enduse::load is hvac system load */
	double hvac_load;
	double total_load;
	enduse total; /* total load */
	double heating_demand;
	double cooling_demand;
	double last_heating_load; ///< stores the previous heater load for use in the controller
	double last_cooling_load; ///< stores the previous A/C load for use in the controller
	bool	compressor_on;
	int64	compressor_count;
	
	/* cycle tracking values */
	TIMESTAMP hvac_last_on;
	TIMESTAMP hvac_last_off;
	double hvac_period_on;
	double hvac_period_off;
	double hvac_period_length; // minutes
	double hvac_duty_cycle;

	/* Energy Storage Variable */
	double thermal_storage_present;		//Indication of if thermal storage is present and available for drawing
	double thermal_storage_inuse;		//Flag to indicate thermal storage is being pulled at the moment

	typedef enum {
		ST_GAS	= 0x00000001,	///< flag to indicate gas heating is used
		ST_AC	= 0x00000002,	///< flag to indicate the air-conditioning is used
		ST_AIR	= 0x00000004,	///< flag to indicate central air is used
		ST_VAR	= 0x00000008,	///< flag to indicate the variable speed system is used
		ST_RST	= 0x00000010,	///< flag to indicate that the heat is purely resistive
	} SYSTEMTYPE; ///< flags for system type options
	set system_type;///< system type
	/* obsolete? -MH */

	typedef enum  {
		AX_NONE = 0x0,
		AX_DEADBAND = 0x1,
		AX_TIMER = 0x2,
		AX_LOCKOUT = 0x4,
	} AUXSTRATEGY;
	set auxiliary_strategy;

	typedef enum{
		AT_UNKNOWN = 0,
		AT_NONE = 1,
		AT_ELECTRIC = 2,
	} AUXILIARYSYSTEMTYPE;
	enumeration auxiliary_system_type;

	typedef enum{
		HT_UNKNOWN = 0,
		HT_NONE = 1,
		HT_GAS = 2,
		HT_HEAT_PUMP = 3,
		HT_RESISTANCE = 4,
	} HEATSYSTEMTYPE;
	enumeration heating_system_type;

	typedef enum {
		CT_UNKNOWN = 0,
		CT_NONE = 1,
		CT_ELECTRIC = 2,
	} COOLSYSTEMTYPE;
	enumeration cooling_system_type;

	typedef enum {
		FT_UNKNOWN = 0,
		FT_NONE = 1,
		FT_ONE_SPEED = 2,
		FT_TWO_SPEED = 3,
	} FANTYPE;
	enumeration fan_type;

	typedef enum {
		GM_OTHER = 0,
		GM_GLASS = 1,
		GM_LOW_E_GLASS = 2,
	} GLASSTYPE;
	enumeration glass_type;

	typedef enum {
		WF_NONE = 0,
		WF_ALUMINUM = 1,
		WF_THERMAL_BREAK = 2,
		WF_WOOD = 3,
		WF_INSULATED = 4,
	} WINDOWFRAME;
	enumeration window_frame;

	typedef enum {
		GT_OTHER = 0,
		GT_CLEAR = 1,
		GT_ABS = 2,
		GT_REFL = 3,
		GT_LOW_S = 4,
		GT_HIGH_S = 5,
	} GLAZINGTREATMENT;
	enumeration glazing_treatment;

	typedef enum {
		GL_ONE=1,
		GL_TWO=2,
		GL_THREE=3,
		GL_OTHER,
	} GLAZINGLAYERS;
	enumeration glazing_layers;

	typedef enum {				///< Thermal integrity level is an "easy" to use
		TI_VERY_LITTLE  =0,		///< parameter, which gives reasonable defaults
		TI_LITTLE	    =1,		///< for R-values and air exchange rates to broadly
		TI_BELOW_NORMAL =2,		///< represent generic types of buildings, ranging
		TI_NORMAL       =3,		///< from extremely un-insulated to very insulated.
		TI_ABOVE_NORMAL =4,		///< Advanced user's who wish to specify the characteristics
		TI_GOOD			=5,		///< of the home should not use this variable.
		TI_VERY_GOOD	=6,
		TI_UNKNOWN		=7
	} THERMAL_INTEGRITY;
	enumeration thermal_integrity_level;

	typedef enum {
		MM_NONE		= 0,		///< Motor model describes the level of detail to be assigned
		MM_BASIC	= 1,		///< in calculating the efficiency of the hvac motor.  NONE will
		MM_FULL		= 2			///< indicate that the electrical motor is 100% electrically efficient.
	} MOTORMODEL;				///< BASIC incorporates shunt losses, while FULL also includes series losses.
	enumeration motor_model;

	// Motor variables and enumerations
	double hvac_motor_efficiency;	///< Actual percent electrical efficiency of hvac motor
	double hvac_motor_loss_power_factor; ///< Between this and efficiency, derives the real and reactive losses
	double hvac_motor_real_loss;		 ///< Actual real power loss of motor at rated voltage
	double hvac_motor_reactive_loss;	 ///< Actual reactive power loss of motor at rated voltage
	typedef enum {
		ME_VERY_POOR	= 0,	///< Gives an "easy" to use parameter for specifying the efficiency
		ME_POOR			= 1,	///< level of the motor when using motor_model BASIC or FULL
		ME_AVERAGE		= 2,
		ME_GOOD			= 3,
		ME_VERY_GOOD	= 4,
	} MOTOREFFICIENCY;
	enumeration motor_efficiency;

	typedef enum {	
		SM_UNKNOWN	=0,				///< unknown mode
		SM_OFF		=1,				///< off
		SM_HEAT		=2,				///< heating mode
		SM_AUX		=3,				///< supplemental heating
		SM_COOL		=4,				///< cooling mode
	} SYSTEMMODE;					///< system mode
	enumeration system_mode,			///< system mode at t1
		last_system_mode;
	int64 last_mode_timer;

	// derived/calculated variables
	double volume;					///< house_e air volume
	double air_mass;				///< mass of air (lbs)
	double air_thermal_mass;		///< thermal mass of air (BTU/F)
	double solar_load;				///< solar load (BTU/h) is set equal to Qs
	//Qs reads in data that is assumed to be in W/sf and
	//converts the data into Btu/hr.
	//please note that the product of 
	//gross_wall_area*window_wall_ratio*glazing_shgc*window_exterior_transmission_coefficient should be the
	//same as the equivalent solar aperature located in Rob Pratt's ETP Excel spreadsheet.
	double cooling_design_temperature;
	double heating_design_temperature;
	double design_peak_solar;
	double design_internal_gains;
	double design_internal_gain_density;

	double Rroof;
	double Rwall;
	double Rfloor;
	double Rwindows;
	double Rdoors;

	double *pTout;	// pointer to outdoor temperature (see climate)
	double *pRhout;	// pointer to outdoor humidity (see climate)
	double *pSolar;	// pointer to solar radiation array (see climate)
	double incident_solar_radiation;///< This variable hold the average incident solar radiation hitting the house in Btu/(hr*sf)

	double Tair;
	double Tmaterials;
	double outside_temperature;
	double outdoor_rh;

	double is_AUX_on,is_HEAT_on,is_COOL_on;	// A logic statement so that the population dynamics of the AC can be observed with collectors.

private:
	TIMESTAMP simulation_beginning_time;
	void set_thermal_integrity();
	void set_window_shgc();
	void set_window_Rvalue();
	// internal variables used to track state of house */
	double dTair;
	double a,b,c,d,c1,c2,A3,A4,k1,k2,r1,r2,Teq,Tevent,Qi,Qa,Qm,adj_cooling_cap,adj_heating_cap,adj_cooling_cop,adj_heating_cop;
	double Qlatent;
	static bool warn_control;
	static double warn_low_temp;
	static double warn_high_temp;
	static double system_dwell_time; // time interval at which hvac checks its state (approximates true dwell time)
	bool check_start;
	bool heat_start;

	complex load_values[3][3];	//Power, Current, and impedance (admittance) load accumulators for

public:
	int error_flag;
	static CLASS *oclass, *pclass;
	house_e( MODULE *module);
	~house_e();

	int create();
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_billing(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_thermostat(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_panel(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_enduses(TIMESTAMP t0, TIMESTAMP t1);
	void update_system(double dt=0);
	void update_model(double dt=0);
	void check_controls(void);
	void update_Tevent(void);

	int init(OBJECT *parent);
	int init_climate(void);
	int isa(char *classname);

	CIRCUIT *attach(OBJECT *obj, double limit, int is220=false, enduse *pEnduse=NULL);
	void attach_implicit_enduses(void);

// access methods
public:
	inline double get_floor_area () { return floor_area; };
	inline double get_Tout () { return *pTout; };
	inline double get_Tair () { return Tair; };

	complex *get_complex(OBJECT *obj, char *name);
	bool *get_bool(OBJECT *obj, char *name);
	int *get_enum(OBJECT *obj, char *name);
};

inline double sgn(double x) 
{
	if (x<0) return -1;
	if (x>0) return 1;
	return 0;
}

#endif

/**@}**/

 	  	 
