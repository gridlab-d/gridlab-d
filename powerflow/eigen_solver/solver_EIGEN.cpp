//TODO: Update commented section

// KLU DLL Export - Wrapper for KLU in GridLAB-D
//
// Copyright (c) 2011, Battelle Memorial Institute
// All Rights Reserved
//
// Author	Frank Tuffner, Pacific Northwest National Laboratory
// Created	May 2011
//
// Pacific Northwest National Laboratory is operated by Battelle 
// Memorial Institute for the US Department of Energy under 
// Contract No. DE-AC05-76RL01830.
//
//
// LICENSE
//
//   This software is released under the Lesser General Public License
//   (LGPL) Version 2.1.  You can read the full text of the LGPL
//   license at http://www.gnu.org/licenses/lgpl-2.1.html.
//
//
// US GOVERNMENT RIGHTS
//
//   The Software was produced by Battelle under Contract No. 
//   DE-AC05-76RL01830 with the Department of Energy.  The U.S. 
//   Government is granted for itself and others acting on its 
//   behalf a nonexclusive, paid-up, irrevocable worldwide license 
//   in this data to reproduce, prepare derivative works, distribute 
//   copies to the public, perform publicly and display publicly, 
//   and to permit others to do so.  The specific term of the license 
//   can be identified by inquiry made to Battelle or DOE.  Neither 
//   the United States nor the United States Department of Energy, 
//   nor any of their employees, makes any warranty, express or implied, 
//   or assumes any legal liability or responsibility for the accuracy, 
//   completeness or usefulness of any data, apparatus, product or 
//   process disclosed, or represents that its use would not infringe 
//   privately owned rights.  
//
//
// DESCRIPTION
//
//   This code was implemented to facilitate the interface between
//   GridLAB-D and KLU. For usage please consult the GridLAB-D wiki at  
//   http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Powerflow_External_LU_Solver_Interface.
//
//   As a general rule, KLU should run about 30% faster than the standard
//   SuperLU solver embedded in GridLAB-D because it uses a method that 
//   assume sparsity and handles it better.  For details, see the
//   documentation of KLU itself.
//
//
// INSTALLATION
//
//   After building the release version, on 32-bit machines copy 
//   solver_klu_win32.dll to solver_klu.dll in the GridLAB-D folder that 
//   contains powerflow.dll.  On 64-bit machines copy solver_klu_x64.dll 
//   to solver_klu.dll in the folder that contains powerflow.dll.
// 

#include <Eigen/SparseLU>
#include <Eigen/Eigen>
#include <Eigen/Sparse>
#include <Eigen/SuperLUSupport>
#include <iostream>
#include <math.h>

#include "solver_EIGEN.h"

using namespace Eigen;

typedef struct NR_SOLVER_VARS NR_SOLVER_VARS;

// Initialization function
// Sets Common property (options)
void *LU_init(void *ext_array)
{
    EIGEN_STRUCT *EigenValues;
    if (ext_array == NULL){
        EigenValues = (EIGEN_STRUCT*)malloc(sizeof(EIGEN_STRUCT));
        EigenValues->col_count = 0;
        EigenValues->row_count = 0;
        EigenValues->initialSetup = true;
        EigenValues->tracker = 1;
        ext_array = (void *)(EigenValues);
    }else{
        EigenValues = static_cast<EIGEN_STRUCT*>(ext_array);
    }

    return ext_array;
}

// Allocation function
void LU_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change)
{
    EIGEN_STRUCT *EigenValues;
    EigenValues = static_cast<EIGEN_STRUCT*>(ext_array);
    EigenValues->col_count = rowcount;
    EigenValues->row_count = colcount;
    EigenValues->admittance_change = admittance_change;
//    EigenValues->solver = new SimplicialLDLT<SparseMatrix<double>>;
//    EigenValues->solver = new SparseLU<SparseMatrix<double, ColMajor>>;
    EigenValues->solver = new SuperLU<SparseMatrix<double, ColMajor>>;
//    EigenValues->solver = new BiCGSTAB<SparseMatrix<double, ColMajor>>;
}

