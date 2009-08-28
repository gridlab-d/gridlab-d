/* $Id
 * Newton-Raphson solver
 */

#include "solver_nr.h"
#include <slu_ddefs.h>

/* access to module global variables */
#include "powerflow.h"

double *deltaI_NR;
double *deltaV_NR;
//double *deltaP;
//double *deltaQ;
double zero = 0;
complex tempPb;
double Maxmismatch;
int size_offdiag_PQ;
int size_diag_fixed;
//double eps = 0.01; //1.0e-4;
bool newiter;
Bus_admit *BA_diag; /// BA_diag store the diagonal elements of the bus admittance matrix, the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
Y_NR *Y_offdiag_PQ; //Y_offdiag_PQ store the row,column and value of off_diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
Y_NR *Y_diag_fixed; //Y_diag_fixed store the row,column and value of fixed diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
Y_NR *Y_diag_update;//Y_diag_update store the row,column and value of updated diagonal elements of 6n*6n Y_NR matrix at each iteration. No PV bus is included.
Y_NR *Y_Amatrix;//Y_Amatrix store all the elements of Amatrix in equation AX=B;
complex tempY[3][3];
double tempIcalcReal, tempIcalcImag;
double tempPbus; //tempPbus store the temporary value of active power load at each bus
double tempQbus; //tempQbus store the temporary value of reactive power load at each bus
unsigned int indexer, tempa, tempb, jindexer;
char jindex, kindex;

int cmp( const void *a , const void *b )
{
struct Y_NR *c = (Y_NR *)a;
struct Y_NR *d = (Y_NR *)b;
if(c->col_ind != d->col_ind) return c->col_ind - d->col_ind;
else return c->row_ind - d->row_ind;
}

/** Newton-Raphson solver
	Solves a power flow problem using the Newton-Raphson method
	
	@return n=0 on failure to complete a single iteration, 
	n>0 to indicate success after n interations, or 
	n<0 to indicate failure after n iterations
 **/
