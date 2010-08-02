/** $Id: random.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file random.c
	@addtogroup random Random number generators
	@ingroup core
	
	Generate random numbers according to various distributions.

	@note The random number generators are not thread-safe.  This isn't generally
	a problem, unless you are using the pseudo-random sequences.  In that case, you
	need to lock the state variable you are using when generating random numbers.

 @{
 **/

#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <float.h>
#include "random.h"
#include "find.h"
#include "output.h"
#include "exception.h"
#include "globals.h"
#include "complex.h"
#include "lock.h"

#ifdef WIN32
#define finite _finite
#endif

#define QNAN sqrt(-1) /* quiet NaN used only for low-key failures */

static unsigned int *ur_state = NULL;

int random_init(void)
{
	/* randomizes the random number generator start value */
	if (global_randomseed==0)
		srand((unsigned int)time(NULL));
	else
	{
		srand(1);
		ur_state = &global_randomseed;
	}

	return 1;
}

/** Converts a distribution name to a #RANDOMTYPE
 **/
static struct {
	char *name;
	RANDOMTYPE type;
	int nargs;
} *p, map[] = {
	/* tested */
	{"degenerate", RT_DEGENERATE,1},
	{"uniform",RT_UNIFORM,2},
	{"normal",RT_NORMAL,2},
	{"bernoulli",RT_BERNOULLI,1},
	{"sampled",RT_SAMPLED,-1},
	{"pareto",RT_PARETO,2},
	{"lognormal",RT_LOGNORMAL,2},
	{"exponential",RT_EXPONENTIAL,1},
	{"rayleigh",RT_RAYLEIGH,1},
	/* untested */
	{"weibull",RT_WEIBULL,2},
	{"gamma",RT_GAMMA,1},
	{"beta",RT_BETA,2},
	{"triangle",RT_TRIANGLE,2},
	/** @todo Add some other distributions (e.g., Cauchy, Laplace)  (ticket #56)*/
};
/** Converts a distribution name to a #RANDOMTYPE
 **/
RANDOMTYPE random_type(char *name) /**< the name of the distribution */
{
	for (p=map; p<map+sizeof(map)/sizeof(map[0]); p++)
	{
		if (strcmp(p->name,name)==0)
			return p->type;
	}
	return RT_INVALID;
}
/** Gets the number of arguments required by distribution (0=failed, -1=variable)
 **/
int random_nargs(char *name)
{
	for (p=map; p<map+sizeof(map)/sizeof(map[0]); p++)
	{
		if (strcmp(p->name,name)==0)
			return p->nargs;
	}
	return 0;
}

/** randwarn checks to see if non-determinism warning is necessary **/
int randwarn()
{
	static int warned=0;
	if (global_nondeterminism_warning && !warned)
	{
		warned=1;
		output_warning("non-deterministic behavior probable--rand was called while running multiple threads");
	}

	return rand();
}

/* uniform distribution in range (0,1( */
double randunit(void)
{
	double u;
	unsigned int ur;
	static int random_lock=0;

	lock(&random_lock);
	
	if (ur_state!=NULL) 
		srand(*ur_state);
TryAgain:
	ur = randwarn();
	if (ur_state!=NULL)
		*ur_state = ur;
	u = ur/(RAND_MAX+1.0);
	if (u>=1) 
		goto TryAgain;

	unlock(&random_lock);
	
	return u;

}
double randunit_pos(void)
{
	double ur = 0.0;
	while (ur<=0)
		ur = randunit();
	return ur;
}

/** Generate the same number always.  This is the Dirac delta function:

	The probability density function for the Dirac delta function
	\f[ \varphi\left(a\right) = 1.0 \f]
	
 **/
double random_degenerate(double a)
{
	/* returns a, i.e., Dirac delta function */
	double aa = fabs(a);
	if (a!=0 && (aa<1e-30 || aa>1e30))
		output_warning("random_degenerate(a=%g): a is outside normal bounds of +/-1e(+/-30)", a);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	return a;
}

