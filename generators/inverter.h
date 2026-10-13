/** $Id: inverter.h,v 1.0 2008/07/16
    Copyright (C) 2008 Battelle Memorial Institute
    @file inverter.h
    @addtogroup inverter

 @{
 **/

#ifndef _inverter_H
#define _inverter_H

#include <utility> //for pair<> in Volt/VAr schedule
#include <vector>  //for list<> in Volt/VAr schedule
#include <string>  //Ab add
#include <stdarg.h>

#include "generators.h"

EXPORT STATUS preupdate_inverter(OBJECT *obj,TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_inverter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_inverter(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);
EXPORT STATUS inverter_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure);

//Alternative PI version Dynamic control Inverter state variable structure
typedef struct {
	double P_Out[3];		///< The real power output
	double Q_Out[3];		///< The reactive power output
	double ed[3];			///< The error in real power output
	double eq[3];			///< The error in reactive power output
	double ded[3];		///< The change in real power error
	double deq[3];		///< The change in reactive power error
	double md[3];			///< The d axis current modulator of the inverter
	double mq[3];			///< The q axis current modulator of the inverter
	double dmd[3];			///< The change in d axis current modulator of the inverter
	double dmq[3];			///< The change in q axis current modulator of the inverter
	gld::complex Idq[3];			///< The dq axis current output of the inverter
	gld::complex Iac[3];	///< The AC current out of the inverter terminals

	// Terminal voltage state variable for VSI isochronous mode
	double V_StateVal[3];	// Magnitude of the VSI terminal voltage
	double dV_StateVal[3];	// Change in magnitude of the VSI terminal voltage
	double e_source_mag[3];	// VSI e_source magnitude

	// Frequency state variable for f/p droop
	double f_mea_delayed; // Delay measured frequency value seen by the inverter
	double df_mea_delayed; // The change of the frequency

	// Voltage state variable for v/q droop
	double V_mea_delayed[3]; // Delay measured terminal voltage values seen by the inverter
	double dV_mea_delayed[3]; // The change of the terminal voltage values

	// Real power state variable for f/p drrop in VSI inverter
	double p_mea_delayed;
	double dp_mea_delayed;

	// Reactive power state variable for f/p drrop in VSI inverter
	double q_mea_delayed;
	double dq_mea_delayed;

	// Pmax controller stuff
    double fmax_ini_StateVal;
    double dfmax_ini_StateVal;
    double fmax_StateVal;

	// Pmin controller stuff
    double fmin_ini_StateVal;
    double dfmin_ini_StateVal;
    double fmin_StateVal;

} INV_STATE;

// Simple PID controller
typedef struct
{
    gld::complex error[3];
    gld::complex mod_vals[3];
    gld::complex integrator_vals[3];
    gld::complex derror[3];
    gld::complex current_vals_ref[3]; // Reference-framed current values
    gld::complex current_vals[3];     // Unrotated "powerflow-postable" current values
    gld::complex current_set_raw[3];  // Actual current value
    gld::complex current_set[3];      // Current rotated to common reference frame
    double reference_angle[3];        // Reference angle tracking
    double max_error_val;
    double phase_Pref;
    double phase_Qref;
    double I_in;
} PID_INV_VARS;

// Inverter class
class inverter : public gld_object
{
private:
    char first_iter_counter;

protected:
    /* TODO: put unpublished but inherited variables */
public:
    // Comaptibility variables - used to be in power_electronics
    int32 number_of_phases_out; // Count for number of phases

    // General status variables
    gld::set phases; /**< device phases (see PHASE codes) */
    enum GENERATOR_MODE
    {
        CONSTANT_V = 1,
        CONSTANT_PQ = 2,
        CONSTANT_PF = 4,
        SUPPLY_DRIVEN = 5
    };
    enumeration gen_mode_v; // operating mode of the generator

    enum INVERTER_TYPE
    {
        TWO_PULSE = 0,
        SIX_PULSE = 1,
        TWELVE_PULSE = 2,
        PWM = 3,
        FOUR_QUADRANT = 4
    };
    enumeration inverter_type_v;
    enum GENERATOR_STATUS
    {
        OFFLINE = 1,
        ONLINE = 2
    };
    enumeration gen_status_v;

