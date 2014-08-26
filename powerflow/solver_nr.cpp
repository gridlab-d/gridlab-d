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

#include "solver_nr.h"

#define MT // this enables multithreaded SuperLU

//#define NR_MATRIX_OUT	//This directive enables a text file dump of the sparse-formatted admittance matrix - useful for debugging
//#define NR_VOLTAGE_PORTION_OUT	//This directive also enables the voltages to be appended to the admittance matrix dump - debugging and requires NR_MATRIX_OUT to be set
//#define NR_REFERENCES_OUT		//This directive also enables a row-range association per node appended to the admittance matrix dump - debugging and requires NR_MATRIX_OUT to be set

#ifdef MT
#include <pdsp_defs.h>	//superLU_MT 
#else
#include <slu_ddefs.h>	//Sequential superLU (other platforms)
#endif

/* access to module global variables */
#include "powerflow.h"

//Generic solver variables
NR_SOLVER_VARS matrices_LU;

//SuperLU variables
int *perm_c, *perm_r;
SuperMatrix A_LU,B_LU;

//External solver global
void *ext_solver_glob_vars;

void merge_sort(Y_NR *Input_Array, unsigned int Alen, Y_NR *Work_Array){	//Merge sorting algorithm - basis stolen from auction.cpp in market module
	unsigned int split_point;
	unsigned int right_length;
	Y_NR *leftside, *rightside;
	Y_NR *Final_P;

	if (Alen>0)	//Only occurs if over zero
	{
		split_point = Alen/2;	//Find the middle point
		right_length = Alen - split_point;	//Figure out how big the right hand side is

		//Make the appropriate pointers
		leftside = Input_Array;
		rightside = Input_Array+split_point;

		//Check to see what condition we are in (keep splitting it)
		if (split_point>1)
			merge_sort(leftside,split_point,Work_Array);

		if (right_length>1)
			merge_sort(rightside,right_length,Work_Array);

		//Point at the first location
		Final_P = Work_Array;

		//Merge them now
		do {
			if (leftside->col_ind < rightside->col_ind)
				*Final_P++ = *leftside++;
			else if (leftside->col_ind == rightside->col_ind)
			{
				if (leftside->row_ind < rightside->row_ind)
					*Final_P++ = *leftside++;
				else if (leftside->row_ind > rightside->row_ind)
					*Final_P++ = *rightside++;
				else	//Catch for duplicate entries
				{
					GL_THROW("NR: duplicate entry found in admittance matrix - look for parallel lines!");
					/*  TROUBLESHOOT
					While sorting the admittance matrix for the Newton-Raphson solver, a duplicate entry was
					found.  This is usually caused by a line between two nodes having another, parallel line between
					the same two nodes.  This is only an issue if the parallel lines overlap in phase (e.g., AB and BC).
					If no overlapping phase is present, this error should not occur.  Methods to narrow down the location
					of this conflict are under development.
					*/
				}
			}
			else
				*Final_P++ = *rightside++;
		} while ((leftside<(Input_Array+split_point)) && (rightside<(Input_Array+Alen)));	//Sort the list until one of them empties

		while (leftside<(Input_Array+split_point))	//Put any remaining entries into the list
			*Final_P++ = *leftside++;

		while (rightside<(Input_Array+Alen))		//Put any remaining entries into the list
			*Final_P++ = *rightside++;

		memcpy(Input_Array,Work_Array,sizeof(Y_NR)*Alen);	//Copy the result back into the input
	}	//End length > 0
}

/** Newton-Raphson solver
	Solves a power flow problem using the Newton-Raphson method
	
	@return n=0 on failure to complete a single iteration, 
	n>0 to indicate success after n interations, or 
	n<0 to indicate failure after n iterations
 **/
