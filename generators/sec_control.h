#ifndef GLD_GENERATORS_SEC_CONTROL_H_
#define GLD_GENERATORS_SEC_CONTROL_H_

#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "generators.h"

EXPORT int isa_sec_control(OBJECT *obj, char *classname);
EXPORT STATUS preupdate_sec_control(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_sec_control(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_sec_control(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);

// State variables for the secondary controller
typedef struct{
	double perr[2]; //power error in MW for times t, t-1.
	double uniterr; //total unit error in MW
	double deltaf; // frequency error signal *after* adjustment for max/min and deadband
	double dxi; //change of PID integrator output
	double xi; //ouput of PID integrator
	double PIDout; //PID controller output
	double dP; //total change in MW (PIDout when sampling, or older value otherwise)
} SEC_CNTRL_STATE;

// Datastructure of participants in secondary control
typedef struct{
	OBJECT *ptr; // pointer to the object
	double alpha; //participation factor between 0 and 1.
	gld_property *pdisp; //pointer to the dispatch property of the object
	gld_property *poffset; //pointer to the dispatch offset propert of the object
	double rate; //the rating of the object in MW/MVA
	double pmax; // maximum set point (units dependent on object)
	double pmin; //minimal set point (units dependent on object)
	double pout; // electrical output in MW.
	double dp_dn; //maximum allowable change to setpoint in MW - downward direction (shouid still be positive)
	double dp_up; //maximum allowable change to setpoint in MW - upward direction
	double Tlp; //Low pass time filter time constant in sec.
	double dP[4]; //Change in stepoint [corrector, predictor, actual value set predictor, t-1]
	double ddP[2]; //Change in stepoint derivative [predictor, corrector]
} SEC_CNTRL_PARTICIPANT;

//sec_control class
class sec_control : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags

	double curr_timestamp_dbl;	//deltamode - tracks last timestep for reiterations and time tracking
	SIMULATIONMODE desired_simulation_mode; //deltamode desired simulation mode after corrector pass - prevents starting iterations again

	gld_property *pFrequency; //pointer to frequency property of parent object
	double fmeas; //storage for current measured frequency value

	/* participating object vector */
	// Enumeration for manipulation of secondary control participants
	enum PARSE_OPTS{
        ADD = 0,    // Add members to secondary controller
        REMOVE = 1, // Remove members from the secondary controller
        MODIFY = 2 // Modify existing secondary controller members
    };
	enumeration parse_opt;

	// Structure for parsing the input glm string
	struct PARSE_KEYS {
        PARSE_OPTS key;
        int offset;
        std::size_t pos;
        friend bool operator<(const PARSE_KEYS& p1, const PARSE_KEYS& p2)
        {
            return (p1.pos < p2.pos);
        }
    };
	
	std::vector<SEC_CNTRL_PARTICIPANT> part_obj; // Vector of participating objects in secondary control
	
	// Sampling
	bool sampleflag; //Boolean flag whether outputs should be published or not
	bool sampleflag_P; //Boolean flag whether unit error inputs should be sampled
	bool sampleflag_f; //Boolean flag whether frequency should be sampled
	double sample_time; //Secondary control should post a new update at this time
	double sample_P; //Secondary control should sample unit error values at this time
	double sample_f; //Secondary control should sample frequency at this time
	bool deadbandflag; // false=frequency within deadband, true=frequency outside of deadband 
	bool tielineflag; // false=all tie lines within tolerance, true=some tielines outside of tolerance
	
	double curr_dt; //Stores current deltamode deltat value OR -1 if not available
	double qsts_time; //if the current time is greater than this time then we can switch to delta mode. -1 indicates unset.
	bool deltaf_flag; // indicates that a non zero deltaf has been detedted at some poin in this deltamode run.
	SIMULATIONMODE prev_simmode_return_value; //store the previously returned simulation mode value (in case iterations > 2)
public:
	char1024 participant_input; // command string for creating/modifing secondary controller participants (or path to csv file)
	int32 participant_count; //Number of participants in the controller
	double dp_dn_default; // Default maximum allowable change to setpoint - downward direction (shouid still be positive)
	double dp_up_default; // Default maximum allowable change to setpoint - upward direction
	double Tlp_default; // Default low pass time filter time constant in sec.

	/* Input to PID controller */
	double f0; // Nominal frequency in Hz (default is 60 Hz)
	double underfrequency_limit; //Maximum positive input limit to PID controller is f0 - underfreuqnecy_limit
	double overfrequency_limit; // Maximum negative input limit to PID controller is f0 - overfrequency_limit 
	double deadband; //Deadband for PID controller input in Hz
	double tieline_tol; // Generic tie-line error tolerance in p.u. (w.r.t set-point)
	double frequency_delta_default; //default delta to nominal frequency

	double B; //frequency bias in MW/Hz

	/* PID Controller */
	double kpPID; //PID proprtional gain in p.u.
	double kiPID; //PID integral gain in p.u./sec
	double kdPID; //PID derivative gain in p.u.*sec 

	enum ANTI_WINDUP{
        NONE = 0, //no anti-windup active
		ZERO_IN_DEADBAND = 1, //zero integrator when frequency is within deadband
		FEEDBACK_PIDOUT = 2,  //feedback difference between PIDout and signal passed to generators
		FEEDBACK_INTEGRATOR = 3 // feedback difference between PIDout and actual "actuated" output (neglecting proportional and derivative paths)
    };
	enumeration anti_windup; //windup handling for integrator

	double Ts; //Sampling interval in sec (should be >= dt)
	double Ts_P; //Sampling interval in sec for unit/tie-line errors (should be >= dt)
	double Ts_f; //Sampling interval in sec for frequency (should be >= dt)
	double deltat_pid; //Minimum of Ts_p and Ts_f in seconds. Integration step for PID contorller.
	double deltath_pid; //deltat_pid/2 for corrector pass.
	SEC_CNTRL_STATE curr_state; //current state variables
	SEC_CNTRL_STATE next_state; //next state variables
	
	/* utility functions */
	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
	gld_property *map_enum_value(OBJECT *obj, const char *name);

	gld::complex get_complex_value(OBJECT *obj, const char *name);
	double get_double_value(OBJECT *obj, const char *name);
	enumeration get_enum_value(OBJECT *obj, const char *name);
	
	void init_check(double&, double, double);
	void participant_tlp_check(SEC_CNTRL_PARTICIPANT *);
	void participant_tlp_check(SEC_CNTRL_PARTICIPANT &);
	void get_perr(void);
	void ts_lb_check(double&, double, double, const char *name);
	void sample_time_update(bool&, double&, double, double);
	void update_pdisp(SEC_CNTRL_PARTICIPANT &, double);
	double get_pelec(SEC_CNTRL_PARTICIPANT &obj);
	double get_tieline_error(SEC_CNTRL_PARTICIPANT &obj);


	/* parsing functions */
	void parse_obj(PARSE_OPTS, std::string&);
    void parse_csv(std::string&);
    void parse_glm(std::string&);
    void parse_praticipant_input(char*);
    double str2double(std::string&, double);
    void add_obj(std::vector<std::string> &);
    void mod_obj(std::vector<std::string> &);
    void rem_obj(std::vector<std::string> &);
    std::vector<SEC_CNTRL_PARTICIPANT>::iterator find_obj(std::string);
	std::vector<std::string> split(const std::string&, char);
	std::string ltrim(const std::string&);
	std::string rtrim(const std::string&);
	std::string trim(const std::string&);
	

	/* required implementations */
	sec_control(MODULE *module);
	int isa(char *classname);

	int create(void);
	int init(OBJECT *parent);
	// TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	// TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	STATUS pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass);

public:
	static CLASS *oclass;
	static sec_control *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_SEC_CONTROL_H_
