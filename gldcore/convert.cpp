/** $Id: convert.cpp 4738 2014-07-03 00:55:39Z dchassin $
        Copyright (C) 2008 Battelle Memorial Institute
        @file convert.c
        @author David P. Chassin
        @date 2007
        @addtogroup convert Conversion of properties
        @ingroup core

        The convert module handles conversion object properties and strings

@{
 **/

#include <cctype>
#include <cmath>
#include <cstdio>

#include <algorithm>
#include <cstring>
#include <string_view>

#include <Eigen/Dense>
#include <cstdlib> // For atof
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "convert.h"
#include "globals.h"
#include "load.h"
#include "object.h"
#include "output.h"
#include "property.h"

#ifdef HAVE_STDINT_H
#include <cstdint>
typedef uint32_t uint32; /* unsigned 32-bit integers */
#else
typedef unsigned int uint32;
#endif

#undef min
#undef max

#if defined(_WIN32) || defined(_MSC_VER)
// Windows already has strtok_s
// Nothing to do as strtok_s is already defined in string.h
#else
// For Linux/POSIX systems, define strtok_s to use strtok_r
#define strtok_s(str, delimiters, context) strtok_r(str, delimiters, context)
#endif

// we're not really using these yet... -MH
int convert_from_real(char *a, int b, void *c, PROPERTY *d) { return 0; }
int convert_to_real(const char *a, void *b, PROPERTY *c) { return 0; }
int convert_from_float(char *a, int b, void *c, PROPERTY *d) { return 0; }
int convert_to_float(const char *a, void *b, PROPERTY *c) { return 0; }

/** Convert from a \e void
        This conversion does not change the data
        @return 6, the number of characters written to the buffer, 0 if not
 enough space
 **/
int convert_from_void(
    char *buffer,   /**< a pointer to the string buffer */
    int size,       /**< the size of the string buffer */
    void *data,     /**< a pointer to the data that is not changed */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    if (size < 7)
        return 0;
    return sprintf(buffer, "%s", "(void)");
}

/** Convert to a \e void
        This conversion ignores the data
        @return always 1, indicated data was successfully ignored
 **/
int convert_to_void(
    const char *buffer, /**< a pointer to the string buffer that is ignored */
    void *data,         /**< a pointer to the data that is not changed */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    return 1;
}

/** Convert from a \e double
        Converts from a \e double property to the string.  This function uses
        the global variable \p global_double_format to perform the conversion.
        @return the number of characters written to the string
 **/
int convert_from_double(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[1025];
    int count = 0;

    double scale = 1.0;
    if (prop->unit != nullptr)
    {

        /* only do conversion if the target unit differs from the class's unit for
         * that property */
        PROPERTY *ptmp = (prop->oclass == nullptr
                              ? prop
                              : class_find_property(prop->oclass, prop->name));
        scale = *(double *)data;
        if (prop->unit != ptmp->unit && ptmp->unit != nullptr)
        {
            if (0 == unit_convert_ex(ptmp->unit, prop->unit, &scale))
            {
                output_error("convert_from_double(): unable to convert unit '%s' to "
                             "'%s' for property '%s' (tape experiment error)",
                             ptmp->unit->name, prop->unit->name, prop->name);
                return 0;
            }
            else
            {
                count = sprintf(temp, global_double_format, scale);
            }
        }
        else
        {
            count = sprintf(temp, global_double_format, *(double *)data);
        }
    }
    else
    {
        count = sprintf(temp, global_double_format, *(double *)data);
    }

    if (count < size + 1)
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
    else
    {
        return 0;
    }
}

/** Convert to a \e double
        Converts a string to a \e double property.  This function uses the
 global variable \p global_double_format to perform the conversion.
        @return 1 on success, 0 on failure, -1 is conversion was incomplete
 **/
int convert_to_double(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    char unit[256];
    int n = sscanf(buffer, "%lg%s", static_cast<double *>(data), unit);
    if (n > 1) /* something else given */
    {
        if (prop->unit != nullptr) /* Unit allowed - see if it is a valid unit */
        {
            UNIT *from = nullptr;
            if (strcmp(unit, "du") == 0)
            {
                from = prop->unit;
            }
            else
            {
                from = unit_find(unit);
            }
            if (from != prop->unit && unit_convert_ex(from, prop->unit, (double *)data) == 0)
            {
                output_error("convert_to_double(const char *buffer='%s', void *data=0x%*p, PROPERTY *prop={name='%s',...}): unit conversion failed", buffer, sizeof(void *), data, prop->name);
                /* TROUBLESHOOT
                This error is caused by an attempt to convert a value from a unit that is
                incompatible with the unit of the target property.  Check your units and
                try again.
                */
                return 0;
            }
        }
        else // Unit not specified, give a more general error
        {
            output_error("convert_to_double(const char *buffer='%s', void *data=0x%*p, PROPERTY *prop={name='%s',...}): conversion failed", buffer, sizeof(void *), data, prop->name);
            /* TROUBLESHOOT
            This error is caused by either an invalid entry in the conversion (extra decimal points), or
            with a unit specified where no unit was on the original property.  Check your source data (GLM entry
            or player file) and	try again.
            */
            return 0;
        }
    }
    return n;
}

/** Convert from a complex
        Converts a complex property to a string.  This function uses
        the global variable \p global_complex_format to perform the conversion.
        @return the number of character written to the string
 **/
int convert_from_complex(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    int count = 0;
    char temp[1025];
    gld::complex *v = static_cast<gld::complex *>(data);
    CNOTATION cplex_output_type = J;

    double scale = 1.0;
    if (prop->unit != nullptr)
    {

        /* only do conversion if the target unit differs from the class's unit for
         * that property */
        PROPERTY *ptmp = (prop->oclass == nullptr
                              ? prop
                              : class_find_property(prop->oclass, prop->name));

        if (prop->unit != ptmp->unit)
        {
            if (0 == unit_convert_ex(ptmp->unit, prop->unit, &scale))
            {
                output_error("convert_from_complex(): unable to convert unit '%s' to "
                             "'%s' for property '%s' (tape experiment error)",
                             ptmp->unit->name, prop->unit->name, prop->name);
                /*	TROUBLESHOOT
                        This is an error with the conversion of units from the complex
                   property's units to the requested units. Please double check the
                   units of the property and compare them to the units defined in the
                        offending tape object.
                */
                scale = 1.0;
            }
        }
    }

    /* Check the format or global override */
    if (global_complex_output_format == CNF_RECT)
    {
        cplex_output_type = J;
    }
    else if (global_complex_output_format == CNF_POLAR_DEG)
    {
        cplex_output_type = A;
    }
    else if (global_complex_output_format == CNF_POLAR_RAD)
    {
        cplex_output_type = R;
    }
    else /* Must be default - see what the property wants */
    {
        cplex_output_type = v->Notation();
    }

    if (cplex_output_type == 'P' || cplex_output_type == 'p')
    {
        cplex_output_type = A;
    }
    else if (cplex_output_type == 'I')
    {
        cplex_output_type = I;
    }
    else if (cplex_output_type == 'J')
    {
        cplex_output_type = J;
    }
    else if (cplex_output_type == 'D')
    {
        cplex_output_type = A;
    }
    else if (cplex_output_type == 'R')
    {
        cplex_output_type = R;
    }
    else if (cplex_output_type != I && cplex_output_type != J && cplex_output_type != A && cplex_output_type != R)
    {
        cplex_output_type = J;
    }

    /* Now output appropriately */
    if (cplex_output_type == A)
    {
        double m = v->Mag() * scale;
        double a = v->Arg();
        if (a > PI)
            a -= (2 * PI);
        count = sprintf(temp, global_complex_format, m, a * 180 / PI, A);
    }
    else if (cplex_output_type == R)
    {
        double m = v->Mag() * scale;
        double a = v->Arg();
        if (a > PI)
            a -= (2 * PI);
        count = sprintf(temp, global_complex_format, m, a, R);
    }
    else
    {
        count =
            sprintf(temp, global_complex_format, v->Re() * scale, v->Im() * scale,
                    cplex_output_type ? cplex_output_type : 'i');
    }
    if (count < size - 1)
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
    else
    {
        return 0;
    }
}

