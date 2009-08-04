/* $Id
 * Newton-Raphson solver
 */

#include "solver_nr.h"
#include <slu_ddefs.h>

/* access to module global variables */
#include "powerflow.h"

double *deltaI_NR;
double *deltaV_NR;
double *deltaP;
double *deltaQ;
double zero = 0;
complex tempPb;
double Maxmismatch;
int size_offdiag_PQ;
int size_diag_fixed;
double eps = 0.01; //1.0e-4;
bool newiter;
Bus_admit *BA_diag; /// BA_diag store the diagonal elements of the bus admittance matrix, the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
Y_NR *Y_offdiag_PQ; //Y_offdiag_PQ store the row,column and value of off_diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
Y_NR *Y_diag_fixed; //Y_diag_fixed store the row,column and value of fixed diagonal elements of 6n*6n Y_NR matrix. No PV bus is included.
Y_NR *Y_diag_update;//Y_diag_update store the row,column and value of updated diagonal elements of 6n*6n Y_NR matrix at each iteration. No PV bus is included.
Y_NR *Y_Amatrix;//Y_Amatrix store all the elements of Amatrix in equation AX=B;
complex *Icalc;
complex tempY[3][3];
double tempIcalcReal, tempIcalcImag;
double tempPbus; //tempPbus store the temporary value of active power load at each bus
double tempQbus; //tempQbus store the temporary value of reactive power load at each bus
int indexer,jindex, kindex, jindexer, tempa, tempb;
extern unsigned global_iteration_limit;