/** Generate a uniformly distributed random number 

	The probability density function for the uniform distribution is
	\f[ \varphi\left(x\right) = \left\{
		\begin{array}{c c}
			\frac{1}{b-a} & a \leq x < b \\
			0 & x<a ; x\geq b
		\end{array}
		\right\}
	\f]

	Note that this uniform distribution includes \e a but does not include \e b.
 **/
double random_uniform(double a, /**< the minimum number */ 
					  double b) /**< the maximum number */
{
	/* uniform distribution in range (a,b( */
	double aa=fabs(a), ab=fabs(b);
	if (a!=0 && (aa<1e-30 || aa>1e30))
		output_warning("random_uniform(a=%g, b=%g): a is outside normal bounds of +/-1e(+/-30)", a, b);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	if (b!=0 && (ab<1e-30 || ab>1e30))
		output_warning("random_uniform(a=%g, b=%g): b is outside normal bounds of +/-1e(+/-30)", a, b);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	if (b<a)
		output_warning("random_uniform(a=%g, b=%g): b is less than a", a, b);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	return randunit()*(b-a)+a;
}

/** Generate a Gaussian distributed random number 

	The probability density function for the Gaussian (normal) distribution is
	\f[ \varphi\left(x\right) = \frac{1}{\sqrt{2\pi}\sigma}e^{\frac{\left(x-\mu\right)^2}{2\sigma^2}}	\f]
 
	Normally distributed random numbers are generated using the Box-Muller method:

	\code
	double random_normal(double m, double s)
	{
		double r, a, b;
		do { 
			a = 2*randunit()-1;
			b = 2*randunit()-1;
			r = a*a+b*b;
		} while (r>=1);
		return sqrt(-2*log(r)/r)*a*r*s+m;
	}
	\endcode
	
**/
double random_normal(double m, /**< the mean of the distribution */ 
					 double s) /**< the standard deviation of the distribution */
{
	/* normal distribution centered on m, with variance s^2 */
	double r=randunit();
	if (s<0)
		output_warning("random_normal(m=%g, s=%g): s is negative", m, s);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	while (r<=0 || r>1)
		r=randunit();
	return sqrt(-2*log(r)) * sin(2*PI*randunit())*s+m;
}

/** Generate a Bernoulli distributed random number 

	The probability density function for the Bernoulli distribution is
	\f[ Prob\left\{x=1\right\} = 1-Prob\left\{x=0\right\} = p \f]

	Note that the Bernoulli distribution is a discrete distribution.
 **/
double random_bernoulli(double p) /**< the probability of generating a 1 */
{
	double ap = fabs(p);
	if (ap!=0 && (ap<1e-30 || ap>1e30))
		output_warning("random_bernoulli(p=%g): p is not within the normal bounds of +/-1e(+/-30)", p);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
if (p<0 || p>1)
		output_warning("random_bernoulli(p=%g): p is not between 0 and 1", p);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	/* 1 if rand<=p, 0 if rand>p */
	return (p>=randunit()) ? 1 : 0;
}

/** Generate a number randomly sample uniformly from a list 

	The probability that the value \e a is drawn from the sample is

	\f[ Prob\left\{x=a\right\} = \frac{1}{n} \f]

	Note that the sampled distriution is a discrete distribution.
 **/
double random_sampled(unsigned n, /**< the number of samples in the list */
					  double *x) /**< the sample list */
{
	if (n>0)
	{
		double v = x[(unsigned)(randunit()*n)];
		double av = fabs(v);
		if (v!=0 && (v<1e-30 || v>1e30))
			output_warning("random_sampled(n=%d,...): sampled value is not within normal bounds of +/-1e(+/-30)",n);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
		return v;
	}
	else
	{
		throw_exception("random_sampled(n=%d,...): n must be a positive number", n);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
		return QNAN; /* never gets here */
	}
}


