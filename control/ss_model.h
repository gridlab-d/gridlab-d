/** $Id: ss_model.h 4738 2014-07-03 00:55:39Z dchassin $

 General purpose assert objects

 **/

#ifndef _SSMODEL_H
#define _SSMODEL_H

#include "gridlabd.h"

class ss_model : public gld_object {
public:
	typedef enum {
		CF_CCF=0, // controllable form
		CF_OCF=1, // observable form
	} CANONICALFORM;

	GL_ATOMIC(CANONICALFORM,form); 
	GL_ATOMIC(double,timestep);
	GL_STRING(char1024,Y);
	GL_STRING(char1024,U);
	GL_STRING(char1024,x);
	GL_STRING(char1024,u);
	GL_STRING(char1024,y);

	GL_STRING(char1024,H);
	GL_STRING(char1024,A);
	GL_STRING(char1024,B);
	GL_STRING(char1024,C);
	GL_STRING(char1024,D);

	GL_STRUCT(double_array,Q);

private:
	struct s_dimensions {
		unsigned int n; // size of x
		unsigned int m; // size of y
	} dim;
	struct s_statespace {
		double *x; // state vector (order n)
		double *xdot; // state change (order n)
		double **u; // ss_model vector (order n)
		double **y; // observation vector (order m)
		double **A; // A matrix (n x n)
		double **B; // B matrix (m x n)
		double **C; // C matrix (n x m)
		double **D; // D matrix (m x m)
	} ss;
	struct s_transferfunction {
		double *Y; // numerator (order n)
		double *U; // denominator (order n)
	} tf;
	gld_property **input; // input references
	gld_property **output; // output references
public:
	/* required implementations */
	ss_model(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int precommit(TIMESTAMP t0);
	inline TIMESTAMP presync(TIMESTAMP t0) { return TS_NEVER; };
	TIMESTAMP sync(TIMESTAMP t0);
	inline TIMESTAMP postsync(TIMESTAMP t0) { return TS_NEVER; };

private:
	unsigned int scan_values(char *input, double *output, unsigned max);
	unsigned int scan_references(char *input, gld_property *output[], unsigned max);

public:
	static CLASS *oclass;
	static ss_model *defaults;
};
#endif // _ASSERT_H