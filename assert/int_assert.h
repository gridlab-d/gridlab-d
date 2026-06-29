/** Assert function
 **/

#ifndef _int_assert_H
#define _int_assert_H

#include <stdarg.h>

#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class int_assert : public gld_object {
public:
  enum { ONCE_FALSE = 0, ONCE_TRUE = 1, ONCE_DONE = 2 };
  enum { IN_ABS = 0, IN_RATIO = 1 };
  enum { ASSERT_TRUE = 1, ASSERT_FALSE, ASSERT_NONE };

  /*GL_ATOMIC(enumeration, status);
  GL_STRING(char1024,target);
  GL_ATOMIC(int64,value);
  GL_ATOMIC(enumeration, once);
  GL_ATOMIC(int64,once_value);
  GL_ATOMIC(enumeration, within_mode);
  GL_ATOMIC(int64,within);*/

  static inline int_assert *get_defaults() {
    if (!defaults) {
      defaults = new int_assert(); // Initialize lazily
    }
    return defaults;
  }

  int_assert() {}
  ~int_assert() {
    if (defaults)
      delete defaults;
  }

protected:
  enumeration status;      // Member variable of type `enumeration`.
  char1024 target;         // Protected member variable
  int32 value;             // Member variable of type `int32`.
  enumeration once;        // Member variable of type `enumeration`.
  int64 once_value;        // Member variable of type `int64`.
  enumeration within_mode; // Member variable of type `enumeration`.
  int64 within;            // Member variable of type `int64`.

public:
  // Static inline method to get the byte offset of the member `status`.
  static inline size_t get_status_offset(void) {
    int_assert *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->status)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Inline function to get the value of `status`.
  inline enumeration get_status(void) { return status; }

  // Inline method to return a gld_property object for `status`.
  inline gld_property get_status_property(void) {
    return gld_property(my(), std::string("status").c_str());
  }

  // Inline method to set the value of `status`.
  inline void set_status(enumeration p) { status = p; }

  // Inline method to get the string representation of the `status` property.
  inline gld_string get_status_string(void) {
    return get_status_property().get_string();
  }

  // Inline method to set the `status` property from a provided string.
  inline void set_status(char *str) { get_status_property().from_string(str); }

public:
  // Static inline method to get the byte offset of the member `operation`.
  static inline size_t get_within_mode_offset(void) {
    int_assert *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->within_mode)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Inline function to get the value of `operation`.
  inline enumeration get_within_mode(void) { return within_mode; }

  // Inline method to return a gld_property object for `operation`.
  inline gld_property get_within_mode_property(void) {
    return gld_property(my(), std::string("operation").c_str());
  }

  // Inline method to set the value of `operation`.
  inline void set_within_mode(enumeration p) { within_mode = p; }

  // Inline method to get the string representation of the `within_mode`
  // property.
  inline gld_string get_within_mode_string(void) {
    return get_within_mode_property().get_string();
  }

  // Inline method to set the `within_mode` property from a provided string.
  inline void set_within_mode(char *str) {
    get_within_mode_property().from_string(str);
  }

public:
  // Static inline method to get the byte offset of the member `once`.
  static inline size_t get_once_offset(void) {
    int_assert *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->once)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Inline function to get the value of `once`.
  inline enumeration get_once(void) { return once; }

  // Inline method to return a gld_property object for `once`.
  inline gld_property get_once_property(void) {
    return gld_property(my(), std::string("once").c_str());
  }

  // Inline method to set the value of `once`.
  inline void set_once(enumeration p) { once = p; }

  // Inline method to get the string representation of the `once` property.
  inline gld_string get_once_string(void) {
    return get_once_property().get_string();
  }

  // Inline method to set the `once` property from a provided string.
  inline void set_once(char *str) { get_once_property().from_string(str); }

public:
  // Static inline method to get the byte offset of the member `within`.
  static inline size_t get_within_offset(void) {
    int_assert *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->within)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Inline function to get the value of `within`.
  inline double get_within(void) { return within; }

  // Inline method to return a gld_property object for `within`.
  inline gld_property get_within_property(void) {
    return gld_property(my(), std::string("within").c_str());
  }

  // Inline method to set the value of `within`.
  inline void set_within(double p) { within = p; }

  // Inline method to get the string representation of the `within` property.
  inline gld_string get_within_string(void) {
    return get_within_property().get_string();
  }

  // Inline method to set the `within` property from a provided string.
  inline void set_within(char *str) { get_within_property().from_string(str); }

public:
  // Static inline method to get the byte offset of the member ``
  static inline size_t get_value_offset(void) {
    int_assert *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->value)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Inline function to get the value of `value`.
  inline int64 get_value(void) { return value; }

  // Inline method to return a gld_property object for `value`.
  inline gld_property get_value_property(void) {
    return gld_property(my(), std::string("value").c_str());
  }

  // Inline method to set the value of `value`.
  inline void set_value(int64 p) { value = p; }

  // Inline method to get the string representation of the `value` property.
  inline gld_string get_value_string(void) {
    return get_value_property().get_string();
  }

  // Inline method to set the `value` property from a provided string.
  inline void set_value(char *str) { get_value_property().from_string(str); }

public:
  // Static inline method to get the byte offset of the member
  static inline size_t get_once_value_offset(void) {
    int_assert *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->once_value)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Inline function to get the value of `once_value`.
  inline int64 get_once_value(void) {
    auto &mtx = SharedMutexManager::get_mutex(my());
    std::shared_lock<std::shared_mutex> lock(mtx);
    return once_value;
  }

  // Inline method to return a gld_property object for `once_value`.
  inline gld_property get_once_value_property(void) {
    return gld_property(my(), std::string("once_value").c_str());
  }

  // Inline method to get the string representation of the `once_value`
  // property.
  inline gld_string get_once_value_string(void) {
    return get_once_value_property().get_string();
  }

  // Inline method to set the `once_value` property from a provided string.
  inline void set_once_value(char *str) {
    get_once_value_property().from_string(str);
  }

  inline void set_once_value(const std::string &str) {
    get_once_value_property().from_string(const_cast<char *>(str.c_str()));
  }

  inline void set_once_value(int64 p) {
    auto &mtx = SharedMutexManager::get_mutex(my());
    std::unique_lock<std::shared_mutex> lock(mtx);
    once_value = p;
  }

public:
  // Static inline method to get the byte offset of the member `target`
  static inline size_t get_target_offset(void) {
    int_assert *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->target)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Getter method to safely retrieve the string value of `target` as
  // std::string
  inline std::string get_target(void) {
    auto &mtx = SharedMutexManager::get_mutex(my());
    std::shared_lock<std::shared_mutex> lock(mtx);
    return std::string(target);
  }

  inline void set_target(const char *str) {
    auto &mtx = SharedMutexManager::get_mutex(my());
    std::unique_lock<std::shared_mutex> lock(mtx);
    strncpy(target, str, sizeof(target) - 1);
    target[sizeof(target) - 1] = '\0'; // Ensure null-termination
  }

  // Getter method to retrieve gld_property for `target`
  inline gld_property get_target_property(void) {
    if (!my()) { // Check if `my()` returns a valid object
      throw std::runtime_error(
          "Invalid object context for retrieving gld_property.");
    }
    return gld_property(
        my(),
        std::string("target").c_str()); // Duplicate string literal `target`
  }

public:
  /* required implementations */
  int_assert(MODULE *module);
  int create(void);
  int init(OBJECT *parent);
  TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);

public:
  static CLASS *oclass;
  static int_assert *defaults;
};
#endif
