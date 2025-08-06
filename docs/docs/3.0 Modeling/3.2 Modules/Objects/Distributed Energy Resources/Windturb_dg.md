# Windturb dg

**Source URL:** https://gridlab-d.shoutwiki.com/wiki/Windturb_dg
# Windturb dg

## Contents

  * 1 Synopsis
  * 2 Power Curve Based Implementation
  * 3 Coefficient of Performance Based Implementation
  * 4 Properties
  * 5 Defaults
  * 6 Example
## Synopsis
    
    
    module generators;
    class windturb_dg
    {
    	set phases;	/**< device phases (see PHASE codes) */
    
    	complex power_A;//power
    	complex power_B;
    	complex power_C;
    
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
    
    	double q;					//number of gearboxes
    
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
    	
    	char power_curve_csv[1024]; // name of csv file containing the power curve
    	bool power_curve_pu;		// Flag when set indicates that user provided power curve has power values in pu. Defaults to false in .cpp
    };
    

## Power Curve Based Implementation

Note that the power curve-based implementation is not included in the v4.2. It is planned to be released in v4.3. 

The power curve-based wind turbine implementation uses the power curves that translate the wind speed directly into output power. Power curves are often available from manufacturers through marketing materials and/or owner documentation. Their use simplifies the wind turbine modeling since they skip the internal details of the wind turbine and instead focus on the input/output characteristics. The implementation uses a default power curve or one of the power curves provided by the user. Examples of commercial and generic power curves can be found at: <https://github.com/NREL/turbine-models>

The power curve-based implementation replaces the coefficient of performance-based implementation. In future versions starting from v4.3, it will be the default implementation. 

## Coefficient of Performance Based Implementation

The coefficient of performance-based implementation was the original wind turbine implementation. It includes an explicit model of the wind turbine and the electrical machine/generator parameters. The implementation contains highly granular models of the synchronous and induction generators with their respective impedances. It uses the wind turbine coefficient of performance data to generate the output for a given wind speed input. The coefficient of performance is defined as the ratio of the power captured by the rotor of the wind turbine divided by the total power available in the wind just before it interacts with the turbine. 

This model remains in the experimental level of development. 

## Properties

Property Name  | Type  | Unit  | Description   
---|---|---|---  
air_density  |  double  |  kg/m^3  |  Estimated air density. Used in COEFF_OF_PERFORMANCE implementation.   
avg_ws  |  double  |  m/s  |  Default value for wind speed   
blade_diam  |  double  |  meters  |  Specifies the diameter of the blades. Used in COEFF_OF_PERFORMANCE implementation.   
current_A  |  complex  |  A  |  Terminal current on phase A   
current_B  |  complex  |  A  |  Terminal current on phase B   
current_C  |  complex  |  A  |  Terminal current on phase C   
cut_in_ws  |  double  |  m/sec  |  Minimum wind speed for generator operation. Used in COEFF_OF_PERFORMANCE implementation.   
cut_out_ws  |  double  |  m/sec  |  Maximum wind speed for generator operation. Used in COEFF_OF_PERFORMANCE implementation.   
Cp_max  |  double  |  p.u.  |  Maximum coefficient of performance. Used in COEFF_OF_PERFORMANCE implementation.   
Cp_rated  |  double  |  p.u.  |  Rated coefficient of performance. Used in COEFF_OF_PERFORMANCE implementation.   
Cp  |  double  |  p.u.  |  Calculated coefficient of performance. Used in COEFF_OF_PERFORMANCE implementation.   
CP_Data  |  enumeration  |  N/A  |  Data set for Coefficient of performance. Used in COEFF_OF_PERFORMANCE implementation. Default is CALCULATED. 

  * GENERAL_LARGE
  * GENERAL_MID
  * GENERAL_SMALL
  * MANUF_TABLE
  * CALCULATED
  * USER_SPECIFY

  
EfA  |  complex  |  V  |  Synchronous Generator induced voltage on phase A in V   
EfB  |  complex  |  V  |  Synchronous Generator induced voltage on phase B in V   
EfC  |  complex  |  V  |  Synchronous Generator induced voltage on phase C in V   
Gen_mode  |  enumeration  |  N/A  |  Control mode that is used for the generator output. Used in COEFF_OF_PERFORMANCE implementation. Default is CONSTANTP. 

  * CONSTANTE
  * CONSTANTP
  * CONSTANTPQ

  
