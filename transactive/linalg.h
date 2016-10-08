// $Id: linalg.h 5063 2015-01-28 19:45:37Z dchassin $

#ifndef _LINALG_H
#define _LINALG_H

#include <stdio.h>
#include <string.h>

#include "gridlabd.h"

typedef enum {
	RD_UNIFORM = 0,
	RD_NORMAL = 1,
} DISTRIBUTION;

class matrix;
class vector
{
	friend class matrix;
private:
	size_t M; // size of vector
	size_t size; // size of data buffer
	double *data; // data buffer
public:
	vector();
	vector(const size_t size, double init=NaN);
	vector(const size_t size, DISTRIBUTION rd, double a, double b);
	vector(const vector &v);
	~vector(void);
public:
	inline operator size_t (void) { return M; };
	inline operator double* (void) { return data; };
//	inline double &operator [] (int i) { return data[i];}; 
	inline void copy(const vector &x);
	inline vector &operator = (const vector &v) { copy(v); return *this; };
	// unary operator
	vector operator - (void) const;
	// compare operators
	bool operator == (const vector &v) const;
	// scalar operators
	vector operator + (const double x) const;
	vector operator - (const double x) const;
	vector operator * (const double x) const;
	vector operator / (const double x) const;
	// vector operators
	vector operator + (const vector &v) const; // sum
	vector operator - (const vector &v) const; // difference
	double operator * (const vector &v) const; // dot product
	// matrix operators
	vector operator * (matrix &m) const; // left row product v*M
	vector operator / (const matrix &m) const; // solve v/M
	// special operators
	double sum(void) const;
	double norm(const unsigned int n=1) const; 
	double maximum(void) const;
	double minimum(void) const;
	vector range(void) const;
	void dump(FILE *fh=stdout,const char *name="?",const char *fmt="%8.4f") const;
};

class matrix  
{ 
	friend class vector;
private: 
    size_t M,N; // row/col dimensions of matrix
    double *data;
	bool have_inv; // flag to indicate use of precomputed inverse is desired
	class matrix *inv; // inverse of matrix to be used by solver
	double cond;
public:
	matrix();
    matrix(const size_t size, double diag=NaN);
	matrix(const size_t size, DISTRIBUTION rd, double a, double b);
	matrix(const size_t rows, const size_t cols, double init=NaN);
	matrix(const size_t rows, const size_t cols, DISTRIBUTION rd, double a, double b);
	matrix(const size_t rows, const size_t cols, double *data);
	matrix(const matrix &m);
	~matrix(void);
public: 
	inline void copy(double*p, size_t n=0) { memcpy(data,p,sizeof(double)*(n>0?n:M*N)); }
	inline size_t size(void) const { return M*N;};
	inline size_t ndx(const size_t r, const size_t c) const { return r*N+c;};
	inline double get(const size_t r, const size_t c) const { return data[ndx(r,c)];};
	inline double set(const size_t r, const size_t c, const double x) { return data[ndx(r,c)] = x;};
	inline double* operator[] (size_t row) { return data+row*N; };
	matrix &operator = (const matrix &m);
	// compare operators
	bool operator == (const matrix &m) const;
	// unary operators
	matrix operator - (void) const; ///< unary sign change
	matrix operator ~ (void) const; ///< tranpose
	// scalar operators
	matrix operator + (const double x) const; ///< scalar addition
	matrix operator - (const double x) const; ///< scalar subtraction
	matrix operator * (const double x) const; ///< scalar multiplication
	matrix operator / (const double x) const; ///< scalar division
	// vector operators
	vector operator * (const vector &v) const; ///< right column product M*v
	// special operations
	bool update_inverse(double *cond=NULL);
	inline bool check_inverse(double *cond=NULL) { return have_inv || update_inverse(cond); };
	bool solve(const vector &x, vector &y, double *cond=NULL) const; ///< inverse
	double norm(const size_t n=2) const; ///< norm
	void dump(FILE *fh=stdout,const char *name="?",const char *fmt="%8.4f") const;
};

#endif // _LINALG_H