/** Convert to a complex
        Converts a string to a complex property.  This function uses the global
        variable \p global_complex_format to perform the conversion.
        @return 1 when only real is read, 2 imaginary part is also read, 3 when
 notation is also read, 0 on failure, -1 is conversion was incomplete
 **/
int convert_to_complex(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    gld::complex *v = (gld::complex *)data;
    char unit[256];
    char notation[2] = {'\0', '\0'}; /* force detection invalid complex number */
    int n;
    double a = 0, b = 0;
    if (buffer[0] == 0)
    {
        /* empty string */
        v->SetRect(0.0, 0.0, v->Notation());
        return 1;
    }
    // Normalize legacy/variant notation bytes so parsing is robust across
    // checkpoints written with mixed complex-output conventions.
    char norm[1024] = "";
    size_t wi = 0;
    for (size_t i = 0; buffer[i] != '\0' && wi + 1 < sizeof(norm); ++i)
    {
        unsigned char c = (unsigned char)buffer[i];
        // UTF-8 degree sign (C2 B0) -> degree notation 'd'
        if (c == 0xC2 && buffer[i + 1] != '\0' && (unsigned char)buffer[i + 1] == 0xB0)
        {
            norm[wi++] = 'd';
            ++i;
            continue;
        }
        // UTF-8 replacement char (EF BF BD) -> degree notation 'd'
        if (c == 0xEF && buffer[i + 1] != '\0' && buffer[i + 2] != '\0' && (unsigned char)buffer[i + 1] == 0xBF && (unsigned char)buffer[i + 2] == 0xBD)
        {
            norm[wi++] = 'd';
            i += 2;
            continue;
        }
        // Legacy polar-degree suffix 'P'/'p' -> 'd'
        if (c == 'P' || c == 'p')
        {
            norm[wi++] = 'd';
            continue;
        }
        // Accept upper-case notations
        if (c == 'I')
            c = 'i';
        else if (c == 'J')
            c = 'j';
        else if (c == 'D')
            c = 'd';
        else if (c == 'R')
            c = 'r';
        norm[wi++] = (char)c;
    }
    norm[wi] = '\0';

    n = sscanf(norm, "%lg%lg%1[ijdr]%s", &a, &b, notation, unit);
    if (n == 1) /* only real part */
        v->SetRect(a, 0, v->Notation());
    else if (n < 3 || strchr("ijdr", notation[0]) == nullptr) /* incomplete imaginary part or missing notation */
    {
        output_error("convert_to_complex('%s',%s): complex number format is not valid", buffer, prop->name);
        /* TROUBLESHOOT
            A complex number was given that doesn't meet the formatting requirements, e.g., <real><+/-><imaginary><notation>.
            Check the format of your complex numbers and try again.
         */
        return 0;
    }
    /* appears ok */
    else if (notation[0] == A) /* polar degrees */
        v->SetPolar(a, b * PI / 180.0, v->Notation());
    else if (notation[0] == R) /* polar radians */
        v->SetPolar(a, b, v->Notation());
    else
        v->SetRect(a, b, v->Notation()); /* rectangular */
    if (v->Notation() == I)              /* only override notation when property is using I */
        v->Notation() = (CNOTATION)notation[0];

    if (n > 3 && prop->unit != nullptr) /* unit given and unit allowed */
    {
        UNIT *from = nullptr;
        if (strcmp(unit, "du") == 0)
        {
            from = prop->unit;
        }
        else
        {
            from = unit_find(unit);
        }
        double scale = 1.0;
        if (from != prop->unit && unit_convert_ex(from, prop->unit, &scale) == 0)
        {
            output_error("convert_to_double(const char *buffer='%s', void *data=0x%*p, PROPERTY *prop={name='%s',...}): unit conversion failed", buffer, sizeof(void *), data, prop->name);
            /* TROUBLESHOOT
               This error is caused by an attempt to convert a value from a unit that is
               incompatible with the unit of the target property.  Check your units and
               try again.
             */
            return 0;
        }
        v->Re() *= scale;
        v->Im() *= scale;
    }
    return 1;
}

/** Convert from an \e enumeration
        Converts an \e enumeration property to a string.
        @return the number of character written to the string
 **/
int convert_from_enumeration(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    KEYWORD *keys = prop->keywords;
    int count = 0;
    char temp[1025];
    /* get the true value */
    int value = *(uint32 *)data;

    /* process the keyword list, if any */
    for (; keys != nullptr; keys = keys->next)
    {
        /* if the key value matched */
        if (keys->value == value)
        {
            /* use the keyword */
            count = strncpy(temp, keys->name, 1024) ? (int)strlen(temp) : 0;
            break;
        }
    }

    /* no keyword found, return the numeric value instead */
    if (count == 0)
    {
        count = sprintf(temp, "%d", value);
    }
    if (count < size - 1)
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
    else
    {
        return 0;
    }
}

/** Convert to an \e enumeration
        Converts a string to an \e enumeration property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_enumeration(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    bool found = false;
    KEYWORD *keys = prop->keywords;

    /* process the keyword list */
    for (; keys != nullptr; keys = keys->next)
    {
        if (strcmp(keys->name, buffer) == 0)
        {
            *(uint32 *)data = (uint32)(keys->value);
            found = true;
            break;
        }
    }
    if (found)
        return 1;
    if (strncmp(buffer, "0x", 2) == 0)
        return sscanf(buffer + 2, "%x", (uint32 *)data);
    if (isdigit(*buffer))
        return sscanf(buffer, "%d", (uint32 *)data);
    else if (strcmp(buffer, "") == 0)
        return 0; // empty string do nothing
    output_error("keyword '%s' is not valid for property %s", buffer, prop->name);
    return 0;
}

