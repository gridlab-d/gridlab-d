/* $Id
 * Newton-Raphson solver
 */

#ifndef _SOLVER_NR
#define _SOLVER_NR

#include "complex.h"
#include "object.h"

typedef struct  {
	int type;				///< bus type (0=PQ, 1=PV, 2=SWING)
	unsigned char phases;	///< Phases property - used for construction of matrices (skip bad entries) - [Split Phase | House present | To side of SPCT | Diff Phase Child | D | A | B | C]
	unsigned char origphases;	///< Original phases property - follows same format - used to save what object's original capabilities
	complex *V;				///< bus voltage
	complex *S;				///< constant power
	complex *Y;				///< constant admittance (impedance loads)
	complex *I;				///< constant current
	complex *S_dy;			///< constant power -- explicit delta/wye values
	complex *Y_dy;			///< constant admittance -- explicit delta/wye values
	complex *I_dy;			///< constant current -- explicit delta/wye values
	complex *full_Y;		///< constant admittance - full 3x3 table (used by dynamic loads) - set to NULL for loads that don't matter
	complex *full_Y_all;	///< Admittance - self admittance value for "static" portions - used by dynamic loads
	complex *extra_var;		///< Extra variable - used mainly for current12 in triplex and "differently-connected" children
	complex *house_var;		///< Extra variable - used mainly for nominal house current 
	int *Link_Table;		///< table of links that connect to us (for population purposes)
	unsigned int Link_Table_Size;	///< Number of entries in the link table (number of links connected to us)
	double PL[3];			///< real power component of total bus load
	double QL[3];			///< reactive power component of total bus load
	bool *dynamics_enabled;	///< Flag indicating this particular node has a dynamics contribution function
	complex *PGenTotal;		///< Total output of any generation at this node - lumped for now for dynamics
	complex *DynCurrent;	///< Dynamics current portions - used as storage/tracking for generator dynamics
	double kv_base;			///< kV basis
    double mva_base;		/// MVA basis
	double Jacob_A[3];		// Element a in equation (37), which is used to update the Jacobian matrix at each iteration
	double Jacob_B[3];		// Element b in equation (38), which is used to update the Jacobian matrix at each iteration
	double Jacob_C[3];		// Element c in equation (39), which is used to update the Jacobian matrix at each iteration
	double Jacob_D[3];		// Element d in equation (40), which is used to update the Jacobian matrix at each iteration
	unsigned int Matrix_Loc;// Starting index of this object's place in all matrices/equations
	double max_volt_error;	///< Maximum voltage error specified for that node
	char *name;				///< original name
	OBJECT *obj;			///< Link to original object header
	int Parent_Node;					///< index to parent node in BUSDATA structure - restoration related - may not be valid for meshed systems or reverse flow (restoration usage)
	int *Child_Nodes;					///< index to child nodes in BUSDATA structure - restoration related - may not be valid for meshed systems or reverse flow (restoration usage)
	unsigned int Child_Node_idx;		///< indexing variable to know location of next valid Child_Nodes entry
} BUSDATA;

typedef struct {
	complex *Yfrom;			///< branch admittance of from side of link
	complex *Yto;			///< branch admittance of to side of link
	complex *YSfrom;		///< self admittance seen on from side
	complex *YSto;			///< self admittance seen on to side
	unsigned char phases;	///< Phases property - used for construction of matrices
	unsigned char origphases;	///< Original phases property - follows same format - used to save what object's original capabilities
	unsigned char faultphases;	///< Flags for induced faults - used to prevent restoration of objects that should otherwise still be broken
	int from;				///< index into bus data
	int to;					///< index into bus data
	int fault_link_below;    ///< index indicating next faulted link object below the current link object
	enumeration *status;	///< status of the object, if it is a switch (restoration module usage)
	unsigned char lnk_type;	///< type of link the object is - 0 = UG/OH line, 1 = Triplex line, 2 = switch, 3 = fuse, 4 = transformer, 5 = sectionalizer, 6 = recloser
	double v_ratio;			///< voltage ratio (v_from/v_to)
	char *name;				///< original name
	OBJECT *obj;			///< Link to original object header
	complex *If_from;		///< 3 phase fault currents on the from side
	complex *If_to;			///< 3 phase fault currents on the to side 
} BRANCHDATA;

typedef struct Y_NR{
	int row_ind;  ///< row location of the element in 6n*6n Y matrix in NR solver
	int	col_ind;  ///< collumn location of the element in 6n*6n Y matrix in NR solver
    double Y_value; ///< value of the element in 6n*6n Y matrix in NR solver
} Y_NR;

typedef struct {
	int row_ind;  ///< row location of the element in n*n bus admittance matrix in NR solver
	int	col_ind;  ///< collumn location of the element in n*n bus admittance matrix in NR solver
    complex Y[3][3]; ///< complex value of elements in bus admittance matrix in NR solver
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

typedef struct {
	double *deltaI_NR;					/// Storage array for current injection
	unsigned int size_offdiag_PQ;		/// Number of fixed off-diagonal matrix elements
	unsigned int size_diag_fixed;		/// Number of fixed diagonal matrix elements
	unsigned int total_variables;		///Total number of phases to be calculating (size of matrices)
	unsigned int max_size_offdiag_PQ;	///Maximum allocated space for off-diagonal portion
	unsigned int max_size_diag_fixed;	///Maximum allocated space for fixed portion of diagonal
	unsigned int max_total_variables;	///Maximum allocated space for "whole solution" variables - e.g., Y_Amatrix
	unsigned int max_size_diag_update;	///Maximum allocated space for updating portion of diagonal
	unsigned int prev_m;				///Track size of matrix put into superLU form - may not need a realloc, but needs to be updated
	bool NR_realloc_needed;				///flag to indicate a matrix reallocation is required
	Bus_admit *BA_diag;					/// BA_diag store the diagonal elements of the bus admittance matrix, the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
	Y_NR *Y_offdiag_PQ;					///Y_offdiag_PQ store the row,column and value of off_diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
	Y_NR *Y_diag_fixed;					///Y_diag_fixed store the row,column and value of fixed diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
	Y_NR *Y_diag_update;				///Y_diag_update store the row,column and value of updated diagonal elements of 6n*6n Y_NR matrix at each iteration. No PV bus is included.
	Y_NR *Y_Amatrix;					///Y_Amatrix store all the elements of Amatrix in equation AX=B;
	Y_NR *Y_Work_Amatrix;				///Temporary storage version of Y_Amatrix
} NR_SOLVER_STRUCT;

//Function prototypes for external solver interface
//void *ext_solver_init(void *ext_array);
//void ext_solver_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change);
//int ext_solver_solve(void *ext_array, NR_SOLVER_VARS *system_info_vars, unsigned int rowcount, unsigned int colcount);
//void ext_solver_destroy(void *ext_array, bool new_iteration);

int64 solver_nr(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, NR_SOLVER_STRUCT *powerflow_values, NRSOLVERMODE powerflow_type ,bool *bad_computations);

#endif
