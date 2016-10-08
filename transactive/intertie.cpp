// $Id: intertie.cpp 5218 2015-07-06 01:32:36Z dchassin $

#include "transactive.h"

EXPORT_IMPLEMENT(intertie)
EXPORT_SYNC(intertie)

intertie::intertie(MODULE *module)
{
	oclass = gld_class::create(module,"intertie",sizeof(intertie),PC_BOTTOMUP|PC_AUTOLOCK);
	oclass->trl = TRL_CONCEPT;
	defaults = this;
	if ( gl_publish_variable(oclass,
		PT_object,"from",get_from_offset(),PT_DESCRIPTION,"control area from which power flows",
		PT_object,"to",get_to_offset(),PT_DESCRIPTION,"control area to which power flows",
		PT_double,"capacity[MW]",get_capacity_offset(),PT_DESCRIPTION,"intertie power capacity",
		PT_double,"flow[MW]",get_flow_offset(),PT_DESCRIPTION,"intertie power flow",
		PT_double,"loss[pu]",get_loss_offset(),PT_DESCRIPTION,"intertie power loss",
		PT_double,"losses[MW]",get_losses_offset(),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"intertie power losses",
		PT_enumeration,"status",get_status_offset(),PT_DESCRIPTION,"status flag",
			PT_KEYWORD,"UNCONSTRAINED",(enumeration)TLS_UNCONSTRAINED,
			PT_KEYWORD,"OVERLIMIT",(enumeration)TLS_OVERLIMIT,
			PT_KEYWORD,"CONSTRAINED",(enumeration)TLS_CONSTRAINED,
			PT_KEYWORD,"NOFLOW",(enumeration)TLS_NOFLOW,
			PT_KEYWORD,"OFFLINE",(enumeration)TLS_OFFLINE,
		NULL)<1 )
		exception("unable to publish transactive intertie properties");
	memset(defaults,0,sizeof(intertie));
}

int intertie::create(void)
{
	status = TLS_UNCONSTRAINED;
	return 1;
}

int intertie::init(OBJECT *parent)
{
	// ignore offline interties
	if ( status==TLS_OFFLINE )
	{	}//return 1;
	else if ( capacity>0 )
		status = TLS_UNCONSTRAINED;
	else if ( capacity<0 )
		exception("intertie constraint cannot be negative");
		/* TROUBLESHOOT
		   TODO
		*/

	// get from and to objects
	from_area = get_object(from);
	to_area = get_object(to);

	// automatic naming
	if ( my()->name==NULL )
	{
		size_t len = strlen(from_area->get_name()) + strlen(to_area->get_name());
		my()->name = new char[len+2];
		sprintf(my()->name,"%s-%s",from_area->get_name(),to_area->get_name());
	}

	// check that from and to are both control areas
	if ( !from_area->isa("controlarea") || !to_area->isa("controlarea") )
		exception("from and to must refer to control areas");
		/* TROUBLESHOOT
		   TODO
		*/

	// check that from and to are not the same control area
	if ( from_area==to_area )
		exception("from and to must refer to different control areas");
		/* TROUBLESHOOT
		   TODO
		*/

	// check that from and to are both in the same interconnection
	if ( from_area->get_parent()!=to_area->get_parent() )
		exception("from and to must refer to control areas in the same interconnection");
		/* TROUBLESHOOT
		   TODO
		*/

	// register tieline with interconnection
	system = from_area->get_parent();
	gld_property system_update(system,"update");
	system_update.notify(TM_REGISTER_INTERTIE,my());

	// register tieline with control areas
	gld_property from_update(from_area,"update");
	from_update.notify(TM_REGISTER_INTERTIE,my());
	gld_property to_update(to_area,"update");
	to_update.notify(TM_REGISTER_INTERTIE,my());
	
	// check that the capacity is not negative
	if ( capacity<0 )
		exception("intertie capacity must not be negative");
		/* TROUBLESHOOT
		   TODO
		*/
	if ( capacity==0 && status!=TLS_OFFLINE )
		warning("zero capacity intertie will behave as though status is OFFLINE");
		/* TROUBLESHOOT
		   TODO
		*/
	else if ( capacity!=0 && status==TLS_UNCONSTRAINED && !(allow_constraint_violation&CV_INTERTIE) )
		warning("intertie constraint is ignored when status is UNCONSTRAINED");
		/* TROUBLESHOOT
		   TODO
		*/

	return 1;
}

