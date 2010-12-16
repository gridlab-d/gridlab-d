/** $Id: legal.c 1182 2008-12-22 22:08:36Z dchassin $
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

#ifndef WIN32
#undef BUILD
#include "build.h"
#endif

/* branch names and histories (named after WECC 500kV busses)
	Allston			Version 1.0 originated at PNNL March 2007, released February 2008
	Buckley			Version 1.1 originated at PNNL January 2008, released April 2008
	Coulee			Version 1.2 originated at PNNL April 2008, released June 2008
	Coyote			Version 1.3 originated at PNNL June 2008, not released
	Diablo			Version 2.0 originated at PNNL August 2008
	Eldorado		Version 2.1 originated at PNNL September 2009
	Four Corners		Version 2.2 originated at PNNL November 2010
	Grizzly
	Hassyampa
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
	int surpress = global_suppress_repeat_messages;
	global_suppress_repeat_messages = 0;
	while ((end = strchr(copyright,'\n'))!=NULL) {
		*end = ' ';
	}
	if (find_file(copyright,NULL,FF_EXIST)==NULL)
	{
		int build = buildinfo ? atoi(strstr(BUILD,":")+1) : 0;
		if (build>0)
		{
			output_message("GridLAB-D Version %d.%02d.%03d (build %d of " BRANCH ")\n" COPYRIGHT
				"", global_version_major, global_version_minor,REV_PATCH,build);
		}
		else
			output_message("GridLAB-D Version %d.%02d." 
#ifdef WIN32
#ifdef _DEBUG
			"WIN32-DEBUG" 
#else
			"WIN32-RELEASE"
#endif
#else
			"DEV"
#endif
			" (" BRANCH ")\n" COPYRIGHT
				"", global_version_major, global_version_minor);
	}
	global_suppress_repeat_messages = surpress;
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

/**@}*/
