/* $Id
 * Newton-Raphson solver
 */

#ifndef _SOLVER_NR
#define _SOLVER_NR

#include "complex.h"

typedef struct  {
	int type;				///< bus type (0=PQ, 1=PV, 2=SWING)
	unsigned char phases;	///< Phases property - used for construction of matrices (skip bad entries)
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
} BUSDATA;

typedef struct {
	complex *Yfrom;			///< branch admittance of from side of link
	complex *Yto;			///< branch admittance of to side of link
	complex *YSfrom;		///< self admittance seen on from side
	complex *YSto;			///< self admittance seen on to side
	unsigned char phases;	///< Phases property - used for construction of matrices
	int from;				///< index into bus data
	int to;					///< index into bus data
	bool *status;			///< status of the object, if it is a switch (restoration module usage)
	double v_ratio;			///< voltage ratio (v_from/v_to)
	char *name;				///< original name
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


int64 solver_nr(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, bool *bad_computations);

#endif
