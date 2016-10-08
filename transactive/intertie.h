// $Id: intertie.h 5077 2015-02-01 17:03:40Z dchassin $

#ifndef _INTERTIE_H
#define _INTERTIE_H

typedef enum {
	TLS_OFFLINE=KC_BLACK,
	TLS_CONSTRAINED=KC_ORANGE,
	TLS_UNCONSTRAINED=KC_GREEN,
	TLS_OVERLIMIT=KC_RED,
	TLS_NOFLOW=KC_CYAN,
} TIELINESTATUS;

class intertie : public gld_object {
public:
	GL_ATOMIC(object,from); ///< from area
	GL_ATOMIC(object,to); ///< to area
	GL_ATOMIC(double,capacity); ///< line capacity [MW]
	GL_ATOMIC(enumeration,status); ///< line status {OFFLINE, CONSTRAINED, UNCONSTRAINED}
	GL_ATOMIC(double,flow); ///< line flow [MW]
	GL_ATOMIC(double,loss); ///< line loss [pu]
	GL_ATOMIC(double,losses); ///< line losses [MW]
	GL_ATOMIC(size_t,line_id);
private:
	gld_object *from_area;
	gld_object *to_area;
	gld_object *system;
public:
	inline controlarea *get_from_area() const { return OBJECTDATA(from,controlarea); };
	inline controlarea *get_to_area() const { return OBJECTDATA(to,controlarea); };
public:
	DECL_IMPLEMENT(intertie);
	DECL_SYNC;
	int kmldump(int (*stream)(const char*, ...));
};

#endif // _INTERTIE_H