/** Convert from an \e set
        Converts a \e set property to a string.
        @return the number of character written to the string
 **/
#define SETDELIM "|"
int convert_from_set(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    KEYWORD *keys;

    /* get the actual value */
    // unsigned int64 value = *(unsigned int64*)data;
    uint32_t value = *static_cast<uint32_t *>(data);

    /* keep track of how characters written */
    int count = 0;

    int ISZERO = (value == 0);
    /* clear the buffer */
    buffer[0] = '\0';

    /* process each keyword */
    for (keys = prop->keywords; keys != nullptr; keys = keys->next)
    {
        /* if the keyword matches */
        if ((!ISZERO && keys->value != 0 && (keys->value & value) == keys->value) ||
            (keys->value == 0 && ISZERO))
        {
            /* get the length of the keyword */
            int len = (int)strlen(keys->name);

            /* remove the key from the copied values */
            value &= ~(keys->value);

            /* if there's room for it in the buffer */
            if (size > count + len + 1)
            {
                /* if the buffer already has keywords in it */
                if (buffer[0] != '\0')
                {
                    /* add a separator to the buffer */
                    if (!(prop->flags & PF_CHARSET))
                    {
                        count++;
                        strcat(buffer, SETDELIM);
                    }
                }

                /* add the keyword to the buffer */
                count += len;
                strcat(buffer, keys->name);
            }

            /* no room in the buffer */
            else

                /* fail */
                return 0;
        }
    }

    /* succeed */
    return count;
}

/** Convert to a \e set
        Converts a string to a \e set property.
        @return number of values read on success, 0 on failure, -1 if conversion
 was incomplete
 **/
int convert_to_set(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    KEYWORD *keys = prop->keywords;
    char temp[4096];
    const char *ptr;
    uint32 value = 0;
    int count = 0;

    /* directly convert numeric strings */
    if (strnicmp_portable(buffer, "0x", 2) == 0)
        return sscanf(buffer, "0x%x", (uint32 *)data);
    else if (isdigit(buffer[0]))
        return sscanf(buffer, "%d", (uint32 *)data);

    /* prevent long buffer from being scanned */
    if (strlen(buffer) > sizeof(temp) - 1)
        return 0;

    /* make a temporary copy of the buffer */
    strcpy(temp, buffer);

    /* check for CHARSET keys (single character keys) and usage without | */
    if ((prop->flags & PF_CHARSET) && strchr(buffer, '|') == nullptr)
    {
        for (ptr = buffer; *ptr != '\0'; ptr++)
        {
            bool found = false;
            KEYWORD *key;
            for (key = keys; key != nullptr; key = key->next)
            {
                if (*ptr == key->name[0])
                {
                    value |= key->value;
                    count++;
                    found = true;
                    break; /* we found our key */
                }
            }
            if (!found)
            {
                output_error("set member '%c' is not a keyword of property %s", *ptr,
                             prop->name);
                return 0;
            }
        }
    }
    else
    {
        /* process each keyword in the temporary buffer*/
        for (ptr = strtok(temp, SETDELIM); ptr != nullptr;
             ptr = strtok(nullptr, SETDELIM))
        {
            bool found = false;
            KEYWORD *key;

            /* scan each of the keywords in the set */
            for (key = keys; key != nullptr; key = key->next)
            {
                if (strcmp(ptr, key->name) == 0)
                {
                    value |= key->value;
                    count++;
                    found = true;
                    break; /* we found our key */
                }
            }
            if (!found)
            {
                output_error("set member '%s' is not a keyword of property %s", ptr,
                             prop->name);
                return 0;
            }
        }
    }
    *(uint32 *)data = value;
    return count;
}

/** Convert from an \e int16
        Converts an \e int16 property to a string.
        @return the number of character written to the string
 **/
int convert_from_int16(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[1025];
    int count = sprintf(temp, "%hd", *(short *)data);
    if (count < size - 1)
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
    else
    {
        return 0;
    }
}

/** Convert to an \e int16
        Converts a string to an \e int16 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_int16(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    return sscanf(buffer, "%hd", static_cast<short *>(data));
}

/** Convert from an \e uint32
        Converts an \e int32 property to a string.
        @return the number of character written to the string
 **/
int convert_from_uint32(char *buffer, int size, void *data, PROPERTY *prop)
{
    if (!buffer || !data)
    {              // Null pointer checks
        return -1; // Return an error code distinct from success
    }

    const int MAX_LENGTH = 1024; // Define a safe maximum length for numbers
    char temp[MAX_LENGTH + 1];

    // Use snprintf for safer bounds checking
    int count =
        snprintf(temp, sizeof(temp), "%d", *(reinterpret_cast<int *>(data)));
    if (count < 0 || count > size - 1)
    {              // Handle snprintf errors and size issues
        return -2; // Return error code for overflow
    }

    strncpy(buffer, temp, static_cast<size_t>(count)); // Safer copy
    buffer[count] = '\0';                              // Null-terminate explicitly
    return count;                                      // Return character count
}

/** Convert to an \e uint32
        Converts a string to an \e uint32 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_uint32(const char *buffer, void *data, PROPERTY *prop)
{
    if (!buffer || !data)
    {              // Null pointer check
        return -1; // Return an error code for null pointers
    }

    int temp;
    int result = sscanf(buffer, "%d", &temp); // Parse into a temporary integer
    if (result != 1)
    {             // Conversion failed
        return 0; // Return 0 for failed conversions
    }

    *reinterpret_cast<int *>(data) = temp; // Write back to `data`
    return 1;                              // Success
}

/** Convert from an \e int32
        Converts an \e int32 property to a string.
        @return the number of character written to the string
 **/
int convert_from_int32(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[1025];
    int count = sprintf(temp, "%d", *(int *)data);
    if (count < size - 1)
    {
        memcpy(buffer, temp, static_cast<size_t>(count));
        buffer[count] = 0;
        return count;
    }
    else
    {
        return 0;
    }
}
#ifdef _WIN32
#define SCNd32 "d"
#endif
/** Convert to an \e int32
        Converts a string to an \e int32 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_int32(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    return sscanf(buffer, "%d", static_cast<int *>(data));
}

/** Convert from an \e int64
        Converts an \e int64 property to a string.
        @return the number of character written to the string
 **/
int convert_from_int64(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[1025];
    int count = sprintf(temp, "%" FMT_INT64 "d", *(int64 *)data);
    if (count < size - 1)
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
    else
    {
        return 0;
    }
}

