/** $Id: house_e.cpp,v 1.71 2008/02/13 02:22:35 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house_e.cpp
	@addtogroup house_e
	@ingroup residential

	The house_e object implements a single family home.  The house_e
	only includes the heating/cooling system and the power panel.
	All other end-uses must be explicitly defined and attached to the
	panel using the house_e::attach() method.

	Residential panels use a split secondary transformer:

	@verbatim
		-----)||(------------------ 1    <-- 120V
		     )||(      120V         ^
		1puV )||(----------- 3(N)  240V  <-- 0V
		     )||(      120V         v
		-----)||(------------------ 2    <-- 120V
	@endverbatim

	120V objects are assigned alternatively to circuits 1-3 and 2-3 in the order
	in which they call attach. 240V objects are assigned to circuit 1-2

	Circuit breakers will open on over-current with respect to the maximum current
	given by load when house_e::attach() was called.  After a breaker opens, it is
	reclosed within an average of 5 minutes (on an exponential distribution).  Each
	time the breaker is reclosed, the breaker failure probability is increased.
	The probability of failure is always 1/N where N is initially a large number (e.g., 100). 
	N is progressively decremented until it reaches 1 and the probability of failure is 100%.

	The Equivalent Thermal Parameter (ETP) approach is used to model the residential loads
	and energy consumption.  Solving the ETP model simultaneously for T_{air} and T_{mass},
	the heating/cooling loads can be obtained as a function of time.

	In the current implementation, the HVAC equipment is defined as part of the house_e and
	attached to the electrical panel with a 50 amp/220-240V circuit.

	@par Implicit enduses

	The use of implicit enduses is controlled globally by the #implicit_enduse global variable.
	All houses in the system will employ the same set of global enduses, meaning that the
	loadshape is controlled by the default schedule.

	@par Credits

	The original concept for ETP was developed by Rob Pratt and Todd Taylor around 1990.  
	The first derivation and implementation of the solution was done by Ross Guttromson 
	and David Chassin for PDSS in 2004.

	@par Billing system

	Contract terms are defined according to which contract type is being used.  For
	subsidized and fixed price contracts, the terms are defined using the following format:
	\code
	period=[YQMWDH];fee=[$/period];energy=[c/kWh];
	\endcode

	Time-use contract terms are defined using
	\code
	period=[YQMWDH];fee=[$/period];offpeak=[c/kWh];onpeak=[c/kWh];hours=[int24mask];
	\endcode

	When TOU includes critical peak pricing, use
	\code
	period=[YQMWDH];fee=[$/period];offpeak=[c/kWh];onpeak=[c/kWh];hours=[int24mask]>;critical=[$/kWh];
	\endcode

	RTP is defined using
	\code
	period=[YQMWDH];fee=[$/period];bid_fee=[$/bid];market=[name];
	\endcode

	Demand pricing uses
	\code
	period=[YQMWDH];fee=[$/period];energy=[c/kWh];demand_limit=[kW];demand_price=[$/kW];
	\endcode
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "solvers.h"
#include "house_e.h"
#include "lock.h"
#include "complex.h"

#ifndef WIN32
char *strlwr(char *s)
{
	char *r=s;
	while (*s!='\0') 
	{
		*s = (*s>='A'&&*s<='Z') ? (*s-'A'+'a') : *s;
		s++;
	}
	return r;
}
#endif

// list of enduses that are implicitly active
set house_e::implicit_enduses_active = IEU_ALL;
static double aux_cutin_temperature = 10;

//////////////////////////////////////////////////////////////////////////
// implicit loadshapes - these are enabled by using implicit_enduses global
//////////////////////////////////////////////////////////////////////////
struct s_implicit_enduse_list {
	char *implicit_name;
	struct {
		double breaker_amps; 
		int circuit_is220;
		struct {
			double z, i, p;
		} fractions;
		double power_factor;
		double heat_fraction;
	} load;
	char *shape;
	char *schedule_name;
	char *schedule_definition;
} implicit_enduse_data[] =
{
	// lighting (source: ELCAP lit-sp.dat)
	{	"LIGHTS", 
		{30, false, {0.5,0.1,0.4}, 0.97, 0.9},
		"type:analog; schedule: residential-lights-default; power: 0.76 kW",
		"residential-lights-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.380; *  1 * 4-9 1-5 0.340; *  2 * 4-9 1-5 0.320; *  3 * 4-9 1-5 0.320;"
		"*  4 * 4-9 1-5 0.320; *  5 * 4-9 1-5 0.350; *  6 * 4-9 1-5 0.410; *  7 * 4-9 1-5 0.450;"
		"*  8 * 4-9 1-5 0.450; *  9 * 4-9 1-5 0.450; * 10 * 4-9 1-5 0.450; * 11 * 4-9 1-5 0.450;"
		"* 12 * 4-9 1-5 0.450; * 13 * 4-9 1-5 0.440; * 14 * 4-9 1-5 0.440; * 15 * 4-9 1-5 0.450;"
		"* 16 * 4-9 1-5 0.470; * 17 * 4-9 1-5 0.510; * 18 * 4-9 1-5 0.540; * 19 * 4-9 1-5 0.560;"
		"* 20 * 4-9 1-5 0.630; * 21 * 4-9 1-5 0.710; * 22 * 4-9 1-5 0.650; * 23 * 4-9 1-5 0.490"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.410; *  1 * 4-9 6-0 0.360; *  2 * 4-9 6-0 0.330; *  3 * 4-9 6-0 0.320;"
		"*  4 * 4-9 6-0 0.320; *  5 * 4-9 6-0 0.320; *  6 * 4-9 6-0 0.340; *  7 * 4-9 6-0 0.390;"
		"*  8 * 4-9 6-0 0.440; *  9 * 4-9 6-0 0.470; * 10 * 4-9 6-0 0.470; * 11 * 4-9 6-0 0.470;"
		"* 12 * 4-9 6-0 0.470; * 13 * 4-9 6-0 0.460; * 14 * 4-9 6-0 0.460; * 15 * 4-9 6-0 0.460;"
		"* 16 * 4-9 6-0 0.470; * 17 * 4-9 6-0 0.490; * 18 * 4-9 6-0 0.520; * 19 * 4-9 6-0 0.540;"
		"* 20 * 4-9 6-0 0.610; * 21 * 4-9 6-0 0.680; * 22 * 4-9 6-0 0.630; * 23 * 4-9 6-0 0.500"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.4200; *  1 * 10-3 1-5 0.3800; *  2 * 10-3 1-5 0.3700; *  3 * 10-3 1-5 0.3600;"
		"*  4 * 10-3 1-5 0.3700; *  5 * 10-3 1-5 0.4200; *  6 * 10-3 1-5 0.5800; *  7 * 10-3 1-5 0.6900;"
		"*  8 * 10-3 1-5 0.6100; *  9 * 10-3 1-5 0.5600; * 10 * 10-3 1-5 0.5300; * 11 * 10-3 1-5 0.5100;"
		"* 12 * 10-3 1-5 0.4900; * 13 * 10-3 1-5 0.4700; * 14 * 10-3 1-5 0.4700; * 15 * 10-3 1-5 0.5100;"
		"* 16 * 10-3 1-5 0.6300; * 17 * 10-3 1-5 0.8400; * 18 * 10-3 1-5 0.9700; * 19 * 10-3 1-5 0.9800;"
		"* 20 * 10-3 1-5 0.9600; * 21 * 10-3 1-5 0.8900; * 22 * 10-3 1-5 0.7400; * 23 * 10-3 1-5 0.5500"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.4900; *  1 * 10-3 6-0 0.4200; *  2 * 10-3 6-0 0.3800; *  3 * 10-3 6-0 0.3800;"
		"*  4 * 10-3 6-0 0.3700; *  5 * 10-3 6-0 0.3800; *  6 * 10-3 6-0 0.4300; *  7 * 10-3 6-0 0.5100;"
		"*  8 * 10-3 6-0 0.6000; *  9 * 10-3 6-0 0.6300; * 10 * 10-3 6-0 0.6300; * 11 * 10-3 6-0 0.6100;"
		"* 12 * 10-3 6-0 0.6000; * 13 * 10-3 6-0 0.5900; * 14 * 10-3 6-0 0.5900; * 15 * 10-3 6-0 0.6100;"
		"* 16 * 10-3 6-0 0.7100; * 17 * 10-3 6-0 0.8800; * 18 * 10-3 6-0 0.9600; * 19 * 10-3 6-0 0.9700;"
		"* 20 * 10-3 6-0 0.9400; * 21 * 10-3 6-0 0.8800; * 22 * 10-3 6-0 0.7600; * 23 * 10-3 6-0 0.5800"
		"}"
		},
		// Plugs (source: ELCAP lit-sp.dat)
	{	"PLUGS", 
		{30, false, {0.0,0.0,1.0}, 0.90, 0.9},
		"type:analog; schedule: residential-plugs-default; power: 0.36 kW",
		"residential-plugs-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.380; *  1 * 4-9 1-5 0.340; *  2 * 4-9 1-5 0.320; *  3 * 4-9 1-5 0.320;"
		"*  4 * 4-9 1-5 0.320; *  5 * 4-9 1-5 0.350; *  6 * 4-9 1-5 0.410; *  7 * 4-9 1-5 0.450;"
		"*  8 * 4-9 1-5 0.450; *  9 * 4-9 1-5 0.450; * 10 * 4-9 1-5 0.450; * 11 * 4-9 1-5 0.450;"
		"* 12 * 4-9 1-5 0.450; * 13 * 4-9 1-5 0.440; * 14 * 4-9 1-5 0.440; * 15 * 4-9 1-5 0.450;"
		"* 16 * 4-9 1-5 0.470; * 17 * 4-9 1-5 0.510; * 18 * 4-9 1-5 0.540; * 19 * 4-9 1-5 0.560;"
		"* 20 * 4-9 1-5 0.630; * 21 * 4-9 1-5 0.710; * 22 * 4-9 1-5 0.650; * 23 * 4-9 1-5 0.490"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.410; *  1 * 4-9 6-0 0.360; *  2 * 4-9 6-0 0.330; *  3 * 4-9 6-0 0.320;"
		"*  4 * 4-9 6-0 0.320; *  5 * 4-9 6-0 0.320; *  6 * 4-9 6-0 0.340; *  7 * 4-9 6-0 0.390;"
		"*  8 * 4-9 6-0 0.440; *  9 * 4-9 6-0 0.470; * 10 * 4-9 6-0 0.470; * 11 * 4-9 6-0 0.470;"
		"* 12 * 4-9 6-0 0.470; * 13 * 4-9 6-0 0.460; * 14 * 4-9 6-0 0.460; * 15 * 4-9 6-0 0.460;"
		"* 16 * 4-9 6-0 0.470; * 17 * 4-9 6-0 0.490; * 18 * 4-9 6-0 0.520; * 19 * 4-9 6-0 0.540;"
		"* 20 * 4-9 6-0 0.610; * 21 * 4-9 6-0 0.680; * 22 * 4-9 6-0 0.630; * 23 * 4-9 6-0 0.500"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.4200; *  1 * 10-3 1-5 0.3800; *  2 * 10-3 1-5 0.3700; *  3 * 10-3 1-5 0.3600;"
		"*  4 * 10-3 1-5 0.3700; *  5 * 10-3 1-5 0.4200; *  6 * 10-3 1-5 0.5800; *  7 * 10-3 1-5 0.6900;"
		"*  8 * 10-3 1-5 0.6100; *  9 * 10-3 1-5 0.5600; * 10 * 10-3 1-5 0.5300; * 11 * 10-3 1-5 0.5100;"
		"* 12 * 10-3 1-5 0.4900; * 13 * 10-3 1-5 0.4700; * 14 * 10-3 1-5 0.4700; * 15 * 10-3 1-5 0.5100;"
		"* 16 * 10-3 1-5 0.6300; * 17 * 10-3 1-5 0.8400; * 18 * 10-3 1-5 0.9700; * 19 * 10-3 1-5 0.9800;"
		"* 20 * 10-3 1-5 0.9600; * 21 * 10-3 1-5 0.8900; * 22 * 10-3 1-5 0.7400; * 23 * 10-3 1-5 0.5500"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.4900; *  1 * 10-3 6-0 0.4200; *  2 * 10-3 6-0 0.3800; *  3 * 10-3 6-0 0.3800;"
		"*  4 * 10-3 6-0 0.3700; *  5 * 10-3 6-0 0.3800; *  6 * 10-3 6-0 0.4300; *  7 * 10-3 6-0 0.5100;"
		"*  8 * 10-3 6-0 0.6000; *  9 * 10-3 6-0 0.6300; * 10 * 10-3 6-0 0.6300; * 11 * 10-3 6-0 0.6100;"
		"* 12 * 10-3 6-0 0.6000; * 13 * 10-3 6-0 0.5900; * 14 * 10-3 6-0 0.5900; * 15 * 10-3 6-0 0.6100;"
		"* 16 * 10-3 6-0 0.7100; * 17 * 10-3 6-0 0.8800; * 18 * 10-3 6-0 0.9600; * 19 * 10-3 6-0 0.9700;"
		"* 20 * 10-3 6-0 0.9400; * 21 * 10-3 6-0 0.8800; * 22 * 10-3 6-0 0.7600; * 23 * 10-3 6-0 0.5800"
		"}"
		},

	{   "CLOTHESWASHER", 
		{20, false, {0.0,0.0,1.0}, 0.9, 1.0},
		"type:analog; schedule: residential-clotheswasher-default; energy: 0.75 kWh; power: 1 kW",
		"residential-clotheswasher-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.0029; *  1 * 4-9 1-5 0.0019; *  2 * 4-9 1-5 0.0014; *  3 * 4-9 1-5 0.0013;"
		"*  4 * 4-9 1-5 0.0018; *  5 * 4-9 1-5 0.0026; *  6 * 4-9 1-5 0.0055; *  7 * 4-9 1-5 0.0126;"
		"*  8 * 4-9 1-5 0.0181; *  9 * 4-9 1-5 0.0208; * 10 * 4-9 1-5 0.0229; * 11 * 4-9 1-5 0.0216;"
		"* 12 * 4-9 1-5 0.0193; * 13 * 4-9 1-5 0.0170; * 14 * 4-9 1-5 0.0145; * 15 * 4-9 1-5 0.0135;"
		"* 16 * 4-9 1-5 0.0135; * 17 * 4-9 1-5 0.0142; * 18 * 4-9 1-5 0.0145; * 19 * 4-9 1-5 0.0148;"
		"* 20 * 4-9 1-5 0.0146; * 21 * 4-9 1-5 0.0141; * 22 * 4-9 1-5 0.0110; * 23 * 4-9 1-5 0.0062"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.0031; *  1 * 4-9 6-0 0.0019; *  2 * 4-9 6-0 0.0013; *  3 * 4-9 6-0 0.0012;"
		"*  4 * 4-9 6-0 0.0012; *  5 * 4-9 6-0 0.0016; *  6 * 4-9 6-0 0.0027; *  7 * 4-9 6-0 0.0066;"
		"*  8 * 4-9 6-0 0.0157; *  9 * 4-9 6-0 0.0220; * 10 * 4-9 6-0 0.0258; * 11 * 4-9 6-0 0.0251;"
		"* 12 * 4-9 6-0 0.0231; * 13 * 4-9 6-0 0.0217; * 14 * 4-9 6-0 0.0186; * 15 * 4-9 6-0 0.0157;"
		"* 16 * 4-9 6-0 0.0156; * 17 * 4-9 6-0 0.0151; * 18 * 4-9 6-0 0.0147; * 19 * 4-9 6-0 0.0150;"
		"* 20 * 4-9 6-0 0.0156; * 21 * 4-9 6-0 0.0148; * 22 * 4-9 6-0 0.0106; * 23 * 4-9 6-0 0.0065"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.0036; *  1 * 10-3 1-5 0.0024; *  2 * 10-3 1-5 0.0020; *  3 * 10-3 1-5 0.0019;"
		"*  4 * 10-3 1-5 0.0026; *  5 * 10-3 1-5 0.0040; *  6 * 10-3 1-5 0.0062; *  7 * 10-3 1-5 0.0118;"
		"*  8 * 10-3 1-5 0.0177; *  9 * 10-3 1-5 0.0211; * 10 * 10-3 1-5 0.0215; * 11 * 10-3 1-5 0.0203;"
		"* 12 * 10-3 1-5 0.0176; * 13 * 10-3 1-5 0.0155; * 14 * 10-3 1-5 0.0133; * 15 * 10-3 1-5 0.0130;"
		"* 16 * 10-3 1-5 0.0145; * 17 * 10-3 1-5 0.0159; * 18 * 10-3 1-5 0.0166; * 19 * 10-3 1-5 0.0164;"
		"* 20 * 10-3 1-5 0.0154; * 21 * 10-3 1-5 0.0149; * 22 * 10-3 1-5 0.0110; * 23 * 10-3 1-5 0.0065"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.0044; *  1 * 10-3 6-0 0.0030; *  2 * 10-3 6-0 0.0022; *  3 * 10-3 6-0 0.0020;"
		"*  4 * 10-3 6-0 0.0021; *  5 * 10-3 6-0 0.0021; *  6 * 10-3 6-0 0.0030; *  7 * 10-3 6-0 0.0067;"
		"*  8 * 10-3 6-0 0.0145; *  9 * 10-3 6-0 0.0244; * 10 * 10-3 6-0 0.0310; * 11 * 10-3 6-0 0.0323;"
		"* 12 * 10-3 6-0 0.0308; * 13 * 10-3 6-0 0.0285; * 14 * 10-3 6-0 0.0251; * 15 * 10-3 6-0 0.0224;"
		"* 16 * 10-3 6-0 0.0215; * 17 * 10-3 6-0 0.0203; * 18 * 10-3 6-0 0.0194; * 19 * 10-3 6-0 0.0188;"
		"* 20 * 10-3 6-0 0.0180; * 21 * 10-3 6-0 0.0151; * 22 * 10-3 6-0 0.0122; * 23 * 10-3 6-0 0.0073"
		"}"		
		},

	{   "WATERHEATER", 
		{30, true, {0.0,0.0,1.0}, 1.0, 0.5},
		"type:analog; schedule: residential-waterheater-default; energy: 1 kWh; power: 5 kW",
		"residential-waterheater-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.21; *  1 * 4-9 1-5 0.16; *  2 * 4-9 1-5 0.13; *  3 * 4-9 1-5 0.12;"
		"*  4 * 4-9 1-5 0.15; *  5 * 4-9 1-5 0.26; *  6 * 4-9 1-5 0.51; *  7 * 4-9 1-5 0.76;"
		"*  8 * 4-9 1-5 0.77; *  9 * 4-9 1-5 0.76; * 10 * 4-9 1-5 0.71; * 11 * 4-9 1-5 0.61;"
		"* 12 * 4-9 1-5 0.54; * 13 * 4-9 1-5 0.49; * 14 * 4-9 1-5 0.43; * 15 * 4-9 1-5 0.41;"
		"* 16 * 4-9 1-5 0.43; * 17 * 4-9 1-5 0.52; * 18 * 4-9 1-5 0.60; * 19 * 4-9 1-5 0.60;"
		"* 20 * 4-9 1-5 0.59; * 21 * 4-9 1-5 0.60; * 22 * 4-9 1-5 0.55; * 23 * 4-9 1-5 0.37"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.23; *  1 * 4-9 6-0 0.17; *  2 * 4-9 6-0 0.14; *  3 * 4-9 6-0 0.13;"
		"*  4 * 4-9 6-0 0.13; *  5 * 4-9 6-0 0.17; *  6 * 4-9 6-0 0.26; *  7 * 4-9 6-0 0.45;"
		"*  8 * 4-9 6-0 0.69; *  9 * 4-9 6-0 0.85; * 10 * 4-9 6-0 0.84; * 11 * 4-9 6-0 0.76;"
		"* 12 * 4-9 6-0 0.65; * 13 * 4-9 6-0 0.58; * 14 * 4-9 6-0 0.49; * 15 * 4-9 6-0 0.46;"
		"* 16 * 4-9 6-0 0.46; * 17 * 4-9 6-0 0.50; * 18 * 4-9 6-0 0.54; * 19 * 4-9 6-0 0.55;"
		"* 20 * 4-9 6-0 0.56; * 21 * 4-9 6-0 0.56; * 22 * 4-9 6-0 0.49; * 23 * 4-9 6-0 0.38"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.25; *  1 * 10-3 1-5 0.19; *  2 * 10-3 1-5 0.16; *  3 * 10-3 1-5 0.15;" 
		"*  4 * 10-3 1-5 0.18; *  5 * 10-3 1-5 0.34; *  6 * 10-3 1-5 0.74; *  7 * 10-3 1-5 1.20;"
		"*  8 * 10-3 1-5 1.10; *  9 * 10-3 1-5 0.94; * 10 * 10-3 1-5 0.82; * 11 * 10-3 1-5 0.71;"
		"* 12 * 10-3 1-5 0.62; * 13 * 10-3 1-5 0.55; * 14 * 10-3 1-5 0.48; * 15 * 10-3 1-5 0.47;"
		"* 16 * 10-3 1-5 0.54; * 17 * 10-3 1-5 0.68; * 18 * 10-3 1-5 0.83; * 19 * 10-3 1-5 0.82;"
		"* 20 * 10-3 1-5 0.74; * 21 * 10-3 1-5 0.68; * 22 * 10-3 1-5 0.57; * 23 * 10-3 1-5 0.40"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.29; *  1 * 10-3 6-0 0.22; *  2 * 10-3 6-0 0.17; *  3 * 10-3 6-0 0.15;"
		"*  4 * 10-3 6-0 0.16; *  5 * 10-3 6-0 0.19; *  6 * 10-3 6-0 0.27; *  7 * 10-3 6-0 0.47;" 
		"*  8 * 10-3 6-0 0.82; *  9 * 10-3 6-0 1.08; * 10 * 10-3 6-0 1.15; * 11 * 10-3 6-0 1.08;"
		"* 12 * 10-3 6-0 0.98; * 13 * 10-3 6-0 0.87; * 14 * 10-3 6-0 0.77; * 15 * 10-3 6-0 0.69;"
		"* 16 * 10-3 6-0 0.72; * 17 * 10-3 6-0 0.78; * 18 * 10-3 6-0 0.83; * 19 * 10-3 6-0 0.79;"
		"* 20 * 10-3 6-0 0.72; * 21 * 10-3 6-0 0.64; * 22 * 10-3 6-0 0.53; * 23 * 10-3 6-0 0.43"
		"}"		
		},

	{   "REFRIGERATOR", 
		{20, false, {0.1,0.0,0.9}, 0.9, 1.0},
		"type:analog; schedule: residential-refrigerator-default; energy: 1 kWh; power: 750 W",
		"residential-refrigerator-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.187; *  1 * 4-9 1-5 0.182; *  2 * 4-9 1-5 0.176; *  3 * 4-9 1-5 0.170;"
		"*  4 * 4-9 1-5 0.168; *  5 * 4-9 1-5 0.168; *  6 * 4-9 1-5 0.177; *  7 * 4-9 1-5 0.174;"
		"*  8 * 4-9 1-5 0.177; *  9 * 4-9 1-5 0.180; * 10 * 4-9 1-5 0.180; * 11 * 4-9 1-5 0.183;"
		"* 12 * 4-9 1-5 0.192; * 13 * 4-9 1-5 0.192; * 14 * 4-9 1-5 0.194; * 15 * 4-9 1-5 0.196;"
		"* 16 * 4-9 1-5 0.205; * 17 * 4-9 1-5 0.217; * 18 * 4-9 1-5 0.225; * 19 * 4-9 1-5 0.221;"
		"* 20 * 4-9 1-5 0.216; * 21 * 4-9 1-5 0.214; * 22 * 4-9 1-5 0.207; * 23 * 4-9 1-5 0.195"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.187; *  1 * 4-9 6-0 0.181; *  2 * 4-9 6-0 0.176; *  3 * 4-9 6-0 0.169;"
		"*  4 * 4-9 6-0 0.166; *  5 * 4-9 6-0 0.164; *  6 * 4-9 6-0 0.167; *  7 * 4-9 6-0 0.169;"
		"*  8 * 4-9 6-0 0.180; *  9 * 4-9 6-0 0.184; * 10 * 4-9 6-0 0.187; * 11 * 4-9 6-0 0.187;"
		"* 12 * 4-9 6-0 0.195; * 13 * 4-9 6-0 0.200; * 14 * 4-9 6-0 0.201; * 15 * 4-9 6-0 0.203;"
		"* 16 * 4-9 6-0 0.209; * 17 * 4-9 6-0 0.218; * 18 * 4-9 6-0 0.222; * 19 * 4-9 6-0 0.221;"
		"* 20 * 4-9 6-0 0.217; * 21 * 4-9 6-0 0.216; * 22 * 4-9 6-0 0.207; * 23 * 4-9 6-0 0.196"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.1530; *  1 * 10-3 1-5 0.1500; *  2 * 10-3 1-5 0.1460; *  3 * 10-3 1-5 0.1420;"
		"*  4 * 10-3 1-5 0.1400; *  5 * 10-3 1-5 0.1450; *  6 * 10-3 1-5 0.1520; *  7 * 10-3 1-5 0.1600;"
		"*  8 * 10-3 1-5 0.1580; *  9 * 10-3 1-5 0.1580; * 10 * 10-3 1-5 0.1560; * 11 * 10-3 1-5 0.1560;"
		"* 12 * 10-3 1-5 0.1630; * 13 * 10-3 1-5 0.1620; * 14 * 10-3 1-5 0.1590; * 15 * 10-3 1-5 0.1620;"
		"* 16 * 10-3 1-5 0.1690; * 17 * 10-3 1-5 0.1850; * 18 * 10-3 1-5 0.1920; * 19 * 10-3 1-5 0.1820;"
		"* 20 * 10-3 1-5 0.1800; * 21 * 10-3 1-5 0.1760; * 22 * 10-3 1-5 0.1670; * 23 * 10-3 1-5 0.1590"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.1560; *  1 * 10-3 6-0 0.1520; *  2 * 10-3 6-0 0.1470; *  3 * 10-3 6-0 0.1430;"
		"*  4 * 10-3 6-0 0.1420; *  5 * 10-3 6-0 0.1430; *  6 * 10-3 6-0 0.1430; *  7 * 10-3 6-0 0.1500;"
		"*  8 * 10-3 6-0 0.1610; *  9 * 10-3 6-0 0.1690; * 10 * 10-3 6-0 0.1670; * 11 * 10-3 6-0 0.1660;"
		"* 12 * 10-3 6-0 0.1740; * 13 * 10-3 6-0 0.1760; * 14 * 10-3 6-0 0.1740; * 15 * 10-3 6-0 0.1750;"
		"* 16 * 10-3 6-0 0.1790; * 17 * 10-3 6-0 0.1910; * 18 * 10-3 6-0 0.1930; * 19 * 10-3 6-0 0.1870;"
		"* 20 * 10-3 6-0 0.1840; * 21 * 10-3 6-0 0.1780; * 22 * 10-3 6-0 0.1700; * 23 * 10-3 6-0 0.1600"
		"}"		
		},

	{   "DRYER", 
		{30, true, {0.9,0.0,0.1}, 0.99, 0.15},
		"type:analog; schedule: residential-dryer-default; energy: 2.5 kWh; power: 5 kW",
		"residential-dryer-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.036; *  1 * 4-9 1-5 0.013; *  2 * 4-9 1-5 0.007; *  3 * 4-9 1-5 0.005;"
		"*  4 * 4-9 1-5 0.005; *  5 * 4-9 1-5 0.017; *  6 * 4-9 1-5 0.048; *  7 * 4-9 1-5 0.085;"
		"*  8 * 4-9 1-5 0.115; *  9 * 4-9 1-5 0.156; * 10 * 4-9 1-5 0.179; * 11 * 4-9 1-5 0.185;"
		"* 12 * 4-9 1-5 0.172; * 13 * 4-9 1-5 0.162; * 14 * 4-9 1-5 0.145; * 15 * 4-9 1-5 0.136;"
		"* 16 * 4-9 1-5 0.133; * 17 * 4-9 1-5 0.134; * 18 * 4-9 1-5 0.127; * 19 * 4-9 1-5 0.130;"
		"* 20 * 4-9 1-5 0.141; * 21 * 4-9 1-5 0.154; * 22 * 4-9 1-5 0.138; * 23 * 4-9 1-5 0.083"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.041; *  1 * 4-9 6-0 0.017; *  2 * 4-9 6-0 0.008; *  3 * 4-9 6-0 0.005;"
		"*  4 * 4-9 6-0 0.005; *  5 * 4-9 6-0 0.007; *  6 * 4-9 6-0 0.018; *  7 * 4-9 6-0 0.047;"
		"*  8 * 4-9 6-0 0.100; *  9 * 4-9 6-0 0.168; * 10 * 4-9 6-0 0.205; * 11 * 4-9 6-0 0.220;"
		"* 12 * 4-9 6-0 0.211; * 13 * 4-9 6-0 0.210; * 14 * 4-9 6-0 0.188; * 15 * 4-9 6-0 0.168;"
		"* 16 * 4-9 6-0 0.154; * 17 * 4-9 6-0 0.146; * 18 * 4-9 6-0 0.138; * 19 * 4-9 6-0 0.137;"
		"* 20 * 4-9 6-0 0.144; * 21 * 4-9 6-0 0.155; * 22 * 4-9 6-0 0.131; * 23 * 4-9 6-0 0.081"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.0360; *  1 * 10-3 1-5 0.0160; *  2 * 10-3 1-5 0.0100; *  3 * 10-3 1-5 0.0070;"
		"*  4 * 10-3 1-5 0.0090; *  5 * 10-3 1-5 0.0230; *  6 * 10-3 1-5 0.0610; *  7 * 10-3 1-5 0.1030;"
		"*  8 * 10-3 1-5 0.1320; *  9 * 10-3 1-5 0.1750; * 10 * 10-3 1-5 0.2050; * 11 * 10-3 1-5 0.2130;"
		"* 12 * 10-3 1-5 0.1940; * 13 * 10-3 1-5 0.1770; * 14 * 10-3 1-5 0.1610; * 15 * 10-3 1-5 0.1560;"
		"* 16 * 10-3 1-5 0.1640; * 17 * 10-3 1-5 0.1710; * 18 * 10-3 1-5 0.1610; * 19 * 10-3 1-5 0.1590;"
		"* 20 * 10-3 1-5 0.1670; * 21 * 10-3 1-5 0.1690; * 22 * 10-3 1-5 0.1380; * 23 * 10-3 1-5 0.0820"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.0390; *  1 * 10-3 6-0 0.0190; *  2 * 10-3 6-0 0.0110; *  3 * 10-3 6-0 0.0070;"
		"*  4 * 10-3 6-0 0.0080; *  5 * 10-3 6-0 0.0090; *  6 * 10-3 6-0 0.0160; *  7 * 10-3 6-0 0.0430;"
		"*  8 * 10-3 6-0 0.1010; *  9 * 10-3 6-0 0.1810; * 10 * 10-3 6-0 0.2640; * 11 * 10-3 6-0 0.3050;"
		"* 12 * 10-3 6-0 0.3110; * 13 * 10-3 6-0 0.3060; * 14 * 10-3 6-0 0.2850; * 15 * 10-3 6-0 0.2700;"
		"* 16 * 10-3 6-0 0.2600; * 17 * 10-3 6-0 0.2450; * 18 * 10-3 6-0 0.2200; * 19 * 10-3 6-0 0.1980;"
		"* 20 * 10-3 6-0 0.1880; * 21 * 10-3 6-0 0.1790; * 22 * 10-3 6-0 0.1480; * 23 * 10-3 6-0 0.0930"
		"}"		
		},

	{   "FREEZER", 
		{20, false, {0.1,0.0,0.9}, 0.9, 1.0},
		"type:analog; schedule: residential-freezer-default; energy: 750 Wh; power: 500 W",
		"residential-freezer-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.210; *  1 * 4-9 1-5 0.213; *  2 * 4-9 1-5 0.208; *  3 * 4-9 1-5 0.202;"
		"*  4 * 4-9 1-5 0.203; *  5 * 4-9 1-5 0.198; *  6 * 4-9 1-5 0.190; *  7 * 4-9 1-5 0.186;"
		"*  8 * 4-9 1-5 0.189; *  9 * 4-9 1-5 0.194; * 10 * 4-9 1-5 0.199; * 11 * 4-9 1-5 0.202;"
		"* 12 * 4-9 1-5 0.211; * 13 * 4-9 1-5 0.214; * 14 * 4-9 1-5 0.219; * 15 * 4-9 1-5 0.222;"
		"* 16 * 4-9 1-5 0.230; * 17 * 4-9 1-5 0.228; * 18 * 4-9 1-5 0.229; * 19 * 4-9 1-5 0.223;"
		"* 20 * 4-9 1-5 0.224; * 21 * 4-9 1-5 0.223; * 22 * 4-9 1-5 0.218; * 23 * 4-9 1-5 0.214"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.203; *  1 * 4-9 6-0 0.202; *  2 * 4-9 6-0 0.202; *  3 * 4-9 6-0 0.193;"
		"*  4 * 4-9 6-0 0.198; *  5 * 4-9 6-0 0.195; *  6 * 4-9 6-0 0.191; *  7 * 4-9 6-0 0.183;"
		"*  8 * 4-9 6-0 0.184; *  9 * 4-9 6-0 0.192; * 10 * 4-9 6-0 0.197; * 11 * 4-9 6-0 0.202;"
		"* 12 * 4-9 6-0 0.208; * 13 * 4-9 6-0 0.219; * 14 * 4-9 6-0 0.219; * 15 * 4-9 6-0 0.225;"
		"* 16 * 4-9 6-0 0.225; * 17 * 4-9 6-0 0.225; * 18 * 4-9 6-0 0.223; * 19 * 4-9 6-0 0.219;"
		"* 20 * 4-9 6-0 0.221; * 21 * 4-9 6-0 0.220; * 22 * 4-9 6-0 0.215; * 23 * 4-9 6-0 0.209"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.149; *  1 * 10-3 1-5 0.148; *  2 * 10-3 1-5 0.145; *  3 * 10-3 1-5 0.144;" 
		"*  4 * 10-3 1-5 0.143; *  5 * 10-3 1-5 0.140; *  6 * 10-3 1-5 0.138; *  7 * 10-3 1-5 0.138;" 
		"*  8 * 10-3 1-5 0.140; *  9 * 10-3 1-5 0.141; * 10 * 10-3 1-5 0.142; * 11 * 10-3 1-5 0.147;"
		"* 12 * 10-3 1-5 0.153; * 13 * 10-3 1-5 0.154; * 14 * 10-3 1-5 0.152; * 15 * 10-3 1-5 0.151;"
		"* 16 * 10-3 1-5 0.161; * 17 * 10-3 1-5 0.174; * 18 * 10-3 1-5 0.176; * 19 * 10-3 1-5 0.176;"
		"* 20 * 10-3 1-5 0.175; * 21 * 10-3 1-5 0.169; * 22 * 10-3 1-5 0.160; * 23 * 10-3 1-5 0.153"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.155; *  1 * 10-3 6-0 0.150; *  2 * 10-3 6-0 0.143; *  3 * 10-3 6-0 0.141;"
		"*  4 * 10-3 6-0 0.141; *  5 * 10-3 6-0 0.139; *  6 * 10-3 6-0 0.138; *  7 * 10-3 6-0 0.139;"
		"*  8 * 10-3 6-0 0.142; *  9 * 10-3 6-0 0.142; * 10 * 10-3 6-0 0.145; * 11 * 10-3 6-0 0.153;"
		"* 12 * 10-3 6-0 0.161; * 13 * 10-3 6-0 0.162; * 14 * 10-3 6-0 0.160; * 15 * 10-3 6-0 0.161;"
		"* 16 * 10-3 6-0 0.165; * 17 * 10-3 6-0 0.177; * 18 * 10-3 6-0 0.179; * 19 * 10-3 6-0 0.177;"
		"* 20 * 10-3 6-0 0.171; * 21 * 10-3 6-0 0.168; * 22 * 10-3 6-0 0.160; * 23 * 10-3 6-0 0.151"
		"}"		
		},

	{   "DISHWASHER", 
		{20, false, {0.8,0,0.2}, 0.98, 1.0},
		"type:analog; schedule: residential-dishwasher-default; energy: 1.0 kWh; power: 1.0 kW",
		"residential-dishwasher-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.0068; *  1 * 4-9 1-5 0.0029; *  2 * 4-9 1-5 0.0016; *  3 * 4-9 1-5 0.0013;"
		"*  4 * 4-9 1-5 0.0012; *  5 * 4-9 1-5 0.0037; *  6 * 4-9 1-5 0.0075; *  7 * 4-9 1-5 0.0129;"
		"*  8 * 4-9 1-5 0.0180; *  9 * 4-9 1-5 0.0177; * 10 * 4-9 1-5 0.0144; * 11 * 4-9 1-5 0.0113;"
		"* 12 * 4-9 1-5 0.0116; * 13 * 4-9 1-5 0.0128; * 14 * 4-9 1-5 0.0109; * 15 * 4-9 1-5 0.0105;"
		"* 16 * 4-9 1-5 0.0124; * 17 * 4-9 1-5 0.0156; * 18 * 4-9 1-5 0.0278; * 19 * 4-9 1-5 0.0343;"
		"* 20 * 4-9 1-5 0.0279; * 21 * 4-9 1-5 0.0234; * 22 * 4-9 1-5 0.0194; * 23 * 4-9 1-5 0.0131"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.0093; *  1 * 4-9 6-0 0.0045; *  2 * 4-9 6-0 0.0021; *  3 * 4-9 6-0 0.0015;"
		"*  4 * 4-9 6-0 0.0013; *  5 * 4-9 6-0 0.0015; *  6 * 4-9 6-0 0.0026; *  7 * 4-9 6-0 0.0067;"
		"*  8 * 4-9 6-0 0.0142; *  9 * 4-9 6-0 0.0221; * 10 * 4-9 6-0 0.0259; * 11 * 4-9 6-0 0.0238;"
		"* 12 * 4-9 6-0 0.0214; * 13 * 4-9 6-0 0.0214; * 14 * 4-9 6-0 0.0188; * 15 * 4-9 6-0 0.0169;"
		"* 16 * 4-9 6-0 0.0156; * 17 * 4-9 6-0 0.0166; * 18 * 4-9 6-0 0.0249; * 19 * 4-9 6-0 0.0298;"
		"* 20 * 4-9 6-0 0.0267; * 21 * 4-9 6-0 0.0221; * 22 * 4-9 6-0 0.0174; * 23 * 4-9 6-0 0.0145"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.0068; *  1 * 10-3 1-5 0.0029; *  2 * 10-3 1-5 0.0016; *  3 * 10-3 1-5 0.0013;"
		"*  4 * 10-3 1-5 0.0012; *  5 * 10-3 1-5 0.0037; *  6 * 10-3 1-5 0.0075; *  7 * 10-3 1-5 0.0129;"
		"*  8 * 10-3 1-5 0.0180; *  9 * 10-3 1-5 0.0177; * 10 * 10-3 1-5 0.0144; * 11 * 10-3 1-5 0.0113;"
		"* 12 * 10-3 1-5 0.0116; * 13 * 10-3 1-5 0.0128; * 14 * 10-3 1-5 0.0109; * 15 * 10-3 1-5 0.0105;"
		"* 16 * 10-3 1-5 0.0124; * 17 * 10-3 1-5 0.0156; * 18 * 10-3 1-5 0.0278; * 19 * 10-3 1-5 0.0343;"
		"* 20 * 10-3 1-5 0.0279; * 21 * 10-3 1-5 0.0234; * 22 * 10-3 1-5 0.0194; * 23 * 10-3 1-5 0.0131"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.0093; *  1 * 10-3 6-0 0.0045; *  2 * 10-3 6-0 0.0021; *  3 * 10-3 6-0 0.0015;"
		"*  4 * 10-3 6-0 0.0013; *  5 * 10-3 6-0 0.0015; *  6 * 10-3 6-0 0.0026; *  7 * 10-3 6-0 0.0067;"
		"*  8 * 10-3 6-0 0.0142; *  9 * 10-3 6-0 0.0221; * 10 * 10-3 6-0 0.0259; * 11 * 10-3 6-0 0.0238;"
		"* 12 * 10-3 6-0 0.0214; * 13 * 10-3 6-0 0.0214; * 14 * 10-3 6-0 0.0188; * 15 * 10-3 6-0 0.0169;"
		"* 16 * 10-3 6-0 0.0156; * 17 * 10-3 6-0 0.0166; * 18 * 10-3 6-0 0.0249; * 19 * 10-3 6-0 0.0298;"
		"* 20 * 10-3 6-0 0.0267; * 21 * 10-3 6-0 0.0221; * 22 * 10-3 6-0 0.0174; * 23 * 10-3 6-0 0.0145"
		"}"		
	},
	{   "RANGE", 
		{40, true, {1,0,0}, 0.85, 0.8},
		"type:analog; schedule: residential-range-default; energy: 1.0 kWh; power: 0.5 kW",
		"residential-range-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.009; *  1 * 4-9 1-5 0.008; *  2 * 4-9 1-5 0.007; *  3 * 4-9 1-5 0.007"
		"*  4 * 4-9 1-5 0.008; *  5 * 4-9 1-5 0.012; *  6 * 4-9 1-5 0.025; *  7 * 4-9 1-5 0.040"
		"*  8 * 4-9 1-5 0.044; *  9 * 4-9 1-5 0.042; * 10 * 4-9 1-5 0.042; * 11 * 4-9 1-5 0.053"
		"* 12 * 4-9 1-5 0.057; * 13 * 4-9 1-5 0.046; * 14 * 4-9 1-5 0.044; * 15 * 4-9 1-5 0.053"
		"* 16 * 4-9 1-5 0.094; * 17 * 4-9 1-5 0.168; * 18 * 4-9 1-5 0.148; * 19 * 4-9 1-5 0.086"
		"* 20 * 4-9 1-5 0.053; * 21 * 4-9 1-5 0.038; * 22 * 4-9 1-5 0.023; * 23 * 4-9 1-5 0.013"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.009; *  1 * 4-9 6-0 0.007; *  2 * 4-9 6-0 0.007; *  3 * 4-9 6-0 0.007"
		"*  4 * 4-9 6-0 0.007; *  5 * 4-9 6-0 0.009; *  6 * 4-9 6-0 0.017; *  7 * 4-9 6-0 0.038"
		"*  8 * 4-9 6-0 0.060; *  9 * 4-9 6-0 0.068; * 10 * 4-9 6-0 0.065; * 11 * 4-9 6-0 0.067"
		"* 12 * 4-9 6-0 0.076; * 13 * 4-9 6-0 0.066; * 14 * 4-9 6-0 0.061; * 15 * 4-9 6-0 0.067"
		"* 16 * 4-9 6-0 0.091; * 17 * 4-9 6-0 0.134; * 18 * 4-9 6-0 0.121; * 19 * 4-9 6-0 0.080"
		"* 20 * 4-9 6-0 0.052; * 21 * 4-9 6-0 0.035; * 22 * 4-9 6-0 0.022; * 23 * 4-9 6-0 0.011"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.010; *  1 * 10-3 1-5 0.009; *  2 * 10-3 1-5 0.009; *  3 * 10-3 1-5 0.009" 
		"*  4 * 10-3 1-5 0.009; *  5 * 10-3 1-5 0.016; *  6 * 10-3 1-5 0.032; *  7 * 10-3 1-5 0.050"
		"*  8 * 10-3 1-5 0.045; *  9 * 10-3 1-5 0.043; * 10 * 10-3 1-5 0.045; * 11 * 10-3 1-5 0.059"
		"* 12 * 10-3 1-5 0.063; * 13 * 10-3 1-5 0.053; * 14 * 10-3 1-5 0.052; * 15 * 10-3 1-5 0.072"
		"* 16 * 10-3 1-5 0.138; * 17 * 10-3 1-5 0.242; * 18 * 10-3 1-5 0.182; * 19 * 10-3 1-5 0.088"
		"* 20 * 10-3 1-5 0.051; * 21 * 10-3 1-5 0.034; * 22 * 10-3 1-5 0.022; * 23 * 10-3 1-5 0.014"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.013; *  1 * 10-3 6-0 0.010; *  2 * 10-3 6-0 0.010; *  3 * 10-3 6-0 0.010"
		"*  4 * 10-3 6-0 0.010; *  5 * 10-3 6-0 0.012; *  6 * 10-3 6-0 0.018; *  7 * 10-3 6-0 0.040"
		"*  8 * 10-3 6-0 0.073; *  9 * 10-3 6-0 0.094; * 10 * 10-3 6-0 0.091; * 11 * 10-3 6-0 0.100"
		"* 12 * 10-3 6-0 0.117; * 13 * 10-3 6-0 0.109; * 14 * 10-3 6-0 0.100; * 15 * 10-3 6-0 0.108"
		"* 16 * 10-3 6-0 0.153;	* 17 * 10-3 6-0 0.215; * 18 * 10-3 6-0 0.161; * 19 * 10-3 6-0 0.085"
		"* 20 * 10-3 6-0 0.050;	* 21 * 10-3 6-0 0.033; * 22 * 10-3 6-0 0.022; * 23 * 10-3 6-0 0.014"
		"}"		
	},
	{   "MICROWAVE", 
		{40, false, {0,0,1}, 0.7, 0.8},
		"type:analog; schedule: residential-microwave-default; energy: 1.0 kWh; power: 0.2 kW",
		"residential-microwave-default", 
		"positive; nonzero; weekday-summer {"
		"*  0 * 4-9 1-5 0.009; *  1 * 4-9 1-5 0.008; *  2 * 4-9 1-5 0.007; *  3 * 4-9 1-5 0.007"
		"*  4 * 4-9 1-5 0.008; *  5 * 4-9 1-5 0.012; *  6 * 4-9 1-5 0.025; *  7 * 4-9 1-5 0.040"
		"*  8 * 4-9 1-5 0.044; *  9 * 4-9 1-5 0.042; * 10 * 4-9 1-5 0.042; * 11 * 4-9 1-5 0.053"
		"* 12 * 4-9 1-5 0.057; * 13 * 4-9 1-5 0.046; * 14 * 4-9 1-5 0.044; * 15 * 4-9 1-5 0.053"
		"* 16 * 4-9 1-5 0.094; * 17 * 4-9 1-5 0.168; * 18 * 4-9 1-5 0.148; * 19 * 4-9 1-5 0.086"
		"* 20 * 4-9 1-5 0.053; * 21 * 4-9 1-5 0.038; * 22 * 4-9 1-5 0.023; * 23 * 4-9 1-5 0.013"
		"}"
		"weekend-summer {"
		"*  0 * 4-9 6-0 0.009; *  1 * 4-9 6-0 0.007; *  2 * 4-9 6-0 0.007; *  3 * 4-9 6-0 0.007"
		"*  4 * 4-9 6-0 0.007; *  5 * 4-9 6-0 0.009; *  6 * 4-9 6-0 0.017; *  7 * 4-9 6-0 0.038"
		"*  8 * 4-9 6-0 0.060; *  9 * 4-9 6-0 0.068; * 10 * 4-9 6-0 0.065; * 11 * 4-9 6-0 0.067"
		"* 12 * 4-9 6-0 0.076; * 13 * 4-9 6-0 0.066; * 14 * 4-9 6-0 0.061; * 15 * 4-9 6-0 0.067"
		"* 16 * 4-9 6-0 0.091; * 17 * 4-9 6-0 0.134; * 18 * 4-9 6-0 0.121; * 19 * 4-9 6-0 0.080"
		"* 20 * 4-9 6-0 0.052; * 21 * 4-9 6-0 0.035; * 22 * 4-9 6-0 0.022; * 23 * 4-9 6-0 0.011"
		"}"
		"weekday-winter {"
		"*  0 * 10-3 1-5 0.010; *  1 * 10-3 1-5 0.009; *  2 * 10-3 1-5 0.009; *  3 * 10-3 1-5 0.009" 
		"*  4 * 10-3 1-5 0.009; *  5 * 10-3 1-5 0.016; *  6 * 10-3 1-5 0.032; *  7 * 10-3 1-5 0.050"
		"*  8 * 10-3 1-5 0.045; *  9 * 10-3 1-5 0.043; * 10 * 10-3 1-5 0.045; * 11 * 10-3 1-5 0.059"
		"* 12 * 10-3 1-5 0.063; * 13 * 10-3 1-5 0.053; * 14 * 10-3 1-5 0.052; * 15 * 10-3 1-5 0.072"
		"* 16 * 10-3 1-5 0.138; * 17 * 10-3 1-5 0.242; * 18 * 10-3 1-5 0.182; * 19 * 10-3 1-5 0.088"
		"* 20 * 10-3 1-5 0.051; * 21 * 10-3 1-5 0.034; * 22 * 10-3 1-5 0.022; * 23 * 10-3 1-5 0.014"
		"}"
		"weekend-winter {"
		"*  0 * 10-3 6-0 0.013; *  1 * 10-3 6-0 0.010; *  2 * 10-3 6-0 0.010; *  3 * 10-3 6-0 0.010"
		"*  4 * 10-3 6-0 0.010; *  5 * 10-3 6-0 0.012; *  6 * 10-3 6-0 0.018; *  7 * 10-3 6-0 0.040"
		"*  8 * 10-3 6-0 0.073; *  9 * 10-3 6-0 0.094; * 10 * 10-3 6-0 0.091; * 11 * 10-3 6-0 0.100"
		"* 12 * 10-3 6-0 0.117; * 13 * 10-3 6-0 0.109; * 14 * 10-3 6-0 0.100; * 15 * 10-3 6-0 0.108"
		"* 16 * 10-3 6-0 0.153;	* 17 * 10-3 6-0 0.215; * 18 * 10-3 6-0 0.161; * 19 * 10-3 6-0 0.085"
		"* 20 * 10-3 6-0 0.050;	* 21 * 10-3 6-0 0.033; * 22 * 10-3 6-0 0.022; * 23 * 10-3 6-0 0.014"
		"}"		
	},

	/// @todo add other implicit enduse schedules and shapes as they are defined
};

EXPORT CIRCUIT *attach_enduse_house_e(OBJECT *obj, enduse *target, double breaker_amps, int is220)
{
	house_e *pHouse = 0;
	CIRCUIT *c = 0;

	if(obj == NULL){
		GL_THROW("attach_house_a: null object reference");
	}
	if(target == NULL){
		GL_THROW("attach_house_a: null enduse target data");
	}
	if(breaker_amps < 0 || breaker_amps > 1000){ /* at 3kA, we're looking into substation power levels, not enduses */
		GL_THROW("attach_house_a: breaker amps of %i unrealistic", breaker_amps);
	}

	pHouse = OBJECTDATA(obj,house_e);
	return pHouse->attach(obj,breaker_amps,is220,target);
}

