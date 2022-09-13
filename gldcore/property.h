#ifndef _PROPERTY_H
#define _PROPERTY_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <iostream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "platform.h"
#include "gld_complex.h"
#include "unit.h"

// also in object.h
typedef struct s_class_list CLASS;

// also in class.h
#ifndef FADDR
#define FADDR
typedef int64 (*FUNCTIONADDR)(void*,...); /** the entry point of a module function */
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
typedef int8_t    int8;     /* 8-bit integers */
typedef int16_t   int16;    /* 16-bit integers */
typedef int32_t   int32;    /* 32-bit integers */
typedef uint16_t  uint16;	/* unsigned 16-bit integers */
typedef uint32_t  uint32;   /* unsigned 32-bit integers */
typedef uint64_t  uint64;   /* unsigned 64-bit integers */
#else /* no HAVE_STDINT_H */
typedef char int8; /** 8-bit integers */
typedef short int16; /** 16-bit integers */
typedef int int32; /* 32-bit integers */
typedef unsigned short uint16;
typedef unsigned int uint32; /* unsigned 32-bit integers */
typedef unsigned int64 uint64;
#endif /* HAVE_STDINT_H */

/* Valid GridLAB data types */
template<size_t size> class charbuf;

template<size_t S>
std::ostream& operator<<(std::ostream& os, const charbuf<S>& buffer)
{
	os << buffer.buffer;
	return os;
}

template<size_t size> class charbuf {
private:
	char buffer[size];
public:
	inline charbuf<size>(void) { erase(); };
	inline charbuf<size>(const char *s) { copy_from(s); };
	inline ~charbuf<size>(void) = default;
	inline size_t get_size(void) { return size; };
	inline size_t get_length(void) { return strlen(buffer); };
	inline char *get_string(void) { return buffer; };
	inline char* erase(void) { return (char*)memset(buffer,0,size); };
	inline char* copy_to(char *s) { return s?strncpy(s,buffer,size):NULL; };
	inline char* copy_from(const char *s) { return s?strncpy(buffer,s,size):NULL; };
	operator char*() { return buffer; };
	operator char() { return *buffer; };
	inline bool operator ==(const char *s) { return strcmp(buffer,s)==0; };
	//inline bool operator <(const char *s) { return strcmp(buffer,s)==-1; };
	inline bool operator <(const char *s) { return strcmp(buffer,s)<0;};
	//inline bool operator >(const char *s) { return strcmp(buffer,s)==1; };
	inline bool operator >(const char *s) { return strcmp(buffer,s)>1; };
	inline bool operator <=(const char *s) { return strcmp(buffer,s)<=0; };
	inline bool operator >=(const char *s) { return strcmp(buffer,s)>=0; };
	inline char *find(const char c) { return strchr(buffer,c); };
	inline char *find(const char *s) { return strstr(buffer,s); };
	inline char *findrev(const char c) { return strrchr(buffer,c); };
	inline char *token(char *from, const char *delim, char **context) { return this->strtok_s(from,delim,context); };
	inline size_t format(char *fmt, ...) { va_list ptr; va_start(ptr,fmt); size_t len=vsnprintf(buffer,size,fmt,ptr); va_end(ptr); return len; };
	inline size_t vformat(char *fmt, va_list ptr) { return vsnprintf(buffer,size,fmt,ptr); };

    template<size_t S>
    friend std::ostream& operator<<(std::ostream& os, const charbuf<S>& buffer);
};

typedef charbuf<2049> char2048;
typedef charbuf<1025> char1024;
typedef charbuf<257> char256;
typedef charbuf<129> char128;
typedef charbuf<65> char64;
typedef charbuf<33> char32;
typedef charbuf<9> char8;
typedef uint64 set;      /* sets (each of up to 64 values may be defined) */
typedef uint32 enumeration; /* enumerations (any one of a list of values) */
typedef struct s_object_list* object; /* GridLAB objects */
typedef double triplet[3];
typedef gld::complex triplex[3];

#define BYREF 0x01
#include <math.h>

class double_array;
class double_vector {
private:
	double **data;
public:
	explicit double_vector(double **x)
	{ 
		data=x; 
	};
	double &operator[] (const size_t n) 
	{ 
		if ( data[n]==NULL ) data[n]=new double; 
		return *data[n]; 
	};
	double operator[] (const size_t n) const
	{
		if ( data[n]==NULL ) data[n]=new double; 
		return *data[n];
	}
};
class double_array {
private:
//#else
//typedef struct s_doublearray {
//#endif
	size_t n, m; /** n rows, m cols */
	size_t max_val; /** current allocation size max x max */
	unsigned int *refs; /** reference count **/
	double ***x; /** pointer to 2D array of pointers to double values */
	unsigned char *f; /** pointer to array of flags: bit0=byref, */
	const char *name;
//#ifndef __cplusplus
//} double_array;
//#else
	friend class double_vector;
private:

