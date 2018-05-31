/* residential.h
	Copyright (C) 2008 Battelle Memorial Institute
 * contains all utility functions and common definitions for the residential module
 */

#ifndef _RESIDENTIAL_H
#define _RESIDENTIAL_H

#include "gridlabd.h"
#include "module.h"

#ifdef _RESIDENTIAL_CPP
#define GLOBAL
#define INIT(A) = (A)
#else
#define GLOBAL extern
#define INIT(A)
#endif

/* useful constants */
#define RHOWATER	(62.4)			// lb/cf
#define CFPGAL		(0.133681)		// cf/gal
#define GALPCF		(7.4805195)		// gal/cf
#define CWATER		(0.9994)		// BTU/lb/F
#define BTUPHPW		(3.4120)		// BTUPH/W
#define BTUPHPKW	(1e3 * 3.4120)		// BTUPH/kW
#define KWPBTUPH	(1e-3/BTUPHPW)	// kW/BTUPH
#define MWPBTUPH	(1e-6/BTUPHPW)	// MW/BTUPH
#define ROUNDOFF	1e-6			// numerical accuracy for zero in float comparisons

const double pi = 3.1415926535897931;
const double Cp = 1;					// Btu/lbm-F

/* approximate tests */
#define AEQ(A,B,C) (fabs(A-B)<C)
#define ANE(A,B,C) (fabs(A-B)>=C)
#define ALT(A,B,C) (A<=B+C)
#define AGT(Ak,B,C) (A>=B-C)

#define MAX(A,B) ((A)>(B)?(A):(B))
#define MIN(A,B) ((A)<(B)?(A):(B))

typedef enum {	BRK_OPEN=0,		///< breaker open
				BRK_CLOSED=1,	///< breaker closed
				BRK_FAULT=-1,	///< breaker faulted
} BREAKERSTATUS; ///< breaker state
typedef enum {	X12=0,	///< circuit from line 1 to line 2    (240V)
				X23=1,	///< circuit from line 2 to line 3(N) (120V)
				X13=2,	///< circuit from line 1 to line 3(N) (120V)
} CIRCUITTYPE; ///< circuit type

typedef struct s_circuit {
	CIRCUITTYPE type;	///< circuit type
	enduse *pLoad;	///< pointer to the load struct (ENDUSELOAD* in house_a, enduse* in house_e)
	gld_property *pV; ///< pointer to appropriate circuit voltage property
	double max_amps; ///< maximum breaker amps
	int id; ///< circuit id
	BREAKERSTATUS status; ///< breaker status
	TIMESTAMP reclose; ///< time at which breaker is reclosed
	unsigned short tripsleft; ///< the number of trips left before breaker faults
	struct s_circuit *next; ///< next circuit in list
	// DPC: commented this out until the rest of house_e is updated
} CIRCUIT; ///< circuit definition

typedef struct s_panel {
	double max_amps; ///< maximum panel amps
	BREAKERSTATUS status; ///< panel breaker status
	TIMESTAMP reclose; ///< time at which breaker is reclosed
	CIRCUIT *circuits; ///< pointer to first circuit in circuit list
} PANEL; ///< panel definition

typedef	CIRCUIT *(*ATTACHFUNCTION)(OBJECT *, enduse *, double , int is220); ///< type definition for attach function

typedef enum {HORIZONTAL, NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST,N_SOLAR_SURFACES} ORIENTATION;

//Globals
GLOBAL double default_line_voltage INIT(120.0);			//Value for the default nominal_voltage
GLOBAL bool ANSI_voltage_check INIT(true);				//Flag to enable/disable ANSI voltage violation checks
GLOBAL double default_outdoor_temperature INIT(74.0);	//Value for default outdoor air temperature
GLOBAL double default_humidity INIT(75.0);				//Value for default humidity
GLOBAL int64 default_etp_iterations INIT(100);			//Value for etp solution iterations
GLOBAL double default_horizontal_solar INIT(0.0);		//Value for horizontal solar gains

#endif  /* _RESIDENTIAL_H */

