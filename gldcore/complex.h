/** $Id: complex.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file complex.h
	@addtogroup complex Complex numbers
	@ingroup core

	The complex number implementation is specifically designed to use in power systems
	calculations.  Hence it supports some things that a peculiar to power systems engineering.
	In particular, the imaginary portion can be identified using \e i, \e j or \e d (for polar
	coordinates).

	Note that the functions are only available to C++ code and only the \p struct is available to
	C code.
 
 @{
 **/

#ifndef _COMPLEX_H
#define _COMPLEX_H

typedef enum {I='i',J='j',A='d', R='r'} CNOTATION; /**< complex number notation to use */
#define CNOTATION_DEFAULT J /* never set this to A */
#define PI 3.1415926535897932384626433832795
#define E 2.71828182845905

#include <math.h>
#include "platform.h"

/* only cpp code may actually do complex math */
#ifndef __cplusplus
typedef struct s_complex {
#else
class complex { 
private:
#endif
	double r; /**< the real part */
	double i; /**< the imaginary part */
	CNOTATION f; /**< the default notation to use */
#ifndef __cplusplus
} complex;
#define complex_set_polar(X,M,A) ((X).r=((M)*cos(A)),(X).i=((M)*sin(A)),(X))
#define complex_set_power_factor(X,M,P)	complex_set_polar((X),(M)/(P),acos(P))
#define complex_get_mag(X) (sqrt((X).r*(X).r + (X).i+(X).i))
#define complex_get_arg(X) ((X).r==0 ? ( (X).i > 0 ? PI/2 : ((X).i<0 ? -PI/2 : 0) ) : ( (X).r>0 ? atan((X).i/(X).r) : PI+atan((X).i/(X).r) ))
double complex_get_part(void *c, char *name);
#else
public:
	/** Construct a complex number with zero magnitude */
	inline complex() /**< create a zero complex number */
	{
		r = 0;
		i = 0;
		f = CNOTATION_DEFAULT;
	};
	inline complex(double re) /**< create a complex number with only a real part */
	{
		r = re;
		i = 0;
		f = CNOTATION_DEFAULT;
	};
	inline complex(double re, double im, CNOTATION nf=CNOTATION_DEFAULT) /**< create a complex number with both real and imaginary parts */
	{
		f = nf;
		//if (nf==A)
		//{
		//	SetPolar(re,im);
		//}
		//else
		//{
			r = re;
			i = im;
		//}
	};
	
	/* assignment operations */
	inline complex &operator = (complex x) /**< complex assignment */
	{
		r = x.r; 
		i = x.i; 
		f = x.f; 
		return *this;
	};
	inline complex &operator = (double x) /**< double assignment */
	{
		r = x; 
		i = 0; 
		f = CNOTATION_DEFAULT;
		return *this;
	};

	/* access operations */
	inline double & Re(void) /**< access to real part */
	{
		return r;
	};
	inline double & Im(void) /**< access to imaginary part */
	{
		return i;
	};
	inline CNOTATION & Notation(void) /**< access to notation */
	{
		return f;
	};
	inline double Mag(void) const /**< compute magnitude */
	{
		return sqrt(r*r+i*i);
	};
	inline double Mag(double m)  /**< set magnitude */
	{
		double old = sqrt(r*r+i*i);
		r *= m/old;
		i *= m/old;
		return m;
	};
	inline double Arg(void) const /**< compute angle */
	{
		if (r==0)
		{
			if (i>0)
				return PI/2;
			else if (i==0)
				return 0;
			else
				return -PI/2;
		}
		else if (r>0)
			return atan(i/r);
		else
			return PI+atan(i/r);
	};
	inline double Arg(double a)  /**< set angle */
	{
		SetPolar(Mag(),a,f);
		return a;
	};
	inline complex Log(void) const /**< compute log */
	{ 
		return complex(log(Mag()),Arg(),f);
	};
	inline void SetReal(double v) /**< set real part */
	{
		r = v;
	};
	inline void SetImag(double v) /**< set imaginary part */
	{
		i = v;
	};
	inline void SetNotation(CNOTATION nf) /**< set notation */
	{
		f = nf;
	}
	inline void SetRect(double rp, double ip, CNOTATION nf=CNOTATION_DEFAULT) /**< set rectangular value */
	{
		r = rp;
		i = ip;
		f = nf;
	};
	inline void SetPolar(double m, double a, CNOTATION nf=A) /**< set polar values */
	{
		r = (m*cos(a)); 
		i = (m*sin(a));
		f = nf;
	};

#if 0
	//inline operator const double (void) const /**< cast real part to double */
	//{
	//	return r;
	//};
#endif

	inline complex operator - (void) /**< change sign */
	{
		return complex(-r,-i,f);
	};
	inline complex operator ~ (void) /**< complex conjugate */
	{ 
		return complex(r,-i,f);
	};

	/* reflexive math operations */
	inline complex &operator += (double x) /**< add a double to the real part */
	{
		r += x; 
		return *this;
	};
	inline complex &operator -= (double x) /**< subtract a double from the real part */
	{
		r -= x; 
		return *this;
	};
	inline complex &operator *= (double x) /**< multiply a double to real part */
	{
		r *= x; 
		i *= x; 
		return *this;
	};
	inline complex &operator /= (double x) /**< divide into the real part */
	{
		r /= x; 
		i /= x;
		return *this;
	};
	inline complex &operator ^= (double x) /**< raise to a real power */
	{ 
		double lm = log(Mag()), a = Arg(), b = exp(x*lm), c = x*a; 
		r = (b*cos(c)); 
		i = (b*sin(c)); 
		return *this;
	};
	inline complex &operator += (complex x) /**< add a complex number */
	{
		r += x.r; 
		i += x.i; 
		return *this;
	};
	inline complex &operator -= (complex x)  /**< subtract a complex number */
	{
		r -= x.r; 
		i -= x.i; 
		return *this;
	};
	inline complex &operator *= (complex x)  /**< multip[le by a complex number */
	{
		double pr=r; 
		r = pr * x.r - i * x.i; 
		i = pr * x.i + i * x.r; 
		return *this;
	};
	inline complex &operator /= (complex y)  /**< divide by a complex number */
	{
		double xr=r;
		double a = y.r*y.r+y.i*y.i;
		r = (xr*y.r+i*y.i)/a; 
		i = (i*y.r-xr*y.i)/a;
		return *this;
	};
	inline complex &operator ^= (complex x) /**< raise to a complex power */
	{ 
		double lm = log(Mag()), a = Arg(), b = exp(x.r*lm-x.i*a), c = x.r*a+x.i*lm; 
		r = (b*cos(c)); 
		i = (b*sin(c)); 
		return *this;
	};

	/* binary math operations */
	inline complex operator + (double y) /**< double sum */
	{
		complex x(*this);
		return x+=y;
	};
	inline complex operator - (double y) /**< double subtract */
	{
		complex x(*this);
		return x-=y;
	};
	inline complex operator * (double y) /**< double multiply */
	{
		complex x(*this);
		return x*=y;
	};
	inline complex operator / (double y) /**< double divide */
	{
		complex x(*this);
		return x/=y;
	};
	inline complex operator ^ (double y) /**< double power */
	{
		complex x(*this);
		return x^=y;
	};
	inline complex operator + (complex y) /**< complex sum */
	{
		complex x(*this);
		return x+=y;
	};
	inline complex operator - (complex y) /**< complex subtract */
	{
		complex x(*this);
		return x-=y;
	};
	inline complex operator * (complex y) /**< complex multiply */
	{
		complex x(*this);
		return x*=y;
	};
	inline complex operator / (complex y) /**< complex divide */
	{
		complex x(*this);
		return x/=y;
	};
	inline complex operator ^ (complex y) /**< complex power */
	{
		complex x(*this);
		return x^=y;
	};

	/** set power factor */
	inline complex SetPowerFactor(double mag, /**< magnitude of power */
		double pf, /**< power factor */
		CNOTATION n=J) /** notation */
	{
		SetPolar(mag/pf, acos(pf),n);
		return *this;
	}


	/* comparison */
	inline bool IsZero(double err=0.0) /**< zero test */
	{
		return Mag()<=err;
	}

	/* magnitude comparisons */
	inline bool operator == (double m)	{ return Mag()==m; };
	inline bool operator != (double m)	{ return Mag()!=m; };
	inline bool operator < (double m)	{ return Mag()<m; };
	inline bool operator <= (double m)	{ return Mag()<=m; };
	inline bool operator > (double m)	{ return Mag()>m; };
	inline bool operator >= (double m)	{ return Mag()>=m; };

	/* angle comparisons */
	inline bool operator == (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)==0.0;};
	inline bool operator != (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)!=0.0;};
	inline bool operator < (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)<PI;};
	inline bool operator <= (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)<=PI;};
	inline bool operator > (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)>PI;};
	inline bool operator >= (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)>=PI;};
	inline bool IsFinite(void) { return isfinite(r) && isfinite(i); };
};
#endif

#endif
 /**@}**/
