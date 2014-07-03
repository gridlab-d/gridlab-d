/** $Id: powerflow.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file powerflow.cpp
	@addtogroup powerflow Distribution flow solver (radial)
	@ingroup modules

	This module contains a model simulating a powerflow distribution network 
	using Kersting's method outlined in his book: <I>Distribution System Modeling and Analysis.</I>
	In this method, the network is looked at as a system of	nodes and links.
	Voltage and current are passed along the network in a series of sweeps.
	During the sweeps, and small system of equations are solved at each link 
	using linear methods.  The sweeps are repeated for each timestep until the
	system reaches equilibrium, at which point the time in the network can 
	advance to the next available timestamp.  During a sweep, 3 passes are
	made across the network; pre-topdown, bottom up, and post-topdown.  During
	the pre-topdown pass, the meter object resets agregator variables, and
	calculates maximum powe and energy consumption.  During the bottom up pass,
	currents are agregated, starting at the meter, and working its way up the
	network to the generation node.  In the post-topdown pass, voltages for
	each node are calculated starting at the generation node and working down
	towards the meter object (and from there into the end use models).
	
 @{
 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "powerflow.h"
#include "node.h"
#include "link.h"

extern "C" {
#include "output.h"
}

void print_matrix(complex mat[3][3])
{
	for (int i = 0; i < 3; i++) {
		gl_testmsg("  %10.6f%+0.6f%c  %10.6f%+0.6f%c  %10.6f%+0.6f%c",
			mat[i][0].Re(), mat[i][0].Im(), mat[i][0].Notation(),
			mat[i][1].Re(), mat[i][1].Im(), mat[i][1].Notation(),
			mat[i][2].Re(), mat[i][2].Im(), mat[i][2].Notation());
	}
	gl_testmsg("\n");
}

EXPORT int kmldump(int (*stream)(const char*,...), OBJECT *obj)
{

	if (obj==NULL) /* dump document styles */
	{
		/* line styles */
		stream("<Style id=\"overhead_line\">\n"
			" <LineStyle>\n"
			"  <color>7f00ffff</color>\n"
			"  <width>4</width>\n"
			" </LineStyle>\n"
			" <PolyStyle>\n"
			"  <color>7f00ff00</color>\n"
			" </PolyStyle>\n"
			"</Style>\n");
		stream("<Style id=\"underground_line\">\n"
			" <LineStyle>\n"
			"  <color>3f00ffff</color>\n"
			"  <width>4</width>\n"
			" </LineStyle>\n"
			" <PolyStyle>\n"
			"  <color>3f00ff00</color>\n"
			" </PolyStyle>\n"
			"</Style>\n");
		return 0;
	}
	else if (gl_object_isa(obj,"node"))
		return OBJECTDATA(obj,node)->kmldump(stream);
	else if (gl_object_isa(obj,"link"))
		return OBJECTDATA(obj,link_object)->kmldump(stream);
	else
		return 0; 
}

/**@}**/
