/* $Id: console.h 4738 2014-07-03 00:55:39Z dchassin $
 * Copyright (C) Battelle Memorial Institute 2012
 * DP Chassin
 *
 * Console functions and classes 
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#define KEY_ESC 27

/* simulate needed curses functions in Windows */
#ifdef _WIN32
#include <windows.h>
#undef HAVE_CURSES
#define HAVE_CURSES
static HANDLE console = NULL;
static HANDLE keyboard = NULL;
#define stdscr console
#define ERR -1
#define A_BOLD FOREGROUND_INTENSITY
#define KEY_UP VK_UP
#define KEY_DOWN VK_DOWN
#define KEY_LEFT VK_LEFT
#define KEY_RIGHT VK_RIGHT
#define KEY_ENTER 13
#define KEY_TAB 9
#define KEY_DEL 892
static int attr = 0;
inline void initscr(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	DWORD mode;

	console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console,&info);
	info.wAttributes &= ~FOREGROUND_INTENSITY;
	SetConsoleTextAttribute(console,info.wAttributes);

	keyboard = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(keyboard,&mode);
	mode &= ~ENABLE_LINE_INPUT;
	SetConsoleMode(keyboard,mode);
}
inline void cbreak(void)
{
	/* nothing to do - Windows already does this by default */
}
inline void echo(void)
{
	/* doesn't work with ENABLE_LINE_INPUT off so it's done manually in wgetch */
}
inline void refresh(void)
{
	/* currently unbuffered output */
}
inline void clear(void)
{
	COORD home={0,0};
	CONSOLE_SCREEN_BUFFER_INFO cbsi;
	DWORD size;
	DWORD done;
	GetConsoleScreenBufferInfo(console,&cbsi);
	size = cbsi.dwSize.X * cbsi.dwSize.Y;
	FillConsoleOutputCharacter(console,(TCHAR)' ',size,home,&done);
	GetConsoleScreenBufferInfo(console,&cbsi);
	FillConsoleOutputAttribute(console,cbsi.wAttributes,size,home,&done);
	SetConsoleCursorPosition(console,home);
}
inline void intrflush(HANDLE w, BOOL bf)
{
	/* nothing to do - Windows already does this by default */
}
inline void keypad(HANDLE w, BOOL bf)
{
}
#include <sys/timeb.h>
static int delay=0;
inline int halfdelay(int t)
{
	delay = t;
	return 0;
}
inline void mvprintw(int y, int x, const char *fmt, ...)
{
	va_list ptr;
	COORD pos={static_cast<SHORT>(x), static_cast<SHORT>(y)};
	SetConsoleCursorPosition(console,pos);
	va_start(ptr,fmt);
	vfprintf(stdout,fmt,ptr);
	va_end(ptr);
}
inline int wgetch(HANDLE w)
{
	struct timeb t0, t1;
	double dt=0;
	DWORD n=-1;

	ftime(&t0); t1=t0;
Retry:
	while ( GetNumberOfConsoleInputEvents(keyboard,&n) && n==0 )
	{
		Sleep(20);
		ftime(&t1);
		dt = ((double)t1.time+(double)t1.millitm/1e3) - ((double)t0.time+(double)t0.millitm/1e3);
		if ( dt*10>=delay )
			return ERR;
	}
	if ( n>0 )
	{
		INPUT_RECORD input;
		n=-1;
		if ( !ReadConsoleInput(keyboard,&input,(DWORD)1,&n) ) goto Error;
		if ( n>0 && input.EventType==KEY_EVENT && input.Event.KeyEvent.bKeyDown ) 
		{
			KEY_EVENT_RECORD key = input.Event.KeyEvent;
			return key.wVirtualKeyCode;
		}
		else
			goto Retry;
	}
	else if ( n==0 )
		return ERR;
	else
	{
Error:
		fprintf(stderr,"keyboard read error: code=%d", GetLastError());
		exit(1);
	}
}
inline void endwin(void)
{
}
inline void attron(int n)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(console,&info);
	info.wAttributes |= n;
	SetConsoleTextAttribute(console,info.wAttributes);
}
inline void attroff(int n)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(console,&info);
	info.wAttributes &= ~n;
	SetConsoleTextAttribute(console,info.wAttributes);
}
#else
#if defined HAVE_NCURSESW_CURSES_H
#  include <ncursesw/curses.h>
#elif defined HAVE_NCURSESW_H
#  include <ncursesw.h>
#elif defined HAVE_NCURSES_CURSES_H
#  include <ncurses/curses.h>
#elif defined HAVE_NCURSES_H
#  include <ncurses.h>
#elif defined HAVE_CURSES_H
#  include <curses.h>
#endif
#endif

#ifdef _WIN32
inline long getwidth(void)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if ( console )
	{
		CONSOLE_SCREEN_BUFFER_INFO cbsi;
		GetConsoleScreenBufferInfo(console,&cbsi);
		return cbsi.srWindow.Right-cbsi.srWindow.Left+1;
	}
	else
		return -1;
}
inline long getheight(void)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if ( console )
	{
		CONSOLE_SCREEN_BUFFER_INFO cbsi;
		GetConsoleScreenBufferInfo(console,&cbsi);
		return cbsi.srWindow.Bottom - cbsi.srWindow.Top;
	}
	else
		return -1;
}
#else
#ifdef HAVE_CURSES
#include <sys/ioctl.h>
#ifndef ttysize
#define ttysize winsize
#define ts_cols ws_col
#define ts_lines ws_row
#endif
inline long getwidth(void)
{
	struct ttysize ws;
	if ( ioctl(1,TIOCGWINSZ,&ws)!=-1 )
		return ws.ts_cols;
	else
		return -1;
}
inline long getheight(void)
{
	struct ttysize ws;
	if ( ioctl(1,TIOCGWINSZ,&ws)!=-1 )
		return ws.ts_lines;
	else
		return -1;
}
#endif

#endif

#endif
