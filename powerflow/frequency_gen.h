// $Id: frequency_gen.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _FREQUENCYGEN_H
#define _FREQUENCYGEN_H

#include "powerflow.h"

typedef enum {
		MATCHED=0,		///< defines matched condition
		GOV_DELAY=1,	///< defines governor delay condition
		GEN_RAMP=2		///< defines generator "catchup" ramping condition
		} FREQSTATE;

typedef enum {
		EMPTY=0,		//No contribution
		CONSTANT=1,		//Constant term
		SCALAR=2,		//Scalar term ax
		EXPONENT=3,		//Exponential a*exp(bx)
		STEPEXP=4,		//Step exponent of a*(1-exp(b*x))
		RAMPEXP=5,		//Ramp exponent of a*[(x-1/b*(exp(b*x)-1))+(c-t-1/b+1/b*exp((t-c)*b))*u(t-c)]
		PERSCONST=6,	//Persistent constant - always there
		CSTEPEXP=7,		//Step exponent with consant - c+a*(1-exp(b*x))
		} CONTRIBTYPE;

typedef enum {
		OFF=0,	//"Dumb" frequency generation - played in presumably
		AUTO=1	//"Active" frequency generation - performed by this object
		} FREQMODE;

typedef struct s_algcomponents {
		double coeffa;			//First coefficient (constant, scalar)
		double coeffb;			//Second coefficient - exponential scalar
		double delay_time;		//Time delay for delayed contributions (RAMPEXP)
		CONTRIBTYPE contype;	//Contribution type
		TIMESTAMP starttime;	//Starting time of this contributor
		TIMESTAMP endtime;		//Ending time of this contributor
		TIMESTAMP enteredtime;	//Time entry was included - used for updates
} ALGCOMPONENTS;	//Algorithm components definition

class frequency_gen : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	double FrequencyValue;
	double FrequencyChange;
	double Mac_Inert, D_Load, PMech, tgov_delay;
	double LoadPower;
	double system_base;
	double ramp_rate;
	double Freq_Low, Freq_High, Freq_Histr;
	double calc_tol_perc;
	enumeration FreqObjectMode;
	int Num_Resp_Eqs;
	int iter_passes;		//Number of passes that have been made over the object - used to figure out if things need removing or not
	double day_average, day_std, week_average, week_std;	//Variables for 24- and 168-hour statistics (mean and std dev)
	
	frequency_gen(MODULE *mod);
	inline frequency_gen(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);

	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);

private:
	FREQSTATE CurrGenCondition, NextGenCondition;	//State machine variables
	double NominalFreq;
	double stored_freq[168];	//Storage array for statistic estimations
	int curr_store_loc;			//Pointer to current storage location
	TIMESTAMP track_time;		//Time to track when an hour has passed to store "new" value
	int eight_tau_value;	//8 times the time constant of the system - theoretically used to remove equation contributions
	int three_tau_value;	//3 times the time constant of the system - used for comparisons
	TIMESTAMP prev_time;	//Previous timestep a run has been accomplished
	TIMESTAMP next_time;	//Next timestep a change is expected
	TIMESTAMP curr_dt;		//Current dt value observed
	TIMESTAMP trans_time;	//Time to next FOI transition (estimated)
	TIMESTAMP Gov_Delay;	//Time governor delay will be up and ramp rate will begin (for delay calculations)

	double PrevLoadPower;	//Load power calculated at last timestep
	double PrevGenPower;	//Generator power calculated at last timestep (used during ramp)

	bool Gov_Delay_Active;	//Boolean to indicate if in the delay period (is Gov_Resp valid)
	bool Gen_Ramp_Direction;	//Boolean to indicate ramping direction - false is down, true is up
	bool Change_Lockout;		//Flag to prevent multiple changes during a timestep.  Mainly used during ramps to prevent multiple time extensions
	double DB_Low, DB_High;	//Frequency dead-band limits for governor operation
	double K_val, T_val, invT_val, AK_val;	//Working variables for simplified power/frequency model
	double F_Low_PU, F_High_PU;	//Low and High FOI - in p.u.
	double P_delta;				//DeltaP associated with the frequency hysteresis
	double calc_tol;			//power calculation threshold (since it wiggles a lot)

	TIMESTAMP pres_updatetime(TIMESTAMP t0, TIMESTAMP dt_val);	//Function to compute frequency changes based on current influencing contributions (writes updates)
	TIMESTAMP updatetime(TIMESTAMP t0, TIMESTAMP dt_val, double &FreqVal, double &DeltaFreq);	//Function to compute frequency changes based on current influencing contributions (does not write anything)
	void RemoveScheduled(TIMESTAMP t0);	//Function to remove all items added for the specified timestamp.  Used to aid in the correction of jitter and changing powerflow conditions that may force something to be schedule early.

	void DumpScheduler(void);	//Temporary - remove me

	ALGCOMPONENTS *CurrEquations;	//Equation "buffer"
};

#endif // _FREQUENCYGEN_H
