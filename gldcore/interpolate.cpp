#include "interpolate.h"
#include "output.h"

double interpolate_linear(double t, double x0, double y0, double x1,
                          double y1) {
  return y0 + (t - x0) * (y1 - y0) / (x1 - x0);
}

// Requires equally spaced x coordinates; checked with exact equality.
// Invalid inputs log an error and return 0.0 (also a valid result).
double interpolate_quadratic(double t, double x0, double y0, double x1, double y1, double x2, double y2){
	double a, b, c, h, v;
	if(x1-x0 != x2-x1){
		output_error("interpolate_quadratic: this only works given three equally spaced points");
		return 0.0;
	}
	h = x1 - x0;
	if (h == 0.0){
		output_error("interpolate_quadratic: x coordinates must be distinct");
		return 0.0;
	}
	c = y0;
	b = (y1 - y0)/h;
	a = (y2 - 2*y1 + y0) / (2*h*h);
	v = a * (t - x0) * (t - x1) + b * (t - x0) + c;
	return v;
}

// EOF
