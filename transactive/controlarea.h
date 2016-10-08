// $Id: controlarea.h 5185 2015-06-23 00:13:36Z dchassin $

#ifndef _CONTROLAREA_H
#define _CONTROLAREA_H

typedef enum {
	CAS_OK=KC_GREEN,
	CAS_OVERCAPACITY=KC_RED,
	CAS_CONSTRAINED=KC_ORANGE,
	CAS_ISLAND=KC_BLUE,
	CAS_UNSCHEDULED=KC_PURPLE,
	CAS_BLACKOUT=KC_BLACK,
} CONTROLAREASTATUS;

class controlarea : public gld_object {
public:
	GL_ATOMIC(int64,update); ///< pseudo variable for receiving messages from generators, loads, and interties
	GL_ATOMIC(double,inertia); ///< total inertia of generators and loads
	GL_ATOMIC(double,capacity); ///< total capacity of generators
	GL_ATOMIC(double,supply); ///< actual generation supply
	GL_ATOMIC(double,demand); ///< actual load demand
	GL_ATOMIC(double,schedule); ///< scheduled intertie exchange
	GL_ATOMIC(double,actual); ///< actual intertie exchange
	GL_ATOMIC(double,ace); ///< ace value
	GL_STRUCT(double_array,ace_filter); ///< ace filter coefficients (maximum is second order)
	GL_ATOMIC(double,bias); ///< frequency bias
	GL_ATOMIC(double,losses); ///< line losses (internal+export)
	GL_ATOMIC(double,internal_losses); ///< internal area losses
	GL_ATOMIC(double,imbalance); ///< net imbalance of power
	GL_ATOMIC(enumeration,status);
	GL_ATOMIC(size_t,node_id); ///< node id
private:
	double f0; ///< system frequency
	double fr; ///< relative frequency difference
	unsigned int n_intertie;
	gld_object *system;
	gld_property update_system;
	gld_property frequency;
	unsigned int init_count;
	OBJECTLIST *intertie_list;
	TIMESTAMP last_update;
public:
	DECL_IMPLEMENT(controlarea);
	DECL_SYNC;
	DECL_NOTIFY(update);
	int kmldump(int (*stream)(const char*, ...));
public:
	inline double get_frequency(void) { return frequency.get_double(); };
};

#endif // _CONTROLAREA_H