/** Generate a Pareto distributed random number 

 	The Pareto distribution is one in which the probability 
	of drawing a number less than \p x is proportional to
	x^k.  The probability density function of the Pareto distribution is
	\f[ \varphi\left( x<k \right) = k \frac{m^k}{x^{k+1}} x \geq m \f]
	The cumulative density function is \f[ \varphi\left( x<k \right) = 1-(\frac{m}{x})^k \f].
**/
double random_pareto(double m, /**< the minimum value */
					 double k) /**< the k value */
{
	double am = fabs(m);
	double ak = fabs(k);
	double r = randunit();
	if (am!=0 && (am<1e-03 || am>1e30))
		output_warning("random_pareto(m=%g, k=%g): m is not within the normal bounds of +/-1e(+/-30)", m, k);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	if (ak>1e30)
		output_warning("random_pareto(m=%g, k=%g): k is not within the normal bounds of +/-1e(+/-30)", m, k);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	if (k<=0)
		throw_exception("random_pareto(m=%g, k=%g): k must be greater than 1", m, k);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	while (r<=0 || r>=1)
		r = randunit();
	return m*pow(r,-1/k);
}

/** Generate a log Gaussian distributed random number 

	The log-normal probability density function is
	\f[ \varphi\left(x\right) = \frac{1}{\sqrt{2\pi}x\sigma}e^{\frac{\left(\ln x-\mu\right)^2}{2\sigma^2}}	\f]
 **/
double random_lognormal(double gmu, /**< the geometric mean */
						double gsigma) /**< the geometric standard deviation */
{
	return exp(random_normal(0,1)*gsigma+gmu);
}

/** Generate an exponentially distributed random number 

	The exponential probability density function is
	\f[ \varphi\left(x\right) = \left\{
		\begin{array}{c c}
			\lambda e^{-\lambda x} & x \geq o \\
			0 & x < 0 
		\end{array}
		\right\}
	\f]
 **/
double random_exponential(double lambda) /**< the rate parameter lambda */
{
	double r=randunit();
	if (lambda<=0)
		throw_exception("random_exponential(l=%g): l must be greater than 0", lambda);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	if (lambda<1e-30 || lambda>1e30)
		output_warning("random_exponential(l=%g): l is not within the normal bounds of 1e(+/-30)", lambda);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	while (r<=0 || r>=1)
		r=randunit();
	return -log(r)/lambda;
}

/** Generate a Weibull distributed random number

	The Weibull probability density function is

	\f[ \varphi\left(x;k,\lambda\right) = \frac{k}{\lambda} \left( \frac{x}{\lambda} \right)^{k-1} e^{-\left(\frac{x}{\lambda}\right)^k} \f]

	@note This distribution is not tested because the test requires the Gamma function, which itself would have to be implemented and tested.

 **/
double random_weibull(double lambda, /**< scale parameter */
					  double k) /**< rate shape parameter */
{
	double r = randunit();
	if (k<=0)
		throw_exception("random_weibull(l=%g, k=%g): k must be greater than 0", lambda, k);
		/* TROUBLESHOOT
			An attempt to generate a random number used a parameter that was outside the expected range of real numbers.  
			Correct the functional definition of the random number and try again.
		 */
	return lambda * pow(-log(1-randunit()),1/k);	
}

/** Generate a Rayleigh distributed random number

	The Rayleigh probability density function is 

	\f[ \varphi \left( x;\sigma \right) = \frac{x e^{-\frac{x^2}{2\sigma^2}} }{\sigma^2} \f]

 **/
double random_rayleigh(double sigma) /**< mode parameter */
{
	return sigma*sqrt(-2*log(1-randunit()));
}

/** Generate a Gamma distributed random number

	The Gamma probability density function is

	\f[ \varphi \left( x; \alpha,\beta\right) = \frac{1}{\Gamma(\alpha) \beta^\alpha} x^{\alpha-1} e^{-x/\beta} \f]

 **/
