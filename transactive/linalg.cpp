// $Id: linalg.cpp 5218 2015-07-06 01:32:36Z dchassin $

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "linalg.h"
#include "platform.h"

#include "transactive.h"

#ifdef HAVE_MATLAB
// you must have matlab installed and ensure matlab/extern/include is in include path

#define DUMP_MATLAB // enable dumping of matlab interactions (requires verbose)

#include "matrix.h"
#include "engine.h"

static Engine *matlab = NULL;
char1024 matlab_command = "c:\\program files\\matlab\\r2014a\\matlab.exe";

#else // otherwise you must install CPPLapack (which doesn't work right anyway)
#include <cpplapack.h>
#endif

static double randu(double a=0, double b=1)
{
	return rand()/(RAND_MAX+1.0)*(b-a)+a;
}
static double randn(double m=0, double s=1)
{
	double r;
	do {
		r = randu();
	} while (r<=0 || r>=1);
	// Box-Muller Gaussian variate
	return sqrt(-2*log(r)) * sin(2*3.141592635*randu())*s+m;
}

/////////////////////////////////////////////////////////////////////////
// VECTOR
/////////////////////////////////////////////////////////////////////////
vector::vector() : M(0)
{
	data = NULL;
	size = 0;
}

vector::vector(const size_t n, double init) : size(n)
{
	M=size;
	double *p = data = new double[M];
	if ( init==0.0 )
		memset(data,0,sizeof(double)*M);
	else if ( !isnan(init) )
		for ( int i=0 ; i<M ; i++ )
			*p++ = init;
}

vector::vector(const size_t n, DISTRIBUTION rt, double a, double b) : size(n)
{
	M=size;
	double *p = data = new double[M];
	switch ( rt ) {
	case RD_UNIFORM:
		for ( int i=0 ; i<M ; i++ ) *p++ = randu(a,b);
		break;
	case RD_NORMAL:
		for ( int i=0 ; i<M ; i++ ) *p++ = randn(a,b);
		break;
	default:
		throw "vector::vector() distribution not supported";
	}
}

vector::vector(const vector &v) : size(v.M)
{
	M=size;
	data = new double[M];
	memcpy(data,v.data,sizeof(double)*M);
}

vector::~vector(void)
{
	delete [] data;
}
void vector::copy(const vector &x)
{
	if (x.size>size) 
	{
		if ( data!=NULL ) delete [] data;
		data = new double[x.size];
		size = x.size;
	}
	memcpy(data,x.data,sizeof(double)*x.M);
	M = x.M;
}

vector vector::operator - (void) const
{
	vector neg(M);
	for ( int i=0 ; i<M ; i++ )
		neg[i] = -data[i];
	return neg;
}

bool vector::operator == ( const vector &v) const
{
	if ( v.M!=M ) throw "vector::operator==(vector): dimension mismatch";
	for ( int i=0 ; i<M ; i++ )
		if ( v.data[i]!=data[i] ) return false;
	return true;
}

vector vector::operator + (const double x) const
{
	vector y(M);
	for ( int i=0 ; i<M ; i++ )
		y[i] = data[i] + x;
	return y;
}

vector vector::operator - (const double x) const
{
	vector y(M);
	for ( int i=0 ; i<M ; i++ )
		y[i] = data[i] - x;
	return y;
}

vector vector::operator * (const double x) const
{
	vector y(M);
	for ( int i=0 ; i<M ; i++ )
		y[i] = data[i] * x;
	return y;
}

vector vector::operator / (const double x) const
{
	vector y(M);
	for ( int i=0 ; i<M ; i++ )
		y[i] = data[i] / x;
	return y;
}

vector vector::operator + (const vector &v) const
{
	if ( v.M!=M ) throw "vector::operator+(vector): dimension mismatch";
	vector y(M);
	for ( int i=0 ; i<M ; i++ )
		y[i] = v.data[i] + data[i];
	return y;
}

vector vector::operator - (const vector &v) const
{
	if ( v.M!=M ) throw "vector::operator-(vector): dimension mismatch";
	vector y(M);
	for ( int i=0 ; i<M ; i++ )
		y[i] = v.data[i] - data[i];
	return y;
}

