// $Id: setup.cpp 4738 2014-07-03 00:55:39Z dchassin $

#define CONSOLE /* enables console/curses support */
#include "gridlabd.h"
#include "load.h"
#include "convert.h"

#ifdef HAVE_CURSES
int height=0, width=0;
char status[1024] = "Ready";
char blank[1024];

static bool edit_bool(int row, int col, int len, PROPERTY *prop)
{
	bool value;
	char string[1024];
	while ( true )
	{
		int key = wgetch(stdscr);
		if ( key==KEY_ENTER ) break;
		if ( key==KEY_ESC ) return false;
		if ( key=='Y' || key=='T' || key=='1' || key==KEY_DOWN ) value = true;
		if ( key=='N' || key=='F' || key=='0' || key==KEY_UP ) value = false;
		if ( convert_from_boolean(string,sizeof(string),&value,prop) )
			mvprintw(row,col,"%-.*s",len,string);
	}
	return value;
}

static bool edit_in_place(int row, int col, int len, PROPERTY *prop)
{
	mvprintw(row,col-1,"[");
	if ( prop->width>0 && prop->width<len ) len=prop->width;
	if ( col+len>width-2 ) len = width-2-col;
	mvprintw(row,col+len+1,"]");
	int key;
	while ( true )
	{
		key = wgetch(stdscr);
		if ( key==KEY_ESC ) goto Escape;
		if ( key==KEY_ENTER ) goto Return;
	}
Escape:
	mvprintw(row,col-1," ");
	mvprintw(row,col+len+1," ");
	return false;
Return:
	mvprintw(row,col-1," ");
	mvprintw(row,col+len+1," ");
	return true;
}

static int edit_globals(void)
{
	int i;

	// global variable list
	static GLOBALVAR **var = NULL;
	static size_t nvars=0;
	static int nsize=0; // max name size
	if ( var==NULL )
	{	// load the var list
		GLOBALVAR *v = NULL;
		while ( (v=global_getnext(v)) ) 
			nvars++;
		var = (GLOBALVAR**)malloc(sizeof(GLOBALVAR*)*nvars);
		nvars=0, v=NULL;
		while ( (v=global_getnext(v)) ) 
		{
			var[nvars++] = v;
			int s = strlen(v->prop->name);
			if ( s>nsize ) nsize=s;
		}
	}

	// window pos and selection
	static int first=0, sel=0, vsize=height-5;
	int last = first+vsize;
	if ( last>=nvars ) last=nvars-1;

	// write data
	int row=4;
	for ( i=first ; i<last ; i++, row++ )
	{
		char value[1024];
		if ( global_getvar(var[i]->prop->name,value,sizeof(value)) )
		{
			// remove quotes
			char *v = value;
			if ( v[0]=='\'' || v[0]=='"' )
			{
				v++;
				v[strlen(v)-1]='\0';
			}

			int selected = (i==sel);
			if ( selected ) attron(A_BOLD);
			mvprintw(row,0,"%s",blank);
			mvprintw(row,0,"%-*s",nsize,var[i]->prop->name);
			mvprintw(row,nsize+1,"%-*s",width-nsize-1,v); 
			if ( selected ) attroff(A_BOLD);
		}
	}

	int key;	
	while ( (key=wgetch(stdscr))==ERR ) 
	{}
	switch ( key ) {	
	case KEY_DOWN:
		if ( sel<nvars-2 )
		{
			sel++;
			if ( sel>=last && first<nvars-vsize-1)
				first++;
		}
		break;
	case KEY_UP:
		if ( sel>0 )
		{
			sel--;
			if ( sel<first ) first=sel;
		}
		break;
	case KEY_ENTER:
		if ( var[sel]->prop->access==PA_PUBLIC )
			edit_in_place(4+sel-first,nsize+1,width-nsize-1,var[sel]->prop);
		break;
	default:
		//sprintf(status,"key=%d", key);
		break;
	}
	if ( var[sel]->prop->access==PA_PUBLIC )
		strcpy(status,"Hit <Return> to edit this variable.");
	else
		strcpy(status,"This variable cannot be changed.");

	return key; // pass up for global input handling, 0 does nothing
}
static int edit_environment(void)
{
	int key;	
	while ( (key=wgetch(stdscr))==ERR ) {}
	// TODO handle input locally
	return key; // pass up for global input handling, 0 does nothing
}
static int edit_macros(void)
{
	int key;	
	while ( (key=wgetch(stdscr))==ERR ) {}
	// TODO handle input locally
	return key; // pass up for global input handling, 0 does nothing
}
static int edit_config(void)
{
	int key;	
	while ( (key=wgetch(stdscr))==ERR ) {}
	// TODO handle input locally
	return key; // pass up for global input handling, 0 does nothing
}
static void show_help(void)
{
	int key;
	while ( (key=wgetch(stdscr))==ERR ) {}
}
static bool do_quit(void)
{
	char fname[1024];
	sprintf(fname,"gridlabd-%s.conf",getenv("USER"));
	mvprintw(height,0,"Save to '%s' (Y/N)? [Y]",fname); 
	return true;
}

typedef struct {
	char *name;
	int (*edit)(void);
} SETUPGROUP;
SETUPGROUP group[] = {
	{"Globals",edit_globals},
	{"Environment",edit_environment},
	{"Macros",edit_macros},
	{"Config",edit_config},
};
#endif

extern "C" int setup(int argc, char *argv[])
{
#ifdef HAVE_CURSES
	if ( !loadall(NULL) )
		sprintf(status,"ERROR: %s","unable to load configuration files");

	bool done = false;
	int tab = 0;
	int ntabs = sizeof(group)/sizeof(group[0]);
	initscr();
	halfdelay(1);
	clear();

	int i;
	for ( i=0 ; i<width ; i++ ) blank[i]=' ';
	blank[i]='\0';

	while ( !done )
	{
		int i;
		height = getheight();
		width = getwidth();
		char hline[1024];
		memset(hline,0,sizeof(hline));
		for ( i=0; i<width; i++ ) hline[i]='-';

		// header
		mvprintw(0,0,"GridLAB-D %d.%d.%d-%d (%s) Setup Editor",
			global_version_major, global_version_minor, global_version_patch, global_version_build,global_version_branch);
		mvprintw(1,0,"%s",hline);
		
		// tabs
		int pos=0;
		for ( i=0 ; i<ntabs ; i++ )
		{
			if ( i==tab ) attron(A_BOLD);
			mvprintw(2,pos,"[%s]",group[i].name);
			if ( i==tab ) attroff(A_BOLD);
			pos += strlen(group[i].name)+3;
		}
		mvprintw(3,0,"%s",hline);

		// post status
		mvprintw(height-1,0,"%s",hline);
		mvprintw(height,0,"%-*s",width-1,blank);
		mvprintw(height,0,"%-*s",width-1,status);

		// display page
		int key = group[tab].edit();

		// position for next status
		mvprintw(height,0,"");

		switch ( key ) {
		case 'Q': done=do_quit(); break;
		case 'H': show_help(); break;
		case KEY_LEFT: 
			if ( tab>0 ) tab--; 
			clear();
			break;
		case KEY_RIGHT: 
			if ( tab<ntabs-1 ) tab++; 
			clear();
			break;
		// TODO handle other keys
		default:
			break;
		}
	}
#else
	output_error("unable to display setup screen without curses library");
#endif
	return 0;
}
