// $Id: fsm.h 4984 2014-12-19 22:05:13Z dchassin $
// Finite State Machine
// @file fsm.h
//
// FSM code syntax:
//   n:b->m; ...
// where n is the state, b is a boolean expression, and m is the new state.
// Boolean expression are of the form b1,b2,...bn where each b is of the form
// xry, where x and y are constants or variables, and r is the relation
// such as "<", "==", ">", and "!=".  Commas act like "and" operations.
// "Or" operations can be done by listing multiple entries for the state n.
//

#ifndef _FSM_H
#define _FSM_H

#include "gridlabd.h"

typedef struct s_fsmrule {
	int64 to; ///< to state value
	double *lhs; ///> left-hand side comparison value
	double *rhs; ///< right-hand side comparison value
	PROPERTYCOMPAREOP cop; ///< comparison operator
	struct s_fsmrule *next_and; ///< and next rule
	struct s_fsmrule *next_or; ///< or next rule
} FSMRULE; ///< implements a state transition rule
typedef struct s_fsmcall {
	void (*entry)(struct s_statemachine *); ///< state entry call
	void (*exit)(struct s_statemachine *); ///< state exit call
	void (*during)(struct s_statemachine *); ///< state sync call
} FSMCALL; ///< implements state function calls
typedef struct s_statemachine {
	// internal finite state machine
	double value; ///< local copy of state variable
	struct s_object_list *context; ///< object context in which this machine runs
	enumeration *variable; ///< integer state variable
	PROPERTY *prop; ///< state variable property
	unsigned int64 n_states; ///< number of possible states
	FSMRULE **transition_rules; ///< state index to rules
	double_array *transition_holds; ///< state index to hold times
	FSMCALL *transition_calls; ///< state index to function calls
	TIMESTAMP entry_time; ///< time of entry into current state
	TIMESTAMP hold_time; ///< time of hold end for current state
	double runtime; ///< elapsed time in the current state
	struct s_statemachine *next; /// next machine
	// external finite state machine
//	unsigned int nhls;
//	PROPERTY *lhs;
//	unsigned int nrhs;
//	PROPERTY *rhs;
//	int (*external_machine)(unsigned int nlhs, double *plhs, unsigned int nrhs, double *prhs);
} statemachine;

#ifdef __cplusplus
extern "C" {
#endif

int fsm_create(statemachine *machine, OBJECT *context);
int fsm_init(statemachine *machine);
TIMESTAMP fsm_sync(statemachine *machine, PASSCONFIG pass, TIMESTAMP t1);
TIMESTAMP fsm_syncall(TIMESTAMP t1);

void fsm_reset(statemachine *machine); ///< reset the state machine to state 0
void fsm_clear(statemachine *machine); ///< clears the state machine rules, etc.
bool fsm_isprogrammed(statemachine *machine); ///< true of the state machine has been programmed

int convert_to_fsm(char *string, void *data, PROPERTY *prop);
int convert_from_fsm(char *string,int size,void *data, PROPERTY *prop);

int fsm_set_state_variable(statemachine *machine, char *name);
int fsm_add_transition_rule(statemachine *machine, char *name);
int fsm_add_transition_hold(statemachine *machine, char *name);


#ifdef __cplusplus
}
#endif


#endif
