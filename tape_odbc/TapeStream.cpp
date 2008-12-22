/*	TapeStream.cpp
 *	Copyright (C) 2008 Battelle Memorial Institute

 *	author: Matt Hauer, matthew.hauer@pnl.gov, 5/30/07 - ***
 */

#include "TapeStream.h"

TapeStream::TapeStream(){
	Reset();
}

TapeStream::~TapeStream(){
	;
}

void TapeStream::SetFileMode(char *mode){
	strncpy(filemode, mode, 2);
	filemode[2]=0;
}

void TapeStream::Reset(){
	state=TSO_DONE;
	strncpy(filemode, "r", 3);
}
//	end of TapeStream.cpp
