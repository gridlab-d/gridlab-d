#include "interpolate.h"

double interpolate_linear(double t, double x0, double y0, double x1, double y1){
	return y0 + (t - x0) * (y1 - y0) / (x1 - x0);
}


// EOF