/** Convert to an \e int64
        Converts a string to an \e int64 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_int64(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    return sscanf(buffer, "%" FMT_INT64 "d", static_cast<long long *>(data));
}

/** Convert from a \e char8
        Converts a \e char8 property to a string.
        @return the number of character written to the string
 **/
int convert_from_char8(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[1025];
    const char *format = "%s";
    int count = 0;
    if (strchr((char *)data, ' ') != nullptr ||
        strchr((char *)data, ';') != nullptr || ((char *)data)[0] == '\0')
        format = "\"%s\"";
    count = sprintf(temp, format, (char *)data);
    if (count > size - 1)
    {
        return 0;
    }
    else
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
}

/** Convert to a \e char8
        Converts a string to a \e char8 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_char8(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    char c = ((char *)buffer)[0];
    switch (c)
    {
    case '\0':
        return ((char *)data)[0] = '\0', 1;
    case '"':
        return sscanf(buffer + 1, "%8[^\"]", static_cast<char *>(data));
    default:
        return sscanf(buffer, "%8s", static_cast<char *>(data));
    }
}

/** Convert from a \e char32
        Converts a \e char32 property to a string.
        @return the number of character written to the string
 **/
int convert_from_char32(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[1025];
    const char *format = "%s";
    int count = 0;
    if (strchr((char *)data, ' ') != nullptr ||
        strchr((char *)data, ';') != nullptr || ((char *)data)[0] == '\0')
        format = "\"%s\"";
    count = sprintf(temp, format, (char *)data);
    if (count > size - 1)
    {
        return 0;
    }
    else
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
}

/** Convert to a \e char32
        Converts a string to a \e char32 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
// I�ve changed the signature so that `buffer` is a C-string
// and `data` is a char array of exactly 32 bytes.

// signature unchanged
int convert_to_char32(const char *buffer, void *data, PROPERTY *prop)
{
    // silence �unused� warning for prop
    (void)prop;

    // out is our 32-byte destination
    char *out = static_cast<char *>(data);

    // work with a string_view for easy slicing
    std::string_view sv{buffer ? buffer : ""};

    // 1) empty string special case from your original code
    if (sv.empty())
    {
        out[0] = '\0';
        return 1; // you returned 1 for the empty buffer case
    }

    // token will point at the text we want to copy
    std::string_view token;

    // 2) if it starts with a quote, grab everything up to the next quote
    if (sv.front() == '"')
    {
        // find the closing quote (or end of string)
        auto end = sv.find('"', /*startpos=*/1);
        if (end == std::string_view::npos)
            end = sv.size();
        // slice out the contents between the quotes
        token = sv.substr(1, end - 1);
    }
    else
    {
        // skip any leading whitespace (sscanf("%s") skips it for us)
        auto start = sv.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos)
        {
            // nothing but whitespace => no assignment
            out[0] = '\0';
            return 0;
        }
        // find the next whitespace to delimit the token
        auto end = sv.find_first_of(" \t\r\n", start);
        if (end == std::string_view::npos)
            token = sv.substr(start);
        else
            token = sv.substr(start, end - start);
    }

    // 3) copy at most 31 chars, then NUL-terminate
    constexpr size_t OUT_SZ = 32;
    constexpr size_t MAX_CH = OUT_SZ - 1;

    size_t len = std::min(token.size(), MAX_CH);
    std::memcpy(out, token.data(), len);
    out[len] = '\0';

    // sscanf would have returned 1 on a successful assignment,
    // so we do the same if we actually copied anything (>0 length)
    return (len > 0) ? 1 : 0;
}

// int convert_to_char32(const char *buffer, /**< a pointer to the string buffer
// */ 					    void *data, /**< a pointer to the data */ 					    PROPERTY *prop) /**< a pointer
// to keywords that are supported */
//{
//	char c=((char*)buffer)[0];
//	switch (c) {
//	case '\0':
//		return ((char*)data)[0]='\0', 1;
//	case '"':
//		return sscanf(buffer+1,"%32[^\"]",static_cast<char*>(data));
//	default:
//		return sscanf(buffer,"%32s", static_cast<char*>(data));
//	}
// }

/** Convert from a \e char256
        Converts a \e char256 property to a string.
        @return the number of character written to the string
 **/
int convert_from_char256(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[1025];
    const char *format = "%s";
    int count = 0;
    if (strchr((char *)data, ' ') != nullptr ||
        strchr((char *)data, ';') != nullptr || ((char *)data)[0] == '\0')
        format = "\"%s\"";
    count = sprintf(temp, format, (char *)data);
    if (count > size - 1)
    {
        return 0;
    }
    else
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
}

/** Convert to a \e char256
        Converts a string to a \e char256 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_char256(const char *buffer, void *data, PROPERTY *prop)
{
    // keep the signature the same
    (void)prop;

    // our 256 byte destination
    char *out = static_cast<char *>(data);

    // build a safe string_view (treat nullptr as empty)
    std::string_view sv{buffer ? buffer : ""};

    // 1) empty-string case
    if (sv.empty())
    {
        out[0] = '\0';
        return 1;
    }

    // 2) decide which chunk of text to copy
    std::string_view token;
    if (sv.front() == '"')
    {
        // quoted: grab everything up to the next quote (or EOL)
        auto end = sv.find('"', /*start=*/1);
        if (end == std::string_view::npos)
            end = sv.size();
        token = sv.substr(1, end - 1);
    }
    else
    {
        // unquoted: skip leading whitespace, then stop at '\n', '\r', or ';'
        auto start = sv.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos)
        {
            out[0] = '\0';
            return 0;
        }
        auto end = sv.find_first_of("\n\r;", start);
        if (end == std::string_view::npos)
            token = sv.substr(start);
        else
            token = sv.substr(start, end - start);
    }

    // 3) copy into our 256 byte buffer (255 chars + NUL)
    constexpr size_t OUT_SZ = 256;
    constexpr size_t MAX_CH = OUT_SZ - 1;

    size_t len = std::min(token.size(), MAX_CH);
    std::memcpy(out, token.data(), len);
    out[len] = '\0';

    // mimic sscanf�s return value: 1 if we actually stored something, else 0
    return (len > 0) ? 1 : 0;
}

// int convert_to_char256(const char *buffer, /**< a pointer to the string
// buffer */ 					    void *data, /**< a pointer to the data */ 					    PROPERTY *prop) /**< a
// pointer to keywords that are supported */
//{
//	char c=((char*)buffer)[0];
//	switch (c) {
//	case '\0':
//		return ((char*)data)[0]='\0', 1;
//	case '"':
//		return sscanf(buffer+1,"%256[^\"]", static_cast<char*>(data));
//	default:
//		//return sscanf(buffer,"%256s",data);
//		return sscanf(buffer,"%256[^\n\r;]", static_cast<char*>(data));
//	}
// }

