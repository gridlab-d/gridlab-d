/*
 * metrics_collector.h
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 */

#ifndef _METRICS_COLLECTOR_H_
#define _METRICS_COLLECTOR_H_

#include "tape.h"
#include "../powerflow/link.h"

#define ALL_MTR_METRICS

#define MTR_MIN_REAL_POWER 0
#define MTR_MAX_REAL_POWER 1
#define MTR_AVG_REAL_POWER 2
#define MTR_MIN_REAC_POWER 3
#define MTR_MAX_REAC_POWER 4
#define MTR_AVG_REAC_POWER 5
#define MTR_REAL_ENERGY    6
#define MTR_REAC_ENERGY    7
#define MTR_BILL           8
#define MTR_MIN_VLL        9
#define MTR_MAX_VLL       10
#define MTR_AVG_VLL       11
#define MTR_ABOVE_A_DUR   12
#define MTR_BELOW_A_DUR   13
#define MTR_ABOVE_B_DUR   14
#define MTR_BELOW_B_DUR   15
#ifdef ALL_MTR_METRICS
#define MTR_MIN_VLN       16
#define MTR_MAX_VLN       17
#define MTR_AVG_VLN       18
#define MTR_MIN_VUNB      19
#define MTR_MAX_VUNB      20
#define MTR_AVG_VUNB      21
#define MTR_ABOVE_A_CNT   22
#define MTR_BELOW_A_CNT   23
#define MTR_ABOVE_B_CNT   24
#define MTR_BELOW_B_CNT   25
#define MTR_BELOW_10_DUR  26
#define MTR_BELOW_10_CNT  27
#define MTR_ARRAY_SIZE    28
#else
#define MTR_ARRAY_SIZE    16
#endif

#define HSE_MIN_TOTAL_LOAD   0
#define HSE_MAX_TOTAL_LOAD   1
#define HSE_AVG_TOTAL_LOAD   2
#define HSE_MIN_HVAC_LOAD    3
#define HSE_MAX_HVAC_LOAD    4
#define HSE_AVG_HVAC_LOAD    5
#define HSE_MIN_AIR_TEMP     6
#define HSE_MAX_AIR_TEMP     7
#define HSE_AVG_AIR_TEMP     8
#define HSE_AVG_DEV_COOLING  9
#define HSE_AVG_DEV_HEATING 10
#define HSE_SYSTEM_MODE     11
#define HSE_ARRAY_SIZE      12

#define WH_MIN_ACTUAL_LOAD  0
#define WH_MAX_ACTUAL_LOAD  1
#define WH_AVG_ACTUAL_LOAD  2
#define WH_MIN_SETPOINT     3
#define WH_MAX_SETPOINT     4
#define WH_AVG_SETPOINT     5
#define WH_MIN_DEMAND       6
#define WH_MAX_DEMAND       7
#define WH_AVG_DEMAND       8
#define WH_MIN_TEMP         9
#define WH_MAX_TEMP        10
#define WH_AVG_TEMP        11

#define WH_MIN_L_SETPOINT  12
#define WH_MAX_L_SETPOINT  13
#define WH_AVG_L_SETPOINT  14
#define WH_MIN_U_SETPOINT  15
#define WH_MAX_U_SETPOINT  16
#define WH_AVG_U_SETPOINT  17
#define WH_MIN_L_TEMP      18
#define WH_MAX_L_TEMP      19
#define WH_AVG_L_TEMP      20
#define WH_MIN_U_TEMP      21
#define WH_MAX_U_TEMP      22
#define WH_AVG_U_TEMP      23
#define WH_ELEM_L_MODE     24
#define WH_ELEM_U_MODE     25
#define WH_ARRAY_SIZE      26

#define INV_MIN_REAL_POWER 0
#define INV_MAX_REAL_POWER 1
#define INV_AVG_REAL_POWER 2
#define INV_MIN_REAC_POWER 3
#define INV_MAX_REAC_POWER 4
#define INV_AVG_REAC_POWER 5
#define INV_ARRAY_SIZE     6

