/** $Id: cmdarg.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file cmdarg.h
	@addtogroup cmdarg
 @{
 **/


#ifndef _CMDARG_H
#define _CMDARG_H

#ifdef __cplusplus
extern "C" {
#endif

STATUS cmdarg_load(int argc,char *argv[]);
int cmdarg_runoption(const char *value);

#ifdef __cplusplus
}
#endif

#endif
/**@}**/

