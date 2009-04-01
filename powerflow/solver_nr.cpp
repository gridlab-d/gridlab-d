/* $Id
 * Newton-Raphson solver
 */

#include "solver_nr.h"

/* access to module global variables */
#include "powerflow.h"

/** Newton-Raphson solver
	Solves a power flow problem using the Newton-Raphson method
	
	@return n=0 on failure to complex a single iteration, 
	n>0 to indicate success after n interations, or 
	n<0 to indicate failure after n iterations
 **/
int solve_nr(int bus_count, BUSDATA *bus, int branch_count, BRANCHDATA *branch)
{
	/// @todo implement NR method
	throw "Newton-Raphson solution method is not yet supported";
}