/** $Id: server.cpp 4738 2014-07-03 00:55:39Z dchassin $
	@file server.cpp
	@defgroup server Template for a new object class
	@ingroup MODULENAME

	You can add an object class to a module using the \e add_class
	command:
	<verbatim>
	linux% add_class CLASS
	</verbatim>

	You must be in the module directory to do this.

        author: Jimmy Du & Kyle Anderson (GridSpice Project),  
                jimmydu@stanford.edu, kyle.anderson@stanford.edu

        Released in Open Source to Gridlab-D Project
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "server.h"
#include "mongoose.h"
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

CLASS *server::oclass = NULL;
server *server::defaults = NULL;

#ifdef OPTIONAL
/* TODO: define this to allow the use of derived classes */
CLASS *PARENTserver::pclass = NULL;
#endif

/* TODO: remove passes that aren't needed */
static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN;

/* TODO: specify which pass the clock advances */
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
server::server(MODULE *module)
#ifdef OPTIONAL
/* TODO: include this if you are deriving this from a superclass */
: SUPERCLASS(module)
#endif
{
#ifdef OPTIONAL
	/* TODO: include this if you are deriving this from a superclass */
	pclass = SUPERCLASS::oclass;
#endif
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"server",sizeof(server),passconfig);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			/* TODO: add your published properties here */
                        PT_int16, "port", PADDR(port), PT_DESCRIPTION, "server port",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(server));
		/* TODO: set the default values of all properties here */
	}
}
/* Object creation is called once for each object that is created by the core */
int server::create(void)
{
	memcpy(this,defaults,sizeof(server));
	/* TODO: set the context-free initial value of properties, such as random distributions */
	return 1; /* return 1 on success, 0 on failure */
}

static int send_element_headers(struct mg_event * event) {
      std::string result;
      std::stringstream stream;
      gld_object *gld_obj;
      char from[1024] = "";
      char to[1024] = "";
      stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
      stream << "<Elements>\n";
      for (gld_obj=gld_object::get_first(); (gld_obj->my())!=NULL; gld_obj=(gld_obj->get_next()))
      {
	  int has_location = !(isnan(gld_obj->get_latitude()) || isnan(gld_obj->get_longitude()) \
			     || (gld_obj->get_name() == NULL) || (gld_obj->get_oclass() == NULL));
	  if (has_location)
	  {
	      stream << "\t<" << gld_obj->get_oclass()->get_name() << ">\n";
	      stream << "\t\t<name>" << gld_obj->get_name() << "</name>\n";
	      stream << "\t\t<latitude>" << std::fixed << std::setprecision(6) << \
		      gld_obj->get_latitude() << "</latitude>\n";
	      stream << "\t\t<longitude>" << std::fixed << std::setprecision(6) << \
		      gld_obj->get_longitude() << "</longitude>\n";
	      stream << "\t<" <<  "/" << gld_obj->get_oclass()->get_name() << ">\n";   
	  } else
	  {
	      gld_property fromProperty(gld_obj, "from");
	      gld_property toProperty(gld_obj, "to");
	      if (fromProperty.get_property() && toProperty.get_property() && (gld_obj->get_oclass())) {
		  int hasFrom = fromProperty.to_string(from, sizeof(from));
		  int hasTo = toProperty.to_string(to, sizeof(to));
		  if (hasFrom && hasTo)
		  {
		      stream << "\t<" << gld_obj->get_oclass()->get_name() << ">\n";
		      stream << "\t\t<name>" << gld_obj->get_name() << "</name>\n";
		      stream << "\t\t<from>" << from << "</from>\n";
		      stream << "\t\t<to>" << to << "</to>\n";
		      stream << "\t<" << "/" << gld_obj->get_oclass()->get_name() << ">\n";
		  }
	      }
	  }
      }

      stream << "</Elements>\n";
      result = stream.str();

      mg_printf(event->conn,
	    "HTTP/1.1 200 OK\r\n"
	    "Content-Type: text/xml\r\n"
	    "Content-Length: %d\r\n"
	    "\r\n"
	    "%s",
	    result.length(), result.c_str());
   
      return 1;
} 


static int event_handler(struct mg_event *event) {
  if (event->type == MG_REQUEST_BEGIN) {
	  if (strcmp(event->request_info->uri, "/element_headers.xml") == 0) {
              return send_element_headers(event); 
          }
  }

  return 0;
}


/* Object initialization is called once after all object have been created */
int server::init(OBJECT *parent)
{
	/* TODO: set the context-dependent initial value of properties */
	gl_warning("connecting server on port %d", port);
	struct mg_context *ctx;
	char portString[10];
	sprintf(portString, "%d", port);

        char path[1024];
	if ( callback->file.find_file("rest/gui_root", NULL, X_OK, path, sizeof(path)) != NULL) {
		gl_warning("%s\n", path);
		// List of options. Last element must be NULL.
		const char *options[] = {
			"listening_ports", portString,
			"document_root", path,
			NULL
		};
		ctx = mg_start(options, &event_handler, NULL);	
		return 1; /* return 1 on success, 0 on failure */
	}
	return 0;
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP server::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP server::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement bottom-up behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP server::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_server(OBJECT **obj)
{
	try
	{
		*obj = gl_create_object(server::oclass);
		if (*obj!=NULL)
			return OBJECTDATA(*obj,server)->create();
	}
	catch (char *msg)
	{
		gl_error("create_server: %s", msg);
	}
	return 0;
}

EXPORT int init_server(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,server)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_server(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_server(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	server *my = OBJECTDATA(obj,server);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	catch (char *msg)
	{
		gl_error("sync_server(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return TS_INVALID;
}