	inline void exception(const char *msg,...) const
	{ 
		static char buf[1024]; 
		va_list ptr;
		va_start(ptr,msg);
		sprintf(buf,"%s", name?name:""); 
		vsprintf(buf+strlen(buf), msg, ptr); 
		throw (const char*)buf;
		va_end(ptr);
	};
	inline void set_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]|=b;};
	inline void clr_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]&=~b;};
	inline bool tst_flag(const size_t r, size_t c, const unsigned char b) const {return (f[r*m+c]&b)==b;};
	double &my(const size_t r, const size_t c) 
	{ 
		if ( x[r][c]==NULL ) x[r][c] = new double;
		return (*x[r][c]); 
	};
public:
	inline double_vector operator[] (const size_t n) { return double_vector(x[n]); }
	inline double_vector operator[] (const size_t n) const { return double_vector(x[n]); }
	double_array(const size_t rows=0, const size_t cols=0, double **data=NULL)
	{
		refs = new unsigned int;
		*refs = 0;
		n = rows;
		m = cols;
		max_val = 0;
		x = NULL;
		f = NULL;
		if ( rows>0 )
			grow_to(rows,cols);
		for ( size_t r=0 ; r<rows ; r++ )
		{
			for ( size_t c=0 ; c<cols ; c++ )
			{
				set_at(r,c, ( data!=NULL ? data[r][c] : 0.0 ) );
			}
		}
	}
	double_array(const double_array &a)
	{
		n = a.n;
		m = a.m;
		max_val = a.max_val;
		refs = a.refs;
		x = a.x;
		f = a.f;
		name = a.name;
		(*refs)++;
	}

	~double_array(void) {
        if (x != nullptr && (*refs)-- == 0) {
//            size_t r, c;
            for (auto r = 0; r < n; r++) {
                if(n > 0 && x[r] != nullptr) {
                    for (auto c = 0; c < m; c++) {
                        if (m > 0 && x[r][c] != nullptr && tst_flag(r, c, BYREF))
                            free(x[r][c]);
                    }
                    free(x[r]);
                }
            }
            free(x);
            delete refs;
        }
    }