double vector::operator * (const vector &v) const
{
	if ( v.M!=M ) throw "vector::operator*(vector): dimension mismatch";
	double y = 0.0;
	for ( int i=0 ; i<M ; i++ )
		y += v.data[i] * data[i];
	return y;
}

vector vector::operator * (matrix &m) const
{
	if ( m.M!=M ) throw "vector::operator*(matrix): dimension mismatch";
	vector y(m.N,0);
	for ( int c=0 ; c<m.N ; c++ )
		for ( int r=0 ; r<M ; r++ )
			y[c] += data[r] * m[r][c] ;
	return y;
}

vector vector::operator / (const matrix &m) const
{
	if ( m.N!=M ) throw "vector::operator/(matrix): dimension mismatch";
	vector y(m.M);
	double cond=1;
	m.solve((*this),y,&cond);
	if ( cond>1e3 )
		gl_warning("vector::operator/(matrix): low matrix condition number");
	return y;
}

double vector::sum(void) const
{
	double y = 0;
	for ( int i=0 ; i<M ; i++ )
		y += data[i];
	return y;
}

double vector::norm(const unsigned int n) const
{
	if ( n<0 ) throw "vector::norm(): negative norm is invalid";
	double y = 0;
	switch ( n ) {
	case 0:
		// TODO
		throw "vector::norm(): zero norm is not implemented";
		break;
	case 1:
		for ( int i=0 ; i<M ; i++ )
			y += fabs(data[i]);
		break;
	case 2:
		for ( int i=0 ; i<M ; i++ )
			y += data[i]*data[i];
		y = sqrt(y);
		break;
	default:
		for ( int i=0 ; i<M ; i++ )
			y += pow(fabs(data[i]),(int)n);
		y = pow(y,1.0/n);
		break;
	}
	return y;
}
 
double vector::maximum(void) const
{
	double y = data[0];
	for ( int i=1 ; i<M ; i++ )
		if ( data[i]>y )
			y = data[i];
	return y;
}

double vector::minimum(void) const
{
	double y = data[0];
	for ( int i=1 ; i<M ; i++ )
		if ( data[i]<y )
			y = data[i];
	return y;
}

vector vector::range(void) const
{
	vector y(2,data[0]);
	for ( int i=1 ; i<M ; i++ )
	{
		if ( data[i]<y[0] )
			y[0] = data[i];
		else if ( data[i]>y[1] )
			y[1] = data[i];
	}
	return y;
}
void vector::dump(FILE *fh, const char *name, const char *fmt) const
{
	fprintf(fh,"%s = [",name);
	for ( int i=0 ; i<M ; i++ )
	{	
		if ( i>0 ) fputs(",",fh);
		fprintf(fh,fmt,data[i]);
	}
	fprintf(fh,"];\n");
}

/////////////////////////////////////////////////////////////////////////
// MATRIX
/////////////////////////////////////////////////////////////////////////

matrix::matrix() : M(0), N(0)
{
	data = NULL;
	inv = NULL;
	have_inv = false;
}

matrix::matrix(const size_t size, double diag) : M(size), N(size)
{ 
	data = new double[M*N];
	if ( !isnan(diag) )
	{
		memset(data,0,sizeof(double)*M*N);
		if ( diag!=0.0 )
		{
			for ( int i=0 ; i<size; i++ ) 
			{
				data[i*N+i] = diag;
			}
		}
	}
	inv = NULL;
	have_inv = false;
}
matrix::matrix(const size_t rows, const size_t cols, double init) : M(rows), N(cols)
{ 
	double *p = data = new double[M*N];
	if ( !isnan(init) )
	{
		for ( int m=0 ; m<M*N ; m++ )
			*p++ = init;
	}
	inv = NULL;
	have_inv = false;
}

matrix::matrix(const size_t size, DISTRIBUTION rt, double a, double b) : M(size), N(size)
{ 
	if ( rt!=RD_UNIFORM ) throw "matrix::matrix() random distribution not supported";
	data = new double[M*N]; 
	double r = b-a;
    for ( int i=0 ; i<M*N; i++ ) 
	{    
		data[i] = ((double)rand())/RAND_MAX*r+a;
	}
	inv = NULL;
	have_inv = false;
}

