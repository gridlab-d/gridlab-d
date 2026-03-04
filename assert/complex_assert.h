/** Assert function
 **/

#ifndef _complex_assert_H
#define _complex_assert_H

#include <stdarg.h>
#include <mutex>
#include <shared_mutex>

#include "gridlabd.h"
#include "object.h"
using gld::complex;

#ifndef _isnan
#define _isnan isnan
#endif

class complex_assert : public gld_object
{
public:
    enum
    {
        FULL = 0,
        REAL = 1,
        IMAGINARY = 2,
        MAGNITUDE = 3,
        ANGLE = 4
    };
    enum
    {
        ONCE_FALSE = 0,
        ONCE_TRUE = 1,
        ONCE_DONE = 2
    };
    enum
    {
        ASSERT_TRUE = 1,
        ASSERT_FALSE,
        ASSERT_NONE
    };

    // GL_ATOMIC(enumeration, status);
    // GL_STRING(char1024,target);
    // GL_ATOMIC(complex,value);
    // GL_ATOMIC(enumeration, operation);
    // GL_ATOMIC(enumeration, once);
    // GL_STRUCT(complex,once_value);
    // GL_ATOMIC(double,within);

    complex_assert()
    {
        // Initialize default values if needed
        // Initialize default values here
        status = ASSERT_TRUE;
        within = 0.0;
        value = 0.0;
        once = ONCE_FALSE;
        once_value = 0;
        operation = FULL;
        strcpy(target, "");
    }
    ~complex_assert()
    {

        defaults = nullptr;
    }

    static inline complex_assert *get_defaults()
    {
        // if (!defaults)
        // {
        //     defaults = new complex_assert(); // Initialize lazily
        // }
        return defaults;
    }

protected:
    enumeration status;      // Member variable of type `enumeration`.
    enumeration operation;   // Member variable of type `enumeration`.
    enumeration once;        // Member variable of type `enumeration`.
    double within;           // Member variable of type `double`.
    gld::complex value;      // Member variable of type `complex`.
    gld::complex once_value; // Member variable of type `complex`.
    char1024 target;         // Protected member variable

public:
    // Static inline method to get the byte offset of the member `status`.
    static inline size_t get_status_offset(void)
    {
        complex_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->status)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `status`.
    inline enumeration get_status(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return status;
    }

    // Inline method to return a gld_property object for `status`.
    inline gld_property get_status_property(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return gld_property(my(), std::string("status").c_str());
    }

    // Inline method to set the value of `status`.
    inline void set_status(enumeration p)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        status = p;
    }

    // Inline method to get the string representation of the `status` property.
    inline gld_string get_status_string(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return get_status_property().get_string();
    }

    // Inline method to set the `status` property from a provided string.
    inline void set_status(char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        get_status_property().from_string(str);
    }

public:
    // Static inline method to get the byte offset of the member `operation`.
    static inline size_t get_operation_offset(void)
    {
        complex_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->operation)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `operation`.
    inline enumeration get_operation(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return operation;
    }

    // Inline method to return a gld_property object for `operation`.
    inline gld_property get_operation_property(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return gld_property(my(), std::string("operation").c_str());
    }

    // Inline method to set the value of `operation`.
    inline void set_operation(enumeration p)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        operation = p;
    }

    // Inline method to get the string representation of the `operation` property.
    inline gld_string get_operation_string(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return get_operation_property().get_string();
    }

    // Inline method to set the `operation` property from a provided string.
    inline void set_operation(char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        get_operation_property().from_string(str);
    }

public:
    // Static inline method to get the byte offset of the member `once`.
    static inline size_t get_once_offset(void)
    {
        complex_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->once)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `once`.
    inline enumeration get_once(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return once;
    }

    // Inline method to return a gld_property object for `once`.
    inline gld_property get_once_property(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return gld_property(my(), std::string("once").c_str());
    }

    // Inline method to set the value of `once`.
    inline void set_once(enumeration p)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        once = p;
    }

    // Inline method to get the string representation of the `once` property.
    inline gld_string get_once_string(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return get_once_property().get_string();
    }

    // Inline method to set the `once` property from a provided string.
    inline void set_once(char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        get_once_property().from_string(str);
    }

public:
    // Static inline method to get the byte offset of the member `within`.
    static inline size_t get_within_offset(void)
    {
        complex_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->within)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `within`.
    inline double get_within(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return within;
    }

    // Inline method to return a gld_property object for `within`.
    inline gld_property get_within_property(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return gld_property(my(), std::string("within").c_str());
    }

    // Inline method to set the value of `within`.
    inline void set_within(double p)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        within = p;
    }

    // Inline method to get the string representation of the `within` property.
    inline gld_string get_within_string(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return get_within_property().get_string();
    }

    // Inline method to set the `within` property from a provided string.
    inline void set_within(char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        get_within_property().from_string(str);
    }

public:
    // Static inline method to get the byte offset of the member ``
    static inline size_t get_value_offset(void)
    {
        complex_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->value)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `value`.
    inline gld::complex get_value(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return value;
    }

    // Inline method to return a gld_property object for `value`.
    inline gld_property get_value_property(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return gld_property(my(), std::string("value").c_str());
    }

    // Inline method to set the value of `value`.
    inline void set_value(gld::complex p)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        value = p;
    }

    // Inline method to get the string representation of the `value` property.
    inline gld_string get_value_string(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return get_value_property().get_string();
    }

    // Inline method to set the `value` property from a provided string.
    inline void set_value(char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        get_value_property().from_string(str);
    }

public:
    // Static inline method to get the byte offset of the member
    static inline size_t get_once_value_offset(void)
    {
        complex_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->once_value)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `once_value`.
    inline gld::complex get_once_value(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::shared_lock<std::shared_mutex> lock(mtx);
        return once_value;
    }

    // Inline method to return a gld_property object for `once_value`.
    inline gld_property get_once_value_property(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return gld_property(my(), std::string("once_value").c_str());
    }

    // Inline method to get the string representation of the `once_value` property.
    inline gld_string get_once_value_string(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        return get_once_value_property().get_string();
    }

    // Inline method to set the `once_value` property from a provided string.
    inline void set_once_value(char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        get_once_value_property().from_string(str);
    }

    inline void set_once_value(const std::string &str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        get_once_value_property().from_string(const_cast<char *>(str.c_str()));
    }

    inline void set_once_value(gld::complex p)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        once_value = p;
    }

public:
    // Static inline method to get the byte offset of the member `target`
    static inline size_t get_target_offset(void)
    {
        complex_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->target)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Getter method to safely retrieve the string value of `target` as std::string
    inline std::string get_target(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::shared_lock<std::shared_mutex> lock(mtx);
        return std::string(target);
    }

    inline void set_target(const char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        strncpy(target, str, sizeof(target) - 1);
        target[sizeof(target) - 1] = '\0'; // Ensure null-termination
    }

    // Getter method to retrieve gld_property for `target`
    inline gld_property get_target_property(void)
    {
        if (!my())
        { // Check if `my()` returns a valid object
            throw std::runtime_error("Invalid object context for retrieving gld_property.");
        }
        return gld_property(my(), std::string("target").c_str()); // Duplicate string literal `target`
    }

public:
    /* required implementations */
    complex_assert(MODULE *module);
    int create(void);
    int init(OBJECT *parent);
    TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
    int postnotify(PROPERTY *prop, char *value);
    inline int prenotify(PROPERTY *, char *) { return 1; };

public:
    static CLASS *oclass;
    // static std::shared_ptr<complex_assert> defaults;
    static complex_assert *defaults;
};

#endif