//    ~double_array(void)
//    {
//        if ( (*refs)-- == 0 )
//        {
//            size_t r,c;
//            for ( r=0 ; r<n ; r++ )
//            {
//                for ( c=0 ; c<m ; c++ )
//                {
//                    if ( tst_flag(r,c,BYREF) )
//                        free(x[r][c]);
//                }
//                free(x[r]);
//            }
//            free(x);
//            delete refs;
//        }
//    }
public:
	void copy_name(const char *v) { char *s=(char*)malloc(strlen(v)+1); strcpy(s,v); name=(const char*)s; };

	// Getters
	inline const char *get_name(void) const { return name; };
	inline size_t get_rows(void) const { return n; };
	inline size_t get_cols(void) const { return m; };
	inline size_t get_max(void) const { return max_val; };

	// Setters
	void set_name(const char *v) { name = v; };
	inline void set_rows(const size_t i) { n=i; };
	inline void set_cols(const size_t i) { m=i; };
	void set_max(const size_t size) 
	{
		if ( size<=max_val ) exception(".set_max(%u): cannot shrink double_array",size);
		size_t r;
		auto ***z = (double***)malloc(sizeof(double**)*size);
		// create new rows
		for ( r=0 ; r<max_val ; r++ )
		{
			if ( x[r]!=NULL )
			{
				auto **y = (double**)malloc(sizeof(double*)*size);
				if ( y==NULL ) exception(".set_max(%u): unable to expand double_array",size);
				memcpy(y,x[r],sizeof(double*)*max_val);
				memset(y+max_val,0,sizeof(double*)*(size-max_val));
				free(x[r]);
				z[r] = y;
			}
			else
				z[r] = NULL;
		}
		memset(z+max_val,0,sizeof(double**)*(size-max_val));
		free(x);
		x = z;
		auto *nf = (unsigned char*)malloc(sizeof(unsigned char)*size);
		if ( f!=NULL )
		{
			memcpy(nf,f,max_val);
			memset(nf+max_val,0,size-max_val);
			free(f);
		}
		else
			memset(nf,0,size);
		f = nf;
		max_val=size;
	};
	void grow_to(const size_t r, const size_t c) 
	{ 
		size_t s = (max_val<1?1:max_val);
		while ( c>=s || r>=s ) s*=2; 
		if ( s>max_val )set_max(s);

		// add rows
		while ( n<r ) 
		{
			if ( x[n]==NULL ) 
			{
				x[n] = (double**)malloc(sizeof(double*)*max_val);
				memset(x[n],0,sizeof(double*)*max_val);
			}
			n++;
		}

		// add columns
		if ( m<c )
		{
			size_t i;
			for ( i=0 ; i<n ; i++ )
			{
				auto **y = (double**)malloc(sizeof(double*)*c);
                memset(y,0,sizeof(double*)*c);

                if ( x[i]!=NULL )
				{
					memcpy(y,x[i],sizeof(double**)*m);
					free(x[i]);
				}
				memset(y+m,0,sizeof(double**)*(c-m));
				x[i] = y;
			}
			m=c;
		}
	};
	void grow_to(const size_t c) { grow_to(n>0?n:1,c); };
	void grow_to(const double_array &y) { grow_to(y.get_rows(),y.get_cols()); };
	void check_valid(const size_t r, const size_t c) const { if ( !is_valid(r,c) ) exception(".check_value(%u,%u): invalid (r,c)",r,c); };
	inline void check_valid(const size_t c) const { check_valid(0,c); };
	bool is_valid(const size_t r, const size_t c) const { return r<n && c<m; };
	inline bool is_valid(const size_t c) const { return is_valid(0,c); };
	bool is_nan(const size_t r, const size_t c)  const
	{
		check_valid(r,c);
		return ! ( x[r][c]!=NULL && isfinite(*(x[r][c])) ); 
	};
	inline bool is_nan(const size_t c) const { return is_nan(0,c); };
	bool is_empty(void) const { return n==0 && m==0; };
	void clr_at(const size_t r, const size_t c) 
	{ 
		check_valid(r,c);
		if ( tst_flag(r,c,BYREF) )
			free(x[r][c]); 
		x[r][c]=NULL; 
	};
	inline void clr_at(const size_t c) { return clr_at(0,c); };
	/// make a new matrix (row major)
	double **copy_matrix(void) 
	{   
		auto **y = new double*[n];
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
		{
			y[r] = new double[m];
			unsigned int c;
			for ( c=0 ; c<m ; c++ )
				y[r][c] = *(x[r][c]);
		}
		return y;               
	};
	/// free a matrix
	void free_matrix(double **y)
	{
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
			delete [] y[r];
		delete [] y;
	};
	/// vector copy (row major)
	double *copy_vector(double *y=NULL)
	{
		if ( y==NULL ) y=new double[m*n];
		unsigned i=0;
		unsigned int r, c;
		for ( r=0 ; r<n ; r++ )
		{
			for ( c=0 ; c<m ; c++ )
				y[i++] = *(x[r][c]);
		}
		return y;
	}
	void transpose(void) {
		auto ***xt = new double**[n];
		size_t i;
		for ( i=0 ; i<m ; i++ )
		{
			xt[i] = new double*[n];
			size_t j;
			for ( j=0 ; j<n ; j++ )
				xt[i][j] = x[j][i];
		}
		for ( i=0 ; i<n ; i++ )
			delete [] x[i];
		delete [] x;
		x = xt;
		size_t t=m; m=n; n=t;
	};
	inline double *get_addr(const size_t r, const size_t c) { return x[r][c]; };
	inline double *get_addr(const size_t c) { return get_addr(0,c); };
	double get_at(const size_t r, const size_t c) { return is_nan(r,c) ? QNAN : *(x[r][c]) ; };
	inline double get_at(const size_t c) { return get_at(0,c); };
	inline double &get(const size_t r, const size_t c) { return *x[r][c]; };
	inline double &get(const size_t c) { return get(0,c); };
	inline void set_at(const size_t c, const double v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, const double v) 
	{ 
		check_valid(r,c);
		if ( x[r][c]==NULL ) 
			x[r][c]=(double*)malloc(sizeof(double)); 
		*(x[r][c]) = v; 
	};
	inline void set_at(const size_t c, double *v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, double *v) 
	{ 
		check_valid(r,c);
		if ( v==NULL ) 
		{
			if ( x[r][c]!=NULL ) 
				clr_at(r,c);
		}
		else 
		{
			set_flag(r,c,BYREF);
			x[r][c] = v; 
		}
	};
	void set_ident(void)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = (r==c) ? 1 : 0;
		}
	};
	void dump(size_t r1=0, size_t r2=-1, size_t c1=0, size_t c2=-1)
	{
		if ( r2==-1 ) r2 = n-1;
		if ( c2==-1 ) c2 = m-1;
		if ( r2<r1 || c2<c1 ) exception(".dump(%u,%u,%u,%u): invalid (r,c)", r1,r2,c1,c2);
		size_t r,c;
		fprintf(stderr,"double_array %s = {\n",name?name:"unnamed"); 
		for ( r=r1 ; r<=n ; r++ )
		{
			for ( c=c1 ; c<=m ; c++ )
				fprintf(stderr," %8g", my(r,c));
			fprintf(stderr,"\n");
		}
		fprintf(stderr," }\n");
	}
    double_array &operator= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = y;
		}
		return *this;
	};
	double_array &operator= (const double_array &y) // TODO: fix self-assignment to be C++-correct
	{
		size_t r,c;
		grow_to(y);
		for ( r=0 ; r<y.get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
				my(r,c) = y[r][c];
		}
		return *this;
	};
	double_array &operator+= (const double &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y;
		}
		return *this;
	}
	double_array &operator+= (const double_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y[r][c];
		}
		return *this;
	};
	double_array &operator-= (const double &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y;
		}
		return *this;
	};
	double_array &operator-= (const double_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y[r][c];
		}
		return *this;
	};
	double_array &operator *= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) *= y;
		}
		return *this;
	};
	double_array &operator /= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) /= y;
		}
		return *this;
	};
	// binary operators
	double_array operator+ (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) + y;
		return a;
	}
	double_array operator- (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) - y;
		return a;
	}
	double_array operator* (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) * y;
		return a;
	}
	double_array operator/ (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) / y;
		return a;
	}
	double_array operator + (const double_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("+%s: size mismatch",y.get_name());
		double_array a(get_rows(),get_cols());
		a.set_name("(?+?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) + y[r][c];
		return a;
	};
	double_array operator - (const double_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("-%s: size mismatch",y.get_name());
		double_array a(get_rows(),get_cols());
		a.set_name("(?-?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) - y[r][c];
		return a;
	};
	double_array operator * (const double_array &y)
	{
		size_t r,c,k;
		if ( get_cols()!=y.get_rows() )
			exception("*%s: size mismatch",y.get_name());
		double_array a(get_rows(),y.get_cols());
		a.set_name("(?*?)");
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
			{	
				double b = 0;
				for ( k=0 ; k<get_cols() ; k++ )
					b += my(r,k) * y[k][c];
				a[r][c] = b;
			}
		}
		return a;
	};
	void extract_row(double*y,const size_t r)
	{
		size_t c;
		for ( c=0 ; c<m ; c++ )
			y[c] = my(r,c);
	}
	void extract_col(double*y,const size_t c)
	{
		size_t r;
		for ( r=0 ; r<n ; r++ )
			y[r] = my(r,c);
	}
};
//#endif