    double V_In; // V_in (DC)
    double Vdc;
    double I_In; // I_in (DC)
    double P_In; // power in (DC)

    double efficiency;

    enum PF_REG
    {
        INCLUDED = 1,
        EXCLUDED = 2,
        INCLUDED_ALT = 3
    } pf_reg;
    enum PF_REG_STATUS
    {
        REGULATING = 1,
        IDLING = 2
    } pf_reg_status;
    enum LOAD_FOLLOW_STATUS
    {
        IDLE = 0,
        DISCHARGE = 1,
        CHARGE = 2
    } load_follow_status; // Status variable for what the load_following mode is doing

    gld::complex VA_Out;
    gld::complex VA_Out_past;
    double P_Out; // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
    double Q_Out;
    double p_in;        // the power into the inverter from the DC side
    double b_soc;       // the SOC of the battery
    double soc_reserve; // the battery reserve in pu
    double p_rated;     // the power rating of the inverter per phase
    double bp_rated;    // the power rating on the DC side
    double f_nominal;
    double inv_eta; // the inverter's efficiency
    double V_Set_A;
    double V_Set_B;
    double V_Set_C;
    double margin;
    gld::complex I_step_max;
    double power_factor;
    double P_Out_t0;
    double Q_Out_t0;
    double power_factor_t0;

    // Hidden variables for wind turbine checks
    bool WT_is_connected;

    gld::complex phaseA_I_Out_prev; // current
    gld::complex phaseB_I_Out_prev;
    gld::complex phaseC_I_Out_prev;

    gld::complex phaseA_V_Out; // voltage
    gld::complex phaseB_V_Out;
    gld::complex phaseC_V_Out;
    gld::complex phaseA_I_Out; // current
    gld::complex phaseB_I_Out;
    gld::complex phaseC_I_Out;
    gld::complex power_val[3];    // power
    gld::complex last_current[4]; // Previously applied power output (used to remove from parent so XML files look proper)
    gld::complex last_power[4];   // Previously applied power output (as constant power) - used to remove from parent so XML looks right

    // properties for multipoint efficiency model. The model used is from Sandia National Laboratory's 2007 paper "Performance Model For Grid-Connected Photovoltaic Inverters".
    bool use_multipoint_efficiency;
    double p_max;
    double p_dco; // The dc power at which maximum ac power is reached.(W)
    double v_dco; // The dc voltage at which maximum ac power is reached.(V)
    double p_so;  // The minumum dc power for inversion.(W)
    double c_o;   // The parameter describing the curvature of the relationship between ac power and dc power at the reference operating condition. default is 0.
    double c_1;   // Coefficient allowing p_dco to vary linearly with dc voltage. default is 0.
    double c_2;   // Coefficient allowing p_so to vary linearly with dc voltage. default is 0.
    double c_3;   // Coefficient allowing c_o to vary linearly with dc voltage. default is 0.
    double C1;
    double C2;
    double C3;
    enum INVERTER_MANUFACTURER
    {
        NONE = 0,
        FRONIUS = 1,
        SMA = 2,
        XANTREX = 3
    };
    enumeration inverter_manufacturer; // known manufacturer to set some presets else use variables themselves for custom inverter.

    // properties for four quadrant control modes
    enum FOUR_QUADRANT_CONTROL_MODE
    {
        FQM_NONE = 0,
        FQM_CONSTANT_PQ = 1,
        FQM_CONSTANT_PF = 2,
        FQM_CONSTANT_V = 3,
        FQM_VOLT_VAR = 4,
        FQM_LOAD_FOLLOWING = 5,
        FQM_GENERIC_DROOP = 6,
        FQM_GROUP_LF = 7,
        FQM_VOLT_VAR_FREQ_PWR = 8,
        FQM_VOLT_WATT = 9
    };
    enumeration four_quadrant_control_mode;

    double excess_input_power; // Variable tracking excess power on the input that is not placed to the output

