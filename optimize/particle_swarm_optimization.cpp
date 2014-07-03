/** $Id: particle_swarm_optimization.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file auction.cpp
	@defgroup auction Template for a new object class
	@ingroup market

	The auction object implements the basic auction. 

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>

#include "gridlabd.h"
#include "particle_swarm_optimization.h"

CLASS* particle_swarm_optimization::oclass = NULL;
CLASS* particle_swarm_optimization::pclass = NULL;
particle_swarm_optimization *particle_swarm_optimization::defaults = NULL;

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_POSTTOPDOWN;

// Class registration is only called once to register the class with the core
particle_swarm_optimization::particle_swarm_optimization(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"particle_swarm_optimization",sizeof(particle_swarm_optimization),passconfig);
		if (oclass==NULL)
			throw "unable to register class particle_swarm_optimization";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			
			PT_double, "no_particles", PADDR(no_particles), PT_DESCRIPTION, "The number of agents (particles) in a swarm.", //this might not work if it isn't specified in input
			PT_double, "C1", PADDR(C1), PT_DESCRIPTION, "Hookes’s coefficients", //
			PT_double, "C2", PADDR(C2), PT_DESCRIPTION, "Hookes’s coefficients", //
			PT_double, "no_unknowns", PADDR(no_unknowns), PT_DESCRIPTION, "This is the dimension of solution space. It is equal to the number of unknowns in a variable to be optimized", //
			PT_double, "max_iterations", PADDR(max_iterations), PT_DESCRIPTION, "Total number of iterations.",

			PT_double, "w", PADDR(w), PT_DESCRIPTION, "Weighting factor.",

			PT_double, "variable_1", PADDR(variable_1), PT_DESCRIPTION, "variable_1 value", //
			PT_double, "variable_2", PADDR(variable_2), PT_DESCRIPTION, "variable_2 value", //
			PT_double, "variable_3", PADDR(variable_3), PT_DESCRIPTION, "variable_3 value", //

			PT_double, "gbest", PADDR(gbest), PT_DESCRIPTION, "gbest", //
			PT_double, "gbest1", PADDR(gbest1), PT_DESCRIPTION, "gbest1", //
			PT_double, "gbest2", PADDR(gbest2), PT_DESCRIPTION, "gbest2", //
			PT_double, "gbest3", PADDR(gbest3), PT_DESCRIPTION, "gbest3", //
			
			PT_double, "position_lb", PADDR(position_lb), PT_DESCRIPTION, "position_lb", //
			PT_double, "position_ub", PADDR(position_ub), PT_DESCRIPTION, "position_ub", //
			
			PT_double, "velocity_lb", PADDR(velocity_lb), PT_DESCRIPTION, "velocity_lb", //
			PT_double, "velocity_ub", PADDR(velocity_ub), PT_DESCRIPTION, "velocity_ub", //

			PT_double,"cycle_interval[s]", PADDR(cycle_interval),
						
			NULL)<1)
		{
			static char msg[256];
			sprintf(msg, "unable to publish properties in %s",__FILE__);
			throw msg;
		}

		defaults = this;
		memset(this,0,sizeof(particle_swarm_optimization));
	}
}


int particle_swarm_optimization::isa(char *classname)
{
	return strcmp(classname,"particle_swarm_optimization")==0;
}

// Object creation is called once for each object that is created by the core
int particle_swarm_optimization::create(void)
{
	/*int rval;*/
	cycle_interval_TS = 0;
	time_cycle_interval = 0;	
	///*
	//retur*/n rval;
	return 1; // return 1 on success, 0 on failure
}

