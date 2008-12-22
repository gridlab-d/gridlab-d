/** $Id: main.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file main.c
	@author David P. Chassin

	@mainpage Introduction

	Welcome to the GridLAB-D developer documentation. \par
	
	Our mission is to create, as a community, the next generation power 
	system simulation that runs on all major platforms, provides open access to 
	all functionality, and supports extended modeling of non-traditional components
	of the electric power system, such as buildings, distributed resources, and
	markets.

	GridLAB-D<sup>TM</sup> is an open source project initiated by the 
	<a href="http://www.doe.gov/">US Department of Energy</a> 
	<a href="http://www.oe.energy.gov">Office of Electricity Delivery & Energy Reliability</a>.  
	Although it was started at <a href="http://www.pnl.gov">Pacific Northwest National Laboratory</a>
	for the US DOE, participation in this project is open to all interested parties at no cost.	
	If you have proprietary products that you would like to connect to GridLAB-D, you may do so 
	at no cost.  However, GridLAB-D itself is not distributed with any proprietary components.

	The \ref license "License" under which GridLAB-D is distributed is a BSD-style license. This means
	that distribution is unrestricted, provided basic common sense and decency 
	is used.  Specifically:
	- nobody may remove existing copyright statements
	- nobody may use existing copyright statements as an implied endorsement of their
	  own contribution.

	In addition, the license stipulates that the software is provided "as-is",
	it is not warranted to be appropriate for any particular use, and all users
	agree not to hold the US government or any of the contributors responsible
	for anything that happens as a consequence of using it.

	Please see \ref join "Joining In" or \ref faq "Frequently asked questions" for more information.
	These links can be found under "Related Pages" along with other helpful items such as the User's Manual.

	David Chassin (email: david.chassin@pnl.gov)\n October 2007

	******************************************************************************************
	@page license The GridLAB License

	The following is the text of the GridLAB-D license:

	@verbatim
	1. Battelle Memorial Institute (hereinafter Battelle) hereby grants permission 
	   to any person or entity lawfully obtaining a copy of this software and 
	   associated documentation files (hereinafter “the Software”) to redistribute 
	   and use the Software in source and binary forms, with or without modification.  
	   Such person or entity may use, copy, modify, merge, publish, distribute, 
	   sublicense, and/or sell copies of the Software, and may permit others to do 
	   so, subject to the following conditions:
	   - Redistributions of source code must retain the above copyright notice, this
		 list of conditions and the following disclaimers. 
	   - Redistributions in binary form must reproduce the above copyright notice, 
		 this list of conditions and the following disclaimer in the documentation 
		 and/or other materials provided with the distribution. 
	   - Other than as used herein, neither the name Battelle Memorial Institute 
		 nor Battelle may be used in any form whatsoever without the express written 
		 consent of Battelle.  
	2. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
	   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
	   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
	   ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY 
	   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
	   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
	   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
	   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
	   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
	   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	3. The Software was produced by Battelle under Contract No. DE-AC05-76RL01830 
	   with the Department of Energy.  The U.S. Government is granted for itself 
	   and others acting on its behalf a nonexclusive, paid-up, irrevocable worldwide 
	   license in this data to reproduce, prepare derivative works, distribute copies 
	   to the public, perform publicly and display publicly, and to permit others to 
	   do so.  The specific term of the license can be identified by inquiry made to 
	   Battelle or DOE.  Neither the United States nor the United States Department 
	   of Energy, nor any of their employees, makes any warranty, express or implied, 
	   or assumes any legal liability or responsibility for the accuracy, completeness 
	   or usefulness of any data, apparatus, product or process disclosed, or 
	   represents that its use would not infringe privately owned rights.  
	@endverbatim

	******************************************************************************************
	@page faq Frequently asked questions

	@section faq1 I am interested in working on GridLAB-D.  Are there any sources of government funding you can recommend?

	No. In fact, it is expected that in time, GridLAB-D will be supported by the collective contributions of users and developers 
	without need for government funding.

	@section faq2 Will you consider proprietary subsystem/modules as a part of the standard distribution of GridLAB-D?

	No, nor should any critical functionality of GridLAB-D require proprietary subsystems or modules.

	@section faq3 Given the short time between releases, are projects mainly to integrate existing technology or can they require substantial R&D effort?
	
	The integration of existing technology is preferred, at least early on in the development of GridLAB-D.  
	The objective of the open source project is to create the tools needed for conducting research, not to perform the research itself.

	@section faq4 What is the typical arrangement for intellectual property?

	Contributors will be responsible for protecting their own intellectual property.  
	Contributors are free to distribute modules they produce in whatever manner they deem appropriate.  
	However, the open source project team reserves the right not to distribute as a part of the standard 
	GridLAB-D distribution any code or module that is not licensed under a \ref license "GridLAB-style license".  

	@section faq5 Are there development conference Q&A sessions we can participate in?

	PNNL hosts technical question and answer teleconferences.  However, non-technical questions may not be answered.

	Please see \ref join "Joining In" for more information.

	******************************************************************************************
	@page join Joining in

	Until January 2008, development of GridLAB-D is conducted under funding
	from the US Department of Energy's Office of Electricity Delivery and Energy
	Reliability under the GridWise program and the Modern Grid Initiative. Before
	this date, only DOE contractors like Pacific Northwest National Laboratory
	and National Energy Technology Laboratory contributed to the project.

	As of January 2008, the GridLAB-D project is open to a group of charter 
	partners selected by the original developers for their ability to contribute
	essential elements to the overall system.  These charter partners have 
	committed themselves to getting the software ready for open source development, 
	which is scheduled to begin in earnest in January 2009.

	Beginning January 2009, GridLAB-D will be developed as a true open source 
	project.

	Please note these dates are anticipated and delays due to funding and technical
	problems may cause them to be changed somewhat.

	Please see \ref starting "Getting started" for more information.

	******************************************************************************************
	@page starting Getting started

	You must register and login on the <b>Subversion</b> site to download the 
	source code to GridLAB-D. GridLAB-D can be built with either of two 
	environments, MS Visual Studio 2005, or GNU tools like \b gcc and \b make 
	(typically on Linux).

	The basic strucutre of GridLAB-D centers around a core module that
	manages each of the runtime modules (e.g., residential, powerflow).  Runtime
	modules are only loaded when the model being processed calls for them.  

	The solver sequences the updates of objects in each module based on the ranks
	established when the model is loaded.  Objects of increasing rank are updated 
	earlier during top-down passes, and later during bottom-up passes.  The ranks
	are determined by the parent-child hierarchy defined in the model.  The ranking
	and synchronization process is controlled by the exec module.

	In principle the core is model agnostic.  All the modeling specifics are managed
	by the runtime modules.  This principle should be maintained as much as possible,
	if not absolutely.  Although GridLAB is mainly intended for use as a
	power system simulation that includes end-use behavior, it can very well be
	used for any type of agent-based simulation that is similar in character, i.e.,
	object have a well-defined hierarchy of implicit updates, a very well-defined 
	but limited	set interactions between objects, and a very high degree of implicit 
	parallelism in the solution methods used.

	Please see \ref background "Background" and other related pages for more information.

	******************************************************************************************
	@page background Background

	The coupling of engineered systems and markets impacts broader and broader areas 
	of the electric power industry. Energy trading products cover shorter time periods 
	and demand response programs are moving more and more toward real-time pricing. 
	Market-based trading activity impacts ever more directly the physical operation of 
	the system and the boundaries of these coupled systems extend beyond the traditional 
	boundaries of utility-centric energy system operations. To address the gaps in our 
	simulation capabilities, the US Department of Energy is developing GridLAB-D at 
	Pacific Northwest National Laboratory in collaboration with industry and academia. 
	This is the first of a new generation of power distribution system simulation software. 

	GridLAB-D combines end-use and power distribution automation models with other 
	software tools, resulting in a powerful tool for power system analysis capable of 
	running on high-performance computers. It is a flexible simulation environment that 
	can further be integrated with a variety of third-party data management and analysis tools.  

	At its core, GridLAB-D has an advanced algorithm that can determine the simultaneous 
	state of millions of independent devices, each of which is described by multiple 
	differential equations solved only locally for both state and time.  The advantages 
	of this algorithm over traditional finite difference-based simulators are: 
	1) it is much more accurate; 
	2) it can handle widely disparate time scales, ranging from sub-seconds to many years; and 
	3) it is easy to integrate with new modules and third-party systems.  
	The advantage over traditional differential-based solvers is that it is not necessary to 
	integrate all the device's behaviors into a single set of equations that must be solved.
	 
	At its simplest GridLAB-D can examine in detail the interplay of every part of a 
	distribution system with every other.  GridLAB-D does not require the use of 
	reduced-order models, so the danger of erroneous assumptions is averted.  GridLAB-D is 
	more likely to find problems with programs and business strategies than any other 
	tool available.  It is therefore an essential tool for industry and government planners.

	Please see \ref status "Current Status" for more information.

	******************************************************************************************
	@page status Current Status

	The GridLAB-D system currently implements modules to perform the following functions:
	- Power flow and controls, including distributed generation and storage
	- End-use appliance technologies, equipment and controls
	- Data collection on every property of every object in the system
	- Boundary condition management including weather and electrical boundaries.

	@section powerflow Power Flow

	The power flow component of GridLAB-D is separated into a distribution 
	module and a transmission module.  While the distribution systems are the 
	primary focus of GridLAB-D, the transmission module is included so that the 
	interactions between two or more distribution systems can be simulated.  

	@subsection transmission Transmission System

	The transmission system is included to allow for the 
	interconnection of multiple distribution feeders.  If a transmission module was not 
	included each distribution system could only be solved independently of other systems.  
	While distribution systems can be solved independently, as is common in current 
	commercial software packages, GridLAB-D will have the ability to generate a power flow 
	solution for multiple distributions systems interconnected via a transmission or 
	sub-transmission network. Traditionally the ability to examine interactions at this 
	level has been limited by computational power. To address this limitation, 
	GridLAB-D is being developed for execution on multiple processor systems. In 
	the current version of GridLAB-D the AC power flow solution method used for the 
	transmission system is the Gauss-Seidel (GS) method, chosen for its inherent ability 
	to solve for poor initial conditions, and to remain numerically stable in 
	multiprocessor environments.

	@subsection distribution Distribution System

	In order to accurately represent the distribution 
	system the individual feeders are expressed in terms of conductor types, 
	conductor placement on poles, underground conductor orientation, phasing, and grounding.  
	GridLAB-D does not simplify the distribution system component models.

	The distribution module of GridLAB-D utilizes the traditional forward and backward 
	sweep method for solving the unbalanced 3-phase AC power flow problem.  This method 
	was selected in lieu of newer methods such as current injection methods for the same 
	reasons that the GS method was selected for the transmission module; 
	converging in the fewest number of iterations is not the primary goal.  Just as 
	with the transmission module the distribution modules will only start with a \e flat \e start 
	at initialization and all subsequent solutions will be derived from the previous time step. 

	Metering is supported for both single/split phase and three phase customers. Support for 
	reclosers, islanding, distributed generation models, and overbuilt lines are anticipated 
	in coming versions.

	The following power distribution system components are implemented and available for use:
	- Overhead and underground lines
	- Transformers
	- Voltage regulators 
	- Fuses 
	- Switches
	- Shunt capacitor banks 

	@section buildings Buildings

	Commercial and residential buildings are implemented using the 
	Equivalent Thermal Parameters model. These are differential models solved for both 
	time as a function of state and state as a function of time.  Currently implemented 
	residential end-uses are:
	- Electric water heaters
	- Refrigerators
	- Dishwashers
	- Clothes washers and dryers
	- Ranges and microwaves
	- Electric plugs and lights
	- Internal gains
	- House loads (including air conditioning, heat pumps, and solar loads) 

	Commercial loads are simulated using an aggregate multi-zone Energy Technology Perspectives 
	(ETP) model that will be enhanced with more detailed end-use behavior in coming versions.

	Please see \ref future "Expanding GridLAB-D" for more information.

	******************************************************************************************
	@page future Expanding GridLAB-D 

	New planned capabilities for GridLAB-D include:
	- Extended quasi-steady state time-series solutions
	- Distributed energy resource models, including appliance-based load shedding technology 
	  and distributed generator and storage models
	- Business and operations simulation tools
	- Retail market modeling tools, including contract selection, to study consumer 
	  behaviors such as  daily, weekly, and seasonal demand profiles, price response, 
	  and contract choice
	- Energy operations such as distribution automation, load-shedding programs, and 
	  emergency operations.
	- Models of SCADA controls, and metering technologies
	- External links to MatlabTM, MySQLTM, MicrosoftTM ExcelTM and AccessTM, and text-based tools
	- External links to SynerGEE's power distribution modeling system
	- Efficient execution on multi-core and multi-processor systems
	- Interfaces to industry-standard power systems tools and analysis systems
	- Graphical user interface for creating input models
	- Graphical user interface for execution and control of the simulation

	The addition of these new capabilities combined with the coupling of GridLAB-D with 
	other tools will offer unparalleled analysis capabilities, including the ability to 
	study phenomenon such as:
	- Rate cases to determine whether the price differentials are sufficient to 
	  encourage customer adoption
	- The potential and benefit of deploying distributed energy resources, such as 
	  Grid-Friendly appliances
	- Business cases for technologies like automated meter reading, distribution automation, 
	  retail markets, or other late-breaking technologies
	- Interactions between multiple technologies, such as how under-frequency load-shedding 
	  remedial action strategies might interact with appliance-based load-relief systems
	- Distribution utility system behavior ranging from a few seconds to decades, simulating 
	  the interaction between physical phenomena, business systems, markets and regional 
	  economics, and consumer behaviors.  

	Simulation results will include many power system statistics, such as reliability 
	metrics (e.g., CAIDI, SAIDI, SAIFI), and business metrics such a profitability, 
	revenue rates of return, and per customer or per line-mile cost metrics.

	Please see \ref applications "GridLAB-D Applications" for more information.

	******************************************************************************************
	@page applications GridLAB-D Applications

	Today's power systems simulation tools don't provide the analysis capabilities 
	needed to study the forces driving change in the energy industry.  The combined 
	influence of fast-changing information technology, novel and cost-effective 
	distributed energy resources, multiple and overlapping energy markets, and new business 
	strategies result in very high uncertainty about the success of these important innovations.  
	Concerns expressed by utility engineers, regulators, various stakeholders, and consumers 
	can be addressed by GridLAB-D.  Some example uses include:

	@section rates Rate structure analysis

	Multiple differentiated energy products based on 
	new rate structure offerings to consumers is very attractive to utilities because 
	it creates the opportunity to reveal demand elasticity and gives utilities the 
	ability to balance supplier market power in the wholesale markets.  The challenge 
	is designing rate structures that are both profitable and attractive to consumers.  
	GridLAB-D will provide the ability to model consumer choice behavior in response 
	to multiple rate offerings (including fixed rates, demand rates, time-of-day rates, 
	and real-time rates) to determine whether a suite of rate offerings is likely to succeed.

	@section dr Distributed resources

	The advent of new distributed energy resource (DER) technologies, 
	such as on-site distributed generation, BCHP and Grid-FriendlyTM appliance controls 
	creates a number of technology opportunities and challenges. GridLAB-D will permit 
	utility managers to better evaluate the cost/benefit trade-off between infrastructure 
	expansion investments and distributed resources investments by including the other 
	economic benefits of DER (e.g., increase wholesale purchasing elasticity, improved 
	reliability metrics, ancillary services products to sell in wholesale markets).

	@section peak Peak load management

	Many peak-shaving programs and emergency curtailment programs 
	have failed to deliver the expected benefits. GridLAB-D can be calibrated to observe 
	consumer behavior to understand its interaction with various peak shaving strategies. 
	The impact of consumer satisfaction on the available of peak-shaving resources can be 
	evaluated and a more accurate forecast of the true available resources can be determined. 
	GridLAB-D will even be able to evaluate the consumer rebound effect following one or more 
	curtailment or load-shed events in a single day.

	@section da Distribution automation design.  

	GridLAB-D will offer capabilities that support the 
	design and analysis of distribution automation technology, to allow utilities to offer 
	heterogeneous reliability within the same system but managing power closer to the point of use.

	Please see \ref opensource "Open Source Distribution" for more information.

	******************************************************************************************
	@page opensource Open Source Distribution

	GridLAB-D will be released as an open-source system to a limited group of charter 
	developers in December 2007.  This release will be the basis for these selected 
	partners to develop modules they propose in response to a Program Opportunity 
	Notice issue by PNNL on behalf of the US DOE.

	A more public release is planned for October 2008, making GridLAB-D widely available 
	to the public under open-source licensing.  Industry partners and collaborators will be 
	invited to examine and test the system, and submit new modules to the open-source 
	library. When modules are vetted and approved by the partners, they will be added 
	to the standard release. Unvetted modules will be available as add-ons from the 
	open-source repository, but will not be installed as a part of the standard download 
	available from the repository.

	Developers are permitted to create proprietary modules, however they will be 
	required to distribute required GridLAB-D components free of charge, and provide 
	prominent acknowledgement of the authors and funding used to create those modules.  
	Any changes made to the open-source modules comprising the GridLAB-D system must be 
	resubmitted to the open-source library for distribution.

	Please see \ref bugs "Reporting problems" for more information

	******************************************************************************************
	@page bugs Reporting problems

	Problems and solutions are managed using <b>TRAC</b>.  When you register on the
	the <b>Subversion</b> site, you will also be registered on the <b>TRAC</b> site.

	Please see \ref todo "To do list" for more information

	@defgroup core GridLAB-D Core

	The GridLAB-D core implements all the major functional components of 
	the GridLAD system.

	@todo Rewrite model compiler using Yacc, Flex, Bison, etc. (ticket #36)
	@todo More granular access control for class variables (ticket #38)
	@todo Need sub-second resolution in convert_from_timestamp (ticket #41)
	@todo Improvements to find.c:\n - Implement regex patterns\n - Trim whitespace in search expression (ticket #42)
	@todo Check macros in gridlabd.h:\n - Some may be unused?\n - Some may be causing problems with compilation on Linux (ticket #43)
	@todo Refine object allocation and implement object CPU affinity so that memory is located "near" processor; crucial for Altix performance (ticket #44)
	@todo Add support in setvar and getvar for more than doubles (ticket #46)
	
	@addtogroup main Main module
	@ingroup core

	The main module handles application entry and exit.

 @{
 **/
