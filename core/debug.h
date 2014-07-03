/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file debug.h
	@version $Id$
	@addtogroup debug
	@ingroup core
 @{
 **/

#ifndef _DEBUG_H
#define _DEBUG_H

void exec_sighandler(int sig);
int exec_debug(struct sync_data *data, int pass, int index, OBJECT *obj);
char *strsignal(int sig);

#endif
/**@}*/
