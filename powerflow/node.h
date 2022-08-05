/** $Id: node.h 1201 2009-01-08 22:31:37Z d3x593 $
	Copyright (C) 2008 Battelle Memorial Institute
	@addtogroup powerflow_node Node
	@ingroup powerflow_object
**/

#ifndef _NODE_H
#define _NODE_H

#include "powerflow.h"

//Deltamode functions
EXPORT SIMULATIONMODE interupdate_node(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
EXPORT STATUS swap_node_swing_status(OBJECT *obj, bool desired_status);
EXPORT STATUS node_swing_status(OBJECT *this_obj, bool *swing_status_check_value, bool *swing_pq_status_value);
EXPORT STATUS node_reset_disabled_status(OBJECT *nodeObj);
EXPORT STATUS node_map_current_update_function(OBJECT *nodeObj, OBJECT *callObj);
EXPORT STATUS attach_vfd_to_node(OBJECT *obj,OBJECT *calledVFD);
EXPORT STATUS node_update_shunt_values(OBJECT *obj);

#define I_INJ(V, S, Z, I) (I_S(S, V) + ((Z.IsFinite()) ? I_Z(Z, V) : gld::complex(0.0)) + I_I(I))
#define I_S(S, V) (~((S) / (V)))  // Current injection - constant power load
#define I_Z(Z, V) ((V) / (Z))     // Current injection - constant impedance load
#define I_I(I) (I)                // Current injection - constant current load

//Moved the following to powerflow_object.h, since they are just #defines
//#define NF_NONE			0x0000	///< flag indicates node has no special conditions */
//#define NF_HASSOURCE	0x0001	///< flag indicates node has a source for voltage */
//#define NF_ISSOURCE		0x0002	///< flag indicates node has a source connected directly to it */

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

#define SNT_NONE 0x0000			///< defines not a child node
#define SNT_CHILD 0x0001		///< defines is a child node
#define SNT_CHILD_NOINIT 0x0002	///< defines is a child node that has not been linked
#define SNT_PARENT 0x0004		///< defines is a parent of a child
#define SNT_DIFF_CHILD 0x0008	///< defines is a child node, but has different phase-connection that our parent
#define SNT_DIFF_PARENT 0x0010	///< defines a parent, but has a different phase-connection than our child

typedef enum {
	NORMAL_NODE=0,		///< We're a plain-old-ugly node
	LOAD_NODE=1,		///< We're a load
	METER_NODE=2		///< We're a meter
} DYN_NODE_TYPE;		/// Definition for deltamode calls

//Frequency measurement variable structure
typedef struct {
	gld::complex voltage_val[3];	//Voltage values stored - used for "prev" version
	double x[3]; 		     //integrator state variable
	double anglemeas[3];	 //angle measurement
	double fmeas[3];		 //frequency measurement
	double average_freq;	//Average of the three-phased frequencies
	double sinangmeas[3];	 //sin of bus voltage angle
	double cosangmeas[3];	 //cos of bus voltage angle
} FREQM_STATES;


class node : public powerflow_object
{
private:
	gld::complex last_voltage[3];		///< voltage at last pass
	gld::complex current_inj[3];			///< current injection (total of current+shunt+power)
	TIMESTAMP prev_NTime;			///< Previous timestep - used for propogating child properties
	gld::complex last_child_power[4][3];	///< Previous power values - used for child object propogation
	gld::complex last_child_power_dy[6][3];	///< Previous power values joint - used for child object propogation
	gld::complex last_child_current12;	///< Previous current value - used for child object propogation (namely triplex)
	bool deltamode_inclusive;		///< Flag for deltamode functionality, just to prevent having to mask the flags
	gld::complex BusHistTerm[3];			///< Pointer for array used to store load history value for deltamode-based in-rush computations
	double prev_delta_time;			///< Tracking variable for last time deltamode call occurred - used for "once a timestep" in-rush computations
	gld::complex *ahrlloadstore;			///< Pointer for array used to store load history constant ahrl -- associated with inductance
	gld::complex *bhrlloadstore;			///< Pointer for array used to store load history constant bhrl -- associated with inductance
	gld::complex *chrcloadstore;			///< Pointer for array used to store load history constant chrc -- associated with capacitance
	gld::complex *LoadHistTermL;			///< Pointer for array used to store load history value for deltamode-based in-rush computations -- Inductive terms
	gld::complex *LoadHistTermC;			///< Pointer for array used to store load history value for deltamode-based in-rush computations -- Shunt capacitance terms

	gld::complex shunt_change_check[3];		///< Change tracker for FPI - to trigger update of "standard" loads
	gld::complex shunt_change_check_dy[6];	///< Change tracker for FPI - trigger update of explicit Wye-Delta loads
	gld::complex *Extra_Data_Track_FPI;		///Link to extra data tracker information (NR-FPI)

	//Frequently measurement variables
	FREQM_STATES curr_freq_state;		//Current state of all vari
	FREQM_STATES prev_freq_state;
	double freq_omega_ref;			//Reference frequency for measurements

	bool first_freq_init;	//Flag to indicate if the first init for frequency - prevents from overwriting an off-nominal frequency

	double freq_violation_time_total;		//Keeps track of how long the device has been in a frequency violation, to see if it needs to disconnect or not
	double volt_violation_time_total;		//Keeps track of how long the device has been in a voltage violation, to see if it needs to disconnect or not
	double out_of_violation_time_total;		//Tracking variable to see how long we've been "outside of bad conditions"
	double prev_time_dbl;					//Tracking variable for GFA functionality
	double GFA_Update_time;

	//Deltamode interfacing matrices
	GL_STRUCT(complex_array,full_Y_matrix);
	GL_STRUCT(complex_array,full_Y_all_matrix);

	//Swing designation
	bool swing_functions_enabled;			//Flag to indicate if a bus is behaving as a swing

	//VFD-related items
	bool VFD_attached;						///< Flag to indicate this is on the to-side of a VFD link
	FUNCTIONADDR VFD_updating_function;		///< Address for VFD updating function, if it is present
	OBJECT *VFD_object;						///< Object pointer for the VFD - for later function calls

	gld::complex *tn_values;		//Variable (mostly for FBS) to map triplex multipliers for neutral current.  Mostly so saves API calls.

	double compute_angle_diff(double angle_B, double angle_A);	//Function to do differences, but handle the phase wrap/jump
	STATUS shunt_update_fxn(void);	//Function to do shunt update for FPI, so sequencing happens correctly

public:
	double frequency;			///< frequency (only valid on reference bus) */
	object reference_bus;		///< reference bus from which frequency is defined */
	static unsigned int n;		///< node count */
	unsigned short k;			///< incidence count (number of links connecting to this node) */
	gld::complex *prev_voltage_value;	// Pointer for array used to store previous voltage value for Master/Slave functionality
	gld::complex *prev_power_value;		// Pointer for array used to store previous power value for Master/Slave functionality

	bool reset_island_state;			//< Flagging variable - indicates the disabled island state should be re-evaluated
public:
	// status
	enum {
		PQ=0,		/**< defines an uncontrolled bus */
		PV=1,		/**< defines a constrained voltage controlled bus */
		SWING=2,	/**< defines an unconstrained voltage controlled bus */
		SWING_PQ=3	/**< defines a bus that is an unconstrained voltage controlled bus, unless another exists on the system. */
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
	enum {
		GFA_NONE=0,		/**< Defines no trip reason */
		GFA_UF=1,		/**< Defines under-frequency trip */
		GFA_OF=2,		/**< Defines over-frequency trip */
		GFA_UV=3,		/**< Defines under-voltage trip */
		GFA_OV=4		/**< Defines over-voltage trip */
	};
	enumeration GFA_trip_method;
	enumeration service_status;
	double service_status_dbl;	///< double value for service - overrides the enumeration if set
	TIMESTAMP last_disconnect;	///< Tracking variable for out of service times
	double previous_uptime;		///< Variable for storing last total uptime
	double current_uptime;		///< Variable for storing current uptime

	//Frequency measurement capabilities
	enum {FM_NONE=1, FM_SIMPLE=2, FM_PLL=3};
	enumeration fmeas_type;

	double freq_sfm_Tf;	//Transducer time constant
	double freq_pll_Kp;	//Proportional gain of PLL frequency measurement
	double freq_pll_Ki;	//Integration gain of PLL frequency measurement

	//GFA functionality
	bool GFA_enable;
	double GFA_freq_low_trip;
	double GFA_freq_high_trip;
	double GFA_voltage_low_trip;
	double GFA_voltage_high_trip;
	double GFA_reconnect_time;
	double GFA_freq_disconnect_time;
	double GFA_volt_disconnect_time;
	bool GFA_status;

	set SubNode;			///< Node child/subtype flags (for NR)
	set busflags;			///< node flags (see NF_*)
	set busphasesIn;		///< phase check flags for "reconvergent" lines (input)
	set busphasesOut;		///< phase check flags for output
	double maximum_voltage_error;  // convergence voltage limit

	// properties
	gld::complex voltage[3];		/// bus voltage to ground
	gld::complex voltaged[3];	/// bus voltage differences
	gld::complex current[3];		/// bus current injection (positive = in)
	gld::complex pre_rotated_current[3];	/// bus current that has been rotated already for deltamode (direct post to powerflow)
	gld::complex deltamode_dynamic_current[3];	/// bus current that is pre-rotated, but also has ability to be reset within powerflow
	gld::complex deltamode_PGenTotal;			/// Bus generated power - used deltamode
	gld::complex power[3];		/// bus power injection (positive = in)
	gld::complex shunt[3];		/// bus shunt admittance 
	gld::complex current_dy[6];	/// bus current injection (positive = in), explicitly specify delta and wye portions
	gld::complex power_dy[6];	/// bus power injection (positive = in), explicitly specify delta and wye portions
	gld::complex shunt_dy[6];	/// bus shunt admittance, explicitly specify delta and wye portions
	gld::complex *full_Y;		/// full 3x3 bus shunt admittance - populate as necessary
	gld::complex full_Y_load[3][3];	/// 3x3 bus shunt admittance - meant to update - used for in-rush or FPI
	gld::complex *full_Y_all;	/// Full 3x3 bus admittance with "other" contributions (self of full admittance) - populate as necessary
	DYN_NODE_TYPE node_type;/// Variable to indicate what we are - prevents needing a gl_object_isa EVERY...SINGLE...TIME in an already slow dynamic simulation
	gld::complex current12;		/// Used for phase 1-2 current injections in triplex
	gld::complex nom_res_curr[3];/// Used for the inclusion of nominal residential currents (for angle adjustments)
	bool house_present;		/// Indicator flag for a house being attached (NR primarily)
	bool dynamic_norton;	/// Norton-equivalent posting on this bus -- deltamode and diesel generator ties
	bool dynamic_norton_child;	/// Flag to indicate a childed node object is posting to this bus - prevents a double-accumulation in meters
	bool dynamic_generator;	/// Swing-type generator posting on this bus -- deltamode and other generator ties
	gld::complex *Triplex_Data;	/// Link to triplex line for extra current calculation information (NR)
	gld::complex *Extra_Data;	/// Link to extra data information (NR)
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
	static node *defaults;
public:
	node(MODULE *mod);
	inline node(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	static int isa(const char *classname);
	int notify(int update_mode, PROPERTY *prop, char *value);
	SIMULATIONMODE inter_deltaupdate_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

	//Functionalized portions for deltamode calls -- allows updates
	TIMESTAMP NR_node_presync_fxn(TIMESTAMP t0_val);
	void NR_node_sync_fxn(OBJECT *obj);
	void BOTH_node_postsync_fxn(OBJECT *obj);
	OBJECT *NR_master_swing_search(const char *node_type_value,bool main_swing);

	void init_freq_dynamics(double deltat);
	STATUS calc_freq_dynamics(double deltat);

	double perform_GFA_checks(double timestepvalue);

	STATUS link_VFD_functions(OBJECT *linkVFD);

	bool current_accumulated;

	int NR_populate(void);
	OBJECT *SubNodeParent;	/// Child node's original parent or child of parent
	int NR_current_update(bool parentcall);
	object TopologicalParent;	/// Child node's original parent as per the topological configuration in the GLM file

	//NR bus status toggle function
	STATUS NR_swap_swing_status(bool desired_status);

	//NR bus swing-status check
	void NR_swing_status_check(bool *swing_status_check_value, bool *swing_pq_status_value);

	//Island-condition reset function
	STATUS reset_node_island_condition(void);

	//Function to map "internal powerflow iteration" current injection updates
	STATUS NR_map_current_update_function(OBJECT *callObj);

	friend class link_object;
	friend class meter;	// needs access to current_inj
	friend class substation; //needs access to current_inj
	friend class triplex_node;	// Needs access to deltamode stuff
	friend class triplex_meter; // needs access to current_inj
	friend class triplex_load;	// Needs access to deltamode stuff
	friend class load;			// Needs access to deltamode_inclusive
	friend class capacitor;		// Needs access to deltamode stuff
	friend class fuse;			// needs access to current_inj
	friend class motor;	// needs access to curr_state
	friend class performance_motor;	//needs access to curr_state

	static int kmlinit(int (*stream)(const char*,...));
	int kmldump(int (*stream)(const char*,...));
};

#endif // _NODE_H