//#ifdef __cplusplus
class complex_array;
class complex_vector {
private:
	gld::complex **data;
public:
	complex_vector(gld::complex **x)
	{
		data=x;
	};
	gld::complex &operator[] (const size_t n)
	{
		if ( data[n]==NULL ) data[n]=new gld::complex;
		return *data[n];
	};
	const gld::complex operator[] (const size_t n) const
	{
		if ( data[n]==NULL ) data[n]=new gld::complex;
		return *data[n];
	}
};
class complex_array {
private:
//#endif
	size_t n, m;
	size_t max_val; /** current allocation size max_val x max_val */
	unsigned int *refs; /** reference count **/
	gld::complex ***x; /** pointer to 2D array of pointers to complex values */
	unsigned char *f; /** pointer to array of flags: bit0=byref, */
	const char *name;
//#ifndef __cplusplus
//} complex_array;
//#else
	friend class complex_vector;
private:

	inline void exception(const char *msg,...) const
	{
		static char buf[1024];
		va_list ptr;
		va_start(ptr,msg);
		sprintf(buf,"%s", name?name:"");
		vsprintf(buf+strlen(buf), msg, ptr);
		throw (const char*)buf;
		va_end(ptr);
	};

	inline void set_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]|=b;};
	inline void clr_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]&=~b;};
	inline bool tst_flag(const size_t r, size_t c, const unsigned char b) const {return (f[r*m+c]&b)==b;};
	gld::complex &my(const size_t r, const size_t c)
	{
		if ( x[r][c]== nullptr) x[r][c] = new gld::complex;
		return (*x[r][c]);
	};
public:
	inline complex_vector operator[] (const size_t n) { return complex_vector(x[n]); }
	inline const complex_vector operator[] (const size_t n) const { return complex_vector(x[n]); }
	complex_array(const size_t rows=0, const size_t cols=0, gld::complex**data=NULL)
	{
		refs = new unsigned int;
		*refs = 0;
		n = rows;
		m = cols;
		max_val = 0;
		x = NULL;
		f = NULL;
		if ( rows>0 )
			grow_to(rows,cols);
		for ( size_t r=0 ; r<rows ; r++ )
		{
			for ( size_t c=0 ; c<cols ; c++ )
			{
				set_at(r,c, ( data!=NULL ? data[r][c] : 0.0 ) );
			}
		}
	}
	complex_array(const complex_array &a)
	{
		n = a.n;
		m = a.m;
		max_val = a.max_val;
		refs = a.refs;
		x = a.x;
		f = a.f;
		name = a.name;
		(*refs)++;
	}
	~complex_array(void)
	{
		if ( (*refs)-- == 0 )
		{
			size_t r,c;
			for ( r=0 ; r<n ; r++ )
			{
				for ( c=0 ; c<m ; c++ )
				{
					if ( tst_flag(r,c,BYREF) )
						free(x[r][c]);
				}
				free(x[r]);
			}
			free(x);
			delete refs;
		}
	}
