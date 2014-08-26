/** $Id: node.h 1201 2009-01-08 22:31:37Z d3x593 $
	Copyright (C) 2008 Battelle Memorial Institute
	@addtogroup powerflow_node Node
	@ingroup powerflow_object
**/

#ifndef _NODE_H
#define _NODE_H

#include "powerflow.h"

//Deltamode funcitons
EXPORT complex *delta_linkage(OBJECT *obj, unsigned char mapvar);
EXPORT STATUS delta_frequency_node(OBJECT *obj, complex *powerval, complex *freqpowerval);
EXPORT SIMULATIONMODE interupdate_node(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

#define I_INJ(V, S, Z, I) (I_S(S, V) + ((Z.IsFinite()) ? I_Z(Z, V) : complex(0.0)) + I_I(I))
#define I_S(S, V) (~((S) / (V)))  // Current injection - constant power load
#define I_Z(Z, V) ((V) / (Z))     // Current injection - constant impedance load
#define I_I(I) (I)                // Current injection - constant current load

#define NF_NONE			0x0000	///< flag indicates node has no special conditions */
#define NF_HASSOURCE	0x0001	///< flag indicates node has a source for voltage */

// these are only valid when PHASE_S is not set
#define voltageA voltage[0]		/// phase A voltage to ground
#define voltageB voltage[1]		/// phase B voltage to ground
#define voltageC voltage[2]		/// phase C voltage to ground

#define voltageAB voltaged[0]	/// phase A-B voltage
#define voltageBC voltaged[1]	/// phase B-C voltage
#define voltageCA voltaged[2]	/// phase C-A voltage

#define currentA current[0]		/// phase A current accumulator
#define currentB current[1]		/// phase B current accumulator
#define currentC current[2]		/// phase C current accumulator

#define powerA power[0]			/// phase A power injection accumulator (AB for Delta)
#define powerB power[1]			/// phase B power injection accumulator (BC for Delta)
#define powerC power[2]			/// phase C power injection accumulator (CA for Delta)

#define shuntA shunt[0]			/// phase A shunt admittance accumulator (reset to 1/impedance on each pass)
#define shuntB shunt[1]			/// phase B shunt admittance accumulator (reset to 1/impedance on each pass)
#define shuntC shunt[2]			/// phase C shunt admittance accumulator (reset to 1/impedance on each pass)

// these are only valid when phases&PHASE_S is set
#define voltage1 voltage[0]		/// phase 1 voltage to ground
#define voltage2 voltage[1]		/// phase 2 voltage to ground
#define voltageN voltage[2]		/// phase N voltage to ground

#define voltage12 voltaged[0]	/// phase 1-2 voltage
#define voltage2N voltaged[1]	/// phase 2-N voltage
#define voltage1N voltaged[2]	/// phase 1-N voltage (note the sign change)

#define current1 current[0]		/// line 1 constant current accumulator
#define current2 current[1]		/// line 2 constant current accumulator
#define currentN current[2]	    /// line N constant current accumulator

#define power1 power[0]			/// phase 1 constant power load
#define power2 power[1]			/// phase 2 constant power load
#define power12 power[2]		/// phase 1-2 constant power load

#define shunt1 shunt[0]			/// phase 1 constant admittance load
#define shunt2 shunt[1]			/// phase 2 constant admittance load
#define shunt12 shunt[2]		/// phase 1-2 constant admittance load

typedef enum {
		NONE=0,			///< defines not a child node
		CHILD=1,		///< defines is a child node
		CHILD_NOINIT=2,	///< defines is a child node that has not been linked
		PARENT=3,		///< defines is a parent of a child
		DIFF_CHILD=4,	///< defines is a child node, but has different phase-connection that our parent
		DIFF_PARENT=5	///< defines a parent, but has a different phase-connection than our child
		} SUBNODETYPE;

typedef enum {
	NORMAL_NODE=0,		///< We're a plain-old-ugly node
	LOAD_NODE=1,		///< We're a load
	METER_NODE=2		///< We're a meter
} DYN_NODE_TYPE;		/// Defition for deltamode calls

class node : public powerflow_object
{
private:
	complex last_voltage[3];		///< voltage at last pass
	complex current_inj[3];			///< current injection (total of current+shunt+power)
	TIMESTAMP prev_NTime;			///< Previous timestep - used for propogating child properties
	complex last_child_power[3][3];	///< Previous power values - used for child object propogation
	complex last_child_power_dy[6][3];	///< Previous power values joint - used for child object propogation
	complex last_child_current12;	///< Previous current value - used for child object propogation (namely triplex)
	bool deltamode_inclusive;		///< Flag for deltamode functionality, just to prevent having to mask the flags
public:
	double frequency;			///< frequency (only valid on reference bus) */
	object reference_bus;		///< reference bus from which frequency is defined */
	static unsigned int n;		///< node count */
	unsigned short k;			///< incidence count (number of links connecting to this node) */
	complex *prev_voltage_value;	// Pointer for array used to store previous voltage value for Master/Slave functionality
	complex *prev_power_value;		// Pointer for array used to store previous power value for Master/Slave functionality
public:
	// status
	enum {
		PQ=0,		/**< defines an uncontrolled bus */
		PV=1,		/**< defines a constrained voltage controlled bus */
		SWING=2		/**< defines an unconstrained voltage controlled bus */
	};
	enumeration bustype;
	enum {	NOMINAL=1,		///< bus voltage is nominal
			UNDERVOLT,		///< bus voltage is too low
			OVERVOLT,		///< bus voltage is too high
	};
	enumeration status;
	enum { 
		ND_OUT_OF_SERVICE = 0, ///< out of service flag for nodes
		ND_IN_SERVICE = 1,     ///< in service flag for nodes - default
	};
	enumeration service_status;
	double service_status_dbl;	///< double value for service - overrides the enumeration if set
	TIMESTAMP last_disconnect;	///< Tracking variable for out of service times
	double previous_uptime;		///< Variable for storing last total uptime
	double current_uptime;		///< Variable for storing current uptime

	SUBNODETYPE SubNode;
	set busflags;			///< node flags (see NF_*)
	set busphasesIn;		///< phase check flags for "reconvergent" lines (input)
	set busphasesOut;		///< phase check flags for output
	double maximum_voltage_error;  // convergence voltage limit

	// properties
	complex voltage[3];		/// bus voltage to ground
	complex voltaged[3];	/// bus voltage differences
	complex current[3];		/// bus current injection (positive = in)
	complex power[3];		/// bus power injection (positive = in)
	complex shunt[3];		/// bus shunt admittance 
	complex current_dy[6];	/// bus current injection (positive = in), explicitly specify delta and wye portions
	complex power_dy[6];	/// bus power injection (positive = in), explicitly specify delta and wye portions
	complex shunt_dy[6];	/// bus shunt admittance, explicitly specify delta and wye portions
	complex *full_Y;		/// full 3x3 bus shunt admittance - populate as necessary
	complex *full_Y_all;	/// Full 3x3 bus admittance with "other" contributions (self of full admittance) - populate as necessary
	complex *DynVariable;	/// Dynamics extra variable (current and power for gens, typically)
	DYN_NODE_TYPE node_type;/// Variable to indicate what we are - prevents needing a gl_object_isa EVERY...SINGLE...TIME in an already slow dynamic simulation
	complex current12;		/// Used for phase 1-2 current injections in triplex
	complex nom_res_curr[3];/// Used for the inclusion of nominal residential currents (for angle adjustments)
	bool house_present;		/// Indicator flag for a house being attached (NR primarily)
	bool dynamic_norton;	/// Norton-equivalent posting on this bus -- deltamode and generator ties
	complex *Triplex_Data;	/// Link to triplex line for extra current calculation information (NR)
	complex *Extra_Data;	/// Link to extra data information (NR)
	unsigned int NR_connected_links[2];	/// Counter for number of connected links in the system
	unsigned int NR_number_child_nodes[2];	/// Counter for number of childed nodes we have (for later NR linking)
	node **NR_child_nodes;	/// Pointer to childed nodes list
	int *NR_link_table;		/// Pointer to link list table
	
	double mean_repair_time;	/// Node's mean repair time - mainly for swing at this point

	int NR_node_reference;		/// Node's reference in NR_busdata
	int *NR_subnode_reference;	/// Pointer to parent node's reference in NR_busdata - just in case things get inited out of synch
	unsigned char prev_phases;	/// Phase tracking variable for use in reliability calls

	inline bool is_split() {return (phases&PHASE_S)!=0;};
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	node(MODULE *mod);
	inline node(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	int isa(char *classname);
	int notify(int update_mode, PROPERTY *prop, char *value);
	SIMULATIONMODE inter_deltaupdate_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

	//Functionalized portions for deltamode calls -- allows updates
	void NR_node_presync_fxn(void);
	void NR_node_sync_fxn(OBJECT *obj);
	void BOTH_node_postsync_fxn(OBJECT *obj);

	bool current_accumulated;

	int NR_populate(void);
	OBJECT *SubNodeParent;	/// Child node's original parent or child of parent
	int NR_current_update(bool postpass, bool parentcall);
	object TopologicalParent;	/// Child node's original parent as per the topological configuration in the GLM file

	friend class link_object;
	friend class meter;	// needs access to current_inj
	friend class substation; //needs access to current_inj
	friend class triplex_meter; // needs access to current_inj
	friend class load;			// Needs access to deltamode_inclusive
	friend class fuse;			// needs access to current_inj
	friend class frequency_gen;	// needs access to current_inj

	int kmldump(int (*stream)(const char*,...));
};

#include "load.h"
#include "triplex_node.h"

#endif // _NODE_H

