/* $Id
 * Newton-Raphson solver
 */

#ifndef _SOLVER_NR
#define _SOLVER_NR

#include "gld_complex.h"
#include "object.h"

typedef struct  {
	int type;				///< bus type (0=PQ, 1=PV, 2=SWING, 3=SWING_PQ)
	unsigned char phases;	///< Phases property - used for construction of matrices (skip bad entries) - [Split Phase | House present | To side of SPCT | Diff Phase Child | D | A | B | C]
	unsigned char origphases;	///< Original phases property - follows same format - used to save what object's original capabilities
	set *busflag;			///< Pointer to busflags property - mainly used for reliability checks
	gld::complex *V;				///< bus voltage
	gld::complex *S;				///< constant power
	gld::complex *Y;				///< constant admittance (impedance loads)
	gld::complex *I;				///< constant current
	gld::complex *prerot_I;		///< pre-rotated current (deltamode)
	gld::complex *S_dy;			///< constant power -- explicit delta/wye values
	gld::complex *Y_dy;			///< constant admittance -- explicit delta/wye values
	gld::complex *I_dy;			///< constant current -- explicit delta/wye values
	gld::complex *full_Y;		///< constant admittance - full 3x3 table (used by fixed dynamic devices) - set to NULL for devices that don't matter
	gld::complex *full_Y_all;	///< Admittance - self admittance value for "static" portions - used by dynamic loads
	gld::complex *full_Y_load;	///< Admittance - 3-element diagonal table (used by in-rush-capable loads, right now) - set to NULL for devices that don't matter
	gld::complex *extra_var;		///< Extra variable - used mainly for current12 in triplex and "differently-connected" children
	gld::complex *house_var;		///< Extra variable - used mainly for nominal house current
	int *Link_Table;		///< table of links that connect to us (for population purposes)
	unsigned int Link_Table_Size;	///< Number of entries in the link table (number of links connected to us)
	double PL[3];			///< real power component of total bus load
	double QL[3];			///< reactive power component of total bus load
	bool *dynamics_enabled;	///< Flag indicating this particular node has a dynamics contribution function
	bool swing_functions_enabled;	///< Flag indicating if this particular node is a swing node, and if so, if it is behaving "all swingy"
	bool swing_topology_entry;		///< Flag to indicate this bus was the source entry point, even if it isn't a swing anymore (SWING_PQ generator stuff)
	gld::complex *PGenTotal;		///< Total output of any generation at this node - lumped for now for dynamics
	gld::complex *DynCurrent;	///< Dynamics current portions - used as storage/tracking for generator dynamics
	gld::complex *BusHistTerm;	///< History term pointer for in-rush-based calculations
	gld::complex *BusSatTerm;	///< Saturation term pointer for in-rush-based transformer calculations - separate for ease
	double volt_base;		///< voltage basis
    double mva_base;		/// MVA basis
	double Jacob_A[3];		// Element a in equation (37), which is used to update the Jacobian matrix at each iteration
	double Jacob_B[3];		// Element b in equation (38), which is used to update the Jacobian matrix at each iteration
	double Jacob_C[3];		// Element c in equation (39), which is used to update the Jacobian matrix at each iteration
	double Jacob_D[3];		// Element d in equation (40), which is used to update the Jacobian matrix at each iteration
	gld::complex FPI_current[3];	// Current for  FPI RHS
	unsigned int Matrix_Loc;// Starting index of this object's place in all matrices/equations
	double max_volt_error;	///< Maximum voltage error specified for that node
	char *name;				///< original name
	OBJECT *obj;			///< Link to original object header
	FUNCTIONADDR ExtraCurrentInjFunc;	///< Link to extra functions of current injection updates -- mostly VSI current updates
	OBJECT *ExtraCurrentInjFuncObject;	///< Link to the object that mapped the current injection function - needed for function calls
	FUNCTIONADDR LoadUpdateFxn;			///< Link to load update function for load objects -- for impedance conversion (inrush or forced)
	FUNCTIONADDR ShuntUpdateFxn;		///< Link to node shunt update function - for FPI - fixes sequence issue in deltamode
	int island_number;		///< Numerical designation for which island this bus belongs to
} BUSDATA;