public:
	void copy_name(const char *v) { char *s=(char*)malloc(strlen(v)+1); strcpy(s,v); name=(const char*)s; };

	// Getters
	inline const char *get_name(void) const { return name; };
	inline size_t get_rows(void) const { return n; };
	inline size_t get_cols(void) const { return m; };
	inline size_t get_max(void) const { return max_val; };

	// Setters
	void set_name(const char *v) { name = v; };
	inline void set_rows(const size_t i) { n=i; };
	inline void set_cols(const size_t i) { m=i; };
	void set_max(const size_t size)
	{
		if ( size<=max_val ) exception(".set_max(%u): cannot shrink complex_array",size);
		size_t r;
		auto ***z = (gld::complex***)malloc(sizeof(gld::complex**)*size);
		// create new rows
		for ( r=0 ; r<max_val ; r++ )
		{
			if ( x[r]!=NULL )
			{
				auto **y = (gld::complex**)malloc(sizeof(gld::complex*)*size);
				if ( y==NULL ) exception(".set_max(%u): unable to expand complex_array",size);
				memcpy(y,x[r],sizeof(gld::complex*)*max_val);
				memset(y+max_val,0,sizeof(gld::complex*)*(size-max_val));
				free(x[r]);
				z[r] = y;
			}
			else
				z[r] = NULL;
		}
		memset(z+max_val,0,sizeof(gld::complex**)*(size-max_val));
		free(x);
		x = z;
		auto *nf = (unsigned char*)malloc(sizeof(unsigned char)*size);
		if ( f!=NULL )
		{
			memcpy(nf,f,max_val);
			memset(nf+max_val,0,size-max_val);
			free(f);
		}
		else
			memset(nf,0,size);
		f = nf;
		max_val=size;
	};
	void grow_to(const size_t r, const size_t c)
	{ 
		size_t s = (max_val<1?1:max_val);
		while ( c>=s || r>=s ) s*=2; 
		if ( s>max_val )set_max(s);

		// add rows
		while ( n<r )
		{
			if ( x[n]==NULL ) 
			{
				x[n] = (gld::complex**)malloc(sizeof(gld::complex*)*max_val);
				memset(x[n],0,sizeof(gld::complex*)*max_val);
			}
			n++;
		}

		// add columns
		if ( m<c )
		{
			size_t i;
			for ( i=0 ; i<n ; i++ )
			{
				auto **y = (gld::complex**)malloc(sizeof(gld::complex*)*c);
				if ( x[i]!=NULL )
				{
					memcpy(y,x[i],sizeof(gld::complex**)*m);
					free(x[i]);
				}
				memset(y+m,0,sizeof(gld::complex**)*(c-m));
				x[i] = y;
			}
			m=c;
		}
	};
	void grow_to(const size_t c) { grow_to(n>0?n:1,c); };
	void grow_to(const complex_array &y) { grow_to(y.get_rows(),y.get_cols()); };
	void check_valid(const size_t r, const size_t c) const { if ( !is_valid(r,c) ) exception(".check_value(%u,%u): invalid (r,c)",r,c); };
	inline void check_valid(const size_t c) const { check_valid(0,c); };
	bool is_valid(const size_t r, const size_t c) const { return r<n && c<m; };
	inline bool is_valid(const size_t c) const { return is_valid(0,c); };
	bool is_nan(const size_t r, const size_t c)  const
	{
		check_valid(r,c);
		return ! ( x[r][c]!=NULL && isfinite(x[r][c]->Re()) && isfinite(x[r][c]->Im()) ); 
	};
	inline bool is_nan(const size_t c) const { return is_nan(0,c); };
	bool is_empty(void) const { return n==0 && m==0; };
	void clr_at(const size_t r, const size_t c)
	{ 
		check_valid(r,c);
		if ( tst_flag(r,c,BYREF) )
			free(x[r][c]); 
		x[r][c]=NULL; 
	};
	inline void clr_at(const size_t c) { return clr_at(0,c); };
	/// make a new matrix (row major)
	gld::complex **copy_matrix(void)
	{
		gld::complex **y = new gld::complex*[n];
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
		{
			y[r] = new gld::complex[m];
			unsigned int c;
			for ( c=0 ; c<m ; c++ )
				y[r][c] = *(x[r][c]);
		}
		return y;
	};
	/// free a matrix
	void free_matrix(gld::complex **y)
	{
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
			delete [] y[r];
		delete [] y;
	};
	/// vector copy (row major)
	gld::complex *copy_vector(gld::complex *y=NULL)
	{
		if ( y==NULL ) y=new gld::complex[m*n];
		unsigned i=0;
		unsigned int r, c;
		for ( r=0 ; r<n ; r++ )
		{
			for ( c=0 ; c<m ; c++ )
				y[i++] = *(x[r][c]);
		}
		return y;
	}
	void transpose(void) {
		gld::complex ***xt = new gld::complex**[n];
		size_t i;
		for ( i=0 ; i<m ; i++ )
		{
			xt[i] = new gld::complex*[n];
			size_t j;
			for ( j=0 ; j<n ; j++ )
				xt[i][j] = x[j][i];
		}
		for ( i=0 ; i<n ; i++ )
			delete [] x[i];
		delete [] x;
		x = xt;
		size_t t=m; m=n; n=t;
	};
	inline gld::complex *get_addr(const size_t r, const size_t c) { return x[r][c]; };
	inline gld::complex *get_addr(const size_t c) { return get_addr(0,c); };
	gld::complex get_at(const size_t r, const size_t c) { return is_nan(r,c) ? QNAN : *(x[r][c]) ; };
	inline gld::complex get_at(const size_t c) { return get_at(0,c); };
	inline gld::complex &get(const size_t r, const size_t c) { return *x[r][c]; };
	inline gld::complex &get(const size_t r, const size_t c) const { return *x[r][c]; };
	inline gld::complex &get(const size_t c) { return get(0,c); };
	inline void set_at(const size_t c, const gld::complex v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, const gld::complex v)
	{ 
		check_valid(r,c);
		if ( x[r][c]==NULL ) 
			x[r][c]=(gld::complex*)malloc(sizeof(gld::complex));
		*(x[r][c]) = v; 
	};
	inline void set_at(const size_t c, gld::complex *v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, gld::complex *v)
	{ 
		check_valid(r,c);
		if ( v==NULL ) 
		{
			if ( x[r][c]!=NULL ) 
				clr_at(r,c);
		}
		else 
		{
			set_flag(r,c,BYREF);
			x[r][c] = v; 
		}
	};
	void set_ident(void)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = (r==c) ? 1 : 0;
		}
	};
	void dump(size_t r1=0, size_t r2=-1, size_t c1=0, size_t c2=-1)
	{
		if ( r2==-1 ) r2 = n-1;
		if ( c2==-1 ) c2 = m-1;
		if ( r2<r1 || c2<c1 ) exception(".dump(%u,%u,%u,%u): invalid (r,c)", r1,r2,c1,c2);
		size_t r,c;
		fprintf(stderr,"complex_array %s = {\n",name?name:"unnamed");
		for ( r=r1 ; r<=n ; r++ )
		{
			for ( c=c1 ; c<=m ; c++ )
				fprintf(stderr," %8g%+8gi", my(r,c).Re(), my(r,c).Im());
			fprintf(stderr,"\n");
		}
		fprintf(stderr," }\n");
	}
	void operator= (const gld::complex y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = y;
		}
	};
	complex_array &operator= (const complex_array &y)
	{
		size_t r,c;
		grow_to(y);
		for ( r=0 ; r<y.get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
				my(r,c) = y[r][c];
		}
		return *this;
	};
	complex_array &operator+= (const gld::complex &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y;
		}
		return *this;
	}
	complex_array &operator+= (const complex_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y[r][c];
		}
		return *this;
	};
	complex_array &operator-= (const gld::complex &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y;
		}
		return *this;
	};
	complex_array &operator-= (const complex_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y[r][c];
		}
		return *this;
	};
	complex_array &operator *= (const gld::complex y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) *= y;
		}
		return *this;
	};
	complex_array &operator /= (const gld::complex y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) /= y;
		}
		return *this;
	};
	// binary operators
	complex_array operator+ (const gld::complex y)
	{
		complex_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) + y;
		return a;
	}
	complex_array operator- (const gld::complex y)
	{
		complex_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) - y;
		return a;
	}
	complex_array operator* (const gld::complex y)
	{
		complex_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) * y;
		return a;
	}
	complex_array operator/ (const gld::complex y)
	{
		complex_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) / y;
		return a;
	}
	complex_array operator + (const complex_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("+%s: size mismatch",y.get_name());
		complex_array a(get_rows(),get_cols());
		a.set_name("(?+?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) + y[r][c];
		return a;
	};
	complex_array operator - (const complex_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("-%s: size mismatch",y.get_name());
		complex_array a(get_rows(),get_cols());
		a.set_name("(?-?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) - y[r][c];
		return a;
	};
	complex_array operator * (const complex_array &y)
	{
		size_t r,c,k;
		if ( get_cols()!=y.get_rows() )
			exception("*%s: size mismatch",y.get_name());
		complex_array a(get_rows(),y.get_cols());
		a.set_name("(?*?)");
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
			{
				gld::complex b = 0;
				for ( k=0 ; k<get_cols() ; k++ )
					b += my(r,k) * y[k][c];
				a[r][c] = b;
			}
		}
		return a;
	};
	void extract_row(gld::complex*y,const size_t r)
	{
		size_t c;
		for ( c=0 ; c<m ; c++ )
			y[c] = my(r,c);
	}
	void extract_col(gld::complex*y,const size_t c)
	{
		size_t r;
		for ( r=0 ; r<n ; r++ )
			y[r] = my(r,c);
	}
};
//#endif

