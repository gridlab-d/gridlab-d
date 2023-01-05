/// $Id$
/// @file varmap.h
/// @addtogroup connection
/// @{

#ifndef _VARMAP_H
#define _VARMAP_H

#include "gridlabd.h"

#include <string>
#include <memory>
#include <variant>

using std::string;

class last_value_buffer {
#if 0 // C++11 and earlier features.
private:
	enum ValueType {STRING, BOOL, COMPLEX, INT, DOUBLE};
	union DataType {
		std::string last_value_s;
		bool last_value_b;
		gld::complex last_value_c;
		int64_t last_value_i;
		double last_value_d;
		~DataType() { }
	};

	ValueType type = { BOOL };
	DataType data = { .last_value_b = false };

public:
	~last_value_buffer() { // This nonsense destructor is the price we pay for a bare union.
		switch (type) {
		case COMPLEX:
			data.last_value_c.~complex();
			break;
		case STRING:
			data.last_value_s.~string();
			break;
		}
	}

	gld::complex getComplex() { return data.last_value_c; }
	void set(gld::complex &in) { data.last_value_c = in; type = COMPLEX; }

	std::string getString() { return data.last_value_s; }
	void set(std::string &in) { data.last_value_s = in; type = STRING; }

	bool getBool() { return data.last_value_b; }
	void set(bool in) { data.last_value_b = in; type = BOOL; }

	double getDouble() { return data.last_value_d; }
	void set(double in) { data.last_value_d = in; type = DOUBLE; }

	int64 getInt() { return data.last_value_i; }
	void set(int64 in) { data.last_value_i = in; type = INT; }

#else // C++17 or later

private:
	std::variant<
	bool,
	double,
	int64,
	std::string,
	gld::complex
	> last_value;

public:
	last_value_buffer() {
		last_value = "";
	}
    ~last_value_buffer() = default;
    last_value_buffer(const last_value_buffer&) = default;
    last_value_buffer(last_value_buffer&&) = default;
    last_value_buffer& operator=(const last_value_buffer&) = default;
    last_value_buffer& operator=(last_value_buffer&&) = default;

	template<class V>
	V get() {return std::get_if<V>(&last_value); }

	template<class Ti>
	void set(Ti in) { last_value = in; }

	// These can be removed and refactored to use the templated getter above once C++17 is adopted.
	std::string getString() {return std::get<std::string>(last_value); }
	gld::complex getComplex() {return std::get<gld::complex>(last_value); }
	int64 getInt() {return std::get<int64>(last_value); }
	bool getBool() {return std::get<bool>(last_value); }
	double getDouble() {return std::get<double>(last_value); }

#endif
};

typedef enum {
	DXD_READ=0x01,  ///< data is incoming
	DXD_WRITE=0x02, ///< data is outgoing
	DXD_ALLOW=0x04, ///< data exchange allowed for protected properties
	DXD_FORBID=0x08,///< data exchange forbidden for unprotected properties
} DATAEXCHANGEDIRECTION;
typedef enum {
	CT_UNKNOWN=1,
	CT_PUBSUB=2, ///< This is a publish subscribe type of communication
	CT_ROUTE=3, ///< This is a point to point type of communication
}COMMUNICATIONTYPE;
typedef struct s_varmap {
	char *local_name; ///< local name
	char *remote_name; ///< remote name
	DATAEXCHANGEDIRECTION dir; ///< direction of exchange
	gld_property *obj; ///< local object and property (obj==NULL for globals)
	struct s_varmap *next; ///< next variable in map
	COMMUNICATIONTYPE ctype; ///< The actual communication type. Used only for communication with FNCS.
	char threshold[1024]; ///< The threshold to exceed to actually trigger sending a message. Used only for communication with FNCS.
	std::unique_ptr<last_value_buffer> last_value { nullptr };


} VARMAP; ///< variable map structure

class varmap {
public:
	VARMAP *map;
public:
	varmap();
	int add(char *spec, COMMUNICATIONTYPE comtype); ///< add a variable mapping using full spec, e.g. "<event>:<local><dir><remote>"
	void resolve(void); ///< process unresolved local names
	void linkcache(class connection_mode*, void *xlate); ///< link cache to variables
	inline VARMAP *getfirst(void) ///< get first variable in map
		{ return map; }; 
	inline VARMAP *getnext(VARMAP*var) /// get next variable in map
		{ return var->next; };
};

#endif /// @} _VARMAP_H
