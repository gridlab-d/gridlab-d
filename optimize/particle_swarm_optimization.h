/** $Id: particle_swarm_optimization.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file particle_swarm_optimization.h
	@addtogroup optimize
	@ingroup optimize

 @{
 **/


#ifndef _particle_swarm_optimization_H

#define _particle_swarm_optimization_H

//#include <complex>
#include <stdarg.h>
#include "gridlabd.h"
#include "optimize.h"
#include "simple.h"


//typedef enum {OG_EXTREMUM, OG_MINIMUM, OG_MAXIMUM} OBJECTIVEGOAL;
//typedef enum {OT_DISCRETE, OT_DISCRETE_ITERATE, OT_TEST} OPTIMIZERTYPE;

class particle_swarm_optimization {
protected:
	OBJECTIVEGOAL goal; // objective goal description
	double no_particles; // The number of agents (particles) in a swarm
	double max_iterations; //Total number of iterations
	int gbest_index;
	double no_unknowns;
	double gbest_value;
	double C1;
	double C2;
	double pbset_fitness;	// a_mat - 3x3 matrix, 'a' matrix
	double particle_position[500][3];
	double particle_velocity[500][3];
	double pbest[1][3];
	double gbest[1][3];
	//double present[1][3];
	double variable_1;
	double variable_2;
	double variable_3;
	double current_fitness [500];
	double solution;
	double abs_solution;
	double gbest1;
	double gbest2;
	double gbest3;
	double position_lb;
	double position_ub;
	double velocity_lb;
	double velocity_ub;

	double rand1;
	double rand2;

	double cycle_interval;

	double w;

	
	int32 trials; // maximum number of trials allowed for one point in DISCRETE_ITERATE
private:
	//int32 *trial; // Might be able to do use this instead of trial if you want to distinguish between NULL and user-set trial limit of 0 (for checking)
	int32 trial; // trial counter for DISCRETE_ITERATE
	int32 pass; // pass number (0-2 is order estimate, 3 is constrained)
	TIMESTAMP t_next; //next time you want particle_swarm_optimization to update
	bool feas_check; // starts at 0, switches to 1 once a feasible solution has been found
	bool violation; // check for constraint violations
	double next_x1; // next x1 value to check around
	double next_x2; // next x2 value to check around
	double next_x3; // next x2 value to check around
	double last_y; //y value from previous iteration. Used in DISCRETE_ITERATE to check convergence.
	double best_x1; // best value of x1 found
	double best_x2; // best value of x2 found
	double best_x3; // best value of x2 found
	double best_y; //best y value found

	TIMESTAMP cycle_interval_TS;
	TIMESTAMP time_cycle_interval;
	TIMESTAMP prev_cycle_time;
	TIMESTAMP curr_cycle_time;
	double dt_val;
	//struct {
	//	double real; // constraint operation
	//	double imag; // constraint value
	//} *pObjective; // objective variable
	double *pObjective; // objective variable
	double *pVariable1; // decision variable
	double *pVariable2; // decision variable
	double *pVariable3; // decision variable
	double *pConstraint1; // Constraint variable. One for each, upper and lower for x and y.
	double *pConstraint2;
	double *pConstraint3;
	double *pConstraint4;
	double *pConstraint5;
	double *pConstraint6;
	double *pConstraint7;
	double *pConstraint8;
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain1; // Describe a constraint. One for each constraint.
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain2; // describe a constraint
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain3; // describe a constraint
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain4; // describe a constraint
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain5; // Describe a constraint. One for each constraint.
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain6; // describe a constraint
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain7; // describe a constraint
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain8; // describe a constraint
	bool constraint_broken(bool (*op)(double,double), double value, double x); // detect constraint
public:
	// required implementations 
	particle_swarm_optimization(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static CLASS *pclass;
	static particle_swarm_optimization *defaults;
};

#endif