/* ADD NEW CORE TYPES HERE */

#ifdef REAL4
typedef float real; 
#else
typedef double real;
#endif

//#ifndef __cplusplus
//#ifndef true
//typedef unsigned char bool;
//#define true (1)
//#define false (0)
//#endif
//#endif

/* delegated types allow module to keep all type operations private
 * this includes convert operations and allocation/deallocation
 */
typedef struct s_delegatedtype
{
	char32 type; /**< the name of the delegated type */
	CLASS *oclass; /**< the class implementing the delegated type */
	int (*from_string)(void *addr, const char *value); /**< the function that converts from a string to the data */
	int (*to_string)(void *addr, char *value, int size); /**< the function that converts from the data to a string */
} DELEGATEDTYPE; /**< type delegation specification */
typedef struct s_delegatedvalue
{
	char *data; /**< the data that is delegated */
	DELEGATEDTYPE *type; /**< the delegation specification to use */
} DELEGATEDVALUE; /**< a delegation entry */
typedef DELEGATEDVALUE* delegated; /* delegated data type */

/* int64 is already defined in platform.h */
typedef enum {_PT_FIRST=-1, 
	PT_void, /**< the type has no data */
	PT_double, /**< the data is a double-precision float */
	PT_complex, /**< the data is a complex value */
	PT_enumeration, /**< the data is an enumeration */
	PT_set, /**< the data is a set */
	PT_int16, /**< the data is a 16-bit integer */
	PT_int32, /**< the data is a 32-bit integer */
	PT_int64, /**< the data is a 64-bit integer */
	PT_char8, /**< the data is \p NULL -terminated string up to 8 characters in length */
	PT_char32, /**< the data is \p NULL -terminated string up to 32 characters in length */ 
	PT_char256, /**< the data is \p NULL -terminated string up to 256 characters in length */
	PT_char1024, /**< the data is \p NULL -terminated string up to 1024 characters in length */
	PT_object, /**< the data is a pointer to a GridLAB object */
	PT_delegated, /**< the data is delegated to a module for implementation */
	PT_bool, /**< the data is a true/false value, implemented as a C++ bool */
	PT_timestamp, /**< timestamp value */
	PT_double_array, /**< the data is a fixed length double[] */
	PT_complex_array, /**< the data is a fixed length complex[] */
/*	PT_object_array, */ /**< the data is a fixed length array of object pointers*/
	PT_real,	/**< Single or double precision float ~ allows double values to be overriden */
	PT_float,	/**< Single-precision float	*/
	PT_loadshape,	/**< Loadshapes are state machines driven by schedules */
	PT_enduse,		/**< Enduse load data */
	PT_random,		/**< Randomized number */
	PT_method,		/**< Method */
	/* add new property types here - don't forget to add them also to rt/gridlabd.h and property.c */
#ifdef USE_TRIPLETS
	PT_triple, /**< triplet of doubles (not supported) */
	PT_triplex, /**< triplet of complexes (not supported) */
#endif
	_PT_LAST, 
	/* never put these before _PT_LAST they have special uses */
	PT_AGGREGATE, /* internal use only */
	PT_KEYWORD, /* used to add an enum/set keyword definition */
	PT_ACCESS, /* used to specify property access rights */
	PT_SIZE, /* used to setup arrayed properties */
	PT_FLAGS, /* used to indicate property flags next */
	PT_INHERIT, /* used to indicate that properties from a parent class are to be published */
	PT_UNITS, /* used to indicate that property has certain units (which following immediately as a string) */
	PT_DESCRIPTION, /* used to provide helpful description of property */
	PT_EXTEND, /* used to enlarge class size by the size of the current property being mapped */
	PT_EXTENDBY, /* used to enlarge class size by the size provided in the next argument */
	PT_DEPRECATED, /* used to flag a property that is deprecated */
	PT_HAS_NOTIFY, /* used to indicate that a notify function exists for the specified property */
	PT_HAS_NOTIFY_OVERRIDE, /* as PT_HAS_NOTIFY, but instructs the core not to set the property to the value being set */
} PROPERTYTYPE; /**< property types */
typedef char CLASSNAME[64]; /**< the name of a GridLAB class */
typedef void* PROPERTYADDR; /**< the offset of a property from the end of the OBJECT header */
typedef char PROPERTYNAME[64]; /**< the name of a property */
typedef char FUNCTIONNAME[64]; /**< the name of a function (not used) */

/* property access rights (R/W apply to modules only, core always has all rights) */
#define PA_N 0x00 /**< no access permitted */
#define PA_R 0x01 /**< read access--modules can read the property */
#define PA_W 0x02 /**< write access--modules can write the property */
#define PA_S 0x04 /**< save access--property is saved to output */
#define PA_L 0x08 /**< load access--property is loaded from input */
#define PA_H 0x10 /**< hidden access--property is not revealed by modhelp */
typedef enum {
	PA_PUBLIC = (PA_R|PA_W|PA_S|PA_L), /**< property is public (readable, writable, saved, and loaded) */
	PA_REFERENCE = (PA_R|PA_S|PA_L), /**< property is FYI (readable, saved, and loaded */
	PA_PROTECTED = (PA_R), /**< property is semipublic (readable, but not saved or loaded) */
	PA_PRIVATE = (PA_S|PA_L), /**< property is nonpublic (not accessible, but saved and loaded) */
	PA_HIDDEN = (PA_PUBLIC|PA_H), /**< property is not visible  */
} PROPERTYACCESS; /**< property access rights */

typedef struct s_keyword {
	char name[32];
	uint64 value;
	struct s_keyword *next;
} KEYWORD;

typedef int (*METHODCALL)(void *obj, char *string, int size); /**< the function that read and writes a string */

typedef uint32 PROPERTYFLAGS;
#define PF_RECALC	0x0001 /**< property has a recalc trigger (only works if recalc_<class> is exported) */
#define PF_CHARSET	0x0002 /**< set supports single character keywords (avoids use of |) */
#define PF_EXTENDED 0x0004 /**< indicates that the property was added at runtime */
#define PF_DEPRECATED 0x8000 /**< set this flag to indicate that the property is deprecated (warning will be displayed anytime it is used */
#define PF_DEPRECATED_NONOTICE 0x04000 /**< set this flag to indicate that the property is deprecated but no reference warning is desired */