#define FDR_MIN_REAL_POWER  0
#define FDR_MAX_REAL_POWER  1
#define FDR_AVG_REAL_POWER  2
#define FDR_MED_REAL_POWER  3
#define FDR_MIN_REAC_POWER  4
#define FDR_MAX_REAC_POWER  5
#define FDR_AVG_REAC_POWER  6
#define FDR_MED_REAC_POWER  7
#define FDR_REAL_ENERGY     8
#define FDR_REAC_ENERGY     9
#define FDR_MIN_REAL_LOSS  10
#define FDR_MAX_REAL_LOSS  11
#define FDR_AVG_REAL_LOSS  12
#define FDR_MED_REAL_LOSS  13
#define FDR_MIN_REAC_LOSS  14
#define FDR_MAX_REAC_LOSS  15
#define FDR_AVG_REAC_LOSS  16
#define FDR_MED_REAC_LOSS  17
#define FDR_ARRAY_SIZE     18

#define CAP_OPERATION_CNT   0
#define CAP_ARRAY_SIZE      1

#define REG_OPERATION_CNT   0
#define REG_ARRAY_SIZE      1

#define TRANS_OVERLOAD_PERC       0
#define TRANS_OVERLOAD_ARRAY_SIZE 1

#define LINE_OVERLOAD_PERC       0
#define LINE_OVERLOAD_ARRAY_SIZE 1

#define EV_MIN_CHARGE_RATE     0
#define EV_MAX_CHARGE_RATE     1
#define EV_AVG_CHARGE_RATE     2
#define EV_MIN_BATTERY_SOC     3
#define EV_MAX_BATTERY_SOC     4
#define EV_AVG_BATTERY_SOC     5
#define EVCHARGER_DET_ARRAY_SIZE 6

EXPORT void new_metrics_collector(MODULE *);

#ifdef __cplusplus

// struct containing two return values for voltage violation results
typedef struct vol_violation {
	double durationViolation; 		     //integrator state variable
	int countViolation;	 //angle measurement
} VIOLATION_RETURN;

// TODO: char buffer sizes, dtors for arrays
class metrics_collector{
public:
	static metrics_collector *defaults;
	static CLASS *oclass, *pclass;

	explicit metrics_collector(MODULE *);
	int create();
	int init(OBJECT *);
	int isa(char *);
	TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

	int commit(TIMESTAMP);

public:
	double interval_length_dbl;			//Metrics output interval length

	friend class metrics_collector_writer;

private:

	int read_line(OBJECT *obj);
	int write_line(TIMESTAMP, OBJECT *obj);

	void copyHistories(int from, int to);
	void interpolateHistories(int idx, TIMESTAMP t);
	void interpolate(double array[], int idx, double denom, double top);
	double findMax(double array[], int size);
	double findMin(double array[], int size);
	double findAverage(double array[], int size);
	double findMedian(double array[], int size);

	double countPerc(int array[], int size);
	
	vol_violation findOutLimit(bool firstCall, double array[], bool checkAbove, double limitVal, int size);

	// saved class properties of my parent object
	static PROPERTY *propTriplexNomV;
	static PROPERTY *propTriplexV1;
	static PROPERTY *propTriplexV2;
	static PROPERTY *propTriplexV12;
	static PROPERTY *propTriplexBill;
	static PROPERTY *propTriplexPrice;
	static PROPERTY *propTriplexP;
	static PROPERTY *propTriplexQ;

	static PROPERTY *propMeterNomV;
	static PROPERTY *propMeterVa;
	static PROPERTY *propMeterVb;
	static PROPERTY *propMeterVc;
	static PROPERTY *propMeterVab;
	static PROPERTY *propMeterVbc;
	static PROPERTY *propMeterVca;
	static PROPERTY *propMeterBill;
	static PROPERTY *propMeterPrice;
	static PROPERTY *propMeterP;
	static PROPERTY *propMeterQ;