/** Convert from a \e char1024
        Converts a \e char1024 property to a string.
        @return the number of character written to the string
 **/
int convert_from_char1024(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    char temp[4097];
    const char *format = "%s";
    int count = 0;
    if (strchr((char *)data, ' ') != nullptr ||
        strchr((char *)data, ';') != nullptr || ((char *)data)[0] == '\0')
        format = "\"%s\"";
    count = sprintf(temp, format, (char *)data);
    if (count > size - 1)
    {
        return 0;
    }
    else
    {
        memcpy(buffer, temp, count);
        buffer[count] = 0;
        return count;
    }
}

/** Convert to a \e char1024
        Converts a string to a \e char1024 property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_char1024(const char *_buffer, void *_data, PROPERTY *_prop)
{
    (void)_prop; // Suppress unused parameter warnings, or use it if needed.

    // Validate the input buffer pointer
    if (!_buffer)
    {
        throw std::invalid_argument("The _buffer pointer is null.");
    }

    // Validate the _data pointer
    if (!_data)
    {
        throw std::invalid_argument("The _data pointer is null.");
    }

    // Cast data to a character pointer
    char *data = static_cast<char *>(_data);

    // Handle the case where the buffer string starts with '\0'
    char c = _buffer[0];
    if (c == '\0')
    {
        data[0] = '\0';
        return 1; // Successfully handled
    }

    // Handle the case where the buffer string starts with a double quote ('"')
    if (c == '"')
    {
        // Safely parse the content within quotes
        if (sscanf(_buffer + 1, "%1023[^\"]", data) == 1)
        {
            return 1; // Successfully parsed
        }
        else
        {
            throw std::runtime_error("Failed to parse the quoted string.");
        }
    }

    // Handle all other cases (read until a newline character or max size)
    if (sscanf(_buffer, "%1023[^\n]", data) == 1)
    {
        return 1; // Successfully parsed
    }
    else
    {
        throw std::runtime_error("Failed to parse the input string.");
    }
}

// int convert_to_char1024(const char *buffer, /**< a pointer to the string
// buffer */ 					    void *data, /**< a pointer to the data */ 					    PROPERTY *prop) /**< a
// pointer to keywords that are supported */
//{
//	char c=((char*)buffer)[0];
//	switch (c) {
//	case '\0':
//		return ((char*)data)[0]='\0', 1;
//	case '"':
//		return sscanf(buffer+1,"%1024[^\"]", static_cast<char*>(data));
//	default:
//		return sscanf(buffer,"%1024[^\n]",static_cast<char*>(data));
//	}
// }

/** Convert from an \e object
        Converts an \e object reference to a string.
        @return the number of character written to the string
 **/
int convert_from_object(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    OBJECT *obj = (data ? *(OBJECT **)data : nullptr);
    char temp[256];
    memset(temp, 0, 256);
    if (obj == nullptr)
    {
        strcpy(buffer, "");
        return 1;
    }

    /* get the object's namespace */
    if (object_current_namespace() != obj->space)
    {
        if (object_get_namespace(obj, buffer, size))
            strcat(buffer, "::");
    }
    else
        strcpy(buffer, "");

    if (obj->name != nullptr)
    {
        size_t a = strlen(obj->name);
        size_t b = size - 1;
        // if ((strlen(obj->name) != 0) && (strlen(obj->name) < (size_t)(size -
        // 1))){
        if ((a != 0) && (a < b))
        {
            strcat(buffer, obj->name);
            return 1;
        }
    }

    /* construct the object's name */
    if (obj->oclass != nullptr &&
        (sprintf(temp, global_object_format, obj->oclass->name, obj->id) < size))
        strcat(buffer, temp);
    else
        return 0;
    return 1;
}

/** Convert to an \e object
        Converts a string to an \e object property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_object(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    CLASSNAME cname;
    OBJECTNUM id;
    OBJECT **target = (OBJECT **)data;
    char oname[256];
    if (strcmp(buffer, "0") == 0) // NOTE: this is inconsistent with what
                                  // convert_from_object does for nullptr object
    {
        *target = nullptr;
        return 1;
    }
    else if (sscanf(buffer, "\"%[^\"]\"", oname) == 1 ||
             (strchr(buffer, ':') == nullptr &&
              strncpy(oname, buffer, sizeof(oname))))
    {
        oname[sizeof(oname) - 1] = '\0'; /* terminate unterminated string */
        *target = object_find_name(oname);
        return (*target) != nullptr;
    }
    else if (sscanf(buffer, global_object_scan, cname, &id) == 2)
    {
        OBJECT *obj = object_find_by_id(id);
        if (obj == nullptr)
        { /* failure case, make noisy if desired. */
            *target = nullptr;
            return 0;
        }
        if (obj != nullptr && strcmp(obj->oclass->name, cname) == 0)
        {
            *target = obj;
            return 1;
        }
    }
    else
        *target = nullptr;
    return 0;
}

/** Convert from a \e delegated data type
        Converts a \e delegated data type reference to a string.
        @return the number of character written to the string
 **/
int convert_from_delegated(
    char *buffer,   /**< pointer to the string buffer */
    int size,       /**< size of the string buffer */
    void *data,     /**< a pointer to the data */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    DELEGATEDVALUE *value = (DELEGATEDVALUE *)data;
    if (value == nullptr || value->type == nullptr ||
        value->type->to_string == nullptr)
        return 0;
    else
        return (*(value->type->to_string))(value->data, buffer, size);
}

/** Convert to a \e delegated data type
        Converts a string to a \e delegated data type property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_delegated(
    const char *buffer, /**< a pointer to the string buffer */
    void *data,         /**< a pointer to the data */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    DELEGATEDVALUE *value = (DELEGATEDVALUE *)data;
    if (value == nullptr || value->type == nullptr ||
        value->type->from_string == nullptr)
        return 0;
    else
        return (*(value->type->from_string))(value->data, buffer);
}

/** Convert from a \e boolean data type
        Converts a \e boolean data type reference to a string.
        @return the number of characters written to the string
 **/
int convert_from_boolean(char *buffer, int size, void *data, PROPERTY *prop)
{
    unsigned int b = 0;
    if (buffer == nullptr || data == nullptr || prop == nullptr)
        return 0;
    b = *(bool *)data;
    if (b == 1 && (size > 4))
    {
        return sprintf(buffer, "TRUE");
    }
    if (b == 0 && (size > 5))
    {
        return sprintf(buffer, "FALSE");
    }
    return 0;
}

