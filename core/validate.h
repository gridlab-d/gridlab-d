/* $Id: validate.h 4738 2014-07-03 00:55:39Z dchassin $
   Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _VALIDATE_H
#define _VALIDATE_H

#include "platform.h"

typedef enum {
	VO_NONE		=0x0000,	///< run no tests (just go through motions)

	// tests
	VO_TSTRUN	=0x0001,	///< run passing tests
	VO_TSTERR	=0x0002,	///< run error tests
	VO_TSTEXC	=0x0004,	///< run exception tests
	VO_TSTSTD	=0x0007,	///< run normal validation tests (run+err+exc)
	VO_TSTOPT	=0x0008,	///< run optional tests
	VO_TSTALL	=0x000f,	///< run all tests

	// test override
	VO_TSTREQ	=0x0010,	///< make all tests required (not supported yet)
	VO_TSTOPTREQ=0x0018,	///< make optional test required (not supported yet)
	VO_TSTALLREQ=0x001f,	///< run all test with optionals required (not supported yet)

	// reports
	VO_RPTDIR   =0x0100,	///< report includes individual directories search results
	VO_RPTGLM	=0x0200,	///< report includes individual run results
	VO_RPTALL	=0x0300,	///< report includes everything

	VO_DEFAULT	=VO_TSTALL|VO_RPTALL,

} VALIDATEOPTIONS;

#ifdef __cplusplus
extern "C" {
#endif

int validate(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif

