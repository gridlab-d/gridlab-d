/// $Id$
/// @file xml.h
/// @addtogroup connection
/// @{

#ifndef _XML_H
#define _XML_H

#include "gridlabd.h"
#include "native.h"

typedef enum {
	UTF8=0, ///< specifies 8-bit XML encoding
	UTF16=1, ///< specifies 16-bit XML encoding
} XML_UTFENCODING; /// UTF encoding specification

class xml : public native {
public:
	//GL_ATOMIC(enumeration, encoding);
	//GL_STRING(char8,version);
	//GL_STRING(char1024,schema);
	//GL_STRING(char1024,stylesheet); 
	// TODO add published properties here

private:
	// TODO add other properties here

public:
	static inline xml* get_defaults() {
		if (!defaults) {
			defaults = new xml(); // Initialize lazily
		}
		return defaults;
	}

	xml() {}
	~xml () { if (defaults) delete defaults; }

protected:
	enumeration encoding;  // Member variable of type `enumeration`.

public:
	// Static inline method to get the byte offset of the member `encoding`.
	static inline size_t get_encoding_offset(void) {
		xml* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->encoding)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `encoding`.
	inline enumeration get_encoding(void) {
		return encoding;
	}

	// Inline method to return a `gld_property` object for `encoding`.
	inline gld_property get_encoding_property(void) {
		return gld_property(my(), std::string("encoding").c_str());
	}

	// Inline method to set the value of `encoding`.
	inline void set_encoding(enumeration p) {
		encoding = p;
	}

	// Inline method to get the string representation of the `encoding` property.
	inline gld_string get_encoding_string(void) {
		return get_encoding_property().get_string();
	}

	// Inline method to set the `encoding` property from a provided string.
	inline void set_encoding(char* str) {
		get_encoding_property().from_string(str);
	}

protected:
	char8 version;  // Member variable of type `char[8]`.

public:
	// Static inline method to get the byte offset of the member `version`.
	static inline size_t get_version_offset(void) {
		xml* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->version)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to safely retrieve the string value of `version`.
	inline std::string get_version(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(version);
	}

	// Getter method to retrieve `gld_property` for `version`.
	inline gld_property get_version_property(void) {
		return gld_property(my(), std::string("version").c_str());
	}

	// Setter method to update the value for `version` using a string.
	inline void set_version(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(version, str.c_str(), sizeof(version) - 1);
		version[sizeof(version) - 1] = '\0';  // Ensure null termination
	}

protected:
	char1024 schema;  // Member variable of type `char[1024]`.

public:
	// Static inline method to get the byte offset of the member `schema`.
	static inline size_t get_schema_offset(void) {
		xml* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->schema)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to safely retrieve the string value of `schema`.
	inline std::string get_schema(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(schema);
	}

	// Getter method to retrieve `gld_property` for `schema`.
	inline gld_property get_schema_property(void) {
		return gld_property(my(), std::string("schema").c_str());
	}

	// Setter method to update the value for `schema` using a string.
	inline void set_schema(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(schema, str.c_str(), sizeof(schema) - 1);
		schema[sizeof(schema) - 1] = '\0';  // Ensure null termination
	}

protected:
	char1024 stylesheet;  // Member variable of type `char[1024]`.

public:
	// Static inline method to get the byte offset of the member `stylesheet`.
	static inline size_t get_stylesheet_offset(void) {
		xml* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->stylesheet)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Getter method to safely retrieve the string value of `stylesheet`.
	inline std::string get_stylesheet(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(stylesheet);
	}

	// Getter method to retrieve `gld_property` for `stylesheet`.
	inline gld_property get_stylesheet_property(void) {
		return gld_property(my(), std::string("stylesheet").c_str());
	}

	// Setter method to update the value for `stylesheet` using a string.
	inline void set_stylesheet(const std::string& str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(stylesheet, str.c_str(), sizeof(stylesheet) - 1);
		stylesheet[sizeof(stylesheet) - 1] = '\0';  // Ensure null termination
	}

public:
	// required implementations
	xml(MODULE*);
	int create(void);
	int init(OBJECT*);
	int precommit(TIMESTAMP);
	TIMESTAMP presync(TIMESTAMP);
	TIMESTAMP sync(TIMESTAMP);
	TIMESTAMP postsync(TIMESTAMP);
	TIMESTAMP commit(TIMESTAMP,TIMESTAMP);
	int prenotify(PROPERTY*,char*);
	int postnotify(PROPERTY*,char*);
	int finalize();
	TIMESTAMP plc(TIMESTAMP);
	int link(char *value);
	int option(char *value);
	void term(TIMESTAMP);
	// TODO add other event handlers here

public:
	// special variables for GridLAB-D classes
	static CLASS *oclass;
	static xml *defaults;
};

#endif /// @} _XML_H 

