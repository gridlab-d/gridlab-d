// line_sensor.cpp
// Copyright (C) 2016, Stanford University
// Author: David P. Chassin (dchassin@slac.stanford.edu)
//
//   General purpose line_sensor objects
//
//

#include "gismo.h"

EXPORT_CREATE(line_sensor);
EXPORT_INIT(line_sensor);
EXPORT_SYNC(line_sensor);

CLASS *line_sensor::oclass = NULL;
line_sensor *line_sensor::defaults = NULL;

line_sensor::line_sensor(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"line_sensor",sizeof(line_sensor),PC_POSTTOPDOWN|PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class line_sensor";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_enumeration,"measured_phase",get_measured_phase_offset(), PT_DESCRIPTION,"phase from which measurement is taken",
				PT_KEYWORD, "A", (enumeration)PHASE_A,
				PT_KEYWORD, "B", (enumeration)PHASE_B,
				PT_KEYWORD, "C", (enumeration)PHASE_C,
			PT_complex,"measured_voltage[V]",get_measured_voltage_offset(), PT_DESCRIPTION,"voltage measurement (with noise)",
			PT_complex,"measured_current[A]",get_measured_current_offset(), PT_DESCRIPTION,"current measurement (with noise)",
			PT_complex,"measured_power[VA]",get_measured_power_offset(), PT_DESCRIPTION,"power measurement (with noise)",
			PT_double,"location[ft]",get_location_offset(), PT_DESCRIPTION,"sensor placement downline",
			PT_double_array,"covariance", get_covariance_offset(), PT_DESCRIPTION,"sensor covariance matrix (components are [Vr Vi Ir Ii])",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(line_sensor));
	}
}

int line_sensor::create(void)
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

// Perform cholesky decomposition of a PSD array
//
// Array A is transformed so that lower left block is L
// while upper right block is original A.
//
// Diagonal D is also returned
//
static void cholesky(size_t N, ///< rows of array A
                     double *A, ///< input PSD array A (size NxN)
                     double *D) ///< input working space for diagonal vector (size N)
{
	size_t col, row, n;
	for ( row = 0 ; row < N ; row++ )
		D[row] = A[(N+1)*row];
	for ( row = 0 ; row < N ; row++ )
	{
		for ( n = 0 ; n < row ; n++ )
			D[row] -= A[N*n+row] * A[N*n+row];
		D[row] = sqrt(D[row]);
		for ( col = row + 1 ; col < N ; col++ )
		{
			for ( n = 0 ; n < row ; n++ )
				A[N*row+col] -= A[N*n+col] * A[N*n+row];
			A[N*row+col] /= D[row];
		}
	}
}

