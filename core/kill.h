/** $Id: kill.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#ifndef _KILL_H
#define _KILL_H

#ifdef WIN32
void kill_starthandler(void);
void kill_stophandler(void);
int kill(unsigned short pid, int sig);
#endif

#endif
