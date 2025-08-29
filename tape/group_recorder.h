// header stuff here

#ifndef _GROUP_RECORDER_H_
#define _GROUP_RECORDER_H_

#include "tape.h"

EXPORT void new_group_recorder(MODULE *);
EXPORT int group_recorder_postroutine(OBJECT *obj, double timedbl);

#ifdef __cplusplus
//FIXME take a look at quickobjlist. May be incorrect.
class quickobjlist{
public:
	quickobjlist(){
		obj = 0;
		next = 0;
		memset(&prop, 0, sizeof(PROPERTY));
	}
	quickobjlist(OBJECT *o, PROPERTY *p){
		obj = o;
		next = 0;
		memcpy(&prop, p, sizeof(PROPERTY));
	}
	~quickobjlist(){
		if(next != 0)
			delete next;
	}
	void tack(OBJECT *o, PROPERTY *p){if(next){next->tack(o, p);} else {next = new quickobjlist(o, p);}}
	OBJECT *obj;
	PROPERTY prop;
	quickobjlist *next;
};

class group_recorder : public gld_object {
public:
	static group_recorder *defaults;
	static CLASS *oclass, *pclass;

	explicit group_recorder(MODULE *);
	int create();
	int init(OBJECT *);
	int isa(char *);
	TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

	int commit(TIMESTAMP t1, double t1dbl, bool deltacall);

    //GL_STRING(char256, filename)
    //GL_STRING(char1024, group_def)
    //GL_ATOMIC(bool, strict)
    //GL_ATOMIC(bool, print_units)
    //GL_STRING(char256, property_name)
    //GL_ATOMIC(int32, limit)
    //GL_ATOMIC(bool, format)
    /*GL_STRING(char32,mode)
    GL_STRING(char256, complex_part)
    GL_ATOMIC(double, dInterval)
    GL_ATOMIC(double, dFlush_interval)*/

	//Public this property, so it can access itself in scope
	TIMESTAMP write_interval;
private:
	int write_header();
	int read_line();
	int write_line(TIMESTAMP t1, double t1dbl, bool deltacall);
	int flush_line();
	int write_footer();
private:
	FILE *rec_file;
	FINDLIST *items;
	quickobjlist *obj_list;
	PROPERTY *prop_ptr;
	int obj_count;
	int write_count;
	TIMESTAMP next_write;
	TIMESTAMP last_write;
	TIMESTAMP last_flush;
	TIMESTAMP flush_interval;
	int32 write_ct;
	TAPESTATUS tape_status; // TS_INIT/OPEN/DONE/ERROR
	char *prev_line_buffer;
	char *line_buffer;
	size_t line_size;
	bool interval_write;
	bool offnominal_time;

public:
	static inline group_recorder* get_defaults() {
		if (!defaults) {
			defaults = new group_recorder(); // Initialize lazily
		}
		return defaults;
	}

	group_recorder() {}
	~group_recorder() { if (defaults) delete defaults; }

protected:
	char256 filename;  // Member variable of type `char[256]`.

public:
	// Static inline method to get the byte offset of the member `filename`.
	static inline size_t get_filename_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->filename)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to safely retrieve the string value of `filename`.
	inline std::string get_filename(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(filename);
	}

	// Getter method to retrieve `gld_property` for `filename`.
	inline gld_property get_filename_property(void) {
		return gld_property(my(), std::string("filename").c_str());
	}

	// Setter method to update the value for `filename` using a string.
	inline void set_filename(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(filename, str.c_str(), sizeof(filename) - 1);
		filename[sizeof(filename) - 1] = '\0';  // Ensure null termination
	}
protected:
	char1024 group_def;  // Member variable of type `char[1024]`.

public:
	// Static inline method to get the byte offset of the member `group_def`.
	static inline size_t get_group_def_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->group_def)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to safely retrieve the string value of `group_def`.
	inline std::string get_group_def(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(group_def);
	}

	// Getter method to retrieve `gld_property` for `group_def`.
	inline gld_property get_group_def_property(void) {
		return gld_property(my(), std::string("group_def").c_str());
	}

	// Setter method to update the value for `group_def` using a string.
	inline void set_group_def(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(group_def, str.c_str(), sizeof(group_def) - 1);
		group_def[sizeof(group_def) - 1] = '\0';  // Ensure null termination
	}

protected:
	bool strict;  // Member variable of type `bool`.

public:
	// Static inline method to get the byte offset of the member `strict`.
	static inline size_t get_strict_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->strict)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `strict`.
	inline bool get_strict(void) {
		return strict;
	}

	// Inline method to return a `gld_property` object for `strict`.
	inline gld_property get_strict_property(void) {
		return gld_property(my(), std::string("strict").c_str());
	}

	// Inline method to set the value of `strict`.
	inline void set_strict(bool p) {
		strict = p;
	}

protected:
	bool print_units;  // Member variable of type `bool`.

