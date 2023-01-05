/** $Id: powerflow.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file powerflow.h
	@ingroup powerflow

 @{
 **/


#ifndef _POWERFLOW_H
#define _POWERFLOW_H

#include "gridlabd.h"
#ifndef GLD_USE_EIGEN
#include "solver_nr.h"
#else
#include "solver_nr_eigen.h"
#endif
#ifdef _POWERFLOW_CPP
#define GLOBAL
#define INIT(A) = (A)
#else
#undef GLOBAL
#define GLOBAL extern
#undef INIT
#define INIT(A)
#endif

#ifdef _DEBUG
void print_matrix(gld::complex mat[3][3]);
#endif

#define GETOBJECT(obj) ((OBJECT *) obj - 1)
//#define IMPORT_CLASS(name) extern CLASS *name##_class

typedef enum {SM_FBS=0, SM_GS=1, SM_NR=2} SOLVERMETHOD;		/**< powerflow solver methodology */
typedef enum {NRM_TCIM=0, NRM_FPI=1} NRSOLVERALG;		/**< NR solver underlying appraoch */
typedef enum {MM_SUPERLU=0, MM_EXTERN=1} MATRIXSOLVERMETHOD;	/**< NR matrix solver methodlogy */
typedef enum {
	MD_NONE=0,			///< No matrix dump desired
	MD_ONCE=1,			///< Single matrix dump desired
	MD_PERCALL=2,		///< Matrix dump every call desired
	MD_ALL=3			///< Matrix dump on every iteration desired
} MATRIXDUMPMETHOD;

typedef enum {
	LS_OPEN=0,			///< defines that that link is open
	LS_CLOSED=1,		///< defines that that link is closed
	LS_INIT=2			///< defines that the link needs to be initalized
} LINESTATUS;	//Line/link status - made at powerflow level for reusability

typedef enum {
	IRM_NONE=0,			///< Flag as no integration method
	IRM_UNDEFINED=1,	///< Basically a flag to indicate "use the default" - specific object-level usage
	IRM_TRAPEZOIDAL=2,	///< Use the trapezoidal implementaiton
	IRM_BACKEULER=3		///< Use the backward-Euler implementation
} INRUSHINTMETHOD;	//Selection of the integration method for the inrush calculations

typedef enum {
	FMM_NONE=0,		///< No default
	FMM_SIMPLE=1,	///< Objects default to simple frequency measurement method
	FMM_PLL=2		///< Objects default to the PLL-based frequency measurement method
} FREQMEASDEFAULT;	//Base default method for frequency measurement

//Structure to hold external LU solver calls
typedef struct s_ext_fxn {
	void *dllLink;
	void *ext_init;
	void *ext_alloc;
	void *ext_solve;
	void *ext_destroy;
} EXT_LU_FXN_CALLS;

