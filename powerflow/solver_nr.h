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
	complex *extra_var;		///< Extra variable - used mainly for current12 in triplex and "differently-connected" children
	complex *house_var;		///< Extra variable - used mainly for nominal house current 
	int *Link_Table;		///< table of links that connect to us (for population purposes)
	unsigned int Link_Table_Size;	///< Number of entries in the link table (number of links connected to us)
	double PL[3];			///< real power component of total bus load
	double QL[3];			///< reactive power component of total bus load
	double PG[3];			///< real power generation at generator bus
	double QG[3];			///< reactive power generation at generator bus
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
	bool *status;			///< status of the object, if it is a switch (restoration module usage)
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

//Function prototypes for external solver interface
//void *ext_solver_init(void *ext_array);
//void ext_solver_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change);
//int ext_solver_solve(void *ext_array, NR_SOLVER_VARS *system_info_vars, unsigned int rowcount, unsigned int colcount);
//void ext_solver_destroy(void *ext_array, bool new_iteration);

int64 solver_nr(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, bool *bad_computations);

#endif
