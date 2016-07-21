/* $Id: job.h 4738 2014-07-03 00:55:39Z dchassin $
   Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _JOB_H
#define _JOB_H

#include "platform.h"

typedef enum {
	JO_NONE		= 0x0000,	///< no job options
	JO_SORTA	= 0x0001,	///< run jobs in alphabetical order
	JO_SORTN	= 0x0002,	///< run jobs in numeric order
	JO_SUBDIR	= 0x0004,	///< run jobs in subdirectories
	JO_CLEAN	= 0x0008,	///< clean subdirectories
	JO_DEFAULT	= JO_SORTA,	///< default job options
} JOBOPTIONS;

#ifdef __cplusplus
extern "C" {
#endif

int job(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif

