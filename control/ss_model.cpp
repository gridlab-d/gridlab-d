/** $Id: ss_model.cpp 4738 2014-07-03 00:55:39Z dchassin $

   General purpose ss_model objects

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <complex.h>

#include "ss_model.h"

EXPORT_CREATE(ss_model);
EXPORT_INIT(ss_model);
EXPORT_SYNC(ss_model);
EXPORT_PRECOMMIT(ss_model);

CLASS *ss_model::oclass = NULL;
ss_model *ss_model::defaults = NULL;

ss_model::ss_model(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"ss_model",sizeof(ss_model),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class ss_model";
		else
			oclass->trl = TRL_PRINCIPLE;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_enumeration,"form",get_form_offset(),PT_DESCRIPTION,"desired canonical controller type",
				PT_KEYWORD,"CCF",CF_CCF,
				PT_KEYWORD,"OCF",CF_OCF,
			PT_double,"timestep",get_timestep_offset(),PT_UNITS,"s",PT_DESCRIPTION,"discrete timestep",
			PT_char1024, "Y", get_Y_offset(),PT_DESCRIPTION,"transfer function numerator coefficients",
			PT_char1024, "U", get_U_offset(),PT_DESCRIPTION,"transfer function denominator coefficients",
			PT_char1024, "x", get_x_offset(),PT_DESCRIPTION,"state variable values",
			PT_char1024, "u", get_u_offset(),PT_DESCRIPTION,"ss_model variable names",
			PT_char1024, "y", get_y_offset(),PT_DESCRIPTION,"output variable names",
			PT_char1024, "H", get_H_offset(),PT_DESCRIPTION,"transfer function",
			PT_char1024, "A", get_y_offset(),PT_DESCRIPTION,"canonical A matrix",
			PT_char1024, "B", get_y_offset(),PT_DESCRIPTION,"canonical B matrix",
			PT_char1024, "C", get_y_offset(),PT_DESCRIPTION,"canonical C matrix",
			PT_char1024, "D", get_y_offset(),PT_DESCRIPTION,"canonical D matrix",

			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(ss_model));
	}
}

int ss_model::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

unsigned int ss_model::scan_values(char *input, double *output, unsigned max)
{
	unsigned int n;
	char *p;
	for ( p=input,n=0 ; *p!='\0' && n<max ; )
	{
		while ( *p!='\0' && isspace(*p) ) p++; // skip white space
		if ( *p!='\0' && sscanf(p,"%lf",&output[n++])==0 )
			break;
		while ( *p!='\0' && !isspace(*p) ) p++; // skip non-space			
	}
	return n;
}
unsigned int ss_model::scan_references(char *input, gld_property *target[], unsigned max)
{
	unsigned int n;
	char *p;
	char ref[1024];
	for ( p=input,n=0 ; *p!='\0' && n<max ; )
	{
		while ( *p!='\0' && isspace(*p) ) p++; // skip white space
		if ( *p!='\0' && sscanf(p,"%s",ref)==0 )
			break;
		char objname[64], propname[64];
		if ( sscanf(ref,"%[^.].%s",objname,propname)!=2 )
			exception("'%s' is not a valid 'object.property' reference",ref); 
		
		OBJECT *obj = gl_get_object(objname);
		if ( obj==NULL )
			exception("object '%s' is not found", objname);

		target[n] = new gld_property(obj,propname);
		if ( !target[n]->is_valid() )
			exception("property '%s.%s' is not found", objname, propname);
		if ( target[n]->get_type()!=PT_double && target[n]->get_type()!=PT_random )
			exception("property '%s.%s' is not a real number", objname, propname);
		while ( *p!='\0' && !isspace(*p) ) p++; // skip non-space	
		n++;
	}
	return n;
}

int ss_model::init(OBJECT *parent)
{
	double val[256];
	unsigned int n,m;

	// determine n by scanning the x vector
	n = scan_values(x,val,sizeof(val)/sizeof(val[0]));
	if ( n==256 )
		exception("no more than 256 states are permitted");
	else if ( n==0 )
		exception("at least one state must be defined");
	else
		dim.n = n;

	// save the x vector
	ss.x = new double[n];
	memcpy(ss.x,val,sizeof(double)*n);
	
	// scan the Y vector
	tf.Y = new double[n];
	if ( scan_values(Y,tf.Y,n)!=n )
		exception("Y does not have the same number of values as x");

	// scan the U vector
	tf.U = new double[n];
	if ( scan_values(U,tf.U,n)!=n )
		exception("Y does not have the same number of values as x");

	// scan the u references
	input = new gld_property*[dim.n];
	if ( scan_references(u,input,n)!=n )
		exception("u does not have the same number of references as x has values");

	// determine m by scanning the y vector
	output = new gld_property*[n];
	if ( (m=scan_references(y,output,n))!=1 )
		exception("y may have only one reference");
	dim.m = m;

	// construct the A matrix
	unsigned int i, j;
	ss.A = new double*[n-1];
	switch ( form ) {
	case CF_OCF:
		for ( i=0 ; i<n-1 ; i++ ) 
		{
			ss.A[i] = new double[n-1];
			memset(ss.A[i],0,sizeof(double)*(n-1));
			ss.A[i][0] = -tf.U[i+1];
			ss.A[i][i+1] = 1;
		}
		break;
	case CF_CCF:
		// TODO
		break;
	}

	// construct the B matrix
	ss.B = new double*[n-1];
	switch ( form ) {
	case CF_OCF:
		for ( i=0 ; i<n-1 ; i++ ) 
		{
			ss.B[i] = new double[m];
			memset(ss.B[i],0,sizeof(double)*m);
			for ( j=0 ; j<m ; j++ )
				// TODO generalize for MIMO
				ss.B[i][j] = tf.Y[i+1] - tf.U[i+1]*tf.Y[j];
		}
		break;
	case CF_CCF:
		// TODO
		break;
	}

	// construct the C matrix
	ss.C = new double*[m];
	switch ( form ) {
	case CF_OCF:
		for ( i=0 ; i<m ; i++ )
		{
			ss.C[i] = new double[n-1];
			memset(ss.C[i],0,sizeof(double)*(n-1));
			ss.C[i][0] = 1;
		}
		break;
	case CF_CCF:
		// TODO
		break;
	}

	ss.D = new double*[1];
	switch ( form ) {
	case CF_OCF:
		ss.D[0] = new double[1];
		// TODO generalize for MIMO
		ss.D[0][0] = tf.Y[0];
	case CF_CCF:
		// TODO
		break;
	}

	// link control vector
	ss.u = new double*[n];
	for ( i=0 ; i<n ; i++ )
		ss.u[i] = (double*)input[i]->get_addr();

	// link observation vector
	ss.y = new double*[m];
	for ( i=0 ; i<m ; i++ )
		ss.y[i] = (double*)output[i]->get_addr();

	// create xdot
	ss.xdot = new double[n];
	memset(ss.xdot,0,sizeof(ss.xdot));

	// update H string (just descriptive)
	int len=1;
	H[0]='(';
	for ( i=0 ; i<n ; i++ )
	{
		if ( tf.Y[i]!=0 )
			if ( i<n-1 )
				len += sprintf(H+len,"%+.3fs^%d",tf.Y[i],n-i-1);
			else
				len += sprintf(H+len,"%+.3f",tf.Y[i]);
	}
	len += sprintf(H+len,"%s",") / (");
	for ( i=0 ; i<n ; i++ )
	{
		if ( tf.U[i]!=0 )
			if ( i<n-1 )
				len += sprintf(H+len,"%+.3fs^%d",tf.U[i],n-i-1);
			else
				len += sprintf(H+len,"%+.3f",tf.U[i]);
	}
	len += sprintf(H+len,"%s",")");
	gl_debug("%s: H(s) = %s", get_name(), (char*)H);

	// update A,B,C,D string (descriptive)
	A[0]='['; B[0]='['; C[0]='['; D[0]='[';
	for ( len=1,i=0 ; i<n-1 ; i++ )
	{
		for ( j=0 ; j<n-1 ; j++ )
			len += sprintf(A+len," %.3f ", ss.A[i][j]);
		if ( i<n-2 )
			len += sprintf(A+len," %s",";");
	}
	strcat(A," ]");
	for ( len=1,i=0 ; i<n-1 ; i++ )
	{
		len += sprintf(B+len," %.3f ", ss.B[i][0]);
	}
	strcat(B," ]");
	for ( len=1,i=0 ; i<n-1 ; i++ )
	{
		len += sprintf(C+len," %.3f ", ss.C[0][i]);
	}
	strcat(C," ]");
	sprintf(D+1," %.2f ]",ss.D[0]);
	gl_debug("%s: A = %s", get_name(), (char*)A);
	gl_debug("%s: B = %s", get_name(), (char*)B);
	gl_debug("%s: C = %s", get_name(), (char*)C);
	gl_debug("%s: D = %s", get_name(), (char*)D);

	return 1;
}

int ss_model::precommit(TIMESTAMP t0)
{
	unsigned int i, j;

	// calculate Ax
	for ( i=0 ; i<dim.n-1 ; i++ )
	{
		ss.xdot[i] = 0;
		for ( j=0 ; j<dim.n-1 ; j++ )
			ss.xdot[i] += ss.A[i][j]*ss.x[i];
	}

	// update x
	for ( i=0 ; i<dim.n-1 ; i++ )
	{
		ss.xdot[i] += ss.B[i][0]*(*ss.u[i]);
		ss.x[i] += ss.xdot[i];
	}

	// update y
	(*ss.y[0]) = ss.C[0][0]*ss.x[0] + ss.D[0][0]*(*ss.u[0]);

	return 1;
}

TIMESTAMP ss_model::sync(TIMESTAMP t0)
{
	return (TIMESTAMP)(((t0/(int)timestep)+1)*timestep);
}

