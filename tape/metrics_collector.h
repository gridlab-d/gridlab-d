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
#define MTR_MIN_VLN       12
#define MTR_MAX_VLN       13
#define MTR_AVG_VLN       14
#define MTR_MIN_VUNB      15
#define MTR_MAX_VUNB      16
#define MTR_AVG_VUNB      17
#define MTR_ABOVE_A_DUR   18
#define MTR_ABOVE_A_CNT   19
#define MTR_BELOW_A_DUR   20
#define MTR_BELOW_A_CNT   21
#define MTR_ABOVE_B_DUR   22
#define MTR_ABOVE_B_CNT   23
#define MTR_BELOW_B_DUR   24
#define MTR_BELOW_B_CNT   25
#define MTR_BELOW_10_DUR  26
#define MTR_BELOW_10_CNT  27
#define MTR_ARRAY_SIZE    28

#define HSE_MIN_HVAC_LOAD    0
#define HSE_MAX_HVAC_LOAD    1
#define HSE_AVG_HVAC_LOAD    2
#define HSE_MIN_TOTAL_LOAD   3
#define HSE_MAX_TOTAL_LOAD   4
#define HSE_AVG_TOTAL_LOAD   5
#define HSE_MIN_AIR_TEMP     6
#define HSE_MAX_AIR_TEMP     7
#define HSE_AVG_AIR_TEMP     8
#define HSE_AVG_DEV_COOLING  9
#define HSE_AVG_DEV_HEATING 10
#define HSE_ARRAY_SIZE      11

#define WH_MIN_ACTUAL_LOAD  0
#define WH_MAX_ACTUAL_LOAD  1
#define WH_AVG_ACTUAL_LOAD  2
#define WH_ARRAY_SIZE       3

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

	metrics_collector(MODULE *);
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
	vol_violation findOutLimit(double lastVol, double array[], bool checkAbove, double limitVal, int size);

	// saved class properties of my parent object
	static PROPERTY *propTriplexNomV;
	static PROPERTY *propTriplexV1;
	static PROPERTY *propTriplexV2;
	static PROPERTY *propTriplexV12;
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
	static PROPERTY *propMeterPrice;
	static PROPERTY *propMeterP;
	static PROPERTY *propMeterQ;

	static PROPERTY *propHouseLoad;
	static PROPERTY *propHouseHVAC;
	static PROPERTY *propHouseAirTemp;
	static PROPERTY *propHouseCoolSet;
	static PROPERTY *propHouseHeatSet;

	static PROPERTY *propWaterLoad;

	static PROPERTY *propInverterS;

	static PROPERTY *propCapCountA;
	static PROPERTY *propCapCountB;
	static PROPERTY *propCapCountC;

	static PROPERTY *propRegCountA;
	static PROPERTY *propRegCountB;
	static PROPERTY *propRegCountC;

	static PROPERTY *propSwingSubLoad;
	static PROPERTY *propSwingMeterS;

	TIMESTAMP next_write; // on global clock, different by interval_length
	TIMESTAMP last_write; // touched only in init and postsync
	TIMESTAMP start_time; //Recording start time of simulation
	bool write_now;
	bool log_me;
	static bool log_set;

	char* parent_string;
	char parent_name[256];
	double *metrics; // depends on the parent class

	// Parameters related to triplex_meter object
	double *real_power_array;		//array storing real power measured at the triplex_meter
	double *reactive_power_array;		//array storing reactive power measured at the triplex_meter
	double *voltage_vll_array;		//array storing voltage12 measured at the triplex_meter
	double *voltage_vln_array;		//array storing voltage12/2 measured at the triplex_meter
	double *voltage_unbalance_array;		//array storing (voltage[0]-voltage[1])/(voltage12/2) measured at the triplex_meter
	double price_parent; 			// Price of the triplex_meter
	double last_vol_val;			// variable that store the voltage value from last time step, to assist in voltage violation counts analysis

	// Parameters related to house object
	double *total_load_array; 		//array storing total_load measured at the house
	double *hvac_load_array; 		//array storing hvac_load measured at the house
	double *air_temperature_array; 		//array storing air_temperature measured at the house
	double *dev_cooling_array;	// array storing air_temperature deviation from the cooling setpoint
	double *dev_heating_array;	// array storing air_temperature deviation from the heating setpoint

	// Parameters related to waterheater object
	double *wh_load_array; 		//array storing actual_load measured at the house
	char waterheaterName[256];				// char array storing names of the waterheater

	// Parameters related to inverter object
	// No new arrays defined for inverter object,
	// since real_power_array and reactive_power_array have been defined for triplex_meter already

	// Parameters related to capacitor and regulator objects
	double *count_array;  // these _count member variables are doubles in capacitor.h and regulator.h

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
