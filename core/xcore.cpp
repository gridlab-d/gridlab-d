/* xcore.cpp
 *
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <pthread.h>

#include "output.h"

static Display *dsp = NULL;
static Window win;
static GC gc;
static Font font;
static PRINTFUNCTION stdprint=NULL;
static PRINTFUNCTION errprint=NULL;

bool is_server = 1;
int pfd[2];

extern "C" int xoutput(char *format, ...)
{
	char text[4096];
        int count;
        va_list ptr;
        va_start(ptr,format);
        count = sprintf(text,format,ptr);
	va_end(ptr);

	static int x=0;
	static int y=30;
	static int dy=10;
	XTextItem items[] = {text,count,10,font};
	XDrawText(dsp,win,gc,x,y+=dy,items,1);
}

void xstreaminit(void)
{
	stdprint = output_set_stdout(xoutput);
	errprint = output_set_stderr(xoutput);
}

void xstreamdone(void)
{
	output_set_stdout(stdprint);
	output_set_stderr(errprint);
}

void xbutton(int x, int y, char *text)
{
	XDrawRectangle(dsp,win,gc,x,y,strlen(text)*12,16);
	XTextItem items[] = {text,strlen(text),10,font};
	XDrawText(dsp,win,gc,x,y+12,items,1);
}

void* xmainloop(void*arg)
{
	int32 eventMask = StructureNotifyMask;
	XSelectInput( dsp, win, eventMask );

	eventMask = ButtonPressMask|ButtonReleaseMask;
	XSelectInput(dsp,win,eventMask); // override prev

	XEvent evt;
	do
	{
		XNextEvent( dsp, &evt );   // calls XFlush()
	} while( evt.type != ButtonRelease );

	xstreamdone();

	XDestroyWindow( dsp, win );
	XCloseDisplay( dsp );

	return 0;
}

extern "C" void xrejoin(void)
{
	xoutput("Simulation done.");	
}

extern "C" int xstart(void)
{
	dsp = XOpenDisplay( NULL );
	if( !dsp ) return 1;

	int screenNumber = DefaultScreen(dsp);
	uint32 white = WhitePixel(dsp,screenNumber);
	uint32 black = BlackPixel(dsp,screenNumber);

	win = XCreateSimpleWindow(dsp,
		DefaultRootWindow(dsp),
		50, 50,   // origin
		200, 200, // size
		0, black, // border
		white );  // backgd

	XMapWindow( dsp, win );

	int32 eventMask = StructureNotifyMask;
	XSelectInput( dsp, win, eventMask );

	XEvent evt;
	do
	{
		XNextEvent( dsp, &evt );   // calls XFlush
	} while( evt.type != MapNotify );

	gc = XCreateGC( dsp, win,
		0,        // mask of values
		NULL );   // array of values
	XSetForeground( dsp, gc, black );

	XSetStandardProperties(dsp, win, "GridLAB-D", "GridLAB-D", None,NULL,0,NULL);

	font = XLoadFont(dsp,"*-lucida-bold-r-normal-*-12-*");

	xstreaminit();

	xbutton(10,10,"Quit");

	pthread_t pt_info;
	if (pthread_create(&pt_info,NULL,xmainloop,NULL))
		perror("xstart(pthread_create)");

	is_server = 1;
//	atexit(xrejoin);
	xoutput("Beginning simulation...");
	return 1;
}

