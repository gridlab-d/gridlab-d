/** $Id: network.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network.cpp
	@addtogroup network Transmission flow solver (network)
	@ingroup modules

	The \e network module implements a balanced three-phase
	positive sequence power flow solver using the Gauss-Seidel
	method as described by Kundur [Kundur 1993, pp.259-260].

	The Gauss-Seidel (GS) method is a linear iterative method (as
	opposed to the Newton-Raphson or NR method, which uses a quadratic
	iterative method).  The only differences between the two
	methods are the rate of convergence and the robustness of
	convergence when the starting state is bad.  Because GridLAB-D is
	quasi-steady time-series simulation, most state changes
	are small and result in only 1 or 2 iterations in either
	method.  However, the GS method can be more easily implemented
	using parallel processing systems (such as multicore systems).
	In addition, the GS method is more likely to solve for new conditions
	that are far from the current solution.  However, the GS method can have
	difficulty computing flows under high-transfer conditions.

	The GS method uses an iterative approach proposed by Seidel in 1874.
	The GS method iterates using the first-order approximation of the
	Taylor expansion, and the NR method	uses a second-order approximation.
	In the GS method the fundamental equation for the \e k<sup>th</sup>
	node can written as

	\f$ \frac{P_k + \jmath Q_k}{\widetilde V_k^*} = Y_{kk} \widetilde V_k + \sum\limits_{i=1,i\neq k}^n{Y_{ki} \widetilde V_i} \f$

	The GS method is often criticized as inferior.  This is categorically
	not true.  It is true that it has strengths and weaknesses when
	compared to the NR method.  Depending on circumstances, these
	differences may cause one to prefer one method over the other.
	But both (indeed \e all) valid methods produce the same answers.
	Indeed, many commercial power flow solvers implement both
	methods concurrently, recognizing the necessity to exploit
	the appropriate method under any given circumstance.  Hence,
	the implementation of the GS method does not make GridLAB-D's
	solution method inferior in any way.  It is simply a recognition
	that the circumstances of the power flow solution needed in
	GridLAB-D leads one to choose the GS as the default power flow
	solver.  Other power flow solvers can and should be implemented
	to address different circumstances.  We encourage users and developers
	to consider doing so.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

#ifdef _DEBUG
EXPORT int cmdargs(int argc, char *argv[])
{
	int n=0;
	int i;
	for (i=0; i<argc; i++)
	{
		if (strncmp(argv[i],"--debug_node=",13)==0)
		{
			debug_node = atoi(argv[i]+13);
			n++;
		}
		else if (strncmp(argv[i],"--debug_link=",13)==0)
		{
			debug_link = atoi(argv[i]+13);
			n++;
		}
	}
	if (debug_node>0) gl_debug("network node debugging mode %d", debug_node);
	if (debug_link>0) gl_debug("network link debugging mode %d", debug_link);
	return n;
}
#endif // _DEBUG

/*
int generator_state_from_string(void *addr, char *value)
{
	if (strcmp(value,"STOPPED")==0)
		*(enumeration*)addr = generator::STOPPED;
	else if (strcmp(value,"STANDBY")==0)
		*(enumeration*)addr = generator::STANDBY;
	else if (strcmp(value,"ONLINE")==0)
		*(enumeration*)addr = generator::ONLINE;
	else if (strcmp(value,"TRIPPED")==0)
		*(enumeration*)addr = generator::TRIPPED;
	else
		return 0;
	return 1;
}

int generator_state_to_string(void *addr, char *value, int size)
{
	enumeration state = *(enumeration*)addr;
	switch (state) {
	case generator::STOPPED: if (size>7) strcpy(value,"STOPPED"); else return 0;
		return 1;
	case generator::STANDBY: if (size>7) strcpy(value,"STANDBY"); else return 0;
		return 1;
	case generator::ONLINE: if (size>6) strcpy(value,"ONLINE"); else return 0;
		return 1;
	case generator::TRIPPED: if (size>7) strcpy(value,"TRIPPED"); else return 0;
		return 1;
	default:
		return 0;
	}
}*/

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
#ifdef _DEBUG
	cmdargs(argc,argv);
#endif // _DEBUG

	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	if (gl_global_create("network::acceleration_factor",PT_double,&acceleration_factor,NULL)==NULL){
		gl_error( "network could not create global 'network::acceleration_factor'");
		return NULL;
	} if (gl_global_create("network::convergence_limit",PT_double,&convergence_limit,NULL)==NULL){
		gl_error( "network could not create global 'network::convergence_limit'");
		return NULL;
	} if (gl_global_create("network::mvabase",PT_double,&mvabase,NULL)==NULL){
		gl_error( "network could not create global 'network::mvabase'");
		return NULL;
	} if (gl_global_create("network::kvbase",PT_double,&kvbase,NULL)==NULL){
		gl_error( "network could not create global 'network::kvbase'");
		return NULL;
	} if (gl_global_create("network::model_year",PT_int16,&model_year,NULL)==NULL){
		gl_error( "network could not create global 'network::model_year'");
		return NULL;
	} if (gl_global_create("network::model_case",PT_char8,model_case,NULL)==NULL){
		gl_error( "network could not create global 'network::model_case'");
		return NULL;
	} if (gl_global_create("network::model_name",PT_char32,model_name,NULL)==NULL){
		gl_error( "network could not create global 'network::model_name'");
		return NULL;
	} 
	CLASS *first = (new node(module))->oclass;
	new link(module);
	new capbank(module);
	new fuse(module);
	new relay(module);
	new regulator(module);
	new transformer(module);
	new meter(module);
	new generator(module);

	/* always return the first class registered */
	return first;
}

CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}


/**@}*/
