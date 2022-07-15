// $Id: link.h 1211 2009-01-17 00:45:28Z d3x593 $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _LINK_H
#define _LINK_H

#include "powerflow.h"

EXPORT int isa_link(OBJECT *obj, char *classname);
EXPORT SIMULATIONMODE interupdate_link(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
EXPORT int updatepowercalc_link(OBJECT *obj);
EXPORT int calculate_overlimit_link(OBJECT *obj, double *overload_value, bool *overloaded);
EXPORT int currentcalculation_link(OBJECT *obj, int nodecall, bool link_fault_mode);

#define impedance(X) (B_mat[X][X])

typedef enum {
		NORMAL=0,			///< defines just a normal link/transformer
		REGULATOR=1,		///< defines the link is a regulator
		DELTAGWYE=2,		///< defines the link is actually a Delta-Gwye transformer
		SPLITPHASE=3,		///< defines the link is a split-phase transformer
		SWITCH=4,			///< defines the link is a switch
		DELTADELTA=5,		///< defines the link is a delta-delta transformer - for power operations
		WYEWYE=6,			///< defines the link is a Wye-wye transformer, mainly to allow triggering for unity turns ratios
		VFD=7				///< defines the link is a VFD device (basically skip admittance)
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
	gld::complex a_mat[3][3];				// a_mat - 3x3 matrix, 'a' matrix
	gld::complex b_mat[3][3];				// b_mat - 3x3 matrix, 'b' matrix
	gld::complex c_mat[3][3];				// c_mat - 3x3 matrix, 'c' matrix
	gld::complex d_mat[3][3];				// d_mat - 3x3 matrix, 'd' matrix
	gld::complex A_mat[3][3];				// A_mat - 3x3 matrix, 'A' matrix
	gld::complex B_mat[3][3];				// B_mat - 3x3 matrix, 'B' matrix
	gld::complex tn[3];						// Used to calculate return current
	gld::complex base_admittance_mat[3][3];	// 3x3 matrix as "pre-inverted" matrix for NR - mostly for transformers
	gld::complex To_Y[3][3];					// To_Y  - 3x3 matrix, object transition to admittance
	gld::complex From_Y[3][3];				// From_Y - 3x3 matrix, object transition from admittance
	gld::complex *YSfrom;					// YSfrom - Pointer to 3x3 matrix representing admittance seen from "from" side (transformers)
	gld::complex *YSto;						// YSto - Pointer to 3x3 matrix representing admittance seen from "to" side (transformers)
	double voltage_ratio;				// voltage ratio (normally 1.0)
	int NR_branch_reference;			//Index of NR_branchdata this link is contained in
	SPECIAL_LINK SpecialLnk;			//Flag for exceptions to the normal handling
	set flow_direction;					// Flag direction of powerflow: 1 is normal, -1 is reverse flow, 0 is no flow
	void calculate_power();
	void calculate_power_splitphase();
	void set_flow_directions();
	int link_fault_on(OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time);		//Function to create fault on line
	int link_fault_off(int *implemented_fault, char *imp_fault_name);	//Function to remove fault from line
	int clear_fault_only(int *implemented_fault, char *imp_fault_name); //Function to remove the fault from the link object but not restore the system
	double mean_repair_time;
	double *link_limits[2][3];		/**< pointers for line limits (emergency vs. continuous) for link objects and by phase - pointered for variation */
	double link_rating[2][3];		/**< Values for current line rating - gives individual segments the ability to set */
	double *get_double(OBJECT *obj, const char *name);	/**< Gets address of double - mainly for mean_repair_time */
	bool overloaded_status;
public:
	enumeration status;	///< link status (open disconnect nodes)
	enumeration prev_status;	///< Previous link status (used for recalculation detection)

	bool current_accumulated;	///< Flag to indicate if NR current has been "handled" yet
	bool check_link_limits;	///< Flag to see if this particular link needs limits checked
	OBJECT *from;			///< from_node - source node
	OBJECT *to;				///< to_node - load node
	gld::complex current_in[3];		///< current flow to link (w.r.t from node)
	gld::complex current_out[3];	///< current flow out of link (w.r.t. to node)
	gld::complex read_I_in[3];	///< published current flow to link (w.r.t from node)
	gld::complex read_I_out[3];  ///< published current flow out of link (w.r.t to node)
	gld::complex If_in[3];		///< fault current flowing in
	gld::complex If_out[3];		///< fault current flowing out
	gld::complex Vf_out[3];
	gld::complex power_in;		///< power flow in (w.r.t from node)
	gld::complex power_out;		///< power flow out (w.r.t to node)
	gld::complex power_loss;		///< power losses
	gld::complex indiv_power_in[3];	///< power flow in (w.r.t. from node) - individual quantities
	gld::complex indiv_power_out[3];	///< power flow out (w.r.t. to node) - individual quantities
	gld::complex indiv_power_loss[3];///< power losses - individual quantities
	int protect_locations[3];	///< Links to protection object for different phase faults - part of reliability
	FUNCTIONADDR link_recalc_fxn;	///< Function address for link recalculation function - frequency dependence

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

	static int kmlinit(int (*stream)(const char*,...));
	int kmldump(int (*stream)(const char*,...));
	//Current injection calculation function - so it can be called remotely
	int CurrentCalculation(int nodecall, bool link_fault_mode);

	void NR_link_sync_fxn(void);
	void BOTH_link_postsync_fxn(void);
	bool perform_limit_checks(double *over_limit_value, bool *over_limits);
	double inrush_tol_value;	///< Tolerance value (of vdiff on the line ends) before "inrush convergence" is accepted
	INRUSHINTMETHOD inrush_int_method_inductance;	//Individual mode selection
	INRUSHINTMETHOD inrush_int_method_capacitance;

	//New matrix functions associated with transformer inrush (bigger)
	void lmatrix_add(gld::complex *matrix_in_A, gld::complex *matrix_in_B, gld::complex *matrix_out, int matsize);
	void lmatrix_mult(gld::complex *matrix_in_A, gld::complex *matrix_in_B, gld::complex *matrix_out, int matsize);
	void lmatrix_vmult(gld::complex *matrix_in, gld::complex *vector_in, gld::complex *vector_out, int matsize);

	// Fault current calculation functions
	void fault_current_calc(gld::complex C[7][7], unsigned int removed_phase, double fault_type); // function traces up from fault to swing bus summing up the link objects' impedances
											  // then calculates the fault current then passes that value back down to the faulted link objects.
	void mesh_fault_current_calc(gld::complex Zth[3][3],gld::complex CV[3][3],gld::complex CI[3][3],gld::complex *VSth,double fault_type);
	SIMULATIONMODE inter_deltaupdate_link(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

private:
	bool deltamode_inclusive;
	bool inrush_computations_needed;	///< Flag for in-rush computations to determine when an exit from deltamode would be allowed
	double inrush_vdiffmag_prev[3];		///< Tracking variable to determine when in-rush has run its course and inrush_computations_needed can be deflagged
	double deltamode_prev_time;	///< Tracking variable to tell when new deltamode timesteps have occurred (in-rush)

public:
	gld::complex *ahrlstore;			///< Pointer for array used to store line history constant ahrl -- associated with inductance
	gld::complex *bhrlstore;			///< Pointer for array used to store line history constant bhrl -- associated with inductance
	gld::complex *ahmstore;			///< Pointer for array used to store magnetic history constant ahtr -- associated with transformers
	gld::complex *bhmstore;			///< Pointer for array used to store magnetic history constant bhtr -- associated with transformers
	double *chrcstore;			///< Pointer for array used to store line history constant chrc -- associated with capacitance
	gld::complex *LinkCapShuntTerm;	///< Pointer for array used to store line history shunt capacitance term -- associated with getting currents out of the in-rush later
	gld::complex *LinkHistTermL;		///< Pointer for array used to store line history value for deltamode-based in-rush computations -- Inductive terms
	gld::complex *LinkHistTermCf;	///< Pointer for array used to store line history value for deltamode-based in-rush computations -- Shunt capacitance or transformer "from" terms
	gld::complex *LinkHistTermCt;	///< Pointer for array used to store line history value for deltamode-based in-rush computations -- Shunt capacitance or transformer "to" terms
	gld::complex *YBase_Full;		///< Pointer for array used to store "base admittance" for deltamode-based in-rush compuations -- Transformer in-rush
	gld::complex *YBase_Pri;			///< Pointer for array used to store "base primary admittance" for deltamode-based in-rush computations -- Transformer in-rush
	gld::complex *YBase_Sec;			///< Pointer for array used to store "base secondary admittance" for deltamode-based in-rush computations -- Transformer in-rush

	//******************* MOVE THESE? *******************************/
	//Saturation-based items -- probably need to be moved, but putting here since me=lazy
	double D_sat;
	gld::complex A_phi;
	gld::complex B_phi;
	gld::complex *hphi;	//History term for phi
	gld::complex *saturation_calculated_vals;

	//******************** Create a function from solver_nr to calculate Isat

};

//Macros
void inverse(gld::complex in[3][3], gld::complex out[3][3]);
void multiply(double a, gld::complex b[3][3], gld::complex c[3][3]);
void multiply(gld::complex a[3][3], gld::complex b[3][3], gld::complex c[3][3]);
void subtract(gld::complex a[3][3], gld::complex b[3][3], gld::complex c[3][3]);
void addition(gld::complex a[3][3], gld::complex b[3][3], gld::complex c[3][3]);
void equalm(gld::complex a[3][3], gld::complex b[3][3]);

//LU Decomp stuff
void lu_decomp(gld::complex *a, gld::complex *l, gld::complex *u, int size_val); //lu decomposition function for a generic square matrix
void forward_sub(gld::complex *l, gld::complex *b, gld::complex *z, int size_val); //backwards substitution  algorithm for a generic square system
void back_sub(gld::complex *u, gld::complex *z, gld::complex *x, int size_val); // forwards substitution algorithm for a generic square system
void lu_matrix_inverse(gld::complex *input_mat, gld::complex *output_mat, int size_val);	//matrix inversion calculated by LU decomp method
#endif // _LINK_H

