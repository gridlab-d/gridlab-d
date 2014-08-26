// $Id: link.h 1211 2009-01-17 00:45:28Z d3x593 $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _LINK_H
#define _LINK_H

#include "powerflow.h"

EXPORT int isa_link(OBJECT *obj, char *classname);
EXPORT SIMULATIONMODE interupdate_link(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

#define impedance(X) (B_mat[X][X])
//#define LS_CLOSED true	//Changed from enums so it could be used by restoration
//#define LS_OPEN false

typedef enum {
		NORMAL=0,			///< defines just a normal link/transformer
		REGULATOR=1,		///< defines the link is a regulator
		DELTAGWYE=2,		///< defines the link is actually a Delta-Gwye transformer
		SPLITPHASE=3,		///< defines the link is a split-phase transformer
		SWITCH=4,			///< defines the link is a switch
		DELTADELTA=5,		///< defines the link is a delta-delta transformer - for power operations
		WYEWYE=6			///< defines the link is a Wye-wye transformer, mainly to allow triggering for unity turns ratios
} SPECIAL_LINK;

// flow directions (see link_object::flow_direction)
#define FD_UNKNOWN		0x000	///< Flow is undetermined
#define FD_A_MASK		0x00f	///< mask to isolate phase A flow information
#define FD_A_NORMAL		0x001	///< Flow over phase A is normal
#define FD_A_REVERSE	0x002	///< Flow over phase A is reversed
#define FD_A_NONE		0x003	///< No flow over of phase A 
#define FD_B_MASK		0x0f0	///< mask to isolate phase B flow information
#define FD_B_NORMAL		0x010	///< Flow over phase B is normal
#define FD_B_REVERSE	0x020	///< Flow over phase B is reversed
#define FD_B_NONE		0x030	///< No flow over of phase B 
#define FD_C_MASK		0xf00	///< mask to isolate phase C flow information
#define FD_C_NORMAL		0x100	///< Flow over phase C is normal
#define FD_C_REVERSE	0x200	///< Flow over phase C is reversed
#define FD_C_NONE		0x300	///< No flow over of phase C 

class link_object : public powerflow_object
{
public: /// @todo make this private and create interfaces to control values
	complex a_mat[3][3];	// a_mat - 3x3 matrix, 'a' matrix
	complex b_mat[3][3];	// b_mat - 3x3 matrix, 'b' matrix
	complex c_mat[3][3];	// c_mat - 3x3 matrix, 'c' matrix
	complex d_mat[3][3];	// d_mat - 3x3 matrix, 'd' matrix
	complex A_mat[3][3];	// A_mat - 3x3 matrix, 'A' matrix
	complex B_mat[3][3];	// B_mat - 3x3 matrix, 'B' matrix
	complex tn[3];			// Used to calculate return current
	complex To_Y[3][3];		// To_Y  - 3x3 matrix, object transition to admittance
	complex From_Y[3][3];	// From_Y - 3x3 matrix, object transition from admittance
	complex *YSfrom;		// YSfrom - Pointer to 3x3 matrix representing admittance seen from "from" side (transformers)
	complex *YSto;			// YSto - Pointer to 3x3 matrix representing admittance seen from "to" side (transformers)
	double voltage_ratio;	// voltage ratio (normally 1.0)
	int NR_branch_reference;	//Index of NR_branchdata this link is contained in
	SPECIAL_LINK SpecialLnk;	//Flag for exceptions to the normal handling
	set flow_direction;		// Flag direction of powerflow: 1 is normal, -1 is reverse flow, 0 is no flow
	void calculate_power();
	void calculate_power_splitphase();
	void set_flow_directions();
	void calc_currents(complex *Current_Vals);	//Function to perform "immediate" current calculation - used by restoration object
	int link_fault_on(OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data);		//Function to create fault on line
	int link_fault_off(int *implemented_fault, char *imp_fault_name, void *Extra_Data);	//Function to remove fault from line
	double mean_repair_time;
	double *link_limits[2];		/**< pointers for line limits (emergency vs. continuous) for link objects - pointered for variation */
	double link_rating[2];		/**< Values for current line rating - gives individual segments the ability to set */
	double *get_double(OBJECT *obj, char *name);	/**< Gets address of double - mainly for mean_repair_time */
public:
	typedef enum {
		LS_OPEN=0,			///< defines that that link is open
		LS_CLOSED=1			///< defines that that link is closed
	} STATUS;
	enumeration status;
	//bool status;	///< link status (open disconnect nodes)
	enumeration prev_status;
	//bool prev_status;	///< Previous link status (used for recalculation detection)

	bool current_accumulated;	///< Flag to indicate if NR current has been "handled" yet
	bool check_link_limits;	///< Flag to see if this particular link needs limits checked
	OBJECT *from;			///< from_node - source node
	OBJECT *to;				///< to_node - load node
	complex current_in[3];		///< current flow to link (w.r.t from node)
	complex current_out[3];	///< current flow out of link (w.r.t. to node)
	complex read_I_in[3];	///< published current flow to link (w.r.t from node)
	complex read_I_out[3];  ///< published current flow out of link (w.r.t to node)
	complex If_in[3];		///< fault current flowing in 
	complex If_out[3];		///< fault current flowing out
	complex power_in;		///< power flow in (w.r.t from node)
	complex power_out;		///< power flow out (w.r.t to node)
	complex power_loss;		///< power losses 
	complex indiv_power_in[3];	///< power flow in (w.r.t. from node) - individual quantities
	complex indiv_power_out[3];	///< power flow out (w.r.t. to node) - individual quantities
	complex indiv_power_loss[3];///< power losses - individual quantities
	int protect_locations[3];	///< Links to protection object for different phase faults - part of reliability

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP prev_LTime;
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	link_object(MODULE *mod);
	link_object(CLASS *cl=oclass):powerflow_object(cl){};
	static CLASS *oclass;
	static CLASS *pclass;
	int isa(char *classname);
public:
	/* status values */
	set affected_phases;				/* use this to determine which phases are affected by status change */
	#define IMPEDANCE_CHANGED		1	/* use this status to indicate an impedance change (e.g., line contact) */
	double resistance;					/* use this resistance when status=IMPEDANCE_CHANGED */
	#define LINE_CONTACT			2	/* use this to indicate line contact */
	set line_contacted;					/* use this to indicate which line was contacted (N means ground) */
	#define CONTROL_FAILED			4	/* use this status to indicate a controller failure (e.g., PLC failure) */

	class node *get_from(void) const;
	class node *get_to(void) const;
	set get_flow(class node **from, class node **to) const; /* determine flow direction (return phases on which flow is reverse) */

	inline enumeration open(void) { enumeration previous=status; status=LS_OPEN; return previous;};
	inline enumeration close(void) { enumeration previous=status; status=LS_CLOSED; return previous;};
	inline enumeration is_open(void) const { return status==LS_OPEN;};
	inline enumeration is_closed(void) const { return status==LS_CLOSED;};
	inline enumeration get_status(void) const {return status;};

	bool is_frequency_nominal();
	bool is_voltage_nominal();

	int kmldump(int (*stream)(const char*,...));
	//Current injection calculation function - so it can be called remotely
	int CurrentCalculation(int nodecall);

	void NR_link_presync_fxn(void);
	void BOTH_link_postsync_fxn(void);
	bool deltamode_inclusive;

	// Fault current calculation functions
	void fault_current_calc(complex C[7][7], unsigned int removed_phase, double fault_type); // function traces up from fault to swing bus summing up the link objects' impedances
											  // then calculates the fault current then passes that value back down to the faulted link objects.

	SIMULATIONMODE inter_deltaupdate_link(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

};

//Macros
void inverse(complex in[3][3], complex out[3][3]);
void multiply(double a, complex b[3][3], complex c[3][3]);
void multiply(complex a[3][3], complex b[3][3], complex c[3][3]);
void subtract(complex a[3][3], complex b[3][3], complex c[3][3]);
void addition(complex a[3][3], complex b[3][3], complex c[3][3]);
void equalm(complex a[3][3], complex b[3][3]);
void lu_decomp(complex a[7][7], complex l[7][7], complex u[7][7]); //lu decomposition function for a 7 by 7 matrix
void forward_sub(complex l[7][7], complex b[7], complex z[7]); //backwards substitution  algorithm for a 7 by 7 system 
void back_sub(complex u[7][7], complex z[7], complex x[7]); // forwards substitution algorithm for a 7 by 7 system
#endif // _LINK_H

