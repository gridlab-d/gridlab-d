/** $Id$

   General purpose building objects

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "building.h"

double initial_air_temperature = 72;
double building::maximum_timestep = 300;
double_array building::enduse_composition;

// enduse composition data
typedef enum {LT_Z, LT_I, LT_P, LT_MA, LT_MB, LT_MC, LT_MD, LT_PA, LT_PB, _NLT} LOADTYPE;
typedef enum {
	EUT_INTLIGHTS, EUT_PLUGS, EUT_HEATING, EUT_COOLING, EUT_VENTILATION, 
	EUT_REFRIGERATION, EUT_PUMPS, EUT_PROCESS, EUT_EXTLIGHT, _NEUT};
double rules_of_association[_NLT][_NEUT] = {
	//  Z,  I,  P, MA, MB, MC, MD, PA, PB, 
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // interior lights
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // plugs
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // heating
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // cooling
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // ventilation
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // refrigeration
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // pumps
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // process
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	}, // exterior lights
};

EXPORT_CREATE(building);
EXPORT_INIT(building);
EXPORT_PRECOMMIT(building);
EXPORT_SYNC(building);
EXPORT_COMMIT(building);
EXPORT_PLC(building);

CLASS *building::oclass = NULL;
building *building::defaults = NULL;

building::building(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"building",sizeof(building),PC_AUTOLOCK|PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class building";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,

			// thermal model
			PT_int16, "N", get_N_offset(), PT_DESCRIPTION,"number of thermal nodes in building",
			PT_double_array, "T", get_T_offset(), PT_UNITS, "degF", PT_DESCRIPTION, "N node temperatures",
			PT_double_array, "U", get_U_offset(), PT_UNITS, "Btu/degF/h", PT_DESCRIPTION, "NxN node conductances",
			PT_double_array, "C", get_C_offset(), PT_UNITS, "Btu/degF", PT_DESCRIPTION, "N node heat capacities (NaN for outdoor node)",
			PT_double_array, "Q", get_Q_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node heat flows",
			PT_double_array, "Qs", get_Qs_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node solar gains",
			PT_double_array, "Qi", get_Qi_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node internal heat gains",

			// equipment model
			PT_double_array, "Qfl", get_Qfl_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node fan minimum heat (0 for none)",
			PT_double_array, "Qfh", get_Qfh_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node fan maximum heat (0 for none)",
			PT_double_array, "Qf", get_Qf_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node fan heat flow",
			PT_double_array, "Qhc", get_Qhc_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node heating capacity (0 for none)",
			PT_double_array, "Qh", get_Qh_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node heating heat flow",
			PT_double_array, "Qcc", get_Qcc_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node cooling capacity (0 for none)",
			PT_double_array, "Qc", get_Qc_offset(), PT_UNITS, "Btu/h", PT_DESCRIPTION, "N node cooling heat flow",

			// control model
			PT_double_array, "Ts", get_Ts_offset(), PT_DESCRIPTION, "N node thermostat mode (0=NONE, 1=OFF, 2=VENT, 3=HEAT, 4=COOL, AUX=5, 6=ECON)",
			PT_double_array, "Vm", get_Vm_offset(), PT_UNITS, "pu/h", PT_DESCRIPTION, "N node minimum ventilation rates",
			PT_double_array, "Th", get_Th_offset(), PT_UNITS, "degF", PT_DESCRIPTION, "N node heating set-point",
			PT_double_array, "Tc", get_Tc_offset(), PT_UNITS, "degF", PT_DESCRIPTION, "N node cooling set-point",
			PT_double_array, "Td", get_Td_offset(), PT_UNITS, "degF", PT_DESCRIPTION, "N node set-point deadbands",
			PT_double, "tl", get_tl_offset(), PT_UNITS, "s", PT_DESCRIPTION, "thermostat lockout time", 

			// special properties
			PT_bool, "autosize", get_autosize_offset(), PT_DESCRIPTION, "flag to enable autosizing of arrays",

			// enduse load
//			PT_enduse, "load", get_load_offset(), PT_DESCRIPTION, "end-use load composition",

			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}
		memset(this,0,sizeof(building));
		gl_global_create("building::maximum_timestep",PT_double,&maximum_timestep,PT_UNITS,"s",PT_DESCRIPTION,"maximum timestep allowed for model",NULL);

		// default enduse composition
//		gl_global_create("building::enduse_composition",PT_double_array,&enduse_composition,PT_UNITS,"s",PT_DESCRIPTION,"maximum timestep allowed for model",NULL);
//		int lt, eut;
//		enduse_composition.grow_to(_NLT,_NEUT);
//		for ( lt=0 ; lt<_NLT ; lt++ )
//		{
//			for ( eut=0 ; eut<_NEUT ; eut++ )
//				enduse_composition.set_at(lt,eut,rules_of_association[lt][eut]);
//		}
	}
}

int building::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

	// TODO set default values
	Tlast = NULL;
	autosize = true;

	return SUCCESS;
}

int building::init(OBJECT *parent)
{
	// set the initial value of the last update time to the start time
	gld_global global_starttime("starttime");
	t_last = global_starttime.get_int64();
	dT = new double[N];

	// check values and fixup defaults

	// transpose vectors if needed
	struct {
		const char *name;
		double_array *vector;
		bool required;
		bool autosize_ok;
		double value;
	} m[] = {
		// general properties
		// name,	addr,	required,	autosize_ok,	value
		{"Q",		&Q,		false,		true,			0,		},
		{"C",		&C,		true,		true,			0,		},
		{"T",		&T,		true,		true,			initial_air_temperature,		},
		{"Qf",		&Qf,	false,		true,			0,		},
		{"Qs",		&Qs,	false,		true,			0,		},
		{"Qi",		&Qi,	false,		true,			0,		},
		// default HVAC properties
		{"Qfl",		&Qfl,	false,		true,			0,	},
		{"Qfh",		&Qfh,	false,		true,			0,	},
		{"Qhc",		&Qhc,	false,		true,			0,	},
		{"Qh",		&Qh,	false,		true,			0,	},
		{"Qcc",		&Qcc,	false,		true,			0,	},
		{"Qc",		&Qc,	false,		true,			0,	},
		// default controller properties
		{"Ts",		&Ts,	false,		true,			NONE,	},
		{"Vm",		&Vm,	false,		true,			0.2,	},
		{"Th",		&Th,	false,		true,			70,	},
		{"Tc",		&Tc,	false,		true,			75,	},
		{"Td",		&Td,	false,		true,			0.5,	},
	};
	for ( unsigned int i=0 ; i<sizeof(m)/sizeof(m[0]) ; i++ )
	{
		m[i].vector->set_name(m[i].name);

		// check for correct dimensions
		unsigned int n = m[i].vector->get_cols();
		if ( n==0 )
		{
			if ( m[i].required ) 
				exception("%s must be specified", m[i].name);
			m[i].vector->grow_to(N);
		}

		// transpose if necessary
		if ( m[i].vector->get_rows()>m[i].vector->get_cols() ) m[i].vector->transpose();
		
		// autosize
		if ( m[i].vector->get_cols()<N && autosize && m[i].autosize_ok )
			m[i].vector->grow_to(N);

		// fill needed defaults
		while ( n<N )
			m[i].vector->set_at(n++, m[i].value);

		// check for correct dimensions
		if ( m[i].vector->get_cols()!=N || m[i].vector->get_rows()!=1 ) 
			exception("%s must have 1x%d or %dx1 (it has %dx%d)", m[i].name, N, N, m[i].vector->get_rows(), m[i].vector->get_cols());
	}

	// check dimensions
	if ( U.get_rows()!=N || U.get_cols()!=U.get_rows() ) exception("U must be %dx%d (it has %dx%d)",N,N,U.get_rows(),U.get_cols());
	
	// check lockout time
	if ( tl<0 ) exception("tl must be zero or positive");

	return SUCCESS;
}

/// precommit - update zone temperatures to current time
STATUS building::precommit(TIMESTAMP t)
{
	// determine the elapsed time since the last update
	double dt = (t-t_last)*3600;

	// save a copy of the last temperatures for update processing
	Tlast = T.copy_vector(Tlast);

	// process each massive node
	unsigned int n;
	for ( n=0 ; n<N ; n++ )
	{
		double Cn = C.get_at(n);
		if ( Cn==0 || isnan(Cn) ) 
			continue; // ignore massless && outdoor nodes
		double Qn = Q.get_at(n);
		if ( isnan(Qn) ) 
			exception("node %d Qn is not a number", Qn);

		// collect values related to other nodes
		unsigned int m;
		double Um = 0.0;
		double Qm = 0.0;
		for ( m=0 ; m<N ; m++ )
		{
			double Unm = U.get_at(n,m);
			if ( isnan(Unm) ) 
				exception ("U[%d,%d] is not a number", Unm);
			if ( Unm!=0.0 )
			{
				Um += Unm;
				Qm += Unm*Tlast[m];
			}
		}

		// perform the explicit solution massive node update
		dT[n] = dt/Cn * ( Qn + Qm + Tlast[n]*Um );
		if ( isnan(dT[n]) ) 
			exception("node %d dT is not a number", n);
		T.set_at(n, Tlast[n] + dT[n] ); 
	}

	// process each massless node
	for ( n=0 ; n<N ; n++ )
	{
		double Cn = C.get_at(n);
		if ( Cn>0 || isnan(Cn) ) 
			continue; // ignore massive and outdoor nodes
		double Qn = Q.get_at(n);
		unsigned int m;
		double Um = 0.0;
		double Qm = 0.0;
		for ( m=0 ; m<N ; m++ )
		{
			if ( C.get_at(m)==0 ) 
				exception("reduction is needed for nodes %d and %d", n, m);
			double Unm = U.get_at(n,m);
			if ( Unm!=0.0 )
			{
				Um += Unm;
				Qm += Unm*T.get_at(m);
			}
		}
	}
	return SUCCESS;
}

TIMESTAMP building::sync(TIMESTAMP t1)
{
	return TS_NEVER;
}

TIMESTAMP building::commit(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	unsigned int n;

	// process each node with HVAC control
	for ( n=0 ; n<N ; n++ )
	{
		double Cn = C.get_at(n);
		if ( isnan(Cn) ) 
			continue; // ignore outdoor nodes
		unsigned int hm = (unsigned int)(Ts.get_at(n)+0.5);
		if ( hm==NONE ) continue; // no control

		double Qn = Qh.get_at(n) + Qc.get_at(n) + Qf.get_at(n) + Qs.get_at(n) + Qi.get_at(n);
		if ( isnan(Qn) )
			exception("node %d Q is not a number", n);
		Q.set_at(n, Qn);
		double Tt; // target temperature
		double Td2 = Td.get_at(n)/2;
		if ( isnan(Td2) )
			exception("node %d Td is not a number", n);
		switch ( hm ) {
		case OFF :
		case VENT :
			if ( dT[n]<0 ) Tt = Th.get_at(n) - Td2; // floating toward heating
			else if (dT[n]>0 ) Tt = Tc.get_at(n) + + Td2; // floating toward cooling
			else Tt = Th.get_at(n); // no target yet (very rare)
			break;
		case HEAT :
			Tt = Th.get_at(n) + Td2; // heating toward heat-off
			break;
		case COOL :
			Tt = Th.get_at(n) - Td2; // cooling toward cool -off
			break;
		default:
			exception("mode %d is invalid", hm);
			break;
		}
		unsigned int m;
		double Qm = 0;
		for ( m=0 ; m<N ; m++ )
			Qm += U.get_at(n,m) * ( T.get_at(m) - T.get_at(n) );
		double dt = 3600 * C.get_at(n) * ( Tt - T.get_at(n) ) / ( Qn + Qm );
		if ( isnan(dt) ) 
			exception("node %d: dt is not a number", n);
		if ( dt>maximum_timestep )
			dt = maximum_timestep;
		gl_debug("%s node %d dt=%.1fs", get_name(), n, dt);
		TIMESTAMP t3 = (TIMESTAMP)(t1+dt);
		if ( t3 < t2 )
			t2 = t3; // round down
	}
	return t2;
}

// default controls (run just prior to bottom-up pass)
TIMESTAMP building::plc(TIMESTAMP t)
{
	unsigned int n;

	// process each node with HVAC control
	for ( n=0 ; n<N ; n++ )
	{
		double Cn = C.get_at(n);
		if ( isnan(Cn) ) 
			continue; // ignore outdoor nodes
		unsigned int hm = (unsigned int)(Ts.get_at(n)+0.5);
		if ( hm==NONE ) continue; // no fan control -> no HVAC control

		// simple bang-bang control
		unsigned int vm = (Vm.get_at(n)>0) ? VENT : OFF;
		double Tn = T.get_at(n); // temperature
		double Td2 = Td.get_at(n)/2; // deadband
		
		// heating control
		if ( Qhc.get_at(n)>0 )
		{
			double Thon = Th.get_at(n)-Td2;
			double Thoff = Th.get_at(n)+Td2;
			if ( hm!=HEAT && Tn<Thon ) 
			{
				Ts.set_at(n,HEAT);
				set_validto(t+tl);
			}
			else if ( hm==HEAT && Tn>Thoff ) 
			{
				Ts.set_at(n,vm);
				set_validto(t+tl);
			}
		}

		// cooling control
		if ( Qcc.get_at(n)>0 ) 
		{
			double Tcon = Tc.get_at(n)+Td2;
			double Tcoff = Tc.get_at(n)-Td2;
			if ( hm!=COOL && Tn>Tcon ) 
			{
				Ts.set_at(n,COOL);
				set_validto(t+tl);
			}
			else if ( hm==COOL && Tn<Tcoff ) 
			{
				Ts.set_at(n,vm);
				set_validto(t+tl);
			}
		}

		// apply capacities
		hm = (unsigned int)(Ts.get_at(n)+0.5);
		switch ( hm ) {
		case OFF :
			Qh.set_at(n,0.0);
			Qc.set_at(n,0.0);
			Qf.set_at(n,0.0);
			break;
		case VENT :
			Qh.set_at(n,0.0);
			Qc.set_at(n,0.0);
			Qf.set_at(n,Qfl.get_at(n));
			break;
		case HEAT :
			Qh.set_at(n,Qhc.get_at(n));
			Qc.set_at(n,0.0);
			Qf.set_at(n,Qfh.get_at(n));
			break;
		case COOL :
			Qh.set_at(n,0.0);
			Qc.set_at(n,Qcc.get_at(n));
			Qf.set_at(n,Qfh.get_at(n));
			break;
		default:
			exception("invalid HVAC mode %d", hm);
		}
	}

	return TS_NEVER;
}