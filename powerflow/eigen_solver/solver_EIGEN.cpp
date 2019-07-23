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
#include "solver_EIGEN.h"
#include <iostream>

using namespace Eigen;

typedef struct NR_SOLVER_VARS NR_SOLVER_VARS;

// Initialization function
// Sets Common property (options)
void *LU_init(void *ext_array)
{
	EIGEN_STRUCT *EigenValues;
	EigenValues = (EIGEN_STRUCT*)malloc(sizeof(EIGEN_STRUCT));
	EigenValues->col_count = 0;
	EigenValues->row_count = 0;
	EigenValues->initialSetup = true;
	ext_array = (void *)(EigenValues);
	return ext_array;
}

// Allocation function
void LU_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change)
{
    if (admittance_change){
        static_cast<EIGEN_STRUCT*>(ext_array)->col_count = rowcount;
        static_cast<EIGEN_STRUCT*>(ext_array)->row_count = colcount;
//        std::cout << "rows: " << rowcount << std::endl;
//        std::cout << "cols: " << colcount << std::endl;
        static_cast<EIGEN_STRUCT*>(ext_array)->mat = SparseMatrix<double> (rowcount, colcount);
    }
    static_cast<EIGEN_STRUCT*>(ext_array)->admittance_change = admittance_change;
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
    const int rows = static_cast<EIGEN_STRUCT*>(ext_array)->row_count;
    const int cols = static_cast<EIGEN_STRUCT*>(ext_array)->col_count;

//    std::cout << "rows: " << rows << std::endl;
//    std::cout << "cols: " << cols << std::endl;

    // catch garbage being thrown in here.
    // TODO: Review if there is ever a reason to have 0 rows or 0 columns
    if (rows == 0 || cols == 0){
        std::cout << "0 rows or columns matrix detected" << std::endl;
        return -1;
    }

    if ( static_cast<EIGEN_STRUCT*>(ext_array)->admittance_change ){ //TODO: check if matrix changes throughout tests
        std::vector<T> tripletList;
        tripletList.reserve(cols * rows);

        // populate the matrix
        double *values = system_info_vars->a_LU;
        int *rows_index = system_info_vars->rows_LU;
        int *cols_index = system_info_vars->cols_LU;
        int col_ref = 0;

        for (int row_ref = 0; row_ref <= rows; row_ref++){
            while(col_ref < cols_index[row_ref + 1]){
                tripletList.push_back(T(row_ref, rows_index[col_ref], values[col_ref]));
                col_ref++;
            }
        }

//        std::cout << "COLS: " << cols << std::endl;
//        std::cout << "ROWS: " << rows << std::endl;
//        for (int i = 0; i < (144); i ++){
//            std::cout << "row: " << tripletList[i].row() << " col: " << tripletList[i].col() << " val: " << tripletList[i].value() <<  std::endl;
//        }
        static_cast<EIGEN_STRUCT*>(ext_array)->mat.setFromTriplets(tripletList.begin(), tripletList.end());
        static_cast<EIGEN_STRUCT*>(ext_array)->initialSetup = false;
    }
    SpMat mat = static_cast<EIGEN_STRUCT*>(ext_array)->mat;

    // set b
    VectorXd b (cols);
    for (int i = 0; i < cols; i++){
        b[i] = system_info_vars->rhs_LU[i];
//        std::cout << "b " << i << ": " << b[i] << std::endl;
    }

//    SimplicialCholesky<SpMat> chol(mat);
//    VectorXd solution = chol.solve(b);

    VectorXd solution;

    SparseLU<SparseMatrix<double> > solver;
//    Eigen::SuperLU<SparseMatrix<double> > solver;
    if (static_cast<EIGEN_STRUCT*>(ext_array)->admittance_change){
        solver.analyzePattern(mat);
    }
//    solver.compute(mat);
    solver.factorize(mat);
    if(solver.info()!=0){
        return solver.info();
    }
    solution = solver.solve(b);

    //set the solution
    for (int i = 0; i < rows; i++){
//        std::cout << "solution " << i << ": " << solution[i] << std::endl;
        system_info_vars->rhs_LU[i] = solution[i];
    }

	return solver.info();
}

// Destruction function
// Frees up numeric array
// New iteration isn't needed here - numeric gets redone EVERY iteration, so we don't care if we were successful or not
void LU_destroy(void *ext_array, bool new_iteration)
{
//    free(static_cast<EIGEN_STRUCT*>(ext_array)->mat);

    /*
    // Recasting variable
    KLU_STRUCT *KLUValues;

    // Link the structure up
    KLUValues = (KLU_STRUCT*)ext_array;

    // KLU destructive commands
    klu_free_numeric(&(KLUValues->NumericVal),KLUValues->CommonVal);
    */
}
