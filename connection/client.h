/** $Id$

 Object class header

 **/

#ifndef _CLIENT_H
#define _CLIENT_H

#include "gridlabd.h"
#include "connection.h"

class client : public connection_mode {
public:
	// TODO add public data here

private:
	// TODO add private data here

public:
	/* required implementations */
	client();
	int create(void);
	int init(void);
	// TODO add other event handlers here

	// utilities
	CONNECTIONMODE get_mode() { return CM_CLIENT;};
	const char *get_mode_name(void) { return "client"; }; ///< get a string describing the mode
	int option(char *command);
};

#endif // _CLIENT_H
