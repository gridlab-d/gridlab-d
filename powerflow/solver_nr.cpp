/* $Id
 * Newton-Raphson solver
 */

#include "solver_nr.h"

/* access to module global variables */
#include "powerflow.h"

/** Newton-Raphson solver
	Solves a power flow problem using the Newton-Raphson method
	
	@return n=0 on failure to complete a single iteration, 
	n>0 to indicate success after n interations, or 
	n<0 to indicate failure after n iterations
 **/
int solver_nr(int bus_count, BUSDATA *bus, int branch_count, BRANCHDATA *branch)
{
#ifdef DEBUG
	//Debugger output for NR solver - dumps into MATPOWER friendly format
	FILE *FP = fopen("caseMATPOWEROutput.m","w");
	int indexer, temper;
	char jindex, kindex;
	complex tempP, tempI, tempY;

	//Setup header stuffs
	fprintf(FP,"function [baseMVA, bus, gen, branch] = caseMATPOWEROutput\n");
	fprintf(FP,"%% This is a dump file of all current information about the GridLAB-D file\n");
	fprintf(FP,"%% the Newton-Raphson solver was just implementing.  This file should be\n");
	fprintf(FP,"%% directly executable inside MATPOWER.\n\n");
	fprintf(FP,"%%%%---- Power Flow Data ----%%%%\n");
	if (bus[0].mva_base==-1)
		fprintf(FP,"baseMVA = 0.000001;\n\n");	//Unspecified, so 1 W = 1 W
	else
		fprintf(FP,"baseMVA = %f;\n\n",bus[0].mva_base);	//Assume they'll all be properly based
	
	//Create busdata section
	fprintf(FP,"%%%%bus data\n%%Three separate networks to represent three phases\n");
	fprintf(FP,"%%B and C phases are set as PV generators, may need adjusting\nbus = [\n");
	for (indexer=0; indexer<bus_count; indexer++)
	{
		for (jindex=0; jindex<3; jindex++)
		{
			//Bus number and type - increment by 1 for MATLAB happiness
			temper=jindex*1000+indexer+1;
			if ((bus[indexer].type==2) && (jindex!=0))
				fprintf(FP," %d  %d",temper,(bus[indexer].type));
			else
				fprintf(FP," %d  %d",temper,(bus[indexer].type+1));
			
			//Power demand (load) - assume we're starting at nominal values (MATPOWER has no provisions for impedance or current loads)
			tempP = *bus[indexer].S[jindex];
			tempI = (*bus[indexer].V[jindex])*~(*bus[indexer].I[jindex]);
			tempY = (*bus[indexer].V[jindex])*~((*bus[indexer].V[jindex])*(*bus[indexer].Y[jindex]));

			tempP +=(tempI + tempY);
			fprintf(FP,"  %f  %f",tempP.Re(),tempP.Im());

			//Shunt conductance and susceptance - not used, so 0
			//Also area - set area to jindex (3 areas, each for a phase)
			fprintf(FP,"  0  0  %d",(jindex+1));

			//Voltage - current values
			tempP = *bus[indexer].V[jindex];
			fprintf(FP,"  %f  %f",tempP.Mag(),tempP.Arg()*180/PI);

			//Base kV
			if (bus[indexer].kv_base==-1)
				fprintf(FP,"  0.001");	//Non-based, just put 1 volt
			else
				fprintf(FP,"  %f",bus[indexer].kv_base);

			//Zone - zero us
			fprintf(FP,"  0");

			//Vmax and Vmin - set Vmin to 0 and Vmax to 4x current Vmag
			fprintf(FP,"  %f  0;\n",tempP.Mag()*4.0);
		}
	}
	fprintf(FP,"];\n\n");

	fprintf(FP,"%%%% Branch data\n");
	fprintf(FP,"%% Branches are formed from the diagonal of the admittance matrix.\n%%No easy method for putting the cross-terms in exists for this implementation.\n");
	fprintf(FP,"branch = [\n");
	//dump link data to do the same
	for (indexer=0; indexer<branch_count; indexer++)
	{
		for (jindex=0; jindex<3; jindex++)
		{
			//From/to indexing - increment by 1 for MATLAB happiness
			temper = 1000*jindex+branch[indexer].from+1;
			fprintf(FP,"  %d",temper);
			
			temper = 1000*jindex+branch[indexer].to+1;	//increment by 1 for MATLAB happiness
			fprintf(FP,"  %d",temper);

			//Admittance values (put back into impedance)
			tempY = *branch[indexer].Y[jindex][jindex];
			tempY = complex(1,0)/tempY;
			fprintf(FP,"  %f  %f  0",tempY.Re(),tempY.Im());

			//Rates (line ratings - use defaults)
			fprintf(FP,"  9900  0  0");

			//Voltage ratio
			temper = branch[indexer].v_ratio;
			if (temper==1.0)	//Normal line, encode as 0 (MATPOWER designation)
				fprintf(FP,"  0");
			else
				fprintf(FP,"  %f",temper);

			//Angle and status
			fprintf(FP,"  0  1;\n");
		}
	}
	fprintf(FP,"];\n");

	//Now create a "gen" item - put it on the swing bus
	fprintf(FP,"\n\n");
	fprintf(FP,"%%%% generator data\ngen = [\n");
	
	//Find the swing busses, put a generic generator on them
	for (indexer=0; indexer<bus_count; indexer++)
	{
		if (bus[indexer].type==2)	//Found one
		{
			for (jindex=0; jindex<3; jindex++)	//Create one on each swing bus
			{
				temper = jindex*1000+indexer+1;	//increment by 1 for MATLAB happiness
				fprintf(FP,"  %d  500  -120  Inf  -Inf",temper);
				
				if (bus[indexer].mva_base==-1)
					fprintf(FP,"  100");	//Unspecified, so set to 100 MVA for gens
				else
					fprintf(FP,"  %f",bus[indexer].mva_base);	//Specified

				tempP = *bus[indexer].V[jindex];
				fprintf(FP,"  %f  1  Inf  0;\n",tempP.Mag());
			}
		}
	}
	fprintf(FP,"];\n");

	fclose(FP);
#endif


//System load at each bus is represented by second order polynomial equations
	int indexer,jindex;
	double tempP[3]; //array tempP store the temporary value of P load at each bus  
    double tempQ[3]; //array tempQ store the temporary value of Q load at each bus 
	for (indexer=0; indexer<bus_count; indexer++)
		{
			for (jindex=0; jindex<3; jindex++)
			{
				tempP[0] = real(*bus[indexer].S[jindex]); // constant power component
				tempP[1] = real(cong(*bus[indexer].I[jindex])*(*bus[indexer].V[jindex]); // constant current component
                tempP[2] = real(cong(*bus[indexer].Y[jindex])*(abs(*bus[indexer].V[jindex])*abs(*bus[indexer].V[jindex])); // constant impedance component
				*bus[indexer].PL[jindex] = tempP[0]+tempP[1]+tempP[2];
                tempQ[0] = imag(*bus[indexer].S[jindex]); // constant power component
				tempQ[1] = imag(cong(*bus[indexer].I[jindex])*(*bus[indexer].V[jindex]); // constant current component
                tempQ[2] = imag(cong(*bus[indexer].Y[jindex])*(abs(*bus[indexer].V[jindex])*abs(*bus[indexer].V[jindex])); // constant impedance component
				*bus[indexer].QL[jindex] = tempQ[0]+tempQ[1]+tempQ[2];
			}
	}

//////////////////////

	/// @todo implement NR method
	GL_THROW("Newton-Raphson solution method is not yet supported");
	return 0;
}