// Object initialization is called once after all object have been created
int particle_swarm_optimization::init(OBJECT *parent)
{
	//int rval;
	OBJECT *obj = OBJECTHDR(this);
	char buffer[1024];

	if (cycle_interval == 0.0)
	cycle_interval = 900.0;
	if (cycle_interval <= 0)
	{
		GL_THROW("emissions:%s must have a positive cycle_interval time",obj->name);
		/*  TROUBLESHOOT
		The calculation cycle interval for the emissions object must be a non-zero, positive
		value.  Please specify one and try again.
		*/
	}

	cycle_interval_TS = (TIMESTAMP)(cycle_interval);
		// read the current values
	if(!no_particles)
		no_particles = 100;
		//check the no_particles limit
	else if ( no_particles<1 )
	{
		gl_error("The no_particles limit 'no_particles' in PSO object '%s' must be a positive integer value", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	if(!no_unknowns)
	    no_unknowns = 1;
		//check the no_particles limit
	else if ( no_unknowns<1 )
	{
		gl_error("The no_particles limit 'no_unknowns' in PSO object '%s' must be a positive integer value", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}


	if(!max_iterations)
	    max_iterations = 200;
		//check the no_particles limit
	else if ( max_iterations<1 )
	{
		gl_error("The no_particles limit 'max_iterations' in PSO object '%s' must be a positive integer value", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	
	if(!variable_1)
	    variable_1 = 0;
		//check the no_particles limit
	else if ( variable_1<0 )
	{
		gl_error("The no_particles limit 'variable_1' in PSO object '%s' must be a positive integer value or zero", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	if(!C1)
	    C1 = 2;
		//check the no_particles limit
	else if ( C1<1 )
	{
		gl_error("The no_particles limit 'C1' in PSO object '%s' must be a positive integer value", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	if(!C2)
	    C2 = 2;
		//check the no_particles limit
	else if ( C2<1 )
	{
		gl_error("The no_particles limit 'C2' in PSO object '%s' must be a positive integer value", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	
	if(!variable_2)
	    variable_2 = 0;
		//check the no_particles limit
	else if ( variable_2<0 )
	{
		gl_error("The no_particles limit 'variable_2' in PSO object '%s' must be a positive integer value or zero", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	
	if(!variable_3)
	    variable_3 = 0;

	else if ( variable_3<0 )
	{
		gl_error("The no_particles limit 'variable_3' in PSO object '%s' must be a positive integer value or zero", gl_name(obj,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	time_cycle_interval = gl_globalclock;
	return 1; // return 1 on success, 0 on failure

}


//Presync is called when the clock needs to advance on the first top-down pass
//For presync and postsync:
//Each case checks all values delta around that case. The last case selects the best of those values.
//If no feasible solution exists, return an error.
//Note that the value from the previous time step will always be tested at the current time step, which is okay because the objective function may
//	change such that this value is once again optimal.
TIMESTAMP particle_swarm_optimization::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);

	if (prev_cycle_time != t0)	//New timestamp - accumulate
	{
		//Store current cycle
		curr_cycle_time = t0;

		//Find dt from last interval
		dt_val = (double)(curr_cycle_time - prev_cycle_time);

		//Update tracking variable
		prev_cycle_time = t0;
	}
	//t_next should only equal 0 when simulation first starts.

	if (curr_cycle_time >= time_cycle_interval)	//Update values
	{

			for (int particle = 0; particle < no_particles; particle++) 
			{
				for (int dimension = 0; dimension < no_unknowns; dimension++) 
				{
					particle_position[particle][dimension] = position_lb + (position_ub - position_lb) * gl_random_uniform(RNGSTATE,0.0,1.0);
					particle_velocity[particle][dimension] = velocity_lb + (velocity_ub - velocity_lb) *gl_random_uniform(RNGSTATE,0.0,1.0);

				}
			}

			// Initialize the pbest fitness 

				pbset_fitness = -1000.0;
				gbest_value = -1000.0;

		for (int iteration = 0; iteration < max_iterations; iteration++) 
		{
			for (int particle = 0; particle < no_particles; particle++) 

			{
				variable_1 = particle_position[particle][0];
				variable_2 = particle_position[particle][1];
				variable_3 = particle_position[particle][2];


				//solution = 100*(variable_2 - (variable_1*variable_1))*(variable_2 - (variable_1*variable_1))+ (1-variable_1)*(1-variable_1) ; 
				//solution = (0.25*variable_1*variable_1*variable_1*variable_1) + (0.5*variable_2*variable_2) - (variable_1*variable_2) + variable_1 - variable_2;

//Minimization example
				solution = 2*variable_1 + 10*variable_2 + 8*variable_3;//example 2, page 514
				
				if ((variable_1 + variable_2 + variable_3 >= 6) && (variable_2 + 2*variable_3 >= 8) && (-variable_1 + 2*variable_2 + 2*variable_3 >= 4)&& (variable_1 >=0)&&(variable_2 >=0)&& (variable_3 >=0))
					current_fitness[particle] = -solution;	
				else
					current_fitness[particle] = -solution-100000000000000;				

//Maximization example
			//	solution = -2*variable_1 + variable_2 - 2*variable_3;//maximize solution = 2*variable_1 - variable_2 + 2*variable_3;(example 2, page 500)
			//	
			//	if ((variable_1 + 2*variable_2 - 2*variable_3 <= 20) && (2*variable_1 + variable_2 <= 10) && (variable_2 + 2*variable_3 <= 5)&& (variable_1 >=0)&&(variable_2 >=0)&& (variable_3 >=0))
			//		current_fitness[particle] = -solution;	
			//	else
			//		current_fitness[particle] = solution-100000000000000;	

			}

			// Decide pbest among all the particles

			for (int particle = 0; particle < no_particles; particle++) 

			{
				if (current_fitness[particle] > pbset_fitness)
				{
					pbset_fitness= current_fitness[particle];
				
				for (int dimension = 0; dimension < no_unknowns; dimension++)
				{
					pbest[0][dimension] = particle_position[particle][dimension];
				}	
				}

			}			

			if (pbset_fitness> gbest_value)
			{
				gbest_value= pbset_fitness;
			for (int dimension = 0; dimension < no_unknowns; dimension++)	
				{
				gbest[0][dimension] = pbest[0][dimension];				
				}
			}

			gbest1 = gbest[0][0];
			gbest2 = gbest[0][1];
			gbest3 = gbest[0][2];

				// Update position and velocity

			for (int particle = 0; particle < no_particles; particle++) 

			{
				for (int dimension = 0; dimension < no_unknowns; dimension++)
				{
					rand1 = gl_random_uniform(RNGSTATE,0.0,1.0);
					rand2 = gl_random_uniform(RNGSTATE,0.0,1.0);

					particle_velocity[particle][dimension] = w*particle_velocity[particle][dimension] + C1 * rand1 * (pbest[0][dimension] - particle_position[particle][dimension])+C2 * rand2 * (gbest[0][dimension] - particle_position[particle][dimension]);

					particle_position[particle][dimension] = particle_position[particle][dimension]+particle_velocity[particle][dimension];

				}
			}
		}
  
		time_cycle_interval += cycle_interval_TS;
		t1 = time_cycle_interval;
	}

	if (t1 > time_cycle_interval)
	{
		t1 = time_cycle_interval;	//Next output cycle is sooner
	}

	//Make sure we don't send out a negative TS_NEVER
	if (t1 == TS_NEVER)
		return TS_NEVER;

	else
		return -t1;
}

// Postsync is called when the clock needs to advance on the second top-down pass
TIMESTAMP particle_swarm_optimization::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *my = OBJECTHDR(this);
	char buffer[1024];

	if(t_next == 0 || t_next == t1)
	{

	}
		return TS_NEVER;
}

bool particle_swarm_optimization::constraint_broken(bool (*op)(double,double), double value, double x)
{
	return !op(x,value);
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_particle_swarm_optimization(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(particle_swarm_optimization::oclass);
		if (*obj!=NULL)
		{
			particle_swarm_optimization *my = OBJECTDATA(*obj,particle_swarm_optimization);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(particle_swarm_optimization);
}

EXPORT int init_particle_swarm_optimization(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,particle_swarm_optimization)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(particle_swarm_optimization);
}

EXPORT int isa_particle_swarm_optimization(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,particle_swarm_optimization)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_particle_swarm_optimization(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	particle_swarm_optimization *my = OBJECTDATA(obj,particle_swarm_optimization);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
 			t2 = my->presync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(particle_swarm_optimization);
}