/** Convert to a \e boolean data type
        Converts a string to a \e boolean data type property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
/* booleans are handled internally as 1-byte uchar's. -MH */
int convert_to_boolean(const char *buffer, void *data, PROPERTY *prop)
{
    char str[32];
    if (sscanf(buffer, "%31[A-Za-z]", str) == 1)
    {
        if (stricmp_portable(str, "TRUE") == 0)
        {
            *(bool *)data = 1;
            return 1;
        }
        if (stricmp_portable(str, "FALSE") == 0)
        {
            *(bool *)data = 0;
            return 1;
        }
        return 0;
    }

    int v;
    if (sscanf(buffer, "%d", &v) == 1)
    {
        *(bool *)data = (v != 0);
        return 1;
    }

    return 0;
}

int convert_from_timestamp_stub(char *buffer, int size, void *data,
                                PROPERTY *prop)
{
    TIMESTAMP ts = *(int64 *)data;
    return convert_from_timestamp(ts, buffer, size);
    // return 0;
}

int convert_to_timestamp_stub(const char *buffer, void *data, PROPERTY *prop)
{
    TIMESTAMP ts = convert_to_timestamp(buffer);
    *(int64 *)data = ts;
    return 1;
}

/** Convert from a \e double_array data type
        Converts a \e double_array data type reference to a string.
        @return the number of character written to the string
 **/
// Example function to simulate converting a double value into a string
// int convert_from_double(char* buffer, int size, void* data, PROPERTY* prop) {
//	double value = *reinterpret_cast<double*>(data); // Extract the double
// value 	return snprintf(buffer, size, "%.3f", value); // Serialize into the
// buffer
// }

// Implementation of convert_from_double_array
int convert_from_double_array(char *_buffer, int size, void *data,
                              PROPERTY *prop)
{
    Eigen::MatrixXd &_a =
        *reinterpret_cast<Eigen::MatrixXd *>(data); // Cast void* data to MatrixXd
    unsigned int n, m;                              // Row and column counters
    unsigned int p = 0;                             // Buffer position tracker

    for (n = 0; n < _a.rows(); n++)
    {
        for (m = 0; m < _a.cols(); m++)
        {
            // Handle NaN
            if (emh::is_element_nan(_a, n, m))
            {
                p += snprintf(_buffer + p, size - p, "%s", "NAN");
            }
            else
            {
                // Serialize valid double elements
                /*p += convert_from_double(_buffer + p, size - p, get_element_addr(_a,
                 * n, m), prop);*/
                p += convert_from_double(_buffer + p, size - p,
                                         reinterpret_cast<void *>(&_a(n, m)), prop);
            }

            // Add space between columns (but not for the last column in a row)
            if (m < _a.cols() - 1)
            {
                std::strcpy(_buffer + p++, " ");
            }
        }

        // Add semicolon between.rows() (but not for the last row of the matrix)
        if (n < _a.rows() - 1)
        {
            std::strcpy(_buffer + p++, ";");
        }
    }

    return p; // Return the total number of bytes added to the buffer
}
// int convert_from_double_array(char *buffer, int size, void *data, PROPERTY
// *prop)
//{
////	double_array *a = (double_array*)data;
//    double_array *a= new double_array(0, 0,
//    reinterpret_cast<double**>(&data)); unsigned int n,m;
//	unsigned int p = 0;
//	for ( n=0 ; n<a-.rows() ; n++ )
//	{
//		for ( m=0 ; m<a->cols() ; m++ )
//		{
//			if ( a->is_nan(n,m) )
//				p += sprintf(buffer+p,"%s","NAN");
//			else
//				p +=
// convert_from_double(buffer+p,size,(void*)a->get_addr(n,m),prop); 			if (
// m<a->cols()-1 ) strcpy(buffer+p++," ");
//		}
//		if ( n<a-.rows()-1 ) strcpy(buffer+p++,";");
//	}
//	return p;
//}

int convert_to_double_array(const char *_buffer, void *data, PROPERTY *prop)
{
    Eigen::MatrixXd &_a =
        *reinterpret_cast<Eigen::MatrixXd *>(data); // Cast void* to MatrixXd
    Eigen::Index row = 0;
    Eigen::Index col = 0;

    // Clear the matrix initially
    _a.resize(0, 0);

    const char *p = _buffer; // Pointer to buffer for parsing
    std::string word;        // Temporary storage for each token

    std::stringstream ss(_buffer); // Use stringstream to tokenize

    // Parse input buffer
    while (std::getline(ss, word,
                        ' '))
    { // Use space delimiter for basic token parsing

        // Skip spaces
        if (word.empty() || std::isspace(word[0]))
        {
            continue;
        }

        // Process end of row (semicolon)
        if (word == ";")
        {
            row++;
            col = 0;
            continue;
        }

        // Process special "NAN" value
        if (word == "NAN")
        {
            if (row >= _a.rows())
            {
                _a.conservativeResize(row + 1, std::max(_a.cols(), col + 1));
            }
            _a(row, col) = std::numeric_limits<double>::quiet_NaN(); // Set to NaN
            col++;
        }
        // Process numerical value
        else if (std::isdigit(word[0]) || word[0] == '.' || word[0] == '-' ||
                 word[0] == '+')
        {
            double value = std::atof(word.c_str()); // Convert to double
            if (row >= _a.rows() || col >= _a.cols())
            {
                _a.conservativeResize(row + 1,
                                      std::max(_a.cols(), col + 1)); // Grow matrix
            }
            _a(row, col) = value; // Set value
            col++;
        }
        // Handle object property or other unsupported types
        else
        {
            std::cerr << "Unsupported or invalid value: " << word << " at row " << row
                      << ", col " << col << std::endl;
            return 0; // Unsupported
        }
    }

    return 1; // Success
}

