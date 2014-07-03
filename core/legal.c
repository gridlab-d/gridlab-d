/** $Id: legal.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file legal.c
	@author David P. Chassin
	@addtogroup legal Version, Copyright, License, etc.
	@ingroup core

	The branch version are named after 500kV busses on the WECC system.
	The following GridLAB-D branches are planned and/or released:

	- <b>Allson</b>: this is the developmental release for the charter partners
	  of the initial release
    - <b>Buckley</b>: this is the planned first release for the public after
	  the charter partners have completed their first modules.

	@section License
	GridLAB-D is provided under a BSD-style license modified appropriately
	for government-funded software.  See legal_license() for details.

	The	restrictions forbid the removal of the copyright and legal notices.
	This is not actually a restriction, but a reminder of what goes
	without saying in a world where common sense is more common.

	The	restrictions also preclude the use of any contributor's name as an endorsement.
	This should not be interpreted as an actual restriction either, but a clarification
	because without the clause, the license does not grant recipients of the
	software permission to use the licensor's name anyway.  But it makes the legal
	types happy.  That's a good thing.

	See \ref License "The GridLAB License" for details.

 @{
 **/

#include "version.h"

#include "globals.h"
#include "legal.h"
#include "output.h"
#include "find.h"

/* branch names and histories (named after WECC 500kV busses)
	Allston			Version 1.0 originated at PNNL March 2007, released February 2008
	Buckley			Version 1.1 originated at PNNL January 2008, released April 2008
	Coulee			Version 1.2 originated at PNNL April 2008, released June 2008
	Coyote			Version 1.3 originated at PNNL June 2008, not released
	Diablo			Version 2.0 originated at PNNL August 2008
	Eldorado		Version 2.1 originated at PNNL September 2009
	Four Corners	Version 2.2 originated at PNNL November 2010
	Grizzly			Version 2.3 originated at PNNL November 2011
	Hassayampa		Version 3.0 originated at PNNL November 2011
	Hatwai
	Jojoba
	Keeler
	Lugo
	McNary
	Navajo
	Ostrander
	Palo Verde
	Perkins
	Redhawk
	Sacajawea
	Tesla
	Troutdale
	Vantage
	Vincent
	Westwing
	Yavapai
	(continue with 345kV WECC busses on WECC after this)
 */

/** Displays the current legal banner
	@return SUCCESS when conditions of use have been satisfied, FAILED when conditions of use have not been satisfied
 **/
STATUS legal_notice(void)
{
	char *buildinfo = strstr(BUILD,":");

	/* suppress copyright info if copyright file exists */
	char copyright[1024] = "GridLAB-D " COPYRIGHT;
	char *end = strchr(copyright,'\n');
	int suppress = global_suppress_repeat_messages;
	char path[1024];
	global_suppress_repeat_messages = 0;
	while ((end = strchr(copyright,'\n'))!=NULL) {
		*end = ' ';
	}
	if (find_file(copyright,NULL,R_OK,path,sizeof(path))==NULL)
	{
		int build = buildinfo ? atoi(strstr(BUILD,":")+1) : 0;
		output_message("GridLAB-D %d.%d.%d-%d (%s) %d-bit %s %s\n%s", 
			global_version_major, global_version_minor, global_version_patch, global_version_build, 
			global_version_branch, 8*sizeof(void*), global_platform,
#ifdef _DEBUG
		"DEBUG",
#else
		"RELEASE",
#endif
		COPYRIGHT);
	}
	global_suppress_repeat_messages = suppress;
	return SUCCESS; /* conditions of use have been met */
}

/** Displays the current user license
	@return SUCCESS when conditions of use have been satisfied, FAILED when conditions of use have not been satisfied
 **/
