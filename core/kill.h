/** $Id: kill.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#ifndef _KILL_H
#define _KILL_H

#ifdef WIN32
void kill_starthandler(void);
void kill_stophandler(void);
int kill(pid_t pid, int sig);
#endif

#endif