/** Convert to a \e double_array data type
        Converts a string to a \e double_array data type property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
// int convert_to_double_array(const char *buffer, void *data, PROPERTY *prop)
//{
//	unsigned row=0, col=0;
//	double_array *a= new double_array(row, col,
// reinterpret_cast<double**>(&data)); 	a->set_name(prop->name); 	const char *p =
// buffer;
//
//	/* new array */
//	/* parse input */
//	for ( p=buffer ; *p!='\0' ; )
//	{
//		char value[256];
//		char objectname[64], propertyname[64];
//		while ( *p!='\0' && isspace(*p) ) p++; /* skip spaces */
//		if ( *p!='\0' && sscanf(p,"%s",value)==1 )
//		{
//
//			if ( *p==';' ) /* end row */
//			{
//				row++;
//				col=0;
//				p++;
//				continue;
//			}
//			else if ( strnicmp(p,"NAN",3)==0 ) /* nullptr value */
//			{
//				a->resize(row,col);
//				a->clr_at(row,col);
//				col++;
//			}
//			else if ( isdigit(*p) || *p=='.' || *p=='-' || *p=='+' )
///* probably real value */
//			{
//				a->resize(row+1,col+1);
//				a->set_at(row,col,atof(p));
//				col++;
//			}
//			else if ( sscanf(value,"%[^.].%[^;
//\t]",objectname,propertyname)==2 ) /* object property */
//			{
//				OBJECT *obj = load_get_current_object();
//				if ( obj!=nullptr &&
// strcmp(objectname,"parent")==0 ) 					obj = obj->parent; 				else if (
// strcmp(objectname,"this")!=0 ) 					obj = object_find_name(objectname); 				if (
// obj==nullptr )
//				{
//					output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d - object property '%s'
// not found", buffer,row,col,objectname); 					return 0;
//				}
//				PROPERTY *prop =
// object_get_property(obj,propertyname,nullptr); 				if ( prop==nullptr )
//				{
//					output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d - property '%s' not found
// in object '%s'", buffer,row,col,propertyname,objectname); 					return 0;
//				}
//				a->resize(row+1,col+1);
//				a->set_at(row,col,object_get_double(obj,prop));
//				if ( a->is_nan(row,col) )
//				{
//					output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object
//'%s' is not accessible", buffer,row,col,propertyname,objectname); 					return 0;
//				}
//				col++;
//			}
//			else if ( sscanf(value,"%[^; \t]",propertyname)==1 ) /*
// current object/global property */
//			{
//				OBJECT *obj;
//				PROPERTY *target = nullptr;
//				obj  = (OBJECT*)((char*)data -
//(char*)prop->addr)-1; 				object_name(obj,objectname,sizeof(objectname)); 				target =
// object_get_property(obj,propertyname,nullptr); 				if ( target!=nullptr )
//				{
//					if ( target->ptype!=PT_double &&
// target->ptype!=PT_random && target->ptype!=PT_enduse &&
// target->ptype!=PT_loadshape && target->ptype!=PT_enduse )
//					{
//						output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object
//'%s' refers to property '%s', which is not an underlying double",
//								buffer,row,col,propertyname,objectname,target->name);
//						return 0;
//					}
//					a->resize(row+1,col+1);
//					a->set_at(row,col,object_get_double(obj,target));
//					if ( a->is_nan(row,col) )
//					{
//						output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object
//'%s' is not accessible", buffer,row,col,propertyname,objectname); 						return 0;
//					}
//					col++;
//				}
//				else
//				{
//					GLOBALVAR *var =
// global_find(propertyname); 					if ( var==nullptr )
//					{
//						output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d global '%s' not found",
// buffer,row,col,propertyname); 						return 0;
//					}
//					if ( var->prop->ptype!=PT_double )
//					{
//						output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object
//'%s' refers to a global '%s', which is not an underlying double",
// buffer,row,col,propertyname,objectname,propertyname); 						return 0;
//					}
//					a->resize(row+1,col+1);
//					a->set_at(row,col,(double*)var->prop->addr);
//					if ( a->is_nan(row,col) )
//					{
//						output_error("convert_to_double_array(const
// char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object
//'%s' is not accessible", buffer,row,col,propertyname,objectname); 						return 0;
//					}
//					col++;
//				}
//			}
//			else /* not a valid entry */
//			{
//				output_error("convert_to_double_array(const char
//*buffer='%10s...',...): entry at row %d, col %d is not valid (value='%10s')",
// buffer,row,col,p); 				return 0;
//			}
//			while ( *p!='\0' && !isspace(*p) && *p!=';' ) p++; /*
// skip characters just parsed */
//		}
//	}
//	return 1;
// }

// int convert_from_complex(char* buffer, int size, void* data, PROPERTY* prop)
// { 	std::complex<double>* complex_value =
// reinterpret_cast<std::complex<double>*>(data); 	return snprintf(buffer, size,
//"(%.3f, %.3f)", complex_value->real(), complex_value->imag());
// }

int convert_from_complex_array(char *buffer, int size, void *data,
                               PROPERTY *prop)
{
    Eigen::MatrixXcd &_a = *reinterpret_cast<Eigen::MatrixXcd *>(
        data); // Cast void* data to Eigen::MatrixXcd
    unsigned int n, m;

    unsigned int p = 0; // Position in the buffer
    for (n = 0; n < _a.rows(); n++)
    {
        for (m = 0; m < _a.cols(); m++)
        {
            // Check if the current element is NaN
            if (emh::is_element_nan(_a, n, m))
            {
                // Serialize "NAN" into the buffer
                p += snprintf(buffer + p, size - p, "%s", "NAN");
            }
            else
            {
                // Serialize the complex value into the buffer using
                // convert_from_complex
                /*p += convert_from_complex(buffer + p, size - p, get_element_addr(_a,
                 * n, m), prop);*/
                p += convert_from_complex(buffer + p, size - p,
                                          reinterpret_cast<void *>(&_a(n, m)), prop);
            }

            // Add space between columns
            if (m < _a.cols() - 1)
            {
                std::strcpy(buffer + p++, " ");
            }
        }

        // Add semicolon between.rows()
        if (n < _a.rows() - 1)
        {
            std::strcpy(buffer + p++, ";");
        }
    }

    return p; // Return the total buffer length used
}

/** Convert from a \e complex_array data type
        Converts a \e complex_array data type reference to a string.
        @return the number of character written to the string
 **/
// int convert_from_complex_array(char *buffer, int size, void *data, PROPERTY
// *prop)
//{
//	Eigen::MatrixXcd *a = (Eigen::MatrixXcd*)data;
//	unsigned int n,m;
//	unsigned int p = 0;
//	for ( n=0 ; n<a-.rows() ; n++ )
//	{
//		for ( m=0 ; m<a->cols() ; m++ )
//		{
//			if (is_element_nan(*a,n,m) )
//				p += sprintf(buffer+p,"%s","NAN");
//			else
//				p +=
// convert_from_complex(buffer+p,size,(void*)get_element_addr(*a,n,m),prop); 			if (
// m<a->cols()-1 ) strcpy(buffer+p++," ");
//		}
//		if ( n<a-.rows()-1 ) strcpy(buffer+p++,";");
//	}
//	return p;
// }