typedef struct {
	gld::complex *Yfrom;			///< branch admittance of from side of link
	gld::complex *Yto;			///< branch admittance of to side of link
	gld::complex *YSfrom;		///< self admittance seen on from side
	gld::complex *YSto;			///< self admittance seen on to side
	unsigned char phases;	///< Phases property - used for construction of matrices
	unsigned char origphases;	///< Original phases property - follows same format - used to save what object's original capabilities
	unsigned char faultphases;	///< Flags for induced faults - used to prevent restoration of objects that should otherwise still be broken
	int from;				///< index into bus data
	int to;					///< index into bus data
	int fault_link_below;    ///< index indicating next faulted link object below the current link object
	enumeration *status;	///< status of the object, if it is a switch (restoration module usage)
	unsigned char lnk_type;	///< type of link the object is - 0 = UG/OH line, 1 = Triplex line, 2 = transformer, 3 = fuse, 4 = switch, , 5 = sectionalizer, 6 = recloser
	double v_ratio;			///< voltage ratio (v_from/v_to)
	char *name;				///< original name
	OBJECT *obj;			///< Link to original object header
	gld::complex *If_from;		///< 3 phase fault currents on the from side
	gld::complex *If_to;			///< 3 phase fault currents on the to side
	FUNCTIONADDR limit_check;	////< Link to overload checking function (calculate_overlimit_link) -- restoration related
	FUNCTIONADDR ExtraDeltaModeFunc;	///< Link to extra functions of deltamode -- notably, transformer saturation
	int island_number;		///< Numerical designation for which island this branch belongs to
} BRANCHDATA;

typedef struct Y_NR{
	int row_ind;  ///< row location of the element in 6n*6n Y matrix in NR solver
	int	col_ind;  ///< column location of the element in 6n*6n Y matrix in NR solver
    double Y_value; ///< value of the element in 6n*6n Y matrix in NR solver
} Y_NR;

typedef struct {
	int row_ind;  ///< row location of the element in n*n bus admittance matrix in NR solver
	int	col_ind;  ///< column location of the element in n*n bus admittance matrix in NR solver
    gld::complex Y[3][3]; ///< complex value of elements in bus admittance matrix in NR solver
	gld::complex Yload[3][3];	///< complex value of elements in load portion of bus admittance matrix
	char size;		///< size of the admittance diagonal - assumed square, useful for smaller size
} Bus_admit;

typedef struct {
	double *a_LU;
	double *rhs_LU;
	int *cols_LU;
	int *rows_LU;
} NR_SOLVER_VARS;

typedef enum {
	PF_NORMAL=0,	///< Standard SWING-based static powerflow
	PF_DYNINIT=1,	///< Modified static powerflow, for initial dynamics mode (SWING manipulations)
	PF_DYNCALC=2	///< Modified powerflow, for dynamics mode after initial powerflow
} NRSOLVERMODE;

// Sparse element
typedef struct SP_E {
	struct SP_E* next;
	int row_ind;  ///< row location of the element
    double value; ///< value of the element
} SP_E;

// Sparse matrix
typedef struct {
	SP_E** cols;
	SP_E* llheap;
	unsigned int llptr;
	unsigned int ncols;
} SPARSE;

