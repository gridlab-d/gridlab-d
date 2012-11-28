/** $Id: solver_matpower.h 683 2008-06-18 20:16:29Z d3g637 $
	@file solver_matpower.h
	@addtogroup solver_matpower
	@ingroup MODULENAME
**/

#ifndef solverMatpower
#define solverMatpower

#include "libopf.h"
#include "matrix.h"

inline mxArray* initArray(double rdata[], int nRow, int nColumn);



extern int solver_matpower();


#endif