int line_sensor::init(OBJECT *parent)
{
	if ( get_parent()==NULL )
		exception("parent target must be specified");

	if ( !get_parent()->isa("line") || strcmp(get_parent()->get_oclass()->get_module()->get_name(),"powerflow")!=0 )
		exception("parent '%s' must be a powerflow line object", get_parent()->get_name());

	from_current_A = gld_property(get_parent(),"current_in_A");
	from_current_B = gld_property(get_parent(),"current_in_B");
	from_current_C = gld_property(get_parent(),"current_in_C");
	if ( !from_current_A.is_valid() ) exception("parent line property 'current_in_A' not found");
	if ( !from_current_B.is_valid() ) exception("parent line property 'current_in_B' not found");
	if ( !from_current_C.is_valid() ) exception("parent line property 'current_in_C' not found");

	to_current_A = gld_property(get_parent(),"current_out_A");
	to_current_B = gld_property(get_parent(),"current_out_B");
	to_current_C = gld_property(get_parent(),"current_out_C");
	if ( !to_current_A.is_valid() ) exception("parent line property 'current_out_A' not found");
	if ( !to_current_B.is_valid() ) exception("parent line property 'current_out_B' not found");
	if ( !to_current_C.is_valid() ) exception("parent line property 'current_out_C' not found");

	gld_property from(get_parent(),"from");
	gld_object *pFrom = from.get_objectref();
	if ( !pFrom->isa("node") ) exception("'from' object is not a powerflow node (it's a '%s')", pFrom->get_oclass()->get_name());
	from_voltage_A = gld_property(pFrom,"voltage_A");
	from_voltage_B = gld_property(pFrom,"voltage_B");
	from_voltage_C = gld_property(pFrom,"voltage_C");
	if ( !from_voltage_A.is_valid() ) exception("'from' node property 'voltage_A' not found");
	if ( !from_voltage_B.is_valid() ) exception("'from' node property 'voltage_B' not found");
	if ( !from_voltage_C.is_valid() ) exception("'from' node property 'voltage_C' not found");

	gld_property to(get_parent(),"to");
	gld_object *pTo = to.get_objectref();
	if ( !pTo->isa("node") ) exception("'to' object is not a powerflow node (it's a '%s')", pTo->get_oclass()->get_name());
	to_voltage_A = gld_property(pTo,"voltage_A");
	to_voltage_B = gld_property(pTo,"voltage_B");
	to_voltage_C = gld_property(pTo,"voltage_C");
	if ( !to_voltage_A.is_valid() ) exception("'to' node property 'voltage_A' not found");
	if ( !to_voltage_B.is_valid() ) exception("'to' node property 'voltage_B' not found");
	if ( !to_voltage_C.is_valid() ) exception("'to' node property 'voltage_C' not found");

	gld_property length(get_parent(),"length");
	if ( !length.is_valid() ) exception("line property 'length' is not found");
	double w = get_location() / length.get_double();
	if ( w > 1.0 )
	{
		warning("sensor location is beyond 'to' node (w>1); moving location to 'to' node");
		w = 1.0;
	}
	else if ( w < 0.0 )
	{
		warning("sensor location is beyond 'from' node (w>1); moving location to 'from' node");
		w = 0.0;
	}

	size_t i, j;
	if ( covariance.is_empty() ) // default is no noise
	{
		memset(L,0,sizeof(L));
	}
	else if ( covariance.get_rows()==1 && covariance.get_cols()==4 ) // compact form for independent noise [s1,s2,s3,s4] -> diag(L)
	{
		memset(L,0,sizeof(L));
		L[0][0] = covariance[0][0];
		L[1][1] = covariance[0][1];
		L[2][2] = covariance[0][2];
		L[3][3] = covariance[0][3];
	}
	else if ( covariance.get_rows()==2 && covariance.get_cols()==6 ) // compact form for dependent noise [s1,s2,s3,s4,~,~ ; a,b,c,d,e,f] -> [ s1,a,b,c ; a,s2,d,e ; b,d,s3,f ; c,e,0,s4 ]
	{
		memset(L,0,sizeof(L));
		L[0][0] = covariance[0][0]; // s1
		L[1][1] = covariance[0][1]; // s2
		L[2][2] = covariance[0][2]; // s3
		L[3][3] = covariance[0][3]; // s4
		L[0][1] = L[1][0] = covariance[1][0]; // a
		L[0][2] = L[2][0] = covariance[1][1]; // b
		L[0][3] = L[3][0] = covariance[1][2]; // c
		L[1][2] = L[2][1] = covariance[1][3]; // d
		L[1][3] = L[3][1] = covariance[1][4]; // e
		L[2][3] = L[3][2] = covariance[1][5]; // f
	}
	else if ( covariance.get_rows()==4 && covariance.get_cols()==4 ) // full form (4x4 PSD matrix)
	{
		for ( i = 0 ; i < 4 ; i++ )
			for ( j = 0 ; j < 4 ; j++ )
				L[i][j] = covariance[i][j];
	}
	else {
		exception("covariance matrix is not in proper form; only 1x4, 2x6 or 4x4 forms allowed");
	}

	debug("C[0] = [%g, %g, %g, %g]", L[0][0], L[0][1], L[0][2], L[0][3]);
	debug("C[1] = [%g, %g, %g, %g]", L[1][0], L[1][1], L[1][2], L[1][3]);
	debug("C[2] = [%g, %g, %g, %g]", L[2][0], L[2][1], L[2][2], L[2][3]);
	debug("C[3] = [%g, %g, %g, %g]", L[3][0], L[3][1], L[3][2], L[3][3]);

	bool is_zero = true;
	for ( i = 0 ; i < 4 ; i++ )
		for ( j = 0 ; j < 4 ; j++ )
			is_zero &= (L[i][j]==0);
	if ( !is_zero )
	{
		double diag[4];
		cholesky(4,(double*)L,diag);
		size_t i, j;
		for ( i = 0 ; i < 4 ; i++ )
		{
			L[i][i] = diag[i];
			size_t j;
			for ( j = 0 ; j < 4 ; j++ )
			{
				if ( isnan(L[i][j]) )
				{
					exception("covariance is not positive symmetric definite");
				}
			}
		}
	}

	debug("L[0] = [%g, %g, %g, %g]", L[0][0], 0, 0, 0);
	debug("L[1] = [%g, %g, %g, %g]", L[1][0], L[1][1], 0, 0);
	debug("L[2] = [%g, %g, %g, %g]", L[2][0], L[2][1], L[2][2], 0);
	debug("L[3] = [%g, %g, %g, %g]", L[3][0], L[3][1], L[3][2], L[3][3]);

	return 1;
}