    // properties for four quadrant load following control mode
    OBJECT *sense_object;           // Object to sense the power level from and provide adjustments toward
    double max_charge_rate;         // Maximum rate of charge for recharging the battery
    double max_discharge_rate;      // Maximum discharge rate for extracting energy from the battery
    double charge_on_threshold;     // Threshold to enable charging of the battery
    double charge_off_threshold;    // Threshold to disable charging of the battery
    double discharge_on_threshold;  // Threshold to enable discharging of the battery
    double discharge_off_threshold; // Threshold to disable discharging of the battery
    double charge_lockout_time;     // Time a charge operation is held before another dispatch operation is allowed
    double discharge_lockout_time;  // Time a discharge operation is held before another dispatch operation is allowed

    // properties for pf regulation mode - first control method
    double pf_reg_activate;   // Lowest acceptable power-factor level below which power-factor regulation will activate.
    double pf_reg_deactivate; // Lowest acceptable power-factor above which no power-factor regulation is needed.

    // properties for pf regulation mode - second control method
    double pf_target_var; // Desired power-factor to maintain (signed)
    double pf_reg_high;   // Upper limit for power factor regulation - if above (other side), go full opposite
    double pf_reg_low;    // Lower limit for power factor regulation - if above, just leave it be (don't regulator)

    double pf_reg_activate_lockout_time; // Mandatory pause between the deactivation of power-factor regulation and it reactivation

    // Properties for group load-following
    double charge_threshold;         // Level at which all inverters in the group will begin charging attached batteries. Regulated minimum load level.
    double discharge_threshold;      // Level at which all inverters in the group will begin discharging attached batteries. Regulated maximum load level.
    double group_max_charge_rate;    // Sum of the charge rates of the inverters involved in the group load-following.
    double group_max_discharge_rate; // Sum of the discharge rates of the inverters involved in the group load-following.
    double group_rated_power;        // Sum of the inverter power ratings of the inverters involved in the group power-factor regulation.

    double inverter_convergence_criterion; // The convergence criteria for the dynamic inverter to exit deltamode

    TIMESTAMP inverter_start_time;
    bool inverter_first_step;
    int64 first_iteration_current_injection; // Initialization variable - mostly so SWING_PQ buses initalize properly for deltamode

    // VoltVar Control Parameters
    double V_base;
    double V1;
    double V2;
    double V3;
    double V4;
    double Q1;
    double Q2;
    double Q3;
    double Q4;
    double m12;
    double b12;
    double m23;
    double b23;
    double m34;
    double b34;
    double getVar(double volt, double m, double b);
    double vv_lockout;
    TIMESTAMP allowed_vv_action;
    TIMESTAMP last_vv_check;
    bool vv_operation;

    // VoltWatt Control parameters
    double VW_V1;
    double VW_V2;
    double VW_P1;
    double VW_P2;

    // 1547 variables
    bool enable_1547_compliance; // Flag to enable IEEE 1547-2003 condition checking
    double reconnect_time;       // Time after a 1547 violation clears before we reconnect
    bool inverter_1547_status;   // Flag to indicate if we are online, or "curtailed" due to 1547 mapping

    enum IEEE_1547_STATUS
    {
        IEEE_NONE = 0,
        IEEE1547 = 1,
        IEEE1547A = 2
    };
    enumeration ieee_1547_version;

    // 1547(a) frequency
    double over_freq_high_band_setpoint;   // OF2 set point for IEEE 1547a
    double over_freq_high_band_delay;      // OF2 clearing time for IEEE1547a
    double over_freq_high_band_viol_time;  // OF2 violation accumulator
    double over_freq_low_band_setpoint;    // OF1 set point for IEEE 1547a
    double over_freq_low_band_delay;       // OF1 clearing time for IEEE 1547a
    double over_freq_low_band_viol_time;   // OF1 violation accumulator
    double under_freq_high_band_setpoint;  // UF2 set point for IEEE 1547a
    double under_freq_high_band_delay;     // UF2 clearing time for IEEE1547a
    double under_freq_high_band_viol_time; // UF2 violation accumulator
    double under_freq_low_band_setpoint;   // UF1 set point for IEEE 1547a
    double under_freq_low_band_delay;      // UF1 clearing time for IEEE 1547a
    double under_freq_low_band_viol_time;  // UF1 violation accumulator

