// $Id: controlarea.cpp 5185 2015-06-23 00:13:36Z dchassin $

#include "transactive.h"

EXPORT_IMPLEMENT(controlarea)
EXPORT_SYNC(controlarea)
EXPORT_NOTIFY_PROP(controlarea,update)

static TIMESTAMP ace_sample_time = 4;

controlarea::controlarea(MODULE *module)
{
	oclass = gld_class::create(module,"controlarea",sizeof(controlarea),PC_PRETOPDOWN|PC_BOTTOMUP|PC_AUTOLOCK);
	oclass->trl = TRL_CONCEPT;
	defaults = this;
	if ( gl_publish_variable(oclass,
		PT_int64,"update",get_update_offset(),PT_HAS_NOTIFY_OVERRIDE,PT_DESCRIPTION,"incoming update handler",
		PT_double,"inertia[MJ]",get_inertia_offset(),PT_DESCRIPTION,"moment of inertia",
		PT_double,"capacity[MW]",get_capacity_offset(),PT_DESCRIPTION,"total rated generation capacity",
		PT_double,"supply[MW]",get_supply_offset(),PT_DESCRIPTION,"total generation supply",
		PT_double,"demand[MW]",get_demand_offset(),PT_DESCRIPTION,"total load demand",
		PT_double,"schedule[MW]",get_schedule_offset(),PT_DESCRIPTION,"scheduled intertie exchange",
		PT_double,"actual[MW]",get_actual_offset(),PT_DESCRIPTION,"actual intertie exchange",
		PT_double,"ace[MW]",get_ace_offset(),PT_DESCRIPTION,"area control error",
		PT_double_array,"ace_filter",get_ace_filter_offset(),PT_DESCRIPTION,"area control filter coefficients (0=P 1=I)",
		PT_double,"bias[MW/Hz]",get_bias_offset(),PT_DESCRIPTION,"frequency control bias",
		PT_double,"losses[MW]",get_losses_offset(),PT_DESCRIPTION,"line losses (internal+export)",
		PT_double,"internal_losses[pu]",get_internal_losses_offset(),PT_DESCRIPTION,"internal line losses per unit load",
		PT_double,"imbalance[MW]",get_imbalance_offset(),PT_DESCRIPTION,"area power imbalance",
		PT_enumeration,"status",get_status_offset(),PT_DESCRIPTION,"status flag",
			PT_KEYWORD,"OK",(enumeration)CAS_OK,
			PT_KEYWORD,"OVERCAPACITY",(enumeration)CAS_OVERCAPACITY,
			PT_KEYWORD,"CONSTRAINED",(enumeration)CAS_CONSTRAINED,
			PT_KEYWORD,"ISLAND",(enumeration)CAS_ISLAND,
			PT_KEYWORD,"UNSCHEDULED",(enumeration)CAS_UNSCHEDULED,
			PT_KEYWORD,"BLACKOUT",(enumeration)CAS_BLACKOUT,
		NULL)<1 )
		exception("unable to publish transactive controlarea properties");
	memset(defaults,0,sizeof(controlarea));
}

int controlarea::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	ace_filter.set_name("ace_filter");
	set_ace_filter("0.1 0.9");
	return 1;
}

int controlarea::init(OBJECT *parent)
{
	// check that there is at least one intertie
	if ( n_intertie==0 )
	{
		if ( init_count++==0 )
			return OI_WAIT; // defer
		else if ( init_count>10 )
			warning("control area with no intertie");
		/* TROUBLESHOOT
		   TODO
		*/
	}

	// check that parent is an interconnection
	if ( parent==NULL ) exception("parent must be defined");
	system = get_object(parent);
	if ( !system->isa("interconnection") ) exception("parent must be an interconnection");

	// collect properties
	update_system = gld_property(system,"update");
	frequency = gld_property(system,"frequency");

	// register control area with interconnection
	update_system.notify(TM_REGISTER_CONTROLAREA,my());

	// update system info with SWING message
	update_system.notify(TM_AREA_STATUS,inertia,capacity,supply,demand,losses);

	// check for undefined bias
	if ( bias==0 )
		bias = capacity/nominal_frequency/droop_control + demand ;

	// check ace filter
	if ( ace_filter.get_rows()!=1 && ace_filter.get_cols()!=2 )
		exception("ace_filter must be 2 PI gains (e.g., [P,I]=\"0.9 0.1\")");

	last_update = gl_globalclock;
	return OI_DONE;
}

int controlarea::isa(const char *type)
{
	return strcmp(type,"controlarea")==0;
}

