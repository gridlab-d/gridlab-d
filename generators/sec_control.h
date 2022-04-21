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
EXPORT STATUS postupdate_sec_control(OBJECT *obj, complex *useful_value, unsigned int mode_pass);

// State variables for the secondary controller
typedef struct{
	double deltaf[2]; //Frequency error for times t, t-1.
	double dxi; //change of PID integrator output
	double xi; //ouput of PID integrator
	double PIDout; //PID controller output
} SEC_CNTRL_STATE;

// Datastructure of participants in secondary control
typedef struct{
	OBJECT *ptr; // pointer to the object
	double alpha; //participation factor between 0 and 1.
	gld_property *pset; //pointer to the setpoint of the object, which will be manipulated
	double dp_min; //maximum allowable change to setpoint - downward direction (shouid still be positive)
	double dp_max; //maximum allowable change to setpoint - upward direction
	double Tlp; //Low pass time filter time constant in sec.
	double dP[2]; //Change in stepoint [predictor, corrector]
	double ddP[2]; //Change in stepoint derivative [predictor, corrector]
} SEC_CNTRL_PARTICIPANT;

//sec_control class
class sec_control : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;

	double curr_timestamp_dbl;	//deltamode - tracks last timestep for reiterations and time tracking
	SIMULATIONMODE desired_simulation_mode; //deltamode desired simulation mode after corrector pass - prevents starting iterations again

	////**************** Sample variable - published property for sake of doing so ****************//
	complex test_published_property;

	double alpha_tol; //tolerance for participations values not summing to one.
	
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
	double sample_time; //Secondary control should post a new update at this time
public:
	char1024 participant_input; // command string for creating/modifing secondary controller participants (or path to csv file)

	double dp_min_default; // Default maximum allowable change to setpoint - downward direction (shouid still be positive)
	double dp_max_default; // Default maximum allowable change to setpoint - upward direction
	double Tlp_default; // Default low pass time filter time constant in sec.

	/* Input to PID controller */
	double f0; // Nominal frequency in Hz (default is 60 Hz)
	double underfrequency_limit; //Maximum positive input limit to PID controller is f0 - underfreuqnecy_limit
	double overfrequency_limit; // Maximum negative input limit to PID controller is f0 - overfrequency_limit 
	double deadband; //Deadband for PID controller input in Hz

	/* PID Controller */
	double kpPID; //PID proprtional gain in MW/Hz
	double kiPID; //PID integral gain in MW/Hz/sec
	double kdPID; //PID derivative gain in MW/Hz*sec 

	double Ts; //Sampling interval in sec (should be >= dt)

	SEC_CNTRL_STATE curr_state; //current state variables
	SEC_CNTRL_STATE next_state; //next state variables
	
	/* utility functions */
	gld_property *map_complex_value(OBJECT *obj, char *name);
	gld_property *map_double_value(OBJECT *obj, char *name);
	
	void get_deltaf(void);
	STATUS check_alpha(void);
	void clear_states(void);
	
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
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	STATUS pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(complex *useful_value, unsigned int mode_pass);

public:
	static CLASS *oclass;
	static sec_control *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_SEC_CONTROL_H_