// Solution function
// allocates an initial symbolic array and changes it as necessary
// allocates numeric array each pass
// Performs the analysis portion and outputs result
// Performs the analysis portion and outputs result
int LU_solve(void *ext_array, NR_SOLVER_VARS *system_info_vars, unsigned int rowcount, unsigned int colcount)
{
    typedef SparseMatrix<double> SpMat;
    typedef Triplet<double> T;
    EIGEN_STRUCT *EigenValues;
    EigenValues = static_cast<EIGEN_STRUCT*>(ext_array);
    const int rows = rowcount;
    const int cols = rowcount;
    SpMat mat;
    mat = SparseMatrix<double>(rows, cols);
    std::vector<T> tripletList;
    tripletList.reserve(cols * rows);

    // announce tracker
    std::cout << "Iteration#: " << EigenValues->tracker << std::endl;

    // populate the list of triplets
    double *values = system_info_vars->a_LU;
    double value;
    int *rows_index = system_info_vars->rows_LU;
    int *cols_index = system_info_vars->cols_LU;
    int col_ref = 0;
    for (int row_ref = 0; row_ref <= rows; row_ref++){
        while(col_ref < cols_index[row_ref + 1]){
            value = values[col_ref];
            // check for NaN's
            if (isnan(value)) {
                std::cout << "NaN detected in matrix A value, returning with -1" << std::endl;
                return -1;
            }
            tripletList.push_back(T(row_ref, rows_index[col_ref], value));
            col_ref++;
        }
    }

    // populate matrix with triplets
    mat.setFromTriplets(tripletList.begin(), tripletList.end());

    // save tripleList in ext_array for reference
    EigenValues->tripletList = &tripletList;

    // do things here you only want done the first time, after mat is populated
    if(EigenValues->initialSetup){
        EigenValues->solver->analyzePattern(mat);
        EigenValues->initialSetup = false;
    }

    // factorize the matrix
    EigenValues->solver->factorize(mat);

    // set b matrix values
    VectorXd b (rows);
    for (int i = 0; i < cols; i++){
        // check for NaN's
        if (isnan(system_info_vars->rhs_LU[i])){
            std::cout << "NaN detected in matrix b value, returning with -1" << std::endl;
//            return -1;
        }
        b[i] = system_info_vars->rhs_LU[i];
    }

    // initialize solution
    VectorXd solution (rows);

    // run the solver
    solution = EigenValues->solver->solve(b);

    // iterative solver stuff
//    BiCGSTAB<SparseMatrix<double>> solver;
//    solver.compute(mat);
//    solver.setMaxIterations(20);
//    solution = solver.solve(b);


    // catch errors with solver -> solve
    if(EigenValues->solver->info()!=0){
        std::cout << "issue with solver: " << EigenValues->solver->info() << std::endl;
//        return -1;
    }

    //set the solution in system_info_vars
    for (int i = 0; i < rows; i++){
        // check for NaN's
        if (isnan(solution[i] || (EigenValues->tracker > 1))){
            std::cout << "NaN detected in matrix x value, returning with -1" << std::endl;
//            return -1;
        }
        system_info_vars->rhs_LU[i] = solution[i];
    }

    // increment iteration tracker and return!
    EigenValues->tracker += 1;
    return 0;
}

// Destruction function
// Frees up numeric array
// New iteration isn't needed here - numeric gets redone EVERY iteration, so we don't care if we were successful or not
void LU_destroy(void *ext_array, bool new_iteration)
{
    EIGEN_STRUCT *EigenValues;
    EigenValues = static_cast<EIGEN_STRUCT*>(ext_array);

//    free(EigenValues->solver);
    /*
    // Recasting variable
    KLU_STRUCT *KLUValues;

    // Link the structure up
    KLUValues = (KLU_STRUCT*)ext_array;

    // KLU destructive commands
    klu_free_numeric(&(KLUValues->NumericVal),KLUValues->CommonVal);
    */
}