TIMESTAMP line_sensor::postsync(TIMESTAMP t1)
{
	// get raw measurements
	switch ( get_measured_phase() ) {
	case PHASE_A:
		debug("from_voltage_A = %g%+gi", from_voltage_A.get_complex().Re(), from_voltage_A.get_complex().Im());
		set_measured_voltage(from_voltage_A.get_complex()*(1-w) + to_voltage_A.get_complex()*w);
		debug("from_current_A = %g%+gi", from_current_A.get_complex().Re(), from_current_A.get_complex().Im());
		set_measured_current(from_current_A.get_complex()*(1-w) + to_current_A.get_complex()*w);
		break;
	case PHASE_B:
		debug("from_voltage_B = %g%+gi", from_voltage_B.get_complex().Re(), from_voltage_B.get_complex().Im());
		set_measured_voltage(from_voltage_B.get_complex()*(1-w) + to_voltage_B.get_complex()*w);
		debug("from_current_B = %g%+gi", from_current_B.get_complex().Re(), from_current_B.get_complex().Im());
		set_measured_current(from_current_B.get_complex()*(1-w) + to_current_B.get_complex()*w);
		break;
	case PHASE_C:
		debug("from_voltage_C = %g%+gi", from_voltage_C.get_complex().Re(), from_voltage_C.get_complex().Im());
		set_measured_voltage(from_voltage_C.get_complex()*(1-w) + to_voltage_C.get_complex()*w);
		debug("from_current_C = %g%+gi", from_current_C.get_complex().Re(), from_current_C.get_complex().Im());
		set_measured_current(from_current_C.get_complex()*(1-w) + to_current_C.get_complex()*w);
		break;
	default:
		break;
	}

	// TODO: apply noise
	double r[4] = {gl_random_normal(RNGSTATE,0,1), gl_random_normal(RNGSTATE,0,1), gl_random_normal(RNGSTATE,0,1), gl_random_normal(RNGSTATE,0,1)};
	double noise[4] = {r[0]*L[0][0], r[0]*L[1][0]+r[1]*L[1][1], r[0]*L[2][0]+r[1]*L[2][1]+r[2]*L[2][2], r[0]*L[3][0]+r[1]*L[3][1]+r[2]*L[3][2]+r[3]*L[3][3]};
	debug("noise = [%g, %g, %g, %g]", noise[0], noise[1], noise[2], noise[3]);
	measured_voltage.Re() *= sqrt(1+noise[0]);
	measured_voltage.Im() *= sqrt(1+noise[1]);
	measured_current.Re() *= sqrt(1+noise[2]);
	measured_current.Im() *= sqrt(1+noise[3]);
	debug("measured_voltage = %g%+gi", measured_voltage.Re(), measured_voltage.Im());
	debug("measured_current = %g%+gi", measured_current.Re(), measured_current.Im());

	// compute power
	set_measured_power(get_measured_voltage()*(~get_measured_current()));
	return TS_NEVER;
}