//////////////////////////////////////////////////////////////////////////
// house_e CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* house_e::oclass = NULL;
CLASS* house_e::pclass = NULL;

double house_e::warn_low_temp = 55; // degF
double house_e::warn_high_temp = 95; // degF
bool house_e::warn_control = true;
double house_e::system_dwell_time = 300; // seconds

/** House object constructor:  Registers the class and publishes the variables that can be set by the user. 
Sets default randomized values for published variables.
**/
house_e::house_e(MODULE *mod) : residential_enduse(mod)
{
	// first time init
	if (oclass==NULL)  
	{
		// register the class definition
		oclass = gl_register_class(mod,"house",sizeof(house_e),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);

		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_object,"weather",PADDR(weather),PT_DESCRIPTION,"reference to the climate object",
			PT_double,"floor_area[sf]",PADDR(floor_area),PT_DESCRIPTION,"home conditioned floor area",
			PT_double,"gross_wall_area[sf]",PADDR(gross_wall_area),PT_DESCRIPTION,"gross outdoor wall area",
			PT_double,"ceiling_height[ft]",PADDR(ceiling_height),PT_DESCRIPTION,"average ceiling height",
			PT_double,"aspect_ratio",PADDR(aspect_ratio), PT_DESCRIPTION,"aspect ratio of the home's footprint",
			PT_double,"envelope_UA[Btu/degF.h]",PADDR(envelope_UA),PT_DESCRIPTION,"overall UA of the home's envelope",
			PT_double,"window_wall_ratio",PADDR(window_wall_ratio),PT_DESCRIPTION,"ratio of window area to wall area",
			PT_double,"number_of_doors",PADDR(number_of_doors),PT_DESCRIPTION,"ratio of door area to wall area",
			PT_double,"exterior_wall_fraction",PADDR(exterior_wall_fraction),PT_DESCRIPTION,"ratio of exterior wall ratio to wall area",
			PT_double,"interior_exterior_wall_ratio",PADDR(interior_exterior_wall_ratio),PT_DESCRIPTION,"ratio of interior to exterior walls",
			PT_double,"exterior_ceiling_fraction",PADDR(exterior_ceiling_fraction),PT_DESCRIPTION,"ratio of external ceiling sf to floor area",
			PT_double,"exterior_floor_fraction",PADDR(exterior_floor_fraction),PT_DESCRIPTION,"ratio of floor area used in UA calculation",
			PT_double,"window_shading",PADDR(glazing_shgc),PT_DESCRIPTION,"transmission coefficient through window due to glazing",
			PT_double,"airchange_per_hour",PADDR(airchange_per_hour),PT_DESCRIPTION,"number of air-changes per hour",
			PT_double,"airchange_UA[Btu/degF.h]",PADDR(airchange_UA),PT_DESCRIPTION,"additional UA due to air infiltration",
			PT_double,"internal_gain[Btu/h]",PADDR(total.heatgain),PT_DESCRIPTION,"internal heat gains",
			PT_double,"solar_gain[Btu/h]",PADDR(solar_load),PT_DESCRIPTION,"solar heat gains",
			PT_double,"heat_cool_gain[Btu/h]",PADDR(load.heatgain),PT_DESCRIPTION,"system heat gains(losses)",

			PT_double,"thermostat_deadband[degF]",PADDR(thermostat_deadband),PT_DESCRIPTION,"deadband of thermostat control",
			PT_double,"heating_setpoint[degF]",PADDR(heating_setpoint),PT_DESCRIPTION,"thermostat heating setpoint",
			PT_double,"cooling_setpoint[degF]",PADDR(cooling_setpoint),PT_DESCRIPTION,"thermostat cooling setpoint",
			PT_double,"design_heating_setpoint[degF]",PADDR(design_heating_setpoint),PT_DESCRIPTION,"system design heating setpoint",
			PT_double,"design_cooling_setpoint[degF]",PADDR(design_cooling_setpoint),PT_DESCRIPTION,"system design cooling setpoint",
			
			PT_double,"design_heating_capacity[Btu/h]",PADDR(design_heating_capacity),PT_DESCRIPTION,"system heating capacity",
			PT_double,"design_cooling_capacity[Btu/h]",PADDR(design_cooling_capacity),PT_DESCRIPTION,"system cooling capacity",
			PT_double,"cooling_design_temperature[degF]", PADDR(cooling_design_temperature),PT_DESCRIPTION,"system cooling design temperature",
			PT_double,"heating_design_temperature[degF]", PADDR(heating_design_temperature),PT_DESCRIPTION,"system heating design temperature",
			PT_double,"design_peak_solar[Btu/h.sf]", PADDR(design_peak_solar),PT_DESCRIPTION,"system design solar load",
			PT_double,"design_internal_gains[W/sf]", PADDR(design_internal_gains),PT_DESCRIPTION,"system design internal gains",
			PT_double,"air_heat_fraction[pu]", PADDR(air_heat_fraction), PT_DESCRIPTION, "fraction of heat gain/loss that goes to air (as opposed to mass)",

			PT_double,"heating_demand",PADDR(heating_demand),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the current power draw to run the heating system",
			PT_double,"cooling_demand",PADDR(cooling_demand),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the current power draw to run the cooling system",
			PT_double,"heating_COP[pu]",PADDR(heating_COP),PT_DESCRIPTION,"system heating performance coefficient",
			PT_double,"cooling_COP[Btu/kWh]",PADDR(cooling_COP),PT_DESCRIPTION,"system cooling performance coefficient",
			//PT_double,"COP_coeff",PADDR(COP_coeff),PT_DESCRIPTION,"effective system performance coefficient",
			PT_double,"air_temperature[degF]",PADDR(Tair),PT_DESCRIPTION,"indoor air temperature",
			PT_double,"outdoor_temperature[degF]",PADDR(outside_temperature),PT_DESCRIPTION,"outdoor air temperature",
			PT_double,"mass_heat_capacity[Btu/degF]",PADDR(house_content_thermal_mass),PT_DESCRIPTION,"interior mass heat capacity",
			PT_double,"mass_heat_coeff[Btu/degF.h]",PADDR(house_content_heat_transfer_coeff),PT_DESCRIPTION,"interior mass heat exchange coefficient",
			PT_double,"mass_temperature[degF]",PADDR(Tmaterials),PT_DESCRIPTION,"interior mass temperature",
			PT_double,"air_volume[cf]", PADDR(volume), PT_DESCRIPTION,"air volume",
			PT_double,"air_mass[lb]",PADDR(air_mass), PT_DESCRIPTION,"air mass",
			PT_double,"air_heat_capacity[Btu/degF]", PADDR(air_thermal_mass), PT_DESCRIPTION,"air thermal mass",
			PT_double,"latent_load_fraction[pu]", PADDR(latent_load_fraction), PT_DESCRIPTION,"fractional increase in cooling load due to latent heat",
			PT_double,"total_thermal_mass_per_floor_area[Btu/degF.sf]",PADDR(total_thermal_mass_per_floor_area),
			PT_double,"interior_surface_heat_transfer_coeff[Btu/h.degF.sf]",PADDR(interior_surface_heat_transfer_coeff),
			PT_double,"number_of_stories",PADDR(number_of_stories),PT_DESCRIPTION,"number of stories within the structure",
			PT_double,"window_exterior_transmission_coefficient",PADDR(window_exterior_transmission_coefficient),PT_DESCRIPTION,"coefficient for the amount of energy that passes through window",

			PT_set,"system_type",PADDR(system_type),PT_DESCRIPTION,"heating/cooling system type/options",
				PT_KEYWORD, "GAS",	(set)ST_GAS,
				PT_KEYWORD, "AIRCONDITIONING", (set)ST_AC,
				PT_KEYWORD, "FORCEDAIR", (set)ST_AIR,
				PT_KEYWORD, "TWOSTAGE", (set)ST_VAR,
			PT_enumeration,"system_mode",PADDR(system_mode),PT_DESCRIPTION,"heating/cooling system operation state",
				PT_KEYWORD,"UNKNOWN",SM_UNKNOWN,
				PT_KEYWORD,"HEAT",SM_HEAT,
				PT_KEYWORD,"OFF",SM_OFF,
				PT_KEYWORD,"COOL",SM_COOL,
				PT_KEYWORD,"AUX",SM_AUX,
			PT_enumeration,"thermal_integrity_level",PADDR(thermal_integrity_level),PT_DESCRIPTION,"default envelope UA settings",
				PT_KEYWORD,"VERY_LITTLE",TI_VERY_LITTLE,
				PT_KEYWORD,"LITTLE",TI_LITTLE,
				PT_KEYWORD,"BELOW_NORMAL",TI_BELOW_NORMAL,
				PT_KEYWORD,"NORMAL",TI_NORMAL,
				PT_KEYWORD,"ABOVE_NORMAL",TI_ABOVE_NORMAL,
				PT_KEYWORD,"GOOD",TI_GOOD,
				PT_KEYWORD,"VERY_GOOD",TI_VERY_GOOD,
				PT_KEYWORD,"UNKNOWN",TI_UNKNOWN,
			PT_double, "Rroof[degF.h/Btu]", PADDR(Rroof),PT_DESCRIPTION,"roof R-value",
			PT_double, "Rwall[degF.h/Btu]", PADDR(Rwall),PT_DESCRIPTION,"wall R-value",
			PT_double, "Rfloor[degF.h/Btu]", PADDR(Rfloor),PT_DESCRIPTION,"floor R-value",
			PT_double, "Rwindows[degF.h/Btu]", PADDR(Rwindows),PT_DESCRIPTION,"window R-value",
			PT_double, "Rdoors[degF.h/Btu]", PADDR(Rdoors),PT_DESCRIPTION,"door R-value",
			
			PT_double,"hvac_load",PADDR(hvac_load),PT_DESCRIPTION,"heating/cooling system load",
			PT_enduse,"panel",PADDR(total),PT_DESCRIPTION,"total panel enduse load",
			PT_double,"design_internal_gain_density[W/sf]",PADDR(design_internal_gain_density),PT_DESCRIPTION,"average density of heat generating devices in the house",
#ifdef _DEBUG
			// these are added in the debugging version so we can spy on ETP
			PT_double,"a",PADDR(a),
			PT_double,"b",PADDR(b),
			PT_double,"c",PADDR(c),
			PT_double,"d",PADDR(d),
			PT_double,"c1",PADDR(c1),
			PT_double,"c2",PADDR(c2),
			PT_double,"A3",PADDR(A3),
			PT_double,"A4",PADDR(A4),
			PT_double,"k1",PADDR(k1),
			PT_double,"k2",PADDR(k2),
			PT_double,"r1",PADDR(r1),
			PT_double,"r2",PADDR(r2),
			PT_double,"Teq",PADDR(Teq),
			PT_double,"Tevent",PADDR(Tevent),
			PT_double,"Qi",PADDR(Qi),
			PT_double,"Qa",PADDR(Qa),
			PT_double,"Qm",PADDR(Qm),
			PT_double,"Qh",PADDR(load.heatgain),
			PT_double,"dTair",PADDR(dTair),
#endif
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);			

		gl_publish_function(oclass,	"attach_enduse", (FUNCTIONADDR)attach_enduse_house_e);
		gl_global_create("residential::implicit_enduses",PT_set,&implicit_enduses_active,
			PT_KEYWORD, "LIGHTS", (set)IEU_LIGHTS,
			PT_KEYWORD, "PLUGS", (set)IEU_PLUGS,
			PT_KEYWORD, "OCCUPANCY", (set)IEU_OCCUPANCY,
			PT_KEYWORD, "DISHWASHER", (set)IEU_DISHWASHER,
			PT_KEYWORD, "MICROWAVE", (set)IEU_MICROWAVE,
			PT_KEYWORD, "FREEZER", (set)IEU_FREEZER,
			PT_KEYWORD, "REFRIGERATOR", (set)IEU_REFRIGERATOR,
			PT_KEYWORD, "RANGE", (set)IEU_RANGE,
			PT_KEYWORD, "EVCHARGER", (set)IEU_EVCHARGER,
			PT_KEYWORD, "WATERHEATER", (set)IEU_WATERHEATER,
			PT_KEYWORD, "CLOTHESWASHER", (set)IEU_CLOTHESWASHER,
			PT_KEYWORD, "DRYER", (set)IEU_DRYER,
			PT_KEYWORD, "NONE", (set)0,
			PT_DESCRIPTION, "list of implicit enduses that are active in houses",
			NULL);
		gl_global_create("residential::house_low_temperature_warning[degF]",PT_double,&warn_low_temp,
			PT_DESCRIPTION, "the low house indoor temperature at which a warning will be generated",
			NULL);
		gl_global_create("residential::house_high_temperature_warning[degF]",PT_double,&warn_high_temp,
			PT_DESCRIPTION, "the high house indoor temperature at which a warning will be generated",
			NULL);
		gl_global_create("residential::thermostat_control_warning",PT_double,&warn_control,
			PT_DESCRIPTION, "boolean to indicate whether a warning is generated when indoor temperature is out of control limits",
			NULL);
		gl_global_create("residential::system_dwell_time[s]",PT_double,&system_dwell_time,
			PT_DESCRIPTION, "the heating/cooling system dwell time interval for changing system state",
			NULL);
	}	
		gl_global_create("residential::aux_cutin_temperature[degF]",PT_double,&aux_cutin_temperature,
			PT_DESCRIPTION, "the outdoor air temperature below which AUX heating is used",
			NULL);
}

