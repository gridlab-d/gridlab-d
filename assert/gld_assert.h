/** $Id: assert.h 4738 2014-07-03 00:55:39Z dchassin $

 General purpose assert objects

 **/

#ifndef _GLD_ASSERT_H
#define _GLD_ASSERT_H

#include "gridlabd.h"

#undef g_assert

class g_assert : public gld_object
{
public:
    typedef enum
    {
        AS_INIT = 0,
        AS_TRUE = 1,
        AS_FALSE = 2,
        AS_NONE = 3
    } ASSERTSTATUS;

    /*GL_ATOMIC(enumeration, status);
    GL_STRING(char1024,target);
    GL_STRING(char32,part);
    GL_ATOMIC(enumeration, relation);
    GL_STRING(char1024,value);
    GL_STRING(char1024,value2);*/
    g_assert() {}

    static inline g_assert *get_defaults()
    {
        if (!defaults)
        {
            defaults = new g_assert(); // Initialize lazily
        }
        return defaults;
    }

protected:
    enumeration status;   // Member variable of type `enumeration`.
    char1024 target;      // Protected member variable
    char32 part;          // Protected member variable
    enumeration relation; // Member variable of type `enumeration`.
    char1024 value;       // Member variable of type `char24`.
    char1024 value2;      // Member variable of type `char24`.

public:
    // Static inline method to get the byte offset of the member `status`.
    static inline size_t get_status_offset(void)
    {
        g_assert *current_defaults = get_defaults();
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
    // Static inline method to get the byte offset of the member `relation`.
    static inline size_t get_relation_offset(void)
    {
        g_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->relation)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `relation`.
    inline enumeration get_relation(void)
    {
        return relation;
    }

    // Inline method to return a gld_property object for `relation`.
    inline gld_property get_relation_property(void)
    {
        return gld_property(my(), std::string("relation").c_str());
    }

    // Inline method to set the value of `relation`.
    inline void set_relation(enumeration p)
    {
        relation = p;
    }

    // Inline method to get the string representation of the `relation` property.
    inline gld_string get_relation_string(void)
    {
        return get_relation_property().get_string();
    }

    // Inline method to set the `relation` property from a provided string.
    inline void set_relation(char *str)
    {
        get_relation_property().from_string(str);
    }

public:
    // Static inline method to get the byte offset of the member ``
    static inline size_t get_value_offset(void)
    {
        g_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->value)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Inline function to get the value of `value`.
    inline std::string get_value(void)
    {
        return std::string(value);
    }

    // Inline method to return a gld_property object for `value`.
    inline gld_property get_value_property(void)
    {
        if (!my())
        { // Check if `my()` returns a valid object
            throw std::runtime_error("Invalid object context for retrieving gld_property.");
        }
        return gld_property(my(), std::string("value").c_str()); // Duplicate string literal `target`
    }

    // Inline method to set the value of `value`.
    inline void set_value(const char *p)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        strncpy(target, p, sizeof(target) - 1);
        target[sizeof(target) - 1] = '\0'; // Ensure null-termination
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
    // Static inline method to get the byte offset of the member `part`
    static inline size_t get_part_offset(void)
    {
        g_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->part)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Getter method to safely retrieve the string value of `part` as std::string
    inline std::string get_part(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::shared_lock<std::shared_mutex> lock(mtx);
        return std::string(part);
    }

    inline void set_part(const char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        strncpy(part, str, sizeof(part) - 1);
        part[sizeof(part) - 1] = '\0'; // Ensure null-termination
    }

    // Getter method to retrieve gld_property for `part`
    inline gld_property get_part_property(void)
    {
        if (!my())
        { // Check if `my()` returns a valid object
            throw std::runtime_error("Invalid object context for retrieving gld_property.");
        }
        return gld_property(my(), std::string("part").c_str()); // Duplicate string literal `part`
    }

public:
    // Static inline method to get the byte offset of the member `target`
    static inline size_t get_target_offset(void)
    {
        g_assert *current_defaults = get_defaults();
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
    // Static inline method to get the byte offset of the member `value2`
    static inline size_t get_value2_offset(void)
    {
        g_assert *current_defaults = get_defaults();
        return reinterpret_cast<const char *>(&(current_defaults->value2)) - reinterpret_cast<const char *>(current_defaults);
    }

    // Getter method to safely retrieve the string value of `value2` as std::string
    inline std::string get_value2(void)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::shared_lock<std::shared_mutex> lock(mtx);
        return std::string(value2);
    }

    inline void set_value2(const char *str)
    {
        auto &mtx = SharedMutexManager::get_mutex(my());
        std::unique_lock<std::shared_mutex> lock(mtx);
        strncpy(value2, str, sizeof(value2) - 1);
        value2[sizeof(value2) - 1] = '\0'; // Ensure null-termination
    }

    // Getter method to retrieve gld_property for `value2`
    inline gld_property get_value2_property(void)
    {
        if (!my())
        { // Check if `my()` returns a valid object
            throw std::runtime_error("Invalid object context for retrieving gld_property.");
        }
        return gld_property(my(), std::string("value2").c_str()); // Duplicate string literal `value2`
    }

private:
    ASSERTSTATUS evaluate_status(void);

public:
    /* required implementations */
    g_assert(MODULE *module);
    int create(void);
    int init(OBJECT *parent);
    TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
    int postnotify(PROPERTY *prop, char *value);
    inline int prenotify(PROPERTY *prop, char *value) { return 1; };

public:
    static CLASS *oclass;
    static g_assert *defaults;
};

#endif // _GLD_ASSERT_H