/** Convert to a \e complex_array data type
        Converts a string to a \e complex_array data type property.
        @return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_complex_array(const char *buffer, void *data, PROPERTY *prop)
{
    Eigen::MatrixXcd *a = (Eigen::MatrixXcd *)data;
    unsigned row = 0, col = 0;
    const char *p = buffer;

    /* new array */
    /* parse input */
    for (p = buffer; *p != '\0';)
    {
        char value[256];
        char objectname[64], propertyname[64];
        gld::complex c;
        while (*p != '\0' && isspace(*p))
            p++; /* skip spaces */
        if (*p != '\0' && sscanf(p, "%s", value) == 1)
        {

            if (*p == ';') /* end row */
            {
                row++;
                col = 0;
                p++;
                continue;
            }
            else if (strnicmp_portable(p, "NAN", 3) == 0) /* nullptr value */
            {
                a->resize(row, col);
                (*a)(row, col) = 0.0;
                col++;
            }
            else if (convert_to_complex(value, (void *)&c,
                                        prop)) /* probably real value */
            {
                a->resize(row, col);
                (*a)(row, col) = c;
                col++;
            }
            else if (sscanf(value, "%[^.].%[^; \t]", objectname, propertyname) ==
                     2) /* object property */
            {
                OBJECT *obj = object_find_name(objectname);
                PROPERTY *prop;
                if (obj == nullptr)
                {
                    output_error(
                        "convert_to_double_array(const char *buffer='%10s...',...): "
                        "entry at row %d, col %d - object '%s' not found",
                        buffer, row, col, objectname);
                    return 0;
                }
                prop = object_get_property(obj, propertyname, nullptr);
                if (prop == nullptr)
                {
                    output_error("convert_to_double_array(const char "
                                 "*buffer='%10s...',...): entry at row %d, col %d - "
                                 "property '%s' not found in object '%s'",
                                 buffer, row, col, propertyname, objectname);
                    return 0;
                }
                a->resize(row, col);
                (*a)(row, col) = *object_get_complex(obj, prop);
                if (emh::is_element_nan(*a, row, col))
                {
                    output_error("convert_to_double_array(const char "
                                 "*buffer='%10s...',...): entry at row %d, col %d "
                                 "property '%s' in object '%s' is not accessible",
                                 buffer, row, col, propertyname, objectname);
                    return 0;
                }
                col++;
            }
            else if (sscanf(value, "%[^; \t]", propertyname) ==
                     1) /* object property */
            {
                GLOBALVAR *var = global_find(propertyname);
                if (var == nullptr)
                {
                    output_error(
                        "convert_to_double_array(const char *buffer='%10s...',...): "
                        "entry at row %d, col %d global '%s' not found",
                        buffer, row, col, propertyname);
                    return 0;
                }
                a->resize(row, col);
                (*a)(row, col) = *(gld::complex *)var->prop->addr;
                if (emh::is_element_nan(*a, row, col))
                {
                    output_error("convert_to_double_array(const char "
                                 "*buffer='%10s...',...): entry at row %d, col %d "
                                 "property '%s' in object '%s' is not accessible",
                                 buffer, row, col, propertyname, objectname);
                    return 0;
                }
                col++;
            }
            else /* not a valid entry */
            {
                output_error(
                    "convert_to_double_array(const char *buffer='%10s...',...): entry "
                    "at row %d, col %d is not valid (value='%10s')",
                    buffer, row, col, p);
                return 0;
            }
            while (*p != '\0' && !isspace(*p) && *p != ';')
                p++; /* skip characters just parsed */
        }
    }
    return 1;
}

/** Convert a string to a double with a given unit
   @return 1 on success, 0 on failure
 **/
extern "C" int convert_unit_double(char *buffer, const char *unit,
                                   double *data)
{
    char *from = strchr(buffer, ' ');
    *data = atof(buffer);

    if (from == nullptr)
        return 1; /* no conversion needed */

    /* skip white space in from of unit */
    while (isspace(*from))
        from++;

    return unit_convert((const char *)from, (const char *)unit, data);
}

/** Convert a struct object to a string
        The structure is defined as a linked list of PROPERTY entities
        @return length of string on success, 0 for empty, <0 for failure
 **/
int convert_from_struct(char *buffer, size_t len, void *data, PROPERTY *prop)
{
    int pos = sprintf(buffer, "%s", "{ ");
    while (prop != nullptr)
    {
        void *addr = (char *)data + (size_t)prop->addr;
        PROPERTYSPEC *spec = property_getspec(prop->ptype);
        char temp[1025];
        size_t n = spec->data_to_string(temp, sizeof(temp), addr, prop);
        if (pos + n >= len - 2)
            return -pos;
        pos += sprintf(buffer + pos, "%s %s; ", prop->name, temp);
        prop = prop->next;
    }
    strcpy(buffer + pos, "}");
    return pos + 1;
}
/** Convert a string to a struct object
        The structure is defined as a linked list of PROPERTY entities
        @return length of string on success, 0 for empty, -1 for failure
 **/
int convert_to_struct(const char *buffer, void *data, PROPERTY *structure)
{
    int len = 0;
    char temp[1025];
    if (buffer[0] != '{')
        return -1;
    strncpy(temp, buffer + 1, sizeof(temp));
    char *item = nullptr;
    char *last = nullptr;
    while ((item = strtok_s(item ? nullptr : temp, ";", &last)) != nullptr)
    {
        char name[64], value[1024];
        while (isspace(*item))
            item++;
        if (*item == '}')
            return len;
        if (sscanf(item, "%s %[^\n]", name, value) != 2)
            return -len;
        PROPERTY *prop;
        for (prop = structure; prop != nullptr; prop = prop->next)
        {
            if (strcmp(prop->name, name) == 0)
            {
                void *addr = (char *)data + (size_t)prop->addr;
                PROPERTYSPEC *spec = property_getspec(prop->ptype);
                len += spec->string_to_data(value, addr, prop);
                break;
            }
        }
        if (prop == nullptr)
            return -len;
    }
    return -len;
}

int convert_from_method(
    char *buffer,   /**< a pointer to the string buffer */
    int size,       /**< the size of the string buffer */
    void *data,     /**< a pointer to the data that is not changed */
    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
    if (buffer == nullptr)
    {
        output_error("gldcore/convert_from_method(): buffer is null");
        return -1;
    }
    if (data == nullptr)
    {
        output_error("gldcore/convert_from_method(): data is null");
        return -1;
    }
    if (prop == nullptr)
    {
        output_error("gldcore/convert_from_method(): prop is null");
        return -1;
    }
    if (prop->method == nullptr)
    {
        output_error("gldcore/convert_from_method(prop='%s'): method is null",
                     prop->name ? prop->name : "(anon)");
        return -1;
    }
    return (prop->method)((OBJECT *)data, buffer, size);
}
int convert_to_method(
    const char *buffer, /**< a pointer to the string buffer that is ignored */
    void *data,         /**< a pointer to the data that is not changed */
    PROPERTY *prop)     /**< a pointer to keywords that are supported */
{
    if (buffer == nullptr)
    {
        output_error("gldcore/convert_to_method(): buffer is null");
        return -1;
    }
    if (data == nullptr)
    {
        output_error("gldcore/convert_to_method(): data is null");
        return -1;
    }
    if (prop == nullptr)
    {
        output_error("gldcore/convert_to_method(): prop is null");
        return -1;
    }
    if (prop->method == nullptr)
    {
        output_error("gldcore/convert_to_method(prop='%s'): method is null",
                     prop->name ? prop->name : "(anon)");
        return -1;
    }
    void *ptr = (void *)buffer; // force to non-const (trust me)
    return (prop->method)((OBJECT *)data, (char *)ptr, 0);
}

/**@}**/
