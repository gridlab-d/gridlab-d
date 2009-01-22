/** $Id: powerflow.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file powerflow.h
	@ingroup powerflow

 @{
 **/


#ifndef _POWERFLOW_H
#define _POWERFLOW_H

#include "gridlabd.h"

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

typedef enum {SM_FBS=0, SM_GS=1} SOLVERMETHOD;		/**< powerflow solver methodology */
GLOBAL SOLVERMETHOD solver_method INIT(SM_FBS);		/**< powerflow solver methodology */
GLOBAL double acceleration_factor INIT(1.4);		/**< Acceleration factor for Gauss-Seidel to increase convergence speed */
GLOBAL bool show_matrix_values INIT(false);			/**< flag to enable dumping matrix calculations as they occur */
GLOBAL double primary_voltage_ratio INIT(60.0);		/**< primary voltage ratio (@todo explain primary_voltage_ratio in powerflow (ticket #131) */
GLOBAL double nominal_frequency INIT(60.0);			/**< nomimal operating frequencty */
GLOBAL double warning_underfrequency INIT(55.0);	/**< frequency below which a warning is posted */
GLOBAL double warning_overfrequency INIT(65.0);		/**< frequency above which a warning is posted */
GLOBAL double nominal_voltage INIT(240.0);			/**< nominal voltage level */
GLOBAL double warning_undervoltage INIT(0.8);		/**< voltage magnitude (per unit) below which a warning is posted */
GLOBAL double warning_overvoltage INIT(1.2);		/**< voltage magnitude (per unit) above which a warning is posted */
GLOBAL double warning_voltageangle INIT(2.0);		/**< voltage angle (over link) above which a warning is posted */
GLOBAL bool require_voltage_control INIT(false);	/**< flag to enable voltage control source requirement */
GLOBAL double geographic_degree INIT(0.0);			/**< topological degree factor */
GLOBAL complex fault_Z INIT(complex(1e-6,0));		/**< fault impedance */
GLOBAL double default_maximum_voltage_error INIT(1e-8);	/**< default sync voltage convergence limit [puV] */

/* used by many powerflow enums */
#define UNKNOWN 0

#include "powerflow_object.h"

#endif // _POWERFLOW_H

/**@}*/
