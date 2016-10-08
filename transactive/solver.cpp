// $Id: solver.cpp 5183 2015-06-20 00:59:14Z dchassin $

#include "solver.h"

char1024 solver::solver_check = "";
#define DUMP_MATLAB // enables solver check dump to matlab

#ifdef _DEBUG
// #define DUMP_STDOUT // enables solver dumps to screen (*very* chatty)
#define TEST_SIZE 0 // enable autotest of random network of size given (must be >3)
#endif

solver::solver()
{
	max_nodes = n_nodes = max_lines = n_lines = n_constraints = 0;
	node = NULL;
	line = NULL;
	status = SS_UNUSED;
#ifdef _DEBUG
#if TEST_SIZE > 3 // anything less is trivial
	// unconstrained solver test
	if ( !test_unconstrained(TEST_SIZE) )
		gl_error("solver::solver(): autotest failed");
	else
		gl_verbose("solver::solver(): autotest passed");
#endif
#endif
}
void solver::create(size_t nodes, size_t lines)
{
	if ( max_nodes>0 || max_lines>0 ) throw "unable to create interconnection solver more than once";
		/* TROUBLESHOOT
		   TODO
		*/
	if ( (max_nodes=nodes)==0 ) throw "unable to create interconnection solver with zero nodes";
		/* TROUBLESHOOT
		   TODO
		*/
	node = (controlarea**)malloc(sizeof(controlarea*)*nodes);
	if ( (max_lines=lines)==0 ) throw "unable to create interconnection solver with zero lines";
		/* TROUBLESHOOT
		   TODO
		*/
	line = (intertie**)malloc(sizeof(intertie*)*lines);
	constraint = (intertie**)malloc(sizeof(intertie*)*lines);
	status = SS_INIT;
}

void solver::add_node(OBJECT *p)
{
	// add to node list
	if ( n_nodes>=max_nodes ) throw "too many nodes added to interconnection solver";
	controlarea *area = OBJECTDATA(p,controlarea);
	size_t n = n_nodes++;
	area->set_node_id(n);
	node[n] = area;

	// check if lists are complete
	if ( n_nodes==max_nodes && n_lines==max_lines ) status = SS_READY;
}

void solver::add_line(OBJECT *p)
{
	// add to line list
	if ( n_lines>=max_lines ) throw "too many lines added to interconnection solver";
		/* TROUBLESHOOT
		   TODO
		*/
	intertie *tie = OBJECTDATA(p,intertie);
	size_t n = n_lines++;
	tie->set_line_id(n);
	line[n] = tie;

	// add to constraint list
	gld_property line_capacity(get_object(p),"capacity");
	gld_property line_status(get_object(p),"status");
	double capacity_value; line_capacity.getp(capacity_value);
	enumeration status_value; line_status.getp(status_value);
	if ( capacity_value>0 && status_value==TLS_CONSTRAINED )
		constraint[n_constraints++] = OBJECTDATA(p,intertie);
}

bool solver::setup(void)
{
	try {
		// create flow and power vectors
		x = vector(n_lines,0.0);
		b = vector(n_nodes,0.0);

		// build graph matrix A
		A = matrix(n_lines,n_nodes,0.0);
		for ( int m=0 ; m<n_lines ; m++ )
		{
			controlarea *fromarea = line[m]->get_from_area();
			controlarea *toarea = line[m]->get_to_area();
			size_t from = fromarea->get_node_id();
			size_t to = toarea->get_node_id();
			double loss = line[m]->get_loss();
			A.set(m,from,1-loss);
			A.set(m,to,loss-1);
		}
		if ( verbose_options&VO_SOLVER )
			A.dump(stdout,"A");
		status = SS_READY;
		return true;
	} catch (char *msg)
	{
		gl_error("solver setup failed: %s",msg);
		return false;
	} catch (...)
	{
		gl_error("solver setup failed: %s","unhandled exception");
		return false;
	}
}

void solver::update_b(void)
{
	// copy node data
	double total_p = 0.0;
	double total_e = 0.0;
	vector e(n_nodes,0.0);
	for ( size_t n=0 ; n<n_nodes ; n++ )
	{
		total_p += ( b[n] = node[n]->get_supply() - node[n]->get_demand() - node[n]->get_actual() ); // - node[n]->get_losses() );
		total_e += ( e[n] = node[n]->get_inertia() );
	}
	if ( total_e>0 )
	{
		double factor = total_p/total_e;
		for ( size_t n=0 ; n<n_nodes ; n++ )
			b[n] -= e[n]*factor;
	}
	else if ( total_p!=0 )
		gl_warning("power is imbalanced with zero system inertia, flow solver may not be stable");
		/* TROUBLESHOOT
		   TODO
		*/
}