STATUS legal_license(void)
{
	int surpress = global_suppress_repeat_messages;
	global_suppress_repeat_messages = 0;
	output_message(
		COPYRIGHT
		"\n"
		"1. Battelle Memorial Institute (hereinafter Battelle) hereby grants\n"
		"   permission to any person or entity lawfully obtaining a copy of\n"
		"   this software and associated documentation files (hereinafter \"the\n"
		"   Software\") to redistribute and use the Software in source and\n"
		"   binary forms, with or without modification.  Such person or entity\n"
		"   may use, copy, modify, merge, publish, distribute, sublicense,\n"
		"   and/or sell copies of the Software, and may permit others to do so,\n"
		"   subject to the following conditions:\n"
		"   - Redistributions of source code must retain the above copyright\n"
		"     notice, this list of conditions and the following disclaimers.\n"
		"   - Redistributions in binary form must reproduce the above copyright\n"
		"     notice, this list of conditions and the following disclaimer in\n"
		"     the documentation and/or other materials provided with the\n"
		"     distribution.\n"
		"   - Other than as used herein, neither the name Battelle Memorial\n"
		"     Institute or Battelle may be used in any form whatsoever without\n"
		"     the express written consent of Battelle.\n"
		"2. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
		"   \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
		"   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
		"   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR\n"
		"   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n"
		"   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n"
		"   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n"
		"   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY\n"
		"   OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
		"   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
		"   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
		"3. The Software was produced by Battelle under Contract No.\n"
		"   DE-AC05-76RL01830 with the Department of Energy.  The U.S. Government\n"
		"   is granted for itself and others acting on its behalf a nonexclusive,\n"
		"   paid-up, irrevocable worldwide license in this data to reproduce,\n"
		"   prepare derivative works, distribute copies to the public, perform\n"
		"   publicly and display publicly, and to permit others to do so.  The\n"
		"   specific term of the license can be identified by inquiry made to\n"
		"   Battelle or DOE.  Neither the United States nor the United States\n"
		"   Department of Energy, nor any of their employees, makes any warranty,\n"
		"   express or implied, or assumes any legal liability or responsibility\n"
		"   for the accuracy, completeness or usefulness of any data, apparatus,\n"
		"   product or process disclosed, or represents that its use would not\n"
		"   infringe privately owned rights.\n"
		"\n"
		);
	global_suppress_repeat_messages = surpress;
	return SUCCESS;
}

/**************************************************************************************
 
 CHECK VERSION

 The purpose of check_version is to look for a file located at 
 http://www.gridlabd.org/versions.xml or http://www.gridlabd.org/versions.txt
 and verify whether the patch and build of this version is the latest one for the 
 major/minor of this version.  If the patch or build is not the latest, a warning 
 notice is displayed.

 **************************************************************************************/
#include <pthread.h>
#include <ctype.h>

#include "http_client.h"

static pthread_t check_version_thread_id;

#define CV_NOINFO 0x0001
#define CV_BADURL 0x0002
#define CV_NODATA 0x0004
#define CV_NEWVER 0x0008
#define CV_NEWPATCH 0x0010
#define CV_NEWBUILD 0x0020

void *check_version_proc(void *ptr)
{
	int patch, build;
	char *url = "http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/core/versions.txt";
	HTTPRESULT *result = http_read(url,0x1000);
	char target[32];
	char *pv = NULL, *nv = NULL;
	int rc = 0;
	int mypatch = REV_PATCH;
	int mybuild = atoi(BUILDNUM);

	/* if result not found */
	if ( result==NULL || result->body.size==0 )
	{
		output_warning("check_version: unable to read %s", url);
		rc=CV_NOINFO;
		goto Done;
	}

	/** @todo check the version against latest available **/
	if ( result->status>0 && result->status<400 )
	{
		output_warning("check_version: '%s' error %d", url, result->status);
		rc=CV_BADURL;
		goto Done;
	}

	/* read version data */
	sprintf(target,"%d.%d:",REV_MAJOR,REV_MINOR);
	pv = strstr(result->body.data,target);
	if ( pv==NULL )
	{
		output_warning("check_version: '%s' has no entry for version %d.%d", url, REV_MAJOR, REV_MINOR);
		rc=CV_NODATA;
		goto Done;
	}
	if ( sscanf(pv,"%*d.%*d:%d:%d", &patch, &build)!=2 )
	{
		output_warning("check_version: '%s' entry for version %d.%d is bad", url, REV_MAJOR, REV_MINOR);
		rc=CV_NODATA;
		goto Done;
	}

	nv = strchr(pv,'\n');
	if ( nv!=NULL )
	{
		while ( *nv!='\0' && isspace(*nv) ) nv++;
		if ( *nv!='\0' )
		{
			output_warning("check_version: newer versions than %s (Version %d.%d) are available", BRANCH, REV_MAJOR, REV_MINOR);
			rc|=CV_NEWVER;
		}
		/* not done yet */
	}
	if ( mypatch<patch )
	{
		output_warning("check_version: a newer patch of %s (Version %d.%d.%d-%d) is available", BRANCH, REV_MAJOR, REV_MINOR, patch, build);
		rc|=CV_NEWPATCH;
		/* not done yet */
	}
	if ( mybuild>0 && mybuild<build )
	{
		output_warning("check_version: a newer build of %s (Version %d.%d.%d-%d) is available", BRANCH, REV_MAJOR, REV_MINOR, patch, build);
		rc|=CV_NEWBUILD;
	}
	if ( rc==0 )
		output_verbose("this version is current");

	/* done */
Done:
	http_delete_result(result);
	return (void*)rc;
}

void check_version(int mt)
{
	/* start version check thread */
	if ( mt==0 || pthread_create(&check_version_thread_id,NULL,check_version_proc,NULL)!=0 )
	{
		/* unable to create a thread to do this so just do it inline (much slower) */
		check_version_proc(NULL);
	}
}

/**@}*/
