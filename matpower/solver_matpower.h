/** $Id$
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
