/** $Id: powerflow.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file powerflow.h
	@ingroup powerflow

 @{
 **/


#ifndef _POWERFLOW_H
#define _POWERFLOW_H

#include "gridlabd.h"
#include "solver_nr.h"

#ifdef _POWERFLOW_CPP
#define GLOBAL
#define INIT(A) = (A)
#else
#define GLOBAL extern
#define INIT(A)
#endif

#ifdef _DEBUG
void print_matrix(complex mat[3][3]);
#endif

#define GETOBJECT(obj) ((OBJECT *) obj - 1)
#define IMPORT_CLASS(name) extern CLASS *name##_class

typedef enum {SM_FBS=0, SM_GS=1, SM_NR=2} SOLVERMETHOD;		/**< powerflow solver methodology */
typedef enum {MM_SUPERLU=0, MM_EXTERN=1} MATRIXSOLVERMETHOD;	/**< NR matrix solver methodlogy */

//Structure to hold external LU solver calls
typedef struct s_ext_fxn {
	void *dllLink;
	void *ext_init;
	void *ext_alloc;
	void *ext_solve;
	void *ext_destroy;
} EXT_LU_FXN_CALLS;

GLOBAL char256 LUSolverName INIT("");				/**< filename for external LU solver */
GLOBAL EXT_LU_FXN_CALLS LUSolverFcns;				/**< links to external LU solver functions */
GLOBAL SOLVERMETHOD solver_method INIT(SM_FBS);		/**< powerflow solver methodology */
GLOBAL bool use_line_cap INIT(false);				/**< Flag to include line capacitance quantities */
GLOBAL bool use_link_limits INIT(true);				/**< Flag to include line/transformer ratings and provide a warning if exceeded */
GLOBAL MATRIXSOLVERMETHOD matrix_solver_method INIT(MM_SUPERLU);	/**< Newton-Raphson uses superLU as the default solver */
GLOBAL unsigned int NR_bus_count INIT(0);			/**< Newton-Raphson bus count - used for determining size of bus vector */
GLOBAL unsigned int NR_branch_count INIT(0);		/**< Newton-Raphson branch count - used for determining size of branch vector */
GLOBAL BUSDATA *NR_busdata INIT(NULL);				/**< Newton-Raphson bus data pointer array */
GLOBAL BRANCHDATA *NR_branchdata INIT(NULL);		/**< Newton-Raphson branch data pointer array */
GLOBAL NR_SOLVER_STRUCT NR_powerflow;				/**< Newton-Raphson solver working variables - "steady-state" powerflow version */
GLOBAL int NR_curr_bus INIT(-1);					/**< Newton-Raphson current bus indicator - used to populate NR_busdata */
GLOBAL int NR_curr_branch INIT(-1);					/**< Newton-Raphson current branch indicator - used to populate NR_branchdata */
GLOBAL int64 NR_iteration_limit INIT(500);			/**< Newton-Raphson iteration limit (per GridLAB-D iteration) */
GLOBAL bool NR_dyn_first_run INIT(true);			/**< Newton-Raphson first run indicator - used by deltamode functionality for initialization powerflow */
GLOBAL bool NR_admit_change INIT(true);				/**< Newton-Raphson admittance matrix change detector - used to prevent complete recalculation of admittance at every timestep */
GLOBAL int NR_superLU_procs INIT(1);				/**< Newton-Raphson related - superLU MT processor count to request - separate from thread_count */
GLOBAL TIMESTAMP NR_retval INIT(TS_NEVER);			/**< Newton-Raphson current return value - if t0 objects know we aren't going anywhere */
GLOBAL OBJECT *NR_swing_bus INIT(NULL);				/**< Newton-Raphson swing bus */
GLOBAL bool FBS_swing_set INIT(false);				/**< Forward-Back Sweep swing assignment variable */
GLOBAL bool show_matrix_values INIT(false);			/**< flag to enable dumping matrix calculations as they occur */
GLOBAL double primary_voltage_ratio INIT(60.0);		/**< primary voltage ratio (@todo explain primary_voltage_ratio in powerflow (ticket #131) */
GLOBAL double nominal_frequency INIT(60.0);			/**< nomimal operating frequencty */
GLOBAL double warning_underfrequency INIT(55.0);	/**< frequency below which a warning is posted */
GLOBAL double warning_overfrequency INIT(65.0);		/**< frequency above which a warning is posted */
GLOBAL double warning_undervoltage INIT(0.8);		/**< voltage magnitude (per unit) below which a warning is posted */
GLOBAL double warning_overvoltage INIT(1.2);		/**< voltage magnitude (per unit) above which a warning is posted */
GLOBAL double warning_voltageangle INIT(2.0);		/**< voltage angle (over link) above which a warning is posted */
GLOBAL bool require_voltage_control INIT(false);	/**< flag to enable voltage control source requirement */
GLOBAL double geographic_degree INIT(0.0);			/**< topological degree factor */
GLOBAL complex fault_Z INIT(complex(1e-6,0));		/**< fault impedance */
GLOBAL double default_maximum_voltage_error INIT(1e-6);	/**< default sync voltage convergence limit [puV] */
GLOBAL double default_maximum_power_error INIT(0.0001);	/**< default power convergence limit for multirun */
GLOBAL OBJECT *restoration_object INIT(NULL);		/**< restoration object of the system */
GLOBAL OBJECT *fault_check_object INIT(NULL);		/**< fault_check object of the system */

GLOBAL bool enable_subsecond_models INIT(false);		/* normally not operating in delta mode */
GLOBAL bool all_powerflow_delta INIT(false);			/* Flag to make all powerflow objects participate in deltamode -- otherwise is individually flagged per object */
GLOBAL unsigned long deltamode_timestep INIT(10000000); /* deltamode timestep value - 10 ms timestep, at first - intermnal */
GLOBAL double deltamode_timestep_publish INIT(10000000.0); /* deltamode module-published 10 ms timestep, at first -- module property version, to be converted*/
GLOBAL OBJECT **delta_objects INIT(NULL);				/* Array pointer objects that need deltamode interupdate calls */
GLOBAL FUNCTIONADDR *delta_functions INIT(NULL);	/* Array pointer functions for objects that need deltamode interupdate calls */
GLOBAL FUNCTIONADDR *delta_freq_functions INIT(NULL);	/* Array pointer functions for objects that have "frequency" updates at end of deltamode */
GLOBAL int pwr_object_count INIT(0);				/* deltamode object count */
GLOBAL int pwr_object_current INIT(-1);				/* Index of current deltamode object */
GLOBAL TIMESTAMP deltamode_starttime INIT(TS_NEVER);	/* Tracking variable for next desired instance of deltamode */
GLOBAL double current_frequency INIT(60.0);			/**< Current operating frequency of the system - used by deltamode stuff */
GLOBAL int64 deltamode_extra_function INIT(0);		/**< Kludge pointer to module-level function, so generators can call it */
GLOBAL double default_resistance INIT(1e-4);		/**< sets the default resistance for safety devices */

// Deltamode stuff
void schedule_deltamode_start(TIMESTAMP tstart);	/* Anticipated time for a deltamode start, even if it is now */
int delta_extra_function(unsigned int mode);

/* used by many powerflow enums */
#define UNKNOWN 0
#define ROUNDOFF 1e-6			// numerical accuracy for zero in float comparisons


#include "powerflow_object.h"

#endif // _POWERFLOW_H

/**@}*/