TIMESTAMP controlarea::presync(TIMESTAMP t1)
{
	inertia = 0;
	capacity = 0;
	supply = 0;
	demand = 0;
	actual = 0;
	// update only on ACE time
	return (gl_globalclock/ace_sample_time+1)*ace_sample_time;
}

TIMESTAMP controlarea::sync(TIMESTAMP t1)
{
	// update actual
	actual = 0;
	double export_losses = 0;
	if ( verbose_options&VO_CONTROLAREA )
	{
		gld_clock ts(t1);
		fprintf(stderr,"CONTROLAREA    : %s at %s: dt=%d s\n",get_name(),(const char*)ts.get_string(),t1-last_update);
		last_update = t1;
		fprintf(stderr,"CONTROLAREA    :   supply.............. %8.3f\n",supply);
		fprintf(stderr,"CONTROLAREA    :   demand.............. %8.3f\n",demand);
	}
	for ( OBJECTLIST *line=intertie_list ; line!=NULL ; line=line->next )
	{
		intertie *tie = OBJECTDATA(line->obj,intertie);

		// primary flow
		double flow = (tie->get_from()==my()?1:-1) * tie->get_flow();
		actual += flow;
		
		// add losses only on outbound schedules
		if ( flow>0 )
		{
			// compute loss factor
			double k = tie->get_loss(); // 1.0/(1.0-tie->get_loss()) - 1.0;

			if ( verbose_options&VO_CONTROLAREA )
			{
				double limit = tie->get_capacity();
				if ( flow>0 )
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f %+8.3f %s (%s->%s)\n",
						flow, flow*k, limit>0&&abs(flow)>limit ? "!!!":"   ", tie->get_from()->name, tie->get_to()->name); 
				else
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f %+8.3f %s (%s->%s) \n", 
						flow, flow*k, limit>0&&abs(flow)>limit ? "!!!":"   ",tie->get_to()->name, tie->get_from()->name); 
			}
			export_losses += flow*k;
		}
		else
		{
			if ( verbose_options&VO_CONTROLAREA )
			{
				double limit = tie->get_capacity();
				if ( flow>0 )
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f         %s (%s->%s)\n",
						flow, limit>0&&abs(flow)>limit ? "!!!":"   ",tie->get_from()->name, tie->get_to()->name); 
				else
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f         %s (%s->%s)\n",
						flow, limit>0&&abs(flow)>limit ? "!!!":"   ",tie->get_to()->name, tie->get_from()->name); 
			}
		}
	}

	// finalize imbalance and total losses
	losses = export_losses + demand*internal_losses;
	imbalance = supply-demand-losses-actual;

	// update system info with SWING message
	update_system.notify(TM_AREA_STATUS,inertia,capacity,supply,demand,losses);

	// update controlarea status flag
	if ( supply>capacity*1.01 ) status = CAS_OVERCAPACITY;
	else if ( supply>capacity*0.99 ) status = CAS_CONSTRAINED;
	else if ( schedule<capacity*0.001 ) status = CAS_UNSCHEDULED;
	else if ( actual<capacity*0.001 ) status= CAS_ISLAND;
	else if ( demand<capacity*0.001 && supply<capacity*0.001 && fabs(schedule)>capacity*0.001 ) status = CAS_BLACKOUT;
	else status = CAS_OK;
	if ( verbose_options&VO_CONTROLAREA )
	{
		fprintf(stdout,"CONTROLAREA    :   actual.............. %8.3f %\n",actual);
		fprintf(stdout,"CONTROLAREA    :   schedule............ %8.3f\n",schedule); 
		fprintf(stdout,"CONTROLAREA    :   losses.............. %8.3f %\n",losses);
		fprintf(stdout,"CONTROLAREA    :   frequency error..... %8.3f\n",ferror); 
		fprintf(stdout,"CONTROLAREA    :   total ace........... %8.3f\n",ace); 
	}

	// calculate ACE only every 4 seconds
	if ( gl_globalclock%ace_sample_time==0 )
	{
		double ferror = bias * ( nominal_frequency - frequency.get_double() );
		if ( true ) // actual!=0 ) // && schedule!=0 )
		{
			ace = ace_filter[0][1]*ace + ace_filter[0][0]*((actual + export_losses - schedule) + ferror);
			if ( !isfinite(ace) )
			{
				gl_warning("ACE for node %s is not a finite number, using 0.0 instead", get_name());
			/* TROUBLESHOOT
			   TODO
			*/
				ace = 0.0;
			}
		}
	}
	return (gl_globalclock/ace_sample_time+1)*ace_sample_time;
}

TIMESTAMP controlarea::postsync(TIMESTAMP t1)
{
	error("postsync not implemented");
		/* TROUBLESHOOT
		   TODO
		*/
	return TS_INVALID;
}