	static PROPERTY *propHouseLoad;
	static PROPERTY *propHouseHVAC;
	static PROPERTY *propHouseAirTemp;
	static PROPERTY *propHouseCoolSet;
	static PROPERTY *propHouseHeatSet;
	static PROPERTY *propHouseSystemMode;
	static PROPERTY *propWaterLoad;
	static PROPERTY *propWaterSetPoint;
	static PROPERTY *propWaterDemand;
	static PROPERTY *propWaterTemp;

	static PROPERTY *propWaterLSetPoint;
	static PROPERTY *propWaterUSetPoint;
	static PROPERTY *propWaterLTemp;
	static PROPERTY *propWaterUTemp;
	static PROPERTY *propWaterElemLMode;
	static PROPERTY *propWaterElemUMode;

	static PROPERTY *propInverterS;

	static PROPERTY *propCapCountA;
	static PROPERTY *propCapCountB;
	static PROPERTY *propCapCountC;

	static PROPERTY *propRegCountA;
	static PROPERTY *propRegCountB;
	static PROPERTY *propRegCountC;

	static PROPERTY *propSwingSubLoad;
	static PROPERTY *propSwingMeterS;

	static PROPERTY *propTransformerOverloaded;
	static PROPERTY *propLineOverloaded;

	static PROPERTY *propChargeRate;
	static PROPERTY *propBatterySOC;

	TIMESTAMP next_write; // on global clock, increments by interval_length
	TIMESTAMP start_time; // start time of simulation
	bool write_now;
	bool first_write;
	bool log_me;
	void log_to_console(char *msg, TIMESTAMP t);
	static bool log_set;

	char* parent_string;
	char parent_name[256];
	double *metrics; // depends on the parent class

	// Parameters related to triplex_meter and (billing) meter objects
	double *real_power_array;		//array storing real power measured at the meter
	double *reactive_power_array;		//array storing reactive power measured at the meter
	double *voltage_vll_array;		//average line-to-line (or hot-to-hot) voltage at the meter
	double *voltage_vln_array;		//average line-to-neutral (or hot-to-neutral) voltage at the meter
	double *voltage_unbalance_array;		//array storing voltage unbalance per ANSI C84.1
	double price_parent;
	double bill_parent;

	// Parameters related to house object
	double *total_load_array; 		//array storing total_load measured at the house
	double *hvac_load_array; 		//array storing hvac_load measured at the house
	double *air_temperature_array; 		//array storing air_temperature measured at the house
	double *set_cooling_array;	// array storing air_temperature cooling setpoint
	double *set_heating_array;	// array storing air_temperature heating setpoint
	int system_mode;

	// Parameters related to waterheater object
	double *wh_load_array; 		
	double *wh_setpoint_array;
	double *wh_demand_array; 	
	double *wh_temp_array; 		

	double *wh_l_setpoint_array;
	double *wh_l_temp_array;
	double *wh_u_setpoint_array;
	double *wh_u_temp_array;
	int elem_l_mode;
	int elem_u_mode;
	char waterheaterName[256];				// char array storing names of the waterheater

	// Parameters related to inverter object
	// No new arrays defined for inverter object,
	// since real_power_array and reactive_power_array have been defined for triplex_meter already

	// Parameters related to capacitor and regulator objects
	double *count_array;  // these _count member variables are doubles in capacitor.h and regulator.h

  // Parameters related to transformer objects
	int *trans_overload_status_array;
	int *line_overload_status_array;

  // Parameters related to evcharger det objects
	double *charge_rate_array;
	double *battery_SOC_array;

	// Parameters related to Swing-bus meter object
	FINDLIST *link_objects;
	double *real_power_loss_array;		//array storing real power losses for the whole feeder
	double *reactive_power_loss_array;		//array storing real power losses for the whole feeder

	TIMESTAMP interval_length;	  // integer averaging length (seconds)
	int vector_length;     // padded interval_length to hold both endpoints
	TIMESTAMP *time_array; // actual sample times

	int curr_index;	// Index [0..interval_length-1] for current position of averaging array
};

#endif // C++

#endif // _METRICS_COLLECTOR_H_

// EOF
