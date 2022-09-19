/* $Id
 * Newton-Raphson solver
 */

// ***** Random Useful Code to Note ****
// This code used to be right under the recasting of sol_LU
// Matrix multiplication using superLU-included stuff
// Contents of xvecttest obviously missing
//
//// Try the multiply - X_LU*xvecttest = yvecttest
//// Random attempts - see if we can perform an "Ax+y=y" action
//double *xvecttest, *yvecttest;
//
//// Allocate them
//xvecttest = (double*)gl_malloc(m*sizeof(double));
//yvecttest = (double*)gl_malloc(m*sizeof(double));
//
//sp_dgemv("n",1.0,&A_LU,xvecttest,1,0.0,yvecttest,1);
//// Free, to be thorough
//gl_free(xvecttest);
//gl_free(yvecttest);
// ***** End Random Useful Code to Note ****

/*
***********************************************************************
Generic in-rush notes here 
--------------------------
What about P/C relationships for history terms, how handle?
Initialization after returning to service?

***********************************************************************
*/
#include <cmath>
#ifndef GLD_USE_EIGEN
#include "solver_nr.h"
#else
#include "solver_nr_eigen.h"
#endif

/* access to module global variables */
#include "powerflow.h"

#define MT // this enables multithreaded SuperLU

#ifdef MT
#include <slu_mt_ddefs.h>	//superLU_MT
#else
#include <slu_ddefs.h>	//Sequential superLU (other platforms)
#endif

namespace gld {
    template<typename Tv, typename Te>
    constexpr Tv pow(Tv value, Te exp) {
        switch (exp) {
            case 2:
                return value * value;
                break;
            case 3:
                return value * value * value;
                break;
            case 4:
                return value * value * value * value;
                break;
            default:
                return std::pow(value, exp);
                break;
        }
    }
}

//SuperLU variable structure
//These are the working variables, but structured for island implementation
typedef struct {
	int *perm_c;
	int *perm_r;
	SuperMatrix A_LU;
	SuperMatrix B_LU;
} SUPERLU_NR_vars;

//Initialize the sparse notation
void sparse_init(SPARSE* sm, int nels, int ncols)
{
	int indexval;

	//Allocate the column pointer GLD heap
	sm->cols = (SP_E**)gl_malloc(ncols*sizeof(SP_E*));

	//Check it
	if (sm->cols == nullptr)
	{
		GL_THROW("NR: Sparse matrix allocation failed");
		/*  TROUBLESHOOT
		While attempting to allocate space for one of the sparse matrix variables, an error was encountered.
		Please try again.  If the error persists, please submit your code and a bug report via the ticketing system.
		*/
	}
	else	//Zero it, for giggles
	{
		for (indexval=0; indexval<ncols; indexval++)
		{
			sm->cols[indexval] = nullptr;
		}
	}

	//Allocate the elements on the GLD heap
	sm->llheap = (SP_E*)gl_malloc(nels*sizeof(SP_E));

	//Check it
	if (sm->llheap == nullptr)
	{
		GL_THROW("NR: Sparse matrix allocation failed");
		//Defined above
	}
	else	//Zero it, for giggles
	{
		for (indexval=0; indexval<nels; indexval++)
		{
			sm->llheap[indexval].next = nullptr;
			sm->llheap[indexval].row_ind = -1;	//Invalid, so it gets upset if we use this (intentional)
			sm->llheap[indexval].value = 0.0;
		}
	}

	//Init others
	sm->llptr = 0;
	sm->ncols = ncols;
}

//Free up/clear the sparse allocations
void sparse_clear(SPARSE* sm)
{
	//Clear them up
	gl_free(sm->llheap);
	gl_free(sm->cols);

	//Null them, because I'm paranoid
	sm->llheap = nullptr;
	sm->cols = nullptr;

	//Zero the last one
	sm->ncols = 0;
}

void sparse_reset(SPARSE* sm, int ncols)
{
	int indexval;

	//Set the size
	sm->ncols = ncols;

	//Do brute force way, for paranoia
	for (indexval=0; indexval<ncols; indexval++)
	{
		sm->cols[indexval] = nullptr;
	}
	
	//Set the location pointer
	sm->llptr = 0;
}

//Add in new elements to the sparse notation
inline void sparse_add(SPARSE* sm, int row, int col, double value, BUSDATA *bus_values, unsigned int bus_values_count, NR_SOLVER_STRUCT *powerflow_information, int island_number_curr)
{
	unsigned int bus_index_val, bus_start_val, bus_end_val;
	bool found_proper_bus_val;

	SP_E* insertion_point = sm->cols[col];
	SP_E* new_list_element = &(sm->llheap[sm->llptr++]);

	new_list_element->next = nullptr;
	new_list_element->row_ind = row;
	new_list_element->value = value;

	//if there's a non empty list, traverse to find our rightful insertion point
	if(insertion_point != nullptr)
	{
		if(insertion_point->row_ind > new_list_element->row_ind)
		{
			//insert the new list element at the first position
			new_list_element->next = insertion_point;
			sm->cols[col] = new_list_element;
		}
		else
		{
			while((insertion_point->next != nullptr) && (insertion_point->next->row_ind < new_list_element->row_ind))
			{
				insertion_point = insertion_point->next;
			}

			//Duplicate check -- see how we exited
			if (insertion_point->next != nullptr)	//We exited because the next element is GEQ to the new element
			{
				if (insertion_point->next->row_ind == new_list_element->row_ind)	//Same entry (by column), so bad
				{
					//Reset the flag
					found_proper_bus_val = false;

					//Loop through and see if we can find the bus
					for (bus_index_val=0; bus_index_val<bus_values_count; bus_index_val++)
					{
						//Island check
						if (bus_values[bus_index_val].island_number == island_number_curr)
						{
							//Extract the start/stop indices
							bus_start_val = 2*bus_values[bus_index_val].Matrix_Loc;
							bus_end_val = bus_start_val + 2*powerflow_information->BA_diag[bus_index_val].size - 1;

							//See if we're in this range
							if ((new_list_element->row_ind >= bus_start_val) && (new_list_element->row_ind <= bus_end_val))
							{
								//See if it is actually named -- it should be available
								if (bus_values[bus_index_val].name != nullptr)
								{
									GL_THROW("NR: duplicate admittance entry found - attaches to node %s - check for parallel circuits between common nodes!",bus_values[bus_index_val].name);
									/*  TROUBLESHOOT
									While building up the admittance matrix for the Newton-Raphson solver, a duplicate entry was found.
									This is often caused by having multiple lines on the same phases in parallel between two nodes.  A reference node
									does not have a name.  Please name your nodes and and try again.  Afterwards, please reconcile this model difference and try again.
									*/
								}
								else
								{
									GL_THROW("NR: duplicate admittance entry found - no name available - check for parallel circuits between common nodes!");
									/*  TROUBLESHOOT
									While building up the admittance matrix for the Newton-Raphson solver, a duplicate entry was found.
									This is often caused by having multiple lines on the same phases in parallel between two nodes.  A reference node
									does not have a name.  Please name your nodes and and try again.  Afterwards, please reconcile this model difference and try again.
									*/
								}//End unnamed bus
							}//End found a bus
							//Default else -- keep going
						}//End part of the island
						//Default else -- next bus
					}//End of the for loop to find the bus
				}//End matches - error
			}
			else	//No next item, so see if our value matches
			{
				if (insertion_point->row_ind == new_list_element->row_ind)	//Same entry (by column), so bad
				{
					//Reset the flag
					found_proper_bus_val = false;

					//Loop through and see if we can find the bus
					for (bus_index_val=0; bus_index_val<bus_values_count; bus_index_val++)
					{
						//Island check
						if (bus_values[bus_index_val].island_number == island_number_curr)
						{
							//Extract the start/stop indices
							bus_start_val = 2*bus_values[bus_index_val].Matrix_Loc;
							bus_end_val = bus_start_val + 2*powerflow_information->BA_diag[bus_index_val].size - 1;

							//See if we're in this range
							if ((new_list_element->row_ind >= bus_start_val) && (new_list_element->row_ind <= bus_end_val))
							{
								//See if it is actually named -- it should be available
								if (bus_values[bus_index_val].name != nullptr)
								{
									GL_THROW("NR: duplicate admittance entry found - attaches to node %s - check for parallel circuits between common nodes!",bus_values[bus_index_val].name);
									//Defined above
								}
								else
								{
									GL_THROW("NR: duplicate admittance entry found - no name available - check for parallel circuits between common nodes!");
									//Defined above
								}//End unnamed bus
							}//End found a bus
							//Default else -- keep going
						}//End part of the island
						//Default else -- next bus
					}//End of the for loop to find the bus
				}
			}

			//insert the new list element at the next position
			new_list_element->next = insertion_point->next;
			insertion_point->next = new_list_element;
		}
	}
	else
		sm->cols[col] = new_list_element;
}

void sparse_tonr(SPARSE* sm, NR_SOLVER_VARS *matrices_LUin)
{
	//traverse each linked list, which are in order, and copy values into new array
	unsigned int rowidx = 0;
	unsigned int colidx = 0;
	unsigned int i;
	SP_E* LL_pointer;

	matrices_LUin->cols_LU[0] = 0;
	LL_pointer = nullptr;
	for(i = 0; i < sm->ncols; i++)
	{
		LL_pointer = sm->cols[i];
		if(LL_pointer != nullptr)
		{
			matrices_LUin->cols_LU[colidx++] = rowidx;
		}		
		while(LL_pointer != nullptr)
		{
			matrices_LUin->rows_LU[rowidx] = LL_pointer->row_ind; // row pointers of non zero values
			matrices_LUin->a_LU[rowidx] = LL_pointer->value;
			++rowidx;
			LL_pointer = LL_pointer->next;
		}		
	}
}

/** Newton-Raphson solver
	Solves a power flow problem using the Newton-Raphson method
	
	@return n=0 on failure to complete a single iteration, 
	n>0 to indicate success after n interations, or 
	n<0 to indicate failure after n iterations
 **/