matrix::matrix(const size_t m, const size_t n, DISTRIBUTION rt, double a, double b) : M(m), N(n)
{ 
	if ( rt!=RD_UNIFORM ) throw "matrix::matrix() random distribution not supported";
	data = new double[M*N]; 
	double r = b-a;
    for ( int i=0 ; i<M*N; i++ ) 
	{    
		data[i] = ((double)rand())/RAND_MAX*r+a;
	}
	inv = NULL;
	have_inv = false;
}

matrix::matrix(const matrix &m) : M(m.M), N(m.N)
{
	data = new double[M*N];
	copy(m.data);
	inv = NULL;
	have_inv = false;
}

matrix::matrix(const size_t m, const size_t n, double *ptr) : M(m), N(n)
{
	data = new double[M*N];
	memcpy(data,ptr,sizeof(double)*M*N);
	inv = NULL;
	have_inv = false;
}

matrix::~matrix(void) 
{
	delete [] data;
	if ( inv ) delete[] inv;
} 

matrix &matrix::operator = (const matrix &m) 
{
	if ( size()<m.size() )
	{
		delete [] data;
		data = new double[m.size()];
	}
	M = m.M;
	N = m.N;
	copy(m.data);
    return *this; 
} 

matrix matrix::operator - (void) const
{
	matrix neg(M,N);
    for ( int i=0 ; i<size() ; i++ ) 
        neg.data[i] = -data[i]; 
    return neg; 
}
matrix matrix::operator ~ (void) const
{ 
	matrix trans(N,M);
	double *p = data;
    for ( int r=0 ; r<M ; r++ ) 
        for(int c=0 ; c<N ; c++ ) 
            trans[c][r] = *p++; 
    return trans; 
} 

matrix matrix::operator + (double x) const
{
	matrix y(size());
	y.copy(data);
    for ( int i=0 ; i<size() ; i++ ) 
        y.data[i] += x; 
    return y; 
}

matrix matrix::operator - (double x) const
{
	matrix y(size());
	y.copy(data);
    for ( int i=0 ; i<size() ; i++ ) 
        y.data[i] -= x; 
    return y; 
}
matrix matrix::operator * (double x) const
{
	matrix y(size());
	y.copy(data);
    for ( int i=0 ; i<size() ; i++ ) 
        y.data[i] *= x; 
    return y; 
}
matrix matrix::operator / (double x) const
{
	matrix y(size());
	y.copy(data);
    for ( int i=0 ; i<size() ; i++ ) 
        y.data[i] /= x; 
    return y; 
}
bool matrix::operator == (const matrix &m) const
{
	if ( m.size()!=size() ) return false;
	if ( m.M!=M || m.N!=N ) throw "matrix::operator==(matrix): dimension mismatch";
    for ( int i=0 ; i<size() ; i++ ) 
		if ( m.data[i]!=data[i] ) return false;
    return true; 
}

vector matrix::operator * (const vector &v) const
{
	if ( v.M!=M ) throw "matrix::operator*(vector): dimension mismatch";
	vector y(N,0.0);
	for ( int r=0 ; r<M ; r++ )
		for ( int c=0 ; c<N ; c++ )
			y[c] += get(r,c) * v.data[c];
	return y;
}

double matrix::norm(const size_t n) const
{
	if ( n==1 ) // 1-norm (max of sum(abs(columns))
		throw "1-norm of matrix not implemented yet";
	else if (n==(size_t)-1) // inf-norm (max of sum(abs(rows)))
		throw "inf-norm of matrix not implemented yet";
	else if ( n>1 ) // n-norm
	{
		double y = 0.0;
		for ( size_t i=0 ; i<M*N ; i++ )
		{
			double x = data[i];
			switch (n) {
			case 4:
				x *= x;
			case 3:
				x *= x;
			case 2:
				x *= x;
				break;
			default:
				x = pow(x,(int)n);
				break;
			}
			y += x;
		}
		return y;
	}
	else
		throw "norm type not recognized";
}