double random_gamma(double alpha, double beta)
{
	/* used a different method depending on alpha */
	double na = floor(alpha);
	if (fabs(na-alpha)<1e-8 && na<12) /* alpha is an integer andsafe against underflow */
	{
		unsigned int i;
		double prod = 1;
		for (i=0; i<na; i++)
			prod *= randunit_pos();
		return -beta * log(prod);
	}
	else if (na<1) /* a is small */
	{
		double p, q, x, u, v;
		p = E/(alpha+E);
		do {
			u = randunit();
			v = randunit_pos();
			if (u<p)
			{
				x = exp((1/alpha)*log(v));
				q = exp(-x);
			}
			else
			{
				x = 1 - log(v);
				q = exp((alpha-1)*log(x));
			}
		} while (randunit()>=q);
		return beta*x;
	}
	else /* a is large */
	{
		double sqrta = sqrt(2*alpha-1);
		double x, y, v;
		do {
			do {
				y = tan(PI*randunit());
				x = sqrta*y+alpha-1;
			} while (x<=0);
			v = randunit();
		} while (v>(1+y*y) * exp((alpha-1)*log(x/(alpha-1))-sqrta*y));
		return beta*x;
	}
}

/** Generate a Beta distributed random number

	The Beta probability density function is

	\f[ \varphi \left( x; \alpha,\beta\right) = \frac{\Gamma(\alpha+\beta)}{\Gamma(\alpha) \Gamma(\beta)} x^(\alpha-1) (1-x)^{\beta-1} \f]

 **/

double random_beta(double alpha, double beta) /**< event parameters */
{
	/* use the transformer generator */
	double x1 = random_gamma(alpha,1);
	double x2 = random_gamma(beta,1);
	return x1 / (x1+x2);
}

/** Generate a symmetric triangle distributed random number

    The triangle probability density function is

	\f[ \varphi\left(x|a,b\right) = \left\{
		\begin{array}{c c}
			\frac{4(x-a)}{(b-a)^2} & a < x \leq (a+b)/2 \\
			\frac{4(b-x)}{(b-a)^2} & (a+b)/2 < x \leq b \\
			0 & x \leq a | x > b
		\end{array}
		\right\}
	\f]

 **/

double random_triangle(double a, double b)
{
	return (randunit() + randunit())*(b-a)/2 + a;
}

/* internal function that generates a random number */
static double _random_value(RANDOMTYPE type, va_list ptr)
{
	switch (type) {
	case RT_DEGENERATE:/* ... double value */
		{	double a = va_arg(ptr,double);
			return random_degenerate(a);
		}
	case RT_UNIFORM:		/* ... double min, double max */
		{	double min = va_arg(ptr,double);
			double max = va_arg(ptr,double);
			return random_uniform(min,max);
		}
	case RT_NORMAL:		/* ... double mean, double stdev */
		{	double mu = va_arg(ptr,double);
			double sigma = va_arg(ptr,double);
			return random_normal(mu,sigma);
		}
	case RT_BERNOULLI:	/* ... double p */
		return random_bernoulli(va_arg(ptr,double));
	case RT_SAMPLED: /* ... unsigned n_samples, double samples[n_samples] */
		{	unsigned n_samples = va_arg(ptr,unsigned);
			double *samples = va_arg(ptr,double*);
			return random_sampled(n_samples,samples);
		}
	case RT_PARETO:	/* ... double base, double gamma */
		{	double base = va_arg(ptr,double);
			double gamma = va_arg(ptr,double);
			return random_pareto(base,gamma);
		}
	case RT_LOGNORMAL:	/* ... double gmean, double gsigma */
		{	double gmu = va_arg(ptr,double);
			double gsigma = va_arg(ptr,double);
			return random_lognormal(gmu,gsigma);
		}
	case RT_EXPONENTIAL: /* ... double lambda */
		{	double lambda = va_arg(ptr,double);
			return random_exponential(lambda);
		}
	case RT_RAYLEIGH: /* ... double sigma */
		{	double sigma = va_arg(ptr,double);
			return random_rayleigh(sigma);
		}
	case RT_WEIBULL: /* ... double lambda, double k */
		{	double lambda = va_arg(ptr,double);
			double k = va_arg(ptr,double);
			return random_weibull(lambda,k);
		}
	case RT_GAMMA: /* ... double alpha, double beta */
		{	double alpha = va_arg(ptr,double);
			double beta = va_arg(ptr,double);
			return random_gamma(alpha,beta);
		}
	case RT_BETA: /* ... double alpha, double beta */
		{	double alpha = va_arg(ptr,double);
			double beta = va_arg(ptr,double);
			return random_beta(alpha,beta);
		}
	case RT_TRIANGLE: /* ... double a, double b */
		{	double a = va_arg(ptr,double);
			double b = va_arg(ptr,double);
			return random_triangle(a,b);
		}
	default:
		throw_exception("_random_value(type=%d,...); type is not valid",type);
		/* TROUBLESHOOT
			An attempt to generate a random number specific a distribution type that isn't recognized.
			Check that the distribution is valid and try again.
		 */
	}
	return QNAN; /* never gets here */
}

