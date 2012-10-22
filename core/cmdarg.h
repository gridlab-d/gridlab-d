/** $Id: cmdarg.h 1182 2008-12-22 22:08:36Z dchassin $
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