    // 1547 voltage(a) voltage
    double under_voltage_lowest_voltage_setpoint; // Lowest voltage threshold for undervoltage
    double under_voltage_middle_voltage_setpoint; // Middle-lowest voltage threshold for undervoltage
    double under_voltage_high_voltage_setpoint;   // High value of low voltage threshold for undervoltage
    double over_voltage_low_setpoint;             // Lowest voltage value for overvoltage
    double over_voltage_high_setpoint;            // High voltage value for overvoltage
    double under_voltage_lowest_delay;            // Lowest voltage clearing time for undervoltage
    double under_voltage_middle_delay;            // Middle-lowest voltage clearing time for undervoltage
    double under_voltage_high_delay;              // Highest voltage clearing time for undervoltage
    double over_voltage_low_delay;                // Lowest voltage clearing time for overvoltage
    double over_voltage_high_delay;               // Highest voltage clearing time for overvoltage
    double under_voltage_lowest_viol_time;        // Lowest low voltage threshold violation accumulator
    double under_voltage_middle_viol_time;        // Middle low voltage threshold violation accumulator
    double under_voltage_high_viol_time;          // Highest low voltage threshold violation accumulator
    double over_voltage_low_viol_time;            // Lowest high voltage threshold violation accumulator
    double over_voltage_high_viol_time;           // Highest high voltage threshold violation accumulator

    enum
    {
        IEEE_1547_NONE = 0,      /**< No trip reason */
        IEEE_1547_HIGH_OF = 1,   /**< High over-frequency level trip */
        IEEE_1547_LOW_OF = 2,    /**< Low over-frequency level trip */
        IEEE_1547_HIGH_UF = 3,   /**< High under-frequency level trip */
        IEEE_1547_LOW_UF = 4,    /**< Low under-frequency level trip */
        IEEE_1547_LOWEST_UV = 5, /**< Lowest under-voltage level trip */
        IEEE_1547_MIDDLE_UV = 6, /**< Middle under-voltage level trip */
        IEEE_1547_HIGH_UV = 7,   /**< High under-voltage level trip */
        IEEE_1547_LOW_OV = 8,    /**< Low over-voltage level trip */
        IEEE_1547_HIGH_OV = 9    /**< High over-voltage level trip */
    };

    enumeration ieee_1547_trip_method;

    // properties for four quadrant volt/var frequency power mode
    bool disable_volt_var_if_no_input_power;              // if true turn off Volt/VAr behavior when no input power (i.e. at night for a solar system)
    double delay_time;                                    // delay time time between seeing a voltage value and responding with appropiate VAr setting (seconds)
    double max_var_slew_rate;                             // maximum rate at which inverter can change its VAr output (VAr/second) *not sure if this is defined anywhere else i.e. power electronics classes
    double max_pwr_slew_rate;                             // maximum rate at which inverter can change its power output for frequency regulation (W/second) *not sure if this is defined anywhere else i.e. power electronics classes
    char volt_var_sched[1024];                            // user input Volt/VAr Schedule
    char freq_pwr_sched[1024];                            // user input freq-power Schedule
    std::vector<std::pair<double, double>> VoltVArSched;  // Volt/VAr schedule -- i realize I'm using goofball data types, what would be the GridLABD-esque way of implementing this data type?
    std::vector<std::pair<double, double>> freq_pwrSched; // freq-power schedule -- i realize I'm using goofball data types, what would be the GridLABD-esque way of implementing this data type?
private:
    // Comaptibility variables - used to be in power_electronics
    bool parent_is_a_meter;        // Boolean to indicate if the parent object is a meter/triplex_meter
    bool parent_is_triplex;        // Boolean to indicate if the parent object is triplex-oriented (for variable exchange)
    enumeration attached_bus_type; // Determines attached bus type - mostly for VSI and grid-forming functionality