public:
	// Static inline method to get the byte offset of the member `print_units`.
	static inline size_t get_print_units_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->print_units)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `print_units`.
	inline bool get_print_units(void) {
		return print_units;
	}

	// Inline method to return a `gld_property` object for `print_units`.
	inline gld_property get_print_units_property(void) {
		return gld_property(my(), std::string("print_units").c_str());
	}

	// Inline method to set the value of `print_units`.
	inline void set_print_units(bool p) {
		print_units = p;
	}
protected:
	char256 property_name;  // Member variable of type `char[256]`.

public:
	// Static inline method to get the byte offset of the member `property_name`.
	static inline size_t get_property_name_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->property_name)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to retrieve the string value of `property_name`.
	inline std::string get_property_name(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(property_name);
	}

	// Getter method to retrieve a `gld_property` for `property_name`.
	inline gld_property get_property_name_property(void) {
		return gld_property(my(), std::string("property_name").c_str());
	}

	// Setter method to set the value of `property_name` using a string.
	inline void set_property_name(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(property_name, str.c_str(), sizeof(property_name) - 1);
		property_name[sizeof(property_name) - 1] = '\0';  // Ensure null termination
	}
protected:
	int32 limit;  // Member variable of type `int32`.

public:
	// Static inline method to get the byte offset of the member `limit`.
	static inline size_t get_limit_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->limit)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `limit`.
	inline int32 get_limit(void) {
		return limit;
	}

	// Inline method to return a `gld_property` object for `limit`.
	inline gld_property get_limit_property(void) {
		return gld_property(my(), std::string("limit").c_str());
	}

	// Inline method to set the value of `limit`.
	inline void set_limit(int32 p) {
		limit = p;
	}
protected:
	bool format;  // Member variable of type `bool`.

public:
	// Static inline method to get the byte offset of the member `format`.
	static inline size_t get_format_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->format)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `format`.
	inline bool get_format(void) {
		return format;
	}

	// Inline method to return a `gld_property` object for `format`.
	inline gld_property get_format_property(void) {
		return gld_property(my(), std::string("format").c_str());
	}

	// Inline method to set the value of `format`.
	inline void set_format(bool p) {
		format = p;
	}
protected:
	char mode[32] = "";  // Member variable of type `char[32]`.

public:
	// Static inline method to calculate the byte offset of the member `mode`.
	static inline size_t get_mode_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->mode)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to retrieve the string value of `mode`.
	inline std::string get_mode(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(mode);
	}

	// Getter method to retrieve the `gld_property` for `mode`.
	inline gld_property get_mode_property(void) {
		return gld_property(my(), std::string("mode").c_str());
	}

	// Setter method to set the value of `mode` using a string.
	inline void set_mode(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(mode, str.c_str(), sizeof(mode) - 1);
		mode[sizeof(mode) - 1] = '\0';  // Ensure null termination
	}
protected:
	char256 complex_part;  // Member variable of type `char[256]`.

public:
	// Static inline method to calculate the byte offset of the member `complex_part`.
	static inline size_t get_complex_part_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->complex_part)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to retrieve the string value of `complex_part`.
	inline std::string get_complex_part(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(complex_part);
	}

	// Getter method to retrieve the `gld_property` for `complex_part`.
	inline gld_property get_complex_part_property(void) {
		return gld_property(my(), std::string("complex_part").c_str());
	}

	// Setter method to set the value of `complex_part` using a string.
	inline void set_complex_part(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(complex_part, str.c_str(), sizeof(complex_part) - 1);
		complex_part[sizeof(complex_part) - 1] = '\0';  // Ensure null termination
	}
protected:
	double dInterval;  // Member variable of type `double`.

public:
	// Static inline method to calculate the byte offset of the member `dInterval`.
	static inline size_t get_dInterval_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->dInterval)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `dInterval`.
	inline double get_dInterval(void) {
		return dInterval;
	}

	// Inline method to return the `gld_property` object for `dInterval`.
	inline gld_property get_dInterval_property(void) {
		return gld_property(my(), std::string("dInterval").c_str());
	}

	// Inline method to set the value of `dInterval`.
	inline void set_dInterval(double p) {
		dInterval = p;
	}


protected:
	double dFlush_interval;  // Member variable of type `double`.

public:
	// Static inline method to calculate the byte offset of the member `dFlush_interval`.
	static inline size_t get_dFlush_interval_offset(void) {
		group_recorder* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->dFlush_interval)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `dFlush_interval`.
	inline double get_dFlush_interval(void) {
		return dFlush_interval;
	}

	// Inline method to return the `gld_property` object for `dFlush_interval`.
	inline gld_property get_dFlush_interval_property(void) {
		return gld_property(my(), std::string("dFlush_interval").c_str());
	}

	// Inline method to set the value of `dFlush_interval`.
	inline void set_dFlush_interval(double p) {
		dFlush_interval = p;
	}

};

#endif // C++

#endif // _GROUP_RECORDER_H_ 

// EOF
