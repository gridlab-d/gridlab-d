/** $Id: random.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file random.h
	@addtogroup random
	@ingroup core
 @{
 **/

#ifndef _RANDOM_H
#define _RANDOM_H

#include "platform.h"

typedef enum {
	RT_INVALID=-1,	/**< used to flag bad random types */
	RT_DEGENERATE,	/**< degenerate distribution (Dirac delta function); double only_value */
	RT_UNIFORM,		/**< uniform distribution; double minimum_value, double maximum_value */
	RT_NORMAL,		/**< normal distribution; double arithmetic_mean, double arithmetic_stdev */
	RT_LOGNORMAL,	/**< log-normal distribution; double geometric_mean, double geometric_stdev */
	RT_BERNOULLI,	/**< Bernoulli distribution; double probability_of_observing_1 */
	RT_PARETO,		/**< Pareto distribution; double minimum_value, double gamma_scale */
	RT_EXPONENTIAL, /**< exponential distribution; double coefficient, double k_scale */
	RT_SAMPLED,		/**< sampled distribution; unsigned number_of_samples, double samples[n_samples] */
	RT_RAYLEIGH,	/**< Rayleigh distribution; double sigma */
	RT_WEIBULL,		/**< Weibull distribution; double lambda, double k */
	RT_GAMMA,		/**< Gamma distribution; double alpha, double beta */
	RT_BETA,		/**< Beta distribution; double alpha, double beta */
	RT_TRIANGLE,	/**< Triangle distribution; double a, double b */
} RANDOMTYPE;

#ifdef __cplusplus
extern "C" {
#endif
	int random_init(void);
	int random_test(void);
	int randwarn(void);
	double randunit(void);
	double random_degenerate(double a);
	double random_uniform(double a, double b);
	double random_normal(double m, double s);
	double random_bernoulli(double p);
	double random_sampled(unsigned int n, double *x);
	double random_pareto(double base, double gamma);
	double random_lognormal(double gmu, double gsigma);
	double random_exponential(double lambda);
	double random_functional(char *text);
	double random_beta(double alpha, double beta);
	double random_gamma(double alpha, double beta);
	double random_weibull(double l, double k);
	double random_rayleigh(double s);
	double random_triangle(double a, double b);
	double random_triangle_asy(double a, double b, double c);
	int random_apply(char *group_expression, char *property, RANDOMTYPE type, ...);
	RANDOMTYPE random_type(char *name);
	int random_nargs(char *name);
	double random_value(RANDOMTYPE type, ...);
	double pseudorandom_value(RANDOMTYPE, unsigned int *state, ...);
#ifdef __cplusplus
}
#endif

#endif

/** @} **/
