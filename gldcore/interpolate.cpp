#include "interpolate.h"
#include "output.h"
#include <cmath>
#include <limits>

double interpolate_linear(double t, double x0, double y0, double x1,
                          double y1) {
  return y0 + (t - x0) * (y1 - y0) / (x1 - x0);
}

// Requires equally spaced x coordinates within rounding tolerance.
// Invalid inputs log an error and return 0.0 (also a valid result).
double interpolate_quadratic(double t, double x0, double y0, double x1, double y1, double x2, double y2){
	double a, b, c, h, v;
	const double h0 = x1 - x0;
	const double h1 = x2 - x1;
	const double scale = std::fmax(std::fabs(h0), std::fabs(h1));
	const double tolerance = 16.0 * std::numeric_limits<double>::epsilon() * scale;
	if (!std::isfinite(h0) || !std::isfinite(h1) ||
	    std::fabs(h0 - h1) > tolerance){
		output_error("interpolate_quadratic: this only works given three equally spaced points");
		return 0.0;
	}
	h = h0;
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