/** Apply a random number to property of a group of objects
	@return the number of objects changed
 **/
int random_apply(char *group_expression, /**< the group definition; see find_objects() */
				 char *property, /**< the property to update */
				 RANDOMTYPE type, /**< the distribution type */
				 ...) /**< the distribution's parameters */
{
	FINDLIST *list = find_objects(FL_GROUP, group_expression);
	OBJECT *obj;
	unsigned count=0;
	va_list ptr;
	va_start(ptr,type);
	for (obj=find_first(list); obj!=NULL; find_next(list,obj))
	{
		/* this is quite slow and should use a class property lookup */
		object_set_double_by_name(obj,property,_random_value(type,ptr));
		count++;
	}
	va_end(ptr);
	return count;
}

/** Generate a random value
	@return a double containing the random number
 **/
double random_value(RANDOMTYPE type, /**< the type of distribution desired */
					...) /**< the distribution's parameters */
{
	double x;
	va_list ptr;
	va_start(ptr,type);
	x = _random_value(type,ptr);
	va_end(ptr);
	return x;
}

/** Generate a pseudo-random value using the known state that is updated.
	@return a double containing the random number
 **/
double pseudorandom_value(RANDOMTYPE type, /**< the type of distribution desired */
						  unsigned int *state, /**< the state of the random number generator */
						  ...)/**< the distribution's parameters */
{
	double x;
	va_list ptr;
	va_start(ptr,state);
	/* switch to deterministic sequence */
	ur_state = state;
	x = _random_value(type,ptr);
	/* switch back to non-deterministic sequence */
	ur_state = NULL;
	va_end(ptr);
	return x;
}

/******************************************************************************/
static double mean(double sample[], unsigned int count)
{
	double sum=0;
	unsigned int i;
	for (i=0; i<count; i++)
		sum += sample[i];
	return sum/count;
}
#ifdef min
#undef min
#endif
static double min(double sample[], unsigned int count)
{
	double min;
	unsigned int i;
	for (i=0; i<count; i++)
	{
		if (i==0) min=sample[0];
		else if (sample[i]<min) min=sample[i];
	}
	return min;
}
#ifdef max
#undef max
#endif
static double max(double sample[], unsigned int count)
{
	double max;
	unsigned int i;
	for (i=0; i<count; i++)
	{
		if (i==0) max=sample[0];
		else if (sample[i]>max) max=sample[i];
	}
	return max;
}
static double stdev(double sample[], unsigned int count)
{
	double sum=0, mean=0;
	unsigned int n = 0, i;
	for (i=0; i<count; i++)
	{
		double delta = sample[i] - mean;
		n++;
		mean += delta/n;
		sum += delta*(sample[i]-mean);
	}
	return sqrt(sum/(n-1));
}
static void sort(double sample[], unsigned int count)
{
}
static int report(char *parameter, double actual, double expected, double error)
{
	if (parameter==NULL)
	{
		output_test("   Parameter       Actual    Expected    Error");
		output_test("---------------- ---------- ---------- ----------");
		return 0;
	}
	else
	{
		int iserror = fabs(actual-expected)>error ? 1 : 0;
		if (expected==0)
			output_test("%-16.16s %10.4f %10.4f %9.6f  %s", parameter,actual,expected,actual-expected, iserror?"ERROR":"");
		else
			output_test("%-16.16s %10.4f %10.4f %9.6f%% %s", parameter,actual,expected,(actual-expected)/expected*100, iserror?"ERROR":"");
		return iserror;
	}
}