int house_e::create() 
{
	int result=SUCCESS;
	char1024 active_enduses;
	gl_global_getvar("residential::implicit_enduses",active_enduses,sizeof(active_enduses));
	char *token = NULL;

	glazing_shgc = 0.65; // assuming generic double glazing
	load.power_fraction = 0.8;
	load.impedance_fraction = 0.2;
	load.current_fraction = 0.0;
	load.power_factor = 1.0;
	design_internal_gain_density = 0.6;//gl_random_triangle(4,6);
	thermal_integrity_level = TI_UNKNOWN;

	// set up implicit enduse list
	implicit_enduse_list = NULL;

	if (strcmp(active_enduses,"NONE")!=0)
	{
		char *eulist[64];
		char n_eu=0;

		// extract the implicit_enduse list
		while ((token=strtok(token?NULL:active_enduses,"|"))!=NULL)
			eulist[n_eu++] = token;

		while (n_eu-->0)
		{
			char *euname = eulist[n_eu];
			strlwr(euname);
			
			// find the implicit enduse description
			struct s_implicit_enduse_list *eu = NULL;
			int found=0;
			for (eu=implicit_enduse_data; eu<implicit_enduse_data+sizeof(implicit_enduse_data)/sizeof(implicit_enduse_data[0]); eu++)
			{
				char name[64];
				sprintf(name,"residential-%s-default",euname);
				// matched enduse
				if (strcmp(eu->schedule_name,name)==0)
				{
					SCHEDULE *sched = gl_schedule_create(eu->schedule_name,eu->schedule_definition);
					IMPLICITENDUSE *item = (IMPLICITENDUSE*)gl_malloc(sizeof(IMPLICITENDUSE));
					memset(item,0,sizeof(IMPLICITENDUSE));
					gl_enduse_create(&(item->load));
					item->load.shape = gl_loadshape_create(sched);
					if (gl_set_value_by_type(PT_loadshape,item->load.shape,eu->shape)==0)
					{
						gl_error("loadshape '%s' could not be created", name);
						result = FAILED;
					}
					item->load.name = eu->implicit_name;
					item->next = implicit_enduse_list;
					item->amps = eu->load.breaker_amps;
					item->is220 = eu->load.circuit_is220;
					item->load.impedance_fraction = eu->load.fractions.z;
					item->load.current_fraction = eu->load.fractions.i;
					item->load.power_fraction = eu->load.fractions.p;
					item->load.power_factor = eu->load.power_factor;
					item->load.heatgain_fraction = eu->load.heat_fraction;
					implicit_enduse_list = item;
					found=1;
					break;
				}
			}
			if (found==0)
			{
				gl_error("house_e data for '%s' implicit enduse not found", euname);
				result = FAILED;
			}
		}
	}
	total.name = "panel";
	load.name = "system";

	return result;
}