GLOBAL char256 LUSolverName INIT("KLU");				/**< filename for external LU solver */
GLOBAL EXT_LU_FXN_CALLS LUSolverFcns;				/**< links to external LU solver functions */
GLOBAL SOLVERMETHOD solver_method INIT(SM_FBS);		/**< powerflow solver methodology */
GLOBAL NRSOLVERALG NR_solver_algorithm INIT(NRM_TCIM);	/**< NR underlying algorithm */
GLOBAL char256 MDFileName INIT("");					/**< filename for matrix dump */
GLOBAL MATRIXDUMPMETHOD NRMatDumpMethod INIT(MD_NONE);	/**< NR-based matrix output method */
GLOBAL bool NRMatReferences INIT(false);			/**< Flag to indicate if the decoding information for the matrix is dumped - row/col to bus */
GLOBAL bool NRMatRHSDump INIT(false);				/**< Flag to indicate if the RHS portion (current injection) should be exported to the file */
GLOBAL bool use_line_cap INIT(false);				/**< Flag to include line capacitance quantities */
GLOBAL bool use_link_limits INIT(true);				/**< Flag to include line/transformer ratings and provide a warning if exceeded */
GLOBAL MATRIXSOLVERMETHOD matrix_solver_method INIT(MM_SUPERLU);	/**< Newton-Raphson uses superLU as the default solver */
GLOBAL unsigned int NR_bus_count INIT(0);			/**< Newton-Raphson bus count - used for determining size of bus vector */
GLOBAL unsigned int NR_branch_count INIT(0);		/**< Newton-Raphson branch count - used for determining size of branch vector */
GLOBAL BUSDATA *NR_busdata INIT(nullptr);				/**< Newton-Raphson bus data pointer array */
GLOBAL BRANCHDATA *NR_branchdata INIT(nullptr);		/**< Newton-Raphson branch data pointer array */
GLOBAL NR_SOLVER_STRUCT NR_powerflow;				/**< Newton-Raphson solver pointer working variables - one per island detected */
GLOBAL int NR_islands_detected INIT(0);				/**< Newton-Raphson solver island count (from fault_check) - determines the array size of NR_powerflow */
GLOBAL bool NR_island_fail_method INIT(false);		/**< Newton-Raphson multiple islands - determine how individual island failure may determined */
GLOBAL bool NR_solver_working INIT(false);			/**< Newton-Raphson global flag to indicate if the solver is working -- mostly to prevent island redetection if it is mid-array */
GLOBAL int NR_curr_bus INIT(-1);					/**< Newton-Raphson current bus indicator - used to populate NR_busdata */
GLOBAL int NR_curr_branch INIT(-1);					/**< Newton-Raphson current branch indicator - used to populate NR_branchdata */
GLOBAL int64 NR_iteration_limit INIT(500);			/**< Newton-Raphson iteration limit (per GridLAB-D iteration) */
GLOBAL bool NR_dyn_first_run INIT(true);			/**< Newton-Raphson first run indicator - used by deltamode functionality for initialization powerflow */
GLOBAL bool NR_admit_change INIT(true);				/**< Newton-Raphson admittance matrix change detector - used to prevent complete recalculation of admittance at every timestep */
GLOBAL bool NR_FPI_imp_load_change INIT(true);		/**< Newton-Raphson Fixed-Point-Iterative - flag to indicate if impedance load changed (for admittance reform) */
GLOBAL int NR_superLU_procs INIT(1);				/**< Newton-Raphson related - superLU MT processor count to request - separate from thread_count */
GLOBAL TIMESTAMP NR_retval INIT(TS_NEVER);			/**< Newton-Raphson current return value - if t0 objects know we aren't going anywhere */
GLOBAL OBJECT *NR_swing_bus INIT(nullptr);				/**< Newton-Raphson swing bus */
GLOBAL int NR_expected_swing_rank INIT(6);			/**< Newton-Raphson expected master swing bus rank - for multi-gen children compatibility */
GLOBAL bool NR_swing_deferred_pass INIT(false);		/**< Newton-Raphson toggle for deferred init - for multi-gen children compatibility */
GLOBAL bool NR_swing_rank_set INIT(false);			/**< Newton-Raphson check to see if SWING has set its rank (mostly for other objects) */
GLOBAL int NR_swing_bus_reference INIT(-1);			/**< Newton-Raphson swing bus index reference in NR_busdata */
GLOBAL int64 NR_delta_iteration_limit INIT(10);		/**< Newton-Raphson iteration limit (per deltamode timestep) */
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
GLOBAL gld::complex fault_Z INIT(gld::complex(1e-6,0));		/**< fault impedance */
GLOBAL gld::complex ground_Z INIT(gld::complex(1e-6,0));		/**< ground impedance */
GLOBAL double default_maximum_voltage_error INIT(1e-6);	/**< default sync voltage convergence limit [puV] */
GLOBAL double default_maximum_power_error INIT(0.0001);	/**< default power convergence limit for multirun */
GLOBAL OBJECT *restoration_object INIT(nullptr);		/**< restoration object of the system */
GLOBAL OBJECT *fault_check_object INIT(nullptr);		/**< fault_check object of the system */
GLOBAL bool fault_check_override_mode INIT(false);	/**< Mode designator for fault_check -- overrides errors and prevents powerflow -- meant for debug */
GLOBAL bool meshed_fault_checking_enabled INIT(false);	/*** fault_check object flag for possible meshing -- adjusts how reliability-related code runs */
GLOBAL bool restoration_checks_active INIT(false);	/***< Overall flag for when reconfigurations are occurring - special actions in devices */
GLOBAL double reliability_metrics_recloser_counts INIT(0.0);	/***< Recloser counts for momentary outages for power_metrics -- primarily to do backwards compatibility*/