/** Test random distributions

	To run the self-test, use the \p --randtest command-line argument.  
	The test results are output to the file specified by the \p global_testoutputfile variable.
	@return number of failed tests
 **/
int random_test(void)
{
	int failed = 0, ok=0, errorcount=0, preverrors=0;
	static double sample[1000000];
	double a, b;
	unsigned int count = sizeof(sample)/sizeof(sample[0]);
	unsigned int i;
	unsigned int initstate, state;

	output_test("\nBEGIN: random random distributions tests");

	/* Dirac distribution test */
	a = 10*randunit()/2-5;
	output_test("\ndegenerate(x=%g)",a);
	for (i=0; i<count; i++)
	{
		sample[i] = random_degenerate(a);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0);
	errorcount+=report("Mean",mean(sample,count),a,0.01);
	errorcount+=report("Stdev",stdev(sample,count),0,0.01);
	errorcount+=report("Min",min(sample,count),a,0.01);
	errorcount+=report("Max",max(sample,count),a,0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* uniform distribution test */
	a = 10*randunit()/2;
	b = 10*randunit()/2 + 5;
	output_test("\nuniform(min=%g, max=%g)",a,b);
	for (i=0; i<count; i++)
	{
		sample[i] = random_uniform(a,b);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0);
	errorcount+=report("Mean",mean(sample,count),(a+b)/2,0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt((b-a)*(b-a)/12),0.01);
	errorcount+=report("Min",min(sample,count),a,0.01);
	errorcount+=report("Max",max(sample,count),b,0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* Bernoulli distribution test */
	a = randunit();
	output_test("\nBernoulli(prob=%g)",a);
	for (i=0; i<count; i++)
	{
		sample[i] = random_bernoulli(a);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),a,0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt(a*(1-a)),0.01);
	errorcount+=report("Min",min(sample,count),0,0.01);
	errorcount+=report("Max",max(sample,count),1,0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* normal distribution test */
	a = 20*randunit()-5;
	b = 5*randunit();
	output_test("\nnormal(mean=%g, stdev=%g)",a,b);
	for (i=0; i<count; i++)
	{
		sample[i] = random_normal(a,b);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),a,0.01);
	errorcount+=report("Stdev",stdev(sample,count),b,0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* exponential distribution test */
	a = 1/randunit()-1;
	output_test("\nexponential(lambda=%g)",a);
	for (i=0; i<count; i++)
	{
		sample[i] = random_exponential(a);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),1/a,0.01);
	errorcount+=report("Stdev",stdev(sample,count),1/a,0.01);
	errorcount+=report("Min",min(sample,count),0,0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;
	
	/* lognormal distribution test */
	a = 2*randunit()-1;
	b = 2*randunit();
	output_test("\nlognormal(gmean=%g, gstdev=%g)",a,b);
	for (i=0; i<count; i++)
	{
		sample[i] = random_lognormal(a,b);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),exp(a+b*b/2),0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt((exp(b*b)-1)*exp(2*a+b*b)),0.1);
	errorcount+=report("Min",min(sample,count),0,0.1);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* Pareto distribution test */
	a = 10*randunit();
	b = randunit()*2+2;
	output_test("\nPareto(base=%g, gamma=%g)",a,b);
	for (i=0; i<count; i++)
	{
		sample[i] = random_pareto(a,b);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),b*a/(b-1),0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt(a*a*b/((b-1)*(b-1)*(b-2))),0.25);
	errorcount+=report("Min",min(sample,count),a,0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* rayleigh distribution test */
	a = 10*randunit();
	b = 4*randunit();
	output_test("\nRayleigh(sigma=%g)",a);
	for (i=0; i<count; i++)
	{
		sample[i] = random_rayleigh(a);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),a*sqrt(3.1415926/2),0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt((4-3.1415926)/2*a*a),0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* beta distribution test */
	a = 15*randunit();
	b = 4*randunit();
	output_test("\nBeta(a=%f,b=%f)",a,b);
	for (i=0; i<count; i++)
	{
		sample[i] = random_beta(a,b);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),a/(a+b),0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt(a*b/((a+b)*(a+b)*(a+b+1))),0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* gamma distribution test */
	a = 15*randunit();
	b = 4*randunit();
	output_test("\nGamma(a=%f,b=%f)",a,b);
	for (i=0; i<count; i++)
	{
		sample[i] = random_gamma(a,b);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),a*b,0.25);
	errorcount+=report("Stdev",stdev(sample,count),sqrt(a*b*b),0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* triangle distribution test */
	a = -randunit()*10;
	b = randunit()*10;
	output_test("\nTriangle(a=%f,b=%f)",a,b);
	for (i=0; i<count; i++)
	{
		sample[i] = random_triangle(a,b);
		if (!finite(sample[i]))
			output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),(a+b)/2,0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt((a-b)*(a-b)/24),0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* sampled distribution test */
	output_test("\nSampled(count=10, sample=[0..9])");
	for (i=0; i<count; i++)
	{
		double set[10]={0,1,2,3,4,5,6,7,8,9};
		sample[i] = random_sampled(10,set);
		if (!finite(sample[i]))
			failed++,output_test("Sample %d is not a finite number!",i--);
	}
	errorcount+=report(NULL,0,0,0.01);
	errorcount+=report("Mean",mean(sample,count),4.5,0.01);
	//report("Stdev",stdev(sample,count),sqrt(9*9/12),0.01);
	errorcount+=report("Stdev",stdev(sample,count),sqrt(99/12),0.1); /* sqrt((b-a+1)^2-1 / 12)*/ /* 2.87 is more accurate and was Mathematica's answer */
	errorcount+=report("Min",min(sample,count),0,0.01);
	errorcount+=report("Max",max(sample,count),9,0.01);
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* test non-deterministic sequences */
	output_test("\nNon-deterministic test (N=%d)",count);
	for (i=0; i<count; i++)
		sample[i] = random_value(RT_NORMAL,0,1);
	for (i=0; i<count; i++)
	{
		double v = random_value(RT_NORMAL,0,1);
		if (sample[i] == v)
			failed++,output_test("Sample %d matched (%f==%f)", sample[i],v);
	}	
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* test deterministic sequences */
	initstate = state = rand();
	output_test("\nDeterministic test for state %u (N=%d)",initstate,count);
	for (i=0; i<count; i++)
		sample[i] = pseudorandom_value(RT_NORMAL,&state,0,1);
	state = initstate;
	for (i=0; i<count; i++)
	{
		double v = pseudorandom_value(RT_NORMAL,&state,0,1);
		if (sample[i] != v)
			failed++,output_test("Sample %d did not match (%f!=%f)", sample[i],v);
	}	
	if (preverrors==errorcount)	ok++; else failed++;
	preverrors=errorcount;

	/* report results */
	if (failed)
	{
		output_error("randtest: %d random distributions tests failed--see test.txt for more information",failed);
		output_test("!!! %d random distributions tests failed, %d errors found",failed,errorcount);
	}
	else
	{
		output_verbose("%d random distributions tests completed with no errors--see test.txt for details",ok);
		output_test("randtest: %d random distributions tests completed, %d errors found",ok,errorcount);
	}
	output_test("END: random distributions tests");
	return failed;
}

/** @} **/