Gen_status  |  enumeration  |  N/A  |  Allows a user to dropout a generator. Default is ONLINE. 

  * OFFLINE
  * ONLINE

  
Gen_type  |  enumeration  |  N/A  |  Allows the user to specify the type of generator. Used in COEFF_OF_PERFORMANCE based implementation. 

  * INDUCTION
  * SYNCHRONOUS

  
GenElecEff  |  double  |  N/A  |  Generator electrical efficiency used for testing   
Irotor_A  |  complex  |  p.u.  |  Induction Generator "rotor" current generated in pu on phase A   
Irotor_B  |  complex  |  p.u.  |  Induction Generator "rotor" current generated in pu on phase B   
Irotor_C  |  complex  |  p.u.  |  Induction Generator "rotor" current generated in pu on phase C   
Max_Vrotor  |  double  |  (p.u.)*V  |  Induction generator maximum induced rotor voltage in p.u., e.g. 1.2. Used in COEFF_OF_PERFORMANCE based implementation.   
Min_Vrotor  |  double  |  (p.u.)*V  |  Induction generator minimum induced rotor voltage in p.u., e.g. 0.8. Used in COEFF_OF_PERFORMANCE based implementation.   
Max_Ef  |  double  |  (p.u.)*V  |  Synchronous generator maximum induced rotor voltage in p.u., e.g. 1.2. Used in COEFF_OF_PERFORMANCE based implementation.   
Min_Ef  |  double  |  (p.u.)*V  |  Synchronous generator minimum induced rotor voltage in p.u., e.g. 0.8. Used in COEFF_OF_PERFORMANCE based implementation.   
Max_P  |  double  |  kW  |  maximum real power capacity in kW   
Min_P  |  double  |  kW  |  minimum real power capacity in kW   
Max_Q  |  double  |  kVar  |  maximum reactive power capacity in kVar   
Min_Q  |  double  |  kVar  |  minimum reactive power capacity in kVar   
power_curve_csv  |  string  |  N/A  |  Specifies the name of .csv file containing user defined power curve   
power_curve_pu  |  Boolean  |  N/A  |  A Boolean when set to TRUE indicates that the user provided power curve has power values in p.u. Default is FALSE.   
pf  |  double  |  p.u.  |  Desired power factor in CONSTANTP mode. Used in COEFF_OF_PERFORMANCE based implementation.   
phases  |  set  |  N/A  |  Specifies which phases to connect to. Triplex mode is only supported for the POWER_CURVE implementation. 

  * ABCN
  * AS
  * BS
  * CS

  
Pconv  |  double  |  kW  |  Power converted from mechanical to electrical before electrical losses   
power_A  |  complex  |  kVA  |  Complex power on phase A   
power_B  |  complex  |  kVA  |  Complex power on phase B   
power_C  |  complex  |  kVA  |  Complex power on phase C   
q  |  double  |  N/A  |  Number of gearboxes. Used in COEFF_OF_PERFORMANCE based implementation.   
Rst  |  double  |  (p.u.)*Ohm  |  Induction generator primary stator resistance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Rr  |  double  |  (p.u.)*Ohm  |  Induction generator primary rotor resistance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Rc  |  double  |  (p.u.)*Ohm  |  Induction generator primary core resistance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Rated_V  |  double  |  V  |  Rated generator terminal voltage. Used in COEFF_OF_PERFORMANCE based implementation.   
Rated_VA  |  double  |  VA  |  Rated wind turbine generator power output. Default is 100 kW.   
roughness_l  |  double  |  N/A  |  European Wind Atlas unitless correction factor for adjusting wind speed at various heights above ground and terrain types, default=0.055.   
Rs  |  double  |  (p.u.)*Ohm  |  Synchronous generator primary stator resistance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Rg  |  double  |  (p.u.)*Ohm  |  Synchronous generator grounding resistance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
ref_height  |  double  |  m  |  height wind data was measured. Default is 10 m.   
turbine_height  |  double  |  meters  |  Specifies the height of the wind turbine hub above the ground. Default is 37 m.   
Turbine_Model  |  enumeration  |  N/A  |  Allows the use of one of the pre-defined generic turbines. Default is GENERIC_DEFAULT. 

  * GENERIC_DEFAULT
  * GENERIC_SYNCH_SMALL
  * GENERIC_SYNCH_MID
  * GENERIC_SYNCH_LARGE
  * GENERIC_IND_SMALL
  * GENERIC_IND_MID
  * GENERIC_IND_LARGE
  * VESTAS_V82
  * GE_25MW
  * BERGEY_10kW
  * GEN_TURB_POW_CURVE_2_4KW
  * GEN_TURB_POW_CURVE_10KW
  * GEN_TURB_POW_CURVE_100KW
  * GEN_TURB_POW_CURVE_1_5MW

  
