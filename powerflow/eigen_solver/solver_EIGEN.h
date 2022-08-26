// EIGEN_DLL.h

#pragma once

#ifndef CDECL
#define CDECL extern "C"
#endif

#ifdef WIN32
#ifndef EXPORT
/** Defines a function as exported to core **/
#define EXPORT CDECL __declspec(dllexport)
#endif
#else
#define EXPORT CDECL
#endif
#include <Eigen/Eigen>
#include <Eigen/SparseLU>
#include <Eigen/SuperLUSupport>

typedef struct NR_SOLVER_VARS {
    double *a_LU;
    double *rhs_LU;
    int *cols_LU;
    int *rows_LU;
} NR_SOLVER_VARS;


typedef struct EIGEN_STRUCT{
	int col_count;
	int row_count;
    Eigen::SuperLU<Eigen::SparseMatrix<double, Eigen::ColMajor>> *solver;
//    Eigen::SparseLU<Eigen::SparseMatrix<double, Eigen::ColMajor>> *solver;
//    Eigen::BiCGSTAB<Eigen::SparseMatrix<double>, Eigen::DiagonalPreconditioner<double>> *solver;
//    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> * solver;
    bool initialSetup;
    bool admittance_change;
    int tracker;
    std::vector<int> cols_index;
	std::vector<Eigen::Triplet<double>> *tripletList;
} EIGEN_STRUCT;


//Initialization function
EXPORT void *LU_init(void *ext_array);

// Allocation function
EXPORT void LU_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change);

// Solver function
EXPORT int LU_solve(void *ext_array, NR_SOLVER_VARS *system_info_vars, unsigned int rowcount, unsigned int colcount);

// Destructive function
EXPORT void LU_destroy(void *ext_array, bool new_iteration);