#define _MAIN_C

#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <direct.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "globals.h"
#include "legal.h"
#include "cmdarg.h"
#include "module.h"
#include "output.h"
#include "environment.h"
#include "test.h"
#include "random.h"
#include "realtime.h"
#include "save.h"
#include "local.h"
#include "exec.h"
#include "kml.h"

#if defined WIN32 && _DEBUG 
/** Implements a pause on exit capability for Windows consoles
 **/
void pause_at_exit(void) 
{
	if (global_pauseatexit)
		system("pause");
}
#endif

void delete_pidfile(void)
{
	unlink(global_pidfile);
}

/** The main entry point of GridLAB-D
 Exit codes
 - \b 0 run completed ok
 - \b 1 command-line processor stopped
 - \b 2 environment startup failed
 - \b 3 test procedure failed
 - \b 4 user rejects conditions of use
 - \b 5 simulation stopped before completing
 - \b 6 initialization failed
 **/
int main(int argc, /**< the number entries on command-line argument list \p argv */
		 char *argv[]) /**< a list of pointers to the command-line arguments */
{
	global_process_id = getpid();
#if defined WIN32 && _DEBUG 
	atexit(pause_at_exit);
#endif
	/* main initialization */
	if (!exec_init())
		exit(6);

	/* process command line arguments */
	if (cmdarg_load(argc,argv)==FAILED)
	{
		output_fatal("shutdown after command line rejected");
		exit(1);
	}

	/* setup the random number generator */
	random_init();

	/* pidfile */
	if (strcmp(global_pidfile,"")!=0)
	{
		FILE *fp = fopen(global_pidfile,"w");
		if (fp==NULL)
		{
			output_fatal("unable to create pidfile '%s'", global_pidfile);
			exit(1);
		}
#ifdef WIN32
#define getpid _getpid
#endif
		fprintf(fp,"%d\n",getpid());
		output_verbose("process id %d written to %s", getpid(), global_pidfile);
		fclose(fp);
		atexit(delete_pidfile);
	}

	/* do legal stuff */
	if (strcmp(global_pidfile,"")==0 && legal_notice()==FAILED)
		exit(4);

	/* set up the test */
	if (global_test_mode)
	{
#ifndef _NO_CPPUNIT
		output_message("Entering test mode");
		if (test_start(argc,argv)==FAILED)
		{
			output_fatal("shutdown after startup test failed");
			exit(3);
		}
		exit(0); /* There is no environment to speak of, so exit. */
#else
		output_message("Unit Tests not enabled.  Recompile with _NO_CPPUNIT unset");
#endif
	}
	
	/* start the processing environment */
	output_verbose("starting up %s environment", global_environment);
	if (environment_start(argc,argv)==FAILED)
	{
		output_fatal("environment startup failed: %s", strerror(errno));
		exit(2);
	}

	/* post process the test */
	if (global_test_mode)
	{
#ifndef _NO_CPPUNIT
		output_message("Exiting test mode");
		if (test_end(argc,argv)==FAILED)
		{
			output_error("shutdown after end test failed");
			exit(3);
		}
#endif
	}

	/* save the model */
	if (strcmp(global_savefile,"")!=0)
	{
		if (saveall(global_savefile)==FAILED)
			output_error("save to '%s' failed", global_savefile);
	}

	/* do module dumps */
	if (global_dumpall!=FALSE)
	{
		output_verbose("dumping module data");
		module_dumpall();
	}

	/* KML output */
	if (strcmp(global_kmlfile,"")!=0)
		kml_dump(global_kmlfile);

	/* wrap up */
	output_verbose("shutdown complete");

	/* profile results */
	if (global_profiler)
		class_profiles();

	/* restore locale */
	locale_pop();

	/* compute elapsed runtime */
	output_verbose("elapsed runtime %d seconds", realtime_runtime());

	exit(0);
}

/** @} **/