/** Checks for climate object and maps the climate variables to the house_e object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 74 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
**/
int house_e::init_climate()
{
	OBJECT *hdr = OBJECTHDR(this);

	// link to climate data
	static FINDLIST *climates = NULL;
	int not_found = 0;
	if (climates==NULL && not_found==0) 
	{
		climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		if (climates==NULL)
		{
			not_found = 1;
			gl_warning("house_e: no climate data found, using static data");

			//default to mock data
			extern double default_outdoor_temperature, default_humidity, default_solar[9];
			pTout = &default_outdoor_temperature;
			pRhout = &default_humidity;
			pSolar = &default_solar[0];
		}
		else if (climates->hit_count>1)
		{
			gl_warning("house_e: %d climates found, using first one defined", climates->hit_count);
		}
	}
	if (climates!=NULL)
	{
		if (climates->hit_count==0)
		{
			//default to mock data
			extern double default_outdoor_temperature, default_humidity, default_solar[9];
			pTout = &default_outdoor_temperature;
			pRhout = &default_humidity;
			pSolar = &default_solar[0];
		}
		else //climate data was found
		{
			// force rank of object w.r.t climate
			OBJECT *obj = gl_find_next(climates,NULL);
			if(weather == NULL){
				weather = obj;
			}
			if (obj->rank<=hdr->rank)
				gl_set_dependent(obj,hdr);
			pTout = (double*)GETADDR(obj,gl_get_property(obj,"temperature"));
			pRhout = (double*)GETADDR(obj,gl_get_property(obj,"humidity"));
			pSolar = (double*)GETADDR(obj,gl_get_property(obj,"solar_flux"));
			struct {
				char *name;
				double *dst;
			} map[] = {
				{"record.high",&cooling_design_temperature},
				{"record.low",&heating_design_temperature},
			};
			int i;
			for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			{
				double *src = (double*)GETADDR(obj,gl_get_property(obj,map[i].name));
				if (src) *map[i].dst = *src;
			}
		}
	}
	return 1;
}