int64 solver_nr(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch, NR_SOLVER_STRUCT *powerflow_values, NRSOLVERMODE powerflow_type ,bool *bad_computations)
{
	//Internal iteration counter - just NR limits
	int64 Iteration;

	//A matrix size variable
	unsigned int size_Amatrix;

	//Voltage mismatch tracking variable
	double Maxmismatch;

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

	//Working matrix for admittance collapsing/determinations
	complex tempY[3][3];

	//Miscellaneous flag variables
	bool Full_Mat_A, Full_Mat_B, proceed_flag;

	//Iteration flag
	bool newiter;

	//Deltamode pass flag - changes how SWING buses are handled
	bool swing_is_a_swing;

	//Deltamode initial dynamics run - swing convergence flag (symmetry)
	bool swing_converged;

	//Deltamode intermediate variables
	complex temp_complex_0, temp_complex_1, temp_complex_2, temp_complex_3, temp_complex_4, temp_complex_5;
	complex aval, avalsq;

	//Temporary size variable
	char temp_size, temp_size_b, temp_size_c;

	//Temporary admittance variables
	complex Temp_Ad_A[3][3];
	complex Temp_Ad_B[3][3];

	//Temporary load calculation variables
	complex undeltacurr[3];
	complex delta_current[3], voltageDel[3];
	complex temp_current[3], temp_power[3], temp_store[3];
	complex adjusted_constant_current[6];
	complex adjust_temp_voltage[3], adjust_temp_nominal_voltage[3];
	double adjust_temp_voltage_mag[3];
	double adjust_nominal_voltage_val;

	//DV checking array
	complex DVConvCheck[3];
	double CurrConvVal;

	//Miscellaneous counter tracker
	unsigned int index_count = 0;

	//Miscellaneous working variable
	double work_vals_double_0, work_vals_double_1,work_vals_double_2,work_vals_double_3,work_vals_double_4;
	char work_vals_char_0;

	//SuperLU variables
	SuperMatrix L_LU,U_LU;
	NCformat *Astore;
	DNformat *Bstore;
	int nnz, info;
	unsigned int m,n;
	double *sol_LU;
	
#ifndef MT
	superlu_options_t options;	//Additional variables for sequential superLU
	SuperLUStat_t stat;
#endif

	//Ensure bad computations flag is set first
	*bad_computations = false;

	//Determine special circumstances of SWING bus -- do we want it to truly participate right
	if (powerflow_type != PF_NORMAL)	//Not normal
	{
		//Swing is always assumed to be bus zero
		if (bus[0].full_Y == NULL)
		{
			//Set variables
			swing_is_a_swing = true;
		}
		else	//We have a generator attached, proceed like "normal"
		{
			//Set the dynamics variable appropriately
			if (powerflow_type != PF_DYNCALC)
				swing_is_a_swing = true;		//First pass of "initial dynamics" treat this as swing
			else
				swing_is_a_swing = false;		//Dynamics solutions (and after first pass of "initial dynamics") swing bus becomes PQ
		}
	}
	else
		swing_is_a_swing = true;	//Normal powerflow, we're always a true SWING

	//Populate aval, if necessary
	if (powerflow_type == PF_DYNINIT)
	{
		//Conversion variables - 1@120-deg
		aval = complex(-0.5,(sqrt(3.0)/2.0));
		avalsq = aval*aval;	//squared value is used a couple places too
	}
	else	//Zero it, just in case something uses it (what would???)
	{
		aval = 0.0;
		avalsq = 0.0;
	}

	if (matrix_solver_method==MM_EXTERN)
	{
		//Call the initialization routine
		ext_solver_glob_vars = ((void *(*)(void *))(LUSolverFcns.ext_init))(ext_solver_glob_vars);

		//Make sure it worked (allocation check)
		if (ext_solver_glob_vars==NULL)
		{
			GL_THROW("External LU matrix solver failed to allocate memory properly!");
			/*  TROUBLESHOOT
			While attempting to allocate memory for the external LU solver, an error occurred.
			Please try again.  If the error persists, ensure your external LU solver is behaving correctly
			and coordinate with their development team as necessary.
			*/
		}
	}

	if (NR_admit_change)	//If an admittance update was detected, fix it
	{
		//Build the diagnoal elements of the bus admittance matrix - this should only happen once no matter what
		if (powerflow_values->BA_diag == NULL)
		{
			powerflow_values->BA_diag = (Bus_admit *)gl_malloc(bus_count *sizeof(Bus_admit));   //BA_diag store the location and value of diagonal elements of Bus Admittance matrix

			//Make sure it worked
			if (powerflow_values->BA_diag == NULL)
			{
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");
				/*  TROUBLESHOOT
				During the allocation stage of the NR algorithm, one of the matrices failed to be allocated.
				Please try again and if this bug persists, submit your code and a bug report using the trac
				website.
				*/
			}
		}
		
		for (indexer=0; indexer<bus_count; indexer++) // Construct the diagonal elements of Bus admittance matrix.
		{
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
			if ((powerflow_type!=PF_NORMAL) && (bus[indexer].full_Y != NULL))
			{
				//Loop and add
				for (jindex=0; jindex<3; jindex++)
				{
					for (kindex=0; kindex<3; kindex++)
					{
						tempY[jindex][kindex] = bus[indexer].full_Y[jindex*3+kindex];	//Adds in any self-admittance terms (generators)
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
							tempY[0][0] += branch[jindexer].YSto[0];
							tempY[0][1] += branch[jindexer].YSto[1];
							tempY[1][0] += branch[jindexer].YSto[3];
							tempY[1][1] += branch[jindexer].YSto[4];
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
			powerflow_values->BA_diag[indexer].col_ind = powerflow_values->BA_diag[indexer].row_ind = index_count;	// Store the row and column starting information (square matrices)
			bus[indexer].Matrix_Loc = index_count;								//Store our location so we know where we go
			index_count += powerflow_values->BA_diag[indexer].size;				// Update the index for this matrix's size, so next one is in appropriate place


			//Store the admittance values into the BA_diag matrix structure
			for (jindex=0; jindex<powerflow_values->BA_diag[indexer].size; jindex++)
			{
				for (kindex=0; kindex<powerflow_values->BA_diag[indexer].size; kindex++)			//Store values - assume square matrix - don't bother parsing what doesn't exist.
				{
					powerflow_values->BA_diag[indexer].Y[jindex][kindex] = tempY[jindex][kindex];// Store the self admittance terms.
				}
			}

			//Copy values into node-specific link (if needed)
			if (bus[indexer].full_Y_all != NULL)
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

		//Store the size of the diagonal, since it represents how many variables we are solving (useful later)
		powerflow_values->total_variables=index_count;

		//Check to see if we've exceeded our max.  If so, reallocate!
		if (powerflow_values->total_variables > powerflow_values->max_total_variables)
			powerflow_values->NR_realloc_needed = true;

		/// Build the off_diagonal_PQ bus elements of 6n*6n Y_NR matrix.Equation (12). All the value in this part will not be updated at each iteration.
		//Constructed using sparse methodology, non-zero elements are the only thing considered (and non-PV)
		//No longer necessarily 6n*6n any more either,
		powerflow_values->size_offdiag_PQ = 0;
		for (jindexer=0; jindexer<branch_count;jindexer++)	//Parse all of the branches
		{
			tempa  = branch[jindexer].from;
			tempb  = branch[jindexer].to;

			//Preliminary check to make sure we weren't missed in the initialization
			if ((bus[tempa].Matrix_Loc == -1) || (bus[tempb].Matrix_Loc == -1))
			{
				GL_THROW("An element in NR line:%d was not properly localized");
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
							powerflow_values->size_offdiag_PQ += 1; 

						if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))  
							powerflow_values->size_offdiag_PQ += 1; 

						if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
							powerflow_values->size_offdiag_PQ += 1; 

						if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
							powerflow_values->size_offdiag_PQ += 1; 
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
										powerflow_values->size_offdiag_PQ += 1; 

									if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))  
										powerflow_values->size_offdiag_PQ += 1; 

									if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
										powerflow_values->size_offdiag_PQ += 1; 

									if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
										powerflow_values->size_offdiag_PQ += 1; 
								}//end column validity check
							}//end columns of 3 phase
						}//End row validity check
					}//end rows of 3 phase
				}//end not SPCT
				else	//SPCT inmplementation
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
									powerflow_values->size_offdiag_PQ += 1; 

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
									powerflow_values->size_offdiag_PQ += 1; 
							}//end columns traverse

							//If row is valid, now traverse the rows of that column for Yto
							for (kindex=0; kindex<3; kindex++)
							{
								if (((branch[jindexer].Yto[kindex*3+jindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))  
									powerflow_values->size_offdiag_PQ += 1; 

								if (((branch[jindexer].Yto[kindex*3+jindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
									powerflow_values->size_offdiag_PQ += 1; 
							}//end rows traverse
						}//End row validity check
					}//end rows of 3 phase
				}//End SPCT
			}//end three phase
		}//end line traversion

		//Allocate the space - double the number found (each element goes in two places)
		if (powerflow_values->Y_offdiag_PQ == NULL)
		{
			powerflow_values->Y_offdiag_PQ = (Y_NR *)gl_malloc((powerflow_values->size_offdiag_PQ*2) *sizeof(Y_NR));   //powerflow_values->Y_offdiag_PQ store the row,column and value of off_diagonal elements of Bus Admittance matrix in which all the buses are not PV buses. 

			//Make sure it worked
			if (powerflow_values->Y_offdiag_PQ == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Save our size
			powerflow_values->max_size_offdiag_PQ = powerflow_values->size_offdiag_PQ;	//Don't care about the 2x, since we'll be comparing it against itself
		}
		else if (powerflow_values->size_offdiag_PQ > powerflow_values->max_size_offdiag_PQ)	//Something changed and we are bigger!!
		{
			//Destroy us!
			gl_free(powerflow_values->Y_offdiag_PQ);

			//Rebuild us, we have the technology
			powerflow_values->Y_offdiag_PQ = (Y_NR *)gl_malloc((powerflow_values->size_offdiag_PQ*2) *sizeof(Y_NR));

			//Make sure it worked
			if (powerflow_values->Y_offdiag_PQ == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Store the new size
			powerflow_values->max_size_offdiag_PQ = powerflow_values->size_offdiag_PQ;

			//Flag for a reallocation
			powerflow_values->NR_realloc_needed = true;
		}

		indexer = 0;
		for (jindexer=0; jindexer<branch_count;jindexer++)	//Parse through all of the branches
		{
			//Extract both ends
			tempa  = branch[jindexer].from;
			tempb  = branch[jindexer].to;

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
								//Indices counted out from Self admittance above.  needs doubling due to complex separation
								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
									indexer += 1;
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
									indexer += 1;
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;	
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 3;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;	
								}
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
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(branch[jindexer].Yfrom[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;	
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;	
								}
							}//end From end SPCT to
							else if ((bus[tempb].phases & 0x20) == 0x20)	//To end is a SPCT to
							{
								//Indices counted out from Self admittance above.  needs doubling due to complex separation

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Im());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(branch[jindexer].Yto[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;	
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;	
								}
							}//end To end SPCT to
							else											//Plain old ugly line
							{
								//Indices counted out from Self admittance above.  needs doubling due to complex separation

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
									indexer += 1;
								}

								if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
									indexer += 1;	
								}

								if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
								{
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;
									
									powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
									powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
									powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
									indexer += 1;	
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
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
							indexer += 1;
							
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
							indexer += 1;
						}

						if (((branch[jindexer].Yto[kindex*3+jindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
						{
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Im());
							indexer += 1;
							
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[kindex*3+jindex]).Im();
							indexer += 1;
						}

						if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
						{
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
							indexer += 1;
							
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
							indexer += 1;	
						}

						if (((branch[jindexer].Yto[kindex*3+jindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
						{
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Re());
							indexer += 1;
							
							powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex;
							powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
							powerflow_values->Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Re());
							indexer += 1;	
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
					GL_THROW("NR: A line's phase was flagged as not full three-phase, but wasn't");
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
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2 + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
								indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2 + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
								indexer += 1;
							}

							if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2 + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								indexer += 1;	
							}

							if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2 + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								indexer += 1;	
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
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2 + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
								indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2 + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
								indexer += 1;
							}

							if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2 + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								indexer += 1;	
							}

							if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2 + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								indexer += 1;	
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
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
								indexer += 1;
							}

							if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
								indexer += 1;
							}

							if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
								indexer += 1;	
							}

							if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
							{
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								indexer += 1;
								
								powerflow_values->Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
								powerflow_values->Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
								powerflow_values->Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
								indexer += 1;	
							}
						}//column end
					}//row end
				}//end not full ABC with AC on either side case
			}//end all others else
		}//end branch for

		//Build the fixed part of the diagonal PQ bus elements of 6n*6n Y_NR matrix. This part will not be updated at each iteration. 
		powerflow_values->size_diag_fixed = 0;
		for (jindexer=0; jindexer<bus_count;jindexer++) 
		{
			for (jindex=0; jindex<3; jindex++)
			{
				for (kindex=0; kindex<3; kindex++)
				{		 
				  if ((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() != 0 && bus[jindexer].type != 1 && jindex!=kindex)  
					  powerflow_values->size_diag_fixed += 1; 
				  if ((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() != 0 && bus[jindexer].type != 1 && jindex!=kindex) 
					  powerflow_values->size_diag_fixed += 1; 
				  else {}
				 }
			}
		}
		if (powerflow_values->Y_diag_fixed == NULL)
		{
			powerflow_values->Y_diag_fixed = (Y_NR *)gl_malloc((powerflow_values->size_diag_fixed*2) *sizeof(Y_NR));   //powerflow_values->Y_diag_fixed store the row,column and value of the fixed part of the diagonal PQ bus elements of 6n*6n Y_NR matrix.

			//Make sure it worked
			if (powerflow_values->Y_diag_fixed == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Update the max size
			powerflow_values->max_size_diag_fixed = powerflow_values->size_diag_fixed;
		}
		else if (powerflow_values->size_diag_fixed > powerflow_values->max_size_diag_fixed)		//Something changed and we are bigger!!
		{
			//Destroy us!
			gl_free(powerflow_values->Y_diag_fixed);

			//Rebuild us, we have the technology
			powerflow_values->Y_diag_fixed = (Y_NR *)gl_malloc((powerflow_values->size_diag_fixed*2) *sizeof(Y_NR));

			//Make sure it worked
			if (powerflow_values->Y_diag_fixed == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Store the new size
			powerflow_values->max_size_diag_fixed = powerflow_values->size_diag_fixed;

			//Flag for a reallocation
			powerflow_values->NR_realloc_needed = true;
		}

		indexer = 0;
		for (jindexer=0; jindexer<bus_count;jindexer++)	//Parse through bus list
		{ 
			for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
			{
				for (kindex=0; kindex<powerflow_values->BA_diag[jindexer].size; kindex++)
				{					
					if ((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im() != 0 && bus[jindexer].type != 1 && jindex!=kindex)
					{
						powerflow_values->Y_diag_fixed[indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex;
						powerflow_values->Y_diag_fixed[indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex;
						powerflow_values->Y_diag_fixed[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im();
						indexer += 1;

						powerflow_values->Y_diag_fixed[indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex +powerflow_values->BA_diag[jindexer].size;
						powerflow_values->Y_diag_fixed[indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex +powerflow_values->BA_diag[jindexer].size;
						powerflow_values->Y_diag_fixed[indexer].Y_value = -(powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Im();
						indexer += 1;
					}

					if ((powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re() != 0 && bus[jindexer].type != 1 && jindex!=kindex)
					{
						powerflow_values->Y_diag_fixed[indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex;
						powerflow_values->Y_diag_fixed[indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex +powerflow_values->BA_diag[jindexer].size;
						powerflow_values->Y_diag_fixed[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re();
						indexer += 1;
						
						powerflow_values->Y_diag_fixed[indexer].row_ind = 2*powerflow_values->BA_diag[jindexer].row_ind + jindex +powerflow_values->BA_diag[jindexer].size;
						powerflow_values->Y_diag_fixed[indexer].col_ind = 2*powerflow_values->BA_diag[jindexer].col_ind + kindex;
						powerflow_values->Y_diag_fixed[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][kindex]).Re();
						indexer += 1;
					}
				}
			}
		}//End bus parse for fixed diagonal
	}//End admittance update

	//Calculate the system load - this is the specified power of the system
	for (Iteration=0; Iteration<NR_iteration_limit; Iteration++)
	{
		//System load at each bus is represented by second order polynomial equations
		for (indexer=0; indexer<bus_count; indexer++)
		{
			if ((bus[indexer].phases & 0x08) == 0x08)	//Delta connected node
			{
				//Populate the values for constant current -- deltamode different right now (all same in future?)
				if (*bus[indexer].dynamics_enabled == true)
				{
					//Create nominal magnitudes
					adjust_nominal_voltage_val = bus[indexer].kv_base * sqrt(3.0);

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
						adjusted_constant_current[0] = 0.0;
					}

					//Start adjustments - BC
					if ((bus[indexer].I[1] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[1] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].I[1] * adjust_temp_voltage_mag[1] / (voltageDel[1] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[1] = 0.0;
					}

					//Start adjustments - CA
					if ((bus[indexer].I[2] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[2] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].I[2] * adjust_temp_voltage_mag[2] / (voltageDel[2] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[2] = 0.0;
					}

					//See if we have any "different children"
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Create nominal magnitudes
						adjust_nominal_voltage_val = bus[indexer].kv_base;

						//Create the nominal voltage vectors
						adjust_temp_nominal_voltage[0].SetPolar(bus[indexer].kv_base,0.0);
						adjust_temp_nominal_voltage[1].SetPolar(bus[indexer].kv_base,-2.0*PI/3.0);
						adjust_temp_nominal_voltage[2].SetPolar(bus[indexer].kv_base,2.0*PI/3.0);

						//Get magnitudes of all
						adjust_temp_voltage_mag[0] = bus[indexer].V[0].Mag();
						adjust_temp_voltage_mag[1] = bus[indexer].V[1].Mag();
						adjust_temp_voltage_mag[2] = bus[indexer].V[2].Mag();

						//Start adjustments - A
						if ((bus[indexer].extra_var[6] != 0.0) && (adjust_temp_voltage_mag[0] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[3] = ~(adjust_temp_nominal_voltage[0] * ~bus[indexer].extra_var[6] * adjust_temp_voltage_mag[0] / (bus[indexer].V[0] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[3] = 0.0;
						}

						//Start adjustments - B
						if ((bus[indexer].extra_var[7] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[4] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].extra_var[7] * adjust_temp_voltage_mag[1] / (bus[indexer].V[1] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[4] = 0.0;
						}

						//Start adjustments - C
						if ((bus[indexer].extra_var[8] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[5] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].extra_var[8] * adjust_temp_voltage_mag[2] / (bus[indexer].V[2] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[5] = 0.0;
						}
					}
					else	//Nope
					{
						//Set to zero, just cause
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}
				else	//"Normal" modes -- handle traditionally
				{
					adjusted_constant_current[0] = bus[indexer].I[0];
					adjusted_constant_current[1] = bus[indexer].I[1];
					adjusted_constant_current[2] = bus[indexer].I[2];

					//See if we have different children too
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Store them too
						adjusted_constant_current[3] = bus[indexer].extra_var[6];
						adjusted_constant_current[4] = bus[indexer].extra_var[7];
						adjusted_constant_current[5] = bus[indexer].extra_var[8];
					}
					else	//Nope, just zero this for now
					{
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}//End adjustment code

				//Delta components - populate according to what is there
				if ((bus[indexer].phases & 0x06) == 0x06)	//Check for AB
				{
					//Voltage calculations
					voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];

					//Power - convert to a current (uses less iterations this way)
					delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].S[0]/voltageDel[0]);

					//Convert delta connected load to appropriate Wye
					delta_current[0] += voltageDel[0] * (bus[indexer].Y[0]);
				}
				else
				{
					//Zero values - they shouldn't be used anyhow
					voltageDel[0] = 0.0;
					delta_current[0] = 0.0;
				}

				if ((bus[indexer].phases & 0x03) == 0x03)	//Check for BC
				{
					//Voltage calculations
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

					//Power - convert to a current (uses less iterations this way)
					delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S[1]/voltageDel[1]);

					//Convert delta connected load to appropriate Wye
					delta_current[1] += voltageDel[1] * (bus[indexer].Y[1]);
				}
				else
				{
					//Zero unused
					voltageDel[1] = 0.0;
					delta_current[1] = 0.0;
				}

				if ((bus[indexer].phases & 0x05) == 0x05)	//Check for CA
				{
					//Voltage calculations
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Power - convert to a current (uses less iterations this way)
					delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S[2]/voltageDel[2]);

					//Convert delta connected load to appropriate Wye
					delta_current[2] += voltageDel[2] * (bus[indexer].Y[2]);
				}
				else
				{
					//Zero unused
					voltageDel[2] = 0.0;
					delta_current[2] = 0.0;
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
						undeltacurr[0] += bus[indexer].extra_var[3]*bus[indexer].V[0];

						//Current values
						undeltacurr[0] += adjusted_constant_current[3];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[0] = 0.0;
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
						undeltacurr[1] += bus[indexer].extra_var[4]*bus[indexer].V[1];

						//Current values
						undeltacurr[1] += adjusted_constant_current[4];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[1] = 0.0;
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
						undeltacurr[2] += bus[indexer].extra_var[5]*bus[indexer].V[2];

						//Current values
						undeltacurr[2] += adjusted_constant_current[5];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[2] = 0.0;
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
					
				}//End phase traversion
			}//end delta connected
			else if ((bus[indexer].phases & 0x80) == 0x80)	//Split-phase connected node
			{
				//Convert it all back to current (easiest to handle)
				//Get V12 first
				voltageDel[0] = bus[indexer].V[0] + bus[indexer].V[1];

				//Start with the currents (just put them in)
				temp_current[0] = bus[indexer].I[0];
				temp_current[1] = bus[indexer].I[1];
				temp_current[2] = *bus[indexer].extra_var;	//Current12 is not part of the standard current array

				//Now add in power contributions
				temp_current[0] += bus[indexer].V[0] == 0.0 ? 0.0 : ~(bus[indexer].S[0]/bus[indexer].V[0]);
				temp_current[1] += bus[indexer].V[1] == 0.0 ? 0.0 : ~(bus[indexer].S[1]/bus[indexer].V[1]);
				temp_current[2] += voltageDel[0] == 0.0 ? 0.0 : ~(bus[indexer].S[2]/voltageDel[0]);

				//Last, but not least, admittance/impedance contributions
				temp_current[0] += bus[indexer].Y[0]*bus[indexer].V[0];
				temp_current[1] += bus[indexer].Y[1]*bus[indexer].V[1];
				temp_current[2] += bus[indexer].Y[2]*voltageDel[0];

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

				//Convert 'em to line currents
				temp_store[0] = temp_current[0] + temp_current[2];
				temp_store[1] = -temp_current[1] - temp_current[2];

				//Update the stored values
				bus[indexer].PL[0] = temp_store[0].Re();
				bus[indexer].QL[0] = temp_store[0].Im();

				bus[indexer].PL[1] = temp_store[1].Re();
				bus[indexer].QL[1] = temp_store[1].Im();
			}//end split-phase connected
			else	//Wye-connected node
			{
				//Populate the values for constant current -- deltamode different right now (all same in future?)
				if (*bus[indexer].dynamics_enabled == true)
				{
					//Create nominal magnitudes
					adjust_nominal_voltage_val = bus[indexer].kv_base;

					//Create the nominal voltage vectors
					adjust_temp_nominal_voltage[0].SetPolar(bus[indexer].kv_base,0.0);
					adjust_temp_nominal_voltage[1].SetPolar(bus[indexer].kv_base,-2.0*PI/3.0);
					adjust_temp_nominal_voltage[2].SetPolar(bus[indexer].kv_base,2.0*PI/3.0);

					//Get magnitudes of all
					adjust_temp_voltage_mag[0] = bus[indexer].V[0].Mag();
					adjust_temp_voltage_mag[1] = bus[indexer].V[1].Mag();
					adjust_temp_voltage_mag[2] = bus[indexer].V[2].Mag();

					//Start adjustments - A
					if ((bus[indexer].I[0] != 0.0) && (adjust_temp_voltage_mag[0] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[0] = ~(adjust_temp_nominal_voltage[0] * ~bus[indexer].I[0] * adjust_temp_voltage_mag[0] / (bus[indexer].V[0] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[0] = 0.0;
					}

					//Start adjustments - B
					if ((bus[indexer].I[1] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[1] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].I[1] * adjust_temp_voltage_mag[1] / (bus[indexer].V[1] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[1] = 0.0;
					}

					//Start adjustments - C
					if ((bus[indexer].I[2] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[2] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].I[2] * adjust_temp_voltage_mag[2] / (bus[indexer].V[2] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[2] = 0.0;
					}

					//See if we have any "different children"
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Create nominal magnitudes
						adjust_nominal_voltage_val = bus[indexer].kv_base * sqrt(3.0);

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
							adjusted_constant_current[3] = 0.0;
						}

						//Start adjustments - BC
						if ((bus[indexer].extra_var[7] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[4] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].extra_var[7] * adjust_temp_voltage_mag[1] / (voltageDel[1] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[4] = 0.0;
						}

						//Start adjustments - CA
						if ((bus[indexer].extra_var[8] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[5] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].extra_var[8] * adjust_temp_voltage_mag[2] / (voltageDel[2] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[5] = 0.0;
						}
					}
					else	//Nope
					{
						//Set to zero, just cause
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}
				else	//"Normal" modes -- handle traditionally
				{
					adjusted_constant_current[0] = bus[indexer].I[0];
					adjusted_constant_current[1] = bus[indexer].I[1];
					adjusted_constant_current[2] = bus[indexer].I[2];

					//See if we have different children too
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Store them too
						adjusted_constant_current[3] = bus[indexer].extra_var[6];
						adjusted_constant_current[4] = bus[indexer].extra_var[7];
						adjusted_constant_current[5] = bus[indexer].extra_var[8];
					}
					else	//Nope, just zero this for now
					{
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}//End adjustment code

				//For Wye-connected, only compute and store phases that exist (make top heavy)
				temp_index = -1;
				temp_index_b = -1;

				if ((bus[indexer].phases & 0x10) == 0x10)	//"Different" child load - in this case it must be delta - also must be three phase (just because that's how I forced it to be implemented)
				{											//Calculate all the deltas to wyes in advance (otherwise they'll get repeated)
					//Delta voltages
					voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Make sure phase combinations exist
					if ((bus[indexer].phases & 0x06) == 0x06)	//Has A-B
					{
						//Power - put into a current value (iterates less this way)
						delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].extra_var[0]/voltageDel[0]);

						//Convert delta connected load to appropriate Wye 
						delta_current[0] += voltageDel[0] * (bus[indexer].extra_var[3]);
					}
					else
					{
						//Zero it, for good measure
						delta_current[0] = 0.0;
					}

					//Check for BC
					if ((bus[indexer].phases & 0x03) == 0x03)	//Has B-C
					{
						//Power - put into a current value (iterates less this way)
						delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].extra_var[1]/voltageDel[1]);

						//Convert delta connected load to appropriate Wye 
						delta_current[1] += voltageDel[1] * (bus[indexer].extra_var[4]);
					}
					else
					{
						//Zero it, for good measure
						delta_current[1] = 0.0;
					}

					//Check for CA
					if ((bus[indexer].phases & 0x05) == 0x05)	//Has C-A
					{
						//Power - put into a current value (iterates less this way)
						delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].extra_var[2]/voltageDel[2]);

						//Convert delta connected load to appropriate Wye 
						delta_current[2] += voltageDel[2] * (bus[indexer].extra_var[5]);
					}
					else
					{
						//Zero it, for good measure
						delta_current[2] = 0.0;
					}

					//Convert delta-current into a phase current - reuse temp variable
					undeltacurr[0]=(adjusted_constant_current[3]+delta_current[0])-(adjusted_constant_current[5]+delta_current[2]);
					undeltacurr[1]=(adjusted_constant_current[4]+delta_current[1])-(adjusted_constant_current[3]+delta_current[0]);
					undeltacurr[2]=(adjusted_constant_current[5]+delta_current[2])-(adjusted_constant_current[4]+delta_current[1]);
				}
				else	//zero the variable so we don't have excessive ifs
				{
					undeltacurr[0] = undeltacurr[1] = undeltacurr[2] = 0.0;	//Zero it
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

				}//end phase traversion
			}//end wye-connected

			//Perform explicit delta/wye load accumulation - do if not triplex
			if ((bus[indexer].phases & 0x80) != 0x80)
			{
				//Delta components - populate according to what is there
				if ((bus[indexer].phases & 0x06) == 0x06)	//Check for AB
				{
					//Voltage calculations
					voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];

					//Power - convert to a current (uses less iterations this way)
					delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].S_dy[0]/voltageDel[0]);

					//Convert delta connected load to appropriate Wye
					delta_current[0] += voltageDel[0] * (bus[indexer].Y_dy[0]);

				}
				else
				{
					//Zero values - they shouldn't be used anyhow
					voltageDel[0] = 0.0;
					delta_current[0] = 0.0;
				}

				if ((bus[indexer].phases & 0x03) == 0x03)	//Check for BC
				{
					//Voltage calculations
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

					//Power - convert to a current (uses less iterations this way)
					delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S_dy[1]/voltageDel[1]);

					//Convert delta connected load to appropriate Wye
					delta_current[1] += voltageDel[1] * (bus[indexer].Y_dy[1]);

				}
				else
				{
					//Zero unused
					voltageDel[1] = 0.0;
					delta_current[1] = 0.0;
				}

				if ((bus[indexer].phases & 0x05) == 0x05)	//Check for CA
				{
					//Voltage calculations
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Power - convert to a current (uses less iterations this way)
					delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S_dy[2]/voltageDel[2]);

					//Convert delta connected load to appropriate Wye
					delta_current[2] += voltageDel[2] * (bus[indexer].Y_dy[2]);

				}
				else
				{
					//Zero unused
					voltageDel[2] = 0.0;
					delta_current[2] = 0.0;
				}
				
				//Convert delta-current into a phase current, where appropriate - reuse temp variable
				//Everything will be accumulated into the "current" field for ease (including differents)
				//Also handle wye currents in here (was a differently connected child code before)
				if ((bus[indexer].phases & 0x04) == 0x04)	//Has a phase A
				{
					undeltacurr[0]=(bus[indexer].I_dy[0]+delta_current[0])-(bus[indexer].I_dy[2]+delta_current[2]);

					//Power values
					undeltacurr[0] += (bus[indexer].V[0] == 0) ? 0 : ~(bus[indexer].S_dy[3]/bus[indexer].V[0]);

					//Shunt values
					undeltacurr[0] += bus[indexer].Y_dy[3]*bus[indexer].V[0];

					//Current values
					undeltacurr[0] += bus[indexer].I_dy[3];
				}
				else
				{
					//Zero it, just in case
					undeltacurr[0] = 0.0;
				}

				if ((bus[indexer].phases & 0x02) == 0x02)	//Has a phase B
				{
					undeltacurr[1]=(bus[indexer].I_dy[1]+delta_current[1])-(bus[indexer].I_dy[0]+delta_current[0]);

					//Power values
					undeltacurr[1] += (bus[indexer].V[1] == 0) ? 0 : ~(bus[indexer].S_dy[4]/bus[indexer].V[1]);

					//Shunt values
					undeltacurr[1] += bus[indexer].Y_dy[4]*bus[indexer].V[1];

					//Current values
					undeltacurr[1] += bus[indexer].I_dy[4];
				}
				else
				{
					//Zero it, just in case
					undeltacurr[1] = 0.0;
				}


				if ((bus[indexer].phases & 0x01) == 0x01)	//Has a phase C
				{
					undeltacurr[2]=(bus[indexer].I_dy[2]+delta_current[2])-(bus[indexer].I_dy[1]+delta_current[1]);

					//Power values
					undeltacurr[2] += (bus[indexer].V[2] == 0) ? 0 : ~(bus[indexer].S_dy[5]/bus[indexer].V[2]);

					//Shunt values
					undeltacurr[2] += bus[indexer].Y_dy[5]*bus[indexer].V[2];

					//Current values
					undeltacurr[2] += bus[indexer].I_dy[5];
				}
				else
				{
					//Zero it, just in case
					undeltacurr[2] = 0.0;
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
						
				}//End phase traversion
			}//End delta/wye explicit load update
		}//end bus traversion for power update
	
		// Calculate the mismatch of three phase current injection at each bus (deltaI), 
		//and store the deltaI in terms of real and reactive value in array powerflow_values->deltaI_NR    
		if (powerflow_values->deltaI_NR==NULL)
		{
			powerflow_values->deltaI_NR = (double *)gl_malloc((2*powerflow_values->total_variables) *sizeof(double));   // left_hand side of equation (11)

			//Make sure it worked
			if (powerflow_values->deltaI_NR == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Update the max size
			powerflow_values->max_total_variables = powerflow_values->total_variables;
		}
		else if (powerflow_values->NR_realloc_needed)		//Bigger sized (this was checked above)
		{
			//Decimate the existing value
			gl_free(powerflow_values->deltaI_NR);

			//Reallocate it...bigger...faster...stronger!
			powerflow_values->deltaI_NR = (double *)gl_malloc((2*powerflow_values->total_variables) *sizeof(double));

			//Make sure it worked
			if (powerflow_values->deltaI_NR == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Store the updated value
			powerflow_values->max_total_variables = powerflow_values->total_variables;
		}

		//If we're in PF_DYNINIT mode, initialize the balancing convergence
		if (powerflow_type == PF_DYNINIT)
		{
			swing_converged = true;	//init it to yes, fail by exception, not default
		}

		//Compute the calculated loads (not specified) at each bus
		for (indexer=0; indexer<bus_count; indexer++) //for specific bus k
		{
			//Update for generator symmetry - only in static dynamic mode and when SWING is a SWING
			if ((powerflow_type == PF_DYNINIT) && (swing_is_a_swing==true))
			{
				//Secondary check - see if it is even needed - basically built around 3-phase right now
				if ((*bus[indexer].dynamics_enabled==true) && (bus[indexer].full_Y != NULL) && (bus[indexer].DynCurrent != NULL))
				{
					//Form denominator term of Ii, since it won't change
					temp_complex_1 = (~bus[indexer].V[0]) + (~bus[indexer].V[1])*avalsq + (~bus[indexer].V[2])*aval;
					
					//Form up numerator portion that doesn't change (Q and admittance)
					//Do in parts, just for readability
					temp_complex_2 = ~bus[indexer].V[0];	//conj(Va)
					
					//Row 1 of admittance mult
					temp_complex_0 = temp_complex_2*(bus[indexer].full_Y[0]*bus[indexer].V[0] + bus[indexer].full_Y[1]*bus[indexer].V[1] + bus[indexer].full_Y[2]*bus[indexer].V[2]);

					// Row 1 also calculate Sysource ( = v * conj(ysource * v)) to substract from PTsource and obtain Pgen at generator bus 
					temp_complex_5 = bus[indexer].full_Y[0]*bus[indexer].V[0] + bus[indexer].full_Y[1]*bus[indexer].V[1] + bus[indexer].full_Y[2]*bus[indexer].V[2];
					temp_complex_4 = ~temp_complex_5;
					temp_complex_3 = bus[indexer].V[0]*temp_complex_4;

					//conj(Vb)
					temp_complex_2 = ~bus[indexer].V[1];

					//Row 2 of admittance
					temp_complex_0 += temp_complex_2*(bus[indexer].full_Y[3]*bus[indexer].V[0] + bus[indexer].full_Y[4]*bus[indexer].V[1] + bus[indexer].full_Y[5]*bus[indexer].V[2]);
					
					// Row 2 also calculate Sysource ( = v * conj(ysource * v)) to substract from PTsource and obtain Pgen at generator bus
					temp_complex_5 = bus[indexer].full_Y[3]*bus[indexer].V[0] + bus[indexer].full_Y[4]*bus[indexer].V[1] + bus[indexer].full_Y[5]*bus[indexer].V[2];
					temp_complex_4 = ~temp_complex_5;
					temp_complex_3 += bus[indexer].V[1]*temp_complex_4;

					//conj(Vc)
					temp_complex_2 = ~bus[indexer].V[2];

					//Row 3 of admittance
					temp_complex_0 += temp_complex_2*(bus[indexer].full_Y[6]*bus[indexer].V[0] + bus[indexer].full_Y[7]*bus[indexer].V[1] + bus[indexer].full_Y[8]*bus[indexer].V[2]);

					// Row 3 also calculate Sysource ( = v * conj(ysource * v)) to substract from PTsource and obtain Pgen at generator bus
					temp_complex_5 = bus[indexer].full_Y[6]*bus[indexer].V[0] + bus[indexer].full_Y[7]*bus[indexer].V[1] + bus[indexer].full_Y[8]*bus[indexer].V[2];
					temp_complex_4 = ~temp_complex_5;
					temp_complex_3 += bus[indexer].V[2]*temp_complex_4;					

					//numerator done, except PT portion (add in below - SWING bus is different

					//On that note, if we are a SWING, zero our PT portion and QT for accumulation
					if ((bus[indexer].type == 2) && (Iteration>0))
					{
						*bus[indexer].PGenTotal = complex(0.0,0.0);
					}
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

					//Get diagonal contributions - only (& always) 2
					//Column 1
					tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Im();// equation (7), the diag elements of bus admittance matrix 
					tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Re();// equation (8), the diag elements of bus admittance matrix 

					//Column 2
					tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Im();// equation (7), the diag elements of bus admittance matrix 
					tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Re();// equation (8), the diag elements of bus admittance matrix 

					//Now off diagonals
					for (kindexer=0; kindexer<(bus[indexer].Link_Table_Size); kindexer++)
					{
						//Apply proper index to jindexer (easier to implement this way)
						jindexer=bus[indexer].Link_Table[kindexer];

						if (branch[jindexer].from == indexer)	//We're the from bus
						{
							if ((bus[indexer].phases & 0x20) == 0x20)	//SPCT from bus - needs different signage
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

							}//End SPCT To bus - from diagonal contributions
							else		//Normal line connection to normal triplex
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

								work_vals_char_0 = jindex*3+temp_index;

								//Perform the update, it only happens for one column (nature of the transformer)
								tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[temp_index]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[temp_index]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

							}//end transformer
							else									//Must be a normal line then
							{
								if ((bus[indexer].phases & 0x20) == 0x20)	//SPCT from bus - needs different signage
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
								}//End SPCT To bus - from diagonal contributions
								else		//Normal line connection to normal triplex
								{
									work_vals_char_0 = jindex*3;
									//Again only, & always 2 columns (just do them explicitly)
									//Column 1
									tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Re() - (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0])).Re() * (bus[branch[jindexer].from].V[0]).Im() + (-(branch[jindexer].Yto[work_vals_char_0])).Im() * (bus[branch[jindexer].from].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

									//Column2
									tempIcalcReal += (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += (-(branch[jindexer].Yto[work_vals_char_0+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + (-(branch[jindexer].Yto[work_vals_char_0+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								}//End normal triplex connection
							}//end normal line
						}//end to bus
						else	//We're nothing
							;

					}//End branch traversion

					//Determine how we are posting this update
					if ((bus[indexer].type == 2) && swing_is_a_swing)	//SWING bus is different (when it really is a SWING bus)
					{
						if ((powerflow_type == PF_DYNINIT) && (bus[indexer].full_Y != NULL))
						{
							//Compute the delta_I, just like below - but don't post it (still zero in calcs)
							work_vals_double_3 = tempPbus - tempIcalcReal;
							work_vals_double_4 = tempQbus - tempIcalcImag;

							//Form a magnitude vector
							work_vals_double_0 = sqrt((work_vals_double_3*work_vals_double_3+work_vals_double_4*work_vals_double_4));
							
							if (work_vals_double_0 > bus[indexer].max_volt_error)	//Failure check (defaults to voltage convergence for now)
							{
								swing_converged=false;
							}
						}//End PF_DYNINIT SWING traversion

						//Must be normal run, since DYNRUN should have swing_is_a_swing deflagged
						//Normal run - zero it - should already be zerod, but let's be paranoid for now
						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = 0.0;
						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
					}//End SWING bus cases
					else	//PQ bus or SWING masquerading as a PQ
					{
						//It's I.  Just a direct conversion (P changed above to I as well)
						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = tempPbus - tempIcalcReal;
						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = tempQbus - tempIcalcImag;
					}//End Normal PQ bus
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

						//Diagonal contributions
						tempIcalcReal += (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Re() - (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Im();// equation (7), the diag elements of bus admittance matrix 
						tempIcalcImag += (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Im() + (powerflow_values->BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Re();// equation (8), the diag elements of bus admittance matrix 


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
										work_vals_char_0 = temp_index_b*3;
										//Do columns individually
										//1
										tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

										//2
										tempIcalcReal += (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[work_vals_char_0+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

									}
								}//end SPCT transformer
								else	///Must be a standard line
								{
									work_vals_char_0 = temp_index_b*3+temp_index;
									work_vals_double_0 = (-branch[jindexer].Yfrom[work_vals_char_0]).Re();
									work_vals_double_1 = (-branch[jindexer].Yfrom[work_vals_char_0]).Im();
									work_vals_double_2 = (bus[branch[jindexer].to].V[temp_index]).Re();
									work_vals_double_3 = (bus[branch[jindexer].to].V[temp_index]).Im();

									tempIcalcReal += work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

								}//end standard line
							}	
							if  (branch[jindexer].to == indexer)
							{
								work_vals_char_0 = temp_index_b*3+temp_index;
								work_vals_double_0 = (-branch[jindexer].Yto[work_vals_char_0]).Re();
								work_vals_double_1 = (-branch[jindexer].Yto[work_vals_char_0]).Im();
								work_vals_double_2 = (bus[branch[jindexer].from].V[temp_index]).Re();
								work_vals_double_3 = (bus[branch[jindexer].from].V[temp_index]).Im();

								tempIcalcReal += work_vals_double_0 * work_vals_double_2 - work_vals_double_1 * work_vals_double_3;// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								tempIcalcImag += work_vals_double_0 * work_vals_double_3 + work_vals_double_1 * work_vals_double_2;// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
							}
							else;
						}
					}//end intermediate current for each phase column

					//Determine how we are posting this update
					if ((bus[indexer].type == 2) && swing_is_a_swing)	//SWING bus is different (when it really is a SWING bus)
					{
						if ((powerflow_type == PF_DYNINIT) && (bus[indexer].full_Y != NULL))
						{
							//Compute our "power generated" value for this phase - conjugated in formation
							temp_complex_2 = bus[indexer].V[jindex] * complex(tempIcalcReal,-tempIcalcImag);

							if (Iteration>0)	//Only update SWING on subsequent passes
							{
								//Now add it into the "generation total" for the SWING
								(*bus[indexer].PGenTotal) += temp_complex_2 - temp_complex_3/3.0; // both PT and QT = Ssource-Sysource = v conj(I) - v conj(ysource v)
							}
								
							//Compute the delta_I, just like below - but don't post it (still zero in calcs)
							work_vals_double_0 = (bus[indexer].V[temp_index_b]).Mag()*(bus[indexer].V[temp_index_b]).Mag();

							if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
							{
								work_vals_double_1 = (bus[indexer].V[temp_index_b]).Re();
								work_vals_double_2 = (bus[indexer].V[temp_index_b]).Im();
								work_vals_double_3 = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal ; // equation(7), Real part of deltaI, left hand side of equation (11)
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
								swing_converged=false;
							}
						}//End PF_DYNINIT SWING traversion

						//Zero out the components, regardless of normal run or not
						//Should already be zerod, but do it again for paranoia sake
						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = 0.0;
						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
					}//End SWING bus cases
					else	//PQ bus or SWING masquerading as a PQ
					{
						work_vals_double_0 = (bus[indexer].V[temp_index_b]).Mag()*(bus[indexer].V[temp_index_b]).Mag();

						if (work_vals_double_0!=0)	//Only normal one (not square), but a zero is still a zero even after that
						{
							work_vals_double_1 = (bus[indexer].V[temp_index_b]).Re();
							work_vals_double_2 = (bus[indexer].V[temp_index_b]).Im();
							powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+ powerflow_values->BA_diag[indexer].size + jindex] = (tempPbus * work_vals_double_1 + tempQbus * work_vals_double_2)/ (work_vals_double_0) - tempIcalcReal ; // equation(7), Real part of deltaI, left hand side of equation (11)
							powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempPbus * work_vals_double_2 - tempQbus * work_vals_double_1)/ (work_vals_double_0) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)
						}
						else
						{
           					powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + jindex] = 0.0;
							powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
						}
					}//End normal bus handling
				}//End three-phase or variant thereof
			}//End delta_I for each phase row

			//Add in generator current amounts, if relevant
			if (powerflow_type != PF_NORMAL)
			{
				//Secondary check - see if it is even needed - basically built around 3-phase right now (checks)
				if ((*bus[indexer].dynamics_enabled==true) && (bus[indexer].full_Y != NULL) && (bus[indexer].DynCurrent != NULL))
				{
					if ((bus[indexer].phases & 0x07) == 0x07)	//Make sure is three-phase
					{
						//Only update in particular mode - SWING bus values should still be treated as such
						if ((swing_is_a_swing == true) && (powerflow_type == PF_DYNINIT))	//Really only true for PF_DYNINIT anyways
						{
							//Power should be all updated, now update current values
							temp_complex_0 += complex((*bus[indexer].PGenTotal).Re(),-(*bus[indexer].PGenTotal).Im()); // total generated power injected congugated

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
								bus[indexer].DynCurrent[0] = complex(0.0,0.0);
								bus[indexer].DynCurrent[1] = complex(0.0,0.0);
								bus[indexer].DynCurrent[2] = complex(0.0,0.0);
							}
						}//End SWING is still swing, otherwise should just accumulate what it had

						//Don't get added in as part of "normal swing" routine
						if ((bus[indexer].type == 0) || ((bus[indexer].type==2) && (swing_is_a_swing==false)))
						{
							//Add these into the system - added because "generation"
       						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size] += bus[indexer].DynCurrent[0].Re();		//Phase A
       						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 1] += bus[indexer].DynCurrent[1].Re();	//Phase B
       						powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size + 2] += bus[indexer].DynCurrent[2].Re();	//Phase C

							powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc] += bus[indexer].DynCurrent[0].Im();		//Phase A
							powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc + 1] += bus[indexer].DynCurrent[1].Im();	//Phase B
							powerflow_values->deltaI_NR[2*bus[indexer].Matrix_Loc + 2] += bus[indexer].DynCurrent[2].Im();	//Phase C
						}
						//Defaulted else - leave as they were
					}//End three-phase
					else	//Not three-phase
					{
						GL_THROW("NR_solver: bus:%s has tried updating deltamode dyanmics, but is not three-phase!",bus[indexer].name);
						/*  TROUBLESHOOT
						The NR_solver routine tried update a three-phase current value for a bus that is not
						three phase.  At this time, the deltamode system only supports three-phase buses for
						generator attachment.
						*/
					}
				}//End extra current adds
			}//End dynamic (generator postings)
		}//End delta_I for each bus

		// Calculate the elements of a,b,c,d in equations(14),(15),(16),(17). These elements are used to update the Jacobian matrix.	
		for (indexer=0; indexer<bus_count; indexer++)
		{
			if ((bus[indexer].phases & 0x08) == 0x08)	//Delta connected node
			{
				//Populate the values for constant current -- deltamode different right now (all same in future?)
				if (*bus[indexer].dynamics_enabled == true)
				{
					//Create nominal magnitudes
					adjust_nominal_voltage_val = bus[indexer].kv_base * sqrt(3.0);

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
						adjusted_constant_current[0] = 0.0;
					}

					//Start adjustments - BC
					if ((bus[indexer].I[1] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[1] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].I[1] * adjust_temp_voltage_mag[1] / (voltageDel[1] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[1] = 0.0;
					}

					//Start adjustments - CA
					if ((bus[indexer].I[2] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[2] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].I[2] * adjust_temp_voltage_mag[2] / (voltageDel[2] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[2] = 0.0;
					}

					//See if we have any "different children"
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Create nominal magnitudes
						adjust_nominal_voltage_val = bus[indexer].kv_base;

						//Create the nominal voltage vectors
						adjust_temp_nominal_voltage[0].SetPolar(bus[indexer].kv_base,0.0);
						adjust_temp_nominal_voltage[1].SetPolar(bus[indexer].kv_base,-2.0*PI/3.0);
						adjust_temp_nominal_voltage[2].SetPolar(bus[indexer].kv_base,2.0*PI/3.0);

						//Get magnitudes of all
						adjust_temp_voltage_mag[0] = bus[indexer].V[0].Mag();
						adjust_temp_voltage_mag[1] = bus[indexer].V[1].Mag();
						adjust_temp_voltage_mag[2] = bus[indexer].V[2].Mag();

						//Start adjustments - A
						if ((bus[indexer].extra_var[6] != 0.0) && (adjust_temp_voltage_mag[0] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[3] = ~(adjust_temp_nominal_voltage[0] * ~bus[indexer].extra_var[6] * adjust_temp_voltage_mag[0] / (bus[indexer].V[0] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[3] = 0.0;
						}

						//Start adjustments - B
						if ((bus[indexer].extra_var[7] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[4] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].extra_var[7] * adjust_temp_voltage_mag[1] / (bus[indexer].V[1] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[4] = 0.0;
						}

						//Start adjustments - C
						if ((bus[indexer].extra_var[8] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[5] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].extra_var[8] * adjust_temp_voltage_mag[2] / (bus[indexer].V[2] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[5] = 0.0;
						}
					}
					else	//Nope
					{
						//Set to zero, just cause
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}
				else	//"Normal" modes -- handle traditionally
				{
					adjusted_constant_current[0] = bus[indexer].I[0];
					adjusted_constant_current[1] = bus[indexer].I[1];
					adjusted_constant_current[2] = bus[indexer].I[2];

					//See if we have different children too
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Store them too
						adjusted_constant_current[3] = bus[indexer].extra_var[6];
						adjusted_constant_current[4] = bus[indexer].extra_var[7];
						adjusted_constant_current[5] = bus[indexer].extra_var[8];
					}
					else	//Nope, just zero this for now
					{
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}//End adjustment code

				//Delta components - populate according to what is there
				if ((bus[indexer].phases & 0x06) == 0x06)	//Check for AB
				{
					//Voltage calculations
					voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];

					//Power - convert to a current (uses less iterations this way)
					delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].S[0]/voltageDel[0]);

					//Convert delta connected load to appropriate Wye
					delta_current[0] += voltageDel[0] * (bus[indexer].Y[0]);

				}
				else
				{
					//Zero values - they shouldn't be used anyhow
					voltageDel[0] = 0.0;
					delta_current[0] = 0.0;
				}

				if ((bus[indexer].phases & 0x03) == 0x03)	//Check for BC
				{
					//Voltage calculations
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

					//Power - convert to a current (uses less iterations this way)
					delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S[1]/voltageDel[1]);

					//Convert delta connected load to appropriate Wye
					delta_current[1] += voltageDel[1] * (bus[indexer].Y[1]);

				}
				else
				{
					//Zero unused
					voltageDel[1] = 0.0;
					delta_current[1] = 0.0;
				}

				if ((bus[indexer].phases & 0x05) == 0x05)	//Check for CA
				{
					//Voltage calculations
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Power - convert to a current (uses less iterations this way)
					delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S[2]/voltageDel[2]);

					//Convert delta connected load to appropriate Wye
					delta_current[2] += voltageDel[2] * (bus[indexer].Y[2]);

				}
				else
				{
					//Zero unused
					voltageDel[2] = 0.0;
					delta_current[2] = 0.0;
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
						undeltacurr[0] += bus[indexer].extra_var[3]*bus[indexer].V[0];

						//Current values
						undeltacurr[0] += adjusted_constant_current[3];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[0] = 0.0;
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
						undeltacurr[1] += bus[indexer].extra_var[4]*bus[indexer].V[1];

						//Current values
						undeltacurr[1] += adjusted_constant_current[4];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[1] = 0.0;
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
						undeltacurr[2] += bus[indexer].extra_var[5]*bus[indexer].V[2];

						//Current values
						undeltacurr[2] += adjusted_constant_current[5];
					}
				}
				else
				{
					//Zero it, just in case
					undeltacurr[2] = 0.0;
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

					if ((temp_index==-1) || (temp_index_b==-1))
					{
						GL_THROW("NR: A Jacobian update element failed.");
						//Defined below
					}

					if ((bus[indexer].V[temp_index_b]).Mag()!=0)
					{
						bus[indexer].Jacob_A[temp_index] = ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(37) - no power term needed
						bus[indexer].Jacob_B[temp_index] = -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() + (undeltacurr[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(38) - no power term needed
						bus[indexer].Jacob_C[temp_index] =((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(39) - no power term needed
						bus[indexer].Jacob_D[temp_index] = ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() - (undeltacurr[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// second part of equation(40) - no power term needed
					}
					else	//Zero voltage = only impedance is valid (others get divided by VMag, so are IND) - not entirely sure how this gets in here anyhow
					{
						bus[indexer].Jacob_A[temp_index] = -1e-4;	//Small offset to avoid singularities (if impedance is zero too)
						bus[indexer].Jacob_B[temp_index] = -1e-4;
						bus[indexer].Jacob_C[temp_index] = -1e-4;
						bus[indexer].Jacob_D[temp_index] = -1e-4;
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

				//Now add in power contributions
				temp_current[0] += bus[indexer].V[0] == 0.0 ? 0.0 : ~(bus[indexer].S[0]/bus[indexer].V[0]);
				temp_current[1] += bus[indexer].V[1] == 0.0 ? 0.0 : ~(bus[indexer].S[1]/bus[indexer].V[1]);
				temp_current[2] += voltageDel[0] == 0.0 ? 0.0 : ~(bus[indexer].S[2]/voltageDel[0]);

				//Last, but not least, admittance/impedance contributions
				temp_current[0] += bus[indexer].Y[0]*bus[indexer].V[0];
				temp_current[1] += bus[indexer].Y[1]*bus[indexer].V[1];
				temp_current[2] += bus[indexer].Y[2]*voltageDel[0];

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

				//Convert 'em to line currents - they need to be negated (due to the convention from earlier)
				temp_store[0] = -(temp_current[0] + temp_current[2]);
				temp_store[1] = -(-temp_current[1] - temp_current[2]);

				for (jindex=0; jindex<2; jindex++)
				{
					if ((bus[indexer].V[jindex]).Mag()!=0)	//Only current
					{
						bus[indexer].Jacob_A[jindex] = ((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Re() + (temp_store[jindex]).Im() *pow((bus[indexer].V[jindex]).Im(),2))/pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(37)
						bus[indexer].Jacob_B[jindex] = -((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Im() + (temp_store[jindex]).Re() *pow((bus[indexer].V[jindex]).Re(),2))/pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(38)
						bus[indexer].Jacob_C[jindex] =((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Im() - (temp_store[jindex]).Re() *pow((bus[indexer].V[jindex]).Im(),2))/pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(39)
						bus[indexer].Jacob_D[jindex] = ((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Re() - (temp_store[jindex]).Im() *pow((bus[indexer].V[jindex]).Re(),2))/pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(40)
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

			}//end split-phase connected
			else	//Wye-connected system/load
			{
				//Populate the values for constant current -- deltamode different right now (all same in future?)
				if (*bus[indexer].dynamics_enabled == true)
				{
					//Create nominal magnitudes
					adjust_nominal_voltage_val = bus[indexer].kv_base;

					//Create the nominal voltage vectors
					adjust_temp_nominal_voltage[0].SetPolar(bus[indexer].kv_base,0.0);
					adjust_temp_nominal_voltage[1].SetPolar(bus[indexer].kv_base,-2.0*PI/3.0);
					adjust_temp_nominal_voltage[2].SetPolar(bus[indexer].kv_base,2.0*PI/3.0);

					//Get magnitudes of all
					adjust_temp_voltage_mag[0] = bus[indexer].V[0].Mag();
					adjust_temp_voltage_mag[1] = bus[indexer].V[1].Mag();
					adjust_temp_voltage_mag[2] = bus[indexer].V[2].Mag();

					//Start adjustments - A
					if ((bus[indexer].I[0] != 0.0) && (adjust_temp_voltage_mag[0] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[0] = ~(adjust_temp_nominal_voltage[0] * ~bus[indexer].I[0] * adjust_temp_voltage_mag[0] / (bus[indexer].V[0] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[0] = 0.0;
					}

					//Start adjustments - B
					if ((bus[indexer].I[1] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[1] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].I[1] * adjust_temp_voltage_mag[1] / (bus[indexer].V[1] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[1] = 0.0;
					}

					//Start adjustments - C
					if ((bus[indexer].I[2] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
					{
						//calculate new value
						adjusted_constant_current[2] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].I[2] * adjust_temp_voltage_mag[2] / (bus[indexer].V[2] * adjust_nominal_voltage_val));
					}
					else
					{
						adjusted_constant_current[2] = 0.0;
					}

					//See if we have any "different children"
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Create nominal magnitudes
						adjust_nominal_voltage_val = bus[indexer].kv_base * sqrt(3.0);

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
							adjusted_constant_current[3] = 0.0;
						}

						//Start adjustments - BC
						if ((bus[indexer].extra_var[7] != 0.0) && (adjust_temp_voltage_mag[1] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[4] = ~(adjust_temp_nominal_voltage[1] * ~bus[indexer].extra_var[7] * adjust_temp_voltage_mag[1] / (voltageDel[1] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[4] = 0.0;
						}

						//Start adjustments - CA
						if ((bus[indexer].extra_var[8] != 0.0) && (adjust_temp_voltage_mag[2] != 0.0))
						{
							//calculate new value
							adjusted_constant_current[5] = ~(adjust_temp_nominal_voltage[2] * ~bus[indexer].extra_var[8] * adjust_temp_voltage_mag[2] / (voltageDel[2] * adjust_nominal_voltage_val));
						}
						else
						{
							adjusted_constant_current[5] = 0.0;
						}
					}
					else	//Nope
					{
						//Set to zero, just cause
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}
				else	//"Normal" modes -- handle traditionally
				{
					adjusted_constant_current[0] = bus[indexer].I[0];
					adjusted_constant_current[1] = bus[indexer].I[1];
					adjusted_constant_current[2] = bus[indexer].I[2];

					//See if we have different children too
					if ((bus[indexer].phases & 0x10) == 0x10)
					{
						//Store them too
						adjusted_constant_current[3] = bus[indexer].extra_var[6];
						adjusted_constant_current[4] = bus[indexer].extra_var[7];
						adjusted_constant_current[5] = bus[indexer].extra_var[8];
					}
					else	//Nope, just zero this for now
					{
						adjusted_constant_current[3] = 0.0;
						adjusted_constant_current[4] = 0.0;
						adjusted_constant_current[5] = 0.0;
					}
				}//End adjustment code

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
						delta_current[0] += voltageDel[0] * (bus[indexer].extra_var[3]);
					}
					else
					{
						//Zero it, for good measure
						voltageDel[0] = 0.0;
						delta_current[0] = 0.0;
					}

					//Check for BC
					if ((bus[indexer].phases & 0x03) == 0x03)	//Has B-C
					{
						//Delta voltages
						voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

						//Power - put into a current value (iterates less this way)
						delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].extra_var[1]/voltageDel[1]);

						//Convert delta connected load to appropriate Wye 
						delta_current[1] += voltageDel[1] * (bus[indexer].extra_var[4]);
					}
					else
					{
						//Zero it, for good measure
						voltageDel[1] = 0.0;
						delta_current[1] = 0.0;
					}

					//Check for CA
					if ((bus[indexer].phases & 0x05) == 0x05)	//Has C-A
					{
						//Delta voltages
						voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

						//Power - put into a current value (iterates less this way)
						delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].extra_var[2]/voltageDel[2]);

						//Convert delta connected load to appropriate Wye 
						delta_current[2] += voltageDel[2] * (bus[indexer].extra_var[5]);
					}
					else
					{
						//Zero it, for good measure
						voltageDel[2] = 0.0;
						delta_current[2] = 0.0;
					}

					//Convert delta-current into a phase current - reuse temp variable
					undeltacurr[0]=(adjusted_constant_current[3]+delta_current[0])-(adjusted_constant_current[5]+delta_current[2]);
					undeltacurr[1]=(adjusted_constant_current[4]+delta_current[1])-(adjusted_constant_current[3]+delta_current[0]);
					undeltacurr[2]=(adjusted_constant_current[5]+delta_current[2])-(adjusted_constant_current[4]+delta_current[1]);
				}
				else	//zero the variable so we don't have excessive ifs
				{
					undeltacurr[0] = undeltacurr[1] = undeltacurr[2] = 0.0;	//Zero it
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
						bus[indexer].Jacob_A[temp_index] = ((bus[indexer].S[temp_index_b]).Im() * (pow((bus[indexer].V[temp_index_b]).Re(),2) - pow((bus[indexer].V[temp_index_b]).Im(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Re())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(37)
						bus[indexer].Jacob_A[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Re() + (adjusted_constant_current[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) + (bus[indexer].Y[temp_index_b]).Im();// second part of equation(37)
						bus[indexer].Jacob_A[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// current part of equation (37) - Handles "different" children
						
						bus[indexer].Jacob_B[temp_index] = ((bus[indexer].S[temp_index_b]).Re() * (pow((bus[indexer].V[temp_index_b]).Re(),2) - pow((bus[indexer].V[temp_index_b]).Im(),2)) + 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Im())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(38)
						bus[indexer].Jacob_B[temp_index] += -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Im() + (adjusted_constant_current[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Re();// second part of equation(38)
						bus[indexer].Jacob_B[temp_index] += -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() + (undeltacurr[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// current part of equation(38) - Handles "different" children
						
						bus[indexer].Jacob_C[temp_index] = ((bus[indexer].S[temp_index_b]).Re() * (pow((bus[indexer].V[temp_index_b]).Im(),2) - pow((bus[indexer].V[temp_index_b]).Re(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Im())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(39)
						bus[indexer].Jacob_C[temp_index] +=((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Im() - (adjusted_constant_current[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Re();// second part of equation(39)
						bus[indexer].Jacob_C[temp_index] +=((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// Current part of equation(39) - Handles "different" children
						
						bus[indexer].Jacob_D[temp_index] = ((bus[indexer].S[temp_index_b]).Im() * (pow((bus[indexer].V[temp_index_b]).Re(),2) - pow((bus[indexer].V[temp_index_b]).Im(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Re())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(40)
						bus[indexer].Jacob_D[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(adjusted_constant_current[temp_index_b]).Re() - (adjusted_constant_current[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Im();// second part of equation(40)
						bus[indexer].Jacob_D[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() - (undeltacurr[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3);// Current part of equation(40) - Handles "different" children
					
					}
					else
					{
						bus[indexer].Jacob_A[temp_index]= (bus[indexer].Y[temp_index_b]).Im() - 1e-4;	//Small offset to avoid singularity issues
						bus[indexer].Jacob_B[temp_index]= -(bus[indexer].Y[temp_index_b]).Re() - 1e-4;
						bus[indexer].Jacob_C[temp_index]= -(bus[indexer].Y[temp_index_b]).Re() - 1e-4;
						bus[indexer].Jacob_D[temp_index]= -(bus[indexer].Y[temp_index_b]).Im() - 1e-4;
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

					//Convert delta connected load to appropriate Wye
					delta_current[0] += voltageDel[0] * (bus[indexer].Y_dy[0]);

				}
				else
				{
					//Zero values - they shouldn't be used anyhow
					voltageDel[0] = 0.0;
					delta_current[0] = 0.0;
				}

				if ((bus[indexer].phases & 0x03) == 0x03)	//Check for BC
				{
					//Voltage calculations
					voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];

					//Power - convert to a current (uses less iterations this way)
					delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S_dy[1]/voltageDel[1]);

					//Convert delta connected load to appropriate Wye
					delta_current[1] += voltageDel[1] * (bus[indexer].Y_dy[1]);

				}
				else
				{
					//Zero unused
					voltageDel[1] = 0.0;
					delta_current[1] = 0.0;
				}

				if ((bus[indexer].phases & 0x05) == 0x05)	//Check for CA
				{
					//Voltage calculations
					voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

					//Power - convert to a current (uses less iterations this way)
					delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S_dy[2]/voltageDel[2]);

					//Convert delta connected load to appropriate Wye
					delta_current[2] += voltageDel[2] * (bus[indexer].Y_dy[2]);

				}
				else
				{
					//Zero unused
					voltageDel[2] = 0.0;
					delta_current[2] = 0.0;
				}
				
				//Convert delta-current into a phase current, where appropriate - reuse temp variable
				//Everything will be accumulated into the "current" field for ease (including differents)
				if ((bus[indexer].phases & 0x04) == 0x04)	//Has a phase A
				{
					undeltacurr[0]=(bus[indexer].I_dy[0]+delta_current[0])-(bus[indexer].I_dy[2]+delta_current[2]);

					//Apply wye-connected loads

					//Power values
					undeltacurr[0] += (bus[indexer].V[0] == 0) ? 0 : ~(bus[indexer].S_dy[3]/bus[indexer].V[0]);

					//Shunt values
					undeltacurr[0] += bus[indexer].Y_dy[3]*bus[indexer].V[0];

					//Current values
					undeltacurr[0] += bus[indexer].I_dy[3];
				}
				else
				{
					//Zero it, just in case
					undeltacurr[0] = 0.0;
				}

				if ((bus[indexer].phases & 0x02) == 0x02)	//Has a phase B
				{
					undeltacurr[1]=(bus[indexer].I_dy[1]+delta_current[1])-(bus[indexer].I_dy[0]+delta_current[0]);

					//Apply wye-connected loads

					//Power values
					undeltacurr[1] += (bus[indexer].V[1] == 0) ? 0 : ~(bus[indexer].S_dy[4]/bus[indexer].V[1]);

					//Shunt values
					undeltacurr[1] += bus[indexer].Y_dy[4]*bus[indexer].V[1];

					//Current values
					undeltacurr[1] += bus[indexer].I_dy[4];
				}
				else
				{
					//Zero it, just in case
					undeltacurr[1] = 0.0;
				}


				if ((bus[indexer].phases & 0x01) == 0x01)	//Has a phase C
				{
					undeltacurr[2]=(bus[indexer].I_dy[2]+delta_current[2])-(bus[indexer].I_dy[1]+delta_current[1]);

					//Apply wye-connected loads

					//Power values
					undeltacurr[2] += (bus[indexer].V[2] == 0) ? 0 : ~(bus[indexer].S_dy[5]/bus[indexer].V[2]);

					//Shunt values
					undeltacurr[2] += bus[indexer].Y_dy[5]*bus[indexer].V[2];

					//Current values
					undeltacurr[2] += bus[indexer].I_dy[5];
				}
				else
				{
					//Zero it, just in case
					undeltacurr[2] = 0.0;
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

					if ((temp_index==-1) || (temp_index_b==-1))
					{
						GL_THROW("NR: A Jacobian update element failed.");
						//Defined below
					}

					if ((bus[indexer].V[temp_index_b]).Mag()!=0)
					{
						//Apply as an accumulation, in case any "normal" connections are present too
						bus[indexer].Jacob_A[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() + (undeltacurr[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3); // + (undeltaimped[temp_index_b]).Im();// second part of equation(37) - no power term needed
						bus[indexer].Jacob_B[temp_index] += -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() + (undeltacurr[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3); // - (undeltaimped[temp_index_b]).Re();// second part of equation(38) - no power term needed
						bus[indexer].Jacob_C[temp_index] +=((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Im() - (undeltacurr[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3); // - (undeltaimped[temp_index_b]).Re();// second part of equation(39) - no power term needed
						bus[indexer].Jacob_D[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(undeltacurr[temp_index_b]).Re() - (undeltacurr[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3); // - (undeltaimped[temp_index_b]).Im();// second part of equation(40) - no power term needed
					}
					else	//Zero voltage = only impedance is valid (others get divided by VMag, so are IND) - not entirely sure how this gets in here anyhow
					{
						bus[indexer].Jacob_A[temp_index] += -1e-4; //(undeltaimped[temp_index_b]).Im() - 1e-4;	//Small offset to avoid singularities (if impedance is zero too)
						bus[indexer].Jacob_B[temp_index] += -1e-4; //-(undeltaimped[temp_index_b]).Re() - 1e-4;
						bus[indexer].Jacob_C[temp_index] += -1e-4; //-(undeltaimped[temp_index_b]).Re() - 1e-4;
						bus[indexer].Jacob_D[temp_index] += -1e-4; //-(undeltaimped[temp_index_b]).Im() - 1e-4;
					}
				}//End phase traversion
			}//End delta/wye explicit loads
		}//end bus traversion for a,b,c, d value computation

		//Build the dynamic diagnal elements of 6n*6n Y matrix. All the elements in this part will be updated at each iteration.
		unsigned int size_diag_update = 0;
		for (jindexer=0; jindexer<bus_count;jindexer++) 
		{
			if  (bus[jindexer].type != 1)	//PV bus ignored (for now?)
				size_diag_update += powerflow_values->BA_diag[jindexer].size; 
			//Defaulted else - PV bus ignored
		}
		
		if (powerflow_values->Y_diag_update == NULL)
		{
			powerflow_values->Y_diag_update = (Y_NR *)gl_malloc((4*size_diag_update) *sizeof(Y_NR));   //powerflow_values->Y_diag_update store the row,column and value of the dynamic part of the diagonal PQ bus elements of 6n*6n Y_NR matrix.

			//Make sure it worked
			if (powerflow_values->Y_diag_update == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Update maximum size
			powerflow_values->max_size_diag_update = size_diag_update;
		}
		else if (size_diag_update > powerflow_values->max_size_diag_update)	//We've exceeded our limits
		{
			//Disappear the old one
			gl_free(powerflow_values->Y_diag_update);

			//Make a new one in its image
			powerflow_values->Y_diag_update = (Y_NR *)gl_malloc((4*size_diag_update) *sizeof(Y_NR));

			//Make sure it worked
			if (powerflow_values->Y_diag_update == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");

			//Update the size
			powerflow_values->max_size_diag_update = size_diag_update;

			//Flag for a realloc
			powerflow_values->NR_realloc_needed = true;
		}

		indexer = 0;	//Rest positional counter

		for (jindexer=0; jindexer<bus_count; jindexer++)	//Parse through bus list
		{
			if ((bus[jindexer].type == 2) && swing_is_a_swing)	//Swing bus - and we aren't ignoring it
			{
				for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
				{
					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					powerflow_values->Y_diag_update[indexer].col_ind = powerflow_values->Y_diag_update[indexer].row_ind;
					powerflow_values->Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;

					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					powerflow_values->Y_diag_update[indexer].col_ind = powerflow_values->Y_diag_update[indexer].row_ind + powerflow_values->BA_diag[jindexer].size;
					powerflow_values->Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re();	//Normal admittance portion
					indexer += 1;

					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
					powerflow_values->Y_diag_update[indexer].col_ind = powerflow_values->Y_diag_update[indexer].row_ind - powerflow_values->BA_diag[jindexer].size;
					powerflow_values->Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re();	//Normal admittance portion
					indexer += 1;

					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
					powerflow_values->Y_diag_update[indexer].col_ind = powerflow_values->Y_diag_update[indexer].row_ind;
					powerflow_values->Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;
				}//End swing bus traversion
			}//End swing bus

			if ((bus[jindexer].type == 0) || ((bus[jindexer].type == 2) && swing_is_a_swing==false))	//Only do on PQ (or SWING masquerading as PQ)
			{
				for (jindex=0; jindex<powerflow_values->BA_diag[jindexer].size; jindex++)
				{
					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					powerflow_values->Y_diag_update[indexer].col_ind = powerflow_values->Y_diag_update[indexer].row_ind;
					powerflow_values->Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_A[jindex]; // Equation(14)
					indexer += 1;
					
					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					powerflow_values->Y_diag_update[indexer].col_ind = powerflow_values->Y_diag_update[indexer].row_ind + powerflow_values->BA_diag[jindexer].size;
					powerflow_values->Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_B[jindex]; // Equation(15)
					indexer += 1;
					
					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
					powerflow_values->Y_diag_update[indexer].col_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					powerflow_values->Y_diag_update[indexer].Y_value = (powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_C[jindex]; // Equation(16)
					indexer += 1;
					
					powerflow_values->Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + powerflow_values->BA_diag[jindexer].size;
					powerflow_values->Y_diag_update[indexer].col_ind = powerflow_values->Y_diag_update[indexer].row_ind;
					powerflow_values->Y_diag_update[indexer].Y_value = -(powerflow_values->BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_D[jindex]; // Equation(17)
					indexer += 1;
				}//end PQ phase traversion
			}//End PQ bus
		}//End bus parse list

		// Build the Amatrix, Amatrix includes all the elements of Y_offdiag_PQ, Y_diag_fixed and Y_diag_update.
		size_Amatrix = powerflow_values->size_offdiag_PQ*2 + powerflow_values->size_diag_fixed*2 + 4*size_diag_update;

		//Test to make sure it isn't an empty matrix - reliability induced 3-phase fault
		if (size_Amatrix==0)
		{
			gl_warning("Empty powerflow connectivity matrix, your system is empty!");
			/*  TROUBLESHOOT
			Newton-Raphson has an empty admittance matrix that it is trying to solve.  Either the whole system
			faulted, or something is not properly defined.  Please try again.  If the problem persists, please
			submit your code and a bug report via the trac website.
			*/

			*bad_computations = false;	//Ensure output is flagged ok
			return 0;					//Just return some arbitrary value - not technically bad
		}

		if (powerflow_values->Y_Amatrix == NULL)
		{
			powerflow_values->Y_Amatrix = (Y_NR *)gl_malloc((size_Amatrix) *sizeof(Y_NR));   // Amatrix includes all the elements of powerflow_values->Y_offdiag_PQ, powerflow_values->Y_diag_fixed and powerflow_values->Y_diag_update.

			//Make sure it worked
			if (powerflow_values->Y_Amatrix == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");
		}
		else if (powerflow_values->NR_realloc_needed)	//If one of the above changed, we changed too
		{
			//Destroy the faulty version
			gl_free(powerflow_values->Y_Amatrix);

			//Create a new one that holds our new ampleness
			powerflow_values->Y_Amatrix = (Y_NR *)gl_malloc((size_Amatrix) *sizeof(Y_NR));

			//Make sure it worked
			if (powerflow_values->Y_Amatrix == NULL)
				GL_THROW("NR: Failed to allocate memory for one of the necessary matrices");
		}

		//integrate off diagonal components
		for (indexer=0; indexer<powerflow_values->size_offdiag_PQ*2; indexer++)
		{
			powerflow_values->Y_Amatrix[indexer].row_ind = powerflow_values->Y_offdiag_PQ[indexer].row_ind;
			powerflow_values->Y_Amatrix[indexer].col_ind = powerflow_values->Y_offdiag_PQ[indexer].col_ind;
			powerflow_values->Y_Amatrix[indexer].Y_value = powerflow_values->Y_offdiag_PQ[indexer].Y_value;
		}

		//Integrate fixed portions of diagonal components
		for (indexer=powerflow_values->size_offdiag_PQ*2; indexer< (powerflow_values->size_offdiag_PQ*2 + powerflow_values->size_diag_fixed*2); indexer++)
		{
			powerflow_values->Y_Amatrix[indexer].row_ind = powerflow_values->Y_diag_fixed[indexer - powerflow_values->size_offdiag_PQ*2 ].row_ind;
			powerflow_values->Y_Amatrix[indexer].col_ind = powerflow_values->Y_diag_fixed[indexer - powerflow_values->size_offdiag_PQ*2 ].col_ind;
			powerflow_values->Y_Amatrix[indexer].Y_value = powerflow_values->Y_diag_fixed[indexer - powerflow_values->size_offdiag_PQ*2 ].Y_value;
		}

		//Integrate the variable portions of the diagonal components
		for (indexer=powerflow_values->size_offdiag_PQ*2 + powerflow_values->size_diag_fixed*2; indexer< size_Amatrix; indexer++)
		{
			powerflow_values->Y_Amatrix[indexer].row_ind = powerflow_values->Y_diag_update[indexer - powerflow_values->size_offdiag_PQ*2 - powerflow_values->size_diag_fixed*2].row_ind;
			powerflow_values->Y_Amatrix[indexer].col_ind = powerflow_values->Y_diag_update[indexer - powerflow_values->size_offdiag_PQ*2 - powerflow_values->size_diag_fixed*2].col_ind;
			powerflow_values->Y_Amatrix[indexer].Y_value = powerflow_values->Y_diag_update[indexer - powerflow_values->size_offdiag_PQ*2 - powerflow_values->size_diag_fixed*2].Y_value;
		}

		/* sorting integers */
		//Declare working array
		if (powerflow_values->Y_Work_Amatrix == NULL)
		{
			powerflow_values->Y_Work_Amatrix = (Y_NR *)gl_malloc(size_Amatrix*sizeof(Y_NR));
			if (powerflow_values->Y_Work_Amatrix==NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");
		}
		else if (powerflow_values->NR_realloc_needed)	//Y_Amatrix was likely resized, so we need it too since we's cousins
		{
			//Get rid of the old
			gl_free(powerflow_values->Y_Work_Amatrix);

			//And in with the new
			powerflow_values->Y_Work_Amatrix = (Y_NR *)gl_malloc(size_Amatrix*sizeof(Y_NR));
			if (powerflow_values->Y_Work_Amatrix==NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");
		}

		merge_sort(powerflow_values->Y_Amatrix, size_Amatrix, powerflow_values->Y_Work_Amatrix);
		
#ifdef NR_MATRIX_OUT
		//Debugging code to export the sparse matrix values - useful for debugging issues, but needs preprocessor declaration
		
		//Open a text file
		FILE *FPoutVal=fopen("matrixinfoout.txt","wt");

		//Print the values - printed as "row index, column index, value"
		//This particular output is after they have been column sorted for the algorithm
		fprintf(FPoutVal,"Matrix Information - size = %d\n",size_Amatrix);

		fprintf(FPoutVal,"Matrix Information - row, column, value\n");
		for (jindexer=0; jindexer<size_Amatrix; jindexer++)
		{
			fprintf(FPoutVal,"%d,%d,%f\n",powerflow_values->Y_Amatrix[jindexer].row_ind,powerflow_values->Y_Amatrix[jindexer].col_ind,powerflow_values->Y_Amatrix[jindexer].Y_value);
		}

		//Close the file, we're done with it
		fclose(FPoutVal);
#endif

		///* Initialize parameters. */
		m = 2*powerflow_values->total_variables; n = 2*powerflow_values->total_variables; nnz = size_Amatrix;

		if (matrices_LU.a_LU == NULL)	//First run
		{
			/* Set aside space for the arrays. */
			matrices_LU.a_LU = (double *) gl_malloc(nnz *sizeof(double));
			if (matrices_LU.a_LU==NULL)
			{
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");
				/*  TROUBLESHOOT
				While attempting to allocate the memory for one of the SuperLU working matrices,
				an error was encountered and it was not allocated.  Please try again.  If it fails
				again, please submit your code and a bug report using the trac website.
				*/
			}
			
			matrices_LU.rows_LU = (int *) gl_malloc(nnz *sizeof(int));
			if (matrices_LU.rows_LU == NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			matrices_LU.cols_LU = (int *) gl_malloc((n+1) *sizeof(int));
			if (matrices_LU.cols_LU == NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			/* Create the right-hand side matrix B. */
			matrices_LU.rhs_LU = (double *) gl_malloc(m *sizeof(double));
			if (matrices_LU.rhs_LU == NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			if (matrix_solver_method==MM_SUPERLU)
			{
				///* Set up the arrays for the permutations. */
				perm_r = (int *) gl_malloc(m *sizeof(int));
				if (perm_r == NULL)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				perm_c = (int *) gl_malloc(n *sizeof(int));
				if (perm_c == NULL)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				//Set up storage pointers - single element, but need to be malloced for some reason
				A_LU.Store = (void *)gl_malloc(sizeof(NCformat));
				if (A_LU.Store == NULL)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				B_LU.Store = (void *)gl_malloc(sizeof(DNformat));
				if (B_LU.Store == NULL)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				//Populate these structures - A_LU matrix
				A_LU.Stype = SLU_NC;
				A_LU.Dtype = SLU_D;
				A_LU.Mtype = SLU_GE;
				A_LU.nrow = n;
				A_LU.ncol = m;

				//Populate these structures - B_LU matrix
				B_LU.Stype = SLU_DN;
				B_LU.Dtype = SLU_D;
				B_LU.Mtype = SLU_GE;
				B_LU.nrow = m;
				B_LU.ncol = 1;
			}
			else if (matrix_solver_method == MM_EXTERN)	//External routine
			{
				//Run allocation routine
				((void (*)(void *,unsigned int, unsigned int, bool))(LUSolverFcns.ext_alloc))(ext_solver_glob_vars,n,n,NR_admit_change);
			}
			else
			{
				GL_THROW("Invalid matrix solution method specified for NR solver!");
				//Defined elsewhere
			}

			//Update tracking variable
			powerflow_values->prev_m = m;
		}
		else if (powerflow_values->NR_realloc_needed)	//Something changed, we'll just destroy everything and start over
		{
			//Get rid of all of them first
			gl_free(matrices_LU.a_LU);
			gl_free(matrices_LU.rows_LU);
			gl_free(matrices_LU.cols_LU);
			gl_free(matrices_LU.rhs_LU);

			if (matrix_solver_method==MM_SUPERLU)
			{
				//Free up superLU matrices
				gl_free(perm_r);
				gl_free(perm_c);
			}
			//Default else - don't care - destructions are presumed to be handled inside external LU's alloc function

			/* Set aside space for the arrays. - Copied from above */
			matrices_LU.a_LU = (double *) gl_malloc(nnz *sizeof(double));
			if (matrices_LU.a_LU==NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");
			
			matrices_LU.rows_LU = (int *) gl_malloc(nnz *sizeof(int));
			if (matrices_LU.rows_LU == NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			matrices_LU.cols_LU = (int *) gl_malloc((n+1) *sizeof(int));
			if (matrices_LU.cols_LU == NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			/* Create the right-hand side matrix B. */
			matrices_LU.rhs_LU = (double *) gl_malloc(m *sizeof(double));
			if (matrices_LU.rhs_LU == NULL)
				GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

			if (matrix_solver_method==MM_SUPERLU)
			{
				///* Set up the arrays for the permutations. */
				perm_r = (int *) gl_malloc(m *sizeof(int));
				if (perm_r == NULL)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				perm_c = (int *) gl_malloc(n *sizeof(int));
				if (perm_c == NULL)
					GL_THROW("NR: One of the SuperLU solver matrices failed to allocate");

				//Update structures - A_LU matrix
				A_LU.Stype = SLU_NC;
				A_LU.Dtype = SLU_D;
				A_LU.Mtype = SLU_GE;
				A_LU.nrow = n;
				A_LU.ncol = m;

				//Update structures - B_LU matrix
				B_LU.Stype = SLU_DN;
				B_LU.Dtype = SLU_D;
				B_LU.Mtype = SLU_GE;
				B_LU.nrow = m;
				B_LU.ncol = 1;
			}
			else if (matrix_solver_method == MM_EXTERN)	//External routine
			{
				//Run allocation routine
				((void (*)(void *,unsigned int, unsigned int, bool))(LUSolverFcns.ext_alloc))(ext_solver_glob_vars,n,n,NR_admit_change);
			}
			else
			{
				GL_THROW("Invalid matrix solution method specified for NR solver!");
				//Defined elsewhere
			}

			//Update tracking variable
			powerflow_values->prev_m = m;
		}
		else if (powerflow_values->prev_m != m)	//Non-reallocing size change occurred
		{
			if (matrix_solver_method==MM_SUPERLU)
			{
				//Update relevant portions
				A_LU.nrow = n;
				A_LU.ncol = m;

				B_LU.nrow = m;
			}
			else if (matrix_solver_method == MM_EXTERN)	//External routine - call full reallocation, just in case
			{
				//Run allocation routine
				((void (*)(void *,unsigned int, unsigned int, bool))(LUSolverFcns.ext_alloc))(ext_solver_glob_vars,n,n,NR_admit_change);
			}
			else
			{
				GL_THROW("Invalid matrix solution method specified for NR solver!");
				//Defined elsewhere
			}

			//Update tracking variable
			powerflow_values->prev_m = m;
		}

#ifndef MT
		if (matrix_solver_method==MM_SUPERLU)
		{
			/* superLU sequential options*/
			set_default_options ( &options );
		}
		//Default else - not superLU
#endif
		
		for (indexer=0; indexer<size_Amatrix; indexer++)
		{
			matrices_LU.rows_LU[indexer] = powerflow_values->Y_Amatrix[indexer].row_ind ; // row pointers of non zero values
			matrices_LU.a_LU[indexer] = powerflow_values->Y_Amatrix[indexer].Y_value;
		}

		matrices_LU.cols_LU[0] = 0;
		indexer = 0;
		temp_index_c = 0;
		
		for ( jindexer = 0; jindexer< (size_Amatrix-1); jindexer++)
		{ 
			indexer += 1;
			tempa = powerflow_values->Y_Amatrix[jindexer].col_ind;
			tempb = powerflow_values->Y_Amatrix[jindexer+1].col_ind;
			if (tempb > tempa)
			{
				temp_index_c += 1;
				matrices_LU.cols_LU[temp_index_c] = indexer;
			}
		}

		matrices_LU.cols_LU[n] = nnz ;// number of non-zeros;

		for (temp_index_c=0;temp_index_c<m;temp_index_c++)
		{ 
			matrices_LU.rhs_LU[temp_index_c] = powerflow_values->deltaI_NR[temp_index_c];
		}

		if (matrix_solver_method==MM_SUPERLU)
		{
			////* Create Matrix A in the format expected by Super LU.*/
			//Populate the matrix values (temporary value)
			Astore = (NCformat*)A_LU.Store;
			Astore->nnz = nnz;
			Astore->nzval = matrices_LU.a_LU;
			Astore->rowind = matrices_LU.rows_LU;
			Astore->colptr = matrices_LU.cols_LU;
		    
			// Create right-hand side matrix B in format expected by Super LU
			//Populate the matrix (temporary values)
			Bstore = (DNformat*)B_LU.Store;
			Bstore->lda = m;
			Bstore->nzval = matrices_LU.rhs_LU;

#ifdef MT
			//superLU_MT commands

			//Populate perm_c
			get_perm_c(1, &A_LU, perm_c);

			//Solve the system
			pdgssv(NR_superLU_procs, &A_LU, perm_c, perm_r, &L_LU, &U_LU, &B_LU, &info);
#else
			//sequential superLU

			StatInit ( &stat );

			// solve the system
			dgssv(&options, &A_LU, perm_c, perm_r, &L_LU, &U_LU, &B_LU, &stat, &info);
#endif

			sol_LU = (double*) ((DNformat*) B_LU.Store)->nzval;
		}
		else if (matrix_solver_method==MM_EXTERN)
		{
			//Call the solver
			info = ((int (*)(void *,NR_SOLVER_VARS *, unsigned int, unsigned int))(LUSolverFcns.ext_solve))(ext_solver_glob_vars,&matrices_LU,n,1);

			//Point the solution to the proper place
			sol_LU = matrices_LU.rhs_LU;
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
		Maxmismatch = 0;

		temp_index = -1;
		temp_index_b = -1;
		newiter = false;	//Reset iteration requester flag - defaults to not needing another

		for (indexer=0; indexer<bus_count; indexer++)
		{
			//Avoid swing bus updates on normal runs
			if ((bus[indexer].type == 0) || ((bus[indexer].type == 2) && (swing_is_a_swing==false)))
			{
				//Figure out the offset we need to be for each phase
				if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase
				{
					//Pull the two updates (assume split-phase is always 2)
					DVConvCheck[0]=complex(sol_LU[2*bus[indexer].Matrix_Loc],sol_LU[(2*bus[indexer].Matrix_Loc+2)]);
					DVConvCheck[1]=complex(sol_LU[(2*bus[indexer].Matrix_Loc+1)],sol_LU[(2*bus[indexer].Matrix_Loc+3)]);
					bus[indexer].V[0] += DVConvCheck[0];
					bus[indexer].V[1] += DVConvCheck[1];	//Negative due to convention
					
					//Pull off the magnitude (no sense calculating it twice)
					CurrConvVal=DVConvCheck[0].Mag();
					if (CurrConvVal > Maxmismatch)	//Update our convergence check if it is bigger
						Maxmismatch=CurrConvVal;

					if (CurrConvVal > bus[indexer].max_volt_error)	//Check for convergence
						newiter=true;								//Flag that a new iteration must occur

					CurrConvVal=DVConvCheck[1].Mag();
					if (CurrConvVal > Maxmismatch)	//Update our convergence check if it is bigger
						Maxmismatch=CurrConvVal;

					if (CurrConvVal > bus[indexer].max_volt_error)	//Check for convergence
						newiter=true;								//Flag that a new iteration must occur
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

						DVConvCheck[jindex]=complex(sol_LU[(2*bus[indexer].Matrix_Loc+temp_index)],sol_LU[(2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size+temp_index)]);
						bus[indexer].V[temp_index_b] += DVConvCheck[jindex];
						
						//Pull off the magnitude (no sense calculating it twice)
						CurrConvVal=DVConvCheck[jindex].Mag();
						if (CurrConvVal > bus[indexer].max_volt_error)	//Check for convergence
							newiter=true;								//Flag that a new iteration must occur

						if (CurrConvVal > Maxmismatch)	//See if the current differential is the largest found so far or not
							Maxmismatch = CurrConvVal;	//It is, store it

					}//End For loop for phase traversion
				}//End not split phase update
			}//end if not swing
			else	//So this must be the swing and in a valid interval
			{
				temp_index +=2*powerflow_values->BA_diag[indexer].size;	//Increment us for what this bus would have been had it not been a swing
			}
		}//End bus traversion

		//Further debug stuff
#ifdef NR_MATRIX_OUT
		
#if defined(NR_VOLTAGE_PORTION_OUT) || defined(NR_REFERENCES_OUT)
		//Open the text file again
		FILE *FPoutValue=fopen("matrixinfoout.txt","at");
#endif

#ifdef NR_VOLTAGE_PORTION_OUT

		//Print voltage information
		fprintf(FPoutValue,"\nVoltage Information - row,value\n");

		for (indexer=0; indexer<bus_count; indexer++)
		{
			//Figure out the offset we need to be for each phase
			if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase
			{
				fprintf(FPoutValue,"%d,%f\n",2*bus[indexer].Matrix_Loc,bus[indexer].V[0].Re());
				fprintf(FPoutValue,"%d,%f\n",(2*bus[indexer].Matrix_Loc+1),bus[indexer].V[1].Re());
				fprintf(FPoutValue,"%d,%f\n",(2*bus[indexer].Matrix_Loc+2),bus[indexer].V[0].Im());
				fprintf(FPoutValue,"%d,%f\n",(2*bus[indexer].Matrix_Loc+3),bus[indexer].V[1].Im());
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

					fprintf(FPoutValue,"%d,%f\n",(2*bus[indexer].Matrix_Loc+temp_index),bus[indexer].V[temp_index_b].Re());
					fprintf(FPoutValue,"%d,%f\n",(2*bus[indexer].Matrix_Loc+powerflow_values->BA_diag[indexer].size+temp_index),bus[indexer].V[temp_index_b].Im());

				}//End For loop for phase traversion
			}//End not split phase update
		}//End bus traversion
#endif

#ifdef NR_REFERENCES_OUT
		//Print the index information
		fprintf(FPoutValue,"\nMatrix Index information - start,stop,name\n");

		for (indexer=0; indexer<bus_count; indexer++)
		{
			jindexer = 2*bus[indexer].Matrix_Loc;
			kindexer = jindexer + 2*powerflow_values->BA_diag[indexer].size - 1;


			fprintf(FPoutValue,"%d,%d,%s\n",jindexer,kindexer,bus[indexer].name);
		}
#endif

#if defined(NR_VOLTAGE_PORTION_OUT) || defined(NR_REFERENCES_OUT)
		//Close the file, we're done with it
		fclose(FPoutValue);
#endif

#endif	//End NR_MATIRX_OUT

		//Additional checks for special modes - only needs to happen in first dynamics powerflow
		if ((powerflow_type == PF_DYNINIT) && (bus[0].full_Y != NULL))
		{
			//See what "mode" we're in
			if (!newiter)	//Overall powerflow has converged
			{
				if (!swing_converged)	//See if deltaI_sym is being met
				{
					//Give a verbose message, just cause
					gl_verbose("NR_Solver: swing bus failed balancing criteria - making it a PQ for future dynamics");

					//Nope, are we still in the right mode
					if (swing_is_a_swing)	//Still a swing bus
					{
						swing_is_a_swing=false;	//Now effectively a PQ bus
						newiter = true;			//Flag for a reiteration
					}
					//Default else - if we're out of symmetry bounds, but already hit close, good enough
				}
				//Defaulted else - we're converged on all fronts, huzzah!
			}
			//Default else - still need to iterate
		}//End special convergence criterion

		//Turn off reallocation flag no matter what
		powerflow_values->NR_realloc_needed = false;

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
			((void (*)(void *, bool))(LUSolverFcns.ext_destroy))(ext_solver_glob_vars,newiter);
		}
		else	//Not sure how we get here
		{
			GL_THROW("Invalid matrix solution method specified for NR solver!");
			//Defined above
		}

		//Break us out if we are done or are singular		
		if (( newiter == false ) || (info!=0))
		{
			if (newiter == false)
			{
				gl_verbose("Power flow calculation converges at Iteration %d \n",Iteration+1);
			}
			break;
		}
	}	//End iteration loop

	//Check to see how we are ending
	if ((Iteration==NR_iteration_limit) && (newiter==true)) //Reached the limit
	{
		gl_verbose("Max solver mismatch of failed solution %f\n",Maxmismatch);
		return -Iteration;
	}
	else if (info!=0)	//failure of computations (singular matrix, etc.)
	{
		//For superLU - 2 = singular matrix it appears - positive values = process errors (singular, etc), negative values = input argument/syntax error
		if (matrix_solver_method==MM_SUPERLU)
		{
			gl_verbose("superLU failed out with return value %d",info);
		}
		else if (matrix_solver_method==MM_EXTERN)
		{
			gl_verbose("External LU solver failed out with return value %d",info);
		}
		//Defaulted else - shouldn't exist (or make it this far), but if it does, we're failing anyways

		*bad_computations = true;	//Flag our output as bad
		return 0;					//Just return some arbitrary value
	}
	else	//Must have converged 
		return Iteration;
}