SuperMatrix Test_Matrix;		//Simple initialization just to make sure SuperLU is linking - can be deleted
superlu_options_t test_options; //Simple initialization just to make sure SuperLU is linking - can be deleted


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
int solver_nr(int bus_count, BUSDATA *bus, int branch_count, BRANCHDATA *branch)
{
	//FILE *pFile = fopen("myfile.txt","w"); ////////////////////////to be delete

	//Pull off the iteration count (it's backwards and incorporates iteration_limit
	int64 Iteration = 0;
	//This may change
	char buffer[64];
	gl_global_getvar("iteration_limit",buffer,sizeof(buffer));

	indexer=0;
	while (buffer[indexer] != NULL)
	{
		Iteration *= 10;
		Iteration += (int)buffer[indexer]-48;
		indexer++;
	}

	//Build the diagnoal elements of the bus admittance matrix	
	if (BA_diag == NULL)
	{
		BA_diag = (Bus_admit *)gl_malloc(bus_count *sizeof(Bus_admit));   //BA_diag store the location and value of diagonal elements of Bus Admittance matrix
	}
	
	for (indexer=0; indexer<bus_count; indexer++) // Construct the diagonal elements of Bus admittance matrix.
	{
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{
			BA_diag[indexer].Y[jindex][kindex] = 0;
			tempY[jindex][kindex] = 0;
			}
		}

		for (jindexer=0; jindexer<branch_count;jindexer++)
		{ 
			if ( branch[jindexer].from == indexer)
			{ 
				for (jindex=0; jindex<3; jindex++)
				{
					for (kindex=0; kindex<3; kindex++)
					{
						tempY[jindex][kindex] += *branch[jindexer].Yfrom[jindex][kindex] / branch[jindexer].v_ratio;
					}
				}
			}
			else if ( branch[jindexer].to == indexer)
			{ 
				for (jindex=0; jindex<3; jindex++)
				{
					for (kindex=0; kindex<3; kindex++)
					{
						tempY[jindex][kindex] += *branch[jindexer].Yto[jindex][kindex] * branch[jindexer].v_ratio;
					}
				}
			}
			else {}
		}
		BA_diag[indexer].col_ind = BA_diag[indexer].row_ind = indexer; // BA_diag store the row and column information of n elements, n = bus_count
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{
				BA_diag[indexer].Y[jindex][kindex] = tempY[jindex][kindex];// BA_diag store the 3*3 complex admittance value of n elements, n = bus_count
			}
	
		}
	}

	/// Build the off_diagonal_PQ bus elements of 6n*6n Y_NR matrix.Equation (12). All the value in this part will not be updated at each iteration.
	int size_offdiag_PQ = 0;
	for (jindexer=0; jindexer<branch_count;jindexer++) 
	{
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{
				tempa  = branch[jindexer].from;
				tempb  = branch[jindexer].to;
				if ((*branch[jindexer].Yfrom[jindex][kindex]).Re() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1)  
					size_offdiag_PQ += 1; 

				if ((*branch[jindexer].Yto[jindex][kindex]).Re() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1)  
					size_offdiag_PQ += 1; 

				if ((*branch[jindexer].Yfrom[jindex][kindex]).Im() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1) 
					size_offdiag_PQ += 1; 

				if ((*branch[jindexer].Yto[jindex][kindex]).Im() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1) 
					size_offdiag_PQ += 1; 
			}
		}
	}

	if (Y_offdiag_PQ == NULL)
	{
		Y_offdiag_PQ = (Y_NR *)gl_malloc((size_offdiag_PQ*4) *sizeof(Y_NR));   //Y_offdiag_PQ store the row,column and value of off_diagonal elements of Bus Admittance matrix in which all the buses are not PV buses. 
	}


	indexer = 0;
	for (jindexer=0; jindexer<branch_count;jindexer++)
	{ 
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{					
				tempa  = branch[jindexer].from;
			    tempb  = branch[jindexer].to;
				if ((*branch[jindexer].Yfrom[jindex][kindex]).Im() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1)	//From imags
				{
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yfrom[jindex][kindex]).Im()); // Note that off diagonal elements of bus admittance matrix is equal to the negative value of corresponding branch admittance.
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex +3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = (*branch[jindexer].Yfrom[jindex][kindex]).Im();
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yto[jindex][kindex]).Im());
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex +3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = (*branch[jindexer].Yto[jindex][kindex]).Im();
					indexer += 1;
				}

				if ((*branch[jindexer].Yto[jindex][kindex]).Im() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1)	//To imags
				{
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yto[jindex][kindex]).Im()); // Note that off diagonal elements of bus admittance matrix is equal to the negative value of corresponding branch admittance.
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex +3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = (*branch[jindexer].Yto[jindex][kindex]).Im();
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yfrom[jindex][kindex]).Im());
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex +3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = (*branch[jindexer].Yfrom[jindex][kindex]).Im();
					indexer += 1;
				}

				if ((*branch[jindexer].Yfrom[jindex][kindex]).Re() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1)	//From reals
				{
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex + 3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yfrom[jindex][kindex]).Re());
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yfrom[jindex][kindex]).Re());
					indexer += 1;	
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex + 3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yto[jindex][kindex]).Re());
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yto[jindex][kindex]).Re());
					indexer += 1;
				}

				if ((*branch[jindexer].Yto[jindex][kindex]).Re() != 0 && bus[tempa].type != 1 && bus[tempb].type != 1)	//To reals
				{
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex + 3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yto[jindex][kindex]).Re());
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].to) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].from) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yto[jindex][kindex]).Re());
					indexer += 1;	
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex + 3;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yfrom[jindex][kindex]).Re());
					indexer += 1;
					Y_offdiag_PQ[indexer].row_ind = 6*(branch[jindexer].from) + jindex;
			        Y_offdiag_PQ[indexer].col_ind = 6*(branch[jindexer].to) + kindex +3;
					Y_offdiag_PQ[indexer].Y_value = -((*branch[jindexer].Yfrom[jindex][kindex]).Re());
					indexer += 1;
				}
			}
		}
	}

	//Build the fixed part of the diagonal PQ bus elements of 6n*6n Y_NR matrix. This part will not be updated at each iteration. 
	int size_diag_fixed = 0;
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
	for (jindexer=0; jindexer<bus_count;jindexer++)
	{ 
		for (jindex=0; jindex<3; jindex++)
		{
			for (kindex=0; kindex<3; kindex++)
			{					
				if ((BA_diag[jindexer].Y[jindex][kindex]).Im() != 0 && bus[jindexer].type != 1 && jindex!=kindex)
				{
					Y_diag_fixed[indexer].row_ind = 6*jindexer + jindex;
					Y_diag_fixed[indexer].col_ind = 6*jindexer + kindex;
					Y_diag_fixed[indexer].Y_value = (BA_diag[jindexer].Y[jindex][kindex]).Im();
					indexer += 1;
					Y_diag_fixed[indexer].row_ind = 6*jindexer + jindex +3;
					Y_diag_fixed[indexer].col_ind = 6*jindexer + kindex +3;
					Y_diag_fixed[indexer].Y_value = -(BA_diag[jindexer].Y[jindex][kindex]).Im();
					indexer += 1;
				}

				if ((BA_diag[jindexer].Y[jindex][kindex]).Re() != 0 && bus[jindexer].type != 1 && jindex!=kindex)
				{
					Y_diag_fixed[indexer].row_ind = 6*jindexer + jindex;
					Y_diag_fixed[indexer].col_ind = 6*jindexer + kindex +3;
					Y_diag_fixed[indexer].Y_value = (BA_diag[jindexer].Y[jindex][kindex]).Re();
					indexer += 1;
					Y_diag_fixed[indexer].row_ind = 6*jindexer + jindex +3;
					Y_diag_fixed[indexer].col_ind = 6*jindexer + kindex;
					Y_diag_fixed[indexer].Y_value = (BA_diag[jindexer].Y[jindex][kindex]).Re();
					indexer += 1;
				}
			}
		}
	}

	complex undeltacurr[3], undeltaimped[3], undeltapower[3];
	complex delta_current[3], voltageDel[3];

	for (; Iteration > 0; Iteration--)
	{
		//System load at each bus is represented by second order polynomial equations
		for (indexer=0; indexer<bus_count; indexer++)
		{
			if (bus[indexer].delta == true)	//Delta connected node
			{
				//Delta voltages
				voltageDel[0] = *bus[indexer].V[0] - *bus[indexer].V[1];
				voltageDel[1] = *bus[indexer].V[1] - *bus[indexer].V[2];
				voltageDel[2] = *bus[indexer].V[2] - *bus[indexer].V[0];

				//Power
				delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(*bus[indexer].S[0]/voltageDel[0]);
				delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(*bus[indexer].S[1]/voltageDel[1]);
				delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(*bus[indexer].S[2]/voltageDel[2]);

				//Use delta current variable at first
				undeltacurr[0] =delta_current[0]-delta_current[2];
				undeltacurr[1] =delta_current[1]-delta_current[0];
				undeltacurr[2] =delta_current[2]-delta_current[1];

				//Now convert back to power
				undeltapower[0] = *bus[indexer].V[0] * (~undeltacurr[0]);
				undeltapower[1] = *bus[indexer].V[1] * (~undeltacurr[1]);
				undeltapower[2] = *bus[indexer].V[2] * (~undeltacurr[2]);

				//Convert delta connected load to appropriate Wye - reuse temp variable
				undeltacurr[0] = voltageDel[0] * (*bus[indexer].Y[0]);
				undeltacurr[1] = voltageDel[1] * (*bus[indexer].Y[1]);
				undeltacurr[2] = voltageDel[2] * (*bus[indexer].Y[2]);

				undeltaimped[0] = (*bus[indexer].V[0] == 0) ? 0 : (undeltacurr[0] - undeltacurr[2]) / (*bus[indexer].V[0]);
				undeltaimped[1] = (*bus[indexer].V[1] == 0) ? 0 : (undeltacurr[1] - undeltacurr[0]) / (*bus[indexer].V[1]);
				undeltaimped[2] = (*bus[indexer].V[2] == 0) ? 0 : (undeltacurr[2] - undeltacurr[1]) / (*bus[indexer].V[2]);

				//Convert delta-current into a phase current - reuse temp variable
				undeltacurr[0]=*bus[indexer].I[0]-*bus[indexer].I[2];
				undeltacurr[1]=*bus[indexer].I[1]-*bus[indexer].I[0];
				undeltacurr[2]=*bus[indexer].I[2]-*bus[indexer].I[1];

				for (jindex=0; jindex<3; jindex++)
				{
					tempPbus = (undeltapower[jindex]).Re();									// Real power portion of constant power portion
					tempPbus += (undeltacurr[jindex]).Re() * (*bus[indexer].V[jindex]).Re() + (undeltacurr[jindex]).Im() * (*bus[indexer].V[jindex]).Im();	// Real power portion of Constant current component multiply the magnitude of bus voltage
					tempPbus += (undeltaimped[jindex]).Re() * (*bus[indexer].V[jindex]).Re() * (*bus[indexer].V[jindex]).Re() + (undeltaimped[jindex]).Re() * (*bus[indexer].V[jindex]).Im() * (*bus[indexer].V[jindex]).Im();	// Real power portion of Constant impedance component multiply the square of the magnitude of bus voltage
					bus[indexer].PL[jindex] = tempPbus;	//Real power portion
					tempQbus = (undeltapower[jindex]).Im();									// Reactive power portion of constant power portion
					tempQbus += (undeltacurr[jindex]).Re() * (*bus[indexer].V[jindex]).Im() - (undeltacurr[jindex]).Im() * (*bus[indexer].V[jindex]).Re();	// Reactive power portion of Constant current component multiply the magnitude of bus voltage
					tempQbus += -(undeltaimped[jindex]).Im() * (*bus[indexer].V[jindex]).Im() * (*bus[indexer].V[jindex]).Im() - (undeltaimped[jindex]).Im() * (*bus[indexer].V[jindex]).Re() * (*bus[indexer].V[jindex]).Re();	// Reactive power portion of Constant impedance component multiply the square of the magnitude of bus voltage				
					bus[indexer].QL[jindex] = tempQbus;	//Reactive power portion  
				}
			}
			else	//Wye-connected node
			{
				for (jindex=0; jindex<3; jindex++)
				{
					tempPbus = (*bus[indexer].S[jindex]).Re();									// Real power portion of constant power portion
					tempPbus += (*bus[indexer].I[jindex]).Re() * (*bus[indexer].V[jindex]).Re() + (*bus[indexer].I[jindex]).Im() * (*bus[indexer].V[jindex]).Im();	// Real power portion of Constant current component multiply the magnitude of bus voltage
					tempPbus += (*bus[indexer].Y[jindex]).Re() * (*bus[indexer].V[jindex]).Re() * (*bus[indexer].V[jindex]).Re() + (*bus[indexer].Y[jindex]).Re() * (*bus[indexer].V[jindex]).Im() * (*bus[indexer].V[jindex]).Im();	// Real power portion of Constant impedance component multiply the square of the magnitude of bus voltage
					bus[indexer].PL[jindex] = tempPbus;	//Real power portion
					tempQbus = (*bus[indexer].S[jindex]).Im();									// Reactive power portion of constant power portion
					tempQbus += (*bus[indexer].I[jindex]).Re() * (*bus[indexer].V[jindex]).Im() - (*bus[indexer].I[jindex]).Im() * (*bus[indexer].V[jindex]).Re();	// Reactive power portion of Constant current component multiply the magnitude of bus voltage
					tempQbus += -(*bus[indexer].Y[jindex]).Im() * (*bus[indexer].V[jindex]).Im() * (*bus[indexer].V[jindex]).Im() - (*bus[indexer].Y[jindex]).Im() * (*bus[indexer].V[jindex]).Re() * (*bus[indexer].V[jindex]).Re();	// Reactive power portion of Constant impedance component multiply the square of the magnitude of bus voltage				
					bus[indexer].QL[jindex] = tempQbus;	//Reactive power portion  
				}
			}
		}
	
		//if (Iteration==499)
		//{
		//	for (indexer=0; indexer<bus_count; indexer++)
		//	{
		//		for (jindex=0; jindex<3; jindex++)
		//		{
		//			fprintf(pFile,"%d %d %f %f\n",indexer,jindex,bus[indexer].PL[jindex],bus[indexer].QL[jindex]);
		//		}
		//	}
		//}

		//if (Iteration==499)
		//{
		//	for (indexer=0; indexer<bus_count; indexer++)
		//	{
		//		for (jindex=0; jindex<3; jindex++)
		//		{
		//			fprintf(pFile,"%d %d %f %f\n",indexer,jindex,(*bus[indexer].V[jindex]).Re(),(*bus[indexer].V[jindex]).Im());
		//		}
		//	}
		//}

		// Calculate the mismatch of three phase current injection at each bus (deltaI), 
		//and store the deltaI in terms of real and reactive value in array deltaI_NR    
		if (deltaI_NR==NULL)
		{
			deltaI_NR = (double *)gl_malloc((6*bus_count) *sizeof(double));   // left_hand side of equation (11)
		}

		if (Icalc==NULL)
		{
			Icalc = (complex *)gl_malloc((3*bus_count) *sizeof(complex));  // Calculated current injections at each bus is stored in Icalc for each iteration 
		}

		for (indexer=0; indexer<bus_count; indexer++) //for specific bus k
		{
			for (jindex=0; jindex<3; jindex++) // for specific phase s
			{
				tempIcalcReal = tempIcalcImag = 0;   
				tempPbus =  - bus[indexer].PL[jindex];	// @@@ PG and QG is assumed to be zero here @@@
				tempQbus =  - bus[indexer].QL[jindex];	
				
				for (kindex=0; kindex<3; kindex++)
				{
					tempIcalcReal += (BA_diag[indexer].Y[jindex][kindex]).Re() * (*bus[indexer].V[kindex]).Re() - (BA_diag[indexer].Y[jindex][kindex]).Im() * (*bus[indexer].V[kindex]).Im();// equation (7), the diag elements of bus admittance matrix 
					tempIcalcImag += (BA_diag[indexer].Y[jindex][kindex]).Re() * (*bus[indexer].V[kindex]).Im() + (BA_diag[indexer].Y[jindex][kindex]).Im() * (*bus[indexer].V[kindex]).Re();// equation (8), the diag elements of bus admittance matrix 
					for (jindexer=0; jindexer<branch_count; jindexer++)
					{
						if (branch[jindexer].from == indexer) 
						{
							tempIcalcReal += (-(*branch[jindexer].Yfrom[jindex][kindex])).Re() * (*bus[branch[jindexer].to].V[kindex]).Re() - (-(*branch[jindexer].Yfrom[jindex][kindex])).Im() * (*bus[branch[jindexer].to].V[kindex]).Im();// equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
							tempIcalcImag += (-(*branch[jindexer].Yfrom[jindex][kindex])).Re() * (*bus[branch[jindexer].to].V[kindex]).Im() + (-(*branch[jindexer].Yfrom[jindex][kindex])).Im() * (*bus[branch[jindexer].to].V[kindex]).Re();// equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance
						}	
						if  (branch[jindexer].to == indexer)
						{
							tempIcalcReal += (-(*branch[jindexer].Yto[jindex][kindex])).Re() * (*bus[branch[jindexer].from].V[kindex]).Re() - (-(*branch[jindexer].Yto[jindex][kindex])).Im() * (*bus[branch[jindexer].from].V[kindex]).Im() ;//equation (7), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance.
							tempIcalcImag += (-(*branch[jindexer].Yto[jindex][kindex])).Re() * (*bus[branch[jindexer].from].V[kindex]).Im() + (-(*branch[jindexer].Yto[jindex][kindex])).Im() * (*bus[branch[jindexer].from].V[kindex]).Re() ;//equation (8), the off_diag elements of bus admittance matrix are equal to negative value of branch admittance.
						}
						else;
					}
				}

				Icalc[indexer *3 + jindex] = complex(tempIcalcReal,tempIcalcImag);// calculated current injection  				
           		deltaI_NR[indexer*6+3 + jindex] = (tempPbus * (*bus[indexer].V[jindex]).Re() + tempQbus * (*bus[indexer].V[jindex]).Im())/ ((*bus[indexer].V[jindex]).Mag()*(*bus[indexer].V[jindex]).Mag()) - tempIcalcReal ; // equation(7), Real part of deltaI, left hand side of equation (11)
				deltaI_NR[indexer*6 + jindex] = (tempPbus * (*bus[indexer].V[jindex]).Im() - tempQbus * (*bus[indexer].V[jindex]).Re())/ ((*bus[indexer].V[jindex]).Mag()*(*bus[indexer].V[jindex]).Mag()) - tempIcalcImag ; // Imaginary part of deltaI, left hand side of equation (11)
			}
		}

		// Calculate the real and reactive power mismatch at each bus.
		if (deltaP==NULL)
		{
			deltaP = (double *)gl_malloc((3*bus_count) *sizeof(double));  // Calculated real power mismatch at each bus for each iteration 
		}

		if (deltaQ==NULL)
		{
			deltaQ = (double *)gl_malloc((3*bus_count) *sizeof(double));  // Calculated reactive power mismatch at each bus for each iteration 
		}

		for (indexer=0; indexer<bus_count; indexer++)
		{
			for (jindex=0; jindex<3; jindex++)
			{
				tempPbus = ((*bus[indexer].V[jindex]) * (~Icalc[indexer * 3 + jindex])).Re(); //corresponding to equation (22)
				tempQbus = -(bus[indexer].PL[jindex]);
				deltaP[indexer*3 + jindex] =  tempQbus - tempPbus;// corresponding to equation (20) PG is assumed to be zero here
				tempPbus = ((*bus[indexer].V[jindex]) * (~Icalc[indexer * 3 + jindex])).Im(); //corresponding to equation (21)
				tempQbus = -(bus[indexer].QL[jindex]);
				deltaQ[indexer*3 + jindex] = tempQbus - tempPbus;// corresponding to equation (23),QG is assumed to be zero here		
			}
		}

		// Convergence test. Check the active and reactive power mismatche deltaP and deltaQ, to see if they are in the tolerance.
		newiter = false;
		Maxmismatch = 0;
		for (indexer=3; indexer<(bus_count*3); indexer++) // do not check the swing bus
		{
			if ( Maxmismatch < abs(deltaP[indexer]))
			{	
				Maxmismatch = abs(deltaP[indexer]);	
			}	
			else if (Maxmismatch < abs(deltaQ[indexer]))
			{
				Maxmismatch = abs(deltaQ[indexer]);	
			}
			else
				;
		}

		if ( Maxmismatch <= eps)
		{
			printf("Power flow calculation converges at Iteration %d \n",Iteration-1);
		}
		else if ( Maxmismatch > eps)
			newiter = true;
		if ( newiter == false )
			break;

		// Calculate the elements of a,b,c,d in equations(14),(15),(16),(17). These elements are used to update the Jacobian matrix.	
		for (indexer=0; indexer<bus_count; indexer++)
		{
			if (bus[indexer].delta == true)	//Delta connected node
			{
				//Delta voltages
				voltageDel[0] = *bus[indexer].V[0] - *bus[indexer].V[1];
				voltageDel[1] = *bus[indexer].V[1] - *bus[indexer].V[2];
				voltageDel[2] = *bus[indexer].V[2] - *bus[indexer].V[0];

				//Power
				delta_current[0] = (voltageDel[0] == 0) ? 0 : ~(*bus[indexer].S[0]/voltageDel[0]);
				delta_current[1] = (voltageDel[1] == 0) ? 0 : ~(*bus[indexer].S[1]/voltageDel[1]);
				delta_current[2] = (voltageDel[2] == 0) ? 0 : ~(*bus[indexer].S[2]/voltageDel[2]);

				//Use delta current variable at first
				undeltacurr[0] =delta_current[0]-delta_current[2];
				undeltacurr[1] =delta_current[1]-delta_current[0];
				undeltacurr[2] =delta_current[2]-delta_current[1];

				//Now convert back to power
				undeltapower[0] = *bus[indexer].V[0] * (~undeltacurr[0]);
				undeltapower[1] = *bus[indexer].V[1] * (~undeltacurr[1]);
				undeltapower[2] = *bus[indexer].V[2] * (~undeltacurr[2]);

				//Convert delta connected load to appropriate Wye - reuse temp variable
				undeltacurr[0] = voltageDel[0] * (*bus[indexer].Y[0]);
				undeltacurr[1] = voltageDel[1] * (*bus[indexer].Y[1]);
				undeltacurr[2] = voltageDel[2] * (*bus[indexer].Y[2]);

				undeltaimped[0] = (*bus[indexer].V[0] == 0) ? 0 : (undeltacurr[0] - undeltacurr[2]) / (*bus[indexer].V[0]);
				undeltaimped[1] = (*bus[indexer].V[1] == 0) ? 0 : (undeltacurr[1] - undeltacurr[0]) / (*bus[indexer].V[1]);
				undeltaimped[2] = (*bus[indexer].V[2] == 0) ? 0 : (undeltacurr[2] - undeltacurr[1]) / (*bus[indexer].V[2]);

				//Convert delta-current into a phase current - reuse temp variable
				undeltacurr[0]=*bus[indexer].I[0]-*bus[indexer].I[2];
				undeltacurr[1]=*bus[indexer].I[1]-*bus[indexer].I[0];
				undeltacurr[2]=*bus[indexer].I[2]-*bus[indexer].I[1];

				for (jindex=0; jindex<3; jindex++)
				{
					bus[indexer].Jacob_A[jindex] = ((undeltapower[jindex]).Im() * (pow((*bus[indexer].V[jindex]).Re(),2) - pow((*bus[indexer].V[jindex]).Im(),2)) - 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Re())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(37)
					bus[indexer].Jacob_A[jindex] += ((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Re() + (undeltacurr[jindex]).Im() *pow((*bus[indexer].V[jindex]).Im(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) + (undeltaimped[jindex]).Im();// second part of equation(37)
					bus[indexer].Jacob_B[jindex] = ((undeltapower[jindex]).Re() * (pow((*bus[indexer].V[jindex]).Re(),2) - pow((*bus[indexer].V[jindex]).Im(),2)) + 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Im())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(38)
					bus[indexer].Jacob_B[jindex] += -((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Im() + (undeltacurr[jindex]).Re() *pow((*bus[indexer].V[jindex]).Re(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) - (undeltaimped[jindex]).Re();// second part of equation(38)
					bus[indexer].Jacob_C[jindex] = ((undeltapower[jindex]).Re() * (pow((*bus[indexer].V[jindex]).Im(),2) - pow((*bus[indexer].V[jindex]).Re(),2)) - 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Im())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(39)
					bus[indexer].Jacob_C[jindex] +=((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Im() - (undeltacurr[jindex]).Re() *pow((*bus[indexer].V[jindex]).Im(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) - (undeltaimped[jindex]).Re();// second part of equation(39)
					bus[indexer].Jacob_D[jindex] = ((undeltapower[jindex]).Im() * (pow((*bus[indexer].V[jindex]).Re(),2) - pow((*bus[indexer].V[jindex]).Im(),2)) - 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltapower[jindex]).Re())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(40)
					bus[indexer].Jacob_D[jindex] += ((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(undeltacurr[jindex]).Re() - (undeltacurr[jindex]).Im() *pow((*bus[indexer].V[jindex]).Re(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) - (undeltaimped[jindex]).Im();// second part of equation(40)
				}
			}
			else
			{
				for (jindex=0; jindex<3; jindex++)
				{
					bus[indexer].Jacob_A[jindex] = ((*bus[indexer].S[jindex]).Im() * (pow((*bus[indexer].V[jindex]).Re(),2) - pow((*bus[indexer].V[jindex]).Im(),2)) - 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].S[jindex]).Re())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(37)
					bus[indexer].Jacob_A[jindex] += ((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].I[jindex]).Re() + (*bus[indexer].I[jindex]).Im() *pow((*bus[indexer].V[jindex]).Im(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) + (*bus[indexer].Y[jindex]).Im();// second part of equation(37)
					bus[indexer].Jacob_B[jindex] = ((*bus[indexer].S[jindex]).Re() * (pow((*bus[indexer].V[jindex]).Re(),2) - pow((*bus[indexer].V[jindex]).Im(),2)) + 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].S[jindex]).Im())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(38)
					bus[indexer].Jacob_B[jindex] += -((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].I[jindex]).Im() + (*bus[indexer].I[jindex]).Re() *pow((*bus[indexer].V[jindex]).Re(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) - (*bus[indexer].Y[jindex]).Re();// second part of equation(38)
					bus[indexer].Jacob_C[jindex] = ((*bus[indexer].S[jindex]).Re() * (pow((*bus[indexer].V[jindex]).Im(),2) - pow((*bus[indexer].V[jindex]).Re(),2)) - 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].S[jindex]).Im())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(39)
					bus[indexer].Jacob_C[jindex] +=((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].I[jindex]).Im() - (*bus[indexer].I[jindex]).Re() *pow((*bus[indexer].V[jindex]).Im(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) - (*bus[indexer].Y[jindex]).Re();// second part of equation(39)
					bus[indexer].Jacob_D[jindex] = ((*bus[indexer].S[jindex]).Im() * (pow((*bus[indexer].V[jindex]).Re(),2) - pow((*bus[indexer].V[jindex]).Im(),2)) - 2*(*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].S[jindex]).Re())/pow((*bus[indexer].V[jindex]).Mag(),4);// first part of equation(40)
					bus[indexer].Jacob_D[jindex] += ((*bus[indexer].V[jindex]).Re()*(*bus[indexer].V[jindex]).Im()*(*bus[indexer].I[jindex]).Re() - (*bus[indexer].I[jindex]).Im() *pow((*bus[indexer].V[jindex]).Re(),2))/pow((*bus[indexer].V[jindex]).Mag(),3) - (*bus[indexer].Y[jindex]).Im();// second part of equation(40)
				}
			}
		}
		//Build the dynamic diagnal elements of 6n*6n Y matrix. All the elements in this part will be updated at each iteration.
		int size_diag_update = 0;
		for (jindexer=0; jindexer<bus_count;jindexer++) 
		{
			if  (bus[jindexer].type != 1)  
				size_diag_update += 1; 
			else {}
		}
		
		if (Y_diag_update == NULL)
		{
			Y_diag_update = (Y_NR *)gl_malloc((size_diag_update*12) *sizeof(Y_NR));   //Y_diag_update store the row,column and value of the dynamic part of the diagonal PQ bus elements of 6n*6n Y_NR matrix.
		}
		indexer = 0;

		for (jindexer=0; jindexer<bus_count; jindexer++)
		{
			for (jindex=0; jindex<3; jindex++)
			{
				if (bus[jindexer].type == 2)
				{
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind + 3;
					Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex + 3;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind - 3;
					Y_diag_update[indexer].Y_value = 1e10; // swing bus gets large admittance
					indexer += 1;
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex + 3;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = -1e10; // swing bus gets large admittance
					indexer += 1;
				}

				if (bus[jindexer].type != 1 && bus[jindexer].type != 2)
				{
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = (BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_A[jindex]; // Equation(14)
					indexer += 1;
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind + 3;
					Y_diag_update[indexer].Y_value = (BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_B[jindex]; // Equation(15)
					indexer += 1;
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex + 3;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind - 3;
					Y_diag_update[indexer].Y_value = (BA_diag[jindexer].Y[jindex][jindex]).Re() + bus[jindexer].Jacob_C[jindex]; // Equation(16)
					indexer += 1;
					Y_diag_update[indexer].row_ind = 6*jindexer + jindex + 3;
					Y_diag_update[indexer].col_ind = Y_diag_update[indexer].row_ind;
					Y_diag_update[indexer].Y_value = -(BA_diag[jindexer].Y[jindex][jindex]).Im() + bus[jindexer].Jacob_D[jindex]; // Equation(17)
					indexer += 1;
				}
				else;
			}
		}

		// Build the Amatrix, Amatrix includes all the elements of Y_offdiag_PQ, Y_diag_fixed and Y_diag_update.
		int size_Amatrix;
		size_Amatrix = size_offdiag_PQ*4 + size_diag_fixed*2 + size_diag_update*12;
		if (Y_Amatrix == NULL)
		{
			Y_Amatrix = (Y_NR *)gl_malloc((size_Amatrix) *sizeof(Y_NR));   // Amatrix includes all the elements of Y_offdiag_PQ, Y_diag_fixed and Y_diag_update.
		}
		for (indexer=0; indexer<size_offdiag_PQ*4; indexer++)
		{
			Y_Amatrix[indexer].row_ind = Y_offdiag_PQ[indexer].row_ind;
			Y_Amatrix[indexer].col_ind = Y_offdiag_PQ[indexer].col_ind;
			Y_Amatrix[indexer].Y_value = Y_offdiag_PQ[indexer].Y_value;
		}
		for (indexer=size_offdiag_PQ*4; indexer< (size_offdiag_PQ*4 + size_diag_fixed*2); indexer++)
		{
			Y_Amatrix[indexer].row_ind = Y_diag_fixed[indexer - size_offdiag_PQ*4 ].row_ind;
			Y_Amatrix[indexer].col_ind = Y_diag_fixed[indexer - size_offdiag_PQ*4 ].col_ind;
			Y_Amatrix[indexer].Y_value = Y_diag_fixed[indexer - size_offdiag_PQ*4 ].Y_value;
		}
		for (indexer=size_offdiag_PQ*4 + size_diag_fixed*2; indexer< size_Amatrix; indexer++)
		{
			Y_Amatrix[indexer].row_ind = Y_diag_update[indexer - size_offdiag_PQ*4 - size_diag_fixed*2].row_ind;
			Y_Amatrix[indexer].col_ind = Y_diag_update[indexer - size_offdiag_PQ*4 - size_diag_fixed*2].col_ind;
			Y_Amatrix[indexer].Y_value = Y_diag_update[indexer - size_offdiag_PQ*4 - size_diag_fixed*2].Y_value;
		}
		
		/* sorting integers using qsort() example */
		qsort(Y_Amatrix,size_Amatrix,sizeof(*Y_Amatrix),cmp);

		//if (Iteration==500)
		//{
		//	for (jindex=0; jindex<size_Amatrix;jindex++)
		//	{ 
		//		fprintf(pFile,"%d %d %f\n",Y_Amatrix[jindex].row_ind,Y_Amatrix[jindex].col_ind,Y_Amatrix[jindex].Y_value);
		//	}
		//}


		//Call SuperLU to solve the equation of AX=b;
		SuperMatrix A,B,L,U;
		double *a,*rhs;
		int *perm_c, *perm_r, *cols, *rows;
		int nnz, info;
		superlu_options_t options;
		SuperLUStat_t stat;
		NCformat *Astore;
		DNformat *Bstore;
		int m,n;
		double *sol;

		///* Initialize parameters. */
		m = bus_count*6; n = bus_count*6; nnz = size_Amatrix;

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
			//fprintf(pFile,"row: %d , a: %4.5f \n", rows[indexer], a[indexer]); // to be deleted;
		}
		cols[0] = 0;
		indexer = 0;
		kindex = 0;
		for ( jindex = 0; jindex< (size_Amatrix-1); jindex++)
		{ 
			indexer += 1;
			tempa = Y_Amatrix[jindex].col_ind;
			tempb = Y_Amatrix[jindex+1].col_ind;
			if (tempb > tempa)
			{
				kindex += 1;
				cols[kindex] = indexer;
				//fprintf(pFile,"col: %d \n", cols[kindex]); // to be deleted;
			}
		}
		cols[n] = nnz ;// number of non-zeros;

		for (jindex=0;jindex<m;jindex++)
		{ 
			rhs[jindex] = deltaI_NR[jindex];
		}

		//if (Iteration==499)
		//{
		//	for (jindex=0; jindex<m;jindex++)
		//	{ 
		//		fprintf(pFile,"%f\n",deltaI_NR[jindex]);
		//	}
		//}

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

		//if (Iteration==499)
		//{
		//	for (jindex=0; jindex<(bus_count*6);jindex++)
		//	{ 
		//		fprintf(pFile,"%f\n",sol[jindex]);
		//	}
		//}

		//for (jindex=0; jindex<6*bus_count; jindex++)
		//{ 
		//	fprintf(pFile," deltaV_NR %d  = %4.5f \n",jindex, sol[jindex]);
		//}
		//////Update the bus voltage with the AX=B solution.

		//if (Iteration==500)
		//{
		//	for (indexer=0; indexer<bus_count; indexer++)
		//	{
		//		for (jindex=0; jindex<3; jindex++)
		//		{
		//			fprintf(pFile,"%d %d %f %f\n",indexer,jindex,(*bus[indexer].V[jindex]).Re(),(*bus[indexer].V[jindex]).Im());
		//		}
		//	}
		//}

		kindex = 0;
		for (indexer=0; indexer<bus_count; indexer++)
		{
			//Avoid swing bus updates
			if (bus[indexer].type != 2)
			{
				for (jindex=0; jindex<3; jindex++)
				{
					((*bus[indexer].V[jindex]).Re()) = ((*bus[indexer].V[jindex]).Re()) + sol [kindex];
					kindex +=1;
				}

				for (jindex=0; jindex<3; jindex++)
				{
					((*bus[indexer].V[jindex]).Im()) = ((*bus[indexer].V[jindex]).Im()) +sol [kindex];
					kindex +=1;
				}
			}
			else
			{
				kindex +=6;	//Increment us for what this bus would have been had it not been a swing
			}
		}	

		//if (Iteration==500)
		//{
		//	for (indexer=0; indexer<bus_count; indexer++)
		//	{
		//		for (jindex=0; jindex<3; jindex++)
		//		{
		//			fprintf(pFile,"%d %d %f %f\n",indexer,jindex,(*bus[indexer].V[jindex]).Re(),(*bus[indexer].V[jindex]).Im());
		//		}
		//	}
		//}

		/* De-allocate storage. */
		gl_free(rhs);              // The Gridlab-d failed to execute free(rhs)!!!!!  
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
	}

	////Update the iteration count back to GridLAB-D
	//char outbuffer[64];

	//if (Iteration == 0)	//Make sure we didn't fail out for no more iterations
	//{
	//	outbuffer[0] = 48;	//0
	//	indexer = 1;		//Update flag so we can make sure to null stuff out
	//}
	//else	//Normal run, parse out what we are
	//{
	//	indexer=0;
	//	int TenBase = (int)log10(double(Iteration));
	//	int64 TempNum = 0;
	//	int64 IterUpdate = Iteration;
	//	int64 CurrBase;
	//	while (TenBase >= 0)
	//	{
	//		CurrBase = 10^TenBase;					//Current 10-base
	//		TempNum = (int)(IterUpdate/CurrBase);	//Amount of this multiplier we are
	//		
	//		IterUpdate -= TempNum * CurrBase;		//Pull this off for the next run
	//		
	//		buffer[indexer] = (char)(TempNum + 48);	//Store it

	//		TenBase--;	//Update multiplier
	//		indexer++;	//Update index
	//	}
	//}
	//for (;indexer<64;indexer++)
	//{
	//	outbuffer[indexer] = NULL;
	//}

	//gl_global_setvar("passes",buffer);


//for (jindexer=0; jindexer<bus_count; jindexer++)
//	{
//		for (jindex=0; jindex<3; jindex++)
//		{
//			fprintf(pFile,"At iteration %d bus %d voltage phase %d = %4.5f \n",Iteration,jindexer,jindex,(*bus[jindexer].V[jindex]).Mag());
//		}
//	}
//}
//////////////////////////////////////
//fclose(pFile); //////////////////////////////////////// to be delete

	gl_warning("Newton-Raphson solution method is not yet supported");
	return Iteration;
}




