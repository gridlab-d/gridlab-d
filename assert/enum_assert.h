/** Assert function
 **/

#ifndef _enum_assert_H
#define _enum_assert_H

#include <stdarg.h>

#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class enum_assert : public gld_object
{
public:
    enum
    {
        ASSERT_TRUE = 1,
        ASSERT_FALSE,
        ASSERT_NONE
    };

    // GL_ATOMIC(enumeration, status);
    // GL_STRING(char1024,target);
    // GL_ATOMIC(int32,value);

protected:
    enumeration status; // Member variable of type `enumeration`.
    char1024 target;    // Protected member variable
    int32 value;        // Member variable of type `int32`.

public:
    // Static inline method to get the byte offset of the member `target`
    static inline size_t get_target_offset(void)
    {
        enum_assert *current_defaults = get_defaults();
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
    // Static inline method to get the byte offset of the member `status`.
    static inline size_t get_status_offset(void)
    {
        enum_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->status)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `status`.
    inline enumeration get_status(void)
    {
        return status;
    }

    // Inline method to return a gld_property object for `status`.
    inline gld_property get_status_property(void)
    {
        return gld_property(my(), std::string("status").c_str());
    }

    // Inline method to set the value of `status`.
    inline void set_status(enumeration p)
    {
        status = p;
    }

    // Inline method to get the string representation of the `status` property.
    inline gld_string get_status_string(void)
    {
        return get_status_property().get_string();
    }

    // Inline method to set the `status` property from a provided string.
    inline void set_status(char *str)
    {
        get_status_property().from_string(str);
    }

public:
    enum_assert() {}
    ~enum_assert()
    {
        if (defaults)
            delete defaults;
    }

    static inline enum_assert *get_defaults()
    {
        if (!defaults)
        {
            defaults = new enum_assert(); // Initialize lazily
        }
        return defaults;
    }

public:
    // Static inline method to get the byte offset of the member `value`.
    static inline size_t get_value_offset(void)
    {
        enum_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->value)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `value`.
    inline int32 get_value(void)
    {
        return value;
    }

    // Inline method to return a gld_property object for `value`.
    inline gld_property get_value_property(void)
    {
        return gld_property(my(), std::string("value").c_str());
    }

    // Inline method to set the value of `value`.
    inline void set_value(int32 p)
    {
        value = p;
    }

    // Inline method to get the string representation of the `value` property.
    inline gld_string get_value_string(void)
    {
        return get_value_property().get_string();
    }

    // Inline method to set the `value` property from a provided string.
    inline void set_value(char *str)
    {
        get_value_property().from_string(str);
    }

public:
    /* required implementations */
    enum_assert(MODULE *module);
    int create(void);
    int init(OBJECT *parent);
    TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);

public:
    static CLASS *oclass;
    static enum_assert *defaults;
};

#endif
