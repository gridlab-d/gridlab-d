// main.h
// Copyright (C) 2018 Regents of the Leland Stanford Junior University

#ifndef _MAIN_H
#define _MAIN_H

class GldMain {
public:		// public variables

private:	// private variables

public:		// constructor/destructor
	GldMain(int argc = 0, char *argv[] = NULL);
	~GldMain(void);
#if defined WIN32 && _DEBUG 
	static void pause_at_exit(void);
#endif
public:		// public methods
	void mainloop(int argc = 0, char *argv[] = NULL);

private:	// private methods
	static void delete_pidfile(void);
	void set_global_browser(const char *path = NULL);
	void set_global_execname(const char *path);
	void set_global_execdir(const char *path);
	void set_global_command_line(int argc = 0,char *argv[] = NULL);
	void set_global_workdir(const char *path = NULL);
	void create_pidfile(void);
};

#endif
