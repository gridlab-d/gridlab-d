/** $Id: server.h 4738 2014-07-03 00:55:39Z dchassin $
	@file server.h
	@addtogroup server
	@ingroup MODULENAME
        author: Jimmy Du & Kyle Anderson (GridSpice Project),  
                jimmydu@stanford.edu, kyle.anderson@stanford.edu

        Released in Open Source to Gridlab-D Project
 @{
 **/

#ifndef _server_H
#define _server_H

#include <stdarg.h>
#include "gridlabd.h"

class server {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
        int port;
public:
	/* required implementations */
	server(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static server *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