typedef struct s_property_map {
	CLASS *oclass; /**< class implementing the property */
	PROPERTYNAME name; /**< property name */
	PROPERTYTYPE ptype; /**< property type */
	uint32 size; /**< property array size */
	uint32 width; /**< property byte size, copied from array in class.c */
	PROPERTYACCESS access; /**< property access flags */
	UNIT *unit; /**< property unit, if any; \p NULL if none */
	PROPERTYADDR addr; /**< property location, offset from OBJECT header; OBJECT header itself for methods */
	DELEGATEDTYPE *delegation; /**< property delegation, if any; \p NULL if none */
	KEYWORD *keywords; /**< keyword list, if any; \p NULL if none (only for set and enumeration types)*/
	const char *description; /**< description of property */
	struct s_property_map *next; /**< next property in property list */
	PROPERTYFLAGS flags; /**< property flags (e.g., PF_RECALC) */
	FUNCTIONADDR notify;
	METHODCALL method; /**< method call, addr must be 0 */
	bool notify_override;
} PROPERTY; /**< property definition item */

typedef struct s_property_struct {
	PROPERTY *prop;
	PROPERTYNAME part;
} PROPERTYSTRUCT;

/** Property comparison operators
 **/
typedef enum { 
	TCOP_EQ=0, /**< property are equal to a **/
	TCOP_LE=1, /**< property is less than or equal to a **/
	TCOP_GE=2, /**< property is greater than or equal a **/
	TCOP_NE=3, /**< property is not equal to a **/
	TCOP_LT=4, /**< property is less than a **/
	TCOP_GT=5, /**< property is greater than a **/
	TCOP_IN=6, /**< property is between a and b (inclusive) **/
	TCOP_NI=7, /**< property is not between a and b (inclusive) **/
	_TCOP_LAST,
	TCOP_NOP,
	TCOP_ERR=-1
} PROPERTYCOMPAREOP;
typedef int PROPERTYCOMPAREFUNCTION(void*,void*,void*);

typedef struct s_property_specs { /**<	the property type conversion specifications.
								It is critical that the order of entries in this list must match 
								the order of entries in the enumeration #PROPERTYTYPE 
						  **/
	const char *name; /**< the property type name */
	const char *xsdname;
	unsigned int size; /**< the size of 1 instance */
	unsigned int csize; /**< the minimum size of a converted instance (not including '\0' or unit, 0 means a call to property_minimum_buffersize() is necessary) */ 
	int (*data_to_string)(char *,int,void*,PROPERTY*); /**< the function to convert from data to a string */
	int (*string_to_data)(const char *,void*,PROPERTY*); /**< the function to convert from a string to data */
	int (*create)(void*); /**< the function used to create the property, if any */
	size_t (*stream)(FILE*,int,void*,PROPERTY*); /**< the function to read data from a stream */
	struct {
		PROPERTYCOMPAREOP op;
		char str[16];
		PROPERTYCOMPAREFUNCTION* fn;
		int trinary;
	} compare[_TCOP_LAST]; /**< the list of comparison operators available for this type */
	double (*get_part)(void*,const char *name); /**< the function to get a part of a property */
	// @todo for greater generality this should be implemented as a linked list
} PROPERTYSPEC;

int property_check(void);
PROPERTYSPEC *property_getspec(PROPERTYTYPE ptype);
PROPERTY *property_malloc(PROPERTYTYPE, CLASS *, char *, void *, DELEGATEDTYPE *);
uint32 property_size(PROPERTY *);
uint32 property_size_by_type(PROPERTYTYPE);
size_t property_minimum_buffersize(PROPERTY *);
int property_create(PROPERTY *, void *);
bool property_compare_basic(PROPERTYTYPE ptype, PROPERTYCOMPAREOP op, void *x, void *a, void *b, char *part);
PROPERTYCOMPAREOP property_compare_op(PROPERTYTYPE ptype, char *opstr);
PROPERTYTYPE property_get_type(char *name);
double property_get_part(struct s_object_list *obj, PROPERTY *prop, const char *part);

/* double array */
int double_array_create(double_array &a);
//double get_double_array_value(double_array*,unsigned int n, unsigned int m);
//void set_double_array_value(double_array*,unsigned int n, unsigned int m, double x);
//double *get_double_array_ref(double_array*,unsigned int n, unsigned int m);
double double_array_get_part(void *x, const char *name);

/* complex array */
int complex_array_create(complex_array &a);
//gld::complex *get_complex_array_value(complex_array*,unsigned int n, unsigned int m);
//void set_complex_array_value(complex_array*,unsigned int n, unsigned int m, gld::complex *x);
//gld::complex *get_complex_array_ref(complex_array*,unsigned int n, unsigned int m);
double complex_array_get_part(void *x, const char *name);

inline PROPERTYTYPE &operator++(PROPERTYTYPE &d){	return d = PROPERTYTYPE(d + 1);}

#endif //_PROPERTY_H

// EOF