GLOBAL bool enable_subsecond_models INIT(false);		/* normally not operating in delta mode */
GLOBAL bool all_powerflow_delta INIT(false);			/* Flag to make all powerflow objects participate in deltamode -- otherwise is individually flagged per object */
GLOBAL FREQMEASDEFAULT all_powerflow_freq_measure_method INIT(FMM_NONE);		/* Flag to enable all capable powerflow objects to do frequency measurements */
GLOBAL unsigned long deltamode_timestep INIT(10000000); /* deltamode timestep value - 10 ms timestep, at first - internal */
GLOBAL double deltamode_timestep_publish INIT(10000000.0); /* deltamode module-published 10 ms timestep, at first -- module property version, to be converted*/
GLOBAL int delta_initialize_iterations INIT(0);			/* deltamode - extra powerflow iterations on first timestep - useful for initialization */
GLOBAL OBJECT **delta_objects INIT(nullptr);				/* Array pointer objects that need deltamode interupdate calls */
GLOBAL FUNCTIONADDR *delta_functions INIT(nullptr);	/* Array pointer functions for objects that need deltamode interupdate calls */
GLOBAL FUNCTIONADDR *post_delta_functions INIT(nullptr);		/* Array pointer functions for objects that need deltamode postupdate calls */
GLOBAL int pwr_object_count INIT(0);				/* deltamode object count */
GLOBAL int pwr_object_current INIT(-1);				/* Index of current deltamode object */
GLOBAL TIMESTAMP deltamode_starttime INIT(TS_NEVER);	/* Tracking variable for next desired instance of deltamode */
GLOBAL TIMESTAMP deltamode_endtime INIT(TS_NEVER);		/* Tracking variable to see when deltamode ended - so differential calculations don't get messed up */
GLOBAL double deltamode_endtime_dbl INIT(TS_NEVER_DBL);		/* Tracking variable to see when deltamode ended - double valued for explicit movement calculations */
GLOBAL TIMESTAMP deltamode_supersec_endtime INIT(TS_NEVER);	/* Tracking variable to indicate the "floored" time of detamode_endtime */
GLOBAL double current_frequency INIT(60.0);			/**< Current operating frequency of the system - used by deltamode stuff */
GLOBAL bool master_frequency_update INIT(false);	/**< Whether a generator has designated itself "keeper of frequency" -- temporary deltamode override */
GLOBAL bool enable_frequency_dependence INIT(false);	/**< Flag to enable frequency-based updates of impedance values, namely loads and lines */
GLOBAL double default_resistance INIT(1e-4);		/**< sets the default resistance for safety devices */

//In-rush deltamode stuff
GLOBAL bool enable_inrush_calculations INIT(false);	/**< Flag to enable in-rush calculations in deltamode */
GLOBAL INRUSHINTMETHOD default_inrush_integration_method INIT(IRM_TRAPEZOIDAL);	/**< Integration method used for inrush calculations */
GLOBAL double impedance_conversion_low_pu INIT(0.7);	/** Lower PU voltage level to convert all loads to impedance */
GLOBAL bool enable_impedance_conversion INIT(false);	/**< Flag to enable conversion of loads to impedance-based - used for non-inrush cases */
GLOBAL double deltatimestep_running INIT(-1.0);			/** Value of the current deltamode simulation - used for integration method in in-rush */

//Mesh fault current stuff
GLOBAL bool enable_mesh_fault_current INIT(false);	/** Flag to enable mesh-based fault current calculations */

//Market-based item -- moved from triplex_meter
GLOBAL char1024 market_price_name INIT("current_market.clearing_price");

// Deltamode stuff
void schedule_deltamode_start(TIMESTAMP tstart);	/* Anticipated time for a deltamode start, even if it is now */

/* used by many powerflow enums */
#define UNKNOWN 0
#define ROUNDOFF 1e-6			// numerical accuracy for zero in float comparisons


#include "powerflow_object.h"

#endif // _POWERFLOW_H

/**@}*/