int intertie::isa(const char *type)
{
	return strcmp(type,"intertie")==0;
}

TIMESTAMP intertie::presync(TIMESTAMP t1)
{
	exception("presync is not expected");
		/* TROUBLESHOOT
		   TODO
		*/
	return TS_INVALID;
}

TIMESTAMP intertie::sync(TIMESTAMP t1)
{
	losses = abs(flow)*loss;
	if ( verbose_options&VO_INTERTIE )
	{
		fprintf(stderr,"INTERTIE       : %s at %s\n", get_name(),(const char *)gld_clock().get_string());
		const char *tls[] = {"OFFLINE","CONSTRAINED","UNCONSTRAINED"};
		fprintf(stderr,"INTERTIE       :   status.............. %s\n", tls[status]);
		if ( status!=TLS_OFFLINE )
		{
			if ( flow>0 )
				fprintf(stderr,"INTERTIE       :   direction........... %s->%s\n", from->name,to->name);
			else
				fprintf(stderr,"INTERTIE       :   direction........... %s->%s\n", to->name,from->name);
			fprintf(stderr,"INTERTIE       :   flow................ %8.3f (%5.1f%%) %s\n", abs(flow),abs(flow)/capacity*100, capacity>0&&abs(flow)>capacity ? "!!!":"   ");
			fprintf(stderr,"INTERTIE       :   losses.............. %8.3f (%5.1f%%)\n", losses,losses/abs(flow)*100);
		}
	}

	// update status flag
	if ( status != TLS_OFFLINE )
	{
		if ( fabs(flow)>capacity ) status = TLS_OVERLIMIT;
		else if ( fabs(flow)>capacity*.99 ) status = TLS_CONSTRAINED;
		else if ( flow==0 ) status = TLS_NOFLOW;
		else status = TLS_UNCONSTRAINED;
	}
	return TS_NEVER;
}

TIMESTAMP intertie::postsync(TIMESTAMP t1)
{
	exception("postsync is not expected");
		/* TROUBLESHOOT
		   TODO
		*/
	return TS_NEVER;
}

int intertie::kmldump(int (*stream)(const char*, ...))
{
	if ( isnan(from->latitude) || isnan(to->latitude) || isnan(from->longitude) || isnan(to->longitude) ) return 0;
	const char *dir="-";
	switch (status) {
	case TLS_OFFLINE: dir = "&gt;&lt;"; break;
	case TLS_UNCONSTRAINED: dir = ( flow<0?"&lt;-":"-&gt;" ); break;
	case TLS_CONSTRAINED: dir = ( flow<0?"|&lt;":"&gt;|" ); break;
	case TLS_OVERLIMIT: dir = ( flow<0?"&lt;&lt;":"&gt;&gt;" ); break;
	case TLS_NOFLOW:	dir = "--"; break;
	default: dir = "??"; break;
	}
	stream("<Placemark>\n");
	stream("  <name>%s -%s- %s</name>\n", from->name,dir,to->name);
	stream("  <description>\n<![CDATA[\n");
	stream("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=3 STYLE=\"font-size:10;\">\n");
#define TR "<TR><TH ALIGN=LEFT>%s</TH><TD ALIGN=RIGHT>%s</TD></TR>\n"
	gld_clock now(my()->clock);
	stream("<caption>%s %s</caption>",get_name(), (const char*)now.get_string());
	stream(TR,"Status",(const char*)get_status_string());
	stream(TR,"Capacity",(const char*)get_capacity_string());
	stream(TR,"Flow",(const char*)get_flow_string());
	stream(TR,"Losses",(const char*)get_losses_string());
	stream("</TABLE>\n");
	stream("    ]]>\n");
	stream("      </description>\n");
	stream("  <styleUrl>#%s_path_%s</styleUrl>\n",my()->oclass->name,(const char*)get_status_string());
	stream("  <LineString>\n");			
	stream("    <extrude>0</extrude>\n");
	stream("    <tessellate>1</tessellate>\n");
	stream("    <altitudeMode>relative</altitudeMode>\n");
	stream("    <coordinates>%f,%f,100 %f,%f,100</coordinates>\n",
		from->longitude,from->latitude,to->longitude,to->latitude);
	stream("  </LineString>\n");			
	stream("</Placemark>\n");
	return 1;
}