typedef struct {
	double *current_RHS_NR;					/// Storage array for current injection/RHS materials
	unsigned int size_offdiag_PQ;		/// Number of fixed off-diagonal matrix elements
	unsigned int size_diag_fixed;		/// Number of fixed diagonal matrix elements
	unsigned int total_variables;		/// Total number of phases to be calculating (size of matrices)
	unsigned int size_diag_update;		/// Number of "update on the diagonal" matrix elements
	unsigned int size_Amatrix;			/// Size of the A matrix variable
	unsigned int max_size_offdiag_PQ;	///Maximum allocated space for off-diagonal portion
	unsigned int max_size_diag_fixed;	///Maximum allocated space for fixed portion of diagonal
	unsigned int max_total_variables;	///Maximum allocated space for "whole solution" variables - e.g., Y_Amatrix
	unsigned int max_size_diag_update;	///Maximum allocated space for updating portion of diagonal
	unsigned int prev_m;				///Track size of matrix put into superLU form - may not need a realloc, but needs to be updated
	unsigned int index_count;			///Temporary variable for figuring out sizes -- put inside each island for size tracking
	unsigned int bus_count;				///Variable to keep track of number of buses in this island -- since it is needed
	unsigned int indexer;				///Temporary variable for indexing arrays
	bool NR_realloc_needed;				///flag to indicate a matrix reallocation is required
	Y_NR *Y_offdiag_PQ;					///Y_offdiag_PQ store the row,column and value of off_diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
	Y_NR *Y_diag_fixed;					///Y_diag_fixed store the row,column and value of fixed diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
	Y_NR *Y_diag_update;				///Y_diag_update store the row,column and value of updated diagonal elements of 6n*6n Y_NR matrix at each iteration. No PV bus is included.
	SPARSE *Y_Amatrix;					///Y_Amatrix store all the elements of Amatrix in equation AX=B;
	NR_SOLVER_VARS matrices_LU;			///Matrices structure for LU solver - superLU, by default
	void *LU_solver_vars;				///Pointer to the LU routine variables for each island
	int64 iteration_count;				///Iteration count for this particular solver system
	bool new_iteration_required;		///Flag to indicate if a new iteration is required
	bool swing_converged;				///Flag to indicate if the swing imbalance has been resolved -- deltamode-oriented
	bool swing_is_a_swing;				///Flag to indicate if swing buses should still be in that mode or not -- deltamode-oriented
	bool SaturationMismatchPresent;		///Flag to indicate if any saturation-based calculations haven't coded
	bool NortonCurrentMismatchPresent;	///Flag to indicate if any Norton-equivalent currents are changing in magnitude too much
	int solver_info;					///Status return value for LU solver -- put into the array for tracking
	int64 return_code;					///Specific return value - just to replicate previous functionality
	double max_mismatch_converge;		///Current difference for convergence checks
} NR_MATRIX_CONSTRUCTION;

typedef struct {
	NR_MATRIX_CONSTRUCTION *island_matrix_values;	///Structure pointer to the individual matrix element "population" portions
	Bus_admit *BA_diag;					/// BA_diag store the diagonal elements of the bus admittance matrix, the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
} NR_SOLVER_STRUCT;

//Mesh-fault-related structure - passing information
typedef struct {
	gld::complex *z_matrix;			/// Matrix for the impedance of that value
	int NodeRefNum;				/// Reference node for the node to calculate the impedance at
	int return_code;			/// Special return codes for impedance check -- 0 = non-descript failure, 1 = success, 2 = unsupported solver
} NR_MESHFAULT_IMPEDANCE;

//Function prototypes for external solver interface
//void *ext_solver_init(void *ext_array);
//void ext_solver_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change);
//int ext_solver_solve(void *ext_array, NR_SOLVER_VARS *system_info_vars, unsigned int rowcount, unsigned int colcount);
//void ext_solver_destroy(void *ext_array, bool new_iteration);

int64 solver_nr(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, NR_SOLVER_STRUCT *powerflow_values, NRSOLVERMODE powerflow_type , NR_MESHFAULT_IMPEDANCE *mesh_imped_vals, bool *bad_computations);
void compute_load_values(unsigned int bus_count, BUSDATA *bus, NR_SOLVER_STRUCT *powerflow_values, bool jacobian_pass, int island_number);
void NR_admittance_update(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, NR_SOLVER_STRUCT *powerflow_values, NRSOLVERMODE powerflow_type);

//Newton-Raphson solver array handlers
STATUS NR_array_structure_free(NR_SOLVER_STRUCT *struct_of_interest,int number_of_islands);		/* Handles freeing NR_SOLVER_STRUCT arrays */
STATUS NR_array_structure_allocate(NR_SOLVER_STRUCT *struct_of_interest,int number_of_islands);	/* Allocates NR_SOLVER_STRUCT item for a given number of entries/islands */

#endif