/** Map circuit variables to meter.  Initalize house_e and HVAC model properties,
and internal gain variables.
**/

int house_e::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	Off_Return = TS_NEVER;

	// local object name,	meter object name
	struct {
			complex **var;
			char *varname;
			} map[] = { {&pCircuit_V,			"voltage_12"}, // assumes 1N and 2N follow immediately in memory
						{&pLine_I,				"residential_nominal_current_1"}, // assumes 2 and 3(12) follow immediately in memory - off-nominal angles are handled externally
						{&pShunt,				"shunt_1"},		// assumes 2 and 3 (12) follow immediately in memory
						{&pPower,				"power_1"},		// assumes 2 and 3 (12) follow immediately in memory
					/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
					};

	extern complex default_line_voltage[3], default_line_current[3], default_line_power[3], default_line_shunt[3];
	extern bool default_NR_mode;
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	OBJECT *obj = OBJECTHDR(this);
	if (parent!=NULL && (gl_object_isa(parent,"triplex_meter","powerflow") || gl_object_isa(obj->parent,"triplex_node","powerflow")))
	{
		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			if ((*(map[i].var) = get_complex(parent,map[i].varname))==NULL)
				GL_THROW("%s (%s:%d) does not implement triplex_meter variable %s for %s (house_e:%d)", 
					parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, map[i].varname, obj->name?obj->name:"unnamed", obj->id);
		}

		//Map to the triplex variable
		pHouseConn = get_bool(parent,"house_present");

		//Connect the NR mode indicator
		NR_mode = get_bool(parent,"NR_mode");
		
		//Flag that we're attached to a node
		*pHouseConn = true;
	}
	else
	{
		gl_warning("house_e:%d %s; using static voltages", obj->id, parent==NULL?"has no parent triplex_meter defined":"parent is not a triplex_meter");

		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line_voltage[0];
		*(map[1].var) = &default_line_current[0];
		*(map[2].var) = &default_line_shunt[0];
		*(map[3].var) = &default_line_power[0];

		//Attach the NR solver method flag to default
		NR_mode = &default_NR_mode;
	}

	// set defaults for panel/meter variables
	if (panel.max_amps==0) panel.max_amps = 200; 
	load.power = complex(0,0,J);

	// Set defaults for published variables nor provided by model definition
	switch (thermal_integrity_level) {
		case TI_VERY_LITTLE:
			Rroof = 11;
			Rwall = 4;
			Rfloor = 4;
			Rdoors = 3;
			Rwindows = 1/1.27;
			airchange_per_hour = 1.5;
			break;
		case TI_LITTLE:
			Rroof = 19;
			Rwall = 11;
			Rfloor = 4;
			Rdoors = 3;
			Rwindows = 1/0.81;
			airchange_per_hour = 1.5;
			break;
		case TI_BELOW_NORMAL:
			Rroof = 19;
			Rwall = 11;
			Rfloor = 11;
			Rdoors = 3;
			Rwindows = 1/0.81;
			airchange_per_hour = 1.0;
			break;
		case TI_NORMAL:
			Rroof = 30;
			Rwall = 11;
			Rfloor = 19;
			Rdoors = 3;
			Rwindows = 1/0.6;
			airchange_per_hour = 1.0;
			break;
		case TI_ABOVE_NORMAL:
			Rroof = 30;
			Rwall = 19;
			Rfloor = 11;
			Rdoors = 3;
			Rwindows = 1/0.6;
			airchange_per_hour = 1.0;
			break;
		case TI_GOOD:
			Rroof = 30;
			Rwall = 19;
			Rfloor = 22;
			Rdoors = 5;
			Rwindows = 1/0.47;
			airchange_per_hour = 0.5;
			break;
		case TI_VERY_GOOD:
			Rroof = 48;
			Rwall = 22;
			Rfloor = 30;
			Rdoors = 11;
			Rwindows = 1/0.31;
			airchange_per_hour = 0.5;
			break;
		case TI_UNKNOWN:
			// do nothing - use all of the built-in defaults or user-specified values as thermal integrity wasn't used
			break;
		default:
			// do nothing - use all of the built-in defaults or user-specified values as thermal integrity wasn't used
			break;
	}


	if (heating_COP==0.0)		heating_COP = 2;//gl_random_triangle(1,2);
	if (cooling_COP==0.0)		cooling_COP = 2;//gl_random_triangle(2,4);
	if (number_of_stories==0)	number_of_stories=1;

	if (aspect_ratio==0.0)		aspect_ratio = 1.5;//gl_random_triangle(1,2);
	if (floor_area==0)			floor_area = 2200;
	if (ceiling_height==0)		ceiling_height = 8;//gl_random_triangle(7,9);
	if (gross_wall_area==0)		gross_wall_area = 2.0 * number_of_stories * (aspect_ratio + 1.0) * ceiling_height * sqrt(floor_area/aspect_ratio/number_of_stories);
	if (window_wall_ratio==0)	window_wall_ratio = 0.07;
	if (number_of_doors==0)		number_of_doors = 4;
	if (interior_exterior_wall_ratio == 0) interior_exterior_wall_ratio = 1.5; //Based partions for six rooms per floor
	if (exterior_wall_fraction==0) exterior_wall_fraction = 1;
	if (exterior_ceiling_fraction==0) exterior_ceiling_fraction = 1;
	if (exterior_floor_fraction==0) exterior_floor_fraction = 1;
	if (window_exterior_transmission_coefficient==0) window_exterior_transmission_coefficient = 0.6; //0.6 represents a window with a screen

	if (Rroof==0)				Rroof = 30;//gl_random_triangle(50,70);
	if (Rwall==0)				Rwall = 19;//gl_random_triangle(15,25);
	if (Rfloor==0)				Rfloor = 22;//gl_random_triangle(100,150);
	if (Rwindows==0)			Rwindows = 2.1;//gl_random_triangle(2,4);
	if (Rdoors==0)				Rdoors = 5;//gl_random_triangle(1,3);
	
	air_density = 0.0735;			// density of air [lb/cf]
	air_heat_capacity = 0.2402;	// heat capacity of air @ 80F [BTU/lb/F]

	//house_e properties for HVAC
	if (volume==0) volume = ceiling_height*floor_area;					// volume of air [cf]
	if (air_mass==0) air_mass = air_density*volume;						// mass of air [lb]
	if (air_thermal_mass==0) air_thermal_mass = 3*air_heat_capacity*air_mass;	// thermal mass of air [BTU/F]  //*3 multiplier is to reflect that the air mass includes surface effects from the mass as well.  
	if (air_heat_fraction==0) air_heat_fraction=0.5;
	if (air_heat_fraction<0.0 || air_heat_fraction>1.0) throw "air heat fraction is not between 0 and 1";
	if (total_thermal_mass_per_floor_area == 0) total_thermal_mass_per_floor_area = 2;
	if (interior_surface_heat_transfer_coeff == 0) interior_surface_heat_transfer_coeff = 1;

	if (airchange_per_hour==0)	airchange_per_hour = 0.5;//gl_random_triangle(0.5,1);
	if (airchange_UA == 0) airchange_UA = airchange_per_hour * volume * air_density * air_heat_capacity;

	double door_area = number_of_doors * 3 * 78 / 12;
	double window_area = gross_wall_area * window_wall_ratio * exterior_wall_fraction;
	double net_exterior_wall_area = exterior_wall_fraction * (gross_wall_area - window_area - door_area);
	double exterior_ceiling_area = floor_area * exterior_ceiling_fraction / number_of_stories;
	double exterior_floor_area = floor_area * exterior_floor_fraction / number_of_stories;

	if (envelope_UA==0)	envelope_UA = exterior_ceiling_area/Rroof + exterior_floor_area/Rfloor + net_exterior_wall_area/Rwall + window_area/Rwindows + door_area/Rdoors;

	// initalize/set system model parameters
    //if (COP_coeff==0)			COP_coeff = gl_random_uniform(0.9,1.1);	// coefficient of cops [scalar]
	if (heating_setpoint==0)	heating_setpoint = 70;//gl_random_triangle(68,72);
	if (cooling_setpoint==0)	cooling_setpoint = 75;//gl_random_triangle(75,79);
	if (design_cooling_setpoint==0) design_cooling_setpoint = 75;
	if (design_heating_setpoint==0) design_heating_setpoint = 70;
	if (design_peak_solar==0)	design_peak_solar = 195; //From Rob's defaults

	if (thermostat_deadband==0)	thermostat_deadband = 2;//gl_random_triangle(2,3);
	if (Tair==0){
		/* bind limits between 60 and 140 degF */
		double Thigh = cooling_setpoint+thermostat_deadband/2;
		double Tlow  = heating_setpoint-thermostat_deadband/2;
		Thigh = clip(Thigh, 60, 140);
		Tlow = clip(Tlow, 60, 140);
		Tair = gl_random_uniform(Tlow, Thigh);	// air temperature [F]
	}
	if (over_sizing_factor==0)  over_sizing_factor = 0;
	if (cooling_design_temperature == 0)	cooling_design_temperature = 95.0;
	if (design_internal_gains==0) design_internal_gains =  167.09 * pow(floor_area,0.442); // Numerical estimate of internal gains
	if (latent_load_fraction==0) latent_load_fraction = 0.3;
	if (design_cooling_capacity==0)	// calculate basic load then round to nearest standard HVAC sizing
	{	
		design_cooling_capacity = (1 + over_sizing_factor) * (1+latent_load_fraction) * ((envelope_UA + airchange_UA) * (cooling_design_temperature - design_cooling_setpoint) + design_internal_gains + (design_peak_solar * window_area * glazing_shgc * window_exterior_transmission_coefficient));
		double round_value = design_cooling_capacity / 12000;
		design_cooling_capacity = ceil(round_value) * 12000;
	}

	if (design_heating_capacity==0)	// calculate basic load then round to nearest standard HVAC sizing
	{
		design_heating_capacity = (1 + over_sizing_factor) * (envelope_UA + airchange_UA) * (design_heating_setpoint - heating_design_temperature);
		double round_value = design_heating_capacity / 10000;
		design_heating_capacity = ceil(round_value) * 10000;
	}
    if (system_mode==SM_UNKNOWN) system_mode = SM_OFF;	// heating/cooling mode {HEAT, COOL, OFF}

	if (house_content_thermal_mass==0) house_content_thermal_mass = total_thermal_mass_per_floor_area*floor_area;		// thermal mass of house_e [BTU/F]
    if (house_content_heat_transfer_coeff==0) house_content_heat_transfer_coeff = interior_surface_heat_transfer_coeff*( net_exterior_wall_area / exterior_wall_fraction + gross_wall_area * interior_exterior_wall_ratio + number_of_stories * exterior_ceiling_area / exterior_ceiling_fraction);	// heat transfer coefficient of house_e contents [BTU/hr.F]

	if(Tair == 0){
		if (system_mode==SM_OFF)
			Tair = gl_random_uniform(heating_setpoint,cooling_setpoint);
		else if (system_mode==SM_HEAT || system_mode==SM_AUX)
			Tair = gl_random_uniform(heating_setpoint-thermostat_deadband/2,heating_setpoint+thermostat_deadband/2);
		else if (system_mode==SM_COOL)
			Tair = gl_random_uniform(cooling_setpoint-thermostat_deadband/2,cooling_setpoint+thermostat_deadband/2);
	}


	Tmaterials = Tair;	
	


	// calculate thermal constants
