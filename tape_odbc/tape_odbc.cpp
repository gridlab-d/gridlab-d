/** $Id: tape_odbc.cpp,v 1.2 2008/01/15 22:03:00 d3p988 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file tape_odbc.cpp
	@addtogroup tape_odbc ODBC Tapes
	@ingroup tapes

	ODBC-type tapes work on databases.  The filename field is used as the command line to
	access and log into a database using ODBC (Open Database Connectivity).  Anonymous
	transactions are allowed by the module, but are dependent on the database.
	The ODBC model relies on locating a HEADER_TABLE, OBJECT_TABLE, and an EVENT_TABLE
	in the database.  Objects are written in mostly as a series of text fields.

	HEADER_TABLE
	 - HEADER_OBJECT_NAME - Text - The name of the referenced tape
	 - HEADER_TIME - Text - Asctime() that the tape was opened
	 - HEADER_USER - Text - Local computer username
	 - HEADER_HOST - Text - Local computer hostname
	 - HEADER_TARGET - Text - Target field for this tape, either and object name or aggregate definition
	 - HEADER_PROPERTY - Text - Property field for this tape
	 - HEADER_INTERVAL - Number - Interval for this tape recording values (-1 == on change)
	 - HEADER_LIMIT - Number - Maximum number of lines written for this tape

	OBJECT_TABLE and EVENT_TABLE
	 - EVENT_OBJECT_NAME - Text - Name of the tape object that wrote this line
	 - EVENT_LINE - Number - Nth line written to or to be read by the tape
	 - EVENT_TIME - Text - Timedate of the event, either in relative or absolute (YYYY-MM-DD HH:MM:SS) format
	 - EVENT_VAL - Text - Value written or read to the target.  Text for a double, complex, or enumeration.

	Future implimentations may change drastically.
@{
 **/

#include "tape_odbc.h"
#include "TapeStream.h"
#include "ODBCTapeStream.h"

EXPORT CALLBACKS *callback = NULL;

int open_player(struct player *my, char *fname, char *flags){
	//	returns ODBCTapeStream *
	my->tsp = ODBCTapeStream::OpenStream(my, fname, flags);
	if(NULL == my->tsp){
		gl_error("player DB %s: unable to connect.", fname);
		my->status=TS_DONE;
		return 0;
	}
	my->loopnum=my->loop;
	my->status=TS_OPEN;
	my->type=FT_ODBC;
	return 1;
}

char *read_player(struct player *my, char *buffer, unsigned int size){
	return ((ODBCTapeStream *)(my->tsp))->ReadLine(buffer, size);
}

int rewind_player(struct player *my){
	return ((ODBCTapeStream *)(my->tsp))->Rewind();
}

void close_player(struct player *my){
	ODBCTapeStream::CloseStream(my);
}

/*** SHAPER ***/
int open_shaper(struct shaper *my, char *fname, char *flags){
	/*	complicated things occur here.  just as soon as we figure out where
	 *	we store the shape data with ODBC. -mh	*/
	gl_error("ODBC does not support shapers (yet!)");
	return 0;
}

char *read_shaper(struct shaper *my,char *buffer,unsigned int size){
	return ((ODBCTapeStream *)(my->tsp))->ReadLine(buffer, size);
}

int rewind_shaper(struct shaper *my){
	return ((ODBCTapeStream *)(my->tsp))->Rewind();
}

void close_shaper(struct shaper *my){
	ODBCTapeStream::CloseStream(my);
}

/*** RECORDER ***/
int open_recorder(struct recorder *my, char *fname, char *flags){
	char buff[128];
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);

	my->tsp=ODBCTapeStream::OpenStream(my, fname, flags);
	if(NULL == my->tsp){
		gl_error("recorder DB %s: unable to connect.", fname);
		my->status=TS_DONE;
		return 0;
	}
	my->status=TS_OPEN;
	my->type=FT_ODBC;
	my->samples=0;
	sprintf(buff, "%s %d", obj->parent->oclass->name, obj->id);
#ifdef WIN32
	((ODBCTapeStream *)(my->tsp))->PrintHeader(asctime(localtime(&now)), getenv("USERNAME"), getenv("MACHINENAME"),
		buff/***/, my->property, my->trigger[0]=='\0'?"(none)":my->trigger,
		(long)my->interval, my->limit);
#else
	my->tsp->PrintHeader(asctime(localtime(&now)), getenv("USER"),
		getenv("HOST"), buff/***/, my->property, my->trigger[0]=='\0'?"(none)":my->trigger,
		my->interval, my->limit);
#endif
	return 1;
}

int write_recorder(struct recorder *my, char *timestamp, char *value){
	return ((ODBCTapeStream *)(my->tsp))->Write(timestamp, value);
}

void close_recorder(struct recorder *my){
	//	write "# EOF"?  maybe not?
	ODBCTapeStream::CloseStream(my);
}

/*** COLLECTOR ***/
/*	mostly a copy-paste of Recorder, with my->group instead of buff and the snprintf. -mh */
int open_collector(struct collector *my, char *fname, char *flags){
	time_t now=time(NULL);
	OBJECT *obj=OBJECTHDR(my);

	my->tsp=ODBCTapeStream::OpenStream(my, fname, flags);
	if(NULL == my->tsp){
		gl_error("collector DB %s: unable to connect.", fname);
		my->status=TS_DONE;
		return 0;
	}
	my->status=TS_OPEN;
	my->type=FT_ODBC;
	my->samples=0;
#ifdef WIN32
	((ODBCTapeStream *)(my->tsp))->PrintHeader(asctime(localtime(&now)), getenv("USERNAME"), getenv("MACHINENAME"),
		my->group, my->property, my->trigger[0]=='\0'?"(none)":my->trigger,
		(long)my->interval, my->limit);
#else
	my->tsp->PrintHeader(asctime(localtime(&now)), getenv("USER"),
		getenv("HOST"), my->group, my->property, my->trigger[0]=='\0'?"(none)":my->trigger,
		my->interval, my->limit);
#endif
	return 1;
}

int write_collector(struct collector *my, char *timestamp, char *value){
	return ((ODBCTapeStream *)(my->tsp))->Write(timestamp, value);
}

void close_collector(struct collector *my){
	//	write "# EOF"?  maybe not?
	ODBCTapeStream::CloseStream(my);
}

//	end of odbc.cpp
/**@}*/