Turbine_implementation  |  enumeration  |  N/A  |  Allows the user to specify the type of implementation for the wind turbine model. Default is POWER_CURVE. 

  * POWER_CURVE
  * COEFF_OF_PERFORMANCE

  
TotalRealPow  |  double  |  kW  |  Total Real power supplied by the generator   
TotalReacPow  |  double  |  kVar  |  Total Reactive power supplied by the generator   
time_advance  |  int  |  sec  |  Amount of time to advance for wind speed data import   
voltage_A  |  complex  |  V  |  Synchronous Generator Terminal voltage on phase A   
voltage_B  |  complex  |  V  |  Synchronous Generator Terminal voltage on phase B   
voltage_C  |  complex  |  V  |  Synchronous Generator Terminal voltage on phase C   
Vrotor_A  |  complex  |  p.u.  |  Induction Generator induced "rotor" voltage in pu on phase A   
Vrotor_B  |  complex  |  p.u.  |  Induction Generator induced "rotor" voltage in pu on phase B   
Vrotor_C  |  complex  |  p.u.  |  Induction Generator induced "rotor" voltage in pu on phase C   
WSadj  |  double  |  m/sec  |  Speed of wind at hub height.   
Wind_Speed  |  double  |  m/sec  |  Wind speed at 5-15m level (typical measurement height).   
ws_rated  |  double  |  m/sec  |  Rated wind speed for generator operation. Used in COEFF_OF_PERFORMANCE implementation.   
ws_maxcp  |  double  |  m/sec  |  Wind speed at which generator reaches maximum Cp. Used in COEFF_OF_PERFORMANCE implementation.   
Xst  |  double  |  (p.u.)*Ohm  |  Induction generator primary stator reactance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Xr  |  double  |  (p.u.)*Ohm  |  Induction generator primary rotor reactance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Xm  |  double  |  (p.u.)*Ohm  |  Induction generator primary core reactance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Xs  |  double  |  (p.u.)*Ohm  |  Synchronous generator primary stator reactance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
Xg  |  double  |  (p.u.)*Ohm  |  Synchronous generator grounding reactance in p.u. Used in COEFF_OF_PERFORMANCE based implementation.   
  
## Defaults

Default Parameter Values  Parameter | Default value   
---|---  
avg_ws | 8 m/s   
blade_diam | 22 m   
q | 3   
Max_P | 90000 kW   
Max_Q | 40000 kVar   
Rated_V | 600 V   
pf | 0.9   
CP_Data | GENERAL_MID   
cut_in_ws | 3.5 m/s   
cut_out_ws | 25 m/s   
Cp_max | 0.302   
ws_maxcp | 7 m/s   
Cp_rated | Cp_max-.05   
ws_rated | 12.5 m/s   
Gen_type | SYNCHRONOUS   
Rs | 0.05   
Xs | 0.2   
Rg | 0   
Xg | 0   
Gen_mode | CONSTANTP   
Gen_status | ONLINE   
power_curve_pu | FALSE   
Rated_VA | 100 kVA   
roughness_l | 0.055   
ref_height | 10 m   
turbine_height | 37 m   
Turbine_Model | GENERIC_DEFAULT   
Turbine_implementation | POWER_CURVE   
  
## Example

A minimal model could be created via: 
    
    
    object windturb_dg {
        parent my_meter1;
        phases ABCN;
        name windturb1;
        Rated_VA 10000;
        turbine_height 40;
    }
    

or using one of the generic turbines: 
    
    
    object windturb_dg {
        parent my_meter1;
        phases ABCN;
        name windturb1;
        Turbine_Model GEN_TURB_POW_CURVE_1_5MW;
    }
    


  