bool solver::update_x(void)
{
	// copy line data
	bool limit_exceeded = false;
	for ( size_t m=0 ; m<n_lines ; m++ )
	{
		double limit = line[m]->get_capacity();
		if ( !isfinite(x[m]) )
		{
			gl_warning("line %d flow is not finite, using 0.0 instead", m);
			x[m] = 0.0;
		}
		else if ( limit>0 && abs(x[m])>limit && !allow_constraint_violation )
		{
			gl_debug("line %d limit exceeded, switching to constrained solver", m);
			limit_exceeded = true;
		}
		line[m]->set_flow(x[m]);
	}
	return limit_exceeded;
}

bool solver::solve(double dt)
{
	if ( n_constraints==0 || allow_constraint_violation ? solve_unconstrained(dt) : solve_constrained(dt) )
		return true;
	else
	{
		gl_error("unable to solve interconnection flows");
		/* TROUBLESHOOT
		   TODO
		*/
		return false;
	}
}

bool solver::solve_unconstrained(double dt)
{
#ifdef DUMP_MATLAB
	static FILE *fh = NULL;
	if ( fh==NULL && strcmp(solver_check,"")!=0 )
	{
		fh=fopen(solver_check,"w");
		gld_clock st(time(NULL));
		fprintf(fh,"%%\n%% gridlabd transactive module version %d.%d started at %s\n%%\n",MAJOR,MINOR,(const char*)st.get_string());
	}
#endif

	update_b();

	// solve flow problem
	if ( A.check_inverse())
		x = b / A;
	else
		throw "no solution exists";
		/* TROUBLESHOOT
		   TODO
		*/

	bool limit_exceeded = update_x();

#ifdef DUMP_MATLAB
	if ( fh!=NULL )
	{
		gld_clock ts(gl_globalclock);
		fprintf(fh,"\n%% solution for %s\n",(const char*)ts.get_string());
		b.dump(fh,"b");
		A.dump(fh,"M");
		x.dump(fh,"x");
		fprintf(fh,"r = norm(x*M-b)/std(b);\nif ( r>1e-6 )\n\twarning('significant residual (r=%%g%%%%) at %s',r*100);\nend;\n",(const char*)ts.get_string());
		fflush(fh);
	}
#endif
	return limit_exceeded ? solve_constrained(dt) : true;
}

bool solver::solve_constrained(double dt)
{
	gl_error("solve_constrained() is not implemented");
		/* TROUBLESHOOT
		   TODO
		*/
	return false;
}

bool solver::test_unconstrained(size_t N)
{
	try {
		if ( N<3 ) return false; // boring

		double p = 0.9; // probability of line in existing & in service (better to use N, i.e., 2/N at a minimum)
		double r = 0.95; // line gain

		// net power matrix
		size_t M = N*(N-1)/2;
		b = vector(N,RD_NORMAL,0,1);

		// connectivity vector
		vector C(M,RD_UNIFORM,0,1);

		// graph matrix
		A = matrix(M,N,0.0);
		size_t l = 0;
		for ( size_t n=0 ; n<N ; n++ )
		{
			for ( size_t m=n+1 ; m<N ; m++ )
			{
				if ( C[l]<p )
				{
					A[l][n] = r;
					A[l][m] = r;
				}
				l++;
			}
		}

		// solver
		if ( !solve_unconstrained(0) ) return false;

		// result check
		vector R = x*A-b;
		double residual = R.sum();
		gl_verbose("solver::test_unconstrained(): test %d residual is %g", N, residual);
		if ( residual>1e-6 )
			gl_warning("solver::test_unconstrained(): large residual indicates possible island or solver error");
		/* TROUBLESHOOT
		   TODO
		*/

		return true;
	}
	catch (const char *msg)
	{
		gl_error("solver::test_unconstrained(): test %d exception: %s", N, msg);
		/* TROUBLESHOOT
		   TODO
		*/
		return false;
	}
	catch (...)
	{
		gl_error("solver::test_unconstrained(): test %d general uncaught exception", N);
		return false;
		/* TROUBLESHOOT
		   TODO
		*/
	}
}