int controlarea::notify_update(const char *message)
{
	trace_message(TMT_CONTROLAREA,message);

	// ACTUAL GENERATION
	double rated, power, energy;
	if ( sscanf(message,TM_ACTUAL_GENERATION,&rated,&power,&energy)==3 )
	{
		capacity += rated;
		supply += power;
		inertia += energy;
		return 1;
	}

	// ACTUAL LOAD
	if ( sscanf(message,TM_ACTUAL_LOAD,&power,&energy)==2 )
	{
		demand += power;
		inertia += energy;
		return 1;
	}


	// REGISTER INTERTIE
	OBJECT *obj;
	if ( sscanf(message,TM_REGISTER_INTERTIE,&obj)==1 )
	{
		intertie_list = add_object(intertie_list,obj);
		gld_object *line = get_object(obj);
		if ( !line->isa("intertie") ) 
			exception("attempt by non-intertie object to register with interconnection as an intertie");
		n_intertie++;
		return 1;
	}

	error("update message '%s' is not recognized",message);
		/* TROUBLESHOOT
		   TODO
		*/
	return 0;
}

int controlarea::kmldump(int (*stream)(const char*, ...))
{
	if ( isnan(get_latitude()) || isnan(get_longitude()) ) return 0;
	stream("<Placemark>\n");
	stream("  <name>%s</name>\n", get_name());
	stream("  <description>\n<![CDATA[<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=3 STYLE=\"font-size:10;\">\n");
#define TR "    <TR><TH ALIGN=LEFT>%s</TH><TD ALIGN=RIGHT>%s</TD></TR>\n"
#define HREF "    <TR><TH ALIGN=LEFT><A HREF=\"%s_%s.png\"  ONCLICK=\"window.open('%s_%s.png');\">%s</A></TH><TD ALIGN=RIGHT>%s</TD></TR>\n"
	gld_clock now(my()->clock);
	stream("<caption>%s</caption>",(const char*)now.get_string());
	stream(TR,"Status",(const char*)get_status_string());
	stream(HREF,(const char*)get_name(),"actual",(const char*)get_name(),"actual","Actual",(const char*)get_actual_string());
	stream(HREF,(const char*)get_name(),"schedule",(const char*)get_name(),"schedule","Schedule",(const char*)get_schedule_string());
	stream(HREF,(const char*)get_name(),"capacity",(const char*)get_name(),"capacity","Capacity",(const char*)get_capacity_string());
	stream(HREF,(const char*)get_name(),"supply",(const char*)get_name(),"supply","Supply",(const char*)get_supply_string());
	stream(HREF,(const char*)get_name(),"demand",(const char*)get_name(),"demand","Demand",(const char*)get_demand_string());
	stream(HREF,(const char*)get_name(),"losses",(const char*)get_name(),"losses","Losses",(const char*)get_losses_string());
	stream(HREF,(const char*)get_name(),"imbalance",(const char*)get_name(),"imbalance","Imbalance",(const char*)get_imbalance_string());
	stream(HREF,(const char*)get_name(),"inertia",(const char*)get_name(),"inertia","Inertia",(const char*)get_inertia_string());
	stream(HREF,(const char*)get_name(),"bias",(const char*)get_name(),"bias","Bias",(const char*)get_bias_string());
	stream("  </TABLE>]]></description>\n");
	stream("  <styleUrl>#%s_mark_%s</styleUrl>\n",my()->oclass->name, (const char*)get_status_string());
	stream("  <Point>\n");
	stream("    <altitudeMode>relative</altitudeMode>\n");
	stream("    <coordinates>%f,%f,100</coordinates>\n", get_longitude(), get_latitude());
	stream("  </Point>\n");
	stream("</Placemark>\n");
#ifdef SHOWUNITS
	stream("<Folder><name>%s</name>\n","Generators");
	for ( OBJECT *obj=callback->object.get_first() ; obj!=NULL ; obj=obj->next )
	{
		gld_object *item = get_object(obj);
		if ( obj->oclass->module==my()->oclass->module && obj->parent==my() && item->isa("generator") )
			OBJECTDATA(obj,generator)->kmldump(stream);
	}
	stream("%s","</Folder>\n");

	stream("<Folder><name>%s</name>\n","Loads");
	for ( OBJECT *obj=callback->object.get_first() ; obj!=NULL ; obj=obj->next )
	{
		gld_object *item = get_object(obj);
		if ( obj->oclass->module==my()->oclass->module && obj->parent==my() && item->isa("load") )
			OBJECTDATA(obj,load)->kmldump(stream);
	}
	stream("%s","</Folder>\n");
#endif
	return 1;
}
