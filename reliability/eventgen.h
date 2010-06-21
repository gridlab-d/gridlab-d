/** $Id: eventgen.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file eventgen.h

 @{
 **/

#ifndef _eventgen_H
#define _eventgen_H

#include <stdarg.h>
#include "gridlabd.h"
#include "reliability.h"
#include "metrics.h"

//Random distribution types - stolen from random.h
//SAMPLE removed since it won't fit well with how this goes
//DEGENERATE removed for same reason
typedef enum {
	UNIFORM=0,		/**< uniform distribution; double minimum_value, double maximum_value */
	NORMAL=1,		/**< normal distribution; double arithmetic_mean, double arithmetic_stdev */
	LOGNORMAL=2,	/**< log-normal distribution; double geometric_mean, double geometric_stdev */
	BERNOULLI=3,	/**< Bernoulli distribution; double probability_of_observing_1 */
	PARETO=4,		/**< Pareto distribution; double minimum_value, double gamma_scale */
	EXPONENTIAL=5,	/**< exponential distribution; double coefficient, double k_scale */
	RAYLEIGH=6,		/**< Rayleigh distribution; double sigma */
	WEIBULL=7,		/**< Weibull distribution; double lambda, double k */
	GAMMA=8,		/**< Gamma distribution; double alpha, double beta */
	BETA=9,			/**< Beta distribution; double alpha, double beta */
	TRIANGLE=10,	/**< Triangle distribution; double a, double b */
		} DISTTYPE;

typedef struct s_objevtdetails {
	OBJECT *obj_of_int;		/// Object that will be made unreliable in some manner
	TIMESTAMP fail_time;	/// Timestep when this object is expected to fail
	TIMESTAMP rest_time;	/// Timestep when this object is expected to be restored
	TIMESTAMP fail_length;	/// Length of time (seconds) the object will fail for
	TIMESTAMP rest_length;	/// Length of time (seconds) the object will need to be repaired/restored
	bool in_fault;			/// Flag to indicate if in fault or not - used to skip over fail_time
	int implemented_fault;	/// Enumeration to keep track of what type of fault created - useful for "random" fault decisions
	int customers_affected;	/// Count of number of customers affected by outage - differential count (fault implemented count - prefault outage count)
} OBJEVENTDETAILS; /// Object Event Details - contains

class eventgen {
private:
	double curr_fail_dist_params[2];	/**< Current parameters of failure_dist - used to track changes */
	double curr_rest_dist_params[2];	/**< Current parameters of restore_dist - used to track changes */
	DISTTYPE curr_fail_dist;			/**< Current failure distribution type - used to track changes */
	DISTTYPE curr_rest_dist;			/**< Current restoration distribution type - used to track changes */
	TIMESTAMP max_outage_length;		/**< Maximum length an outage may be */
	TIMESTAMP next_event_time;			/**< Time next event (outage or restoration) will occur */
	metrics *metrics_obj;				/**< Link to metrics object we need to report "restoration" statii to */
	int curr_time_interrupted;			/**< Number of customers interrupted at current time - used to determine "differential" counts */
	bool diff_count_needed;				/**< Flag to indicate a differential count needs to be obtained */
public:
	DISTTYPE failure_dist;		/**< failure distribution */
	DISTTYPE restore_dist;		/**< restoration distribution */
	char1024 target_group;		/**< object group aggregation */
	char256 fault_type;			/**< type of fault to induce */
	double fail_dist_params[2];	/**< Parameters for failure_dist */
	double rest_dist_params[2];	/**< Parameters for restore_dist */
	double max_outage_length_dbl;	/**< Maximum length of time an outage may be - published before casting */
	OBJEVENTDETAILS *UnreliableObjs;	/**< Array of found objects in system and parameters of their failure */
	int UnreliableObjCount;			/**< Size of UnreliableObjs */
	int max_simult_faults;		/**< Number of simultaneous faults this event_gen object can induce */
	int faults_in_prog;			/**< Number of faults currently induced by this event_gen object */
	TIMESTAMP gen_random_time(DISTTYPE rand_dist_type, double param_1, double param_2);	//Random time function - easier to call this way
public:
	/* required implementations */
	eventgen(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static eventgen *defaults;

};

#endif

/**@}*/