int64 solver_nr(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, NR_SOLVER_STRUCT *powerflow_values, NRSOLVERMODE powerflow_type , NR_MESHFAULT_IMPEDANCE *mesh_imped_vals, bool *bad_computations)
{
	//Index for "islanding operations", when needed
	int island_index_val;

	//Temporary island looping value
	int island_loop_index;

	//File pointer for debug outputs
	FILE *FPoutVal;

	//Saturation mismatch tracking variable
	int func_result_val;

	//Temporary function variable
	FUNCTIONADDR temp_fxn_val;

	//Generic status variable
	STATUS call_return_status;

	//Phase collapser variable
	unsigned char phase_worka, phase_workb, phase_workc, phase_workd, phase_worke;

	//Temporary calculation variables
	double tempIcalcReal, tempIcalcImag;
	double tempPbus; //tempPbus store the temporary value of active power load at each bus
	double tempQbus; //tempQbus store the temporary value of reactive power load at each bus

	//Miscellaneous index variable
	unsigned int indexer, tempa, tempb, jindexer, kindexer;
	char jindex, kindex;
	char temp_index, temp_index_b;
	unsigned int temp_index_c;
	char mat_temp_index;

	//Working matrix for admittance collapsing/determinations
	gld::complex tempY[3][3];

	//Working matrix for mesh fault impedance storage, prior to "reconstruction"
	double temp_z_store[6][6];

	//Miscellaneous flag variables
	bool Full_Mat_A, Full_Mat_B, proceed_flag, temp_bool_value;

	//Deltamode intermediate variables
	gld::complex temp_complex_0, temp_complex_1, temp_complex_2, temp_complex_3, temp_complex_4;
	gld::complex aval, avalsq;

	//Temporary size variable
	char temp_size, temp_size_b, temp_size_c;

	//Temporary admittance variables
	gld::complex Temp_Ad_A[3][3];
	gld::complex Temp_Ad_B[3][3];

	//DV checking array
	gld::complex DVConvCheck[3];
	gld::complex currVoltConvCheck[3];
	double CurrConvVal;

	//Miscellaneous working variable
	double work_vals_double_0, work_vals_double_1,work_vals_double_2,work_vals_double_3,work_vals_double_4;
	char work_vals_char_0;
	bool something_has_been_output;

	//SuperLU variables
	SuperMatrix L_LU,U_LU;
	NCformat *Astore;
	DNformat *Bstore;
	int nnz, info;
	unsigned int m,n;
	double *sol_LU;

	//Spare notation variable - for output
	SP_E *temp_element;
	int row, col;
	double value;

	//Multi-island passes
	bool still_iterating_islands;
	bool proceed_to_next_island;
	int64 return_value_for_solver_NR;

	//Multi-island pointer to current superLU variables
	SUPERLU_NR_vars *curr_island_superLU_vars;
	
#ifndef MT
	superlu_options_t options;	//Additional variables for sequential superLU
	SuperLUStat_t stat;
#endif

	//Set the global - we're working now, so no more adjustments to island arrays until we're done (except removals)
	NR_solver_working = true;

	//General "short circuit check" - if there are no islands, just leave
	if (NR_islands_detected <= 0)
	{
		//Make sure the bad computations flag is set
		*bad_computations = false;

		return 0;	//Not really a valid return, but with the false above, should still continue
	}

	//Ensure bad computations flag is set first
	*bad_computations = false;

	//Determine special circumstances of SWING bus -- do we want it to truly participate right
	if (powerflow_type != PF_NORMAL)
	{
		if (powerflow_type == PF_DYNCALC)	//Parse the list -- anything that is a swing and a generator, deflag it out of principle (and to make it work right)
		{
			//Set the master swing flag - do for all islands
			for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
			{
				powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing = false;
			}

			//Check the buses
			for (indexer=0; indexer<bus_count; indexer++)
			{
				//See if we're a swing-flagged bus
				if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)
				{
					//See if we're "generator ready"
					if (*bus[indexer].dynamics_enabled && (bus[indexer].DynCurrent != nullptr))
					{
						//Deflag us back to "PQ" status
						bus[indexer].swing_functions_enabled = false;

						//If FPI, flag an admittance update (because now the SWING exists again)
						if (NR_solver_algorithm == NRM_FPI)
						{
							NR_admit_change = true;
						}
					}
				}
				//Default else -- normal bus
			}//End bus traversion loop
		}//Handle running dynamics differently
		else	//Must be PF_DYNINIT
		{
			//Loop through the islands
			for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
			{
				//Flag us as true, initially
				powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing = true;
			}
		}
	}//End not normal
	else	//Must be normal
	{
		//Loop through all islands
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing = true;	//Flag as a swing, even though this shouldn't do anything
		}
	}

	//Populate aval, if necessary
	if (powerflow_type == PF_DYNINIT)
	{
		//Conversion variables - 1@120-deg
		aval = gld::complex(-0.5,(sqrt(3.0)/2.0));
		avalsq = aval*aval;	//squared value is used a couple places too
	}
	else	//Zero it, just in case something uses it (what would???)
	{
		aval = 0.0;
		avalsq = 0.0;
	}

	if (matrix_solver_method==MM_EXTERN)
	{
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			//Call the initialization routine
			powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars = ((void *(*)(void *))(LUSolverFcns.ext_init))(powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars);

			//Make sure it worked (allocation check)
			if (powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars==nullptr)
			{
				GL_THROW("External LU matrix solver failed to allocate memory properly!");
				/*  TROUBLESHOOT
				While attempting to allocate memory for the external LU solver, an error occurred.
				Please try again.  If the error persists, ensure your external LU solver is behaving correctly
				and coordinate with their development team as necessary.
				*/
			}
		}//End Island loop
	}
	else if (matrix_solver_method == MM_SUPERLU)	//SuperLU multi-island-related item
	{
		//Loop through and see if we need to allocate things
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			//See if the superLU variables are populated
			if (powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars == nullptr)
			{
				//Allocate one up
				curr_island_superLU_vars = (SUPERLU_NR_vars *)gl_malloc(sizeof(SUPERLU_NR_vars));

				//Make sure it worked
				if (curr_island_superLU_vars == nullptr)
				{
					GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");
					//Defined elsewhere
				}

				//Initialize it, for giggles/paranoia
				curr_island_superLU_vars->A_LU.Store = nullptr;

				//Do the same for the underlying properties, just because
				curr_island_superLU_vars->A_LU.Stype = SLU_NC;
				curr_island_superLU_vars->A_LU.Dtype = SLU_D;
				curr_island_superLU_vars->A_LU.Mtype = SLU_GE;
				curr_island_superLU_vars->A_LU.nrow = 0;
				curr_island_superLU_vars->A_LU.ncol = 0;

				//Repeat for B_LU
				curr_island_superLU_vars->B_LU.Store = nullptr;

				//Do the same for the underlying properties, just because
				curr_island_superLU_vars->B_LU.Stype = SLU_NC;
				curr_island_superLU_vars->B_LU.Dtype = SLU_D;
				curr_island_superLU_vars->B_LU.Mtype = SLU_GE;
				curr_island_superLU_vars->B_LU.nrow = 0;
				curr_island_superLU_vars->B_LU.ncol = 0;

				//Other arrays
				curr_island_superLU_vars->perm_c = nullptr;
				curr_island_superLU_vars->perm_r = nullptr;

				//Assign it in
				powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars = (void *)curr_island_superLU_vars;
			}
		}

		//Renull the random pointer, just in case
		curr_island_superLU_vars = nullptr;
	}

	//If FPI, call the shunt update function
	if (NR_solver_algorithm == NRM_FPI)
	{
		//Bus loop it
		for (indexer=0; indexer<bus_count; indexer++)
		{
			if (bus[indexer].ShuntUpdateFxn != nullptr)
			{
				//Call the function
				call_return_status = ((STATUS (*)(OBJECT *))(*bus[indexer].ShuntUpdateFxn))(bus[indexer].obj);

				//Make sure it worked
				if (call_return_status == FAILED)
				{
					GL_THROW("NR: Shunt update failed for device %s",bus[indexer].obj->name ? bus[indexer].obj->name : "Unnamed");
					/*  TROUBLESHOOT
					While attempting to perform the shunt update function call, something failed.  Please try again.
					If the error persists, please submit your code and a bug report via the ticketing system.
					*/
				}
				//Default else - it worked
			}
		}//End bus loop
	}//End FPI shunt update call

	//Call the admittance update code
	NR_admittance_update(bus_count,bus,branch_count,branch,powerflow_values,powerflow_type);

	//Loop through each island to reset these flags
	for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
	{
		//Reset saturation checks
		powerflow_values->island_matrix_values[island_loop_index].SaturationMismatchPresent = false;

		//Reset Norton equivalent checks
		powerflow_values->island_matrix_values[island_loop_index].NortonCurrentMismatchPresent = false;

		//Reset the iteration counters
		powerflow_values->island_matrix_values[island_loop_index].iteration_count = 0;
	}

	//Reset the global flag for iterations on islands -- will be set at the end
	still_iterating_islands = true;

	//Also reset island index
	island_loop_index = 0;

	//While it - loop through until we solve the issue
	while (still_iterating_islands)
	{
		//Map the superLU variables each time -- just easier to do it always
		if (matrix_solver_method==MM_SUPERLU)
		{
			//Put the void pointer into a local context
			curr_island_superLU_vars = (SUPERLU_NR_vars *)powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars;
		}

		//Call the load subfunction
		compute_load_values(bus_count,bus,powerflow_values,false,island_loop_index);
	
		// Calculate the mismatch of three phase current injection at each bus (deltaI), 
		//and store the deltaI in terms of real and reactive value in array powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR
		if (powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR==nullptr)
		{
			powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR = (double *)gl_malloc((2*powerflow_values->island_matrix_values[island_loop_index].total_variables) *sizeof(double));   // left_hand side of equation (11)

			//Make sure it worked
			if (powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR == nullptr)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Update the max size
			powerflow_values->island_matrix_values[island_loop_index].max_total_variables = powerflow_values->island_matrix_values[island_loop_index].total_variables;
		}
		else if (powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed)		//Bigger sized (this was checked above)
		{
			//Decimate the existing value
			gl_free(powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR);

			//Reallocate it...bigger...faster...stronger!
			powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR = (double *)gl_malloc((2*powerflow_values->island_matrix_values[island_loop_index].total_variables) *sizeof(double));

			//Make sure it worked
			if (powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR == nullptr)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Store the updated value
			powerflow_values->island_matrix_values[island_loop_index].max_total_variables = powerflow_values->island_matrix_values[island_loop_index].total_variables;
		}

		//If we're in PF_DYNINIT mode, initialize the balancing convergence
		if (powerflow_type == PF_DYNINIT)
		{
			powerflow_values->island_matrix_values[island_loop_index].swing_converged = true;	//init it to yes, fail by exception, not default
		}

		//Compute the calculated loads (not specified) at each bus
		for (indexer=0; indexer<bus_count; indexer++) //for specific bus k
		{
			//Make sure the bus we're looking at is relevant to this island - otherwise, we can skip it
			if (bus[indexer].island_number == island_loop_index)
			{
				//Mode check
				if (NR_solver_algorithm == NRM_FPI)
				{
					if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)
					{
						//See if we're a "SWING SWING" or a generator
						if (*bus[indexer].dynamics_enabled && (bus[indexer].full_Y != nullptr) && (bus[indexer].DynCurrent != nullptr))
						{
							//We're not (SWING and SWING-enabled and doing  a dynamic init) (not at beginning)
							if (!(powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing && (powerflow_type == PF_DYNINIT)))
							{
								continue;	//Skip us - won't be part of the fixed diagonal
							}
						}
						else
						{
							continue;	//Normal SWING, just skip us
						}
					}//End SWING and acting as SWING
				}//End mode check
				//Default TCIM - keep

				//Update for generator symmetry - only in static dynamic mode and when SWING is a SWING
				if ((powerflow_type == PF_DYNINIT) && powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing)
				{
					//Secondary check - see if it is even needed - basically built around 3-phase right now
					//Initializes the Norton equivalent item -- normal generators shoudln't need this
					if (*bus[indexer].dynamics_enabled && (bus[indexer].full_Y != nullptr) && (bus[indexer].DynCurrent != nullptr))
					{
						//See if we're three phase or triplex
						if ((bus[indexer].phases & 0x07) == 0x07)
						{
							//Form denominator term of Ii, since it won't change
							temp_complex_1 = (~bus[indexer].V[0]) + (~bus[indexer].V[1])*avalsq + (~bus[indexer].V[2])*aval;

							//Form up numerator portion that doesn't change (Q and admittance)
							//Do in parts, just for readability
							//Row 1 of admittance mult
							temp_complex_0 = ~bus[indexer].V[0]*(bus[indexer].full_Y[0]*bus[indexer].V[0] + bus[indexer].full_Y[1]*bus[indexer].V[1] + bus[indexer].full_Y[2]*bus[indexer].V[2]);

							//Row 2 of admittance
							temp_complex_0 += ~bus[indexer].V[1]*(bus[indexer].full_Y[3]*bus[indexer].V[0] + bus[indexer].full_Y[4]*bus[indexer].V[1] + bus[indexer].full_Y[5]*bus[indexer].V[2]);

							//Row 3 of admittance
							temp_complex_0 += ~bus[indexer].V[2]*(bus[indexer].full_Y[6]*bus[indexer].V[0] + bus[indexer].full_Y[7]*bus[indexer].V[1] + bus[indexer].full_Y[8]*bus[indexer].V[2]);

							//Make the conjugate - used for individual phase accumulation later
							temp_complex_3 = ~temp_complex_0;

							//If we are a SWING, zero our PT portion and QT for accumulation
							if (((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled) && (powerflow_values->island_matrix_values[island_loop_index].iteration_count>0))
							{
								*bus[indexer].PGenTotal = gld::complex(0.0,0.0);
							}
						}
						else if ((bus[indexer].phases & 0x80) == 0x80)	//Triplex
						{
							//Get the "delta voltage" for use here
							temp_complex_4 = bus[indexer].V[0] + bus[indexer].V[1];

							//Form denominator term of Ii, since it won't change
							temp_complex_1 = ~temp_complex_4;

							//Form up numerator portion that doesn't change (Q and admittance)
							//Should just be a single entry, for "reasons/asssumptions"
							temp_complex_0 = ~temp_complex_4*(bus[indexer].full_Y[0]*temp_complex_4);

							//Make the conjugate - used for individual phase accumulation later
							temp_complex_3 = ~temp_complex_0;

							//If we are a SWING, zero our PT portion and QT for accumulation
							if (((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled) && (powerflow_values->island_matrix_values[island_loop_index].iteration_count>0))
							{
								*bus[indexer].PGenTotal = -temp_complex_3;	//Gets piecemeal removed from three-phase, just all at once here!
							}
						}
						else if ((bus[indexer].phases & 0x07) == 0x00)	//No phases
						{
							temp_complex_1 = gld::complex(0.0,0.0);
							temp_complex_0 = gld::complex(0.0,0.0);
							temp_complex_3 = gld::complex(0.0,0.0);

							//If we are a SWING, zero our PT portion and QT for accumulation
							if (((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled) && (powerflow_values->island_matrix_values[island_loop_index].iteration_count>0))
							{
								*bus[indexer].PGenTotal = gld::complex(0.0,0.0);
							}
						}
						//Default else - some other permutation, but not really supported (single-phase swing, or partial swing

						//numerator done, except PT portion (add in below - SWING bus is different
					}
					else	//Not enabled or not "full-Y-ed" - set to zero
					{
						temp_complex_0 = 0.0;
						temp_complex_1 = 0.0;	//Basically a flag to ignore it
					}
				}//End PF_DYNINIT and SWING is still the same

				for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++) // rows - for specific phase that exists
				{
					tempIcalcReal = tempIcalcImag = 0;   

					if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase - triplex bus
					{
						//Two states of triplex bus - To node of SPCT transformer needs to be different
						//First different - Delta-I and diagonal contributions
						if ((bus[indexer].phases & 0x20) == 0x20)	//We're the To bus
						{
							//Pre-negated due to the nature of how it's calculated (V1 compared to I1)
							tempPbus =  bus[indexer].PL[jindex];	//Copy load amounts in
							tempQbus =  bus[indexer].QL[jindex];	
						}
						else	//We're just a normal triplex bus
						{
							//This one isn't negated (normal operations)
							tempPbus =  -bus[indexer].PL[jindex];	//Copy load amounts in
							tempQbus =  -bus[indexer].QL[jindex];	
						}//end normal triplex bus

						if (NR_solver_algorithm == NRM_TCIM)
						{
							//Get diagonal contributions - only (& always) 2
							//Column 1
							tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Im();// equation (7), the diag elements of bus admittance matrix
							tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Re();// equation (8), the diag elements of bus admittance matrix

							//Column 2
							tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Im();// equation (7), the diag elements of bus admittance matrix
							tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Re();// equation (8), the diag elements of bus admittance matrix
						}
						else	//Implies FPI
						{
							if (powerflow_type == PF_DYNINIT)
							{
								if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
								{
									//Copy from above, since initialization is same
									//Get diagonal contributions - only (& always) 2
									//Column 1
									tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Im();// equation (7), the diag elements of bus admittance matrix
									tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Re();// equation (8), the diag elements of bus admittance matrix

									//Column 2
									tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Im();// equation (7), the diag elements of bus admittance matrix
									tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Re();// equation (8), the diag elements of bus admittance matrix
								}
								//Not a swing-initing-generator, so ignore it
							}
							//Default else -Not initailization, so no injection if not SWING (and not here)
						}

						//Now off diagonals
						for (kindexer=0; kindexer<(bus[indexer].Link_Table_Size); kindexer++)
						{
							//Apply proper index to jindexer (easier to implement this way)
							jindexer=bus[indexer].Link_Table[kindexer];

							if (branch[jindexer].from == indexer)	//We're the from bus
							{
								if ((bus[indexer].phases & 0x20) == 0x20)	//SPCT from bus - needs different signage
								{
									if (NR_solver_algorithm == NRM_TCIM)
									{
										work_vals_char_0 = jindex*3;

										//This situation can only be a normal line (triplex will never be the from for another type)
										//Again only, & always 2 columns (just do them explicitly)
										//Column 1
										tempIcalcReal += ((branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - ((branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += ((branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + ((branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

										//Column2
										tempIcalcReal += ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									}
									else	//Implies FPI
									{
										if (powerflow_type == PF_DYNINIT)
										{
											if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
											{
												//Copy from above, since initialization is same
												work_vals_char_0 = jindex*3;

												//This situation can only be a normal line (triplex will never be the from for another type)
												//Again only, & always 2 columns (just do them explicitly)
												//Column 1
												tempIcalcReal += ((branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - ((branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += ((branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + ((branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

												//Column2
												tempIcalcReal += ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											}
											//Not a swing-initing-generator, so ignore it
										}
										//Not initing, so no care

										//SWING bus does something different - injections
										if ((bus[branch[jindexer].to].type > 1) && bus[branch[jindexer].to].swing_functions_enabled)
										{
											work_vals_char_0 = jindex*3;

											//This situation can only be a normal line (triplex will never be the from for another type)
											//Again only, & always 2 columns (just do them explicitly)
											//Column 1
											tempIcalcReal -= ((branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - ((branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();
											tempIcalcImag -= ((branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + ((branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();

											//Column2
											tempIcalcReal -= ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();
											tempIcalcImag -= ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();
										}
										//Default else - normal line, so no injection
									}
								}//End SPCT To bus - from diagonal contributions
								else		//Normal line connection to normal triplex
								{
									if (NR_solver_algorithm == NRM_TCIM)
									{
										work_vals_char_0 = jindex*3;
										//This situation can only be a normal line (triplex will never be the from for another type)
										//Again only, & always 2 columns (just do them explicitly)
										//Column 1
										tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

										//Column2
										tempIcalcReal += (-(branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += (-(branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									}
									else	//Implies FPI
									{
										if (powerflow_type == PF_DYNINIT)
										{
											if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
											{
												//Copy from above, since initialization is same
												work_vals_char_0 = jindex*3;
												//This situation can only be a normal line (triplex will never be the from for another type)
												//Again only, & always 2 columns (just do them explicitly)
												//Column 1
												tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

												//Column2
												tempIcalcReal += (-(branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += (-(branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											}
											//Not a swing-initing-generator, so ignore it
										}
										//Not initing, so no care

										if ((bus[branch[jindexer].to].type > 1) && bus[branch[jindexer].to].swing_functions_enabled)
										{
											work_vals_char_0 = jindex*3;
											//This situation can only be a normal line (triplex will never be the from for another type)
											//Again only, & always 2 columns (just do them explicitly)
											//Column 1
											tempIcalcReal -= (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();
											tempIcalcImag -= (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();

											//Column2
											tempIcalcReal -= (-(branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();
											tempIcalcImag -= (-(branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();
										}
										//Default else - normal bus, so ignore
									}
								}//end normal triplex from
							}//end from bus
							else if (branch[jindexer].to == indexer)	//We're the to bus
							{
								if (branch[jindexer].v_ratio != 1.0)	//Transformer
								{
									//Only a single contributor on the from side - figure out how to get to it
									if ((branch[jindexer].phases & 0x01) == 0x01)	//C
									{
										temp_index=2;
									}
									else if ((branch[jindexer].phases & 0x02) == 0x02)	//B
									{
										temp_index=1;
									}
									else if ((branch[jindexer].phases & 0x04) == 0x04)	//A
									{
										temp_index=0;
									}
									else	//How'd we get here!?!
									{
										GL_THROW("NR: A split-phase transformer appears to have an invalid phase");
									}

									if (NR_solver_algorithm == NRM_TCIM)
									{
										work_vals_char_0 = jindex*3+temp_index;

										//Perform the update, it only happens for one column (nature of the transformer)
										tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									}
									else	//Implies FPI
									{
										if (powerflow_type == PF_DYNINIT)
										{
											if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
											{
												//Copy from above, since initialization is same
												work_vals_char_0 = jindex*3+temp_index;

												//Perform the update, it only happens for one column (nature of the transformer)
												tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											}
											//Not a swing-initing-generator, so ignore it
										}
										//Not initing, so no care

										//Bit ridiculous - another "low side triplex is a SWING" scenario
										if ((bus[branch[jindexer].from].type > 1) && bus[branch[jindexer].from].swing_functions_enabled)
										{
											work_vals_char_0 = jindex*3+temp_index;

											//Perform the update, it only happens for one column (nature of the transformer)
											tempIcalcReal -= (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Im();
											tempIcalcImag -= (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Re();
										}
										//Default else - other buses need nothing done
									}
								}//end transformer
								else									//Must be a normal line then
								{
									if ((bus[indexer].phases & 0x20) == 0x20)	//SPCT from bus - needs different signage
									{
										if (NR_solver_algorithm == NRM_TCIM)
										{
											work_vals_char_0 = jindex*3;
											//This case should never really exist, but if someone reverses a secondary or is doing meshed secondaries, it might
											//Again only, & always 2 columns (just do them explicitly)
											//Column 1
											tempIcalcReal += ((branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Re() - ((branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											tempIcalcImag += ((branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Im() + ((branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

											//Column2
											tempIcalcReal += ((branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - ((branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											tempIcalcImag += ((branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + ((branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										}
										else	//Implifes FPI
										{
											if (powerflow_type == PF_DYNINIT)
											{
												if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
												{
													//Copy from above, since initialization is same
													work_vals_char_0 = jindex*3;
													//This case should never really exist, but if someone reverses a secondary or is doing meshed secondaries, it might
													//Again only, & always 2 columns (just do them explicitly)
													//Column 1
													tempIcalcReal += ((branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Re() - ((branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
													tempIcalcImag += ((branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Im() + ((branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

													//Column2
													tempIcalcReal += ((branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - ((branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
													tempIcalcImag += ((branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + ((branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												}
												//Not a swing-initing-generator, so ignore it
											}
											//Not initing, so no care

											//SWING injections for FPIM
											if ((bus[branch[jindexer].from].type > 1) && bus[branch[jindexer].from].swing_functions_enabled)
											{
												work_vals_char_0 = jindex*3;
												//This case should never really exist, but if someone reverses a secondary or is doing meshed secondaries, it might
												//Again only, & always 2 columns (just do them explicitly)
												//Column 1
												tempIcalcReal -= ((branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Re() - ((branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Im();
												tempIcalcImag -= ((branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Im() + ((branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Re();

												//Column2
												tempIcalcReal -= ((branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - ((branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();
												tempIcalcImag -= ((branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + ((branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();
											}
											//Default else - not a swing, do nothing
										}
									}//End SPCT To bus - from diagonal contributions
									else		//Normal line connection to normal triplex
									{
										if (NR_solver_algorithm == NRM_TCIM)
										{
											work_vals_char_0 = jindex*3;
											//Again only, & always 2 columns (just do them explicitly)
											//Column 1
											tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

											//Column2
											tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										}
										else	//Implies FPI
										{
											if (powerflow_type == PF_DYNINIT)
											{
												if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
												{
													//Copy from above, since initialization is same
													work_vals_char_0 = jindex*3;
													//Again only, & always 2 columns (just do them explicitly)
													//Column 1
													tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
													tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

													//Column2
													tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
													tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												}
												//Not a swing-initing-generator, so ignore it
											}
											//Not initing, so no care

											if ((bus[branch[jindexer].from].type > 1) && bus[branch[jindexer].from].swing_functions_enabled)
											{
												work_vals_char_0 = jindex*3;
												//Again only, & always 2 columns (just do them explicitly)
												//Column 1
												tempIcalcReal -= (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Im();
												tempIcalcImag -= (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Re();

												//Column2
												tempIcalcReal -= (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();
												tempIcalcImag -= (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();
											}
											//Default else - not SWING, so do nothing
										}
									}//End normal triplex connection
								}//end normal line
							}//end to bus
							else	//We're nothing
								;

						}//End branch traversion

						//Determine how we are posting this update
						if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)	//SWING bus is different (when it really is a SWING bus)
						{
							if ((powerflow_type == PF_DYNINIT) && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
							{
								//See if we're a Norton-equivalent-based generator
								if (bus[indexer].full_Y != nullptr)
								{
									//Compute our "power generated" value for this phase - conjugated in formation
									temp_complex_2 = bus[indexer].V[jindex] * gld::complex(tempIcalcReal,-tempIcalcImag);

									if (powerflow_values->island_matrix_values[island_loop_index].iteration_count>0)	//Only update SWING on subsequent passes
									{
										//Now add it into the "generation total" for the SWING
										//Accumulate our power from the bus injections
										(*bus[indexer].PGenTotal) += temp_complex_2; // both PT and QT = Ssource-Sysource = v conj(I) - v conj(ysource v) - overall gen handled above
									}

									//Compute the delta_I, just like below - but don't post it (still zero in calcs)
									work_vals_double_0 = (bus[indexer].V[jindex]).Mag()*(bus[indexer].V[jindex]).Mag();

									if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
									{
										work_vals_double_1 = (bus[indexer].V[jindex]).Re();
										work_vals_double_2 = (bus[indexer].V[jindex]).Im();
										work_vals_double_3 = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal; // equation(7), Real part of deltaI, left hand side of equation (11)
										work_vals_double_4 = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)

										//Form a magnitude value - put in work_vals_double_0, since we're done with it
										work_vals_double_0 = sqrt((work_vals_double_3*work_vals_double_3+work_vals_double_4*work_vals_double_4));
									}
									else	//Would give us a NAN, so it must be out of service or something (case from below - really shouldn't apply to SWING)
									{
										work_vals_double_0 = 0.0;	//Assumes "converged"
									}

									if (work_vals_double_0 > bus[indexer].max_volt_error)	//Failure check (defaults to voltage convergence for now)
									{
										powerflow_values->island_matrix_values[island_loop_index].swing_converged=false;
									}
								}//End Norton-equivalent SWING
								else	//Other generator types
								{
									//Compute the delta_I, just like below - but don't post it (still zero in calcs)
									work_vals_double_0 = (bus[indexer].V[jindex]).Mag()*(bus[indexer].V[jindex]).Mag();

									if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
									{
										work_vals_double_1 = (bus[indexer].V[jindex]).Re();
										work_vals_double_2 = (bus[indexer].V[jindex]).Im();
										work_vals_double_3 = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal; // equation(7), Real part of deltaI, left hand side of equation (11)
										work_vals_double_4 = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)

										//Form a magnitude value - put in work_vals_double_0, since we're done with it
										work_vals_double_0 = sqrt((work_vals_double_3*work_vals_double_3+work_vals_double_4*work_vals_double_4));
									}
									else	//Would give us a NAN, so it must be out of service or something (case from below - really shouldn't apply to SWING)
									{
										work_vals_double_0 = 0.0;	//Assumes "converged"
									}

									if (work_vals_double_0 > bus[indexer].max_volt_error)	//Failure check (defaults to voltage convergence for now)
									{
										powerflow_values->island_matrix_values[island_loop_index].swing_converged=false;
									}

									//Put this into DynCurrent for storage
									/*** NOTE - This is untested and it's not clear if it is even needed (triplex swing bus?) ******/
									bus[indexer].DynCurrent[jindex] = gld::complex(tempIcalcReal,tempIcalcImag);
								}//End other generator types
							}//End PF_DYNINIT SWING traversion

							//Effectively Zero out the components, regardless of normal run or not
							//Should already be zerod, but do it again for paranoia sake
							if (NR_solver_algorithm == NRM_TCIM)
							{
								powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = 0.0;
								powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
							}
							//Assume not needed for FPI, due to it being a SWING bus

							//Saturation skipped for "swing is a swing" case, since it doesn't affect the admittance (no need to offset)
						}//End SWING bus cases
						else	//PQ bus or SWING masquerading as a PQ
						{
							work_vals_double_0 = (bus[indexer].V[jindex]).Mag()*(bus[indexer].V[jindex]).Mag();

							if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
							{
								if (NR_solver_algorithm == NRM_TCIM)
								{
									work_vals_double_1 = (bus[indexer].V[jindex]).Re();
									work_vals_double_2 = (bus[indexer].V[jindex]).Im();

									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal ; // equation(7), Real part of deltaI, left hand side of equation (11)
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)
								}
								else	//Must be FPI
								{
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = (tempIcalcReal - bus[indexer].FPI_current[jindex].Re()); // Real part of I
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempIcalcImag - bus[indexer].FPI_current[jindex].Im()); // Imaginary part of I
								}
							}
							else
							{
								//Zero both
								powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = 0.0;
								powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
							}
						}//End normal bus handling
					}//End split-phase present
					else	//Three phase or some variant thereof
					{
						tempPbus =  - bus[indexer].PL[jindex];	//Copy load amounts in
						tempQbus =  - bus[indexer].QL[jindex];	

						for (kindex=0; kindex<powerflow_values->BA_diag[indexer].size; kindex++)		//cols - Still only for specified phases
						{
							//Determine our indices, based on phase information
							temp_index = -1;
							switch(bus[indexer].phases & 0x07) {
								case 0x01:	//C
									{
										temp_index=2;
										break;
									}//end 0x01
								case 0x02:	//B
									{
										temp_index=1;
										break;
									}//end 0x02
								case 0x03:	//BC
									{
										if (kindex==0)	//B
											temp_index=1;
										else	//C
											temp_index=2;
										break;
									}//end 0x03
								case 0x04:	//A
									{
										temp_index=0;
										break;
									}//end 0x04
								case 0x05:	//AC
									{
										if (kindex==0)	//A
											temp_index=0;
										else			//C
											temp_index=2;
										break;
									}//end 0x05
								case 0x06:	//AB
									{
										if (kindex==0)	//A
											temp_index=0;
										else			//B
											temp_index=1;
										break;
									}//end 0x06
								case 0x07:	//ABC
									{
										temp_index = kindex;	//Will loop all 3
										break;
									}//end 0x07
							}//End switch/case

							if (temp_index==-1)	//Error check
							{
								GL_THROW("NR: A voltage index failed to be found.");
								/*  TROUBLESHOOT
								While attempting to compute the calculated power current, a voltage index failed to be
								resolved.  Please submit your code and a bug report via the trac website.
								*/
							}

							//Normal diagonal contributions
							if (NR_solver_algorithm == NRM_TCIM)
							{
								tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Im();// equation (7), the diag elements of bus admittance matrix
								tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Re();// equation (8), the diag elements of bus admittance matrix
							}
							else
							{
								if (powerflow_type == PF_DYNINIT)
								{
									if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
									{
										//Copy from above, since initialization is same
										tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Im();// equation (7), the diag elements of bus admittance matrix
										tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Re();// equation (8), the diag elements of bus admittance matrix
									}
									//Not a swing-initing-generator, so ignore it
								}
								//Default else -Not initailization, so no injection if not SWING (and not here)
							}

							//In-rush load contributions (if any) - only along explicit diagonal
							if ((bus[indexer].full_Y_load != nullptr) && (jindex==kindex) && (NR_solver_algorithm == NRM_TCIM))
							{
								mat_temp_index = temp_index*3+temp_index;	//Create index - make diagonal for inrush
								tempIcalcReal += (bus[indexer].full_Y_load[mat_temp_index]).Re() * (bus[indexer].V[temp_index]).Re() - (bus[indexer].full_Y_load[mat_temp_index]).Im() * (bus[indexer].V[temp_index]).Im();// equation (7), the diag elements of bus admittance matrix
								tempIcalcImag += (bus[indexer].full_Y_load[mat_temp_index]).Re() * (bus[indexer].V[temp_index]).Im() + (bus[indexer].full_Y_load[mat_temp_index]).Im() * (bus[indexer].V[temp_index]).Re();// equation (8), the diag elements of bus admittance matrix
							}

							//Off diagonal contributions
							//Need another variable to handle the rows
							temp_index_b = -1;
							switch(bus[indexer].phases & 0x07) {
								case 0x01:	//C
									{
										temp_index_b=2;
										break;
									}//end 0x01
								case 0x02:	//B
									{
										temp_index_b=1;
										break;
									}//end 0x02
								case 0x03:	//BC
									{
										if (jindex==0)	//B
											temp_index_b=1;
										else	//C
											temp_index_b=2;
										break;
									}//end 0x03
								case 0x04:	//A
									{
										temp_index_b=0;
										break;
									}//end 0x04
								case 0x05:	//AC
									{
										if (jindex==0)	//A
											temp_index_b=0;
										else			//C
											temp_index_b=2;
										break;
									}//end 0x05
								case 0x06:	//AB
									{
										if (jindex==0)	//A
											temp_index_b=0;
										else			//B
											temp_index_b=1;
										break;
									}//end 0x06
								case 0x07:	//ABC
									{
										temp_index_b = jindex;	//Will loop all 3
										break;
									}//end 0x07
							}//End switch/case

							if (temp_index_b==-1)	//Error check
							{
								GL_THROW("NR: A voltage index failed to be found.");
							}

							for (kindexer=0; kindexer<(bus[indexer].Link_Table_Size); kindexer++)	//Parse through the branch list
							{
								//Apply proper index to jindexer (easier to implement this way)
								jindexer=bus[indexer].Link_Table[kindexer];

								if (branch[jindexer].from == indexer) 
								{
									//See if we're a triplex transformer (will only occur on the from side)
									if ((branch[jindexer].phases & 0x80) == 0x80)	//Triplexy
									{
										proceed_flag = false;
										phase_worka = branch[jindexer].phases & 0x07;

										if (kindex==0)	//All of this will only occur the first column iteration
										{
											switch (bus[indexer].phases & 0x07)	{
												case 0x01:	//C
													{
														if (phase_worka==0x01)
															proceed_flag=true;
														break;
													}//end 0x01
												case 0x02:	//B
													{
														if (phase_worka==0x02)
															proceed_flag=true;
														break;
													}//end 0x02
												case 0x03:	//BC
													{
														if ((jindex==0) && (phase_worka==0x02))	//First row and is a B
															proceed_flag=true;
														else if ((jindex==1) && (phase_worka==0x01))	//Second row and is a C
															proceed_flag=true;
														else
															;
														break;
													}//end 0x03
												case 0x04:	//A
													{
														if (phase_worka==0x04)
															proceed_flag=true;
														break;
													}//end 0x04
												case 0x05:	//AC
													{
														if ((jindex==0) && (phase_worka==0x04))	//First row and is a A
															proceed_flag=true;
														else if ((jindex==1) && (phase_worka==0x01))	//Second row and is a C
															proceed_flag=true;
														else
															;
														break;
													}//end 0x05
												case 0x06:	//AB - shares with ABC
												case 0x07:	//ABC 
													{
														if ((jindex==0) && (phase_worka==0x04))	//A & first row
															proceed_flag=true;
														else if ((jindex==1) && (phase_worka==0x02))	//B & second row
															proceed_flag=true;
														else if ((jindex==2) && (phase_worka==0x01))	//C & third row
															proceed_flag=true;
														else;
														break;
													}//end 0x07
											}//end switch
										}//End if kindex==0

										if (proceed_flag)
										{
											if (NR_solver_algorithm == NRM_TCIM)
											{
												work_vals_char_0 = temp_index_b*3;
												//Do columns individually
												//1
												tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

												//2
												tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											}
											else	//Implies FPI - odd case, - implies a triplex SWING, but might as well be thorough
											{
												//Initialization code
												if (powerflow_type == PF_DYNINIT)
												{
													if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
													{
														//Same code as above
														work_vals_char_0 = temp_index_b*3;
														//Do columns individually
														//1
														tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
														tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

														//2
														tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
														tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
													}
													//Not a swing-initing-generator, so ignore it
												}
												//Not initing, so do normal things

												//SWING bus does something different
												if ((bus[branch[jindexer].to].type > 1) && bus[branch[jindexer].to].swing_functions_enabled)
												{
													work_vals_char_0 = temp_index_b*3;
													//Do columns individually
													//1
													tempIcalcReal -= (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();
													tempIcalcImag -= (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();

													//2
													tempIcalcReal -= (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();
													tempIcalcImag -= (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();
												}
												//Default else - not a SWING, so no injections
											}
										}
									}//end SPCT transformer
									else	///Must be a standard line
									{
										work_vals_char_0 = temp_index_b*3+temp_index;
										work_vals_double_0 = (-branch[jindexer].Yfrom[work_vals_char_0]).Re();
										work_vals_double_1 = (-branch[jindexer].Yfrom[work_vals_char_0]).Im();
										work_vals_double_2 = (bus[branch[jindexer].to].V[temp_index]).Re();
										work_vals_double_3 = (bus[branch[jindexer].to].V[temp_index]).Im();

										if (NR_solver_algorithm == NRM_TCIM)
										{
											tempIcalcReal += work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											tempIcalcImag += work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										}
										else	//Assumes FPI
										{
											//Initialization code
											if (powerflow_type == PF_DYNINIT)
											{
												if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
												{
													//Same code as above
													tempIcalcReal += work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
													tempIcalcImag += work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												}
												//Not a swing-initing-generator, so ignore it
											}
											//Not initing, so do normal things

											//SWING bus does something different
											if ((bus[branch[jindexer].to].type > 1) && bus[branch[jindexer].to].swing_functions_enabled)
											{
												tempIcalcReal -= work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;
												tempIcalcImag -= work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;
											}
											//Default else - do nothing
										}
									}//end standard line
								}	
								else if  (branch[jindexer].to == indexer)
								{
									work_vals_char_0 = temp_index_b*3+temp_index;
									work_vals_double_0 = (-branch[jindexer].Yto[work_vals_char_0]).Re();
									work_vals_double_1 = (-branch[jindexer].Yto[work_vals_char_0]).Im();
									work_vals_double_2 = (bus[branch[jindexer].from].V[temp_index]).Re();
									work_vals_double_3 = (bus[branch[jindexer].from].V[temp_index]).Im();

									if (NR_solver_algorithm == NRM_TCIM)
									{
										tempIcalcReal += work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									}
									else	//Assumed FPI
									{
										//Initialization code
										if (powerflow_type == PF_DYNINIT)
										{
											if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled && (bus[indexer].DynCurrent != nullptr))	//We're a generator-type bus
											{
												//Same code as above
												tempIcalcReal += work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
												tempIcalcImag += work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
											}
											//Not a swing-initing-generator, so ignore it
										}
										//Not initing, so do normal things

										//SWING bus does something different
										if ((bus[branch[jindexer].from].type > 1) && bus[branch[jindexer].from].swing_functions_enabled)
										{
											tempIcalcReal -= work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;
											tempIcalcImag -= work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;
										}
										//Default else - if not a SWING, no injection here
									}
								}
								//Default else - must be islanded or something
							}
						}//end intermediate current for each phase column

						//Determine how we are posting this update
						if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)	//SWING bus is different (when it really is a SWING bus)
						{
							if ((powerflow_type == PF_DYNINIT) && (bus[indexer].DynCurrent != nullptr))
							{
								//See if we're a Norton-equivalent-based generator
								if (bus[indexer].full_Y != nullptr)
								{
									//Compute our "power generated" value for this phase - conjugated in formation
									temp_complex_2 = bus[indexer].V[jindex] * gld::complex(tempIcalcReal,-tempIcalcImag);

									if (powerflow_values->island_matrix_values[island_loop_index].iteration_count>0)	//Only update SWING on subsequent passes
									{
										//Now add it into the "generation total" for the SWING
										//Accumulate our power from the bus injections
										(*bus[indexer].PGenTotal) += temp_complex_2 - temp_complex_3/3.0; // both PT and QT = Ssource-Sysource = v conj(I) - v conj(ysource v)
									}

									//Compute the delta_I, just like below - but don't post it (still zero in calcs)
									work_vals_double_0 = (bus[indexer].V[temp_index_b]).Mag()*(bus[indexer].V[temp_index_b]).Mag();

									if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
									{
										work_vals_double_1 = (bus[indexer].V[temp_index_b]).Re();
										work_vals_double_2 = (bus[indexer].V[temp_index_b]).Im();
										work_vals_double_3 = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal; // equation(7), Real part of deltaI, left hand side of equation (11)
										work_vals_double_4 = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)

										//Form a magnitude value - put in work_vals_double_0, since we're done with it
										work_vals_double_0 = sqrt((work_vals_double_3*work_vals_double_3+work_vals_double_4*work_vals_double_4));
									}
									else	//Would give us a NAN, so it must be out of service or something (case from below - really shouldn't apply to SWING)
									{
										work_vals_double_0 = 0.0;	//Assumes "converged"
									}
									
									if (work_vals_double_0 > bus[indexer].max_volt_error)	//Failure check (defaults to voltage convergence for now)
									{
										powerflow_values->island_matrix_values[island_loop_index].swing_converged=false;
									}
								}//End Norton-equivalent SWING
								else	//Other generator types
								{
									//Compute the delta_I, just like below - but don't post it (still zero in calcs)
									work_vals_double_0 = (bus[indexer].V[temp_index_b]).Mag()*(bus[indexer].V[temp_index_b]).Mag();

									if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
									{
										work_vals_double_1 = (bus[indexer].V[temp_index_b]).Re();
										work_vals_double_2 = (bus[indexer].V[temp_index_b]).Im();
										work_vals_double_3 = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal; // equation(7), Real part of deltaI, left hand side of equation (11)
										work_vals_double_4 = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)

										//Form a magnitude value - put in work_vals_double_0, since we're done with it
										work_vals_double_0 = sqrt((work_vals_double_3*work_vals_double_3+work_vals_double_4*work_vals_double_4));
									}
									else	//Would give us a NAN, so it must be out of service or something (case from below - really shouldn't apply to SWING)
									{
										work_vals_double_0 = 0.0;	//Assumes "converged"
									}

									if (work_vals_double_0 > bus[indexer].max_volt_error)	//Failure check (defaults to voltage convergence for now)
									{
										powerflow_values->island_matrix_values[island_loop_index].swing_converged=false;
									}

									//Put this into DynCurrent for storage
									bus[indexer].DynCurrent[jindex] = gld::complex(tempIcalcReal,tempIcalcImag);
								}//End other generator types
							}//End PF_DYNINIT SWING traversion

							//Effectively Zero out the components, regardless of normal run or not
							//Should already be zerod, but do it again for paranoia sake
							if (NR_solver_algorithm == NRM_TCIM)
							{
								if (NR_busdata[indexer].BusHistTerm != nullptr)	//See if we're "delta-capable"
								{
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = NR_busdata[indexer].BusHistTerm[jindex].Re();
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = NR_busdata[indexer].BusHistTerm[jindex].Im();
								}
								else
								{
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = 0.0;
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
								}
							}

							//Saturation skipped for "swing is a swing" case, since it doesn't affect the admittance (no need to offset)
						}//End SWING bus cases
						else	//PQ bus or SWING masquerading as a PQ
						{
							work_vals_double_0 = (bus[indexer].V[temp_index_b]).Mag()*(bus[indexer].V[temp_index_b]).Mag();

							if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
							{
								work_vals_double_1 = (bus[indexer].V[temp_index_b]).Re();
								work_vals_double_2 = (bus[indexer].V[temp_index_b]).Im();

								//See if deltamode needs to include extra term
								if (NR_busdata[indexer].BusHistTerm != nullptr)
								{
									if (NR_solver_algorithm == NRM_TCIM)
									{
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) + NR_busdata[indexer].BusHistTerm[jindex].Re() - tempIcalcReal ; // equation(7), Real part of deltaI, left hand side of equation (11)
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) + NR_busdata[indexer].BusHistTerm[jindex].Im() - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)
									}
									else	//Must be FPI
									{
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = (tempIcalcReal - bus[indexer].FPI_current[temp_index_b].Re() + NR_busdata[indexer].BusHistTerm[jindex].Re()); // Real part of I
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempIcalcImag - bus[indexer].FPI_current[temp_index_b].Im() + NR_busdata[indexer].BusHistTerm[jindex].Im()); // Imaginary part of I
									}
								}
								else	//Nope
								{
									if (NR_solver_algorithm == NRM_TCIM)
									{
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal ; // equation(7), Real part of deltaI, left hand side of equation (11)
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)
									}
									else	//Must be FPI
									{
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = (tempIcalcReal - bus[indexer].FPI_current[temp_index_b].Re()); // Real part of I
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempIcalcImag - bus[indexer].FPI_current[temp_index_b].Im()); // Imaginary part of I
									}
								}

								//Accumulate in any saturation current values as well, while we're here
								if (NR_busdata[indexer].BusSatTerm != nullptr)
								{
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] -= NR_busdata[indexer].BusSatTerm[jindex].Re();
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] -= NR_busdata[indexer].BusSatTerm[jindex].Im();
								}
							}
							else
							{
								//FPI doesn't appear to need an exception here, though it is a zero voltage case (so what is this?)
								if (NR_busdata[indexer].BusHistTerm != nullptr)	//See if extra deltamode term needs to be included
								{
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = NR_busdata[indexer].BusHistTerm[jindex].Re();
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = NR_busdata[indexer].BusHistTerm[jindex].Im();
								}
								else
								{
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = 0.0;
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
								}

								//Accumulate in any saturation current values as well, while we're here
								if (NR_busdata[indexer].BusSatTerm != nullptr)
								{
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] -= NR_busdata[indexer].BusSatTerm[jindex].Re();
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + jindex] -= NR_busdata[indexer].BusSatTerm[jindex].Im();
								}
							}
						}//End normal bus handling
					}//End three-phase or variant thereof
				}//End delta_I for each phase row

				//Call the overall current injection update for compatible objects (mostly deltamode Norton-equivalent generators)
				//See if this particular bus has any "current injection update" requirements - semi-silly to do this for SWING-enabled buses, but makes the code more consistent
				if ((bus[indexer].ExtraCurrentInjFunc != nullptr) && ((bus[indexer].type == 0) || ((bus[indexer].type > 1) && !bus[indexer].swing_functions_enabled)))
				{
					//Call the function
					call_return_status = ((STATUS (*)(OBJECT *,int64,bool *))(*bus[indexer].ExtraCurrentInjFunc))(bus[indexer].ExtraCurrentInjFuncObject,powerflow_values->island_matrix_values[island_loop_index].iteration_count,&temp_bool_value);

					//Make sure it worked
					if (call_return_status == FAILED)
					{
						GL_THROW("External current injection update failed for device %s",bus[indexer].ExtraCurrentInjFuncObject->name ? bus[indexer].ExtraCurrentInjFuncObject->name : "Unnamed");
						/*  TROUBLESHOOT
						While attempting to perform the external current injection update function call, something failed.  Please try again.
						If the error persists, please submit your code and a bug report via the ticketing system.
						*/
					}
					//Default else - it worked

					//Update the convergence flag, if needed
					if (temp_bool_value)
					{
						gl_verbose("External current injection indicated convergence failure on device %s",bus[indexer].ExtraCurrentInjFuncObject->name ? bus[indexer].ExtraCurrentInjFuncObject->name : "Unnamed");
						/*  TROUBLESHOOT
						An external current injection update indicated it is still changing and needs a reiteration.  This is typically
						the Norton-equivalent current injection for a generation source.  If this is unacceptable, loosen the convergence
						criterion on that that particular device.
						*/

						//Flag it
						powerflow_values->island_matrix_values[island_loop_index].NortonCurrentMismatchPresent = true;
					}
				}

				//Add in generator current amounts, if relevant
				if (powerflow_type != PF_NORMAL)
				{
					//Secondary check - see if it is even needed - basically built around 3-phase right now (checks)
					if (*bus[indexer].dynamics_enabled && (bus[indexer].DynCurrent != nullptr))
					{
						//See if we're a Norton-equivalent generator
						if (bus[indexer].full_Y != nullptr)
						{
							if ((bus[indexer].phases & 0x07) == 0x07)	//Make sure is three-phase
							{
								//Only update in particular mode - SWING bus values should still be treated as such
								if (powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing && (powerflow_type == PF_DYNINIT) && (bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)	//Really only true for PF_DYNINIT anyways
								{
									//Power should be all updated, now update current values
									temp_complex_0 += gld::complex((*bus[indexer].PGenTotal).Re(),-(*bus[indexer].PGenTotal).Im()); // total generated power injected conjugated

									//Compute Ii
									if (temp_complex_1.Mag() > 0.0)	//Non zero
									{
										temp_complex_2 = temp_complex_0/temp_complex_1;	//Forms Ii

										//Now convert Ii to the individual phase amounts
										bus[indexer].DynCurrent[0] = temp_complex_2;
										bus[indexer].DynCurrent[1] = temp_complex_2*avalsq;
										bus[indexer].DynCurrent[2] = temp_complex_2*aval;
									}
									else	//Must make zero
									{
										bus[indexer].DynCurrent[0] = gld::complex(0.0,0.0);
										bus[indexer].DynCurrent[1] = gld::complex(0.0,0.0);
										bus[indexer].DynCurrent[2] = gld::complex(0.0,0.0);
									}
								}//End SWING is still swing, otherwise should just accumulate what it had

								//Don't get added in as part of "normal swing" routine
								if ((bus[indexer].type == 0) || ((bus[indexer].type>1) && !bus[indexer].swing_functions_enabled))
								{
									//Add these into the system - added because "generation"
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[1].Re();	//Phase B
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 2] += bus[indexer].DynCurrent[2].Re();	//Phase C

									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[1].Im();	//Phase B
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 2] += bus[indexer].DynCurrent[2].Im();	//Phase C
								}
								//Defaulted else - leave as they were
							}//End three-phase
							else if ((bus[indexer].phases & 0x80) == 0x80)	//Triplex
							{
								//Duplicate of the three-phase code, just tweaked for triplex implementaiton

								//Only update in particular mode - SWING bus values should still be treated as such
								if (powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing && (powerflow_type == PF_DYNINIT) && (bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)	//Really only true for PF_DYNINIT anyways
								{
									//Power should be all updated, now update current values
									temp_complex_0 += gld::complex((*bus[indexer].PGenTotal).Re(),-(*bus[indexer].PGenTotal).Im()); // total generated power injected conjugated

									//Compute Ii
									if (temp_complex_1.Mag() > 0.0)	//Non zero
									{
										temp_complex_2 = temp_complex_0/temp_complex_1;	//Forms Ii

										//Just store Ii - no need to convert what is a single-phase representation!
										bus[indexer].DynCurrent[0] = temp_complex_2;
									}
									else	//Must make zero
									{
										bus[indexer].DynCurrent[0] = gld::complex(0.0,0.0);
									}
								}//End SWING is still swing, otherwise should just accumulate what it had

								//Don't get added in as part of "normal swing" routine
								if ((bus[indexer].type == 0) || ((bus[indexer].type>1) && !bus[indexer].swing_functions_enabled))
								{
									//See what kind of triplex we are
									if ((bus[indexer].phases & 0x20) == 0x20)	//SPCT "exception"
									{
										//Add these into the system - added because "generation" - assumption is it is a flow between 1 and 2
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] -= bus[indexer].DynCurrent[0].Re();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[0].Re();	//Phase 2

										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] -= bus[indexer].DynCurrent[0].Im();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[0].Im();	//Phase 2
									}
									else	//Standard triplex
									{
										//Add these into the system - added because "generation" - assumption is it is a flow between 1 and 2
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] -= bus[indexer].DynCurrent[0].Re();	//Phase 2

										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] -= bus[indexer].DynCurrent[0].Im();	//Phase 2
									}
								}
								//Defaulted else - leave as they were
							}
							else if ((bus[indexer].phases & 0x07) != 0x00)	//Some type of phasing
							{
								//SWING update functionality is not in here, just "normal"
								//Don't get added in as part of "normal swing" routine
								if (bus[indexer].type == 0)	//"SWING not a SWING" was here, but given the exception to being a SWING, it isn't needed here
								{
									//Figure out "where/what" to add
									//Case it out by phases
									switch (bus[indexer].phases & 0x07)	{
										case 0x01:	//C
											{
												//Add these into the system - added because "generation"
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[2].Re();		//Phase C
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[2].Im();		//Phase C
												break;
											}//end 0x01
										case 0x02:	//B
											{
												//Add these into the system - added because "generation"
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[1].Re();		//Phase B
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[1].Im();		//Phase B
												break;
											}
										case 0x03:	//BC
											{
												//Add these into the system - added because "generation"
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[1].Re();		//Phase C
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[2].Re();	//Phase C

												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[1].Im();		//Phase B
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[2].Im();	//Phase C
												break;
											}
										case 0x04:	//A
											{
												//Add these into the system - added because "generation"
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
												break;
											}
										case 0x05:	//AC
											{
												//Add these into the system - added because "generation"
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[2].Re();	//Phase C

												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[2].Im();	//Phase C
												break;
											}
										case 0x06:	//AB
											{
												//Add these into the system - added because "generation"
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[1].Re();	//Phase B

												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
												powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[1].Im();	//Phase B

												break;
											}
										default:	//Don't need an ABC or anything - how would we get here!?!
											{
												//Error for good measure
												GL_THROW("NR: A Norton-equivalent generator had an odd phase combination");
												/*  TROUBLESHOOT
												A Norton-equivalent generator attempted to interface via an unknow phase configuration.  Please try again.  If the error
												persists, please submit your code and a bug report via the ticking/issues system.
												*/
											}
									}//End switch/case
								}
								//Defaulted else - leave as they were - some other bustype
							}
							else	//Not any of the above
							{
								//See if we ever were (reliability call)
								if ((bus[indexer].origphases & 0x80) == 0x80)
								{
									//Zero the only one that should be being used
									bus[indexer].DynCurrent[0] = gld::complex(0.0,0.0);
								}
								else if ((bus[indexer].origphases & 0x07) != 0x00)	//Non-zero original phases, s just zero all
								{
									//Just zero it
									bus[indexer].DynCurrent[0] = gld::complex(0.0,0.0);
									bus[indexer].DynCurrent[1] = gld::complex(0.0,0.0);
									bus[indexer].DynCurrent[2] = gld::complex(0.0,0.0);
								}
								else	//Never was, just fail out
								{
									//Not sure how we ever reach this error check now - leaving for posterity
									GL_THROW("NR_solver: bus:%s has tried updating deltamode dynamics, but is not three-phase, single-phase, or triplex!",bus[indexer].name);
									/*  TROUBLESHOOT
									The NR_solver routine tried update a three-phase current value for a bus that is not
									a supported phase configuration.  At this time, the deltamode system only supports three-phase,
									single-phase, and triplex-conected buses for Norton-equivalent generator attachment.
									*/
								}
							}
						}//End Norton-equivalent call
						else	//Must be another generator
						{
							//Replicate the injection from above -- assume three-phase right now
							//Don't get added in as part of "normal swing" routine
							if ((bus[indexer].type == 0) || ((bus[indexer].type>1) && !bus[indexer].swing_functions_enabled))
							{
								//Check for three-phase
								if ((bus[indexer].phases & 0x07) == 0x07)
								{
									//Add these into the system - added because "generation"
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[1].Re();	//Phase B
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 2] += bus[indexer].DynCurrent[2].Re();	//Phase C

									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[1].Im();	//Phase B
									powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 2] += bus[indexer].DynCurrent[2].Im();	//Phase C
								}
								else if ((bus[indexer].phases & 0x80) == 0x80)
								{
									//Add these into the system - added because "generation"
									//Assume is a 1-2 connection (1 flowing into 2)

									//See what kind of triplex we are
									if ((bus[indexer].phases & 0x20) == 0x20) //SPCT "special"
									{
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] -= bus[indexer].DynCurrent[0].Re();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[0].Re();	//Phase 2

										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] -= bus[indexer].DynCurrent[0].Im();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[0].Im();	//Phase 2
									}
									else	//"Standard"
									{
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] -= bus[indexer].DynCurrent[0].Re();	//Phase 2

										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase 1
										powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] -= bus[indexer].DynCurrent[0].Im();	//Phase 2
									}
								}
								else if ((bus[indexer].phases & 0x07) != 0x00)	//It has something on it
									{
									//SWING update functionality is not in here, just "normal"
									//Don't get added in as part of "normal swing" routine
									if (bus[indexer].type == 0)	//"SWING not a SWING" was here, but given the exception to being a SWING, it isn't needed here
									{
										//Figure out "where/what" to add
										//Case it out by phases
										switch (bus[indexer].phases & 0x07)	{
											case 0x01:	//C
												{
													//Add these into the system - added because "generation"
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[2].Re();		//Phase C
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[2].Im();		//Phase C
													break;
												}//end 0x01
											case 0x02:	//B
												{
													//Add these into the system - added because "generation"
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[1].Re();		//Phase B
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[1].Im();		//Phase B
													break;
												}
											case 0x03:	//BC
												{
													//Add these into the system - added because "generation"
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[1].Re();		//Phase C
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[2].Re();	//Phase C

													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[1].Im();		//Phase B
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[2].Im();	//Phase C
													break;
												}
											case 0x04:	//A
												{
													//Add these into the system - added because "generation"
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
													break;
												}
											case 0x05:	//AC
												{
													//Add these into the system - added because "generation"
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[2].Re();	//Phase C

													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[2].Im();	//Phase C
													break;
												}
											case 0x06:	//AB
												{
													//Add these into the system - added because "generation"
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[1].Re();	//Phase B

													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
													powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[1].Im();	//Phase B

													break;
												}
											default:	//Don't need an ABC or anything - how would we get here!?!
												{
													//Error for good measure
													GL_THROW("NR: A Norton-equivalent generator had an odd phase combination");
													//Defined above
												}
										}//End switch/case
									}
									//Defaulted else - leave as they were - some other bustype

								}
								//Default else - should probably just be zero anyways
							}
							//Defaulted else - leave as they were - if it is still a swing, this is all moot anyways
						}//End other generator delta call
					}//End extra current adds
				}//End dynamic (generator postings)
			}//End part of this island check
			//Default else -- not in this island, so skip us!
		}//End delta_I for each bus

		//Call the load subfunction - flag for Jacobian update - only need to do for TCIM right now
		if (NR_solver_algorithm == NRM_TCIM)
		{
			compute_load_values(bus_count,bus,powerflow_values,true,island_loop_index);
		}

		//Build the dynamic diagnal elements of 6n*6n Y matrix. All the elements in this part will be updated at each iteration.

		//Reset it
		powerflow_values->island_matrix_values[island_loop_index].size_diag_update = 0;

		//Method check - FPIM doesn't need
		if (NR_solver_algorithm == NRM_TCIM)
		{
			//Adjust it
			for (jindexer=0; jindexer<bus_count;jindexer++)
			{
				//Check our relevance to this matrix
				if (bus[jindexer].island_number == island_loop_index)
				{
					if  (bus[jindexer].type != 1)	//PV bus ignored (for now?)
						powerflow_values->island_matrix_values[island_loop_index].size_diag_update += powerflow_values->BA_diag[jindexer].size;
					//Defaulted else - PV bus ignored
				}
				//Default else -- different island, skip for now
			}

			if (powerflow_values->island_matrix_values[island_loop_index].Y_diag_update == nullptr)
			{
				powerflow_values->island_matrix_values[island_loop_index].Y_diag_update = (Y_NR *)gl_malloc((4*powerflow_values->island_matrix_values[island_loop_index].size_diag_update) *sizeof(Y_NR));   //powerflow_values->island_matrix_values[island_loop_index].Y_diag_update store the row,column and value of the dynamic part of the diagonal PQ bus elements of 6n*6n Y_NR matrix.

				//Make sure it worked
				if (powerflow_values->island_matrix_values[island_loop_index].Y_diag_update == nullptr)
					GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

				//Update maximum size
				powerflow_values->island_matrix_values[island_loop_index].max_size_diag_update = powerflow_values->island_matrix_values[island_loop_index].size_diag_update;
			}
			else if (powerflow_values->island_matrix_values[island_loop_index].size_diag_update > powerflow_values->island_matrix_values[island_loop_index].max_size_diag_update)	//We've exceeded our limits
			{
				//Disappear the old one
				gl_free(powerflow_values->island_matrix_values[island_loop_index].Y_diag_update);

				//Make a new one in its image
				powerflow_values->island_matrix_values[island_loop_index].Y_diag_update = (Y_NR *)gl_malloc((4*powerflow_values->island_matrix_values[island_loop_index].size_diag_update) *sizeof(Y_NR));

				//Make sure it worked
				if (powerflow_values->island_matrix_values[island_loop_index].Y_diag_update == nullptr)
					GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

				//Update the size
				powerflow_values->island_matrix_values[island_loop_index].max_size_diag_update = powerflow_values->island_matrix_values[island_loop_index].size_diag_update;

				//Flag for a realloc
				powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed = true;
			}

			indexer = 0;	//Rest positional counter

			for (jindexer=0; jindexer<bus_count; jindexer++)	//Parse through bus list
			{
				//See if we're part of the currently progressing island -- if not, skip us
				if (bus[jindexer].island_number == island_loop_index)
				{
					if ((bus[jindexer].type > 1) && bus[jindexer].swing_functions_enabled)	//Swing bus - and we aren't ignoring it
					{
						for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
						{
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
							indexer += 1;

							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind + powerflow_values->BA_diag[jindexer].size;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re();	//Normal admittance portion
							indexer += 1;

							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind - powerflow_values->BA_diag[jindexer].size;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re();	//Normal admittance portion
							indexer += 1;

							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
							indexer += 1;
						}//End swing bus traversion
					}//End swing bus

					if ((bus[jindexer].type == 0) || ((bus[jindexer].type > 1) && !bus[jindexer].swing_functions_enabled))	//Only do on PQ (or SWING masquerading as PQ)
					{
						for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
						{
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_A[jindex]; // Equation(14)
							indexer += 1;

							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind + powerflow_values->BA_diag[jindexer].size;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_B[jindex]; // Equation(15)
							indexer += 1;

							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = 2*bus[jindexer].Matrix_Loc + jindex;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_C[jindex]; // Equation(16)
							indexer += 1;

							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind;
							powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value = -(powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_D[jindex]; // Equation(17)
							indexer += 1;
						}//end PQ phase traversion
					}//End PQ bus
				}//End island relevance check
			}//End bus parse list
		}
		//Default else - FPIM, nothing needed (zero size and NULL still valid)

		// Build the Amatrix, Amatrix includes all the elements of Y_offdiag_PQ, Y_diag_fixed and Y_diag_update.
		powerflow_values->island_matrix_values[island_loop_index].size_Amatrix = powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ*2 + powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed*2 + 4*powerflow_values->island_matrix_values[island_loop_index].size_diag_update;

		//Test to make sure it isn't an empty matrix - reliability induced 3-phase fault
		if (powerflow_values->island_matrix_values[island_loop_index].size_Amatrix==0)
		{
			//Figure out which message to send
			if (NR_islands_detected > 1)
			{
				gl_warning("Empty powerflow connectivity matrix for island %d, your system is empty!",(island_loop_index+1));
				/*  TROUBLESHOOT
				Newton-Raphson has an empty admittance matrix that it is trying to solve.  Either the whole system
				faulted, or something is not properly defined.  Please try again.  If the problem persists, please
				submit your code and a bug report via the trac website.
				*/
			}
			else	//Single island
			{
				gl_warning("Empty powerflow connectivity matrix, your system is empty!");
				/*  TROUBLESHOOT
				Newton-Raphson has an empty admittance matrix that it is trying to solve.  Either the whole system
				faulted, or something is not properly defined.  Please try again.  If the problem persists, please
				submit your code and a bug report via the trac website.
				*/
			}

			//Set our return code, like success
			powerflow_values->island_matrix_values[island_loop_index].return_code = 0;

			//Figure out next steps - routine copied from the end of the iteration loop

			//See if we need to continue
			if (island_loop_index >= (NR_islands_detected - 1))	//We're the last one
			{
				//Set the flag to be done
				still_iterating_islands = false;
			}
			else	//Not last -- increment the counter and onwards
			{
				//Increment the counter
				island_loop_index++;

				//Force the flag, just to be paranoid
				still_iterating_islands = true;
			}

			//Continue the while (or attempt to do so)
			continue;
		}

		if (powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix == nullptr)
		{
			powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix = (SPARSE*) gl_malloc(sizeof(SPARSE));

			//Make sure it worked
			if (powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix == nullptr)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Initiliaze it
			sparse_init(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix, powerflow_values->island_matrix_values[island_loop_index].size_Amatrix, 6*powerflow_values->island_matrix_values[island_loop_index].bus_count);
		}
		else if (powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed)	//If one of the above changed, we changed too
		{
			//Destroy the old version
			sparse_clear(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix);

			//Create a new 
			sparse_init(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix, powerflow_values->island_matrix_values[island_loop_index].size_Amatrix, 6*powerflow_values->island_matrix_values[island_loop_index].bus_count);
		}
		else
		{
			//Just clear it out
			sparse_reset(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix, 6*powerflow_values->island_matrix_values[island_loop_index].bus_count);
		}

		//integrate off diagonal components
		for (indexer=0; indexer<powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ*2; indexer++)
		{
			row = powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ[indexer].row_ind;
			col = powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ[indexer].col_ind;
			value = powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ[indexer].Y_value;
			sparse_add(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix, row, col, value, bus, bus_count, powerflow_values, island_loop_index);
		}

		//Integrate fixed portions of diagonal components
		for (indexer=0; indexer< (powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed*2); indexer++)
		{
			row = powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed[indexer].row_ind;
			col = powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed[indexer].col_ind;
			value = powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed[indexer].Y_value;
			sparse_add(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix, row, col, value, bus, bus_count, powerflow_values, island_loop_index);
		}

		//Only happens in TCIM
		if (NR_solver_algorithm == NRM_TCIM)
		{
			//Integrate the variable portions of the diagonal components
			for (indexer=0; indexer< (4*powerflow_values->island_matrix_values[island_loop_index].size_diag_update); indexer++)
			{
				row = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].row_ind;
				col = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].col_ind;
				value = powerflow_values->island_matrix_values[island_loop_index].Y_diag_update[indexer].Y_value;
				sparse_add(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix, row, col, value, bus, bus_count, powerflow_values, island_loop_index);
			}
		}

		//See if we want to dump out the matrix values
		if (NRMatDumpMethod != MD_NONE)
		{
			//Code to export the sparse matrix values - useful for debugging issues

			//Reset the flag
			something_has_been_output = false;

			//Check our frequency
			if ((NRMatDumpMethod == MD_ALL) || ((NRMatDumpMethod != MD_ALL) && (powerflow_values->island_matrix_values[island_loop_index].iteration_count == 0)))
			{
				//Open the text file - append now
				FPoutVal=fopen(MDFileName,"at");

				//See if we wanted references - Only do this once per call, regardless (keeps file size down)
				if (NRMatReferences && (powerflow_values->island_matrix_values[island_loop_index].iteration_count == 0))
				{
					//Print the index information
					if (NR_islands_detected > 1)
					{
						fprintf(FPoutVal,"Matrix Index information for this call in island:%d - start,stop,name\n",(island_loop_index+1));
					}
					else
					{
						fprintf(FPoutVal,"Matrix Index information for this call - start,stop,name\n");
					}

					for (indexer=0; indexer<bus_count; indexer++)
					{
						//Island check
						if (bus[indexer].island_number == island_loop_index)
						{
							//Extract the start/stop indices
							jindexer = 2*bus[indexer].Matrix_Loc;
							kindexer = jindexer + 2*powerflow_values->BA_diag[indexer].size - 1;

							//Print them out
							fprintf(FPoutVal,"%d,%d,%s\n",jindexer,kindexer,bus[indexer].name);

							//Set the flag
							something_has_been_output = true;
						}//End island check
					}

					//Add in a blank line so it looks pretty
					fprintf(FPoutVal,"\n");
				}//End print the references

				//Print the simulation time and iteration number - see how we're running
				if (deltatimestep_running == -1)	//QSTS
				{
					fprintf(FPoutVal,"Timestamp: %lld - Iteration %lld\n",gl_globalclock,powerflow_values->island_matrix_values[island_loop_index].iteration_count);
				}
				else
				{
					fprintf(FPoutVal,"Timestamp: %f (deltamode) - Iteration %lld\n",gl_globaldeltaclock,powerflow_values->island_matrix_values[island_loop_index].iteration_count);
				}

				//See if anything wrote out, or we're "all"
				if (something_has_been_output || (NRMatDumpMethod == MD_ALL))
				{
					//Print size - for parsing ease
					fprintf(FPoutVal,"Matrix Information - non-zero element count = %d\n",powerflow_values->island_matrix_values[island_loop_index].size_Amatrix);
					
					//Print the values - printed as "row index, column index, value"
					//This particular output is after they have been column sorted for the algorithm
					//Header
					fprintf(FPoutVal,"Matrix Information - row, column, value\n");

					//Null temp variable
					temp_element = nullptr;

					//Loop through the columns, extracting starting point each time
					for (jindexer=0; jindexer<powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix->ncols; jindexer++)
					{
						//Extract the column starting point
						temp_element = powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix->cols[jindexer];

						//Check for nulling
						if (temp_element != nullptr)
						{
							//Print this value
							fprintf(FPoutVal,"%d,%d,%f\n",temp_element->row_ind,jindexer,temp_element->value);

							//Loop
							while (temp_element->next != nullptr)
							{
								//Get next element
								temp_element = temp_element->next;

								//Repeat the print
								fprintf(FPoutVal,"%d,%d,%f\n",temp_element->row_ind,jindexer,temp_element->value);
							}
						}
						//If it is null, go next.  Implies we have an invalid matrix size, but that may be what we're looking for
					}//End sparse matrix traversion for dump

					//Print an extra line, so it looks nice for ALL/PERCALL
					fprintf(FPoutVal,"\n");

					//Output the RHS information, if desired
					if (NRMatRHSDump)
					{
						//Get the output size - double size, due to complex separation
						m = 2*powerflow_values->island_matrix_values[island_loop_index].total_variables;

						//Print the size - should be the matrix size, but put here for ease of use
						fprintf(FPoutVal,"Matrix RHS Information - %u elements\n",m);

						//Print a header - row information isn't really needed, but include, just for ease of viewing
						fprintf(FPoutVal,"RHS Information - row, value\n");

						//Loop through and output the RHS values
						for (jindexer=0; jindexer<m; jindexer++)
						{
							fprintf(FPoutVal,"%u,%f\n",jindexer,powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[jindexer]);
						}//End output RHS

						//Print an extra line, just for spacing for ALL/PERCALL
						fprintf(FPoutVal,"\n");
					}//end RHS dump, if desired
				}
				else //Nothing written - indicate as much
				{
					//Print the emptiness
					fprintf(FPoutVal,"Empty island (possibly powerflow-removed)\n\n");
				}

				//Close the file, we're done with it
				fclose(FPoutVal);

				//See if we were a "ONCE" - if so, deflag us
				if (NRMatDumpMethod == MD_ONCE)
				{
					//See if we're a multi-island -- if so, only do this on the last one
					if (NR_islands_detected > 1)
					{
						//See if we're the last one
						if (island_loop_index >= (NR_islands_detected - 1))
						{
							NRMatDumpMethod = MD_NONE;	//Flag to do no more
						}
						//Default else -- we're not the last one, so stay as MD_ONCE
					}
					else
					{
						NRMatDumpMethod = MD_NONE;	//Flag to do no more
					}
				}
			}//End Actual output
		}//End matrix dump desired

		///* Initialize parameters. */
		m = 2*powerflow_values->island_matrix_values[island_loop_index].total_variables;
		n = 2*powerflow_values->island_matrix_values[island_loop_index].total_variables;
		nnz = powerflow_values->island_matrix_values[island_loop_index].size_Amatrix;

		if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.a_LU == nullptr)	//First run
		{
			/* Set aside space for the arrays. */
			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.a_LU = (double *) gl_malloc(nnz *sizeof(double));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.a_LU==nullptr)
			{
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");
				/*  TROUBLESHOOT
				While attempting to allocate the memory for one of the SuperLU working matrices,
				an error was encountered and it was not allocated.  Please try again.  If it fails
				again, please submit your code and a bug report using the trac website.
				*/
			}
			
			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rows_LU = (int *) gl_malloc(nnz *sizeof(int));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rows_LU == nullptr)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.cols_LU = (int *) gl_malloc((n+1) *sizeof(int));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.cols_LU == nullptr)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			/* Create the right-hand side matrix B. */
			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU = (double *) gl_malloc(m *sizeof(double));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU == nullptr)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			if (matrix_solver_method==MM_SUPERLU)
			{
				///* Set up the arrays for the permutations. */
				curr_island_superLU_vars->perm_r = (int *) gl_malloc(m *sizeof(int));
				if (curr_island_superLU_vars->perm_r == nullptr)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				curr_island_superLU_vars->perm_c = (int *) gl_malloc(n *sizeof(int));
				if (curr_island_superLU_vars->perm_c == nullptr)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				//Set up storage pointers - single element, but need to be malloced for some reason
				curr_island_superLU_vars->A_LU.Store = (void *)gl_malloc(sizeof(NCformat));
				if (curr_island_superLU_vars->A_LU.Store == nullptr)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				curr_island_superLU_vars->B_LU.Store = (void *)gl_malloc(sizeof(DNformat));
				if (curr_island_superLU_vars->B_LU.Store == nullptr)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				//Populate these structures - A_LU matrix
				curr_island_superLU_vars->A_LU.Stype = SLU_NC;
				curr_island_superLU_vars->A_LU.Dtype = SLU_D;
				curr_island_superLU_vars->A_LU.Mtype = SLU_GE;
				curr_island_superLU_vars->A_LU.nrow = n;
				curr_island_superLU_vars->A_LU.ncol = m;

				//Populate these structures - B_LU matrix
				curr_island_superLU_vars->B_LU.Stype = SLU_DN;
				curr_island_superLU_vars->B_LU.Dtype = SLU_D;
				curr_island_superLU_vars->B_LU.Mtype = SLU_GE;
				curr_island_superLU_vars->B_LU.nrow = m;
				curr_island_superLU_vars->B_LU.ncol = 1;
			}
			else if (matrix_solver_method == MM_EXTERN)	//External routine
			{
				//Run allocation routine
				((void (*)(void *,unsigned int, unsigned int, bool))(LUSolverFcns.ext_alloc))(powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars,n,n,NR_admit_change);
			}
			else
			{
				GL_THROW("Invalid matrix solution method specified for NR solver!");
				//Defined elsewhere
			}

			//Update tracking variable
			powerflow_values->island_matrix_values[island_loop_index].prev_m = m;
		}
		else if (powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed)	//Something changed, we'll just destroy everything and start over
		{
			//Get rid of all of them first
			gl_free(powerflow_values->island_matrix_values[island_loop_index].matrices_LU.a_LU);
			gl_free(powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rows_LU);
			gl_free(powerflow_values->island_matrix_values[island_loop_index].matrices_LU.cols_LU);
			gl_free(powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU);

			if (matrix_solver_method==MM_SUPERLU)
			{
				//Free up superLU matrices
				gl_free(curr_island_superLU_vars->perm_r);
				gl_free(curr_island_superLU_vars->perm_c);
			}
			//Default else - don't care - destructions are presumed to be handled inside external LU's alloc function

			/* Set aside space for the arrays. - Copied from above */
			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.a_LU = (double *) gl_malloc(nnz *sizeof(double));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.a_LU==nullptr)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");
			
			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rows_LU = (int *) gl_malloc(nnz *sizeof(int));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rows_LU == nullptr)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.cols_LU = (int *) gl_malloc((n+1) *sizeof(int));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.cols_LU == nullptr)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			/* Create the right-hand side matrix B. */
			powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU = (double *) gl_malloc(m *sizeof(double));
			if (powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU == nullptr)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			if (matrix_solver_method==MM_SUPERLU)
			{
				///* Set up the arrays for the permutations. */
				curr_island_superLU_vars->perm_r = (int *) gl_malloc(m *sizeof(int));
				if (curr_island_superLU_vars->perm_r == nullptr)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				curr_island_superLU_vars->perm_c = (int *) gl_malloc(n *sizeof(int));
				if (curr_island_superLU_vars->perm_c == nullptr)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				//Update structures - A_LU matrix
				curr_island_superLU_vars->A_LU.Stype = SLU_NC;
				curr_island_superLU_vars->A_LU.Dtype = SLU_D;
				curr_island_superLU_vars->A_LU.Mtype = SLU_GE;
				curr_island_superLU_vars->A_LU.nrow = n;
				curr_island_superLU_vars->A_LU.ncol = m;

				//Update structures - B_LU matrix
				curr_island_superLU_vars->B_LU.Stype = SLU_DN;
				curr_island_superLU_vars->B_LU.Dtype = SLU_D;
				curr_island_superLU_vars->B_LU.Mtype = SLU_GE;
				curr_island_superLU_vars->B_LU.nrow = m;
				curr_island_superLU_vars->B_LU.ncol = 1;
			}
			else if (matrix_solver_method == MM_EXTERN)	//External routine
			{
				//Run allocation routine
				((void (*)(void *,unsigned int, unsigned int, bool))(LUSolverFcns.ext_alloc))(powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars,n,n,NR_admit_change);
			}
			else
			{
				GL_THROW("Invalid matrix solution method specified for NR solver!");
				//Defined elsewhere
			}

			//Update tracking variable
			powerflow_values->island_matrix_values[island_loop_index].prev_m = m;
		}
		else if (powerflow_values->island_matrix_values[island_loop_index].prev_m != m)	//Non-reallocing size change occurred
		{
			if (matrix_solver_method==MM_SUPERLU)
			{
				//Update relevant portions
				curr_island_superLU_vars->A_LU.nrow = n;
				curr_island_superLU_vars->A_LU.ncol = m;

				curr_island_superLU_vars->B_LU.nrow = m;
			}
			else if (matrix_solver_method == MM_EXTERN)	//External routine - call full reallocation, just in case
			{
				//Run allocation routine
				((void (*)(void *,unsigned int, unsigned int, bool))(LUSolverFcns.ext_alloc))(powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars,n,n,NR_admit_change);
			}
			else
			{
				GL_THROW("Invalid matrix solution method specified for NR solver!");
				//Defined elsewhere
			}

			//Update tracking variable
			powerflow_values->island_matrix_values[island_loop_index].prev_m = m;
		}

#ifndef MT
		if (matrix_solver_method==MM_SUPERLU)
		{
			/* superLU sequential options*/
			set_default_options ( &options );
		}
		//Default else - not superLU
#endif
		
		sparse_tonr(powerflow_values->island_matrix_values[island_loop_index].Y_Amatrix, &powerflow_values->island_matrix_values[island_loop_index].matrices_LU);
		powerflow_values->island_matrix_values[island_loop_index].matrices_LU.cols_LU[n] = nnz ;// number of non-zeros;

		//Determine how to populate the rhs vector
		if (mesh_imped_vals == nullptr)	//Normal powerflow, copy in the values
		{
			for (temp_index_c=0;temp_index_c<m;temp_index_c++)
			{ 
				powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU[temp_index_c] = powerflow_values->island_matrix_values[island_loop_index].current_RHS_NR[temp_index_c];
			}
		}
		//Default else -- it is NULL - zero it and "populate it" below

		if (matrix_solver_method==MM_SUPERLU)
		{
			////* Create Matrix A in the format expected by Super LU.*/
			//Populate the matrix values (temporary value)
			Astore = (NCformat*)curr_island_superLU_vars->A_LU.Store;
			Astore->nnz = nnz;
			Astore->nzval = powerflow_values->island_matrix_values[island_loop_index].matrices_LU.a_LU;
			Astore->rowind = powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rows_LU;
			Astore->colptr = powerflow_values->island_matrix_values[island_loop_index].matrices_LU.cols_LU;
			
			// Create right-hand side matrix B in format expected by Super LU
			//Populate the matrix (temporary values)
			Bstore = (DNformat*)curr_island_superLU_vars->B_LU.Store;
			Bstore->lda = m;
			Bstore->nzval = powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU;

			//See how to call the function - if normal mode or not
			//**************************** How will this work with islanding stuff !?!?!?!? ****************************
			if (mesh_imped_vals != nullptr)
			{
				//Start with "failure" option on the structure
				mesh_imped_vals->return_code = 0;

				//Determine the base index (error check)
				if (mesh_imped_vals->NodeRefNum > 0)
				{
					tempa = 2*bus[mesh_imped_vals->NodeRefNum].Matrix_Loc;	//These are already separated into real/imag, so the base is 2x
				}
				else	//Error
				{
					gl_error("solver_nr: Node reference for mesh fault calculation was invalid -- it may be a child");
					/*  TROUBLESHOOT
					While attempting to compute the equivalent impedance at a mesh faulted node, an invalid node reference
					was found.  This is likely an internal error to the code -- submit your code and a bug report via the
					tracking website.
					*/

					//Set the flags
					mesh_imped_vals->return_code = 0;
					*bad_computations = true;

					//Deflag the island locker, though not really needed
					NR_solver_working = false;

					//Return a 0 to flag
					return 0;
				}

				//Determine our phasing size - what entries to pull - double for the complex notation
				temp_size = 2*powerflow_values->BA_diag[mesh_imped_vals->NodeRefNum].size;

				//Check for error - size of zero
				if (temp_size == 0)
				{
					gl_error("solver_nr: Mesh fault impedance extract failed - invalid node");
					/*  TROUBLESHOOT
					While attempting to extract the mesh fault impedance value, a node with either
					invalid phasing or no phases (already removed) was somehow flagged.  Please check
					your file and try again.  If the error persists, please submit your code
					and a bug report via the ticketing system.
					*/

					//Flags
					*bad_computations = true;
					mesh_imped_vals->return_code = 0;

					//Deflag the island locker, though not really needed
					NR_solver_working = false;

					//Exit
					return 0;
				}
				//Default else -- okay

				//Clear out the output matrix first -- assumes it is 3x3, regardless
				for (jindex=0; jindex<9; jindex++)
				{
					mesh_imped_vals->z_matrix[jindex] = gld::complex(0.0,0.0);
				}

				//Zero the double matrix - the big storage (6x6 - re & im split)
				for (jindex=0; jindex<6; jindex++)
				{
					for (kindex=0; kindex<6; kindex++)
					{
						temp_z_store[jindex][kindex] = 0.0;
					}
				}

				//Overall loop - for number of phases
				for (kindex=0; kindex<temp_size; kindex++)
				{

					//Start by zeroing the "solution" vector
					for (temp_index_c=0;temp_index_c<m;temp_index_c++)
					{ 
						powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU[temp_index_c] = 0.0;
					}

					//"Identity"-ize the real part of the current index
					powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU[tempa + kindex] = 1.0;

					//Do a solution to get this entry (copied from below - includes "destructors"
#ifdef MT
					//superLU_MT commands

					//Populate perm_c
					get_perm_c(1, &curr_island_superLU_vars->A_LU, curr_island_superLU_vars->perm_c);

					//Solve the system 
					pdgssv(NR_superLU_procs, &curr_island_superLU_vars->A_LU, curr_island_superLU_vars->perm_c, curr_island_superLU_vars->perm_r, &L_LU, &U_LU, &curr_island_superLU_vars->B_LU, &info);

					/* De-allocate storage - superLU matrix types must be destroyed at every iteration, otherwise they balloon fast (65 MB norma becomes 1.5 GB) */
					//superLU_MT commands
					Destroy_SuperNode_SCP(&L_LU);
					Destroy_CompCol_NCP(&U_LU);
#else
					//sequential superLU

					StatInit ( &stat );

					// solve the system
					dgssv(&options, &curr_island_superLU_vars->A_LU, curr_island_superLU_vars->perm_c, curr_island_superLU_vars->perm_r, &L_LU, &U_LU, &curr_island_superLU_vars->B_LU, &stat, &info);

					/* De-allocate storage - superLU matrix types must be destroyed at every iteration, otherwise they balloon fast (65 MB norma becomes 1.5 GB) */
					//sequential superLU commands
					Destroy_SuperNode_Matrix( &L_LU );
					Destroy_CompCol_Matrix( &U_LU );
					StatFree ( &stat );
#endif
					//Crude superLU checks - see if it is mission accomplished or not
					if (info != 0)	//Failed inversion, for various reasons
					{
						gl_error("solver_nr: superLU failed mesh fault matrix inversion with code %d",info);
						/*  TROUBLESHOOT
						superLU failed to invert the equivalent impedance matrix for the mesh fault calculation
						method.  Please try again and make sure your system is valid.  If the error persists, please
						submit your code and a bug report via the ticketing system.
						*/

						//Flag as bad
						*bad_computations = true;
						mesh_imped_vals->return_code = 0;

						//Deflag the island locker, though not really needed
						NR_solver_working = false;

						//Exit
						return 0;
					}
					//Default else, must have converged!

					//Map up the solution vector
					sol_LU = (double*) ((DNformat*) curr_island_superLU_vars->B_LU.Store)->nzval;

					//Extract out this column into the temporary matrix
					for (jindex=0; jindex<temp_size; jindex++)
					{
						temp_z_store[jindex][kindex] = sol_LU[tempa+jindex];
					}
				}//End "phase loop" - impedance components are ready

				//Extract the values - do with a massive case/switch, just because
				switch(bus[mesh_imped_vals->NodeRefNum].phases & 0x07) {
					case 0x01:	//C
						{
							mesh_imped_vals->z_matrix[8] = gld::complex(temp_z_store[1][0],temp_z_store[1][1]);
							break;
						}
					case 0x02:	//B
						{
							mesh_imped_vals->z_matrix[4] = gld::complex(temp_z_store[1][0],temp_z_store[1][1]);
							break;
						}
					case 0x03:	//BC
						{
							//BB and BC portion
							mesh_imped_vals->z_matrix[4] = gld::complex(temp_z_store[2][0],temp_z_store[2][2]);
							mesh_imped_vals->z_matrix[5] = gld::complex(temp_z_store[2][1],temp_z_store[2][3]);

							//CB and CC portion
							mesh_imped_vals->z_matrix[7] = gld::complex(temp_z_store[3][0],temp_z_store[3][2]);
							mesh_imped_vals->z_matrix[8] = gld::complex(temp_z_store[3][1],temp_z_store[3][3]);
							break;
						}
					case 0x04:	//A
						{
							mesh_imped_vals->z_matrix[0] = gld::complex(temp_z_store[1][0],temp_z_store[1][1]);
							break;
						}
					case 0x05:	//AC
						{
							//AA and AC portion
							mesh_imped_vals->z_matrix[0] = gld::complex(temp_z_store[2][0],temp_z_store[2][2]);
							mesh_imped_vals->z_matrix[2] = gld::complex(temp_z_store[2][1],temp_z_store[2][3]);

							//CA and CC portion
							mesh_imped_vals->z_matrix[6] = gld::complex(temp_z_store[3][0],temp_z_store[3][2]);
							mesh_imped_vals->z_matrix[8] = gld::complex(temp_z_store[3][1],temp_z_store[3][3]);
							break;
						}
					case 0x06:	//AB
						{
							//AA and AB portion
							mesh_imped_vals->z_matrix[0] = gld::complex(temp_z_store[2][0],temp_z_store[2][2]);
							mesh_imped_vals->z_matrix[1] = gld::complex(temp_z_store[2][1],temp_z_store[2][3]);

							//BA and BB portion
							mesh_imped_vals->z_matrix[3] = gld::complex(temp_z_store[3][0],temp_z_store[3][2]);
							mesh_imped_vals->z_matrix[4] = gld::complex(temp_z_store[3][1],temp_z_store[3][3]);
							break;
						}
					case 0x07:	//ABC
						{
							//A row
							mesh_imped_vals->z_matrix[0] = gld::complex(temp_z_store[3][0],temp_z_store[3][3]);
							mesh_imped_vals->z_matrix[1] = gld::complex(temp_z_store[3][1],temp_z_store[3][4]);
							mesh_imped_vals->z_matrix[2] = gld::complex(temp_z_store[3][2],temp_z_store[3][5]);

							//B row
							mesh_imped_vals->z_matrix[3] = gld::complex(temp_z_store[4][0],temp_z_store[4][3]);
							mesh_imped_vals->z_matrix[4] = gld::complex(temp_z_store[4][1],temp_z_store[4][4]);
							mesh_imped_vals->z_matrix[5] = gld::complex(temp_z_store[4][2],temp_z_store[4][5]);

							//C row
							mesh_imped_vals->z_matrix[6] = gld::complex(temp_z_store[5][0],temp_z_store[5][3]);
							mesh_imped_vals->z_matrix[7] = gld::complex(temp_z_store[5][1],temp_z_store[5][4]);
							mesh_imped_vals->z_matrix[8] = gld::complex(temp_z_store[5][2],temp_z_store[5][5]);
							break;
						}
					default:	//Not sure how we got here
						{
							gl_error("solver_nr: Mesh fault impedance extract failed - invalid node");
							//Defined above, same message

							//Flags
							*bad_computations = true;
							mesh_imped_vals->return_code = 0;

							//Deflag the island locker, though not really needed
							NR_solver_working = false;

							//Exit
							return 0;
						}
				}//end phase switch/case

				//Flag as success
				*bad_computations = false;
				mesh_imped_vals->return_code = 1;

				//Exit
				return 1;	//Non-zero, so success (manual checks outside though)
			}//End "just mesh impedance calculations"
			else	//Nulled, "normal" powerflow
			{
#ifdef MT
				//superLU_MT commands

				//Populate perm_c
				get_perm_c(1, &curr_island_superLU_vars->A_LU, curr_island_superLU_vars->perm_c);

				//Solve the system
				pdgssv(NR_superLU_procs, &curr_island_superLU_vars->A_LU, curr_island_superLU_vars->perm_c, curr_island_superLU_vars->perm_r, &L_LU, &U_LU, &curr_island_superLU_vars->B_LU, &powerflow_values->island_matrix_values[island_loop_index].solver_info);
#else
				//sequential superLU

				StatInit ( &stat );

				// solve the system
				dgssv(&options, &curr_island_superLU_vars->A_LU, curr_island_superLU_vars->perm_c, curr_island_superLU_vars->perm_r, &L_LU, &U_LU, &curr_island_superLU_vars->B_LU, &stat, &powerflow_values->island_matrix_values[island_loop_index].solver_info);
#endif

				sol_LU = (double*) ((DNformat*) curr_island_superLU_vars->B_LU.Store)->nzval;
			}
		}
		else if (matrix_solver_method==MM_EXTERN)
		{
			//General error check right now -- mesh fault current may not work properly
			if (mesh_imped_vals != nullptr)
			{
				gl_error("solver_nr: Mesh impedance attempted from unsupported LU solver");
				/* TROUBLESHOOT
				The mesh fault current impedance "pull" was selected, but for the external LU
				solver.  This is unsupported at this time.  To use the mesh fault current capabilities,
				please use the superLU solver.
				*/

				//Set return code
				mesh_imped_vals->return_code = 2;

				//Perform the clean up - external LU destructor routing
				((void (*)(void *, bool))(LUSolverFcns.ext_destroy))(powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars,powerflow_values->island_matrix_values[island_loop_index].new_iteration_required);

				//Flag bad computations, just because
				*bad_computations = true;

				//Deflag us, even though that may be moot here
				NR_solver_working = false;

				//Exit
				return 0;
			}
			//Default else -- not mesh fault mode, so go like normal

			//Call the solver
			powerflow_values->island_matrix_values[island_loop_index].solver_info = ((int (*)(void *,NR_SOLVER_VARS *, unsigned int, unsigned int))(LUSolverFcns.ext_solve))(powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars,&powerflow_values->island_matrix_values[island_loop_index].matrices_LU,n,1);

			//Point the solution to the proper place
			sol_LU = powerflow_values->island_matrix_values[island_loop_index].matrices_LU.rhs_LU;
		}
		else
		{
			GL_THROW("Invalid matrix solution method specified for NR solver!");
			/*  TROUBLESHOOT
			An invalid matrix solution method was selected for the Newton-Raphson solver method.
			Valid options are the superLU solver or an external solver.  Please select one of these methods.
			*/
		}

		//Update bus voltages - check convergence while we're here
		powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge = 0;

		temp_index = -1;
		temp_index_b = -1;
		powerflow_values->island_matrix_values[island_loop_index].new_iteration_required = false;	//Reset iteration requester flag - defaults to not needing another

		for (indexer=0; indexer<bus_count; indexer++)
		{
			//See if we're part of the relevant island
			if (bus[indexer].island_number == island_loop_index)
			{
				//Avoid swing bus updates on normal runs
				if ((bus[indexer].type == 0) || ((bus[indexer].type > 1) && !bus[indexer].swing_functions_enabled))
				{
					//Figure out the offset we need to be for each phase
					if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase
					{
						//TCIM convergence check
						if (NR_solver_algorithm == NRM_TCIM)
						{
							//Pull the two updates (assume split-phase is always 2)
							DVConvCheck[0]=gld::complex(sol_LU[2*bus[indexer].Matrix_Loc],sol_LU[(2*bus[indexer].Matrix_Loc+2)]);
							DVConvCheck[1]=gld::complex(sol_LU[(2*bus[indexer].Matrix_Loc+1)],sol_LU[(2*bus[indexer].Matrix_Loc+3)]);
							bus[indexer].V[0] += DVConvCheck[0];
							bus[indexer].V[1] += DVConvCheck[1];	//Negative due to convention
						}
						else if (NR_solver_algorithm == NRM_FPI)
						{
							//Pull the two updates (assume split-phase is always 2)
							currVoltConvCheck[0] = gld::complex(sol_LU[2*bus[indexer].Matrix_Loc],sol_LU[(2*bus[indexer].Matrix_Loc+2)]);
							currVoltConvCheck[1] = gld::complex(sol_LU[(2*bus[indexer].Matrix_Loc+1)],sol_LU[(2*bus[indexer].Matrix_Loc+3)]);

							//Convergence values
							DVConvCheck[0]=bus[indexer].V[0] - currVoltConvCheck[0];
							DVConvCheck[1]=bus[indexer].V[1] - currVoltConvCheck[1];

							//Update all the values
							bus[indexer].V[0] = currVoltConvCheck[0];
							bus[indexer].V[1] = currVoltConvCheck[1];
						}
						//Default else covered elsewhere
						
						//Pull off the magnitude (no sense calculating it twice)
						CurrConvVal=DVConvCheck[0].Mag();
						if (CurrConvVal > powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge)	//Update our convergence check if it is bigger
							powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge=CurrConvVal;

						if (CurrConvVal > bus[indexer].max_volt_error)	//Check for convergence
							powerflow_values->island_matrix_values[island_loop_index].new_iteration_required=true;								//Flag that a new iteration must occur

						CurrConvVal=DVConvCheck[1].Mag();
						if (CurrConvVal > powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge)	//Update our convergence check if it is bigger
							powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge=CurrConvVal;

						if (CurrConvVal > bus[indexer].max_volt_error)	//Check for convergence
							powerflow_values->island_matrix_values[island_loop_index].new_iteration_required=true;								//Flag that a new iteration must occur
					}//end split phase update
					else										//Not split phase
					{
						for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)	//parse through the phases
						{
							switch(bus[indexer].phases & 0x07) {
								case 0x01:	//C
									{
										temp_index=0;
										temp_index_b=2;
										break;
									}
								case 0x02:	//B
									{
										temp_index=0;
										temp_index_b=1;
										break;
									}
								case 0x03:	//BC
									{
										if (jindex==0)	//B
										{
											temp_index=0;
											temp_index_b=1;
										}
										else			//C
										{
											temp_index=1;
											temp_index_b=2;
										}

										break;
									}
								case 0x04:	//A
									{
										temp_index=0;
										temp_index_b=0;
										break;
									}
								case 0x05:	//AC
									{
										if (jindex==0)	//A
										{
											temp_index=0;
											temp_index_b=0;
										}
										else			//C
										{
											temp_index=1;
											temp_index_b=2;
										}
										break;
									}
								case 0x06:	//AB
								case 0x07:	//ABC
									{
										temp_index = jindex;
										temp_index_b = jindex;
										break;
									}
							}//end phase switch/case

							if ((temp_index==-1) || (temp_index_b==-1))
							{
								GL_THROW("NR: An error occurred indexing voltage updates");
								/*  TROUBLESHOOT
								While attempting to create the voltage update indices for the
								Newton-Raphson solver, an error was encountered.  Please submit
								your code and a bug report using the trac website.
								*/
							}

							if (NR_solver_algorithm == NRM_TCIM)
							{
								DVConvCheck[jindex]=gld::complex(sol_LU[(2*bus[indexer].Matrix_Loc+temp_index)],sol_LU[(2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size+temp_index)]);
								bus[indexer].V[temp_index_b] += DVConvCheck[jindex];
							}
							else if (NR_solver_algorithm == NRM_FPI)
							{
								//Pull the update
								currVoltConvCheck[jindex] = gld::complex(sol_LU[(2*bus[indexer].Matrix_Loc+temp_index)],sol_LU[(2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size+temp_index)]);

								//Calculate convergence
								DVConvCheck[jindex] = bus[indexer].V[temp_index_b] - currVoltConvCheck[jindex];

								//Update
								bus[indexer].V[temp_index_b] = currVoltConvCheck[jindex];
							}
							//Default else handled elsewhere

							//Pull off the magnitude (no sense calculating it twice)
							CurrConvVal=DVConvCheck[jindex].Mag();
							if (CurrConvVal > bus[indexer].max_volt_error)	//Check for convergence
								powerflow_values->island_matrix_values[island_loop_index].new_iteration_required=true;								//Flag that a new iteration must occur

							if (CurrConvVal > powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge)	//See if the current differential is the largest found so far or not
								powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge = CurrConvVal;	//It is, store it

						}//End For loop for phase traversion
					}//End not split phase update
				}//end if not swing
				//Default else -- this must be the swing and in a valid interval
			}//End "in the island"
			//Default else - not in the island, skip it
		}//End bus traversion

		//Perform saturation current update/convergence check
		//******************** FIGURE OUT HOW TO DO THIS BETTER - This is very inefficient!***********************//
		if (enable_inrush_calculations && (deltatimestep_running > 0))	//Don't even both with this if inrush not on
		{
			//Start out assuming the convergence is true (reiter is false)
			powerflow_values->island_matrix_values[island_loop_index].SaturationMismatchPresent = false;

			//Loop through all branches -- inefficient, but meh
			for (indexer=0; indexer<branch_count; indexer++)
			{
				//See if this branch is associated with this island
				if (branch[indexer].island_number == island_loop_index)
				{
					//See if the function exists
					if (branch[indexer].ExtraDeltaModeFunc != nullptr)
					{
						//Call the function
						func_result_val = ((int (*)(OBJECT *,bool *))(*branch[indexer].ExtraDeltaModeFunc))(branch[indexer].obj,&(powerflow_values->island_matrix_values[island_loop_index].SaturationMismatchPresent));

						//Make sure it worked
						if (func_result_val != 1)
						{
							GL_THROW("Extra delta update failed for device %s",branch[indexer].name ? branch[indexer].name : "Unnamed");
							/*  TROUBLESHOOT
							While attempting to perform the extra deltamode update, something failed.  Please try again.
							If the error persists, please submit your code and a bug report via the ticketing system.
							*/
						}
						//Default else -- it worked, keep going
					}
					//Default else - is NULL
				}
				//Default else -- not part of this island, so don't mess with it
			}//End inefficient branch traversion
		}//End in-rush and deltamode running

		//Reset an arbitrary flag - if we were FPI and initing, change the value
		proceed_flag = true;

		//Additional checks for special modes - only needs to happen in first dynamics powerflow
		if (powerflow_type == PF_DYNINIT)
		{
			//See what "mode" we're in
			if (!powerflow_values->island_matrix_values[island_loop_index].new_iteration_required)	//Overall powerflow has converged
			{
				if (!powerflow_values->island_matrix_values[island_loop_index].swing_converged)	//See if deltaI_sym is being met
				{
					//See which message to print
					if (NR_islands_detected > 1)
					{
						//Give a verbose message, just cause
						gl_verbose("NR_Solver: swing bus failed balancing criteria on island %d - making it a PQ for future dynamics",(island_loop_index+1));
					}
					else	//Single system
					{
						//Give a verbose message, just cause
						gl_verbose("NR_Solver: swing bus failed balancing criteria - making it a PQ for future dynamics");
					}

					//See if we're still swing_is_a_swing mode
					if (powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing)
					{
						//Deflag us
						powerflow_values->island_matrix_values[island_loop_index].swing_is_a_swing = false;

						//Loop through and deflag appropriate swing buses
						for (indexer=0; indexer<bus_count; indexer++)
						{
							//See if we're in the "island of interest"
							if (bus[indexer].island_number == island_loop_index)
							{
								//See if we're a swing-flagged bus
								if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)
								{
									//See if we're "generator ready"
									if (*bus[indexer].dynamics_enabled && (bus[indexer].DynCurrent != nullptr))
									{
										//Deflag us back to "PQ" status
										bus[indexer].swing_functions_enabled = false;

										//If FPI, flag an admittance update (because now the SWING exists again)
										if (NR_solver_algorithm == NRM_FPI)
										{
											//Paranoia flag, just in case
											NR_admit_change = true;

											//Arbitrary flag - skips the realloc override below, since that will break things
											proceed_flag = false;
										}

										//Force a reiteration
										powerflow_values->island_matrix_values[island_loop_index].new_iteration_required=true;
									}
								}
								//Default else -- normal bus
							}//End Island check
							//Default else -- not in the current relevant island
						}//End bus traversion loop
					}
					//Default else - if we're out of symmetry bounds, but already hit close, good enough
				}
				//Defaulted else - we're converged on all fronts, huzzah!
			}
			//Default else - still need to iterate
		}//End special convergence criterion

		//Check Saturation mismatch 
		if (powerflow_values->island_matrix_values[island_loop_index].SaturationMismatchPresent)
		{
			//Force a reiter
			powerflow_values->island_matrix_values[island_loop_index].new_iteration_required = true;
		}
		//Default else -- either no saturation, or it has converged - leave it be

		//Check Norton equivalent mismatch
		if (powerflow_values->island_matrix_values[island_loop_index].NortonCurrentMismatchPresent)
		{
			//Force a reiteration for this too
			powerflow_values->island_matrix_values[island_loop_index].new_iteration_required = true;
		}

		//Check conditions
		if (proceed_flag)
		{
			//Turn off reallocation flag no matter what
			powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed = false;
		}
		else
		{
			//Call the admittance update code
			NR_admittance_update(bus_count,bus,branch_count,branch,powerflow_values,powerflow_type);

			//Force it on, just in case (probably was just de-SWINGed)
			powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed = true;
		}

		if (matrix_solver_method==MM_SUPERLU)
		{
			/* De-allocate storage - superLU matrix types must be destroyed at every iteration, otherwise they balloon fast (65 MB norma becomes 1.5 GB) */
#ifdef MT
			//superLU_MT commands
			Destroy_SuperNode_SCP(&L_LU);
			Destroy_CompCol_NCP(&U_LU);
#else
			//sequential superLU commands
			Destroy_SuperNode_Matrix( &L_LU );
			Destroy_CompCol_Matrix( &U_LU );
			StatFree ( &stat );
#endif
		}
		else if (matrix_solver_method==MM_EXTERN)
		{
			//Call destruction routine
			((void (*)(void *, bool))(LUSolverFcns.ext_destroy))(powerflow_values->island_matrix_values[island_loop_index].LU_solver_vars,powerflow_values->island_matrix_values[island_loop_index].new_iteration_required);
		}
		else	//Not sure how we get here
		{
			GL_THROW("Invalid matrix solution method specified for NR solver!");
			//Defined above
		}

		//Final check -- see if the info result wasn't zero -- if so, deflag us no matter what here
		if (powerflow_values->island_matrix_values[island_loop_index].solver_info != 0)
		{
			powerflow_values->island_matrix_values[island_loop_index].new_iteration_required = false;
		}

		//Set the flag to not continue -- all should be handled below, this is just logic paranoia
		proceed_to_next_island = false;

		//Determine how we got here and what the next steps are
		if (powerflow_values->island_matrix_values[island_loop_index].new_iteration_required)	//We think we need a new one
		{
			//Make sure we still can iterate - if we hit the iteration limit, we can't
			if (powerflow_values->island_matrix_values[island_loop_index].iteration_count>=NR_iteration_limit)
			{
				powerflow_values->island_matrix_values[island_loop_index].return_code = -(powerflow_values->island_matrix_values[island_loop_index].iteration_count);
	
				if (NR_islands_detected > 1)
				{
					gl_verbose("Max solver mismatch of failed solution %f on island %d\n",powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge,(island_loop_index+1));
				}
				else
				{
					gl_verbose("Max solver mismatch of failed solution %f\n",powerflow_values->island_matrix_values[island_loop_index].max_mismatch_converge);
				}

				//We got here, we're done with this island
				proceed_to_next_island = true;
			}
			else	//Normal iteration procedure - update the counter
			{
				powerflow_values->island_matrix_values[island_loop_index].iteration_count++;

				//Set the overall progression flag
				proceed_to_next_island = false;
			}
		}
		else	//Should be done
		{
			//All paths lead to the next island/completion
			proceed_to_next_island = true;

			//Report how we think we're done
			if (powerflow_values->island_matrix_values[island_loop_index].solver_info == 0)	//It was a convergence exit
			{
				//See if we're multi-islanded or not
				if (NR_islands_detected > 1)
				{
					gl_verbose("Power flow calculation for island %d converges at Iteration %d \n",(island_loop_index+1),(powerflow_values->island_matrix_values[island_loop_index].iteration_count+1));
				}
				else	//Singular
				{
					gl_verbose("Power flow calculation converges at Iteration %d \n",(powerflow_values->island_matrix_values[island_loop_index].iteration_count+1));
				}

				//Store the value as the return code
				powerflow_values->island_matrix_values[island_loop_index].return_code = powerflow_values->island_matrix_values[island_loop_index].iteration_count;
			}
			else
			{
				//Write out the "failure"

				//See which version to write
				if (NR_islands_detected > 1)
				{
					//For superLU - 2 = singular matrix it appears - positive values = process errors (singular, etc), negative values = input argument/syntax error
					if (matrix_solver_method==MM_SUPERLU)
					{
						gl_verbose("superLU failed out of island %d with return value %d",(island_loop_index+1),powerflow_values->island_matrix_values[island_loop_index].solver_info);
					}
					else if (matrix_solver_method==MM_EXTERN)
					{
						gl_verbose("External LU solver failed out of island %d with return value %d",(island_loop_index+1),powerflow_values->island_matrix_values[island_loop_index].solver_info);
					}
					//Defaulted else - shouldn't exist (or make it this far), but if it does, we're failing anyways
				}
				else	//Singular
				{
					//For superLU - 2 = singular matrix it appears - positive values = process errors (singular, etc), negative values = input argument/syntax error
					if (matrix_solver_method==MM_SUPERLU)
					{
						gl_verbose("superLU failed out with return value %d",powerflow_values->island_matrix_values[island_loop_index].solver_info);
					}
					else if (matrix_solver_method==MM_EXTERN)
					{
						gl_verbose("External LU solver failed out with return value %d",powerflow_values->island_matrix_values[island_loop_index].solver_info);
					}
					//Defaulted else - shouldn't exist (or make it this far), but if it does, we're failing anyways
				}

				//Store the return value as the failure
				powerflow_values->island_matrix_values[island_loop_index].return_code = 0;
			}
		}

		//Overall progression flag
		if (proceed_to_next_island)
		{
			//See if we need to continue
			if (island_loop_index >= (NR_islands_detected - 1))	//We're the last one
			{
				//Set the flag to be done
				still_iterating_islands = false;
			}
			else	//Not last -- increment the counter and onwards
			{
				//Increment the counter
				island_loop_index++;

				//Force the flag, just to be paranoid
				still_iterating_islands = true;
			}
		}
	}//End iteration still needed while - indexed on island_loop_index

	//Figure out the exit return values -- make them "worst case", so if all failed, return "bad_computations"
	//If at least one passes, just return the max iteration count
	*bad_computations = true;
	return_value_for_solver_NR = 0;

	//Loop through the islands and figure this out
	for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
	{
		if (powerflow_values->island_matrix_values[island_loop_index].return_code >= 0)	//Success or really bad failure
		{
			//See which one it is
			if (!powerflow_values->island_matrix_values[island_loop_index].new_iteration_required)	//We converged to get here!
			{
				*bad_computations = false;	//Deflag us

				//See if we're the biggest iteration limit so far - assuming no one is reiterating
				if (return_value_for_solver_NR >= 0)
				{
					if (powerflow_values->island_matrix_values[island_loop_index].iteration_count > return_value_for_solver_NR)
					{
						return_value_for_solver_NR = powerflow_values->island_matrix_values[island_loop_index].iteration_count;
					}
				}
				//Default else -- someone is already negative, so just accept it
			}
			else if (NR_island_fail_method)	//Really bad failure, but let's see if we're in "special handling mode"
			{
				*bad_computations = false;	//Deflag us, so it keeps going

				//Set the return value to the iteration limit (negative)
				//No checks necessary -- this will just rail, regardless
				return_value_for_solver_NR = -NR_iteration_limit;

				if (fault_check_object == nullptr)	//Make sure multi-island support is supported!
				{
					GL_THROW("NR: Multi-island handing without fault_check is not permitted");
					/*  TROUBLESHOOT
					The NR_island_failure_handled flag requires a fault_check object to be present.  Please add one to your GLM
					to continue.
					*/
				}
				else
				{
					//Call the "removal" routine - map it
					temp_fxn_val = (FUNCTIONADDR)(gl_get_function(fault_check_object,"island_removal_function"));

					//Make sure it was found
					if (temp_fxn_val == nullptr)
					{
						GL_THROW("NR: Unable to map island removal function");
						/*  TROUBLESHOOT
						While attempting to map the island removal function, an error was encountered.  Please
						try again.  If the error persists, please submit your code and a report via the issues
						system.
						*/
					}

					//Call the function
					call_return_status = ((STATUS (*)(OBJECT *,int))(temp_fxn_val))(fault_check_object,island_loop_index);

					//Make sure it worked
					if (call_return_status != SUCCESS)
					{
						GL_THROW("NR: Failed to remove island %d from the powerflow",(island_loop_index+1));
						/*  TROUBLESHOOT
						While attempting to remove an island from the powerflow, an error occurred.  Please try again.
						If the error persists, please submit your code and a report via the issues system.
						*/
					}
					//If we succeeded, good to go!
				}//End fault_check present
			}
			//Default else -- a bad failure we just want to ignore
		}
		else	//must be negative -- Not quite a full failure, just "something didn't converge"
		{
			*bad_computations = false;	//Deflag us

			//See which mode we're in
			if (NR_island_fail_method)	//See if we're in "special island mode" or not
			{
				//Set the return value to the iteration limit (negative)
				//No checks necessary -- this will just rail, regardless
				return_value_for_solver_NR = -NR_iteration_limit;

				if (fault_check_object == nullptr)	//Make sure multi-island support is supported!
				{
					GL_THROW("NR: Multi-island handing without fault_check is not permitted");
					//Defined above
				}
				else
				{
					//Call the "removal" routine - map it
					temp_fxn_val = (FUNCTIONADDR)(gl_get_function(fault_check_object,"island_removal_function"));

					//Make sure it was found
					if (temp_fxn_val == nullptr)
					{
						GL_THROW("NR: Unable to map island removal function");
						//Defined above
					}

					//Call the function
					call_return_status = ((STATUS (*)(OBJECT *,int))(temp_fxn_val))(fault_check_object,island_loop_index);

					//Make sure it worked
					if (call_return_status != SUCCESS)
					{
						GL_THROW("NR: Failed to remove island %d from the powerflow",(island_loop_index+1));
						//Defined above
					}
					//If we succeeded, good to go!
				}//End fault_check present
			}
			else	//Nope, just handle like "normal"
			{
				//See if we're the first negative one - if so, populate us (should just be NR limit)
				if (powerflow_values->island_matrix_values[island_loop_index].return_code < return_value_for_solver_NR)
				{
					return_value_for_solver_NR = powerflow_values->island_matrix_values[island_loop_index].return_code;
				}
			}
		}
		//Default else - it was a failure, just keep going
	}

	//Deflag the "island locker"
	NR_solver_working = false;

	//Check bad computations
	if (*bad_computations)
	{
		//Return 0 - just arbitrary here
		return 0;
	}
	else
	{
		//Return the maximum iteration count
		return return_value_for_solver_NR;
	}
}

//Performs the admittance matrix update/reformulation
//Functionalized so SWING-swap in FPI for dynamics can reallocate things
void NR_admittance_update(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, NR_SOLVER_STRUCT *powerflow_values, NRSOLVERMODE powerflow_type)
{
	//Temp variables - see main solver_nr for details
	int island_index_val;
	int island_loop_index;
	unsigned int indexer, tempa, tempb, jindexer, kindexer;
	char jindex, kindex;
	gld::complex tempY[3][3];
	gld::complex Temp_Ad_A[3][3];
	gld::complex Temp_Ad_B[3][3];
	unsigned char phase_worka, phase_workb, phase_workc, phase_workd, phase_worke;
	char temp_index, temp_index_b;
	bool Full_Mat_A, Full_Mat_B;
	char temp_size, temp_size_b, temp_size_c;
	char mat_temp_index;
	STATUS call_return_status;


	if (NR_admit_change)	//If an admittance update was detected, fix it
	{
		//Initialize the tracker variable inside each island, just in case
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			powerflow_values->island_matrix_values[island_loop_index].index_count = 0;
		}

		//Build the diagonal elements of the bus admittance matrix - this should only happen once no matter what
		if (powerflow_values->BA_diag == nullptr)
		{
			powerflow_values->BA_diag = (Bus_admit *)gl_malloc(bus_count *sizeof(Bus_admit));   //BA_diag store the location and value of diagonal elements of Bus Admittance matrix

			//Make sure it worked
			if (powerflow_values->BA_diag == nullptr)
			{
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");
				/*  TROUBLESHOOT
				During the allocation stage of the NR algorithm, one of the matrices failed to be allocated.
				Please try again and if this bug persists, submit your code and a bug report using the trac
				website.
				*/
			}
		}

		//Loop the islands and reset the bus count, just in case
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			powerflow_values->island_matrix_values[island_loop_index].bus_count = 0;
		}

		for (indexer=0; indexer<bus_count; indexer++) // Construct the diagonal elements of Bus admittance matrix.
		{
			//Increment the island bus counter
			if (bus[indexer].island_number != -1)
			{
				powerflow_values->island_matrix_values[bus[indexer].island_number].bus_count++;
			}

			//Determine the size we need
			if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase
				powerflow_values->BA_diag[indexer].size = 2;
			else										//Other cases, figure out how big they are
			{
				phase_worka = 0;
				for (jindex=0; jindex<3; jindex++)		//Accumulate number of phases
				{
					phase_worka += ((bus[indexer].phases & (0x01 << jindex)) >> jindex);
				}
				powerflow_values->BA_diag[indexer].size = phase_worka;
			}

			//Ensure the admittance matrix is zeroed
			for (jindex=0; jindex<3; jindex++)
			{
				for (kindex=0; kindex<3; kindex++)
				{
					powerflow_values->BA_diag[indexer].Y[jindex][kindex] = 0;
					tempY[jindex][kindex] = 0;
				}
			}

			//If we're in any deltamode, store out self-admittance as well, if it exists
			if ((powerflow_type!=PF_NORMAL) && (bus[indexer].full_Y != nullptr))
			{
				//Not triplex
				if ((bus[indexer].phases & 0x80) != 0x80)
				{
					//Figure out what our casing looks like - populate based on this (in case some were children)
					switch(bus[indexer].phases & 0x07) {
						case 0x00:	//No phases
						{
							break;	//Just get us out
						}
						case 0x01:	//Phase C
						{
							tempY[0][0] = bus[indexer].full_Y[8];
							break;
						}
						case 0x02:	//Phase B
						{
							tempY[0][0] = bus[indexer].full_Y[4];
							break;
						}
						case 0x03:	//Phase BC
						{
							tempY[0][0] = bus[indexer].full_Y[4];
							tempY[0][1] = bus[indexer].full_Y[5];
							tempY[1][0] = bus[indexer].full_Y[7];
							tempY[1][1] = bus[indexer].full_Y[8];
							break;
						}
						case 0x04:	//Phase A
						{
							tempY[0][0] = bus[indexer].full_Y[0];
							break;
						}
						case 0x05:	//Phase AC
						{
							tempY[0][0] = bus[indexer].full_Y[0];
							tempY[0][1] = bus[indexer].full_Y[2];
							tempY[1][0] = bus[indexer].full_Y[6];
							tempY[1][1] = bus[indexer].full_Y[8];
							break;
						}
						case 0x06:	//Phase AB
						{
							tempY[0][0] = bus[indexer].full_Y[0];
							tempY[0][1] = bus[indexer].full_Y[1];
							tempY[1][0] = bus[indexer].full_Y[3];
							tempY[1][1] = bus[indexer].full_Y[4];
							break;
						}
						case 0x07:	//Phase ABC
						{
							//Loop and add all in
							for (jindex=0; jindex<3; jindex++)
							{
								for (kindex=0; kindex<3; kindex++)
								{
									tempY[jindex][kindex] = bus[indexer].full_Y[jindex*3+kindex];	//Adds in any self-admittance terms (generators)
								}
							}
							break;
						}
						default:
						{
							GL_THROW("NR: Improper Norton equivalent admittance found");
							/*  TROUBLESHOOT
							While mapping Norton equivalent impedances into the system, an unexpected
							phase condition was encountered with the admittance.  Please try again.  If the error
							persists, please submit a bug report via the issue tracker.
							*/
						}
					}//End case
				}
				else  //Must be Triplex - add the "12" combination for the shunt
				{
					//See if we're the stupid "backwards notation" bus or not
					if ((bus[indexer].phases & 0x20) == 0x20)	//Special case
					{
						//Need to be negated, due to crazy conventions
						tempY[0][0] = -bus[indexer].full_Y[0];
						tempY[0][1] = -bus[indexer].full_Y[0];
						tempY[1][0] = bus[indexer].full_Y[0];
						tempY[1][1] = bus[indexer].full_Y[0];
					}
					else	//Standard triplex
					{
						tempY[0][0] = bus[indexer].full_Y[0];
						tempY[0][1] = bus[indexer].full_Y[0];
						tempY[1][0] = -bus[indexer].full_Y[0];
						tempY[1][1] = -bus[indexer].full_Y[0];
					}
				}
			}

			//Now go through all of the branches to get the self admittance information (hinges on size)
			for (kindexer=0; kindexer<(bus[indexer].Link_Table_Size);kindexer++)
			{
				//Assign jindexer as intermediate variable (easier for me this way)
				jindexer = bus[indexer].Link_Table[kindexer];

				if ((branch[jindexer].from == indexer) || (branch[jindexer].to == indexer))	//Bus is the from or to side of things - not sure how it would be in link table otherwise, but meh
				{
					if ((bus[indexer].phases & 0x07) == 0x07)		//Full three phase
					{
						for (jindex=0; jindex<3; jindex++)	//Add in all three phase values
						{
							//See if this phase is valid
							phase_workb = 0x04 >> jindex;

							if ((phase_workb & branch[jindexer].phases) == phase_workb)
							{
								for (kindex=0; kindex<3; kindex++)
								{
									//Check phase
									phase_workd = 0x04 >> kindex;

									if ((phase_workd & branch[jindexer].phases) == phase_workd)
									{
										if (branch[jindexer].from == indexer)	//We're the from version
										{
											tempY[jindex][kindex] += branch[jindexer].YSfrom[jindex*3+kindex];
										}
										else									//Must be the to version
										{
											tempY[jindex][kindex] += branch[jindexer].YSto[jindex*3+kindex];
										}
									}//End valid column phase
								}
							}//End valid row phase
						}
					}
					else if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase - add in 2x2 element to upper left 2x2
					{
						if (branch[jindexer].from == indexer)	//From branch
						{
							//End of SPCT transformer requires slightly different Diagonal components (so when it's the To bus of SPCT and from for other triplex
							if ((bus[indexer].phases & 0x20) == 0x20)	//Special case
							{
								//Other triplexes need to be negated to match sign conventions
								tempY[0][0] -= branch[jindexer].YSfrom[0];
								tempY[0][1] -= branch[jindexer].YSfrom[1];
								tempY[1][0] -= branch[jindexer].YSfrom[3];
								tempY[1][1] -= branch[jindexer].YSfrom[4];
							}
							else										//Just a normal to bus
							{
								tempY[0][0] += branch[jindexer].YSfrom[0];
								tempY[0][1] += branch[jindexer].YSfrom[1];
								tempY[1][0] += branch[jindexer].YSfrom[3];
								tempY[1][1] += branch[jindexer].YSfrom[4];
							}
						}
						else									//To branch
						{
							//Replicate the above for SPCT test, in case people put lines in backwards
							//End of SPCT transformer requires slightly different Diagonal components (so when it's the To bus of SPCT and from for other triplex
							if (((bus[indexer].phases & 0x20) == 0x20) && (branch[jindexer].lnk_type == 1))	//Special case, but make sure we're not the transformer
							{
								tempY[0][0] -= branch[jindexer].YSto[0];
								tempY[0][1] -= branch[jindexer].YSto[1];
								tempY[1][0] -= branch[jindexer].YSto[3];
								tempY[1][1] -= branch[jindexer].YSto[4];
							}
							else	//Normal bus - no funky SPCT nonsense
							{
								tempY[0][0] += branch[jindexer].YSto[0];
								tempY[0][1] += branch[jindexer].YSto[1];
								tempY[1][0] += branch[jindexer].YSto[3];
								tempY[1][1] += branch[jindexer].YSto[4];
							}
						}
					}
					else	//We must be a single or two-phase line - always populate the upper left portion of matrix (easier for later)
					{
						switch(bus[indexer].phases & 0x07) {
							case 0x00:	//No phases (we've been faulted out
								{
									break;	//Just get us outta here
								}
							case 0x01:	//Only C
								{
									if ((branch[jindexer].phases & 0x01) == 0x01)	//Phase C valid on branch
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[8];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[8];
										}
									}//End valid phase C
									break;
								}
							case 0x02:	//Only B
								{
									if ((branch[jindexer].phases & 0x02) == 0x02)	//Phase B valid on branch
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[4];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[4];
										}
									}//End valid phase B
									break;
								}
							case 0x03:	//B & C
								{
									phase_worka = (branch[jindexer].phases & 0x03);	//Extract branch phases

									if (phase_worka == 0x03)	//Full B & C
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[4];
											tempY[0][1] += branch[jindexer].YSfrom[5];
											tempY[1][0] += branch[jindexer].YSfrom[7];
											tempY[1][1] += branch[jindexer].YSfrom[8];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[4];
											tempY[0][1] += branch[jindexer].YSto[5];
											tempY[1][0] += branch[jindexer].YSto[7];
											tempY[1][1] += branch[jindexer].YSto[8];
										}
									}//End valid B & C
									else if (phase_worka == 0x01)	//Only C branch
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[1][1] += branch[jindexer].YSfrom[8];
										}
										else									//To branch
										{
											tempY[1][1] += branch[jindexer].YSto[8];
										}
									}//end valid C
									else if (phase_worka == 0x02)	//Only B branch
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[4];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[4];
										}
									}//end valid B
									else	//Must be nothing then - all phases must be faulted, or something
										;
									break;
								}
							case 0x04:	//Only A
								{
									if ((branch[jindexer].phases & 0x04) == 0x04)	//Phase A is valid
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[0];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[0];
										}
									}//end valid phase A
									break;
								}
							case 0x05:	//A & C
								{
									phase_worka = branch[jindexer].phases & 0x05;	//Extract phases

									if (phase_worka == 0x05)	//Both A & C valid
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[0];
											tempY[0][1] += branch[jindexer].YSfrom[2];
											tempY[1][0] += branch[jindexer].YSfrom[6];
											tempY[1][1] += branch[jindexer].YSfrom[8];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[0];
											tempY[0][1] += branch[jindexer].YSto[2];
											tempY[1][0] += branch[jindexer].YSto[6];
											tempY[1][1] += branch[jindexer].YSto[8];
										}
									}//End A & C valid
									else if (phase_worka == 0x04)	//Only A valid
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[0];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[0];
										}
									}//end only A valid
									else if (phase_worka == 0x01)	//Only C valid
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[1][1] += branch[jindexer].YSfrom[8];
										}
										else									//To branch
										{
											tempY[1][1] += branch[jindexer].YSto[8];
										}
									}//end only C valid
									else	//No connection - must be faulted
										;
									break;
								}
							case 0x06:	//A & B
								{
									phase_worka = branch[jindexer].phases & 0x06;	//Extract phases

									if (phase_worka == 0x06)	//Valid A & B phase
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[0];
											tempY[0][1] += branch[jindexer].YSfrom[1];
											tempY[1][0] += branch[jindexer].YSfrom[3];
											tempY[1][1] += branch[jindexer].YSfrom[4];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[0];
											tempY[0][1] += branch[jindexer].YSto[1];
											tempY[1][0] += branch[jindexer].YSto[3];
											tempY[1][1] += branch[jindexer].YSto[4];
										}
									}//End valid A & B
									else if (phase_worka == 0x04)	//Only valid A
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[0][0] += branch[jindexer].YSfrom[0];
										}
										else									//To branch
										{
											tempY[0][0] += branch[jindexer].YSto[0];
										}
									}//end valid A
									else if (phase_worka == 0x02)	//Only valid B
									{
										if (branch[jindexer].from == indexer)	//From branch
										{
											tempY[1][1] += branch[jindexer].YSfrom[4];
										}
										else									//To branch
										{
											tempY[1][1] += branch[jindexer].YSto[4];
										}
									}//end valid B
									else	//Default - must be already handled
										;
									break;
								}
							default:	//How'd we get here?
								{
									GL_THROW("Unknown phase connection in NR self admittance diagonal");
									/*  TROUBLESHOOT
									An unknown phase condition was encountered in the NR solver when constructing
									the self admittance diagonal.  Please report this bug and submit your code to
									the trac system.
									*/
								break;
								}
						}	//switch end
					}	//1 or 2 phase end
				}	//phase accumulation end
				else		//It's nothing (no connnection)
					;
			}//branch traversion end

			//Store the self admittance into BA_diag.  Also update the indices for possible use later
			//Only do this if we're actually populated
			island_index_val = bus[indexer].island_number;

			if (island_index_val != -1)
			{
				if (NR_solver_algorithm == NRM_TCIM)
				{
					powerflow_values->BA_diag[indexer].col_ind = powerflow_values->BA_diag[indexer].row_ind = powerflow_values->island_matrix_values[island_index_val].index_count;	// Store the row and column starting information (square matrices)
					bus[indexer].Matrix_Loc = powerflow_values->island_matrix_values[island_index_val].index_count;								//Store our location so we know where we go
					powerflow_values->island_matrix_values[island_index_val].index_count += powerflow_values->BA_diag[indexer].size;				// Update the index for this matrix's size, so next one is in appropriate place
				}
				else //Assumed to be FPI instead
				{
					if ((bus[indexer].type > 1) && bus[indexer].swing_functions_enabled)
					{
						powerflow_values->BA_diag[indexer].col_ind = powerflow_values->BA_diag[indexer].row_ind = -1;	//Flag variable
						bus[indexer].Matrix_Loc = 65536;	//Just big value - idea being it should serve as debugging
					}
					else	//Standard bus of SWING masquerading as PQ
					{
						powerflow_values->BA_diag[indexer].col_ind = powerflow_values->BA_diag[indexer].row_ind = powerflow_values->island_matrix_values[island_index_val].index_count;	// Store the row and column starting information (square matrices)
						bus[indexer].Matrix_Loc = powerflow_values->island_matrix_values[island_index_val].index_count;								//Store our location so we know where we go
						powerflow_values->island_matrix_values[island_index_val].index_count += powerflow_values->BA_diag[indexer].size;				// Update the index for this matrix's size, so next one is in appropriate place
					}
				}
			}

			//Store the admittance values into the BA_diag matrix structure
			for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)
			{
				for (kindex=0; kindex<powerflow_values->BA_diag[indexer].size; kindex++)			//Store values - assume square matrix - don't bother parsing what doesn't exist.
				{
					powerflow_values->BA_diag[indexer].Y[jindex][kindex] = tempY[jindex][kindex];// Store the self admittance terms.
				}
			}

			//Copy values into node-specific link (if needed)
			if (bus[indexer].full_Y_all != nullptr)
			{
				for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)
				{
					for (kindex=0; kindex<powerflow_values->BA_diag[indexer].size; kindex++)			//Store values - assume square matrix - don't bother parsing what doesn't exist.
					{
						bus[indexer].full_Y_all[jindex*3+kindex] = tempY[jindex][kindex];// Store the self admittance terms.
					}
				}
			}//End self-admittance update
		}//End diagonal construction

		//Loop through the islands and set the initial items
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			//Store the size of the diagonal, since it represents how many variables we are solving (useful later)
			powerflow_values->island_matrix_values[island_loop_index].total_variables = powerflow_values->island_matrix_values[island_loop_index].index_count;

			//Check to see if we've exceeded our max.  If so, reallocate!
			if (powerflow_values->island_matrix_values[island_loop_index].total_variables > powerflow_values->island_matrix_values[island_loop_index].max_total_variables)
				powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed = true;

			/// Build the off_diagonal_PQ bus elements of 6n*6n Y_NR matrix.Equation (12). All the value in this part will not be updated at each iteration.
			//Constructed using sparse methodology, non-zero elements are the only thing considered (and non-PV)
			//No longer necessarily 6n*6n any more either,
			powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ = 0;
		}//End loop traversion

		for (jindexer=0; jindexer<branch_count;jindexer++)	//Parse all of the branches
		{
			tempa  = branch[jindexer].from;
			tempb  = branch[jindexer].to;

			//Determine if we should proceed - based off if SWING bus
			if (NR_solver_algorithm == NRM_FPI)
			{
				//See if either of us are SWING-enabled buses
				if (((bus[tempa].type > 1) && bus[tempa].swing_functions_enabled) || (((bus[tempb].type > 1) && bus[tempb].swing_functions_enabled)))
				{
					continue;	//Skip this entry - it won't exist in the final matrix
				}
			}
			//TCIM does all branches - regardless of connection

			//Pull the island index off the from bus -- for islands, both ends SHOULD be in the same index
			island_index_val = bus[tempa].island_number;

			//Check and see if we should just skip this branch -- if it's an unassociated node, probably don't need this branch
			//Explicitly check both ends
			if ((bus[tempa].island_number == -1) || (bus[tempb].island_number == -1))
			{
				//Random error check - see if the branch somehow is oddly associated
				if (branch[jindexer].island_number != -1)
				{
					GL_THROW("NR line:%d - %s has an island association, but the ends do not",branch[jindexer].obj->id,(branch[jindexer].name ? branch[jindexer].name : "Unnamed"));
					/*  TROUBLESHOOT
					When parsing the line list, a line connected to an isolated node is still somehow associated with a proper island.
					This should not occur.  Please submit your GLM file and a description to the
					issue tracker.
					*/
				}
				//Default else - unassociated, so just let it go

				//Basically not part of any system, just skip it
				continue;
			}
			else	//from/to are valid
			{
				//Paranoia check - make sure this line isn't somehow unassociated - ignore switching-type lines, since they are skipped when open
				if ((branch[jindexer].island_number == -1) && (branch[jindexer].lnk_type < 3))
				{
					GL_THROW("NR line:%d - %s does not have a proper island association",branch[jindexer].obj->id,(branch[jindexer].name ? branch[jindexer].name : "Unnamed"));
					/*  TROUBLESHOOT
					When parsing the line list, a line connected to two valid nodes is still somehow unassociated.
					This should not occur.  Please submit your GLM file and a description to the
					issue tracker.
					*/
				}
			}

			//Preliminary check to make sure we weren't missed in the initialization
			if ((bus[tempa].Matrix_Loc == -1) || (bus[tempb].Matrix_Loc == -1))
			{
				GL_THROW("An element in NR line:%d - %s was not properly localized",branch[jindexer].obj->id,(branch[jindexer].name ? branch[jindexer].name : "Unnamed"));
				/*  TROUBLESHOOT
				When parsing the bus list, the Newton-Raphson solver found a bus that did not
				appear to have a location within the overall admittance/Jacobian matrix.  Please
				submit this as a bug with your code on the Trac site.
				*/
			}

			if (((branch[jindexer].phases & 0x80) == 0x80) && (branch[jindexer].v_ratio==1.0))	//Triplex, but not SPCT
			{
				for (jindex=0; jindex<2; jindex++)			//rows
				{
					for (kindex=0; kindex<2; kindex++)		//columns
					{
						if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
							powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

						if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
							powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

						if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
							powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

						if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
							powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;
					}//end columns of split phase
				}//end rows of split phase
			}//end traversion of split-phase
			else											//Three phase or some variety
			{
				//Make sure we aren't SPCT, otherwise things get jacked
				if ((branch[jindexer].phases & 0x80) != 0x80)	//SPCT, but v_ratio not = 1
				{
					for (jindex=0; jindex<3; jindex++)			//rows
					{
						//See if this phase is valid
						phase_workb = 0x04 >> jindex;

						if ((phase_workb & branch[jindexer].phases) == phase_workb)	//Row check
						{
							for (kindex=0; kindex<3; kindex++)		//columns
							{
								//Check this phase as well
								phase_workd = 0x04 >> kindex;

								if ((phase_workd & branch[jindexer].phases) == phase_workd)	//Column validity check
								{
									if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
										powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

									if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
										powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

									if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
										powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

									if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
										powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;
								}//end column validity check
							}//end columns of 3 phase
						}//End row validity check
					}//end rows of 3 phase
				}//end not SPCT
				else	//SPCT implementation
				{
					for (jindex=0; jindex<3; jindex++)			//rows
					{
						//See if this phase is valid
						phase_workb = 0x04 >> jindex;

						if ((phase_workb & branch[jindexer].phases) == phase_workb)	//Row check
						{
							for (kindex=0; kindex<3; kindex++)		//Row valid, traverse all columns for SPCT Yfrom
							{
								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
									powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
									powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;
							}//end columns traverse

							//If row is valid, now traverse the rows of that column for Yto
							for (kindex=0; kindex<3; kindex++)
							{
								if (((branch[jindexer].Yto[kindex*3+jindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
									powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;

								if (((branch[jindexer].Yto[kindex*3+jindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
									powerflow_values->island_matrix_values[island_index_val].size_offdiag_PQ += 1;
							}//end rows traverse
						}//End row validity check
					}//end rows of 3 phase
				}//End SPCT
			}//end three phase
		}//end line traversion

		//Loop through the islands again
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			//Allocate the space - double the number found (each element goes in two places)
			if (powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ == nullptr)
			{
				powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ = (Y_NR *)gl_malloc((powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ*2) *sizeof(Y_NR));   //powerflow_values->island_matrix_values[x].Y_offdiag_PQ store the row,column and value of off_diagonal elements of Bus Admittance matrix in which all the buses are not PV buses.

				//Make sure it worked
				if (powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ == nullptr)
					GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

				//Save our size
				powerflow_values->island_matrix_values[island_loop_index].max_size_offdiag_PQ = powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ;	//Don't care about the 2x, since we'll be comparing it against itself
			}
			else if (powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ > powerflow_values->island_matrix_values[island_loop_index].max_size_offdiag_PQ)	//Something changed and we are bigger!!
			{
				//Destroy us!
				gl_free(powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ);

				//Rebuild us, we have the technology
				powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ = (Y_NR *)gl_malloc((powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ*2) *sizeof(Y_NR));

				//Make sure it worked
				if (powerflow_values->island_matrix_values[island_loop_index].Y_offdiag_PQ == nullptr)
					GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

				//Store the new size
				powerflow_values->island_matrix_values[island_loop_index].max_size_offdiag_PQ = powerflow_values->island_matrix_values[island_loop_index].size_offdiag_PQ;

				//Flag for a reallocation
				powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed = true;
			}

			//Zero the indexer term used in the next section
			powerflow_values->island_matrix_values[island_loop_index].indexer = 0;
		}//End island loop

		for (jindexer=0; jindexer<branch_count;jindexer++)	//Parse through all of the branches
		{
			//Extract both ends
			tempa  = branch[jindexer].from;
			tempb  = branch[jindexer].to;

			//Determine if we should proceed - based off if SWING bus
			if (NR_solver_algorithm == NRM_FPI)
			{
				//See if either of us are SWING-enabled buses
				if (((bus[tempa].type > 1) && bus[tempa].swing_functions_enabled) || (((bus[tempb].type > 1) && bus[tempb].swing_functions_enabled)))
				{
					continue;	//Skip this entry - it won't exist in the final matrix
				}
			}
			//TCIM does all branches - regardless of connection

			//Pull the island index off the from bus -- for islands, both ends SHOULD be in the same index
			island_index_val = bus[tempa].island_number;

			//Make sure it isn't invalid -- if it is, just skip this, since that means these lines do nothing anyways
			//Explicit check on "oddities" for island association was earlier
			if (branch[jindexer].island_number == -1)
			{
				continue;
			}

			phase_worka = 0;
			phase_workb = 0;
			for (jindex=0; jindex<3; jindex++)		//Accumulate number of phases
			{
				phase_worka += ((bus[tempa].phases & (0x01 << jindex)) >> jindex);
				phase_workb += ((bus[tempb].phases & (0x01 << jindex)) >> jindex);
			}

			if ((phase_worka==3) && (phase_workb==3))	//Both ends are full three phase, normal operations
			{
				for (jindex=0; jindex<3; jindex++)		//Loop through rows of admittance matrices
				{
					//See if this row is valid for this branch
					phase_workd = 0x04 >> jindex;

					if ((branch[jindexer].phases & phase_workd) == phase_workd)	//Validity check
					{
						for (kindex=0; kindex<3; kindex++)	//Loop through columns of admittance matrices
						{
							//Extract column information
							phase_worke = 0x04 >> kindex;

							if ((branch[jindexer].phases & phase_worke) == phase_worke)	//Valid column too!
							{
								//Make sure neither are a PV bus (even though unsupported so far)
								if ((bus[tempa].type != 1) && (bus[tempb].type != 1))
								{
									//Indices counted out from Self admittance above.  needs doubling due to complex separation
									if ((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0)	//From imags
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if ((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0)	//To imags
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if ((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0)	//From reals
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if ((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0)	//To reals
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 3;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}
								}//End not PV bus
								//Default else - somehow is PV bus - we don't support those, so ignore it
							}//End valid column
						}//column end
					}//End valid row
				}//row end
			}//if all 3 end
			else if (((bus[tempa].phases & 0x80) == 0x80) || ((bus[tempb].phases & 0x80) == 0x80))	//Someone's a triplex
			{
				if (((bus[tempa].phases & 0x80) == 0x80) && ((bus[tempb].phases & 0x80) == 0x80))	//Both are triplex, easy case
				{
					for (jindex=0; jindex<2; jindex++)		//Loop through rows of admittance matrices (only 2x2)
					{
						for (kindex=0; kindex<2; kindex++)	//Loop through columns of admittance matrices (only 2x2)
						{
							//Make sure one end of us isn't a SPCT transformer To node (they are different)
							if (((bus[tempa].phases & 0x20) & (bus[tempb].phases & 0x20)) == 0x20)	//Both ends are SPCT tos
							{
								GL_THROW("NR: SPCT to SPCT via triplex connections are unsupported at this time.");
								/*  TROUBLESHOOT
								The Newton-Raphson solve does not currently support running a triplex line between the low-voltage
								side of two different split-phase center tapped transformers.  This functionality may be added if needed
								in the future.
								*/
							}//end both ends SPCT to
							else if ((bus[tempa].phases & 0x20) == 0x20)	//From end is a SPCT to
							{
								//Indices counted out from Self admittance above.  needs doubling due to complex separation

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(branch[jindexer].Yfrom[jindex*3+kindex]).Im();
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}
							}//end From end SPCT to
							else if ((bus[tempb].phases & 0x20) == 0x20)	//To end is a SPCT to
							{
								//Indices counted out from Self admittance above.  needs doubling due to complex separation

								//Make sure we aren't the transformer
								if (branch[jindexer].lnk_type == 1)	//We're not, we're a line - proceed
								{

									//Indices counted out from Self admittance above.  needs doubling due to complex separation
									if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Im());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(branch[jindexer].Yto[jindex*3+kindex]).Im();
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}
								}//End SPCT TO bus and we're a line
								else	//Transformer to - don't do things weird
								{
									if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Im());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(branch[jindexer].Yto[jindex*3+kindex]).Im();
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}

									if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
									{
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;

										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
										powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
										powerflow_values->island_matrix_values[island_index_val].indexer += 1;
									}
								}//End SPCT TO and we're the transformer
							}//end To end SPCT to
							else											//Plain old ugly line
							{
								//Indices counted out from Self admittance above.  needs doubling due to complex separation

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
								{
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;

									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									powerflow_values->island_matrix_values[island_index_val].indexer += 1;
								}
							}//end Normal triplex branch
						}//column end
					}//row end
				}//end both triplexy
				else if ((bus[tempa].phases & 0x80) == 0x80)	//From is the triplex - this implies transformer with or something, we don't support this
				{
					GL_THROW("NR does not support triplex to 3-phase connections.");
					/*  TROUBLESHOOT
					The Newton-Raphson solver does not have any implementation elements
					to support the connection of a split-phase or triplex node to a three-phase
					node.  The opposite (3-phase to triplex) is available as the split-phase-center-
					tapped transformer model.  See if that will work for your implementation.
					*/
				}//end from triplexy
				else	//Only option left is the to must be the triplex - implies SPCT xformer - so only one phase on the three-phase side (we just need to figure out where)
				{
					//Extract the line phase
					phase_workc = (branch[jindexer].phases & 0x07);

					//Reset temp_index and size, just in case
					temp_index = -1;
					temp_size = -1;

					//Figure out what the offset on the from side is (how many phases and which one we are)
					switch(bus[tempa].phases & 0x07)
					{
						case 0x01:	//C
							{
								temp_size = 1;	//Single phase matrix

								if (phase_workc==0x01)	//Line is phase C
								{
									//Only C in the node, so no offset
									temp_index = 0;
								}
								else if (phase_workc==0x02)	//Line is phase B
								{
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");
									/*  TROUBLESHOOT
									A split-phase, center-tapped transformer in the Newton-Raphson solver is somehow attached
									to a node that is missing the required phase of the transformer.  This should have been caught.
									Please submit your code and a bug report using the trac website.
									*/
								}
								else					//Has to be phase A
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");

								break;
							}
						case 0x02:	//B
							{
								temp_size = 1;	//Single phase matrix

								if (phase_workc==0x01)	//Line is phase C
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");
								else if (phase_workc==0x02)	//Line is phase B
								{
									//Only B in the node, so no offset
									temp_index = 0;
								}
								else					//Has to be phase A
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");

								break;
							}
						case 0x03:	//BC
							{
								temp_size = 2;	//Two phase matrix

								if (phase_workc==0x01)	//Line is phase C
								{
									//BC in the node, so offset by 1
									temp_index = 1;
								}
								else if (phase_workc==0x02)	//Line is phase B
								{
									//BC in the node, so offset by 0
									temp_index = 0;
								}
								else					//Has to be phase A
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");

								break;
							}
						case 0x04:	//A
							{
								temp_size = 1;	//Single phase matrix

								if (phase_workc==0x01)	//Line is phase C
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");
								else if (phase_workc==0x02)	//Line is phase B
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");
								else					//Has to be phase A
								{
									//Only A in the node, so no offset
									temp_index = 0;
								}

								break;
							}
						case 0x05:	//AC
							{
								temp_size = 2;	//Two phase matrix

								if (phase_workc==0x01)	//Line is phase C
								{
									//AC in the node, so offset by 1
									temp_index = 1;
								}
								else if (phase_workc==0x02)	//Line is phase B
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");
								else					//Has to be phase A
								{
									//AC in the node, so offset by 0
									temp_index = 0;
								}

								break;
							}
						case 0x06:	//AB
							{
								temp_size = 2;	//Two phase matrix

								if (phase_workc==0x01)	//Line is phase C
									GL_THROW("NR: A center-tapped transformer has an invalid phase matching");
								else if (phase_workc==0x02)	//Line is phase B
								{
									//BC in the node, so offset by 1
									temp_index = 1;
								}
								else					//Has to be phase A
								{
									//AB in the node, so offset by 0
									temp_index = 0;
								}

								break;
							}
						case 0x07:	//ABC
							{
								temp_size = 3;	//Three phase matrix

								if (phase_workc==0x01)	//Line is phase C
								{
									//ABC in the node, so offset by 2
									temp_index = 2;
								}
								else if (phase_workc==0x02)	//Line is phase B
								{
									//ABC in the node, so offset by 1
									temp_index = 1;
								}
								else					//Has to be phase A
								{
									//ABC in the node, so offset by 0
									temp_index = 0;
								}

								break;
							}
						default:
							GL_THROW("NR: A center-tapped transformer has an invalid phase matching");
							break;
					}//end switch
					if ((temp_index==-1) || (temp_size==-1))	//Should never get here
						GL_THROW("NR: A center-tapped transformer has an invalid phase matching");

					//Determine first index
					if (phase_workc==0x01)	//Line is phase C
					{
						jindex=2;
					}//end line C if
					else if (phase_workc==0x02)	//Line is phase B
					{
						jindex=1;
					}//end line B if
					else						//Line has to be phase A
					{
						jindex=0;
					}//End line A if


					//Indices counted out from Self admittance above.  needs doubling due to complex separation
					for (kindex=0; kindex<2; kindex++)	//Loop through columns of admittance matrices (only 2x2)
					{

						if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
						{
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;

							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;
						}

						if (((branch[jindexer].Yto[kindex*3+jindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
						{
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Im());
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;

							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (branch[jindexer].Yto[kindex*3+jindex]).Im();
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;
						}

						if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
						{
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;

							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;
						}

						if (((branch[jindexer].Yto[kindex*3+jindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
						{
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Re());
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;

							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Re());
							powerflow_values->island_matrix_values[island_index_val].indexer += 1;
						}
					}//secondary index end

				}//end to triplexy
			}//end triplex in here
			else					//Some combination of not-3 phase
			{
				//Clear working variables, just in case
				temp_index = temp_index_b = -1;
				temp_size = temp_size_b = temp_size_c = -1;
				Full_Mat_A = Full_Mat_B = false;

				//Intermediate store the admittance matrices so they can be directly indexed later
				switch(branch[jindexer].phases & 0x07) {
					case 0x00:	//No phases (open switch or reliability excluded item)
						{
							temp_size_c = -99;	//Arbitrary flag
							break;
						}
					case 0x01:	//C only
						{
							Temp_Ad_A[0][0] = branch[jindexer].Yfrom[8];
							Temp_Ad_B[0][0] = branch[jindexer].Yto[8];
							temp_size_c = 1;
							break;
						}
					case 0x02:	//B only
						{
							Temp_Ad_A[0][0] = branch[jindexer].Yfrom[4];
							Temp_Ad_B[0][0] = branch[jindexer].Yto[4];
							temp_size_c = 1;
							break;
						}
					case 0x03:	//BC only
						{
							Temp_Ad_A[0][0] = branch[jindexer].Yfrom[4];
							Temp_Ad_A[0][1] = branch[jindexer].Yfrom[5];
							Temp_Ad_A[1][0] = branch[jindexer].Yfrom[7];
							Temp_Ad_A[1][1] = branch[jindexer].Yfrom[8];

							Temp_Ad_B[0][0] = branch[jindexer].Yto[4];
							Temp_Ad_B[0][1] = branch[jindexer].Yto[5];
							Temp_Ad_B[1][0] = branch[jindexer].Yto[7];
							Temp_Ad_B[1][1] = branch[jindexer].Yto[8];

							temp_size_c = 2;
							break;
						}
					case 0x04:	//A only
						{
							Temp_Ad_A[0][0] = branch[jindexer].Yfrom[0];
							Temp_Ad_B[0][0] = branch[jindexer].Yto[0];
							temp_size_c = 1;
							break;
						}
					case 0x05:	//AC only
						{
							Temp_Ad_A[0][0] = branch[jindexer].Yfrom[0];
							Temp_Ad_A[0][1] = branch[jindexer].Yfrom[2];
							Temp_Ad_A[1][0] = branch[jindexer].Yfrom[6];
							Temp_Ad_A[1][1] = branch[jindexer].Yfrom[8];

							Temp_Ad_B[0][0] = branch[jindexer].Yto[0];
							Temp_Ad_B[0][1] = branch[jindexer].Yto[2];
							Temp_Ad_B[1][0] = branch[jindexer].Yto[6];
							Temp_Ad_B[1][1] = branch[jindexer].Yto[8];

							temp_size_c = 2;
							break;
						}
					case 0x06:	//AB only
						{
							Temp_Ad_A[0][0] = branch[jindexer].Yfrom[0];
							Temp_Ad_A[0][1] = branch[jindexer].Yfrom[1];
							Temp_Ad_A[1][0] = branch[jindexer].Yfrom[3];
							Temp_Ad_A[1][1] = branch[jindexer].Yfrom[4];

							Temp_Ad_B[0][0] = branch[jindexer].Yto[0];
							Temp_Ad_B[0][1] = branch[jindexer].Yto[1];
							Temp_Ad_B[1][0] = branch[jindexer].Yto[3];
							Temp_Ad_B[1][1] = branch[jindexer].Yto[4];

							temp_size_c = 2;
							break;
						}
					default:
						{
							break;
						}
				}//end line switch/case

				if (temp_size_c==-99)
				{
					continue;	//Next iteration of branch loop
				}

				if (temp_size_c==-1)	//Make sure it is right
				{
					GL_THROW("NR: A line's phase was flagged as not full three-phase, but wasn't: (%s) %u %u %u %u",
									 branch[jindexer].name, branch[jindexer].phases, branch[jindexer].origphases, phase_worka, phase_workb);
					/*  TROUBLESHOOT
					A line inside the powerflow model was flagged as not being full three-phase or
					triplex in any form.  It failed the other cases though, so it must have been.
					Please submit your code and a bug report to the trac website.
					*/
				}

				//Check the from side and get all appropriate offsets
				switch(bus[tempa].phases & 0x07) {
					case 0x01:	//C
						{
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_size = 1;		//Single size
								temp_index = 0;		//No offset (only 1 big)
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
								/*  TROUBLESHOOT
								One of the lines in the powerflow model has an invalid phase in
								reference to its to and from ends.  This should have been caught
								earlier, so submit your code and a bug report using the trac website.
								*/
							}
							break;
						}//end 0x01
					case 0x02:	//B
						{
							if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_size = 1;		//Single size
								temp_index = 0;		//No offset (only 1 big)
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x02
					case 0x03:	//BC
						{
							temp_size = 2;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_index = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_index = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x03)	//BC
							{
								temp_index = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x03
					case 0x04:	//A
						{
							if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_size = 1;		//Single size
								temp_index = 0;		//No offset (only 1 big)
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x04
					case 0x05:	//AC
						{
							temp_size = 2;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_index = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_index = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x05)	//AC
							{
								temp_index = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x05
					case 0x06:	//AB
						{
							temp_size = 2;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_index = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_index = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x06)	//AB
							{
								temp_index = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x06
					case 0x07:	//ABC
						{
							temp_size = 3;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_index = 2;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_index = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x03)	//BC
							{
								temp_index = 1;
							}
							else if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_index = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x05)	//AC
							{
								temp_index = 0;
								Full_Mat_A = true;		//Flag so we know C needs to be gapped
							}
							else if ((branch[jindexer].phases & 0x07) == 0x06)	//AB
							{
								temp_index = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x07
					default:
						{
							break;
						}
				}//End switch/case for from

				//Check the to side and get all appropriate offsets
				switch(bus[tempb].phases & 0x07) {
					case 0x01:	//C
						{
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_size_b = 1;		//Single size
								temp_index_b = 0;		//No offset (only 1 big)
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
								/*  TROUBLESHOOT
								One of the lines in the powerflow model has an invalid phase in
								reference to its to and from ends.  This should have been caught
								earlier, so submit your code and a bug report using the trac website.
								*/
							}
							break;
						}//end 0x01
					case 0x02:	//B
						{
							if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_size_b = 1;		//Single size
								temp_index_b = 0;		//No offset (only 1 big)
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x02
					case 0x03:	//BC
						{
							temp_size_b = 2;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_index_b = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_index_b = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x03)	//BC
							{
								temp_index_b = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x03
					case 0x04:	//A
						{
							if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_size_b = 1;		//Single size
								temp_index_b = 0;		//No offset (only 1 big)
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x04
					case 0x05:	//AC
						{
							temp_size_b = 2;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_index_b = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_index_b = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x05)	//AC
							{
								temp_index_b = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x05
					case 0x06:	//AB
						{
							temp_size_b = 2;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_index_b = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_index_b = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x06)	//AB
							{
								temp_index_b = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x06
					case 0x07:	//ABC
						{
							temp_size_b = 3;	//Size of this matrix's admittance
							if ((branch[jindexer].phases & 0x07) == 0x01)	//C
							{
								temp_index_b = 2;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x02)	//B
							{
								temp_index_b = 1;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x03)	//BC
							{
								temp_index_b = 1;
							}
							else if ((branch[jindexer].phases & 0x07) == 0x04)	//A
							{
								temp_index_b = 0;		//offset
							}
							else if ((branch[jindexer].phases & 0x07) == 0x05)	//AC
							{
								temp_index_b = 0;
								Full_Mat_B = true;		//Flag so we know C needs to be gapped
							}
							else if ((branch[jindexer].phases & 0x07) == 0x06)	//AB
							{
								temp_index_b = 0;
							}
							else
							{
								GL_THROW("NR: One of the lines has invalid phase parameters");
							}
							break;
						}//end 0x07
					default:
						{
							break;
						}
				}//End switch/case for to

				//Make sure everything was set before proceeding
				if ((temp_index==-1) || (temp_index_b==-1) || (temp_size==-1) || (temp_size_b==-1) || (temp_size_c==-1))
				{
					GL_THROW("NR: Failure to construct single/double phase line indices");
					/*  TROUBLESHOOT
					A single or double phase line (e.g., just A or AB) has failed to properly initialize all of the indices
					necessary to form the admittance matrix.  Please submit a bug report, with your code, to the trac site.
					*/
				}

				if (Full_Mat_A)	//From side is a full ABC and we have AC
				{
					for (jindex=0; jindex<temp_size_c; jindex++)		//Loop through rows of admittance matrices
					{
						for (kindex=0; kindex<temp_size_c; kindex++)	//Loop through columns of admittance matrices
						{
							//Indices counted out from Self admittance above.  needs doubling due to complex separation
							if ((Temp_Ad_A[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2 + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2 + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2 + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2 + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}
						}//column end
					}//row end
				}//end full ABC for from AC

				if (Full_Mat_B)	//To side is a full ABC and we have AC
				{
					for (jindex=0; jindex<temp_size_c; jindex++)		//Loop through rows of admittance matrices
					{
						for (kindex=0; kindex<temp_size_c; kindex++)	//Loop through columns of admittance matrices
						{
							//Indices counted out from Self admittance above.  needs doubling due to complex separation
							if ((Temp_Ad_A[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2 + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2 + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2 + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2 + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}
						}//column end
					}//row end
				}//end full ABC for to AC

				if ((!Full_Mat_A) && (!Full_Mat_B))	//Neither is a full ABC, or we aren't doing AC, so we don't care
				{
					for (jindex=0; jindex<temp_size_c; jindex++)		//Loop through rows of admittance matrices
					{
						for (kindex=0; kindex<temp_size_c; kindex++)	//Loop through columns of admittance matrices
						{
							//Indices counted out from Self admittance above.  needs doubling due to complex separation
							if ((Temp_Ad_A[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
							{
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->island_matrix_values[island_index_val].Y_offdiag_PQ[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}
						}//column end
					}//row end
				}//end not full ABC with AC on either side case
			}//end all others else
		}//end branch for
	}// Off-diagonal elements update, as well as diagon fixed "base" elements- same for both

	//If an impedance load change
	if (NR_admit_change || NR_FPI_imp_load_change)
	{
		//Loop through the islands
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			//Build the fixed part of the diagonal PQ bus elements of 6n*6n Y_NR matrix. This part will not be updated at each iteration.
			powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed = 0;
		}

		for (jindexer=0; jindexer<bus_count;jindexer++)
		{
			if (bus[jindexer].island_number != -1)
			{
				//Pull the index
				island_loop_index = bus[jindexer].island_number;

				//Mode check
				if (NR_solver_algorithm == NRM_FPI)
				{
					if ((bus[jindexer].type > 1) && bus[jindexer].swing_functions_enabled)
					{
						continue;	//Skip us - won't be part of the fixed diagonal
					}

					//Secondary - call the load update function for the device, if it has one (ensures full_Y_load gets updated, if needed)
					if (bus[jindexer].LoadUpdateFxn != nullptr)
					{
						//Call the function
						call_return_status = ((STATUS (*)(OBJECT *))(*bus[jindexer].LoadUpdateFxn))(bus[jindexer].obj);

						//Make sure it worked
						if (call_return_status == FAILED)
						{
							GL_THROW("NR: Load update failed for device %s",bus[jindexer].obj->name ? bus[jindexer].obj->name : "Unnamed");
							//Defined below in compute_load_values
						}
						//Default else - it worked
					}

					//Copy load values in appropriately
					if (bus[jindexer].full_Y_load != nullptr)
					{
						//Not triplex
						if ((bus[jindexer].phases & 0x80) != 0x80)
						{
							//Figure out what our casing looks like - populate based on this (in case some were children)
							switch(bus[jindexer].phases & 0x07) {
								case 0x00:	//No phases
								{
									break;	//Just get us out
								}
								case 0x01:	//Phase C
								{
									powerflow_values->BA_diag[jindexer].Yload[0][0] = bus[jindexer].full_Y_load[8];
									break;
								}
								case 0x02:	//Phase B
								{
									powerflow_values->BA_diag[jindexer].Yload[0][0] = bus[jindexer].full_Y_load[4];
									break;
								}
								case 0x03:	//Phase BC
								{
									powerflow_values->BA_diag[jindexer].Yload[0][0] = bus[jindexer].full_Y_load[4];
									powerflow_values->BA_diag[jindexer].Yload[0][1] = bus[jindexer].full_Y_load[5];
									powerflow_values->BA_diag[jindexer].Yload[1][0] = bus[jindexer].full_Y_load[7];
									powerflow_values->BA_diag[jindexer].Yload[1][1] = bus[jindexer].full_Y_load[8];
									break;
								}
								case 0x04:	//Phase A
								{
									powerflow_values->BA_diag[jindexer].Yload[0][0] = bus[jindexer].full_Y_load[0];
									break;
								}
								case 0x05:	//Phase AC
								{
									powerflow_values->BA_diag[jindexer].Yload[0][0] = bus[jindexer].full_Y_load[0];
									powerflow_values->BA_diag[jindexer].Yload[0][1] = bus[jindexer].full_Y_load[2];
									powerflow_values->BA_diag[jindexer].Yload[1][0] = bus[jindexer].full_Y_load[6];
									powerflow_values->BA_diag[jindexer].Yload[1][1] = bus[jindexer].full_Y_load[8];
									break;
								}
								case 0x06:	//Phase AB
								{
									powerflow_values->BA_diag[jindexer].Yload[0][0] = bus[jindexer].full_Y_load[0];
									powerflow_values->BA_diag[jindexer].Yload[0][1] = bus[jindexer].full_Y_load[1];
									powerflow_values->BA_diag[jindexer].Yload[1][0] = bus[jindexer].full_Y_load[3];
									powerflow_values->BA_diag[jindexer].Yload[1][1] = bus[jindexer].full_Y_load[4];
									break;
								}
								case 0x07:	//Phase ABC
								{
									//Loop and add all in
									for (jindex=0; jindex<3; jindex++)
									{
										for (kindex=0; kindex<3; kindex++)
										{
											powerflow_values->BA_diag[jindexer].Yload[jindex][kindex] = bus[jindexer].full_Y_load[jindex*3+kindex];	//Adds in any self-admittance terms (generators)
										}
									}
									break;
								}
								default:
								{
									GL_THROW("NR: Improper Impedance Reference Found");
									/*  TROUBLESHOOT
									While populating the impedance portion of the FPI admittance matrix, an unexpected
									phase condition was encountered with the admittance.  Please try again.  If the error
									persists, please submit a bug report via the issue tracker.
									*/
								}
							}//End case
						}
						else  //Must be Triplex - add the "12" combination for the shunt
						{
							//See if we're the stupid "backwards notation" bus or not
							if ((bus[jindexer].phases & 0x20) == 0x20)	//Special case
							{
								//Need to be negated, due to crazy conventions
								powerflow_values->BA_diag[jindexer].Yload[0][0] = -bus[jindexer].full_Y_load[0];
								powerflow_values->BA_diag[jindexer].Yload[0][1] = -bus[jindexer].full_Y_load[1];
								powerflow_values->BA_diag[jindexer].Yload[1][0] = bus[jindexer].full_Y_load[3];
								powerflow_values->BA_diag[jindexer].Yload[1][1] = bus[jindexer].full_Y_load[4];
							}
							else	//Standard triplex
							{
								powerflow_values->BA_diag[jindexer].Yload[0][0] = bus[jindexer].full_Y_load[0];
								powerflow_values->BA_diag[jindexer].Yload[0][1] = bus[jindexer].full_Y_load[1];
								powerflow_values->BA_diag[jindexer].Yload[1][0] = -bus[jindexer].full_Y_load[3];
								powerflow_values->BA_diag[jindexer].Yload[1][1] = -bus[jindexer].full_Y_load[4];
							}
						}
					}//End full_Y_load not NULL
				}//End FPI provisions

				//Loop through and get sizes - make sure not a PV bus (not sure how would be)
				if (bus[jindexer].type != 1)
				{
					for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
					{
						for (kindex=0; kindex<powerflow_values->BA_diag[jindexer].size; kindex++)
						{
							if (NR_solver_algorithm == NRM_TCIM)
							{
								if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() != 0) && (jindex!=kindex))
									powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed += 1;
								if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() != 0) && (jindex!=kindex))
									powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed += 1;
							}
							else	//Implies FPIM
							{
								//Calculate matrix index
								mat_temp_index = jindex*3+kindex;

								if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() != 0) || ((powerflow_values->BA_diag[jindexer].Yload[jindex][kindex]).Re() !=0))
									powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed += 1;
								if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() != 0) || ((powerflow_values->BA_diag[jindexer].Yload[jindex][kindex]).Im() !=0))
									powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed += 1;
							}	//End FPIM
						}
					}
				}//End not a PV bus
			}
			//Default else -- not a valid island anyways
		}//End Size estimator

		//Loop through the islands again
		for (island_loop_index=0; island_loop_index<NR_islands_detected; island_loop_index++)
		{
			if (powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed == nullptr)
			{
				powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed = (Y_NR *)gl_malloc((powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed*2) *sizeof(Y_NR));   //powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed store the row,column and value of the fixed part of the diagonal PQ bus elements of 6n*6n Y_NR matrix.

				//Make sure it worked
				if (powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed == nullptr)
					GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

				//Update the max size
				powerflow_values->island_matrix_values[island_loop_index].max_size_diag_fixed = powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed;
			}
			else if (powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed > powerflow_values->island_matrix_values[island_loop_index].max_size_diag_fixed)		//Something changed and we are bigger!!
			{
				//Destroy us!
				gl_free(powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed);

				//Rebuild us, we have the technology
				powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed = (Y_NR *)gl_malloc((powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed*2) *sizeof(Y_NR));

				//Make sure it worked
				if (powerflow_values->island_matrix_values[island_loop_index].Y_diag_fixed == nullptr)
					GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

				//Store the new size
				powerflow_values->island_matrix_values[island_loop_index].max_size_diag_fixed = powerflow_values->island_matrix_values[island_loop_index].size_diag_fixed;

				//Flag for a reallocation
				powerflow_values->island_matrix_values[island_loop_index].NR_realloc_needed = true;
			}

			//Zero the accumulator for the next section, while we're in here
			powerflow_values->island_matrix_values[island_loop_index].indexer = 0;
		}//End island loop routine

		for (jindexer=0; jindexer<bus_count;jindexer++)	//Parse through bus list
		{
			//Extract our island reference
			island_index_val = bus[jindexer].island_number;

			//If we're unallocated, just skip us
			if (island_index_val == -1)
			{
				continue;
			}
			//Default else -- proceed

			//Mode check
			if (NR_solver_algorithm == NRM_FPI)
			{
				if ((bus[jindexer].type > 1) && bus[jindexer].swing_functions_enabled)
				{
					continue;	//Skip us - won't be part of the fixed diagonal
				}
			}
			//Defauilt TCIM - keep

			//Wrapper for both - no PV buses supported (does this even need to be checked?)
			if (bus[jindexer].type != 1)	//Not PV
			{
				//Populate matrix - TCIM neglects main diagonal, so handle differently
				if (NR_solver_algorithm == NRM_TCIM)
				{
					for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
					{
						for (kindex=0; kindex<powerflow_values->BA_diag[jindexer].size; kindex++)
						{
							if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() != 0) && (jindex!=kindex))
							{
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im();
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -(powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im();
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() != 0) && (jindex!=kindex))
							{
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re();
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re();
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}
						}
					}
				}//End TCIM
				else	//Implies FPI
				{
					for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
					{
						for (kindex=0; kindex<powerflow_values->BA_diag[jindexer].size; kindex++)
						{
							if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() != 0) || (powerflow_values->BA_diag[jindexer].Yload[jindex][kindex].Im() != 0))
							{
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() + powerflow_values->BA_diag[jindexer].Yload[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = -((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() + powerflow_values->BA_diag[jindexer].Yload[jindex][kindex].Im());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}

							if (((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() != 0) || (powerflow_values->BA_diag[jindexer].Yload[jindex][kindex].Re() != 0))
							{
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() + powerflow_values->BA_diag[jindexer].Yload[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;

								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex +powerflow_values->BA_diag[jindexer].size;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex;
								powerflow_values->island_matrix_values[island_index_val].Y_diag_fixed[powerflow_values->island_matrix_values[island_index_val].indexer].Y_value = ((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() + powerflow_values->BA_diag[jindexer].Yload[jindex][kindex].Re());
								powerflow_values->island_matrix_values[island_index_val].indexer += 1;
							}
						}
					}
				}
			}//End not PV bus
			//Default else - is a PV bus somehow, but skipped
		}//End bus parse for fixed diagonal
	}//End admittance update - fixed diagonal or load change

	//Deflag the load change, regardless
	NR_FPI_imp_load_change = false;
}

//Performs the load calculation portions of the current injection or Jacobian update
//jacobian_pass should be set to true for the a,b,c, and d updates
// For first approach, working on system load at each bus for current injection
// For the second approach, calculate the elements of a,b,c,d in equations(14),(15),(16),(17).
void compute_load_values(unsigned int bus_count, BUSDATA *bus, NR_SOLVER_STRUCT *powerflow_values, bool jacobian_pass, int island_number)
{
	unsigned int indexer;
	double adjust_nominal_voltage_val, adjust_nominal_voltaged_val;
	double tempPbus, tempQbus;
	double adjust_temp_voltage_mag[6];
	gld::complex adjust_temp_nominal_voltage[6], adjusted_constant_current[6];
	gld::complex delta_current[3], voltageDel[3], undeltacurr[3];
	gld::complex temp_current[3], temp_store[3];
	char jindex, temp_index, temp_index_b;
	STATUS temp_status;

	//Loop through the buses
	for (indexer=0; indexer<bus_count; indexer++)
	{
		//See if we're relevant to the island of interest
		if (bus[indexer].island_number == island_number)
		{
			//See if we have a load update function to call
			if (bus[indexer].LoadUpdateFxn != nullptr)
			{
				//Call the function
				temp_status = ((STATUS (*)(OBJECT *))(*bus[indexer].LoadUpdateFxn))(bus[indexer].obj);

				//Make sure it worked
				if (temp_status == FAILED)
				{
					GL_THROW("NR: Load update failed for device %s",bus[indexer].obj->name ? bus[indexer].obj->name : "Unnamed");
					/*  TROUBLESHOOT
					While attempting to perform the load update function call, something failed.  Please try again.
					If the error persists, please submit your code and a bug report via the ticketing system.
					*/
				}
				//Default else - it worked
			}

			if ((bus[indexer].phases & 0x08) == 0x08)	//Delta connected node
			{
				//Populate the values for constant current -- adjusted to maintain PF (used to be deltamode-only)
				//Create nominal magnitudes
				adjust_nominal_voltage_val = bus[indexer].volt_base * sqrt(3.0);

				//Create the nominal voltage vectors
				adjust_temp_nominal_voltage[0].SetPolar(adjust_nominal_voltage_val,PI/6.0);
				adjust_temp_nominal_voltage[1].SetPolar(adjust_nominal_voltage_val,-1.0*PI/2.0);
				adjust_temp_nominal_voltage[2].SetPolar(adjust_nominal_voltage_val,5.0*PI/6.0);

				//Compute delta voltages
				voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];
				voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];
				voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

				//Get magnitudes of all
				adjust_temp_voltage_mag[0] = voltageDel[0].Mag();
				adjust_temp_voltage_mag[1] = voltageDel[1].Mag();
				adjust_temp_voltage_mag[2] = voltageDel[2].Mag();

				//Start adjustments - AB
				if ((bus[indexer].I[0] != 0.0) && (adjust_temp_voltage_mag[0] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[0] = ~(adjust_temp_nominal_voltage[0] * ~bus[indexer].I[0] * adjust_temp_voltage_mag[0] / (voltageDel[0] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[0] = gld::complex(0.0,0.0);
				}

				//Start adjustments - BC
				if ((bus[indexer].I[1] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[1] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].I[1] * adjust_temp_voltage_mag[1] / (voltageDel[1] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[1] = gld::complex(0.0,0.0);
				}

				//Start adjustments - CA
				if ((bus[indexer].I[2] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[2] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].I[2] * adjust_temp_voltage_mag[2] / (voltageDel[2] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[2] = gld::complex(0.0,0.0);
				}

				//See if we have any "different children"
				if ((bus[indexer].phases & 0x10) == 0x10)
				{
					//Create nominal magnitudes
					adjust_nominal_voltage_val = bus[indexer].volt_base;

					//Create the nominal voltage vectors
					adjust_temp_nominal_voltage[3].SetPolar(bus[indexer].volt_base,0.0);
					adjust_temp_nominal_voltage[4].SetPolar(bus[indexer].volt_base,-2.0*PI/3.0);
					adjust_temp_nominal_voltage[5].SetPolar(bus[indexer].volt_base,2.0*PI/3.0);

					//Get magnitudes of all
					adjust_temp_voltage_mag[3] = bus[indexer].V[0].Mag();
					adjust_temp_voltage_mag[4] = bus[indexer].V[1].Mag();
					adjust_temp_voltage_mag[5] = bus[indexer].V[2].Mag();

					//Start adjustments - A
					if ((bus[indexer].extra_var[6] != 0.0) && (adjust_temp_voltage_mag[3] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[3] = ~(adjust_temp_nominal_voltage[3] * ~bus[indexer].extra_var[6] * adjust_temp_voltage_mag[3] / (bus[indexer].V[0] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[3] = gld::complex(0.0,0.0);
					}

					//Start adjustments - B
					if ((bus[indexer].extra_var[7] != 0.0) && (adjust_temp_voltage_mag[4] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[4] = ~(adjust_temp_nominal_voltage[4] * ~bus[indexer].extra_var[7] * adjust_temp_voltage_mag[4] / (bus[indexer].V[1] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[4] = gld::complex(0.0,0.0);
					}

					//Start adjustments - C
					if ((bus[indexer].extra_var[8] != 0.0) && (adjust_temp_voltage_mag[5] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[5] = ~(adjust_temp_nominal_voltage[5] * ~bus[indexer].extra_var[8] * adjust_temp_voltage_mag[5] / (bus[indexer].V[2] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[5] = gld::complex(0.0,0.0);
					}
				}
				else	//Nope
				{
					//Set to zero, just cause
					adjusted_constant_current[3] = gld::complex(0.0,0.0);
					adjusted_constant_current[4] = gld::complex(0.0,0.0);
					adjusted_constant_current[5] = gld::complex(0.0,0.0);
				}

				//Delta components - populate according to what is there
				if ((bus[indexer].phases & 0x06) == 0x06)	//Check for AB
				{
					//Voltage calculations
					voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];

					//Power - convert to a current (uses less iterations this way)
					delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].S[0]/voltageDel[0]);

					if (NR_solver_algorithm == NRM_TCIM)
					{
						//Convert delta connected load to appropriate Wye
						delta_current[0] += voltageDel[0] * (bus[indexer].Y[0]);
					}
				}
				else
				{
					//Zero values - they shouldn't be used anyhow
					voltageDel[0] = gld::complex(0.0,0.0);
					delta_current[0] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x03) == 0x03)	//Check for BC
				{
					//Voltage calculations
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

					//Power - convert to a current (uses less iterations this way)
					delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S[1]/voltageDel[1]);

					if (NR_solver_algorithm == NRM_TCIM)
					{
						//Convert delta connected load to appropriate Wye
						delta_current[1] += voltageDel[1] * (bus[indexer].Y[1]);
					}
				}
				else
				{
					//Zero unused
					voltageDel[1] = gld::complex(0.0,0.0);
					delta_current[1] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x05) == 0x05)	//Check for CA
				{
					//Voltage calculations
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Power - convert to a current (uses less iterations this way)
					delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S[2]/voltageDel[2]);

					if (NR_solver_algorithm == NRM_TCIM)
					{
						//Convert delta connected load to appropriate Wye
						delta_current[2] += voltageDel[2] * (bus[indexer].Y[2]);
					}
				}
				else
				{
					//Zero unused
					voltageDel[2] = gld::complex(0.0,0.0);
					delta_current[2] = gld::complex(0.0,0.0);
				}

				//Convert delta-current into a phase current, where appropriate - reuse temp variable
				//Everything will be accumulated into the "current" field for ease (including differents)
				if ((bus[indexer].phases & 0x04) == 0x04)	//Has a phase A
				{
					undeltacurr[0]=(adjusted_constant_current[0]+delta_current[0])-(adjusted_constant_current[2]+delta_current[2]);

					//Check for "different" children and apply them, as well
					if ((bus[indexer].phases & 0x10) == 0x10)	//We do, so they must be Wye-connected
					{
						//Power values
						undeltacurr[0] += (bus[indexer].V[0] == 0) ? 0 : ~(bus[indexer].extra_var[0]/bus[indexer].V[0]);

						//Shunt values
						if (NR_solver_algorithm == NRM_TCIM)
						{
							undeltacurr[0] += bus[indexer].extra_var[3]*bus[indexer].V[0];
						}
						//Default else - FPI handles impedance/admittance directly

						//Current values
						undeltacurr[0] += adjusted_constant_current[3];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[0] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x02) == 0x02)	//Has a phase B
				{
					undeltacurr[1]=(adjusted_constant_current[1]+delta_current[1])-(adjusted_constant_current[0]+delta_current[0]);

					//Check for "different" children and apply them, as well
					if ((bus[indexer].phases & 0x10) == 0x10)	//We do, so they must be Wye-connected
					{
						//Power values
						undeltacurr[1] += (bus[indexer].V[1] == 0) ? 0 : ~(bus[indexer].extra_var[1]/bus[indexer].V[1]);

						//Shunt values
						if (NR_solver_algorithm == NRM_TCIM)
						{
							undeltacurr[1] += bus[indexer].extra_var[4]*bus[indexer].V[1];
						}
						//Default else - FPI handles impedance/admittance directly

						//Current values
						undeltacurr[1] += adjusted_constant_current[4];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[1] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x01) == 0x01)	//Has a phase C
				{
					undeltacurr[2]=(adjusted_constant_current[2]+delta_current[2])-(adjusted_constant_current[1]+delta_current[1]);

					//Check for "different" children and apply them, as well
					if ((bus[indexer].phases & 0x10) == 0x10)		//We do, so they must be Wye-connected
					{
						//Power values
						undeltacurr[2] += (bus[indexer].V[2] == 0) ? 0 : ~(bus[indexer].extra_var[2]/bus[indexer].V[2]);

						//Shunt values
						if (NR_solver_algorithm == NRM_TCIM)
						{
							undeltacurr[2] += bus[indexer].extra_var[5]*bus[indexer].V[2];
						}
						//Default else - FPI handles impedance/admittance directly

						//Current values
						undeltacurr[2] += adjusted_constant_current[5];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[2] = gld::complex(0.0,0.0);
				}

				//Provide updates to relevant phases
				//only compute and store phases that exist (make top heavy)
				temp_index = -1;
				temp_index_b = -1;

				for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)
				{
					switch(bus[indexer].phases & 0x07) {
						case 0x01:	//C
							{
								temp_index=0;
								temp_index_b=2;
								break;
							}
						case 0x02:	//B
							{
								temp_index=0;
								temp_index_b=1;
								break;
							}
						case 0x03:	//BC
							{
								if (jindex==0)	//B
								{
									temp_index=0;
									temp_index_b=1;
								}
								else			//C
								{
									temp_index=1;
									temp_index_b=2;
								}
								break;
							}
						case 0x04:	//A
							{
								temp_index=0;
								temp_index_b=0;
								break;
							}
						case 0x05:	//AC
							{
								if (jindex==0)	//A
								{
									temp_index=0;
									temp_index_b=0;
								}
								else			//C
								{
									temp_index=1;
									temp_index_b=2;
								}
								break;
							}
						case 0x06:	//AB
						case 0x07:	//ABC
							{
								temp_index=jindex;
								temp_index_b=jindex;
								break;
							}
						default:
							break;
					}//end case

					//Update methods
					if (NR_solver_algorithm == NRM_TCIM)
					{
						if (!jacobian_pass)	//current-injection updates
						{
							if ((temp_index==-1) || (temp_index_b==-1))
							{
								GL_THROW("NR: A scheduled power update element failed.");
								//Defined below
							}

							//Real power calculations
							tempPbus = (undeltacurr[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im();	// Real power portion of Constant current component multiply the magnitude of bus voltage
							bus[indexer].PL[temp_index] = tempPbus;	//Real power portion - all is current based

							//Reactive load calculations
							tempQbus = (undeltacurr[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Re();	// Reactive power portion of Constant current component multiply the magnitude of bus voltage
							bus[indexer].QL[temp_index] = tempQbus;	//Reactive power portion - all is current based
						}
						else	//Jacobian-type update
						{
							if ((temp_index==-1) || (temp_index_b==-1))
							{
								GL_THROW("NR: A Jacobian update element failed.");
								//Defined below
							}

							if ((bus[indexer].V[temp_index_b]).Mag()!=0)
							{
								bus[indexer].Jacob_A[temp_index] = ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(37) - no power term needed
								bus[indexer].Jacob_B[temp_index] = -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() + (undeltacurr[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(38) - no power term needed
								bus[indexer].Jacob_C[temp_index] =((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(39) - no power term needed
								bus[indexer].Jacob_D[temp_index] = ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() - (undeltacurr[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(40) - no power term needed
							}
							else	//Zero voltage = only impedance is valid (others get divided by VMag, so are IND) - not entirely sure how this gets in here anyhow
							{
								bus[indexer].Jacob_A[temp_index] = -1e-4;	//Small offset to avoid singularities (if impedance is zero too)
								bus[indexer].Jacob_B[temp_index] = -1e-4;
								bus[indexer].Jacob_C[temp_index] = -1e-4;
								bus[indexer].Jacob_D[temp_index] = -1e-4;
							}
						}//End specific bus update method
					}//End TCIM Update
					else	//FPI
					{
						if ((temp_index==-1) || (temp_index_b==-1))
						{
							GL_THROW("NR: A Jacobian update element failed.");
							//Defined below
						}

						//Include contributions - handled above
						bus[indexer].FPI_current[temp_index] = undeltacurr[temp_index_b];
					}
				}//End phase traversion
			}//end delta-connected load
			else if	((bus[indexer].phases & 0x80) == 0x80)	//Split phase computations
			{
				//Convert it all back to current (easiest to handle)
				//Get V12 first
				voltageDel[0] = bus[indexer].V[0] + bus[indexer].V[1];

				//Start with the currents (just put them in)
				temp_current[0] = bus[indexer].I[0];
				temp_current[1] = bus[indexer].I[1];
				temp_current[2] = *bus[indexer].extra_var; //current12 is not part of the standard current array

				//Add in unrotated, if necessary
				//Same note as above.  With exception to house currents, rotational correction happened elsewhere (due to triplex being how it is)
				if (bus[indexer].prerot_I[0] != 0.0)
					temp_current[0] += bus[indexer].prerot_I[0];

				if (bus[indexer].prerot_I[1] != 0.0)
					temp_current[1] += bus[indexer].prerot_I[1];

				if (bus[indexer].prerot_I[2] != 0.0)
					temp_current[2] += bus[indexer].prerot_I[2];

				//Now add in power contributions
				temp_current[0] += bus[indexer].V[0] == 0.0 ? 0.0 : ~(bus[indexer].S[0]/bus[indexer].V[0]);
				temp_current[1] += bus[indexer].V[1] == 0.0 ? 0.0 : ~(bus[indexer].S[1]/bus[indexer].V[1]);
				temp_current[2] += voltageDel[0] == 0.0 ? 0.0 : ~(bus[indexer].S[2]/voltageDel[0]);

				//Last, but not least, admittance/impedance contributions - but only if TCIM
				if (NR_solver_algorithm == NRM_TCIM)
				{
					temp_current[0] += bus[indexer].Y[0]*bus[indexer].V[0];
					temp_current[1] += bus[indexer].Y[1]*bus[indexer].V[1];
					temp_current[2] += bus[indexer].Y[2]*voltageDel[0];
				}

				//See if we are a house-connected node, if so, adjust and add in those values as well
				if ((bus[indexer].phases & 0x40) == 0x40)
				{
					//Update phase adjustments
					temp_store[0].SetPolar(1.0,bus[indexer].V[0].Arg());	//Pull phase of V1
					temp_store[1].SetPolar(1.0,bus[indexer].V[1].Arg());	//Pull phase of V2
					temp_store[2].SetPolar(1.0,voltageDel[0].Arg());		//Pull phase of V12

					//Update these current contributions (use delta current variable, it isn't used in here anyways)
					delta_current[0] = bus[indexer].house_var[0]/(~temp_store[0]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
					delta_current[1] = bus[indexer].house_var[1]/(~temp_store[1]);
					delta_current[2] = bus[indexer].house_var[2]/(~temp_store[2]);

					//Now add it into the current contributions
					temp_current[0] += delta_current[0];
					temp_current[1] += delta_current[1];
					temp_current[2] += delta_current[2];
				}//End house-attached splitphase

				if (NR_solver_algorithm == NRM_TCIM)
				{
					if (!jacobian_pass)	//Current injection update
					{
						//Convert 'em to line currents, then make power (to be consistent with others)
						temp_store[0] = bus[indexer].V[0]*~(temp_current[0] + temp_current[2]);
						temp_store[1] = bus[indexer].V[1]*~(-temp_current[1] - temp_current[2]);

						//Update the stored values
						bus[indexer].PL[0] = temp_store[0].Re();
						bus[indexer].QL[0] = temp_store[0].Im();

						bus[indexer].PL[1] = temp_store[1].Re();
						bus[indexer].QL[1] = temp_store[1].Im();
					}
					else	//Jacobian update
					{
						//Convert 'em to line currents - they need to be negated (due to the convention from earlier)
						temp_store[0] = -(temp_current[0] + temp_current[2]);
						temp_store[1] = -(-temp_current[1] - temp_current[2]);

						for (jindex=0; jindex<2; jindex++)
						{
							if ((bus[indexer].V[jindex]).Mag()!=0)	//Only current
							{
								bus[indexer].Jacob_A[jindex] = ((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Re() + (temp_store[jindex]).Im() *gld::pow((bus[indexer].V[jindex]).Im(),2))/gld::pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(37)
								bus[indexer].Jacob_B[jindex] = -((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Im() + (temp_store[jindex]).Re() *gld::pow((bus[indexer].V[jindex]).Re(),2))/gld::pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(38)
								bus[indexer].Jacob_C[jindex] =((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Im() - (temp_store[jindex]).Re() *gld::pow((bus[indexer].V[jindex]).Im(),2))/gld::pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(39)
								bus[indexer].Jacob_D[jindex] = ((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Re() - (temp_store[jindex]).Im() *gld::pow((bus[indexer].V[jindex]).Re(),2))/gld::pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(40)
							}
							else
							{
								bus[indexer].Jacob_A[jindex]=  -1e-4;	//Put very small to avoid singularity issues
								bus[indexer].Jacob_B[jindex]=  -1e-4;
								bus[indexer].Jacob_C[jindex]=  -1e-4;
								bus[indexer].Jacob_D[jindex]=  -1e-4;
							}
						}

						//Zero the last elements, just to be safe (shouldn't be an issue, but who knows)
						bus[indexer].Jacob_A[2] = 0.0;
						bus[indexer].Jacob_B[2] = 0.0;
						bus[indexer].Jacob_C[2] = 0.0;
						bus[indexer].Jacob_D[2] = 0.0;
					}//End specific update type
				}//End TCIM Update
				else	//Assumes FPI
				{
					//Convert 'em to line currents
					temp_store[0] = (temp_current[0] + temp_current[2]);
					temp_store[1] = (-temp_current[1] - temp_current[2]);

					//SPCT end node and its wonkiness makes this different in FPI
					if ((bus[indexer].phases & 0x20) == 0x20)
					{
						//Store update - negated for wonky current convention
						bus[indexer].FPI_current[0] = -temp_store[0];
						bus[indexer].FPI_current[1] = -temp_store[1];
					}
					else	//Normal
					{
						//Store update - negated for wonky current convention
						bus[indexer].FPI_current[0] = temp_store[0];
						bus[indexer].FPI_current[1] = temp_store[1];
					}
				}
			}//end split-phase connected
			else	//Wye-connected system/load
			{
				//Populate the values for constant current -- adjusted to maintain PF (used to be deltamode-only)
				//Create nominal magnitudes
				adjust_nominal_voltage_val = bus[indexer].volt_base;

				//Create the nominal voltage vectors
				adjust_temp_nominal_voltage[3].SetPolar(bus[indexer].volt_base,0.0);
				adjust_temp_nominal_voltage[4].SetPolar(bus[indexer].volt_base,-2.0*PI/3.0);
				adjust_temp_nominal_voltage[5].SetPolar(bus[indexer].volt_base,2.0*PI/3.0);

				//Get magnitudes of all
				adjust_temp_voltage_mag[3] = bus[indexer].V[0].Mag();
				adjust_temp_voltage_mag[4] = bus[indexer].V[1].Mag();
				adjust_temp_voltage_mag[5] = bus[indexer].V[2].Mag();

				//Start adjustments - A
				if ((bus[indexer].I[0] != 0.0) && (adjust_temp_voltage_mag[3] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[0] = ~(adjust_temp_nominal_voltage[3] * ~bus[indexer].I[0] * adjust_temp_voltage_mag[3] / (bus[indexer].V[0] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[0] = gld::complex(0.0,0.0);
				}

				//Start adjustments - B
				if ((bus[indexer].I[1] != 0.0) && (adjust_temp_voltage_mag[4] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[1] = ~(adjust_temp_nominal_voltage[4] * ~bus[indexer].I[1] * adjust_temp_voltage_mag[4] / (bus[indexer].V[1] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[1] = gld::complex(0.0,0.0);
				}

				//Start adjustments - C
				if ((bus[indexer].I[2] != 0.0) && (adjust_temp_voltage_mag[5] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[2] = ~(adjust_temp_nominal_voltage[5] * ~bus[indexer].I[2] * adjust_temp_voltage_mag[5] / (bus[indexer].V[2] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[2] = gld::complex(0.0,0.0);
				}

				if (bus[indexer].prerot_I[0] != 0.0)
					adjusted_constant_current[0] += bus[indexer].prerot_I[0];

				if (bus[indexer].prerot_I[1] != 0.0)
					adjusted_constant_current[1] += bus[indexer].prerot_I[1];

				if (bus[indexer].prerot_I[2] != 0.0)
					adjusted_constant_current[2] += bus[indexer].prerot_I[2];

				//See if we have any "different children"
				if ((bus[indexer].phases & 0x10) == 0x10)
				{
					//Create nominal magnitudes
					adjust_nominal_voltage_val = bus[indexer].volt_base * sqrt(3.0);

					//Create the nominal voltage vectors
					adjust_temp_nominal_voltage[0].SetPolar(adjust_nominal_voltage_val,PI/6.0);
					adjust_temp_nominal_voltage[1].SetPolar(adjust_nominal_voltage_val,-1.0*PI/2.0);
					adjust_temp_nominal_voltage[2].SetPolar(adjust_nominal_voltage_val,5.0*PI/6.0);

					//Compute delta voltages
					voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Get magnitudes of all
					adjust_temp_voltage_mag[0] = voltageDel[0].Mag();
					adjust_temp_voltage_mag[1] = voltageDel[1].Mag();
					adjust_temp_voltage_mag[2] = voltageDel[2].Mag();

					//Start adjustments - AB
					if ((bus[indexer].extra_var[6] != 0.0) && (adjust_temp_voltage_mag[0] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[3] = ~(adjust_temp_nominal_voltage[0] * ~bus[indexer].extra_var[6] * adjust_temp_voltage_mag[0] / (voltageDel[0] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[3] = gld::complex(0.0,0.0);
					}

					//Start adjustments - BC
					if ((bus[indexer].extra_var[7] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[4] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].extra_var[7] * adjust_temp_voltage_mag[1] / (voltageDel[1] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[4] = gld::complex(0.0,0.0);
					}

					//Start adjustments - CA
					if ((bus[indexer].extra_var[8] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[5] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].extra_var[8] * adjust_temp_voltage_mag[2] / (voltageDel[2] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[5] = gld::complex(0.0,0.0);
					}
				}
				else	//Nope
				{
					//Set to zero, just cause
					adjusted_constant_current[3] = gld::complex(0.0,0.0);
					adjusted_constant_current[4] = gld::complex(0.0,0.0);
					adjusted_constant_current[5] = gld::complex(0.0,0.0);
				}

				//For Wye-connected, only compute and store phases that exist (make top heavy)
				temp_index = -1;
				temp_index_b = -1;

				if ((bus[indexer].phases & 0x10) == 0x10)	//"Different" child load - in this case it must be delta - also must be three phase (just because that's how I forced it to be implemented)
				{											//Calculate all the deltas to wyes in advance (otherwise they'll get repeated)
					//Make sure phase combinations exist
					if ((bus[indexer].phases & 0x06) == 0x06)	//Has A-B
					{
						//Delta voltages
						voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];

						//Power - put into a current value (iterates less this way)
						delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].extra_var[0]/voltageDel[0]);

						//Convert delta connected load to appropriate Wye
						if (NR_solver_algorithm == NRM_TCIM)
						{
							delta_current[0] += voltageDel[0] * (bus[indexer].extra_var[3]);
						}
						//Default else - FPI handles impedance/admittance directly
					}
					else
					{
						//Zero it, for good measure
						voltageDel[0] = gld::complex(0.0,0.0);
						delta_current[0] = gld::complex(0.0,0.0);
					}

					//Check for BC
					if ((bus[indexer].phases & 0x03) == 0x03)	//Has B-C
					{
						//Delta voltages
						voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

						//Power - put into a current value (iterates less this way)
						delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].extra_var[1]/voltageDel[1]);

						//Convert delta connected load to appropriate Wye
						if (NR_solver_algorithm == NRM_TCIM)
						{
							delta_current[1] += voltageDel[1] * (bus[indexer].extra_var[4]);
						}
						//Default else - FPI handles impedance/admittance directly
					}
					else
					{
						//Zero it, for good measure
						voltageDel[1] = gld::complex(0.0,0.0);
						delta_current[1] = gld::complex(0.0,0.0);
					}

					//Check for CA
					if ((bus[indexer].phases & 0x05) == 0x05)	//Has C-A
					{
						//Delta voltages
						voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

						//Power - put into a current value (iterates less this way)
						delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].extra_var[2]/voltageDel[2]);

						//Convert delta connected load to appropriate Wye
						if (NR_solver_algorithm == NRM_TCIM)
						{
							delta_current[2] += voltageDel[2] * (bus[indexer].extra_var[5]);
						}
						//Default else - FPI handles impedance/admittance directly
					}
					else
					{
						//Zero it, for good measure
						voltageDel[2] = gld::complex(0.0,0.0);
						delta_current[2] = gld::complex(0.0,0.0);
					}

					//Convert delta-current into a phase current - reuse temp variable
					undeltacurr[0]=(adjusted_constant_current[3]+delta_current[0])-(adjusted_constant_current[5]+delta_current[2]);
					undeltacurr[1]=(adjusted_constant_current[4]+delta_current[1])-(adjusted_constant_current[3]+delta_current[0]);
					undeltacurr[2]=(adjusted_constant_current[5]+delta_current[2])-(adjusted_constant_current[4]+delta_current[1]);
				}
				else	//zero the variable so we don't have excessive ifs
				{
					undeltacurr[0] = undeltacurr[1] = undeltacurr[2] = gld::complex(0.0,0.0);	//Zero it
				}

				for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)
				{
					switch(bus[indexer].phases & 0x07) {
						case 0x01:	//C
							{
								temp_index=0;
								temp_index_b=2;
								break;
							}
						case 0x02:	//B
							{
								temp_index=0;
								temp_index_b=1;
								break;
							}
						case 0x03:	//BC
							{
								if (jindex==0)	//B
								{
									temp_index=0;
									temp_index_b=1;
								}
								else			//C
								{
									temp_index=1;
									temp_index_b=2;
								}
								break;
							}
						case 0x04:	//A
							{
								temp_index=0;
								temp_index_b=0;
								break;
							}
						case 0x05:	//AC
							{
								if (jindex==0)	//A
								{
									temp_index=0;
									temp_index_b=0;
								}
								else			//C
								{
									temp_index=1;
									temp_index_b=2;
								}
								break;
							}
						case 0x06:	//AB
						case 0x07:	//ABC
							{
								temp_index=jindex;
								temp_index_b=jindex;
								break;
							}
						default:
							break;
					}//end case

					if (NR_solver_algorithm == NRM_TCIM)
					{
						if (!jacobian_pass)	//Current injection pass
						{
							if ((temp_index==-1) || (temp_index_b==-1))
							{
								GL_THROW("NR: A scheduled power update element failed.");
								/*  TROUBLESHOOT
								While attempting to calculate the scheduled portions of the
								attached loads, an update failed to process correctly.
								Submit you code and a bug report using the trac website.
								*/
							}

							//Perform the power calculation
							tempPbus = (bus[indexer].S[temp_index_b]).Re();									// Real power portion of constant power portion
							tempPbus += (adjusted_constant_current[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() + (adjusted_constant_current[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im();	// Real power portion of Constant current component multiply the magnitude of bus voltage
							tempPbus += (undeltacurr[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im();	// Real power portion of Constant current from "different" children
							tempPbus += (bus[indexer].Y[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() + (bus[indexer].Y[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im();	// Real power portion of Constant impedance component multiply the square of the magnitude of bus voltage
							bus[indexer].PL[temp_index] = tempPbus;	//Real power portion


							tempQbus = (bus[indexer].S[temp_index_b]).Im();									// Reactive power portion of constant power portion
							tempQbus += (adjusted_constant_current[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Im() - (adjusted_constant_current[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Re();	// Reactive power portion of Constant current component multiply the magnitude of bus voltage
							tempQbus += (undeltacurr[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Re();	// Reactive power portion of Constant current from "different" children
							tempQbus += -(bus[indexer].Y[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im() - (bus[indexer].Y[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re();	// Reactive power portion of Constant impedance component multiply the square of the magnitude of bus voltage
							bus[indexer].QL[temp_index] = tempQbus;	//Reactive power portion
						}
						else	//Jacobian update pass
						{
							if ((temp_index==-1) || (temp_index_b==-1))
							{
								GL_THROW("NR: A Jacobian update element failed.");
								/*  TROUBLESHOOT
								While attempting to calculate the "dynamic" portions of the
								Jacobian matrix that encompass attached loads, an update failed to process correctly.
								Submit you code and a bug report using the trac website.
								*/
							}

							if ((bus[indexer].V[temp_index_b]).Mag()!=0)
							{
								bus[indexer].Jacob_A[temp_index] = ((bus[indexer].S[temp_index_b]).Im() * (gld::pow((bus[indexer].V[temp_index_b]).Re(),2) - gld::pow((bus[indexer].V[temp_index_b]).Im(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Re())/gld::pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(37)
								bus[indexer].Jacob_A[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Re() + (adjusted_constant_current[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3) + (bus[indexer].Y[temp_index_b]).Im();// second part of equation(37)
								bus[indexer].Jacob_A[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// current part of equation (37) - Handles "different" children

								bus[indexer].Jacob_B[temp_index] = ((bus[indexer].S[temp_index_b]).Re() * (gld::pow((bus[indexer].V[temp_index_b]).Re(),2) - gld::pow((bus[indexer].V[temp_index_b]).Im(),2)) + 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Im())/gld::pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(38)
								bus[indexer].Jacob_B[temp_index] += -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Im() + (adjusted_constant_current[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Re();// second part of equation(38)
								bus[indexer].Jacob_B[temp_index] += -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() + (undeltacurr[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// current part of equation(38) - Handles "different" children

								bus[indexer].Jacob_C[temp_index] = ((bus[indexer].S[temp_index_b]).Re() * (gld::pow((bus[indexer].V[temp_index_b]).Im(),2) - gld::pow((bus[indexer].V[temp_index_b]).Re(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Im())/gld::pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(39)
								bus[indexer].Jacob_C[temp_index] +=((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Im() - (adjusted_constant_current[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Re();// second part of equation(39)
								bus[indexer].Jacob_C[temp_index] +=((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// Current part of equation(39) - Handles "different" children

								bus[indexer].Jacob_D[temp_index] = ((bus[indexer].S[temp_index_b]).Im() * (gld::pow((bus[indexer].V[temp_index_b]).Re(),2) - gld::pow((bus[indexer].V[temp_index_b]).Im(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Re())/gld::pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(40)
								bus[indexer].Jacob_D[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Re() - (adjusted_constant_current[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Im();// second part of equation(40)
								bus[indexer].Jacob_D[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() - (undeltacurr[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3);// Current part of equation(40) - Handles "different" children

							}
							else
							{
								bus[indexer].Jacob_A[temp_index]= (bus[indexer].Y[temp_index_b]).Im() - 1e-4;	//Small offset to avoid singularity issues
								bus[indexer].Jacob_B[temp_index]= -(bus[indexer].Y[temp_index_b]).Re() - 1e-4;
								bus[indexer].Jacob_C[temp_index]= -(bus[indexer].Y[temp_index_b]).Re() - 1e-4;
								bus[indexer].Jacob_D[temp_index]= -(bus[indexer].Y[temp_index_b]).Im() - 1e-4;
							}
						}//End of pass-specific bus updates
					}//End TCIM
					else	//Assumed FPIM
					{
						if ((temp_index==-1) || (temp_index_b==-1))
						{
							GL_THROW("NR: A scheduled power update element failed.");
							//Defined above
						}

						//Calculate the current contribution - currents
						bus[indexer].FPI_current[temp_index_b] = adjusted_constant_current[temp_index_b] + undeltacurr[temp_index_b];

						//Do Power, as relevant
						if ((bus[indexer].V[temp_index_b]).Mag()!=0)
						{
							bus[indexer].FPI_current[temp_index_b] += ~(bus[indexer].S[temp_index_b]/bus[indexer].V[temp_index_b]);
						}
					}
				}//End phase traversion - Wye
			}//End wye-connected load

			//Perform delta/wye explicit load updates -- no triplex
			if ((bus[indexer].phases & 0x80) != 0x80)	//Not triplex
			{
				//Delta components - populate according to what is there
				if ((bus[indexer].phases & 0x06) == 0x06)	//Check for AB
				{
					//Voltage calculations
					voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];

					//Power - convert to a current (uses less iterations this way)
					delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].S_dy[0]/voltageDel[0]);

					//Convert delta connected load to appropriate Wye - if TCIM
					if (NR_solver_algorithm == NRM_TCIM)
					{
						delta_current[0] += voltageDel[0] * (bus[indexer].Y_dy[0]);
					}
					//Default else - FPI handled directly
				}
				else
				{
					//Zero values - they shouldn't be used anyhow
					voltageDel[0] = gld::complex(0.0,0.0);
					delta_current[0] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x03) == 0x03)	//Check for BC
				{
					//Voltage calculations
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

					//Power - convert to a current (uses less iterations this way)
					delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S_dy[1]/voltageDel[1]);

					//Convert delta connected load to appropriate Wye - if TCIM
					if (NR_solver_algorithm == NRM_TCIM)
					{
						delta_current[1] += voltageDel[1] * (bus[indexer].Y_dy[1]);
					}
					//Default else - FPI handled directly
				}
				else
				{
					//Zero unused
					voltageDel[1] = gld::complex(0.0,0.0);
					delta_current[1] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x05) == 0x05)	//Check for CA
				{
					//Voltage calculations
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Power - convert to a current (uses less iterations this way)
					delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S_dy[2]/voltageDel[2]);

					//Convert delta connected load to appropriate Wye - if TCIM
					if (NR_solver_algorithm == NRM_TCIM)
					{
						delta_current[2] += voltageDel[2] * (bus[indexer].Y_dy[2]);
					}
					//Default else - FPI handled directly
				}
				else
				{
					//Zero unused
					voltageDel[2] = gld::complex(0.0,0.0);
					delta_current[2] = gld::complex(0.0,0.0);
				}

				//Populate the values for constant current -- adjusted to maintain PF (used to be deltamode-only)
				//Create line-line nominal magnitude
				adjust_nominal_voltage_val = bus[indexer].volt_base;
				adjust_nominal_voltaged_val = bus[indexer].volt_base * sqrt(3.0);

				//Create the nominal voltage vectors
				adjust_temp_nominal_voltage[0].SetPolar(adjust_nominal_voltaged_val,PI/6.0);
				adjust_temp_nominal_voltage[1].SetPolar(adjust_nominal_voltaged_val,-1.0*PI/2.0);
				adjust_temp_nominal_voltage[2].SetPolar(adjust_nominal_voltaged_val,5.0*PI/6.0);
				adjust_temp_nominal_voltage[3].SetPolar(adjust_nominal_voltage_val,0.0);
				adjust_temp_nominal_voltage[4].SetPolar(adjust_nominal_voltage_val,-2.0*PI/3.0);
				adjust_temp_nominal_voltage[5].SetPolar(adjust_nominal_voltage_val,2.0*PI/3.0);

				//Compute delta voltages
				voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];
				voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];
				voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

				//Get magnitudes of all
				adjust_temp_voltage_mag[0] = voltageDel[0].Mag();
				adjust_temp_voltage_mag[1] = voltageDel[1].Mag();
				adjust_temp_voltage_mag[2] = voltageDel[2].Mag();
				adjust_temp_voltage_mag[3] = bus[indexer].V[0].Mag();
				adjust_temp_voltage_mag[4] = bus[indexer].V[1].Mag();
				adjust_temp_voltage_mag[5] = bus[indexer].V[2].Mag();

				//Start adjustments - A
				if ((bus[indexer].I_dy[3] != 0.0) && (adjust_temp_voltage_mag[3] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[3] = ~(adjust_temp_nominal_voltage[3] * ~bus[indexer].I_dy[3] * adjust_temp_voltage_mag[3] / (bus[indexer].V[0] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[3] = gld::complex(0.0,0.0);
				}

				//Start adjustments - B
				if ((bus[indexer].I_dy[4] != 0.0) && (adjust_temp_voltage_mag[4] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[4] = ~(adjust_temp_nominal_voltage[4] * ~bus[indexer].I_dy[4] * adjust_temp_voltage_mag[4] / (bus[indexer].V[1] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[4] = gld::complex(0.0,0.0);
				}

				//Start adjustments - C
				if ((bus[indexer].I_dy[5] != 0.0) && (adjust_temp_voltage_mag[5] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[5] = ~(adjust_temp_nominal_voltage[5] * ~bus[indexer].I_dy[5] * adjust_temp_voltage_mag[5] / (bus[indexer].V[2] * adjust_nominal_voltage_val));
				}
				else
				{
					adjusted_constant_current[5] = gld::complex(0.0,0.0);
				}

				//Start adjustments - AB
				if ((bus[indexer].I_dy[0] != 0.0) && (adjust_temp_voltage_mag[0] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[0] = ~(adjust_temp_nominal_voltage[0] * ~bus[indexer].I_dy[0] * adjust_temp_voltage_mag[0] / (voltageDel[0] * adjust_nominal_voltaged_val));
				}
				else
				{
					adjusted_constant_current[0] = gld::complex(0.0,0.0);
				}

				//Start adjustments - BC
				if ((bus[indexer].I_dy[1] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[1] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].I_dy[1] * adjust_temp_voltage_mag[1] / (voltageDel[1] * adjust_nominal_voltaged_val));
				}
				else
				{
					adjusted_constant_current[1] = gld::complex(0.0,0.0);
				}

				//Start adjustments - CA
				if ((bus[indexer].I_dy[2] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
				{
					//calculate new value
					adjusted_constant_current[2] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].I_dy[2] * adjust_temp_voltage_mag[2] / (voltageDel[2] * adjust_nominal_voltaged_val));
				}
				else
				{
					adjusted_constant_current[2] = gld::complex(0.0,0.0);
				}

				//Convert delta-current into a phase current, where appropriate - reuse temp variable
				//Everything will be accumulated into the "current" field for ease (including differents)
				//Also handle wye currents in here (was a differently connected child code before)
				if ((bus[indexer].phases & 0x04) == 0x04)	//Has a phase A
				{
					undeltacurr[0]=(adjusted_constant_current[0]+delta_current[0])-(adjusted_constant_current[2]+delta_current[2]);

					//Apply explicit wye-connected loads

					//Power values
					undeltacurr[0] += (bus[indexer].V[0] == 0) ? 0 : ~(bus[indexer].S_dy[3]/bus[indexer].V[0]);

					//Shunt values - if TCIM
					if (NR_solver_algorithm == NRM_TCIM)
					{
						undeltacurr[0] += bus[indexer].Y_dy[3]*bus[indexer].V[0];
					}
					//Default else - FPI handled directly

					//Current values
					undeltacurr[0] += adjusted_constant_current[3];

					//Add any three-phase/non-triplex house contributions, if they exist
					if ((bus[indexer].phases & 0x40) == 0x40)
					{
						//Update phase adjustments - use the temp array (not really needed)
						temp_store[0].SetPolar(1.0,bus[indexer].V[0].Arg());

						//Update these current contributions
						undeltacurr[0] += bus[indexer].house_var[0]/(~temp_store[0]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
					}//End house-attached non-triplex
				}
				else
				{
					//Zero it, just in case
					undeltacurr[0] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x02) == 0x02)	//Has a phase B
				{
					undeltacurr[1]=(adjusted_constant_current[1]+delta_current[1])-(adjusted_constant_current[0]+delta_current[0]);

					//Apply explicit wye-connected loads

					//Power values
					undeltacurr[1] += (bus[indexer].V[1] == 0) ? 0 : ~(bus[indexer].S_dy[4]/bus[indexer].V[1]);

					//Shunt values - if TCIM
					if (NR_solver_algorithm == NRM_TCIM)
					{
						undeltacurr[1] += bus[indexer].Y_dy[4]*bus[indexer].V[1];
					}
					//Default else - FPI handled directly

					//Current values
					undeltacurr[1] += adjusted_constant_current[4];

					//Add any three-phase/non-triplex house contributions, if they exist
					if ((bus[indexer].phases & 0x40) == 0x40)
					{
						//Update phase adjustments - use the temp array (not really needed)
						temp_store[1].SetPolar(1.0,bus[indexer].V[1].Arg());

						//Update these current contributions
						undeltacurr[1] += bus[indexer].house_var[1]/(~temp_store[1]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
					}//End house-attached non-triplex
				}
				else
				{
					//Zero it, just in case
					undeltacurr[1] = gld::complex(0.0,0.0);
				}

				if ((bus[indexer].phases & 0x01) == 0x01)	//Has a phase C
				{
					undeltacurr[2]=(adjusted_constant_current[2]+delta_current[2])-(adjusted_constant_current[1]+delta_current[1]);

					//Apply explicit wye-connected loads

					//Power values
					undeltacurr[2] += (bus[indexer].V[2] == 0) ? 0 : ~(bus[indexer].S_dy[5]/bus[indexer].V[2]);

					//Shunt values - if TCIM
					if (NR_solver_algorithm == NRM_TCIM)
					{
						undeltacurr[2] += bus[indexer].Y_dy[5]*bus[indexer].V[2];
					}
					//Default else - FPI handled directly

					//Current values
					undeltacurr[2] += adjusted_constant_current[5];

					//Add any three-phase/non-triplex house contributions, if they exist
					if ((bus[indexer].phases & 0x40) == 0x40)
					{
						//Update phase adjustments - use the temp array (not really needed)
						temp_store[2].SetPolar(1.0,bus[indexer].V[2].Arg());

						//Update these current contributions
						undeltacurr[2] += bus[indexer].house_var[2]/(~temp_store[2]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
					}//End house-attached non-triplex
				}
				else
				{
					//Zero it, just in case
					undeltacurr[2] = gld::complex(0.0,0.0);
				}

				//Provide updates to relevant phases
				//only compute and store phases that exist (make top heavy)
				temp_index = -1;
				temp_index_b = -1;

				for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)
				{
					switch(bus[indexer].phases & 0x07) {
						case 0x01:	//C
							{
								temp_index=0;
								temp_index_b=2;
								break;
							}
						case 0x02:	//B
							{
								temp_index=0;
								temp_index_b=1;
								break;
							}
						case 0x03:	//BC
							{
								if (jindex==0)	//B
								{
									temp_index=0;
									temp_index_b=1;
								}
								else			//C
								{
									temp_index=1;
									temp_index_b=2;
								}
								break;
							}
						case 0x04:	//A
							{
								temp_index=0;
								temp_index_b=0;
								break;
							}
						case 0x05:	//AC
							{
								if (jindex==0)	//A
								{
									temp_index=0;
									temp_index_b=0;
								}
								else			//C
								{
									temp_index=1;
									temp_index_b=2;
								}
								break;
							}
						case 0x06:	//AB
						case 0x07:	//ABC
							{
								temp_index=jindex;
								temp_index_b=jindex;
								break;
							}
						default:
							break;
					}//end case

					if (NR_solver_algorithm == NRM_TCIM)
					{
						if (!jacobian_pass)	//Current injection update
						{
							if ((temp_index==-1) || (temp_index_b==-1))
							{
								GL_THROW("NR: A scheduled power update element failed.");
								//Defined below
							}

							//Real power calculations
							tempPbus = (undeltacurr[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im();	// Real power portion of Constant current component multiply the magnitude of bus voltage
							bus[indexer].PL[temp_index] += tempPbus;	//Real power portion - all is current based -- accumulate in case mixed and matched with old above

							//Reactive load calculations
							tempQbus = (undeltacurr[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Re();	// Reactive power portion of Constant current component multiply the magnitude of bus voltage
							bus[indexer].QL[temp_index] += tempQbus;	//Reactive power portion - all is current based -- accumulate in case mixed and matched with old above
						}
						else	//Jacobian update
						{
							if ((temp_index==-1) || (temp_index_b==-1))
							{
								GL_THROW("NR: A Jacobian update element failed.");
								//Defined below
							}

							if ((bus[indexer].V[temp_index_b]).Mag()!=0)
							{
								//Apply as an accumulation, in case any "normal" connections are present too
								bus[indexer].Jacob_A[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3); // + (undeltaimped[temp_index_b]).Im();// second part of equation(37) - no power term needed
								bus[indexer].Jacob_B[temp_index] += -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() + (undeltacurr[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3); // - (undeltaimped[temp_index_b]).Re();// second part of equation(38) - no power term needed
								bus[indexer].Jacob_C[temp_index] +=((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Re() *gld::pow((bus[indexer].V[temp_index_b]).Im(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3); // - (undeltaimped[temp_index_b]).Re();// second part of equation(39) - no power term needed
								bus[indexer].Jacob_D[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() - (undeltacurr[temp_index_b]).Im() *gld::pow((bus[indexer].V[temp_index_b]).Re(),2))/gld::pow((bus[indexer].V[temp_index_b]).Mag(),3); // - (undeltaimped[temp_index_b]).Im();// second part of equation(40) - no power term needed
							}
							else	//Zero voltage = only impedance is valid (others get divided by VMag, so are IND) - not entirely sure how this gets in here anyhow
							{
								bus[indexer].Jacob_A[temp_index] += -1e-4; //(undeltaimped[temp_index_b]).Im() - 1e-4;	//Small offset to avoid singularities (if impedance is zero too)
								bus[indexer].Jacob_B[temp_index] += -1e-4; //-(undeltaimped[temp_index_b]).Re() - 1e-4;
								bus[indexer].Jacob_C[temp_index] += -1e-4; //-(undeltaimped[temp_index_b]).Re() - 1e-4;
								bus[indexer].Jacob_D[temp_index] += -1e-4; //-(undeltaimped[temp_index_b]).Im() - 1e-4;
							}
						}//End pass differentiation
					}//End TCIM
					else	//Implied FPI
					{
						if ((temp_index==-1) || (temp_index_b==-1))
						{
							GL_THROW("NR: A Jacobian update element failed.");
							//Defined below
						}

						//Apply the current accumulation
						bus[indexer].FPI_current[temp_index_b] += undeltacurr[temp_index_b];
					}
				}//End phase traversion
			}//End delta/wye explicit loads

			//TCIM full_Y_load - explicitly handled in FPI
			if (jacobian_pass && (NR_solver_algorithm == NRM_TCIM))	//This part only gets done on the Jacobian update and TCIM
			{
				//Delta load components  get added to the Jacobian values too -- mostly because this is the most convenient place to do it
				//See if we're even needed first
				if (bus[indexer].full_Y_load != nullptr)
				{
					//Provide updates to relevant phases
					//only compute and store phases that exist (make top heavy)
					temp_index = -1;
					temp_index_b = -1;

					for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)
					{
						switch(bus[indexer].phases & 0x07) {
							case 0x01:	//C
								{
									temp_index=0;
									temp_index_b=8;
									break;
								}
							case 0x02:	//B
								{
									temp_index=0;
									temp_index_b=4;
									break;
								}
							case 0x03:	//BC
								{
									if (jindex==0)	//B
									{
										temp_index=0;
										temp_index_b=4;
									}
									else			//C
									{
										temp_index=1;
										temp_index_b=8;
									}
									break;
								}
							case 0x04:	//A
								{
									temp_index=0;
									temp_index_b=0;
									break;
								}
							case 0x05:	//AC
								{
									if (jindex==0)	//A
									{
										temp_index=0;
										temp_index_b=0;
									}
									else			//C
									{
										temp_index=1;
										temp_index_b=8;
									}
									break;
								}
							case 0x06:	//AB
							case 0x07:	//ABC
								{
									temp_index=jindex;
									temp_index_b=(jindex*3+jindex);
									break;
								}
							default:
								break;
						}//end case

						if ((temp_index==-1) || (temp_index_b==-1))
						{
							GL_THROW("NR: A Jacobian update element failed.");
							//Defined below
						}

						//Accumulate the values
						bus[indexer].Jacob_A[temp_index] += bus[indexer].full_Y_load[temp_index_b].Im();
						bus[indexer].Jacob_B[temp_index] += bus[indexer].full_Y_load[temp_index_b].Re();
						bus[indexer].Jacob_C[temp_index] += bus[indexer].full_Y_load[temp_index_b].Re();
						bus[indexer].Jacob_D[temp_index] -= bus[indexer].full_Y_load[temp_index_b].Im();
					}//End phase traversion
				}//End deltamode-enabled in-rush loads updates
			}//End Jacobian pass for deltamode loads
		}//End relevant island check
	}//end bus traversion for Jacobian or current injection items
}//End load update function

//Function to free up array of NR_SOLVER_STRUCT variables
STATUS NR_array_structure_free(NR_SOLVER_STRUCT *struct_of_interest,int number_of_islands)
{
	int index_val;
	SUPERLU_NR_vars *curr_island_superLU_vars;

	//Null the pointer, because
	curr_island_superLU_vars = nullptr;

	//Loop through the individual entries, freeing them all
	for (index_val=0; index_val<number_of_islands; index_val++)
	{
		//Free the arrays - check htem all - otherwise this could have issues
		if (struct_of_interest->island_matrix_values[index_val].current_RHS_NR != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].current_RHS_NR);

		if (struct_of_interest->island_matrix_values[index_val].Y_offdiag_PQ != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].Y_offdiag_PQ);
		
		if (struct_of_interest->island_matrix_values[index_val].Y_diag_fixed != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].Y_diag_fixed);
		
		if (struct_of_interest->island_matrix_values[index_val].Y_diag_update != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].Y_diag_update);
		
		if (struct_of_interest->island_matrix_values[index_val].Y_Amatrix != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].Y_Amatrix);

		//Do the sub-matrix elements too
		if (struct_of_interest->island_matrix_values[index_val].matrices_LU.a_LU != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].matrices_LU.a_LU);

		if (struct_of_interest->island_matrix_values[index_val].matrices_LU.cols_LU != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].matrices_LU.cols_LU);

		if (struct_of_interest->island_matrix_values[index_val].matrices_LU.rhs_LU != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].matrices_LU.rhs_LU);

		if (struct_of_interest->island_matrix_values[index_val].matrices_LU.rows_LU != nullptr)
			gl_free(struct_of_interest->island_matrix_values[index_val].matrices_LU.rows_LU);

		if (struct_of_interest->island_matrix_values[index_val].LU_solver_vars != nullptr)
		{
			//If we're superLU - be sure to get rid of the submatrix sub-items
			if (matrix_solver_method == MM_SUPERLU)
			{
				//Map it
				curr_island_superLU_vars = (SUPERLU_NR_vars*)struct_of_interest->island_matrix_values[index_val].LU_solver_vars;

				//Free the others, if necessary
				if (curr_island_superLU_vars->A_LU.Store != nullptr)
					gl_free(curr_island_superLU_vars->A_LU.Store);
				
				if (curr_island_superLU_vars->B_LU.Store != nullptr)
					gl_free(curr_island_superLU_vars->B_LU.Store);
				
				if (curr_island_superLU_vars->perm_c != nullptr)
					gl_free(curr_island_superLU_vars->perm_c);
				
				if (curr_island_superLU_vars->perm_r != nullptr)
					gl_free(curr_island_superLU_vars->perm_r);

				//Null the pointer again, just because
				curr_island_superLU_vars = nullptr;

				//Free the value
				gl_free(struct_of_interest->island_matrix_values[index_val].LU_solver_vars);
			}
			else if (matrix_solver_method == MM_EXTERN)
			{
				//Call destruction routine
				((void (*)(void *, bool))(LUSolverFcns.ext_destroy))(struct_of_interest->island_matrix_values[index_val].LU_solver_vars,false);
			}
			//Default else -- ??? Not sure how we get here
		}
	}

	//Free the overall array
	gl_free(struct_of_interest->island_matrix_values);

	//Null it out, so it gets detected
	struct_of_interest->island_matrix_values = nullptr;

	//Free up the "common/base" item, as well
	gl_free(struct_of_interest->BA_diag);

	//Null the final indicator, just to be safe
	struct_of_interest->BA_diag = nullptr;

	//If it made it this far, succeed
	return SUCCESS;
}

//Function to perform the base allocation of NR_SOLVER_STRUCT variables (solver_NR will handle the lower-level details)
STATUS NR_array_structure_allocate(NR_SOLVER_STRUCT *struct_of_interest,int number_of_islands)
{
	int index_val;

	//Make sure the existing item is empty -- otherwise, error, because we have no idea how big it was!
	if (struct_of_interest->island_matrix_values != nullptr)
	{
		gl_error("solver_NR: Attempted to allocate over an existing NR solver variable array!");
		/*  TROUBLESHOOT
		While attempting to allocate over a NR solver/island variable, the variable to allocate
		to was not empty.  The base size is unknown, so this will fail (it may leave stranded memory locations).
		Please try again, and adjust any code added.  If the error persists, post an issue under the ticketing system.
		*/

		return FAILED;
	}

	//Allocate the base array
	struct_of_interest->island_matrix_values = (NR_MATRIX_CONSTRUCTION *)gl_malloc(number_of_islands*sizeof(NR_MATRIX_CONSTRUCTION));

	//Make sure it worked
	if (struct_of_interest->island_matrix_values == nullptr)
	{
		gl_error("solver_NR: An attempt to allocate a variable array for the NR solver failed!");
		/*  TROUBLESHOOT
		While attempting to allocate an array for the NR solver, a memory allocation issue was encountered.
		Please try again.  If the error persists, please submit an issue via the ticketing system.
		*/

		return FAILED;
	}

	//Now loop through and initialize the variables
	for (index_val=0; index_val<number_of_islands; index_val++)
	{
		//Some of these are done in solver_NR as well, but let's be paranoid
		struct_of_interest->island_matrix_values[index_val].current_RHS_NR = nullptr;
		struct_of_interest->island_matrix_values[index_val].size_offdiag_PQ = 0;
		struct_of_interest->island_matrix_values[index_val].size_diag_fixed = 0;
		struct_of_interest->island_matrix_values[index_val].total_variables = 0;
		struct_of_interest->island_matrix_values[index_val].size_diag_update = 0;
		struct_of_interest->island_matrix_values[index_val].size_Amatrix = 0;
		struct_of_interest->island_matrix_values[index_val].max_size_offdiag_PQ = 0;
		struct_of_interest->island_matrix_values[index_val].max_size_diag_fixed = 0;
		struct_of_interest->island_matrix_values[index_val].max_total_variables = 0;
		struct_of_interest->island_matrix_values[index_val].max_size_diag_update = 0;
		struct_of_interest->island_matrix_values[index_val].prev_m = 0;
		struct_of_interest->island_matrix_values[index_val].index_count = 0;
		struct_of_interest->island_matrix_values[index_val].bus_count = 0;
		struct_of_interest->island_matrix_values[index_val].indexer = 0;
		struct_of_interest->island_matrix_values[index_val].NR_realloc_needed = false;
		struct_of_interest->island_matrix_values[index_val].Y_offdiag_PQ = nullptr;
		struct_of_interest->island_matrix_values[index_val].Y_diag_fixed = nullptr;
		struct_of_interest->island_matrix_values[index_val].Y_diag_update = nullptr;
		struct_of_interest->island_matrix_values[index_val].Y_Amatrix = nullptr;
		
		//Sub-structure of matrices
		struct_of_interest->island_matrix_values[index_val].matrices_LU.a_LU = nullptr;
		struct_of_interest->island_matrix_values[index_val].matrices_LU.cols_LU = nullptr;
		struct_of_interest->island_matrix_values[index_val].matrices_LU.rhs_LU = nullptr;
		struct_of_interest->island_matrix_values[index_val].matrices_LU.rows_LU = nullptr;

		//Other values
		struct_of_interest->island_matrix_values[index_val].LU_solver_vars = nullptr;
		struct_of_interest->island_matrix_values[index_val].iteration_count = 0;
		struct_of_interest->island_matrix_values[index_val].new_iteration_required = false;
		struct_of_interest->island_matrix_values[index_val].swing_converged = false;
		struct_of_interest->island_matrix_values[index_val].swing_is_a_swing = false;
		struct_of_interest->island_matrix_values[index_val].SaturationMismatchPresent = false;
		struct_of_interest->island_matrix_values[index_val].NortonCurrentMismatchPresent = false;
		struct_of_interest->island_matrix_values[index_val].solver_info = -1;	//"Bad", by default
		struct_of_interest->island_matrix_values[index_val].return_code = -1;	//Still "bad" too
		struct_of_interest->island_matrix_values[index_val].max_mismatch_converge = 0.0;
	}

	//Null out the main item too
	struct_of_interest->BA_diag = nullptr;

	//If it made it this far, declare success
	return SUCCESS;
}