    FUNCTIONADDR swing_test_fxn; // Function to map to swing testing function, if needed

    gld_property *pCircuit_V[3]; ///< pointer to the three L-N voltage fields
    gld_property *pLine_I[3];    ///< pointer to the three current fields
    gld_property *pPower[3];     ///< pointer to power value on meter parent
    gld_property *pLine12;       //< used in triplex metering
    gld_property *pPower12;      //< used in triplex metering
    gld_property *pMeterStatus;  ///< Pointer to service_status variable on meter parent

    // Default or "connecting point" values for powerflow interactions
    gld::complex value_Circuit_V[3]; ///< value holeder for the three L-N voltage fields
    gld::complex value_Line_I[3];    ///< value holeder for the three current fields
    gld::complex value_Power[3];     ///< value holeder for power value on meter parent
    gld::complex value_Line12;       //< value holder for triplex L-L variable
    gld::complex value_Power12;      //< value holder for triplex L-L variable
    enumeration value_MeterStatus;   ///< value holder for service_status variable on meter parent
    gld::complex value_Meter_I[3];   ///< value holder for meter measured current on three lines

    double Max_P;     //< maximum real power capacity in kW
    double Max_Q;     //< maximum reactive power capacity in kVar
    double Rated_kVA; //< nominal capacity in kVA
    double Rated_kV;  //< nominal line-line voltage in kV

    // load following variables
    FUNCTIONADDR powerCalc;          // Address for power_calculate in link object, if it is a link
    bool sense_is_link;              // Boolean flag for if the sense object is a link or a node
    gld_property *sense_power;       // Link to measured power value fo sense_object
    double lf_dispatch_power;        // Amount of real power to try and dispatch to meet thresholds
    TIMESTAMP next_update_time;      // TIMESTAMP of next dispatching change allowed
    bool lf_dispatch_change_allowed; // Flag to indicate if a change in dispatch is allowed

    // pf regulation variables
    bool pf_reg_dispatch_change_allowed; // Flag to indicate if a change in dispatch is allowed for power factor regulation
    double pf_reg_dispatch_VAR;          //(Reactive only?) power dispatched to meet power factor regulation threshold.
    TIMESTAMP pf_reg_next_update_time;   // TIMESTAMP of next dispatching change allowed

    TIMESTAMP prev_time;  // Tracking variable for previous "new time" run
    double prev_time_dbl; // Tracking variable for 1547 checks and ramp rates

    gld::complex last_I_Out[3];
    gld::complex I_Out[3];
    double last_I_In;

    // Volt-Watt variables
    double pa_vw_limited;
    double pb_vw_limited;
    double pc_vw_limited;
    double VW_m;

    // 1547 variables
    double out_of_violation_time_total; // Tracking variable to see how long we've been "outside of bad conditions" to re-enable the inverter
    gld_property *pFrequency;           // Pointer to frequency value for checking 1547 compliance
    double value_Frequency;             // Value storage for current frequency value
    double node_nominal_voltage;        // Nominal voltage for per-unit-izing for 1547 checks
    double ieee_1547_double;            // Deltamode tracker - made global for "off-cycle" checks

    void update_control_references(void);
    STATUS initalize_IEEE_1547_checks(OBJECT *parent);

    // Map functions
    void pull_complex_powerflow_values(void);
    void reset_complex_powerflow_accumulators(void);
    void push_complex_powerflow_values(void);

    double lin_eq_volt(double volt, double m, double b);

public:
    /* required implementations */
    inverter(MODULE *module);
    int isa(char *classname);
    int create(void);
    int init(OBJECT *parent);
    TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
    TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
    TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
    double perform_1547_checks(double timestepvalue);
    gld::complex check_VA_Out(gld::complex temp_VA, double p_max);
    double getEff(double val);

public:
    static CLASS *oclass;
    static inverter *defaults;
    static CLASS *plcass;
    gld::complex complex_exp(double angle);
#ifdef OPTIONAL
    static CLASS *pclass;                      /**< defines the parent class */
    TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
