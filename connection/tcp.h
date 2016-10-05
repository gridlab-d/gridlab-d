/** $Id$

 Object class header

 **/

#ifndef _TCP_H
#define _TCP_H

#include "gridlabd.h"
#include "connection.h"

class tcp {
public:
	// utilities
	void error(char *fmt, ...);
	void warning(char *fmt, ...);
	void info(char *fmt, ...);
	void debug(int level, const char *fmt, ...);
	void exception(const char *fmt, ...);

	// utilities
	inline CONNECTIONTRANSPORT get_transport() { return CT_TCP;};
	const char *get_transport_name(void) { return "tcp";} ;
	int option(char *command);

public:
	// TODO add public data here

private:
	// TODO add private data here

public:
	/* required implementations */
	tcp();
	int create(void);
	int init(void);
	// TODO add other event handlers here
};

#endif // _TCP_H