int64 solver_nr(unsigned int bus_count, BUSDATA *bus, unsigned int branch_count, BRANCHDATA *branch)
{
	//Internal iteration counter - just NR limits
	int64 Iteration;

	//Total number of phases to be calculating (size of matrices)
	unsigned int total_variables;

	//Phase collapser variable
	unsigned char phase_worka, phase_workb, phase_workc;

	//Miscellaneous index variable
	unsigned int temp_index, temp_index_b;

	//Miscellaneous flag variables
	bool Full_Mat_A, Full_Mat_B, proceed_flag;

	//Temporary size variable
	char temp_size, temp_size_b, temp_size_c;

	//Temporary admittance variables
	complex Temp_Ad_A[3][3];
	complex Temp_Ad_B[3][3];

	//Temporary load calculation variables
	complex undeltacurr[3], undeltaimped[3], undeltapower[3];
	complex delta_current[3], voltageDel[3];
	complex temp_current[3], temp_power[3], temp_store[3];

	//convergence limit
	double eps;

	//DV checking array
	complex DVConvCheck[3];
	double CurrConvVal;

	//Miscellaneous counter tracker
	unsigned int index_count = 0;

	//Calculate the convergence limit - base it off of the swing bus
	eps = default_maximum_voltage_error * bus[0].V[0].Mag();

	//Build the diagnoal elements of the bus admittance matrix	
	if (BA_diag == NULL)
	{
		BA_diag = (Bus_admit *)gl_malloc(bus_count *sizeof(Bus_admit));   //BA_diag store the location and value of diagonal elements of Bus Admittance matrix
	}
	
	for (indexer=0; indexer<bus_count; indexer++) // Construct the diagonal elements of Bus admittance matrix.
	{
		//Determine the size we need
		if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase
			BA_diag[indexer].size = 2;
		else if ((bus[indexer].phases & 0x08) == 0x08)	//Delta - assume it is fully three phase
			BA_diag[indexer].size = 3;
		else										//Other cases, figure out how big they are
		{
			phase_worka = 0;
			for (jindex=0; jindex<3; jindex++)		//Accumulate number of phases
			{
				phase_worka += ((bus[indexer].phases & (0x01 << jindex)) >> jindex);
			}
			BA_diag[indexer].size = phase_worka;
		}

		//Ensure the admittance matrix is zeroed
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{
			BA_diag[indexer].Y[jindex][kindex] = 0;
			tempY[jindex][kindex] = 0;
			}
		}

		//Now go through all of the branches to get the self admittance information (hinges on size)
		for (jindexer=0; jindexer<branch_count;jindexer++)
		{ 
			if ((branch[jindexer].from == indexer) || (branch[jindexer].to == indexer))	//Bus is the from or to side of things
			{
				if (BA_diag[indexer].size == 3)		//Full three phase
				{
					for (jindex=0; jindex<3; jindex++)	//Add in all three phase values
					{
						for (kindex=0; kindex<3; kindex++)
						{
							if (branch[jindexer].from == indexer)	//We're the from version
							{
								tempY[jindex][kindex] += branch[jindexer].YSfrom[jindex*3+kindex];
							}
							else									//Must be the to version
							{
								tempY[jindex][kindex] += branch[jindexer].YSto[jindex*3+kindex];
							}
						}
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
						case 0x01:	//Only C
							{
								if (branch[jindexer].from == indexer)	//From branch
								{
									tempY[0][0] += branch[jindexer].YSfrom[8];
								}
								else									//To branch
								{
									tempY[0][0] += branch[jindexer].YSto[8];
								}
								break;
							}
						case 0x02:	//Only B
							{
								if (branch[jindexer].from == indexer)	//From branch
								{
									tempY[0][0] += branch[jindexer].YSfrom[4];
								}
								else									//To branch
								{
									tempY[0][0] += branch[jindexer].YSto[4];
								}
								break;
							}
						case 0x03:	//B & C
							{
								if (branch[jindexer].from == indexer)	//From branch
								{
									tempY[0][0] += branch[jindexer].YSfrom[4];
									tempY[0][1] += branch[jindexer].YSfrom[5];
									tempY[1][0] += branch[jindexer].YSfrom[7];
									tempY[1][0] += branch[jindexer].YSfrom[8];
								}
								else									//To branch
								{
									tempY[0][0] += branch[jindexer].YSto[4];
									tempY[0][1] += branch[jindexer].YSto[5];
									tempY[1][0] += branch[jindexer].YSto[7];
									tempY[1][0] += branch[jindexer].YSto[8];
								}
								break;
							}
						case 0x04:	//Only A
							{
								if (branch[jindexer].from == indexer)	//From branch
								{
									tempY[0][0] += branch[jindexer].YSfrom[0];
								}
								else									//To branch
								{
									tempY[0][0] += branch[jindexer].YSto[0];
								}
								break;
							}
						case 0x05:	//A & C
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
								break;
							}
						case 0x06:	//A & B
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
		BA_diag[indexer].col_ind = BA_diag[indexer].row_ind = index_count;	// Store the row and column starting information (square matrices)
		bus[indexer].Matrix_Loc = index_count;								//Store our location so we know where we go
		index_count += BA_diag[indexer].size;								// Update the index for this matrix's size, so next one is in appropriate place


		//Store the admittance values into the BA_diag matrix structure
		for (jindex=0; jindex<BA_diag[indexer].size; jindex++)
		{
			for (kindex=0; kindex<BA_diag[indexer].size; kindex++)			//Store values - assume square matrix - don't bother parsing what doesn't exist.
			{
				BA_diag[indexer].Y[jindex][kindex] = tempY[jindex][kindex];// Store the self admittance terms.
			}
		}
	}//End diagonal construction

	//Store the size of the diagonal, since it represents how many variables we are solving (useful later)
	total_variables=index_count;

	/// Build the off_diagonal_PQ bus elements of 6n*6n Y_NR matrix.Equation (12). All the value in this part will not be updated at each iteration.
	//Constructed using sparse methodology, non-zero elements are the only thing considered (and non-PV)
	//No longer necessarily 6n*6n any more either,
	unsigned int size_offdiag_PQ = 0;
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
						size_offdiag_PQ += 1; 

					if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))  
						size_offdiag_PQ += 1; 

					if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
						size_offdiag_PQ += 1; 

					if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
						size_offdiag_PQ += 1; 
				}//end columns of split phase
			}//end rows of split phase
		}//end traversion of split-phase
		else											//Three phase or some variety
		{
			for (jindex=0; jindex<3; jindex++)			//rows
			{
				for (kindex=0; kindex<3; kindex++)		//columns
				{
					if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))
						size_offdiag_PQ += 1; 

					if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))  
						size_offdiag_PQ += 1; 

					if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
						size_offdiag_PQ += 1; 

					if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1)) 
						size_offdiag_PQ += 1; 
				}//end columns of 3 phase
			}//end rows of 3 phase
		}//end three phase
	}//end line traversion

	//Allocate the space - double the number found (each element goes in two places)
	if (Y_offdiag_PQ == NULL)
	{
		Y_offdiag_PQ = (Y_NR *)gl_malloc((size_offdiag_PQ*2) *sizeof(Y_NR));   //Y_offdiag_PQ store the row,column and value of off_diagonal elements of Bus Admittance matrix in which all the buses are not PV buses. 
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
				for (kindex=0; kindex<3; kindex++)	//Loop through columns of admittance matrices
				{
					//Indices counted out from Self admittance above.  needs doubling due to complex separation

					if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
					{
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
						indexer += 1;
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 3;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 3;
						Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
						indexer += 1;
					}

					if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
					{
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
						indexer += 1;
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 3;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 3;
						Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
						indexer += 1;
					}

					if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
					{
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 3;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
						indexer += 1;
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 3;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
						indexer += 1;	
					}

					if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
					{
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 3;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
						indexer += 1;
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 3;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
						indexer += 1;	
					}
				}//column end
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
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = -(branch[jindexer].Yfrom[jindex*3+kindex]).Im();
								indexer += 1;
							}

							if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
								indexer += 1;
							}

							if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
								indexer += 1;	
							}

							if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
								indexer += 1;	
							}
						}//end From end SPCT to
						else if ((bus[tempb].phases & 0x20) == 0x20)	//To end is a SPCT to
						{
							//Indices counted out from Self admittance above.  needs doubling due to complex separation

							if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
								indexer += 1;
							}

							if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Im());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = -(branch[jindexer].Yto[jindex*3+kindex]).Im();
								indexer += 1;
							}

							if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
								indexer += 1;	
							}

							if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = ((branch[jindexer].Yto[jindex*3+kindex]).Re());
								indexer += 1;	
							}
						}//end To end SPCT to
						else											//Plain old ugly line
						{
							//Indices counted out from Self admittance above.  needs doubling due to complex separation

							if (((branch[jindexer].Yfrom[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
								indexer += 1;
							}

							if (((branch[jindexer].Yto[jindex*3+kindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Im());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[jindex*3+kindex]).Im();
								indexer += 1;
							}

							if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
								indexer += 1;	
							}

							if (((branch[jindexer].Yto[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1 && bus[tempb].type != 1))	//To reals
							{
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex + 2;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
								indexer += 1;
								
								Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + jindex;
								Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + kindex + 2;
								Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[jindex*3+kindex]).Re());
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
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Im());
						indexer += 1;
						
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
						Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yfrom[jindex*3+kindex]).Im();
						indexer += 1;
					}

					if (((branch[jindexer].Yto[kindex*3+jindex]).Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
					{
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Im());
						indexer += 1;
						
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
						Y_offdiag_PQ[indexer].Y_value = (branch[jindexer].Yto[kindex*3+jindex]).Im();
						indexer += 1;
					}

					if (((branch[jindexer].Yfrom[jindex*3+kindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
					{
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
						indexer += 1;
						
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yfrom[jindex*3+kindex]).Re());
						indexer += 1;	
					}

					if (((branch[jindexer].Yto[kindex*3+jindex]).Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
					{
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex + 2;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Re());
						indexer += 1;
						
						Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + kindex;
						Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + temp_size;
						Y_offdiag_PQ[indexer].Y_value = -((branch[jindexer].Yto[kindex*3+jindex]).Re());
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
				GL_THROW("NR: Failure to construct single/double phase line indices");
				/*  TROUBLESHOOT
				A single or double phase line (e.g., just A or AB) has failed to properly initialize all of the indices
				necessary to form the admittance matrix.  Please submit a bug report, with your code, to the trac site.
				*/

			if (Full_Mat_A)	//From side is a full ABC and we have AC
			{
				for (jindex=0; jindex<temp_size_c; jindex++)		//Loop through rows of admittance matrices				
				{
					for (kindex=0; kindex<temp_size_c; kindex++)	//Loop through columns of admittance matrices
					{
						//Indices counted out from Self admittance above.  needs doubling due to complex separation
						if ((Temp_Ad_A[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2 + temp_size;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
							Y_offdiag_PQ[indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
							indexer += 1;
						}

						if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2 + temp_size;
							Y_offdiag_PQ[indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
							indexer += 1;
						}

						if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2 + temp_size;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex*2;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
							indexer += 1;	
						}

						if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex*2 + temp_size;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
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
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2 + temp_size_b;
							Y_offdiag_PQ[indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
							indexer += 1;
						}

						if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2 + temp_size_b;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
							Y_offdiag_PQ[indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
							indexer += 1;
						}

						if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex*2 + temp_size_b;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
							indexer += 1;	
						}

						if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2 + temp_size_b;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex*2;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
							indexer += 1;	
						}
					}//column end
				}//row end
			}//end full ABD for to AC

			if ((!Full_Mat_A) && (!Full_Mat_B))	//Neither is a full ABC, or we aren't doing AC, so we don't care
			{
				for (jindex=0; jindex<temp_size_c; jindex++)		//Loop through rows of admittance matrices				
				{
					for (kindex=0; kindex<temp_size_c; kindex++)	//Loop through columns of admittance matrices
					{

						//Indices counted out from Self admittance above.  needs doubling due to complex separation
						if ((Temp_Ad_A[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From imags
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Im());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
							Y_offdiag_PQ[indexer].Y_value = (Temp_Ad_A[jindex][kindex].Im());
							indexer += 1;
						}

						if ((Temp_Ad_B[jindex][kindex].Im() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To imags
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Im());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
							Y_offdiag_PQ[indexer].Y_value = Temp_Ad_B[jindex][kindex].Im();
							indexer += 1;
						}

						if ((Temp_Ad_A[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//From reals
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex + temp_size;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempa].Matrix_Loc + temp_index + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + kindex + temp_size_b;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_A[jindex][kindex].Re());
							indexer += 1;	
						}

						if ((Temp_Ad_B[jindex][kindex].Re() != 0) && (bus[tempa].type != 1) && (bus[tempb].type != 1))	//To reals
						{
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex + temp_size_b;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
							indexer += 1;
							
							Y_offdiag_PQ[indexer].row_ind = 2*bus[tempb].Matrix_Loc + temp_index_b + jindex;
							Y_offdiag_PQ[indexer].col_ind = 2*bus[tempa].Matrix_Loc + temp_index + kindex + temp_size;
							Y_offdiag_PQ[indexer].Y_value = -(Temp_Ad_B[jindex][kindex].Re());
							indexer += 1;	
						}
					}//column end
				}//row end
			}//end not full ABC with AC on either side case
		}//end all others else
	}//end branch for

	//Build the fixed part of the diagonal PQ bus elements of 6n*6n Y_NR matrix. This part will not be updated at each iteration. 
	unsigned int size_diag_fixed = 0;
	for (jindexer=0; jindexer<bus_count;jindexer++) 
	{
		for (jindex=0; jindex<3; jindex++)
				{
					for (kindex=0; kindex<3; kindex++)
					{		 
					  if ((BA_diag[jindexer].Y[jindex][kindex]).Re() != 0 && bus[jindexer].type != 1 && jindex!=kindex)  
					  size_diag_fixed += 1; 
					  if ((BA_diag[jindexer].Y[jindex][kindex]).Im() != 0 && bus[jindexer].type != 1 && jindex!=kindex) 
					  size_diag_fixed += 1; 
					  else {}
					 }
				}
	}
	if (Y_diag_fixed == NULL)
	{
		Y_diag_fixed = (Y_NR *)gl_malloc((size_diag_fixed*2) *sizeof(Y_NR));   //Y_diag_fixed store the row,column and value of the fixed part of the diagonal PQ bus elements of 6n*6n Y_NR matrix.
	}
	indexer = 0;
	for (jindexer=0; jindexer<bus_count;jindexer++)	//Parse through bus list
	{ 
		for (jindex=0; jindex<BA_diag[jindexer].size; jindex++)
		{
			for (kindex=0; kindex<BA_diag[jindexer].size; kindex++)
			{					
				if ((BA_diag[jindexer].Y[jindex][kindex]).Im() != 0 && bus[jindexer].type != 1 && jindex!=kindex)
				{
					Y_diag_fixed[indexer].row_ind = 2*BA_diag[jindexer].row_ind + jindex;
					Y_diag_fixed[indexer].col_ind = 2*BA_diag[jindexer].col_ind + kindex;
					Y_diag_fixed[indexer].Y_value = (BA_diag[jindexer].Y[jindex][kindex]).Im();
					indexer += 1;

					Y_diag_fixed[indexer].row_ind = 2*BA_diag[jindexer].row_ind + jindex +BA_diag[jindexer].size;
					Y_diag_fixed[indexer].col_ind = 2*BA_diag[jindexer].col_ind + kindex +BA_diag[jindexer].size;
					Y_diag_fixed[indexer].Y_value = -(BA_diag[jindexer].Y[jindex][kindex]).Im();
					indexer += 1;
				}

				if ((BA_diag[jindexer].Y[jindex][kindex]).Re() != 0 && bus[jindexer].type != 1 && jindex!=kindex)
				{
					Y_diag_fixed[indexer].row_ind = 2*BA_diag[jindexer].row_ind + jindex;
					Y_diag_fixed[indexer].col_ind = 2*BA_diag[jindexer].col_ind + kindex +BA_diag[jindexer].size;
					Y_diag_fixed[indexer].Y_value = (BA_diag[jindexer].Y[jindex][kindex]).Re();
					indexer += 1;
					
					Y_diag_fixed[indexer].row_ind = 2*BA_diag[jindexer].row_ind + jindex +BA_diag[jindexer].size;
					Y_diag_fixed[indexer].col_ind = 2*BA_diag[jindexer].col_ind + kindex;
					Y_diag_fixed[indexer].Y_value = (BA_diag[jindexer].Y[jindex][kindex]).Re();
					indexer += 1;
				}
			}
		}
	}//End bus parse for fixed diagonal

	//Calculate the system load - this is the specified power of the system
	for (Iteration=0; Iteration<NR_iteration_limit; Iteration++)
	{
		//System load at each bus is represented by second order polynomial equations
		for (indexer=0; indexer<bus_count; indexer++)
		{
			if ((bus[indexer].phases & 0x08) == 0x08)	//Delta connected node
			{
				//Delta voltages
				voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];
				voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];
				voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

				//Power - put into a current value (iterates less this way)
				delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].S[0]/voltageDel[0]);
				delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S[1]/voltageDel[1]);
				delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S[2]/voltageDel[2]);

				//Convert delta connected load to appropriate Wye - reuse temp variable
				undeltacurr[0] = voltageDel[0] * (bus[indexer].Y[0]);
				undeltacurr[1] = voltageDel[1] * (bus[indexer].Y[1]);
				undeltacurr[2] = voltageDel[2] * (bus[indexer].Y[2]);

				undeltaimped[0] = (bus[indexer].V[0] == 0) ? 0 : (undeltacurr[0] - undeltacurr[2]) / (bus[indexer].V[0]);
				undeltaimped[1] = (bus[indexer].V[1] == 0) ? 0 : (undeltacurr[1] - undeltacurr[0]) / (bus[indexer].V[1]);
				undeltaimped[2] = (bus[indexer].V[2] == 0) ? 0 : (undeltacurr[2] - undeltacurr[1]) / (bus[indexer].V[2]);

				//Convert delta-current into a phase current - reuse temp variable
				undeltacurr[0]=(bus[indexer].I[0]+delta_current[0])-(bus[indexer].I[2]+delta_current[2]);
				undeltacurr[1]=(bus[indexer].I[1]+delta_current[1])-(bus[indexer].I[0]+delta_current[0]);
				undeltacurr[2]=(bus[indexer].I[2]+delta_current[2])-(bus[indexer].I[1]+delta_current[1]);

				//Aggregate the different values into a complete power
				for (jindex=0; jindex<3; jindex++)
				{
					//Real power calculations
					tempPbus = (undeltacurr[jindex]).Re() * (bus[indexer].V[jindex]).Re() + (undeltacurr[jindex]).Im() * (bus[indexer].V[jindex]).Im();	// Real power portion of Constant current component multiply the magnitude of bus voltage
					tempPbus += (undeltaimped[jindex]).Re() * (bus[indexer].V[jindex]).Re() * (bus[indexer].V[jindex]).Re() + (undeltaimped[jindex]).Re() * (bus[indexer].V[jindex]).Im() * (bus[indexer].V[jindex]).Im();	// Real power portion of Constant impedance component multiply the square of the magnitude of bus voltage
					bus[indexer].PL[jindex] = tempPbus;	//Real power portion
					
					//Reactive load calculations
					tempQbus = (undeltacurr[jindex]).Re() * (bus[indexer].V[jindex]).Im() - (undeltacurr[jindex]).Im() * (bus[indexer].V[jindex]).Re();	// Reactive power portion of Constant current component multiply the magnitude of bus voltage
					tempQbus += -(undeltaimped[jindex]).Im() * (bus[indexer].V[jindex]).Im() * (bus[indexer].V[jindex]).Im() - (undeltaimped[jindex]).Im() * (bus[indexer].V[jindex]).Re() * (bus[indexer].V[jindex]).Re();	// Reactive power portion of Constant impedance component multiply the square of the magnitude of bus voltage				
					bus[indexer].QL[jindex] = tempQbus;	//Reactive power portion
				}
			}//end delta connected
			else if ((bus[indexer].phases & 0x80) == 0x80)	//Split-phase connected node
			{
				//Convert it all back to current (easiest to handle)
				//Get V12 first
				voltageDel[0] = bus[indexer].V[0] + bus[indexer].V[1];

				//Start with the currents (just put them in)
				temp_current[0] = bus[indexer].I[0];
				temp_current[1] = bus[indexer].I[1];
				temp_current[2] = bus[indexer].extra_var;	//Current12 is not part of the standard current array

				//Now add in power contributions
				temp_current[0] += bus[indexer].V[0] == 0.0 ? 0.0 : ~(bus[indexer].S[0]/bus[indexer].V[0]);
				temp_current[1] += bus[indexer].V[1] == 0.0 ? 0.0 : ~(bus[indexer].S[1]/bus[indexer].V[1]);
				temp_current[2] += voltageDel[0] == 0.0 ? 0.0 : ~(bus[indexer].S[2]/voltageDel[0]);

				//Last, but not least, admittance/impedance contributions
				temp_current[0] += bus[indexer].Y[0]*bus[indexer].V[0];
				temp_current[1] += bus[indexer].Y[1]*bus[indexer].V[1];
				temp_current[2] += bus[indexer].Y[2]*voltageDel[0];

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
				//For Wye-connected, only compute and store phases that exist (make top heavy)
				temp_index = -1;
				temp_index_b = -1;
				for (jindex=0; jindex<BA_diag[indexer].size; jindex++)
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
					tempPbus += (bus[indexer].I[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() + (bus[indexer].I[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im();	// Real power portion of Constant current component multiply the magnitude of bus voltage
					tempPbus += (bus[indexer].Y[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re() + (bus[indexer].Y[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im();	// Real power portion of Constant impedance component multiply the square of the magnitude of bus voltage
					bus[indexer].PL[temp_index] = tempPbus;	//Real power portion
					tempQbus = (bus[indexer].S[temp_index_b]).Im();									// Reactive power portion of constant power portion
					tempQbus += (bus[indexer].I[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Im() - (bus[indexer].I[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Re();	// Reactive power portion of Constant current component multiply the magnitude of bus voltage
					tempQbus += -(bus[indexer].Y[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Im() - (bus[indexer].Y[temp_index_b]).Im() * (bus[indexer].V[temp_index_b]).Re() * (bus[indexer].V[temp_index_b]).Re();	// Reactive power portion of Constant impedance component multiply the square of the magnitude of bus voltage				
					bus[indexer].QL[temp_index] = tempQbus;	//Reactive power portion  

				}//end phase traversion
			}//end wye-connected
		}//end bus traversion for power update
	
		// Calculate the mismatch of three phase current injection at each bus (deltaI), 
		//and store the deltaI in terms of real and reactive value in array deltaI_NR    
		if (deltaI_NR==NULL)
		{
			deltaI_NR = (double *)gl_malloc((2*total_variables) *sizeof(double));   // left_hand side of equation (11)
		}

		//Compute the calculated loads (not specified) at each bus
		for (indexer=0; indexer<bus_count; indexer++) //for specific bus k
		{
			for (jindex=0; jindex<BA_diag[indexer].size; jindex++) // rows - for specific phase that exists
			{
				tempIcalcReal = tempIcalcImag = 0;   
				
				if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase - triplex bus
				{
					//Two states of triplex bus - To node of SPCT transformer needs to be different
					//First different - Delta-I and diagonal contributions
					if ((bus[indexer].phases & 0x20) == 0x20)	//We're the To bus
					{
						//Pre-negated due to the nature of how it's calculated (V1 compared to I1)
						tempPbus =  bus[indexer].PL[jindex];	// @@@ PG and QG is assumed to be zero here @@@ - this may change later (PV busses)
						tempQbus =  bus[indexer].QL[jindex];	
					}
					else	//We're just a normal triplex bus
					{
						//This one isn't negated (normal operations)
						tempPbus =  -bus[indexer].PL[jindex];	// @@@ PG and QG is assumed to be zero here @@@ - this may change later (PV busses)
						tempQbus =  -bus[indexer].QL[jindex];	
					}//end normal triplex bus

					//Get diagonal contributions - only (& always) 2
					//Column 1
					tempIcalcReal += (BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Re() - (BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Im();// equation (7), the diag elements of bus admittance matrix 
					tempIcalcImag += (BA_diag[indexer].Y[jindex][0]).Re() * (bus[indexer].V[0]).Im() + (BA_diag[indexer].Y[jindex][0]).Im() * (bus[indexer].V[0]).Re();// equation (8), the diag elements of bus admittance matrix 

					//Column 2
					tempIcalcReal += (BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Re() - (BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Im();// equation (7), the diag elements of bus admittance matrix 
					tempIcalcImag += (BA_diag[indexer].Y[jindex][1]).Re() * (bus[indexer].V[1]).Im() + (BA_diag[indexer].Y[jindex][1]).Im() * (bus[indexer].V[1]).Re();// equation (8), the diag elements of bus admittance matrix 

					//Now off diagonals
					for (jindexer=0; jindexer<branch_count; jindexer++)
					{
						if (branch[jindexer].from == indexer)	//We're the from bus
						{
							if ((bus[indexer].phases & 0x20) == 0x20)	//SPCT from bus - needs different signage
							{
								//This situation can only be a normal line (triplex will never be the from for another type)
								//Again only, & always 2 columns (just do them explicitly)
								//Column 1
								tempIcalcReal += ((branch[jindexer].Yfrom[jindex*3])).Re() * (bus[branch[jindexer].to].V[0]).Re() - ((branch[jindexer].Yfrom[jindex*3])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								tempIcalcImag += ((branch[jindexer].Yfrom[jindex*3])).Re() * (bus[branch[jindexer].to].V[0]).Im() + ((branch[jindexer].Yfrom[jindex*3])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

								//Column2
								tempIcalcReal += ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								tempIcalcImag += ((branch[jindexer].Yfrom[jindex*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + ((branch[jindexer].Yfrom[jindex*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
							}//End SPCT To bus - from diagonal contributions
							else		//Normal line connection to normal triplex
							{
								//This situation can only be a normal line (triplex will never be the from for another type)
								//Again only, & always 2 columns (just do them explicitly)
								//Column 1
								tempIcalcReal += (-(branch[jindexer].Yfrom[jindex*3])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[jindex*3])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								tempIcalcImag += (-(branch[jindexer].Yfrom[jindex*3])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[jindex*3])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

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

								//Perform the update, it only happens for one column (nature of the transformer)
								tempIcalcReal += (-(branch[jindexer].Yto[jindex*3+temp_index])).Re() * (bus[branch[jindexer].from].V[temp_index]).Re() - (-(branch[jindexer].Yto[jindex*3+temp_index])).Im() * (bus[branch[jindexer].from].V[temp_index]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								tempIcalcImag += (-(branch[jindexer].Yto[jindex*3+temp_index])).Re() * (bus[branch[jindexer].from].V[temp_index]).Im() + (-(branch[jindexer].Yto[jindex*3+temp_index])).Im() * (bus[branch[jindexer].from].V[temp_index]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
							}//end transformer
							else									//Must be a normal line then
							{
								if ((bus[indexer].phases & 0x20) == 0x20)	//SPCT from bus - needs different signage
								{
									//This case should never really exist, but if someone reverses a secondary or is doing meshed secondaries, it might
									//Again only, & always 2 columns (just do them explicitly)
									//Column 1
									tempIcalcReal += ((branch[jindexer].Yto[jindex*3])).Re() * (bus[branch[jindexer].from].V[0]).Re() - ((branch[jindexer].Yto[jindex*3])).Im() * (bus[branch[jindexer].from].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += ((branch[jindexer].Yto[jindex*3])).Re() * (bus[branch[jindexer].from].V[0]).Im() + ((branch[jindexer].Yto[jindex*3])).Im() * (bus[branch[jindexer].from].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

									//Column2
									tempIcalcReal += ((branch[jindexer].Yto[jindex*3+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - ((branch[jindexer].Yto[jindex*3+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += ((branch[jindexer].Yto[jindex*3+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + ((branch[jindexer].Yto[jindex*3+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								}//End SPCT To bus - from diagonal contributions
								else		//Normal line connection to normal triplex
								{
									//Again only, & always 2 columns (just do them explicitly)
									//Column 1
									tempIcalcReal += (-(branch[jindexer].Yto[jindex*3])).Re() * (bus[branch[jindexer].from].V[0]).Re() - (-(branch[jindexer].Yto[jindex*3])).Im() * (bus[branch[jindexer].from].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += (-(branch[jindexer].Yto[jindex*3])).Re() * (bus[branch[jindexer].from].V[0]).Im() + (-(branch[jindexer].Yto[jindex*3])).Im() * (bus[branch[jindexer].from].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

									//Column2
									tempIcalcReal += (-(branch[jindexer].Yto[jindex*3+1])).Re() * (bus[branch[jindexer].from].V[1]).Re() - (-(branch[jindexer].Yto[jindex*3+1])).Im() * (bus[branch[jindexer].from].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += (-(branch[jindexer].Yto[jindex*3+1])).Re() * (bus[branch[jindexer].from].V[1]).Im() + (-(branch[jindexer].Yto[jindex*3+1])).Im() * (bus[branch[jindexer].from].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								}//End normal triplex connection
							}//end normal line
						}//end to bus
						else	//We're nothing
							;
					}//End branch traversion

					//It's I.  Just a direct conversion (P changed above to I as well)
					deltaI_NR[2*bus[indexer].Matrix_Loc+ BA_diag[indexer].size + jindex] = tempPbus - tempIcalcReal;
					deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = tempQbus - tempIcalcImag;

				}//End split-phase present
				else	//Three phase or some variant thereof
				{
					tempPbus =  - bus[indexer].PL[jindex];	// @@@ PG and QG is assumed to be zero here @@@ - this may change later (PV busses)
					tempQbus =  - bus[indexer].QL[jindex];	

					for (kindex=0; kindex<BA_diag[indexer].size; kindex++)		//cols - Still only for specified phases
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
						tempIcalcReal += (BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Re() - (BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Im();// equation (7), the diag elements of bus admittance matrix 
						tempIcalcImag += (BA_diag[indexer].Y[jindex][kindex]).Re() * (bus[indexer].V[temp_index]).Im() + (BA_diag[indexer].Y[jindex][kindex]).Im() * (bus[indexer].V[temp_index]).Re();// equation (8), the diag elements of bus admittance matrix 

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

						for (jindexer=0; jindexer<branch_count; jindexer++)	//Parse through the branch list
						{
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
										//Do columns individually
										//1
										tempIcalcReal += (-(branch[jindexer].Yfrom[temp_index_b*3])).Re() * (bus[branch[jindexer].to].V[0]).Re() - (-(branch[jindexer].Yfrom[temp_index_b*3])).Im() * (bus[branch[jindexer].to].V[0]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += (-(branch[jindexer].Yfrom[temp_index_b*3])).Re() * (bus[branch[jindexer].to].V[0]).Im() + (-(branch[jindexer].Yfrom[temp_index_b*3])).Im() * (bus[branch[jindexer].to].V[0]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance

										//2
										tempIcalcReal += (-(branch[jindexer].Yfrom[temp_index_b*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Re() - (-(branch[jindexer].Yfrom[temp_index_b*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
										tempIcalcImag += (-(branch[jindexer].Yfrom[temp_index_b*3+1])).Re() * (bus[branch[jindexer].to].V[1]).Im() + (-(branch[jindexer].Yfrom[temp_index_b*3+1])).Im() * (bus[branch[jindexer].to].V[1]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									}
								}//end SPCT transformer
								else	///Must be a standard line
								{
									tempIcalcReal += (-(branch[jindexer].Yfrom[temp_index_b*3+temp_index])).Re() * (bus[branch[jindexer].to].V[temp_index]).Re() - (-(branch[jindexer].Yfrom[temp_index_b*3+temp_index])).Im() * (bus[branch[jindexer].to].V[temp_index]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
									tempIcalcImag += (-(branch[jindexer].Yfrom[temp_index_b*3+temp_index])).Re() * (bus[branch[jindexer].to].V[temp_index]).Im() + (-(branch[jindexer].Yfrom[temp_index_b*3+temp_index])).Im() * (bus[branch[jindexer].to].V[temp_index]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
								}//end standard line
							}	
							if  (branch[jindexer].to == indexer)
							{
								tempIcalcReal += (-(branch[jindexer].Yto[temp_index_b*3+temp_index])).Re() * (bus[branch[jindexer].from].V[temp_index]).Re() - (-(branch[jindexer].Yto[temp_index_b*3+temp_index])).Im() * (bus[branch[jindexer].from].V[temp_index]).Im() ;//equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance.
								tempIcalcImag += (-(branch[jindexer].Yto[temp_index_b*3+temp_index])).Re() * (bus[branch[jindexer].from].V[temp_index]).Im() + (-(branch[jindexer].Yto[temp_index_b*3+temp_index])).Im() * (bus[branch[jindexer].from].V[temp_index]).Re() ;//equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance.
							}
							else;
						}
					}//end intermediate current for each phase column

					if ((bus[indexer].V[temp_index]).Mag()!=0)
					{
						deltaI_NR[2*bus[indexer].Matrix_Loc+ BA_diag[indexer].size + jindex] = (tempPbus * (bus[indexer].V[temp_index_b]).Re() + tempQbus * (bus[indexer].V[temp_index_b]).Im())/ ((bus[indexer].V[temp_index_b]).Mag()*(bus[indexer].V[temp_index_b]).Mag()) - tempIcalcReal ; // equation(7), Real part of deltaI, left hand side of equation (11)
						deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = (tempPbus * (bus[indexer].V[temp_index_b]).Im() - tempQbus * (bus[indexer].V[temp_index_b]).Re())/ ((bus[indexer].V[temp_index_b]).Mag()*(bus[indexer].V[temp_index_b]).Mag()) - tempIcalcImag; // Imaginary part of deltaI, left hand side of equation (11)
					}
					else
					{
           				deltaI_NR[2*bus[indexer].Matrix_Loc+BA_diag[indexer].size + jindex] = 0.0;
						deltaI_NR[2*bus[indexer].Matrix_Loc + jindex] = 0.0;
					}
				}//End three-phase or variant thereof
			}//End delta_I for each phase row
		}//End delta_I for each bus

		// Calculate the elements of a,b,c,d in equations(14),(15),(16),(17). These elements are used to update the Jacobian matrix.	
		for (indexer=0; indexer<bus_count; indexer++)
		{
			if ((bus[indexer].phases & 0x08) == 0x08)	//Delta connected node
			{
				//Delta voltages
				voltageDel[0] = bus[indexer].V[0] - bus[indexer].V[1];
				voltageDel[1] = bus[indexer].V[1] - bus[indexer].V[2];
				voltageDel[2] = bus[indexer].V[2] - bus[indexer].V[0];

				//Power - convert to a current (uses less iterations this way)
				delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(bus[indexer].S[0]/voltageDel[0]);
				delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(bus[indexer].S[1]/voltageDel[1]);
				delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(bus[indexer].S[2]/voltageDel[2]);

				//Zero the powers - to be removed later (embedded in below equations)
				undeltapower[0] = undeltapower[1] = undeltapower[2] = 0.0;

				//Convert delta connected load to appropriate Wye - reuse temp variable
				undeltacurr[0] = voltageDel[0] * (bus[indexer].Y[0]);
				undeltacurr[1] = voltageDel[1] * (bus[indexer].Y[1]);
				undeltacurr[2] = voltageDel[2] * (bus[indexer].Y[2]);

				undeltaimped[0] = (bus[indexer].V[0] == 0) ? 0 : (undeltacurr[0] - undeltacurr[2]) / (bus[indexer].V[0]);
				undeltaimped[1] = (bus[indexer].V[1] == 0) ? 0 : (undeltacurr[1] - undeltacurr[0]) / (bus[indexer].V[1]);
				undeltaimped[2] = (bus[indexer].V[2] == 0) ? 0 : (undeltacurr[2] - undeltacurr[1]) / (bus[indexer].V[2]);

				//Convert delta-current into a phase current - reuse temp variable
				undeltacurr[0]=(bus[indexer].I[0]+delta_current[0])-(bus[indexer].I[2]+delta_current[2]);
				undeltacurr[1]=(bus[indexer].I[1]+delta_current[1])-(bus[indexer].I[0]+delta_current[0]);
				undeltacurr[2]=(bus[indexer].I[2]+delta_current[2])-(bus[indexer].I[1]+delta_current[1]);

				for (jindex=0; jindex<3; jindex++)	//All three are always assumed to exist for Delta (otherwise things get wierd)
				{
					if ((bus[indexer].V[jindex]).Mag()!=0)	//Make sure we aren't creating any indeterminants
					{
						bus[indexer].Jacob_A[jindex] = ((undeltapower[jindex]).Im() * (pow((bus[indexer].V[jindex]).Re(),2) - pow((bus[indexer].V[jindex]).Im(),2)) - 2*(bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Re())/pow((bus[indexer].V[jindex]).Mag(),4);// first part of equation(37)
						bus[indexer].Jacob_A[jindex] += ((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Re() + (undeltacurr[jindex]).Im() *pow((bus[indexer].V[jindex]).Im(),2))/pow((bus[indexer].V[jindex]).Mag(),3) + (undeltaimped[jindex]).Im();// second part of equation(37)
						bus[indexer].Jacob_B[jindex] = ((undeltapower[jindex]).Re() * (pow((bus[indexer].V[jindex]).Re(),2) - pow((bus[indexer].V[jindex]).Im(),2)) + 2*(bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Im())/pow((bus[indexer].V[jindex]).Mag(),4);// first part of equation(38)
						bus[indexer].Jacob_B[jindex] += -((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Im() + (undeltacurr[jindex]).Re() *pow((bus[indexer].V[jindex]).Re(),2))/pow((bus[indexer].V[jindex]).Mag(),3) - (undeltaimped[jindex]).Re();// second part of equation(38)
						bus[indexer].Jacob_C[jindex] = ((undeltapower[jindex]).Re() * (pow((bus[indexer].V[jindex]).Im(),2) - pow((bus[indexer].V[jindex]).Re(),2)) - 2*(bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Im())/pow((bus[indexer].V[jindex]).Mag(),4);// first part of equation(39)
						bus[indexer].Jacob_C[jindex] +=((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Im() - (undeltacurr[jindex]).Re() *pow((bus[indexer].V[jindex]).Im(),2))/pow((bus[indexer].V[jindex]).Mag(),3) - (undeltaimped[jindex]).Re();// second part of equation(39)
						bus[indexer].Jacob_D[jindex] = ((undeltapower[jindex]).Im() * (pow((bus[indexer].V[jindex]).Re(),2) - pow((bus[indexer].V[jindex]).Im(),2)) - 2*(bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Re())/pow((bus[indexer].V[jindex]).Mag(),4);// first part of equation(40)
						bus[indexer].Jacob_D[jindex] += ((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Re() - (undeltacurr[jindex]).Im() *pow((bus[indexer].V[jindex]).Im(),2))/pow((bus[indexer].V[jindex]).Mag(),3) - (undeltaimped[jindex]).Im();// second part of equation(40)
					}
					else	//Zero voltage = only impedance is valid (others get divided by VMag, so are IND)
					{
						bus[indexer].Jacob_A[jindex] = (undeltaimped[jindex]).Im();
						bus[indexer].Jacob_B[jindex] = -(undeltaimped[jindex]).Re();
						bus[indexer].Jacob_C[jindex] = -(undeltaimped[jindex]).Re();
						bus[indexer].Jacob_D[jindex] = -(undeltaimped[jindex]).Im();
					}
				}//End delta-three phase traverse
			}//end delta-connected load
			else if	((bus[indexer].phases & 0x80) == 0x80)	//Split phase computations
			{
				//Convert it all back to current (easiest to handle)
				//Get V12 first
				voltageDel[0] = bus[indexer].V[0] + bus[indexer].V[1];

				//Start with the currents (just put them in)
				temp_current[0] = bus[indexer].I[0];
				temp_current[1] = bus[indexer].I[1];
				temp_current[2] = bus[indexer].extra_var; //current12 is not part of the standard current array

				//Now add in power contributions
				temp_current[0] += bus[indexer].V[0] == 0.0 ? 0.0 : ~(bus[indexer].S[0]/bus[indexer].V[0]);
				temp_current[1] += bus[indexer].V[1] == 0.0 ? 0.0 : ~(bus[indexer].S[1]/bus[indexer].V[1]);
				temp_current[2] += voltageDel[0] == 0.0 ? 0.0 : ~(bus[indexer].S[2]/voltageDel[0]);

				//Last, but not least, admittance/impedance contributions
				temp_current[0] += bus[indexer].Y[0]*bus[indexer].V[0];
				temp_current[1] += bus[indexer].Y[1]*bus[indexer].V[1];
				temp_current[2] += bus[indexer].Y[2]*voltageDel[0];

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
						bus[indexer].Jacob_D[jindex] = ((bus[indexer].V[jindex]).Re()*(bus[indexer].V[jindex]).Im()*(temp_store[jindex]).Re() - (temp_store[jindex]).Im() *pow((bus[indexer].V[jindex]).Im(),2))/pow((bus[indexer].V[jindex]).Mag(),3);// second part of equation(40)
					}
					else
					{
						bus[indexer].Jacob_A[jindex]= 0.0;
						bus[indexer].Jacob_B[jindex]= 0.0;
						bus[indexer].Jacob_C[jindex]= 0.0;
						bus[indexer].Jacob_D[jindex]= 0.0;
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
				//For Wye-connected, only compute and store phases that exist (make top heavy)
				temp_index = -1;
				temp_index_b = -1;
				for (jindex=0; jindex<BA_diag[indexer].size; jindex++)
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

					if ((bus[indexer].V[jindex]).Mag()!=0)
					{
						bus[indexer].Jacob_A[temp_index] = ((bus[indexer].S[temp_index_b]).Im() * (pow((bus[indexer].V[temp_index_b]).Re(),2) - pow((bus[indexer].V[temp_index_b]).Im(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Re())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(37)
						bus[indexer].Jacob_A[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].I[temp_index_b]).Re() + (bus[indexer].I[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) + (bus[indexer].Y[temp_index_b]).Im();// second part of equation(37)
						bus[indexer].Jacob_B[temp_index] = ((bus[indexer].S[temp_index_b]).Re() * (pow((bus[indexer].V[temp_index_b]).Re(),2) - pow((bus[indexer].V[temp_index_b]).Im(),2)) + 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Im())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(38)
						bus[indexer].Jacob_B[temp_index] += -((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].I[temp_index_b]).Im() + (bus[indexer].I[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Re(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Re();// second part of equation(38)
						bus[indexer].Jacob_C[temp_index] = ((bus[indexer].S[temp_index_b]).Re() * (pow((bus[indexer].V[temp_index_b]).Im(),2) - pow((bus[indexer].V[temp_index_b]).Re(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Im())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(39)
						bus[indexer].Jacob_C[temp_index] +=((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].I[temp_index_b]).Im() - (bus[indexer].I[temp_index_b]).Re() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Re();// second part of equation(39)
						bus[indexer].Jacob_D[temp_index] = ((bus[indexer].S[temp_index_b]).Im() * (pow((bus[indexer].V[temp_index_b]).Re(),2) - pow((bus[indexer].V[temp_index_b]).Im(),2)) - 2*(bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].S[temp_index_b]).Re())/pow((bus[indexer].V[temp_index_b]).Mag(),4);// first part of equation(40)
						bus[indexer].Jacob_D[temp_index] += ((bus[indexer].V[temp_index_b]).Re()*(bus[indexer].V[temp_index_b]).Im()*(bus[indexer].I[temp_index_b]).Re() - (bus[indexer].I[temp_index_b]).Im() *pow((bus[indexer].V[temp_index_b]).Im(),2))/pow((bus[indexer].V[temp_index_b]).Mag(),3) - (bus[indexer].Y[temp_index_b]).Im();// second part of equation(40)
					}
					else
					{
						bus[indexer].Jacob_A[temp_index]= (bus[indexer].Y[temp_index_b]).Im();
						bus[indexer].Jacob_B[temp_index]= -(bus[indexer].Y[temp_index_b]).Re();
						bus[indexer].Jacob_C[temp_index]= -(bus[indexer].Y[temp_index_b]).Re();
						bus[indexer].Jacob_D[temp_index]= -(bus[indexer].Y[temp_index_b]).Im();
					}
				}//End phase traversion - Wye
			}//End wye-connected load
		}//end bus traversion for a,b,c, d value computation

		//Build the dynamic diagnal elements of 6n*6n Y matrix. All the elements in this part will be updated at each iteration.
		unsigned int size_diag_update = 0;
		for (jindexer=0; jindexer<bus_count;jindexer++) 
		{
			if  (bus[jindexer].type != 1)	//PV bus ignored (for now?)
				size_diag_update += BA_diag[jindexer].size; 
			else {}
		}
		
		if (Y_diag_update == NULL)
		{
			Y_diag_update = (Y_NR *)gl_malloc((4*size_diag_update) *sizeof(Y_NR));   //Y_diag_update store the row,column and value of the dynamic part of the diagonal PQ bus elements of 6n*6n Y_NR matrix.
		}
		
		indexer = 0;	//Rest positional counter

		for (jindexer=0; jindexer<bus_count; jindexer++)	//Parse through bus list
		{
			if (bus[jindexer].type == 2)	//Swing bus
			{
				for (jindex=0; jindex<BA_diag[jindexer].size; jindex++)
				{
					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;

					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind + BA_diag[jindexer].size;
					Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;

					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + BA_diag[jindexer].size;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind - BA_diag[jindexer].size;
					Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;

					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + BA_diag[jindexer].size;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = -1e10; // swing bus gets large admittance
					indexer += 1;
				}//End swing bus traversion
			}//End swing bus

			if (bus[jindexer].type != 1 && bus[jindexer].type != 2)	//No PV or swing (so must be PQ)
			{
				for (jindex=0; jindex<BA_diag[jindexer].size; jindex++)
				{
					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = (BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_A[jindex]; // Equation(14)
					indexer += 1;
					
					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind + BA_diag[jindexer].size;
					Y_diag_update[indexer].Y_value = (BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_B[jindex]; // Equation(15)
					indexer += 1;
					
					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + BA_diag[jindexer].size;
					Y_diag_update[indexer].col_ind = 2*bus[jindexer].Matrix_Loc + jindex;
					Y_diag_update[indexer].Y_value = (BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_C[jindex]; // Equation(16)
					indexer += 1;
					
					Y_diag_update[indexer].row_ind = 2*bus[jindexer].Matrix_Loc + jindex + BA_diag[jindexer].size;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = -(BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_D[jindex]; // Equation(17)
					indexer += 1;
				}//end PQ phase traversion
			}//End PQ bus
		}//End bus parse list

		// Build the Amatrix, Amatrix includes all the elements of Y_offdiag_PQ, Y_diag_fixed and Y_diag_update.
		unsigned int size_Amatrix;
		size_Amatrix = size_offdiag_PQ*2 + size_diag_fixed*2 + 4*size_diag_update;
		if (Y_Amatrix == NULL)
		{
			Y_Amatrix = (Y_NR *)gl_malloc((size_Amatrix) *sizeof(Y_NR));   // Amatrix includes all the elements of Y_offdiag_PQ, Y_diag_fixed and Y_diag_update.
		}

		//integrate off diagonal components
		for (indexer=0; indexer<size_offdiag_PQ*2; indexer++)
		{
			Y_Amatrix[indexer].row_ind = Y_offdiag_PQ[indexer].row_ind;
			Y_Amatrix[indexer].col_ind = Y_offdiag_PQ[indexer].col_ind;
			Y_Amatrix[indexer].Y_value = Y_offdiag_PQ[indexer].Y_value;
		}

		//Integrate fixed portions of diagonal components
		for (indexer=size_offdiag_PQ*2; indexer< (size_offdiag_PQ*2 + size_diag_fixed*2); indexer++)
		{
			Y_Amatrix[indexer].row_ind = Y_diag_fixed[indexer - size_offdiag_PQ*2 ].row_ind;
			Y_Amatrix[indexer].col_ind = Y_diag_fixed[indexer - size_offdiag_PQ*2 ].col_ind;
			Y_Amatrix[indexer].Y_value = Y_diag_fixed[indexer - size_offdiag_PQ*2 ].Y_value;
		}

		//Integrate the variable portions of the diagonal components
		for (indexer=size_offdiag_PQ*2 + size_diag_fixed*2; indexer< size_Amatrix; indexer++)
		{
			Y_Amatrix[indexer].row_ind = Y_diag_update[indexer - size_offdiag_PQ*2 - size_diag_fixed*2].row_ind;
			Y_Amatrix[indexer].col_ind = Y_diag_update[indexer - size_offdiag_PQ*2 - size_diag_fixed*2].col_ind;
			Y_Amatrix[indexer].Y_value = Y_diag_update[indexer - size_offdiag_PQ*2 - size_diag_fixed*2].Y_value;
		}

		/* sorting integers using qsort() example */
		qsort(Y_Amatrix,size_Amatrix,sizeof(*Y_Amatrix),cmp);

		//Call SuperLU to solve the equation of AX=b;
		SuperMatrix A,B,L,U;
		double *a,*rhs;
		int *perm_c, *perm_r, *cols, *rows;
		int nnz, info;
		superlu_options_t options;
		SuperLUStat_t stat;
		NCformat *Astore;
		DNformat *Bstore;
		unsigned int m,n;
		double *sol;

		///* Initialize parameters. */
		m = 2*total_variables; n = 2*total_variables; nnz = size_Amatrix;

		/* Set aside space for the arrays. */
		a = (double *) gl_malloc(nnz *sizeof(double));
		rows = (int *) gl_malloc(nnz *sizeof(int));
		cols = (int *) gl_malloc((n+1) *sizeof(int));

		/* Create the right-hand side matrix B. */
		rhs = (double *) gl_malloc(m *sizeof(double));

		///* Set up the arrays for the permutations. */
		perm_r = (int *) gl_malloc(m *sizeof(int));
		perm_c = (int *) gl_malloc(n *sizeof(int));
		//
		///* Set the default input options, and then adjust some of them. */
		set_default_options ( &options );

		for (indexer=0; indexer<size_Amatrix; indexer++)
		{
			rows[indexer] = Y_Amatrix[indexer].row_ind ; // row pointers of non zero values
			a[indexer] = Y_Amatrix[indexer].Y_value;
		}
		cols[0] = 0;
		indexer = 0;
		temp_index = 0;
		for ( jindexer = 0; jindexer< (size_Amatrix-1); jindexer++)
		{ 
			indexer += 1;
			tempa = Y_Amatrix[jindexer].col_ind;
			tempb = Y_Amatrix[jindexer+1].col_ind;
			if (tempb > tempa)
			{
				temp_index += 1;
				cols[temp_index] = indexer;
			}
		}
		cols[n] = nnz ;// number of non-zeros;

		for (temp_index=0;temp_index<m;temp_index++)
		{ 
			rhs[temp_index] = deltaI_NR[temp_index];
		}

		//* Create Matrix A in the format expected by Super LU.*/
		dCreate_CompCol_Matrix ( &A, m, n, nnz, a, rows, cols, SLU_NC,SLU_D,SLU_GE );
		Astore =(NCformat*)A.Store;

		//* Create right-hand side matrix B in the format expected by Super LU.*/
		dCreate_Dense_Matrix(&B, m, 1, rhs, m, SLU_DN, SLU_D, SLU_GE);

		//Astore=(NCformat*)A.Store;
		Bstore=(DNformat*)B.Store;
		StatInit ( &stat );

		Astore->nzval=a;
		Bstore->nzval=rhs;

		// solve the system
		dgssv(&options, &A, perm_c, perm_r, &L, &U, &B, &stat, &info);
		sol = (double*) ((DNformat*) B.Store)->nzval;

		//Update bus voltages - check convergence while we're here
		Maxmismatch = 0;

		temp_index = -1;
		temp_index_b = -1;

		for (indexer=0; indexer<bus_count; indexer++)
		{
			//Avoid swing bus updates
			if (bus[indexer].type != 2)
			{
				//Figure out the offset we need to be for each phase
				if ((bus[indexer].phases & 0x80) == 0x80)	//Split phase
				{
					//Pull the two updates (assume split-phase is always 2)
					DVConvCheck[0]=complex(sol[2*bus[indexer].Matrix_Loc],sol[(2*bus[indexer].Matrix_Loc+2)]);
					DVConvCheck[1]=complex(sol[(2*bus[indexer].Matrix_Loc+1)],sol[(2*bus[indexer].Matrix_Loc+3)]);
					bus[indexer].V[0] += DVConvCheck[0];
					bus[indexer].V[1] += DVConvCheck[1];	//Negative due to convention
					
					//Pull off the magnitude (no sense calculating it twice)
					CurrConvVal=DVConvCheck[0].Mag();
					if (CurrConvVal > Maxmismatch)	//Update our convergence check if it is bigger
						Maxmismatch=CurrConvVal;

					CurrConvVal=DVConvCheck[1].Mag();
					if (CurrConvVal > Maxmismatch)	//Update our convergence check if it is bigger
						Maxmismatch=CurrConvVal;

				}//end split phase update
				else										//Not split phase
				{
					for (jindex=0; jindex<BA_diag[indexer].size; jindex++)	//parse through the phases
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

						DVConvCheck[jindex]=complex(sol[(2*bus[indexer].Matrix_Loc+temp_index)],sol[(2*bus[indexer].Matrix_Loc+BA_diag[indexer].size+temp_index)]);
						bus[indexer].V[temp_index_b] += DVConvCheck[jindex];
						
						//Pull off the magnitude (no sense calculating it twice)
						CurrConvVal=DVConvCheck[jindex].Mag();
						if (CurrConvVal > Maxmismatch)	//Update our convergence check if it is bigger
							Maxmismatch=CurrConvVal;

					}//End For loop for phase traversion
				}//End not split phase update
			}//end if not swing
			else	//So this must be the swing
			{
				temp_index +=2*BA_diag[indexer].size;	//Increment us for what this bus would have been had it not been a swing
			}
		}//End bus traversion

		//New convergence test - voltage check
		newiter=false;
		if ( Maxmismatch <= eps)
		{
			printf("Power flow calculation converges at Iteration %d \n",Iteration-1);
		}
		else if ( Maxmismatch > eps)
			newiter = true;
		
		//Break us out if we are done		
		if ( newiter == false )
			break;

		/* De-allocate storage. */
		gl_free(rhs);
		gl_free(a);
		gl_free(cols);
		gl_free(rows);
		gl_free(perm_r);
		gl_free(perm_c);
		Destroy_SuperNode_Matrix( &L );
		Destroy_CompCol_Matrix( &U );
		Destroy_SuperMatrix_Store( &A );
		Destroy_SuperMatrix_Store(&B);
		StatFree ( &stat );
	}	//End iteration loop

	gl_warning("Newton-Raphson solution method is not yet supported");

	//Check to see how we are ending
	if ((Iteration==NR_iteration_limit) && (newiter==true)) //Reached the limit
		return -Iteration;
	else	//Must have converged (failure to solve not handled yet)
		return Iteration;
}




