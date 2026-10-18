#include "interpolate.h"
#include "output.h"
#include <cmath>
#include <limits>
#include <numeric>

double interpolate_linear(double t, double x0, double y0, double x1,
                          double y1) {
  return y0 + (t - x0) * (y1 - y0) / (x1 - x0);
}

// Requires equally spaced x coordinates within rounding tolerance.
// Invalid inputs log an error and return 0.0 (also a valid result).
double interpolate_quadratic(double t, double x0, double y0, double x1, double y1, double x2, double y2){
	const double h0 = x1 - x0;
	const double h1 = x2 - x1;
	const double scale = std::fmax(std::fabs(h0), std::fabs(h1));
	const double tolerance = 16.0 * std::numeric_limits<double>::epsilon() * scale;
	if (!std::isfinite(h0) || !std::isfinite(h1) ||
	    std::fabs(h0 - h1) > tolerance){
		output_error("interpolate_quadratic: this only works given three equally spaced points");
		return 0.0;
	}
	if (h0 == 0.0){
		output_error("interpolate_quadratic: x coordinates must be distinct");
		return 0.0;
	}
	// Keep the represented sample values exact, even when their magnitudes
	// are too far apart to share a useful floating-point scale.
	if (t == x0) return y0;
	if (t == x1) return y1;
	if (t == x2) return y2;
	// Evaluate in dimensionless coordinates centered on the middle sample.
	// Squaring h can overflow or underflow even when the answer is finite.
	const double u = (t - x1) / h0;
	// Power-of-two scaling is exact for ordinary operands. Normalize finite
	// ordinates before differencing to retain subnormal curvature and avoid
	// overflow when adjacent finite values have opposite signs.
	int exponent = 0;
	const double magnitude = std::fmax(std::fabs(y0), std::fmax(std::fabs(y1), std::fabs(y2)));
	if (magnitude > 0.0 && std::isfinite(magnitude)) std::frexp(magnitude, &exponent);
	const double z0 = std::scalbn(y0, -exponent);
	const double z1 = std::scalbn(y1, -exponent);
	const double z2 = std::scalbn(y2, -exponent);
	const double left = z1 - z0;
	const double right = z2 - z1;
	// midpoint avoids overflow and premature underflow in the half sums.
	const double a = std::midpoint(-left, right);
	const double b = std::midpoint(left, right);
	// Fuse Horner's steps so a subnormal linear contribution is not rounded
	// away before it is added to the middle sample.
	return std::scalbn(std::fma(std::fma(a, u, b), u, z1), exponent);
}

// EOF
