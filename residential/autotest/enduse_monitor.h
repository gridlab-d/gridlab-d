// enduse_monitoring.h
//
// Header for enduse_monitoring class

#ifndef _ENDUSEMONITORING_H
#define _ENDUSEMONITORING_H

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
    struct s_enduse *pLoad;	///< pointer to the load struct (ENDUSELOAD* in house_a, enduse* in house_e)
    complex *pV; ///< pointer to circuit voltage
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
} PANEL;

#endif
