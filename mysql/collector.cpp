/** $Id$
    DP Chassin - July 2012
    Copyright (C) 2012 Battelle Memorial Institute
 **/

#ifdef HAVE_MYSQL

#include "database.h"

EXPORT_CREATE(collector);
EXPORT_INIT(collector);

CLASS *collector::oclass = NULL;
collector *collector::defaults = NULL;

collector::collector(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"collector",sizeof(collector),PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class collector";
		else
			oclass->trl = TRL_PROTOTYPE;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_char1024,"property",get_property_offset(),PT_DESCRIPTION,"target property name",
			PT_char32,"trigger",get_trigger_offset(),PT_DESCRIPTION,"collector trigger condition",
			PT_char32,"table",get_table_offset(),PT_DESCRIPTION,"table name to store samples",
			PT_char32,"mode",get_mode_offset(),PT_DESCRIPTION,"table output mode",
			PT_int32,"limit",get_limit_offset(),PT_DESCRIPTION,"maximum number of records to output",
			PT_double,"interval[s]",get_interval_offset(),PT_DESCRIPTION,"sampling interval",
			PT_object,"connection",get_connection_offset(),PT_DESCRIPTION,"database connection",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(collector));
	}
}

int collector::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	return 1; /* return 1 on success, 0 on failure */
}

int collector::init(OBJECT *parent)
{
	gl_error("mysql::collector is not supported yet");
	return 0;
}

#endif // HAVE_MYSQL
