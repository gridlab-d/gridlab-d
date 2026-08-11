/** $Id: assert.cpp 4738 2014-07-03 00:55:39Z dchassin $

   General purpose assert objects

 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <gld_complex.h>
#include <stdexcept>

#include "gld_assert.h"

EXPORT_CREATE_C(assert, g_assert);
EXPORT_INIT_C(assert, g_assert);
EXPORT_COMMIT_C(assert, g_assert);

CLASS *g_assert::oclass = nullptr;
// g_assert *g_assert::defaults = nullptr;
static g_assert defaults_storage; // Static storage for default values
g_assert *g_assert::defaults = &defaults_storage;

g_assert::g_assert(MODULE *module)
{

    if (oclass == nullptr)
    {
        // register to receive notice for first top down. bottom up, and second top
        // down synchronizations
        oclass = gld_class::create(module, "assert", sizeof(g_assert),
                                   PC_AUTOLOCK | PC_OBSERVER);
        if (oclass == nullptr)
            throw std::runtime_error("unable to register class assert");
        else
            oclass->trl = TRL_PROVEN;

        // defaults = this;
        if (gl_publish_variable(
                oclass,
                PT_enumeration, "status", get_status_offset(), PT_DESCRIPTION, "desired outcome of assert test",
                PT_KEYWORD, "INIT", (enumeration)AS_INIT,
                PT_KEYWORD, "TRUE", (enumeration)AS_TRUE,
                PT_KEYWORD, "FALSE", (enumeration)AS_FALSE,
                PT_KEYWORD, "NONE", (enumeration)AS_NONE,
                PT_char1024, "target", get_target_offset(), PT_DESCRIPTION, "the target property to test",
                PT_char32, "part", get_part_offset(), PT_DESCRIPTION, "the target property part to test",
                PT_enumeration, "relation", get_relation_offset(), PT_DESCRIPTION, "the relation to use for the test",
                PT_KEYWORD, "==", (enumeration)TCOP_EQ,
                PT_KEYWORD, "<", (enumeration)TCOP_LT,
                PT_KEYWORD, "<=", (enumeration)TCOP_LE,
                PT_KEYWORD, ">", (enumeration)TCOP_GT,
                PT_KEYWORD, ">=", (enumeration)TCOP_GE,
                PT_KEYWORD, "!=", (enumeration)TCOP_NE,
                PT_KEYWORD, "inside", (enumeration)TCOP_IN,
                PT_KEYWORD, "outside", (enumeration)TCOP_NI,
                PT_char1024, "value", get_value_offset(), PT_DESCRIPTION, "the value to compare with for binary tests",
                PT_char1024, "within", get_value2_offset(), PT_DESCRIPTION, "the bounds within which the value must bed compared",
                PT_char1024, "lower", get_value_offset(), PT_DESCRIPTION, "the lower bound to compare with for interval tests",
                PT_char1024, "upper", get_value2_offset(), PT_DESCRIPTION, "the upper bound to compare with for interval tests",
                nullptr) < 1)
        {
            char msg[256];
            sprintf(msg, "unable to publish properties in %s", __FILE__);
            throw std::runtime_error(msg);
        }

        // memset(this,0,sizeof(g_assert));
        status = AS_INIT;
        relation = TCOP_EQ;
        target[0] = '\0';
        part[0] = '\0';
        value[0] = '\0';
        value2[0] = '\0';
    }
}

int g_assert::create(void)
{
    // memcpy(this, defaults, sizeof(*this));
    status = defaults->status;
    relation = defaults->relation;
    strcpy(target, defaults->target);
    strcpy(part, defaults->part);
    strcpy(value, defaults->value);
    strcpy(value2, defaults->value2);

    return 1; /* return 1 on success, 0 on failure */
}

int g_assert::init(OBJECT *parent)
{
    gld_property target(get_parent(), get_target().c_str());
    if (!target.is_valid())
        exception("target '%s' property '%s' does not exist",
                  get_parent() ? get_parent()->get_name() : "global",
                  get_target().c_str());

    set_status(AS_TRUE);
    return 1;
}

TIMESTAMP g_assert::commit(TIMESTAMP t1, TIMESTAMP t2)
{
    // get the target property
    gld_property target_prop(get_parent(), get_target().c_str());
    if (!target_prop.is_valid())
    {
        gl_error("%s: target %s is not valid", get_target().c_str(), get_name());
        /*  TROUBLESHOOT
        Check to make sure the target you are specifying is a published variable for
        the object that you are pointing to.  Refer to the documentation of the
        command flag --modhelp, or check the wiki page to determine which variables
        can be published within the object you are pointing to with the assert
        function.
        */
        return 0;
    }

    // determine the relation status
    if (status == AS_NONE)
    {
        gl_verbose("%s: test is not being run on %s", get_name(),
                   get_parent()->get_name());
        return TS_NEVER;
    }
    else
    {
        if (evaluate_status() != get_status())
        {
            gld_property relation_prop(my(), "relation");
            // gld_keyword *pKeyword = relation_prop.find_keyword(relation);

            const char *rel_name = "(unknown)";
            if (gld_keyword *pKeyword = relation_prop.find_keyword(relation))
            {
                rel_name = pKeyword->get_name();
            }

            char buf[1024];
            // char *p = get_part();
            gl_error("%s: assert failed on %s %s.%s.%s %s %s %s %s", get_name(),
                     status == AS_TRUE ? "" : "NOT",
                     get_parent() ? get_parent()->get_name() : "global variable",
                     get_target().c_str(), get_part().c_str(),
                     target_prop.to_string(buf, sizeof(buf)) ? buf : "(void)",
                     rel_name, // pKeyword->get_name(),
                     get_value().c_str(), get_value2().c_str());
            return 0;
        }
        else
        {
            gl_verbose("%s: assert passed on %s", get_name(),
                       get_parent() ? get_parent()->get_name() : "global variable");
            return TS_NEVER;
        }
    }
    // should never get here
}

g_assert::ASSERTSTATUS g_assert::evaluate_status(void)
{
    gld_property target_prop(get_parent(), get_target().c_str());
    if (get_part() == "")
        return target_prop.compare(relation, get_value().c_str(),
                                   get_value2().c_str())
                   ? AS_TRUE
                   : AS_FALSE;
    else
        return target_prop.compare(relation, get_value().c_str(),
                                   get_value2().c_str(), get_part().c_str())
                   ? AS_TRUE
                   : AS_FALSE;
}