#define Ca (air_thermal_mass)
#define Tout (outside_temperature)
#define Ua (envelope_UA)
#define Cm (house_content_thermal_mass)
#define Hm (house_content_heat_transfer_coeff)
#define Qs (solar_load)
#define Qh (load.heatgain)
#define Ta (Tair)
#define dTa (dTair)
#define Tm (Tmaterials)

	if (Ca<=0)
		throw "Ca must be positive";
	if (Cm<=0)
		throw "Cm must be positive";

	a = Cm*Ca/Hm;
	b = Cm*(Ua+airchange_UA+Hm)/Hm+Ca;
	c = Ua + airchange_UA;
	c1 = -(Ua+airchange_UA + Hm)/Ca;
	c2 = Hm/Ca;
	double rr = sqrt(b*b-4*a*c)/(2*a);
	double r = -b/(2*a);
	r1 = r+rr;
	r2 = r-rr;
	A3 = Ca/Hm * r1 + (Ua+airchange_UA+Hm)/Hm;
	A4 = Ca/Hm * r2 + (Ua+airchange_UA+Hm)/Hm;

	// outside temperature init
	extern double default_outdoor_temperature;
	outside_temperature = default_outdoor_temperature;

	// connect any implicit loads
	attach_implicit_enduses();
	update_system();
	update_model();

	// attach the house_e HVAC to the panel
	load.breaker_amps = 200;
	load.config = EUC_IS220;
	attach(OBJECTHDR(this),200, true, &load);

	return 1;
}

