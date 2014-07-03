/** $Id: random.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file random.h
	@addtogroup random
	@ingroup core
 @{
 **/

#ifndef _RANDOM_H
#define _RANDOM_H

#include "platform.h"
#include "timestamp.h"
#include "property.h"

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
	int randwarn(unsigned int *state);
	double randunit(unsigned int *state);
	double random_degenerate(unsigned int *state, double a);
	double random_uniform(unsigned int *state, double a, double b);
	double random_normal(unsigned int *state, double m, double s);
	double random_bernoulli(unsigned int *state, double p);
	double random_sampled(unsigned int *state, unsigned int n, double *x);
	double random_pareto(unsigned int *state, double base, double gamma);
	double random_lognormal(unsigned int *state, double gmu, double gsigma);
	double random_exponential(unsigned int *state, double lambda);
	double random_functional(char *text);
	double random_beta(unsigned int *state, double alpha, double beta);
	double random_gamma(unsigned int *state, double alpha, double beta);
	double random_weibull(unsigned int *state, double l, double k);
	double random_rayleigh(unsigned int *state, double s);
	double random_triangle(unsigned int *state, double a, double b);
	double random_triangle_asy(unsigned int *state, double a, double b, double c);
	int random_apply(char *group_expression, char *property, RANDOMTYPE type, ...);
	RANDOMTYPE random_type(char *name);
	int random_nargs(char *name);
	double random_value(RANDOMTYPE type, ...);
	double pseudorandom_value(RANDOMTYPE, unsigned int *state, ...);
#ifdef __cplusplus
}
#endif

#define RNF_INTEGRATE 0x0001 /**< RNG flag for integral number, e.g., random walk */

typedef struct s_randomvar randomvar;
struct s_randomvar {
	double value;				/**< current value */
	unsigned int state;			/**< RNG state */
	RANDOMTYPE type;			/**< RNG distribution */
	double a, b;				/**< RNG distribution parameters */
	double low, high;			/**< RNG truncations limits */
	unsigned int update_rate;	/**< RNG refresh rate in seconds */
	unsigned int flags;			/**< RNG flags */
	/* internal parameters */
	randomvar *next;
};

int randomvar_update(randomvar *var);
int randomvar_create(randomvar *var);
int randomvar_init(randomvar *var);
int randomvar_initall(void);
TIMESTAMP randomvar_sync(randomvar *var, TIMESTAMP t1);
TIMESTAMP randomvar_syncall(TIMESTAMP t1);
int convert_to_randomvar(char *string, void *data, PROPERTY *prop);
int convert_from_randomvar(char *string,int size,void *data, PROPERTY *prop);
unsigned int64 random_id(void);
double random_get_part(void *x, char *name);
unsigned entropy_source(void);

#endif

/** @} **/
