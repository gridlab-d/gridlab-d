// $Id: load.h 5188 2015-06-23 22:57:40Z dchassin $

#ifndef _LOAD_H
#define _LOAD_H

#define LCF_NONE 0x00 ///< no control
#define LCF_UFLS 0x01 ///< underfrequency load shedding
#define LCF_LIFT 0x02 ///< enable lift control
#define LCF_ALC 0x04 ///< enable automatic load control

class load : public gld_object {
public:
	GL_ATOMIC(int64,update); ///< pseudo variable for receiving messages from controlarea
	GL_ATOMIC(double,inertia); ///< total inertia of load
	GL_ATOMIC(double,capacity); ///< rated capacity of load
	GL_ATOMIC(double,schedule); ///< scheduled load power
	GL_ATOMIC(double,actual); ///< actual load power
	GL_ATOMIC(set,control_flags); ///< load control flags
	GL_ATOMIC(double,dr_fraction); ///< load fraction under dr control
	GL_ATOMIC(double_array,ufls); ///< underfrequency load shedding matrix (c1: freq; c2: trip fraction)
	GL_STRUCT(double_array,A); ///< DR model A matrix (state transition)
	GL_STRUCT(double_array,B); ///< DR model B matrix (control)
	GL_STRUCT(double_array,C); ///< DR model C matrix (output)
	GL_STRUCT(double_array,D); ///< DR model D matrix (feedfwd)
	GL_STRUCT(double_array,X); ///< DR state vector X (state)
	GL_STRUCT(double_array,U); ///< DR control vector U (control)
	GL_STRUCT(double_array,Y); /// DR output vector Y (output)
private:
	gld_object *area;
	gld_property update_area;
	controlarea *control;
public:
	DECL_IMPLEMENT(load);
	DECL_SYNC;
	DECL_NOTIFY(update);
	int kmldump(int (*stream)(const char*, ...));
};

#endif // _LOAD_H