void house_e::attach_implicit_enduses()
{
	IMPLICITENDUSE *item;
	for (item=implicit_enduse_list; item!=NULL; item=item->next){
		attach(NULL,item->amps,item->is220,&(item->load));
	}

	return;
}

/// Attaches an end-use object to a house_e panel
/// The attach() method automatically assigns an end-use load
/// to the first appropriate available circuit.
/// @return pointer to the voltage on the assigned circuit
CIRCUIT *house_e::attach(OBJECT *obj, ///< object to attach
					   double breaker_amps, ///< breaker capacity (in Amps)
					   int is220, ///< 0 for 120V, 1 for 240V load
					   enduse *pLoad) ///< reference to load structure
{
	// construct and id the new circuit
	CIRCUIT *c = new CIRCUIT;
	if (c==NULL)
	{
		gl_error("memory allocation failure");
		return 0;
		/* Note ~ this returns a null pointer, but iff the malloc fails.  If
		 * that happens, we're already in SEGFAULT sort of bad territory. */
	}
	c->next = panel.circuits;
	c->id = panel.circuits ? panel.circuits->id+1 : 1;

	// set the breaker capacity
	c->max_amps = breaker_amps;

	// get address of load values (if any)
	if (pLoad)
		c->pLoad = pLoad;
	else if (obj)
	{
		c->pLoad = (enduse*)gl_get_addr(obj,"enduse_load");
		if (c->pLoad==NULL)
			GL_THROW("end-use load %s couldn't be connected because it does not publish 'enduse_load' property", c->pLoad->name);
	}
	else
			GL_THROW("end-use load couldn't be connected neither an object nor a enduse property was given");
	
	// choose circuit
	if (is220 == 1) // 220V circuit is on x12
	{
		c->type = X12;
		c->id++; // use two circuits
	}
	else if (c->id&0x01) // odd circuit is on x13
		c->type = X13;
	else // even circuit is on x23
		c->type = X23;

	// attach to circuit list
	panel.circuits = c;

	// get voltage
	c->pV = &(pCircuit_V[(int)c->type]);

	// close breaker
	c->status = BRK_CLOSED;

	// set breaker lifetime (at average of 3.5 ops/year, 100 seems reasonable)
	// @todo get data on residential breaker lifetimes (residential, low priority)
	c->tripsleft = 100;

	return c;
}

void house_e::update_model(double dt)
{
#ifndef _DEBUG
	double d;
#endif

	/* compute solar gains */
	Qs = 0; 
	int i;

	for (i=1; i<9; i++) //Compass points of pSolar include direct normal and diffuse radiation into one value
		Qs += pSolar[i];

	Qs *= 3.412 * (gross_wall_area*window_wall_ratio) / 8.0 * (glazing_shgc * window_exterior_transmission_coefficient);

	if (Qs<0)
		throw "solar gain is negative";

	// split gains to air and mass
	Qi = total.heatgain - load.heatgain;
	Qa = Qh + air_heat_fraction*(Qi + Qs);
	Qm = (1-air_heat_fraction)*(Qi + Qs);

	d = Qa + Qm + (Ua+airchange_UA)*Tout;
	Teq = d/c;

	/* compute next initial condition */
	dTa = c2*Tm + c1*Ta - (c1+c2)*Tout + Qa/Ca;
	k1 = (r2*Tair - r2*Teq - dTa)/(r2-r1);
	k2 = Tair - Teq - k1;
}

/** HVAC load synchronizaion is based on the equipment capacity, COP, solar loads and total internal gain
from end uses.  The modeling approach is based on the Equivalent Thermal Parameter (ETP)
method of calculating the air and mass temperature in the conditioned space.  These are solved using
a dual decay solver to obtain the time for next state change based on the thermostat set points.
This synchronization function updates the HVAC equipment load and power draw.
**/

void house_e::update_system(double dt)
{
	// compute system performance 
	/// @todo document COP calculation constants
	const double heating_cop_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_cop_adj = -(-0.0108*(*pTout)+2.0389);
	const double heating_capacity_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_capacity_adj = -(-0.0108*(*pTout)+2.0389);

	switch (system_mode) {
	case SM_HEAT:
		heating_demand = design_heating_capacity*heating_capacity_adj/(heating_COP * heating_cop_adj)*KWPBTUPH;
		system_rated_capacity = design_heating_capacity;//*heating_capacity_adj;
		system_rated_power = heating_demand;
		break;
	case SM_AUX:
		heating_demand = design_heating_capacity*KWPBTUPH;
		system_rated_capacity = design_heating_capacity;
		system_rated_power = heating_demand;
		break;
	case SM_COOL:
		cooling_demand = design_cooling_capacity*cooling_capacity_adj/(1+latent_load_fraction)/(cooling_COP * cooling_cop_adj)*(1+latent_load_fraction)*KWPBTUPH;
		system_rated_capacity = -design_cooling_capacity/(1+latent_load_fraction);//*cooling_capacity_adj
		system_rated_power = cooling_demand;
		break;
	default:
		// two-speed systems use a little power at when off (vent mode)
		system_rated_capacity = 0.0;
		system_rated_power = 0.0;
	}

	/* calculate the power consumption */
	// manually add 'total', we should be unshaped
	// central-air fan consumes only ~5% of total energy when using GAS, 2% when ventilating at low power
	load.current = load.admittance = complex(0,0);
	if(!(system_type&ST_GAS)){
		complex Drawn_load = 0.0;
		complex temp_power;
		complex curr_angle;
		
		//Calculate "current" power draw of furnace
		Drawn_load += (load.total*load.power_fraction);
		
		//Current fraction
		temp_power = ~(load.total*load.current_fraction / 240.0);
		curr_angle.SetPolar(1.0,pCircuit_V[0].Arg());
		Drawn_load += pCircuit_V[0] * ~(temp_power/~curr_angle);

		//Impedance fraction
		if ((load.total.Mag() != 0.0) && (load.impedance_fraction != 0.0))
		{
			temp_power = ~((complex(240.0,0) * complex(240.0,0)) / (load.total*load.impedance_fraction));
			Drawn_load += (pCircuit_V[0] * ~pCircuit_V[0])/(~temp_power);
		}

		// design_heating_capacity scaled by ZIP(voltage)		
		system_rated_capacity *= system_rated_power == 0.0 ? 0.0 : Drawn_load.Mag()/system_rated_power;
	
	}

	if ((system_type&ST_VAR) && (system_mode==SM_OFF))
	{
		load.total =  design_heating_capacity*KWPBTUPH*0.02;
		load.heatgain = design_heating_capacity*0.02;
	}
	else
	{
		load.total = system_rated_power * ((system_mode==SM_HEAT || system_mode==SM_AUX) && (system_type&ST_GAS) ? 0.05 : 1.0);
		load.heatgain = system_rated_capacity;
	}



	if (load.power_factor != 0.0)
	{
		load.power = complex(load.power_fraction * load.total.Re() , load.power_fraction * load.total.Re() * sqrt( 1 / (load.power_factor*load.power_factor) - 1) );
		load.admittance = complex(load.impedance_fraction * load.total.Re() , load.impedance_fraction * load.total.Re() * sqrt( 1 / (load.power_factor*load.power_factor) - 1) );
		load.current = complex(load.current_fraction * load.total.Re(), load.current_fraction * load.total.Re() * sqrt( 1 / (load.power_factor*load.power_factor) - 1) );
	}
	else
	{
		load.power = complex(0,0);
		load.admittance = complex(0,0);
		load.current = complex(0,0);
	}
	hvac_load = load.total.Re();
}

/**  Updates the aggregated power from all end uses, calculates the HVAC kWh use for the next synch time
**/
TIMESTAMP house_e::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	const double dt = (double)((t1-t0)*TS_SECOND)/3600;
	CIRCUIT *c;

	if (*NR_mode == false)
	{
		//Zero the accumulator
		load_values[0][0] = load_values[0][1] = load_values[0][2] = 0.0;
		load_values[1][0] = load_values[1][1] = load_values[1][2] = 0.0;
		load_values[2][0] = load_values[2][1] = load_values[2][2] = 0.0;
	}

	/* advance the thermal state of the building */
	if (t0>0 && dt>0)
	{
		/* calculate model update, if possible */
		if (c2!=0)
		{
			/* update temperatures */
			const double e1 = k1*exp(r1*dt);
			const double e2 = k2*exp(r2*dt);
			Tair = e1 + e2 + Teq;
			Tmaterials = A3*e1 + A4*e2 + Qm/Hm + (Qm+Qa)/(Ua+airchange_UA) + Tout;
		}
	}

	/* update all voltage factors */
	for (c=panel.circuits; c!=NULL; c=c->next)
	{
		// get circuit type
		int n = (int)c->type;
		if (n<0 || n>2)
			GL_THROW("%s:%d circuit %d has an invalid circuit type (%d)", obj->oclass->name, obj->id, c->id, (int)c->type);
		c->pLoad->voltage_factor = c->pV->Mag() / ((c->pLoad->config&EUC_IS220) ? 240 : 120);
	}
	return TS_NEVER;
}

/** Updates the total internal gain and synchronizes with the system equipment load.  
Also synchronizes the voltages and current in the panel with the meter.
**/
TIMESTAMP house_e::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER, t;
	const double dt1 = (double)(t1-t0)*TS_SECOND;

	if (*NR_mode == false)
	{
		/* update HVAC power before panel sync */
		if (t0==0 || t1>t0){
			outside_temperature = *pTout;

			// update the state of the system
			update_system(dt1);
		}

		t2 = sync_enduses(t0, t1);
#ifdef _DEBUG
		gl_debug("house %s (%d) sync_enduses event at '%s'", obj->name, obj->id, gl_strftime(t2));
#endif

		// get the fractions to properly apply themselves
	//	gl_enduse_sync(&(residential_enduse::load),t1);

		// sync circuit panel
		t = sync_panel(t0,t1); 
		if (t < t2) {
			t2 = t;
#ifdef _DEBUG
			gl_debug("house %s (%d) sync_panel event '%s'", obj->name, obj->id, gl_strftime(t2));
#endif
		}

		if (t0==0 || t1>t0)

			// update the model of house
			update_model(dt1);

		// determine temperature of next event
		update_Tevent();

		/* solve for the time to the next event */
		double dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600;

		// if no solution is found or it has already occurred
		if (isnan(dt2) || !isfinite(dt2) || dt2<0)
		{
#ifdef _DEBUG
		gl_debug("house %s (%d) time to next event is indeterminate", obj->name, obj->id);
#endif
			// try again in 1 second if there is a solution in the future
			//if (sgn(dTair)==sgn(Tevent-Tair) && Tevent) 
			//	if (t2>t1) t2 = t1+1;
		}

		// if the solution is less than time resolution
		else if (dt2<TS_SECOND)
		{	
			// need to do a second pass to get next state
			t = t1+1; if (t<t2) t2 = t;
#ifdef _DEBUG
		gl_debug("house %s (%d) time to next event is less than time resolution", obj->name, obj->id);
#endif
		}	
		else
		{
			// next event is found
			t = t1+(TIMESTAMP)(ceil(dt2)*TS_SECOND); if (t<t2) t2 = t;
#ifdef _DEBUG
		gl_debug("house %s (%d) time to next event is %.2f hrs", obj->name, obj->id, dt2/3600);
#endif
		}

#ifdef _DEBUG
		char tbuf[64];
		gl_printtime(t2, tbuf, 64);
		gl_debug("house %s (%d) next event at '%s'", obj->name, obj->id, tbuf);
#endif

		// enforce dwell time
		if (t2!=TS_NEVER)
		{
			double t = ceil((t2<0 ? -t2 : t2)/system_dwell_time)*system_dwell_time;
			t2 = (t2<0 ? -t : t);
		}

		//Update the off-return value
		Off_Return = t2;
		return t2;
	}
	else
	{
		// compute line currents and post to meter
		if (obj->parent != NULL)
			LOCK_OBJECT(obj->parent);

		//Post accumulations up to parent meter/node
		//Update power
		pPower[0] += load_values[0][0];
		pPower[1] += load_values[0][1];
		pPower[2] += load_values[0][2];
		
		//Current
		pLine_I[0] += load_values[1][0];
		pLine_I[1] += load_values[1][1];
		pLine_I[2] += load_values[1][2];
		//Neutral assumed 0, since it was anyways

		//Admittance
		pShunt[0] += load_values[2][0];
		pShunt[1] += load_values[2][1];
		pShunt[2] += load_values[2][2];

		if (obj->parent != NULL)
			UNLOCK_OBJECT(obj->parent);

		return Off_Return;
	}
}