bool matrix::update_inverse(double *cond)
{
#ifdef HAVE_MATLAB
	// start matlab if not already started
	char buffer[65536];
	static char output_buffer[65536];
	if ( matlab==NULL )
	{
		matlab = engOpen(matlab_command);
		if ( !matlab )
		{
			gl_error("matrix::update_inverse(): unable to connect to matlab");
			return (have_inv=false);
		}
#ifdef DUMP_MATLAB
		engOutputBuffer(matlab,output_buffer,sizeof(output_buffer));
		engSetVisible(matlab,true);
#else
		engSetVisible(matlab,false);
#endif
		sprintf(buffer,"clear all; gridlabd.version.major=%d; gridlabd.version.minor=%d; gridlabd.version.patch=%d; gridlabd.version.buildnum=%d; gridlabd.version.branch='%s'; gridlabd.module='transactive'; gridlabd.ready=true; ",REV_MAJOR,REV_MINOR,REV_PATCH,BUILDNUM,BRANCH);
		engEvalString(matlab,buffer);

	}

	// prepare pinv command
	size_t len = sprintf(buffer,"%s","pinv(");
	for ( size_t r=0 ; r<M ; r++ )
	{
		for ( size_t c=0 ; c<N ; c++ )
		{
			char d = ',';
			if ( c==0 )
			{
				if ( r==0 ) d='[';
				else d=';';
			}
			len += sprintf(buffer+len,"%c%g",d,get(r,c));
		}
	}
	len += sprintf(buffer+len,"%s","])'");
	engEvalString(matlab,buffer);
	if ( verbose_options&VO_SOLVER )
		gl_verbose( "matrix::update_inverse(): %s -> %s", buffer, output_buffer );

	// get result
	mxArray *ans = engGetVariable(matlab,"ans");
	if ( ans )
	{
		if ( inv==NULL ) inv = new matrix(N,M,mxGetPr(ans));
		else inv->copy(mxGetPr(ans));
		if ( verbose_options&VO_SOLVER )
			inv->dump(stdout,"inv");
		return (have_inv=true);
	}
	else
		return (have_inv=false);
#else
	gl_warning("lapack inverse is not stable");

	// get norm of this matrix
	if ( cond ) *cond = norm();

	// prepare solution for 0M=0
	vector y(std::max(M,N),0.0);
	vector x(std::max(M,N),0.0);
	vector work(N+M+2);
	long result;
	long lwork = (long)work;
	matrix tmp(*this);
	dgels_('N',(const long&)N,(const long&)M,1,tmp.data,(const long&)N,y,(const long&)M,(double*)work,lwork,result);
	if ( result<0 )
	{
		const char *name[] = {NULL,"TRANS","M","N","NRHS","A","LDA","B","LDB","WORK","LWORK","INFO"};
		double value[] = {NaN,NaN,(double)N,(double)M,1,NaN,(double)N,NaN,(double)M,NaN,(double)lwork,NaN};
		fprintf(stderr,"matrix::solve(x,y): bad DGELS parameter name is '%s'\n",name[-result]);
		if ( _finite(value[-result]) )
			fprintf(stderr,"matrix::solve(x,y): bad DGELS parameter value is '%lg'\n",value[-result]);
		return false;
	}

	// get norm of inverse matrix
	if ( cond ) *cond *= tmp.norm();
	if ( inv==NULL ) 
		inv = new matrix(N,M,tmp.data);
	else
		inv->copy(tmp.data);
	have_inv = true;
	return true;
#endif
}

bool matrix::solve(const vector &x, vector &y, double *cond) const
{
	if ( inv==NULL )
		throw "cannot solve before inverse is update";
#ifdef DUMP_STDOUT
	x.dump(stdout,"x");
	inv->dump(stdout,"A");
#endif
	y = x*(*inv);
#ifdef DUMP_STDOUT
	y.dump(stdout,"y");
#endif
	return true;
}

void matrix::dump(FILE *fh, const char *name, const char *fmt) const
{
	fprintf(fh,"%s = [",name);
	char indent[32]; memset(indent,' ',sizeof(indent)); indent[sizeof(indent)-1] = '\0';
	size_t len = strlen(name);
	if ( len<sizeof(indent) ) indent[len]='\0';
	for ( int r=0 ; r<M ; r++ )
	{
		if ( r>0 ) fprintf(fh,";\n    %s",indent);
		for ( int c=0 ; c<N ; c++ )
		{	
			if ( c>0 ) fputs(",",fh);
			fprintf(fh,fmt,get(r,c));
		}
	}
    fprintf(fh,"];\n");
}

