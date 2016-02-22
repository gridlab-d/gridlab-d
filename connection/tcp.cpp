// $Id$
//
// Implement the stream transport mechanism
//

#include "tcp.h"

tcp::tcp()
{
	// TODO add construction code here
}

/// udp pseudo-property handler
int tcp::option(char *command)
{
	// TODO
	return 0;
}

int tcp::create(void) 
{
	// TODO add creation code here
	return 1; /* return 1 on success, 0 on failure */
}

int tcp::init(void)
{
	// TODO add initialization code here
	return 1;
}