/** Removes load contributions from parent object **/
TIMESTAMP house_e::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);

	// compute line currents and post to meter
	if (obj->parent != NULL)
		LOCK_OBJECT(obj->parent);

	//Post accumulations up to parent meter/node
	//Update power
	pPower[0] -= load_values[0][0];
	pPower[1] -= load_values[0][1];
	pPower[2] -= load_values[0][2];
	
	//Current
	pLine_I[0] -= load_values[1][0];
	pLine_I[1] -= load_values[1][1];
	pLine_I[2] -= load_values[1][2];
	//Neutral not handled in here, since it was always zero anyways

	//Admittance
	pShunt[0] -= load_values[2][0];
	pShunt[1] -= load_values[2][1];
	pShunt[2] -= load_values[2][2];

	if (obj->parent != NULL)
		UNLOCK_OBJECT(obj->parent);

	return TS_NEVER;
}


void house_e::update_Tevent()
{
	OBJECT *obj = OBJECTHDR(this);
	double tdead = thermostat_deadband/2;
	double terr = dTair/3600; // this is the time-error of 1 second uncertainty
	const double TcoolOn = cooling_setpoint+tdead;
	const double TcoolOff = cooling_setpoint-tdead;
	const double TheatOn = heating_setpoint-tdead;
	const double TheatOff = heating_setpoint+tdead;

	// Tevent is based on temperature bracket and assumes state is correct
	switch(system_mode) {

	case SM_HEAT: case SM_AUX: // temperature rising actively
		Tevent = TheatOff;
		break;

	case SM_COOL: // temperature falling actively
		Tevent = TcoolOff;
		break;

	default: // temperature floating passively
		if (dTair<0) // falling
			Tevent = TheatOn;
		else if (dTair>0) // rising 
			Tevent = ( (system_type&ST_AC) ? TcoolOn : warn_high_temp) ;
		else
			Tevent = Tair;
		break;
	}
}

/** The PLC control code for house_e thermostat.  The heat or cool mode is based
	on the house_e air temperature, thermostat setpoints and deadband.
**/
TIMESTAMP house_e::sync_thermostat(TIMESTAMP t0, TIMESTAMP t1)
{
	double tdead = thermostat_deadband/2;
	double terr = dTair/3600; // this is the time-error of 1 second uncertainty
	const double TcoolOn = cooling_setpoint+tdead;
	const double TcoolOff = cooling_setpoint-tdead;
	const double TheatOn = heating_setpoint-tdead;
	const double TheatOff = heating_setpoint+tdead;

	// check for deadband overlap
	if (cooling_setpoint-tdead<heating_setpoint+tdead && (system_type&ST_AC))
	{
		gl_error("house_e: thermostat setpoints deadbands overlap (TcoolOff=%.1f < TheatOff=%.1f)", cooling_setpoint-tdead, heating_setpoint+tdead);
		return TS_INVALID;
	}

	if(system_mode == SM_UNKNOWN)
		system_mode = SM_OFF;
	
	// change control mode if necessary
	switch(system_mode){
		case SM_HEAT:
		case SM_AUX:
			if(Tair > TheatOff - terr/2)
				system_mode = SM_OFF;
			break;
		case SM_COOL:
			if(Tair < TcoolOff - terr/2)
				system_mode = SM_OFF;
			break;
		case SM_OFF:
			if(Tair > TcoolOn - terr/2 && (system_type&ST_AC))
				system_mode = SM_COOL;
			else if(Tair < TheatOn - terr/2)
			{
				if (outside_temperature < aux_cutin_temperature)
					system_mode = SM_AUX;
				else
					system_mode = SM_HEAT;
			}
			break;
	}

	return TS_NEVER;
}

TIMESTAMP house_e::sync_panel(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	// clear accumulator
	if(t0 != 0 && t1 > t0){
		total.heatgain = 0;
	}
	total.total = total.power = total.current = total.admittance = complex(0,0);

	// gather load power and compute current for each circuit
	CIRCUIT *c;
	for (c=panel.circuits; c!=NULL; c=c->next)
	{
		// get circuit type
		int n = (int)c->type;
		if (n<0 || n>2)
			GL_THROW("%s:%d circuit %d has an invalid circuit type (%d)", obj->oclass->name, obj->id, c->id, (int)c->type);

		// if breaker is open and reclose time has arrived
		if (c->status==BRK_OPEN && t1>=c->reclose)
		{
			c->status = BRK_CLOSED;
			c->reclose = TS_NEVER;
			t2 = t1; // must immediately reevaluate devices affected
			gl_debug("house_e:%d panel breaker %d closed", obj->id, c->id);
		}

		// if breaker is closed
		if (c->status==BRK_CLOSED)
		{
			// compute circuit current
			if ((c->pV)->Mag() == 0)
			{
				gl_debug("house_e:%d circuit %d (enduse %s) voltage is zero", obj->id, c->id, c->pLoad->name);
				continue;
			}
			
			complex current = ~(c->pLoad->total*1000 / *(c->pV)); 

			// check breaker
			if (current.Mag()>c->max_amps)
			{
				// probability of breaker failure increases over time
				if (c->tripsleft>0 && gl_random_bernoulli(1/(c->tripsleft--))==0)
				{
					// breaker opens
					c->status = BRK_OPEN;

					// average five minutes before reclosing, exponentially distributed
					c->reclose = t1 + (TIMESTAMP)(gl_random_exponential(1/300.0)*TS_SECOND); 
					gl_debug("house_e:%d circuit breaker %d tripped - enduse %s overload at %.0f A", obj->id, c->id,
						c->pLoad->name, current.Mag());
				}

				// breaker fails from too frequent operation
				else
				{
					c->status = BRK_FAULT;
					c->reclose = TS_NEVER;
					gl_warning("house_e:%d circuit breaker %d failed - enduse %s is no longer running", obj->id, c->id, c->pLoad->name);
				}

				// must immediately reevaluate everything
				t2 = t1; 
			}

			// add to panel current
			else
			{
				//Convert values appropriately - assume nominal voltages of 240 and 120 (0 degrees)
				//All values are given in kW, so convert to normal

				if (n==0)	//1-2 240 V load
				{
					load_values[0][2] += c->pLoad->power * 1000.0;
					load_values[1][2] += ~(c->pLoad->current * 1000.0 / 240.0);
					load_values[2][2] += ~(c->pLoad->admittance * 1000.0 / (240.0 * 240.0));
				}
				else if (n==1)	//2-N 120 V load
				{
					load_values[0][1] += c->pLoad->power * 1000.0;
					load_values[1][1] += ~(c->pLoad->current * 1000.0 / 120.0);
					load_values[2][1] += ~(c->pLoad->admittance * 1000.0 / (120.0 * 120.0));
				}
				else	//n has to equal 2 here (checked above) - 1-N 120 V load
				{
					load_values[0][0] += c->pLoad->power * 1000.0;
					load_values[1][0] += ~(c->pLoad->current * 1000.0 / 120.0);
					load_values[2][0] += ~(c->pLoad->admittance * 1000.0 / (120.0 * 120.0));
				}

				//load_values[0][1] += c->pLoad->power * 1000.0;
				//load_values[1][1] += ~(c->pLoad->current * 1000.0 / V);
				//load_values[2][1] += ~(c->pLoad->admittance * 1000.0 / (V*V));

				total.total += c->pLoad->total;
				total.power += c->pLoad->power;
				total.current += c->pLoad->current;
				total.admittance += c->pLoad->admittance;
				if(t0 != 0 && t1 > t0){
					total.heatgain += c->pLoad->heatgain;
				}
				c->reclose = TS_NEVER;
			}
		}

		// sync time
		if (t2 > c->reclose)
			t2 = c->reclose;
	}
	/* using an enduse structure for the total is more a matter of having all the values add up for the house,
	 * and it should not sync the struct! ~MH */
	//TIMESTAMP t = gl_enduse_sync(&total,t1); if (t<t2) t2 = t;

	// compute line currents and post to meter
	if (obj->parent != NULL)
		LOCK_OBJECT(obj->parent);

	//Post accumulations up to parent meter/node
	//Update power
	pPower[0] += load_values[0][0];
	pPower[1] += load_values[0][1];
	pPower[2] += load_values[0][2];
	
	//Current
	pLine_I[0] += load_values[1][0];
	pLine_I[1] += load_values[1][1];
	pLine_I[2] += load_values[1][2];
	//Neutral assumed 0, since it was anyways

	//Admittance
	pShunt[0] += load_values[2][0];
	pShunt[1] += load_values[2][1];
	pShunt[2] += load_values[2][2];

	if (obj->parent != NULL)
		UNLOCK_OBJECT(obj->parent);

	return t2;
}

TIMESTAMP house_e::sync_enduses(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	IMPLICITENDUSE *eu;
	for (eu=implicit_enduse_list; eu!=NULL; eu=eu->next)
	{
		TIMESTAMP t = 0;
		t = gl_enduse_sync(&(eu->load),t1);
		if (t<t2) t2 = t;
	}
	return t2;
}

void house_e::check_controls(void)
{
	if (warn_control)
	{
		OBJECT *obj = OBJECTHDR(this);
		/* check for air temperature excursion */
		if (Tair<warn_low_temp || Tair>warn_high_temp)
		{
			gl_warning("house_e:%d (%s) air temperature excursion (%.1f degF) at %s", 
				obj->id, obj->name?obj->name:"anonymous", Tair, gl_strftime(obj->clock));
		}

		/* check for mass temperature excursion */
		if (Tmaterials<warn_low_temp || Tmaterials>warn_high_temp)
		{
			gl_warning("house_e:%d (%s) mass temperature excursion (%.1f degF) at %s", 
				obj->id, obj->name?obj->name:"anonymous", Tmaterials, gl_strftime(obj->clock));
		}

		/* check for heating equipment sizing problem */
		if ((system_mode==SM_HEAT || system_mode==SM_AUX) && Teq<heating_setpoint)
		{
			gl_warning("house_e:%d (%s) heating equipement undersized at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_strftime(obj->clock));
		}

		/* check for cooling equipment sizing problem */
		else if (system_mode==SM_COOL && (system_type&ST_AC) && Teq>cooling_setpoint)
		{
			gl_warning("house_e:%d (%s) cooling equipement undersized at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_strftime(obj->clock));
		}

		/* check for invalid event temperature */
		if ((dTair>0 && Tevent<Tair) || (dTair<0 && Tevent>Tair))
		{
			gl_warning("house_e:%d (%s) possible control problem (system_mode %s) -- Tevent-Tair mismatch with dTair (Tevent=%.1f, Tair=%.1f, dTair=%.1f) at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_getvalue(obj,"system_mode"), Tevent, Tair, dTair, gl_strftime(obj->clock));
		}
	}
}

complex *house_e::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

bool *house_e::get_bool(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_house(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(house_e::oclass);
	if (*obj!=NULL)
	{
		house_e *my = OBJECTDATA(*obj,house_e);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_house(OBJECT *obj)
{
	try {
		house_e *my = OBJECTDATA(obj,house_e);
		my->init_climate();
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("house_e:%d (%s) %s", obj->id, obj->name?obj->name:"anonymous", msg);
		return 0;
	}
}

EXPORT TIMESTAMP sync_house(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	house_e *my = OBJECTDATA(obj,house_e);
	TIMESTAMP t1 = TS_NEVER;
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the object clock if it has not been set yet

	try {
		switch (pass) 
		{
			case PC_PRETOPDOWN:
				t1 = my->presync(obj->clock, t0);
				break;

			case PC_BOTTOMUP:
				t1 = my->sync(obj->clock, t0);
				obj->clock = t0;
				break;
			case PC_POSTTOPDOWN:
				t1 = my->postsync(obj->clock, t0);
				obj->clock = t0;
				break;
			default:
				gl_error("house_e::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
	} 
	catch (char *msg)
	{
		gl_error("house_e::sync exception caught: %s", msg);
		t1 = TS_INVALID;
	}
	catch (...)
	{
		gl_error("house_e::sync exception caught: no info");
		t1 = TS_INVALID;
	}
	return t1;
}

EXPORT TIMESTAMP plc_house(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the waterheater
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the clock if it has not been set yet

	house_e *my = OBJECTDATA(obj,house_e);
	return my->sync_thermostat(obj->clock, t0);
}

/**@}**/
