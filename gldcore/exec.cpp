/** $Id: exec.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file exec.c
	@addtogroup exec Main execution loop
	@ingroup core
	
	The main execution loop sets up the main simulation, initializes the
	objects, and runs the simulation until it either settles to equilibrium
	or runs into a problem.  It also takes care multicore/multiprocessor
	parallelism when possible.  Objects of the same rank will be synchronized
	simultaneously, resources permitting.

	The main processing loop calls each object passing to it a TIMESTAMP
	indicating the desired synchronization time.  The sync() call attempts to
	advance the object's internal clock to the time indicated, and if successful it
	returns the time of the next expected change in the object's state.  An
	object state change is one which requires the equilibrium equations of
	the object to be updated.  When an object's state changes, all the other
	objects in the simulator are given an opportunity to consider the change
	and possibly alter the time of their next state change.  The core
	continues calling objects, advancing the global clock when
	necessary, and continuing in this way until all objects indicate that
	no further state changes are expected.  This is the equilibrium condition
	and the simulation consequently ends.

	\section exec_sync Sync Event API

	 Sync handling is done using an API implemented in #core_exec
	 There are several types of sync events that are handled.
	 - Hard sync is a sync time that should always be considered
	 - Soft sync is a sync time should only be considered when
	   the hard sync is not TS_NEVER
	 - Status is SUCCESS if the sync time is valid

	 Sync logic table:
@verbatim
 hard t
 
  sync_to    | t==INVALID  | t<sync_to   | t==sync_to  | sync_to<t<NEVER | t==NEVER
  ---------- | ----------- | ----------- | ----------- | --------------- | ----------
  INVALID    | INVALID     | INVALID     | INVALID     | INVALID         | INVALID
  soft       | INVALID     | t           | t           | sync_to         | sync_to
  hard       | INVALID     | t           | sync_to     | sync_to         | sync_to
  NEVER      | INVALID     | t           | sync_to     | sync_to         | NEVER
@endverbatim

@verbatim
 soft t
 
  sync_to    | t==INVALID  | t<sync_to   | t==sync_to  | sync_to<t<NEVER | t==NEVER
  ---------- | ----------- | ----------- | ----------- | --------------- | ----------
  INVALID    | INVALID     | INVALID     | INVALID     | INVALID         | INVALID
  soft       | INVALID     | t           | t           | sync_to         | sync_to
  hard       | INVALID     | t*          | t*          | sync_to         | sync_to
  NEVER      | INVALID     | t           | sync_to     | sync_to         | NEVER
 
 * indicates soft event is made hard
@endverbatim

    The Sync Event API functions are as follows:
	- #exec_sync_reset is used to reset a sync event to initial steady state sync (NEVER)
	- #exec_sync_merge is used to update an existing sync event with a new sync event
	- #exec_sync_set is used to set a sync event
	- #exec_sync_get is used to get a sync event
	- #exec_sync_getevents is used to get the number of hard events in a sync event
	- #exec_sync_getstatus is used to get the status of a sync event
	- #exec_sync_ishard is used to determine whether a sync event is a hard event
	- #exec_sync_isinvalid is used to determine whether a sync event is valid
	- #exec_sync_isnever is used to determine whether a sync event is pending

	@future [Chassin Oct'07]

	There is some value in exploring whether it is necessary to update all
	objects when a particular objects implements a state change.  The idea is
	based on the fact that updates propagate through the model based on known
	relations, such at the parent-child relation or the link-node relation.
	Consequently, it should obvious that unless a value in a related object
	has changed, there can be no significant change to an object that hasn't reached
	it's declared update time.  Thus only the object that "won" the next update
	time and those that are immediately related to it need be updated.  This 
	change could result in a very significant improvement in performance,
	particularly in models with many lightly coupled objects. 

 @{
 **/

#include <algorithm> // Add this include for std::ranges
#include <cctype>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <ranges>
#include <set>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <winbase.h>
#include <winsock2.h>
#else
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <list>
#include <netinet/in.h>
#include <ratio>
#include <set>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

#include "class.h"
#include "convert.h"
#include "debug.h"
#include "deltamode.h"
#include "enduse.h"
#include "exception.h"
#include "exec.h"
#include "gldrandom.h"
#include "globals.h"
#include "index.h"
#include "instance.h"
#include "link.h"
#include "linkage.h"
#include "loadshape.h"
#include "local.h"
#include "lock.h"
#include "module.h"
#include "object.h"
#include "output.h"
#include "platform.h"
#include "realtime.h"
#include "save.h"
#include "schedule.h"
#include "stream.h"
#include "test.h"
#include "transform.h"
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "cpp_threadpool.h"

using namespace std::literals;

#ifdef _WIN32
#define WEXITSTATUS(X) (X & 127)
#endif

// Only setup threadpool for each object rank list at the first iteration;
cpp_threadpool *threadpool;


/** Set/get exit code **/
int exec_setexitcode(int xc)
{
    int oldxc = global_exit_code;
    if (oldxc != XC_SUCCESS)
        output_warning("new exitcode %d overwrites existing exitcode %d", xc,
                       oldxc);
    global_exit_code = xc;
    output_debug("exit code %d", xc);
    return oldxc;
}
int exec_getexitcode() { return global_exit_code; }

const char *exec_getexitcodestr(EXITCODE xc)
{
    switch (xc)
    {
    case XC_SUCCESS: /* per system(3) */
        return "ok";
    case XC_EXFAILED: /* exec/wait failure - per system(3) */
        return "exec/wait failed";
    case XC_ARGERR: /* error processing command line arguments */
        return "bad command";
    case XC_ENVERR: /* bad environment startup */
        return "environment startup/load failed";
    case XC_TSTERR: /* test failed */
        return "test failed";
    case XC_USRERR: /* user reject terms of use */
        return "user rejected license terms";
    case XC_RUNERR: /* simulation did not complete as desired */
        return "simulation failed";
    case XC_INIERR: /* initialization failed */
        return "initialization failed";
    case XC_PRCERR: /* process control error */
        return "process control error";
    case XC_SVRKLL: /* server killed */
        return "server killed";
    case XC_IOERR: /* I/O error */
        return "I/O error";
    case XC_SHFAILED: /* shell failure - per system(3) */
        return "shell failed";
    case XC_SIGNAL: /* signal caught - must be or'd with SIG value if known */
        return "signal caught";
    case XC_SIGINT: /* SIGINT caught */
        return "interrupt received";
    case XC_EXCEPTION: /* exception caught */
        return "exception caught";
    default:
        return "unknown exception";
    }
}

/** Elapsed wallclock **/
int64 exec_clock()
{
    using std::chrono::system_clock;
    static bool initialized = false;
    static std::chrono::time_point<system_clock> nt1;
    static std::chrono::time_point<system_clock> nt2;
    if (!initialized)
    { // [[unlikely]] {
        nt1 = system_clock::now();
        nt2 = nt1;
        initialized = true;
    }
    else
    { // [[likely]] {
        nt2 = system_clock::now();
    }
    return std::chrono::duration_cast<std::chrono::microseconds>(nt2 - nt1)
        .count();
}

/** The main system initialization sequence
        @return 1 on success, 0 on failure
 **/
int exec_init()
{
#if 0
#ifdef _WIN32
	char glpathvar[1024];
#endif
#endif
    size_t glpathlen = 0;

    /* set thread count equal to processor count if not passed on command-line */
    if (global_threadcount == 0)
        global_threadcount = processor_count();
    output_verbose("detected %d processor(s)", processor_count());
    output_verbose("using %d helper thread(s)", global_threadcount);

    /* setup clocks */
    if (global_starttime == 0)
        global_starttime = realtime_now();

    /* set the start time */
    // global_clock = global_starttime + local_tzoffset(global_starttime);
    global_clock = global_starttime;

    /* save locale for simulation */
    locale_push();

#if 0 /* isn't cooperating for strange reasons -mh */
#ifdef _WIN32
	glpathlen=strlen("GLPATH=");
	sprintf(glpathvar, "GLPATH=");
	ExpandEnvironmentStrings(getenv("GLPATH"), glpathvar+glpathlen, (DWORD)(1024-glpathlen));
#endif
#endif

    if (global_init() == FAILED)
        return 0;
    return 1;
}

clock_t cstart, clock_end;

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif

#define PASSINIT(p) (p % 2 ? ranks[p]->first_used : ranks[p]->last_used)
#define PASSCMP(i, p) \
    (p % 2 ? i <= ranks[p]->last_used : i >= ranks[p]->first_used)
#define PASSINC(p) (p % 2 ? 1 : -1)

// static struct thread_data *thread_data = nullptr;
static std::shared_ptr<struct thread_data> thread_data = nullptr;

static threadpool_thread_data *threadpool_data = nullptr;
static INDEX **ranks = nullptr;
const PASSCONFIG passtype[] = {PC_PRETOPDOWN, PC_BOTTOMUP, PC_POSTTOPDOWN};
static unsigned int pass;
int iteration_counter = 0;            /* number of redos completed */
int federation_iteration_counter = 0; /* number of federate redos completed */

#ifndef NOLOCKS
int64 rlock_count = 0, rlock_spin = 0;
int64 wlock_count = 0, wlock_spin = 0;
#endif

extern unsigned int mls_inst_lock;
extern std::condition_variable_any mls_inst_signal;

// struct for thread_create arguments
struct arg_data
{
    int thread;
    void *item;
    int incr;
};
struct arg_data arg_data_array[2];

INDEX **exec_getranks() { return ranks; }

static STATUS setup_ranks()
{
    OBJECT *obj;
    int i;
    static INDEX *passlist[] = {
        nullptr, nullptr, nullptr,
        nullptr}; /* extra nullptr marks the end of the list */

    /* create index object */
    ranks = passlist;
    ranks[0] = index_create(0, 10);
    ranks[1] = index_create(0, 10);
    ranks[2] = index_create(0, 10);

    /* build the ranks of each pass type */
    for (i = 0; i < sizeof(passtype) / sizeof(passtype[0]); i++)
    {
        if (ranks[i] == nullptr)
            return FAILED;

        /* add every object to index based on rank */
        for (obj = object_get_first(); obj != nullptr; obj = object_get_next(obj))
        {
            /* ignore objects that don't use this passconfig */
            if ((obj->oclass->passconfig & passtype[i]) == 0)
                continue;

            /* add this object to the ranks for this passconfig */
            if (index_insert(ranks[i], obj, obj->rank) == FAILED)
                return FAILED;
            // sjin: print out obj id, pass, rank information
            // else
            //	printf("obj[%d]: pass = %d, rank = %d\n", obj->id, passtype[i],
            // obj->rank);
        }

        if (global_debug_mode == 0 && global_nolocks == 0)

            /* shuffle the objects in the index */
            index_shuffle(ranks[i]);
    }

    return SUCCESS;
}

const char *simtime()
{
    static char buffer[64];
    return convert_from_timestamp(global_clock, buffer, sizeof(buffer)) > 0
               ? buffer
               : "(invalid)";
}

static STATUS show_progress()
{
    extern GUIACTIONSTATUS wait_status;
    output_progress();
    /* reschedule report */
    realtime_schedule_event(realtime_now() + 1, show_progress);
    return SUCCESS;
}

/***********************************************************************/
/* CHECKPOINTS */

// Helper function to check if directory exists
static bool directory_exists(const char *path)
{
    if (!path || strlen(path) == 0)
        return false;

    struct stat info;
    if (stat(path, &info) != 0)
    {
        return false; // Path doesn't exist or can't be accessed
    }
    return (info.st_mode & S_IFDIR) != 0; // Check if it's a directory
}

// Do Checkpoint
/**
 * Creates a JSON checkpoint of the current simulation state.
 * 
 * @param output_directory Directory path for writing checkpoint files. 
 *                        If nullptr or empty, generates JSON in-memory without writing files.
 * @return nlohmann::ordered_json containing the checkpoint data
 */
nlohmann::ordered_json do_checkpoint(const char *output_directory)
{
    /* last checkpoint value */
    static TIMESTAMP last_checkpoint = 0;
    TIMESTAMP now = 0;
    /* check point type selection */
    switch (global_checkpoint_type)
    {
    /* wallclock checkpoint interval */
    case CPT_WALL:
        /* checkpoint based on wall time */
        now = time(nullptr);
        /* default checkpoint for WALL */
        if (global_checkpoint_interval == 0)
            global_checkpoint_interval = 3600;
        break;

    /* simulation checkpoint interval */
    case CPT_SIM:
        /* checkpoint based on sim time */
        now = global_clock;
        /* default checkpoint for SIM */
        if (global_checkpoint_interval == 0)
            global_checkpoint_interval = 86400;
        break;

    /* no checkpoints used */
    case CPT_NONE:
        now = 0;
        break;
    }

	nlohmann::ordered_json checkpoint;
	/* checkpoint may be needed */
	if (now > 0)
	{

		// TODO: Take a look at global_checkpoint_interval and if we want to keep it
		/* checkpoint time lapsed */
		// if ( last_checkpoint + global_checkpoint_interval <= now )
		if (last_checkpoint <= now)
		{
			static char json_fn[1024] = "";

			// Strip extension from global_modelname if present
			char modelname_noext[1024];
			strncpy(modelname_noext, global_modelname, sizeof(modelname_noext));
			modelname_noext[sizeof(modelname_noext)-1] = '\0';
			char *last_dot = strrchr(modelname_noext, '.');
			if (last_dot) {
				*last_dot = '\0';
			}

			/* create current checkpoint save filename */
			sprintf(json_fn, "%s_%s", modelname_noext, "checkpoint.json");

			const char *json_dir = (output_directory && strlen(output_directory) > 0) ? output_directory : ".";
			/* check if output directory exists */
			if (!directory_exists(json_dir))
			{
				output_error("directory '%s' does not exist for JSON checkpoint files", json_dir);
				return nlohmann::ordered_json(); // Return empty JSON value on error
			}

			/* initial value of last checkpoint */
			if (last_checkpoint == 0)
				last_checkpoint = now;
			/* Write JSON data using JsonCpp */
			// Create JSON structure using nlohmann::json
			// Add preamble (ensure comments is an array)
			if (!checkpoint.contains("__preamble"))
				checkpoint["__preamble"] = nlohmann::ordered_json::object();
			if (!checkpoint["__preamble"].contains("comments") || !checkpoint["__preamble"]["comments"].is_array())
				checkpoint["__preamble"]["comments"] = nlohmann::ordered_json::array();
			checkpoint["__preamble"]["comments"].push_back("// GridLAB-D checkpoint data export");
			{
				std::time_t sys_now = std::time(nullptr);
				struct tm *tm_local = std::localtime(&sys_now);
				char sys_time_buf[64] = "";
				std::strftime(sys_time_buf, sizeof(sys_time_buf), "%Y-%m-%d %H:%M:%S %Z", tm_local);
				std::string timestamp_comment = std::string("// Generated at: ") + sys_time_buf;
				checkpoint["__preamble"]["comments"].push_back(timestamp_comment);
			}

			// Add clock info (format timestamps as strings with timezone)
			char ts_buffer[64];
			std::string tz = timestamp_current_timezone();
			size_t first_digit = tz.find_first_of("0123456789");
			if (first_digit != std::string::npos && (first_digit == 0 || (tz[first_digit - 1] != '+' && tz[first_digit - 1] != '-')))
			{
				tz.insert(first_digit, "+");
			}
			
			convert_from_timestamp(global_clock, ts_buffer, sizeof(ts_buffer));
			checkpoint["clock"]["timestamp"] = "'" + std::string(ts_buffer) + "'";
			
			convert_from_timestamp(global_stoptime, ts_buffer, sizeof(ts_buffer));
			checkpoint["clock"]["stoptime"] = "'" + std::string(ts_buffer) + "'";
			
			convert_from_timestamp(global_starttime, ts_buffer, sizeof(ts_buffer));
			checkpoint["clock"]["starttime"] = "'" + std::string(ts_buffer) + "'";
			
			checkpoint["clock"]["timezone"] = tz;
			checkpoint["_checkpoint"] = true;

			// First, collect objects by class name
			std::map<std::string, std::vector<OBJECT *>> objects_by_class;
			std::set<OBJECT *> processed_objects; // Track processed objects to prevent duplicates

			// Helper function to parse property value string into JSON based on type
			auto parse_property_value = [](PROPERTYTYPE ptype, const char *value_str) -> nlohmann::json {
				switch (ptype)
				{
				case PT_double:
				{
					double val = strtod(value_str, nullptr);
					return val;
				}
				case PT_int32:
				{
					int32 val = (int32)strtol(value_str, nullptr, 10);
					return val;
				}
				case PT_int64:
				{
					int64 val = strtoll(value_str, nullptr, 10);
					return static_cast<int64_t>(val);
				}
				case PT_bool:
				{
					bool val = (strcmp(value_str, "TRUE") == 0 || strcmp(value_str, "1") == 0);
					return val;
				}
				case PT_timestamp:
				{
					TIMESTAMP val = strtoll(value_str, nullptr, 10);
					return static_cast<int64_t>(val);
				}
				case PT_char8:
				case PT_char32:
				case PT_char256:
				case PT_char1024:
				case PT_complex:
					return std::string(value_str);
				default:
					// For all other types, store as string
					return std::string(value_str);
				}
			};				

			/* Traverse all objects to group by class */
			for (int pass = 0; ranks[pass] != nullptr; pass++)
			{
				for (int i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
				{
					if (ranks[pass]->ordinal[i] == nullptr)
						continue;

					for (LISTITEM *ptr = ranks[pass]->ordinal[i]->first; ptr != nullptr; ptr = ptr->next)
					{
						OBJECT *obj = static_cast<OBJECT *>(ptr->data);

						// Skip if we've already processed this object
						if (processed_objects.find(obj) != processed_objects.end())
							continue;

						std::string class_name = obj->oclass->name;
						objects_by_class[class_name].push_back(obj);
						processed_objects.insert(obj); // Mark as processed
					}
				}
			}

			// Build objects section grouped by class
			int classnameCounter = 0;
			for (auto &class_pair : objects_by_class)
			{
				nlohmann::json instances = nlohmann::json::array();

				for (OBJECT *obj : class_pair.second)
				{
					nlohmann::json instance = nlohmann::json::object();

					// Add object name if it exists
					if (obj->name && strlen(obj->name) > 0)
						instance["name"] = obj->name;
					else
					{
						instance["object_declaration"] = std::string(obj->oclass->name) + ":" + std::to_string(static_cast<int>(obj->id));
						classnameCounter++;
					}

					// Add reference to parent object
					if (obj->parent && obj->parent != nullptr){
						if(obj->parent->name && strlen(obj->parent->name) > 0){
							instance["parent"] = obj->parent->name;
						}
						else {
							instance["parent"] = std::string(obj->parent->oclass->name) + ":" + std::to_string(static_cast<int>(obj->parent->id));
						}
					}

					// Write s_object_list (built-in) properties if not null/empty/default

					// groupid
					if (obj->groupid[0] != '\0')
						instance["groupid"] = std::string(obj->groupid);

					// rank
					if (obj->rank != 0)
						instance["rank"] = static_cast<int>(obj->rank);

					// schedule_skew
					if (obj->schedule_skew != 0)
						instance["schedule_skew"] = static_cast<int64_t>(obj->schedule_skew);

					// latitude / longitude
					if (!std::isnan(obj->latitude) && obj->latitude != 0.0)
						instance["latitude"] = obj->latitude;
					if (!std::isnan(obj->longitude) && obj->longitude != 0.0)
						instance["longitude"] = obj->longitude;

					// in_svc / out_svc (as formatted timestamp strings, skip TS_NEVER/0)
					if (obj->in_svc != 0 && obj->in_svc != TS_NEVER)
					{
						char svc_buf[64] = "";
						convert_from_timestamp(obj->in_svc, svc_buf, sizeof(svc_buf));
						instance["in_svc"] = std::string(svc_buf);
					}
					if (obj->out_svc != 0 && obj->out_svc != TS_NEVER)
					{
						char svc_buf[64] = "";
						convert_from_timestamp(obj->out_svc, svc_buf, sizeof(svc_buf));
						instance["out_svc"] = std::string(svc_buf);
					}

					// in_svc_double / out_svc_double
					if (!std::isnan(obj->in_svc_double) && obj->in_svc_double != 0.0 && obj->in_svc_double != TS_NEVER_DBL)
						instance["in_svc_double"] = obj->in_svc_double;
					if (!std::isnan(obj->out_svc_double) && obj->out_svc_double != 0.0 && obj->out_svc_double != TS_NEVER_DBL)
						instance["out_svc_double"] = obj->out_svc_double;

					// rng_state
					if (obj->rng_state != 0)
						instance["rng_state"] = static_cast<uint32_t>(obj->rng_state);

					// heartbeat
					if (obj->heartbeat != 0 && obj->heartbeat != TS_NEVER)
						instance["heartbeat"] = static_cast<int64_t>(obj->heartbeat);

					// flags — only write if deltamode flag is set
					if (obj->flags & OF_DELTAMODE)
						instance["flags"] = static_cast<uint32_t>(obj->flags);

					// Add all properties from this object's class and all parent classes
					std::set<std::string> processed_properties; // Track processed properties to avoid duplicates

					// Traverse the entire class hierarchy (class and all parents)
					CLASS *current_class = obj->oclass;
					while (current_class != nullptr)
					{
						PROPERTY *pmap = current_class->pmap;
						for (; pmap != nullptr; pmap = pmap->next)
						{
							// Skip if this is the name property and we already added it
							if (strcmp(pmap->name, "name") == 0)
								continue;

							// Skip if we've already processed this property (from a derived class)
							std::string prop_name(pmap->name);
							if (processed_properties.find(prop_name) != processed_properties.end())
								continue;

							// Mark this property as processed
							processed_properties.insert(prop_name);

							/* Get property value based on type */
							switch (pmap->ptype)
							{
							case PT_double:
							{
								double *dptr = object_get_double_quick(obj, pmap);
								if (dptr != nullptr)
								{
									double val = *dptr;
									// Skip NaN and subnormal values - subnormals (< DBL_MIN)
									// are almost always uninitialized memory, not valid data.
									if (!std::isnan(val) && std::fpclassify(val) != FP_SUBNORMAL)
										instance[pmap->name] = val;
								}
								break;
							}
							case PT_int32:
							{
								int32 *iptr = object_get_int32(obj, pmap);
								if (iptr != nullptr)
									instance[pmap->name] = *iptr;
								break;
							}
							case PT_int64:
							{
								int64 *iptr = object_get_int64(obj, pmap);
								if (iptr != nullptr)
									instance[pmap->name] = static_cast<int64_t>(*iptr);
								break;
							}
							case PT_bool:
							{
								bool *bptr = object_get_bool(obj, pmap);
								if (bptr != nullptr)
									instance[pmap->name] = *bptr;
								break;
							}
							case PT_timestamp:
							{
								int64 *tptr = object_get_int64(obj, pmap);
								if (tptr != nullptr)
									instance[pmap->name] = static_cast<int64_t>(*tptr);
								break;
							}
							default:
							{
								// For string and all other types, use object_get_value_by_name
								char value_str[1024] = "";
								if (object_get_value_by_name(obj, pmap->name, value_str, sizeof(value_str)) > 0
								    && strlen(value_str) > 0
								    && strcmp(value_str, "null") != 0 && strcmp(value_str, "NULL") != 0
								    && strcmp(value_str, "\"\"") != 0 && strcmp(value_str, "''") != 0
								    && strcmp(value_str, "NAN") != 0 && strcmp(value_str, "nan") != 0)
								{
									instance[pmap->name] = std::string(value_str);
								}
								break;
							}
							}
							// Skip properties that couldn't be retrieved - don't add null values
						}

						// Move to parent class
						current_class = current_class->parent;
					}

					instances.push_back(instance);
				}

				checkpoint["objects"][class_pair.first]["instances"] = instances;
			}

			// Get modules data
			nlohmann::ordered_json modules = nlohmann::ordered_json::object();
			std::map<std::string, MODULE *> module_map;
			
			for (MODULE *mod = module_get_first(); mod != nullptr; mod = mod->next)
			{
				nlohmann::ordered_json module = nlohmann::ordered_json::object();
				
			// Iterate through module's own property list
			if (mod->globals != nullptr)
			{
				char value_buffer[1024];
				for (PROPERTY *prop = mod->globals; prop != nullptr; prop = prop->next)
				{
					// Get the value using module_getvar
					if (module_getvar(mod, prop->name, value_buffer, sizeof(value_buffer)) != nullptr)
					{
						// Skip if value is empty, null, or invalid
						if (value_buffer == nullptr || strlen(value_buffer) == 0 ||
						    strcmp(value_buffer, "null") == 0 || strcmp(value_buffer, "NULL") == 0 ||
						    strcmp(value_buffer, "\"\"") == 0 || strcmp(value_buffer, "''") == 0 ||
						    strcmp(value_buffer, "NAN") == 0 || strcmp(value_buffer, "nan") == 0)
						{
							continue;
						}
						// Parse the value based on property type
						module[prop->name] = parse_property_value(prop->ptype, value_buffer);
					}
				}
			}					modules[mod->name] = module;
				module_map[mod->name] = mod;
			}

			// Get globals data and assign to modules
			nlohmann::ordered_json globals = nlohmann::ordered_json::object();
			GLOBALVAR *global = nullptr;
			char buffer[1024];
			
			// Iterate through all global variables
			while ((global = global_getnext(global)) != nullptr)
			{
				// Get the global variable value
				if (global_getvar(global->prop->name, buffer, sizeof(buffer)))
				{
					std::string global_name(global->prop->name);
					
					// Check if this global belongs to a module (contains "::")
					size_t separator_pos = global_name.find("::");
					if (separator_pos != std::string::npos)
					{
						// Extract module name and variable name
						std::string module_name = global_name.substr(0, separator_pos);
						std::string var_name = global_name.substr(separator_pos + 2);
						
						// Check if the module exists
						if (module_map.find(module_name) != module_map.end())
						{
							// Skip if value is empty, null, or invalid
							if (buffer != nullptr && strlen(buffer) > 0 &&
							    strcmp(buffer, "null") != 0 && strcmp(buffer, "NULL") != 0 &&
							    strcmp(buffer, "\"\"") != 0 && strcmp(buffer, "''") != 0 &&
							    strcmp(buffer, "NAN") != 0 && strcmp(buffer, "nan") != 0)
							{
								// Assign global to the module with proper type parsing
								modules[module_name][var_name] = parse_property_value(global->prop->ptype, buffer);
							}
						}
						else
						{
							// Module not found, issue a warning
							output_warning("Global variable '%s' references module '%s' which is not loaded", 
								global_name.c_str(), module_name.c_str());
							// Still add to core globals as fallback if value is valid
							if (buffer != nullptr && strlen(buffer) > 0 &&
							    strcmp(buffer, "null") != 0 && strcmp(buffer, "NULL") != 0 &&
							    strcmp(buffer, "\"\"") != 0 && strcmp(buffer, "''") != 0 &&
							    strcmp(buffer, "NAN") != 0 && strcmp(buffer, "nan") != 0)
							{
								globals[global_name] = parse_property_value(global->prop->ptype, buffer);
							}
						}
					}
					else
					{
						// Core global variable (no module prefix)
						if (buffer != nullptr && strlen(buffer) > 0 &&
						    strcmp(buffer, "null") != 0 && strcmp(buffer, "NULL") != 0 &&
						    strcmp(buffer, "\"\"") != 0 && strcmp(buffer, "''") != 0 &&
						    strcmp(buffer, "NAN") != 0 && strcmp(buffer, "nan") != 0)
						{
							globals[global_name] = parse_property_value(global->prop->ptype, buffer);
						}
					}
				}
			}
			// Assign globals and modules to checkpoint
			checkpoint["globals"] = globals;
			checkpoint["modules"] = modules;	

			// Write JSON to file with pretty formatting
			std::ofstream json_file(json_fn);
			if (json_file.is_open())
			{
				// pretty print with 2-space indentation
				std::string out = checkpoint.dump(2);
				json_file << out;
				json_file.close();
				output_verbose("JSON checkpoint written to '%s'", json_fn);
			}
			else
			{
				output_error("unable to open JSON checkpoint file '%s' for writing", json_fn);
			}

			return checkpoint;	
		}
	}
	return checkpoint;
}
/***********************************************************************/

threadpool_thread_data::threadpool_thread_data(int size)
{
    //	data = std::vector<struct sync_data>(size);
    data = new struct sync_data[size];
    count = size;
    thread_map = threadpool->get_threadmap();
    for (int index = 0; index < size; index++)
        data[index].status = SUCCESS;
}

clock_t objs_synctime = 0;
struct sync_data *
threadpool_thread_data::get_thread_data(std::thread::id thread_id)
{
    return &data[thread_map.at(thread_id)];
}

struct sync_data *threadpool_thread_data::get_data(int index)
{
    return &data[index];
};

static void tp_do_object_sync(OBJECT *obj)
{
    std::thread::id thread_id = std::this_thread::get_id();
    struct sync_data *data = threadpool_data->get_thread_data(thread_id);
    TIMESTAMP this_t;
    char b[64];

    /* check in and out-of-service dates */
    if (global_clock < obj->in_svc)
        this_t = obj->in_svc; /* yet to go in service */
    else if ((global_clock == obj->in_svc) &&
             (obj->in_svc_micro !=
              0))                 /* If our in service is a little higher, delay to next time */
        this_t = obj->in_svc + 1; /* Technically yet to go into service -- deltamode
                                     handled separately */
    else if (global_clock <= obj->out_svc)
    {
        this_t = object_sync(obj, global_clock, passtype[pass]);
        if (this_t == global_clock)
        {
            output_verbose("%s: object %s calling for re-sync", simtime(),
                           object_name(obj, b, 63));
        }
    }
    else
        this_t = TS_NEVER; /* already out of service */

    /* check for "soft" event (events that are ignored when stopping) */
    if (this_t < -1)
        this_t = -this_t;
    else if (this_t != TS_NEVER)
        data->hard_event++; /* this counts the number of hard events */

    /* check for stopped clock */
    if (this_t < global_clock)
    {
        char b[64];
        output_error("%s: object %s stopped its clock (exec)!", simtime(),
                     object_name(obj, b, 63));
        /* TROUBLESHOOT
                This indicates that one of the objects in the simulator has
           encountered a state where it cannot calculate the time to the next state.
           This usually is caused by a bug in the module that implements that
           object's class.
         */
        data->status = FAILED;
    }
    else
    {
        /* check for iteration limit approach */
        if (iteration_counter == 2 && this_t == global_clock)
        {
            char b[64];
            output_verbose("%s: object %s iteration limit imminent", simtime(),
                           object_name(obj, b, 63));
        }
        else if (iteration_counter == 1 && this_t == global_clock)
        {
            output_error("convergence iteration limit reached for object %s:%d",
                         obj->oclass->name, obj->id);
            /* TROUBLESHOOT
                    This indicates that the core's solver was unable to determine
                    a steady state for all objects for any time horizon.  Identify
                    the object that is causing the convergence problem and contact
                    the developer of the module that implements that object's class.
             */
        }

        /* manage minimum timestep */
        if (global_minimum_timestep > 1 && this_t > global_clock &&
            this_t < TS_NEVER)
            this_t = (((this_t - 1) / global_minimum_timestep) + 1) *
                     global_minimum_timestep;

        /* if this event precedes next step, next step is now this event */
        if (data->step_to > this_t)
        {
            // LOCK(data);
            data->step_to = this_t;
            // UNLOCK(data);
        }
        // printf("data->step_to=%d, this_t=%d\n", data->step_to, this_t);
    }
}

static void ss_do_object_sync(int thread, void *item)
{
    // struct sync_data *data = &thread_data->data[thread];
    std::shared_ptr<struct sync_data> data = thread_data->data[thread];
    OBJECT *obj = (OBJECT *)item;
    TIMESTAMP this_t;
    char b[64];

    // printf("thread %d\t%d\t%s\n", thread, obj->rank, obj->name);
    // this_t = object_sync(obj, global_clock, passtype[pass]);

    /* check in and out-of-service dates */
    if (global_clock < obj->in_svc)
        this_t = obj->in_svc; /* yet to go in service */
    else if ((global_clock == obj->in_svc) &&
             (obj->in_svc_micro !=
              0))                 /* If our in service is a little higher, delay to next time */
        this_t = obj->in_svc + 1; /* Technically yet to go into service -- deltamode
                                     handled separately */
    else if (global_clock <= obj->out_svc)
    {
        this_t = object_sync(obj, global_clock, passtype[pass]);
        if (this_t == global_clock)
        {
            output_verbose("%s: object %s calling for re-sync", simtime(),
                           object_name(obj, b, 63));
        }

#ifdef _DEBUG
        /* sync dumpfile */
        if (global_sync_dumpfile[0] != '\0')
        {
            static FILE *fp = nullptr;
            if (fp == nullptr)
            {
                static int tried = 0;
                if (!tried)
                {
                    fp = fopen(global_sync_dumpfile, "wt");
                    if (fp == nullptr)
                        output_error("sync_dumpfile '%s' is not writeable",
                                     global_sync_dumpfile);
                    else
                        fprintf(fp, "timestamp,pass,iteration,thread,object,sync\n");
                    tried = 1;
                }
            }
            if (fp != nullptr)
            {
                static int64 lasttime = 0;
                static char lastdate[64] = "";
                char syncdate[64] = "";
                static std::string passname;
                static int lastpass = -1;
                char objname[1024];
                if (lastpass != passtype[pass])
                {
                    lastpass = passtype[pass];
                    switch (lastpass)
                    {
                    case PC_PRETOPDOWN:
                        passname = "PRESYNC";
                        break;
                    case PC_BOTTOMUP:
                        passname = "SYNC";
                        break;
                    case PC_POSTTOPDOWN:
                        passname = "POSTSYNC";
                        break;
                    default:
                        passname = "UNKNOWN";
                        break;
                    }
                }
                if (lasttime != global_clock)
                {
                    lasttime = global_clock;
                    convert_from_timestamp(global_clock, lastdate, sizeof(lastdate));
                }
                convert_from_timestamp(this_t < 0 ? -this_t : this_t, syncdate,
                                       sizeof(syncdate));
                if (obj->name == nullptr)
                    sprintf(objname, "%s:%d", obj->oclass->name, obj->id);
                else
                    strcpy(objname, obj->name);
                fprintf(fp, "%s,%s,%d,%d,%s,%s\n", lastdate, passname,
                        global_iteration_limit - iteration_counter, thread, objname,
                        syncdate);
            }
        }
#endif
    }
    else
        this_t = TS_NEVER; /* already out of service */

    /* check for "soft" event (events that are ignored when stopping) */
    if (this_t < -1)
        this_t = -this_t;
    else if (this_t != TS_NEVER)
        data->hard_event++; /* this counts the number of hard events */

    /* check for stopped clock */
    if (this_t < global_clock)
    {
        char b[64];
        output_error("%s: object %s stopped its clock (exec)!", simtime(),
                     object_name(obj, b, 63));
        /* TROUBLESHOOT
                This indicates that one of the objects in the simulator has
           encountered a state where it cannot calculate the time to the next state.
           This usually is caused by a bug in the module that implements that
           object's class.
         */
        data->status = FAILED;
    }
    else
    {
        /* check for iteration limit approach */
        if (iteration_counter == 2 && this_t == global_clock)
        {
            char b[64];
            output_verbose("%s: object %s iteration limit imminent", simtime(),
                           object_name(obj, b, 63));
        }
        else if (iteration_counter == 1 && this_t == global_clock)
        {
            output_error("convergence iteration limit reached for object %s:%d",
                         obj->oclass->name, obj->id);
            /* TROUBLESHOOT
                    This indicates that the core's solver was unable to determine
                    a steady state for all objects for any time horizon.  Identify
                    the object that is causing the convergence problem and contact
                    the developer of the module that implements that object's class.
             */
        }

        /* manage minimum timestep */
        if (global_minimum_timestep > 1 && this_t > global_clock &&
            this_t < TS_NEVER)
            this_t = (((this_t - 1) / global_minimum_timestep) + 1) *
                     global_minimum_timestep;

        /* if this event precedes next step, next step is now this event */
        if (data->step_to > this_t)
        {
            // LOCK(data);
            data->step_to = this_t;
            // UNLOCK(data);
        }
        // printf("data->step_to=%d, this_t=%d\n", data->step_to, this_t);
    }
}

static STATUS init_by_creation()
{
    OBJECT *obj;
    char b[64];
    STATUS rv = SUCCESS;
    TRY
    {
        for (obj = object_get_first(); obj != nullptr; obj = object_get_next(obj))
        {
            if (object_init(obj) == FAILED)
            {
                memset(b, 0, 64);
                output_error("init_all(): object %s initialization failed",
                             object_name(obj, b, 63));
                /* TROUBLESHOOT
                        The initialization of the named object has failed.  Make sure
                   that the object's requirements for initialization are satisfied and
                   try again.
                 */
                rv = FAILED;
                break;
            }
            if ((obj->oclass->passconfig & PC_FORCE_NAME) == PC_FORCE_NAME)
            {
                if (0 == strcmp(obj->name, ""))
                {
                    output_warning("init: object %s:%d should have a name, but doesn't",
                                   obj->oclass->name, obj->id);
                    /* TROUBLESHOOT
                       The object indicated has been flagged by the module which
                       implements its class as one which must be named to work properly.
                       Please provide the object with a name and try again.
                     */
                }
            }
        }
    }
    CATCH(const char *msg)
    {
        output_error("init failure: %s", msg);
        /* TROUBLESHOOT
                The initialization procedure failed.  This is usually preceded
                by a more detailed message that explains why it failed.  Follow
                the guidance for that message and try again.
         */
        rv = FAILED;
    }
    ENDCATCH;
    return rv;
}

static int init_by_deferral_retry(std::vector<OBJECT *> &def_array, int def_ct)
{
    OBJECT *obj;
    int ct = 0, i = 0, obj_rv = 0;
    // OBJECT **next_arr, **tarray;
    std::vector<OBJECT *> next_arr(def_ct, nullptr);
    std::vector<OBJECT *> tarray = {};
    int rv = SUCCESS;
    char b[64];
    int retry = 1, tries = 0, exit_check = 0;
    // tarray = nullptr;

    // Split out the malloc so it can be checked
    // next_arr = (OBJECT **)malloc(def_ct * sizeof(OBJECT *));

    if (global_init_max_defer < 1)
    {
        output_warning(
            "init_max_defer is less than 1, disabling deferred initialization");
    }
    while (retry)
    {
        if (global_init_max_defer <= tries)
        {
            output_error(
                "init_by_deferral_retry(): exhausted initialization attempts");
            rv = FAILED;
            break;
        }

        // Zero the temp array AND its tracking variable
        // memset(next_arr, 0, def_ct * sizeof(OBJECT *));
        // std::ranges::fill(next_arr, nullptr);  // Fill with nullptr
        // std::fill(next_arr, next_arr + size, nullptr);
        ct = 0;

        // initialize each object in def_array
        for (i = 0; i < def_ct; ++i)
        {
            obj = def_array[i];
            obj_rv = object_init(obj);
            switch (obj_rv)
            {
            case 0:
                rv = FAILED;
                memset(b, 0, 64);
                output_error(
                    "init_by_deferral_retry(): object %s initialization failed",
                    object_name(obj, b, 63));
                break;
            case 1:
            {
                // wlock(&obj->lock);
                // replace the above with SharedMutexManager
                std::unique_lock<std::shared_mutex> write_lock(
                    SharedMutexManager::get_mutex(&obj->lock));

                obj->flags |= OF_INIT;
                obj->flags -= OF_DEFERRED;
                // wunlock(&obj->lock);
                write_lock.unlock();
                break;
            }
            case 2:
                next_arr[ct] = obj;
                ++ct;
                break;
                // no default
            }
            if (rv == FAILED)
            {
                // free(next_arr);
                next_arr = {}; // nullptr;
                return rv;
            }
        }

        if (ct == def_ct)
        {
            output_error("init_by_deferral_retry(): all uninitialized objects "
                         "deferred, model is unable to initialize");
            rv = FAILED;
            retry = 0;

            // See which iteration we exited on - multi-swap messes up pointers
            // alternatingly
            exit_check = tries % 2;

            // Determine how to handle that iteration
            if (exit_check == 1)
            {
                // Yes - fix the pointers before leaving, otherwise we'll double-free
                // things!
                tarray = def_array;
                def_array = next_arr;
                next_arr = tarray;
            }
            // Default else - we failed first try, so pointer swap-around below didn't
            // occur
        }
        else if (0 == ct)
        {
            rv = SUCCESS;
            retry = 0;

            // See which iteration we exited on - multi-swap messes up pointers
            // alternatingly
            exit_check = tries % 2;

            // Determine how to handle that iteration
            if (exit_check == 1)
            {
                // Yes - fix the pointers before leaving, otherwise we'll double-free
                // things!
                tarray = def_array;
                def_array = next_arr;
                next_arr = tarray;
            }
            // Default else - we succeeded first try, so pointer swap-around below
            // didn't occur
        }
        else
        {
            ++tries;
            retry = 1;
            tarray = next_arr;
            next_arr = def_array;
            def_array = tarray;
            def_ct = ct;
            // three-point turn to swap the 'next' and the 'old' arrays, memset 0'ing
            // at the top, along with ct reset
        }
    }

    // free(next_arr);
    next_arr = {}; // nullptr;
    return rv;
}

static int init_by_deferral()
{
    // OBJECT **def_array = 0;
    int i = 0, obj_rv = 0, def_ct = 0;
    OBJECT *obj = 0;
    STATUS rv = SUCCESS;
    char b[64];

    // def_array = (OBJECT **)malloc(sizeof(OBJECT *) * object_get_count());

    // use vector for dynamic sizing
    std::vector<OBJECT *> def_array(object_get_count());

    obj = object_get_first();
    while (obj != 0)
    {
        obj_rv = object_init(obj);
        switch (obj_rv)
        {
        case 0:
            rv = FAILED;
            memset(b, 0, 64);
            output_error("init_by_deferral(): object %s initialization failed",
                         object_name(obj, b, 63));
            break;
        case 1:
        {
            // wlock(&obj->lock);
            // replace the above with SharedMutexManager
            std::unique_lock<std::shared_mutex> write_lock(
                SharedMutexManager::get_mutex(&obj->lock));
            obj->flags |= OF_INIT;
            // wunlock(&obj->lock);
            write_lock.unlock();
            break;
        }
        case 2:
        {
            def_array[def_ct] = obj;
            ++def_ct;
            // wlock(&obj->lock);
            // replace the above with SharedMutexManager
            std::unique_lock<std::shared_mutex> write_lock2(
                SharedMutexManager::get_mutex(&obj->lock));
            obj->flags |= OF_DEFERRED;
            // wunlock(&obj->lock);
            write_lock2.unlock();
            break;
        }
            // no default
        }

        if (rv == FAILED)
        {
            // free(def_array);
            def_array = {};
            return rv;
        }

        obj = obj->next;
    }

    // recursecursecursive
    if (def_ct > 0)
    {
        rv = static_cast<STATUS>(init_by_deferral_retry(def_array, def_ct));
        if (rv == FAILED) // got hung up retrying
        {
            // free(def_array);
            def_array = {}; // nullptr;
            return FAILED;
        }
    }
    // free(def_array);
    def_array = {}; // nullptr;

    obj = object_get_first();
    while (obj != 0)
    {
        if ((obj->oclass->passconfig & PC_FORCE_NAME) == PC_FORCE_NAME)
        {
            if (0 == strcmp(obj->name, ""))
            {
                output_warning("init: object %s:%d should have a name, but doesn't",
                               obj->oclass->name, obj->id);
                /* TROUBLESHOOT
                   The object indicated has been flagged by the module which implements
                   its class as one which must be named to work properly.  Please
                   provide the object with a name and try again.
                 */
            }
        }
        obj = obj->next;
    }
    return SUCCESS;
}

// OBJECT **object_heartbeats = nullptr;
std::vector<OBJECT *> object_heartbeats = {}; // use vector for dynamic sizing
unsigned int n_object_heartbeats = 0;
unsigned int max_object_heartbeats = 0;

static STATUS init_all()
{
    OBJECT *obj;
    STATUS rv = SUCCESS;
    output_verbose("initializing objects...");

    /* initialize instances */
    // if ( instance_initall()==FAILED )
    // return FAILED;

    /* initialize loadshapes */
    if (loadshape_initall() == FAILED || enduse_initall() == FAILED)
        return FAILED;

    switch (global_init_sequence)
    {
    case IS_CREATION:
        rv = init_by_creation();
        break;
    case IS_DEFERRED:
        rv = static_cast<STATUS>(init_by_deferral());
        break;
    case IS_BOTTOMUP:
        output_fatal("Bottom-up rank-based initialization mode not yet supported");
        rv = FAILED;
        break;
    case IS_TOPDOWN:
        output_fatal("Top-down rank-based initialization mode not yet supported");
        rv = FAILED;
        break;
    default:
        output_fatal("Unrecognized initialization mode");
        rv = FAILED;
    }
    errno = EINVAL;
    if (rv == FAILED)
        return FAILED;

    /* collect heartbeat objects */
    for (obj = object_get_first(); obj != nullptr; obj = obj->next)
    {
        /* this is a heartbeat object */
        if (obj->heartbeat > 0)
        {
            /* need more space */
            if (n_object_heartbeats >= max_object_heartbeats)
            {
                // OBJECT **bigger;
                int size =
                    (max_object_heartbeats == 0 ? 256 : (max_object_heartbeats * 2));
                // bigger = (OBJECT**)malloc(size*sizeof(OBJECT*));

                std::vector<OBJECT *> bigger(size);

                if (max_object_heartbeats > 0)
                {
                    // memcpy(bigger,object_heartbeats,max_object_heartbeats*sizeof(OBJECT*));
                    std::copy(object_heartbeats.begin(),
                              object_heartbeats.begin() + max_object_heartbeats,
                              bigger.begin());
                    // free(object_heartbeats);
                }
                object_heartbeats = bigger;
                max_object_heartbeats = size;
            }

            /* add this one */
            object_heartbeats[n_object_heartbeats++] = obj;
        }
    }

    /* initialize external links */
    return static_cast<STATUS>(link_initall());
}

// Define the linked list node structure
struct s_simplelinklist
{
    // Use std::shared_ptr<void> for data to improve safety compared to raw void*.
    void *data;
    // Use std::unique_ptr for managing ownership of the next node.
    std::unique_ptr<s_simplelinklist> next;
};

// Alias type for convenience
using SIMPLELINKLIST = s_simplelinklist;

/**************************************************************************
 ** PRECOMMIT ITERATOR
 *		This callback function allows an object to perform actions at
 *  the beginning of a timestep, before the sync process.  This callback is only
 *  triggered once per timestep, and will not fire between iterations.
 **************************************************************************/
static STATUS precommit_all(TIMESTAMP t0)
{
    STATUS rv = SUCCESS;
    static int first = 1;
    static std::unique_ptr<SIMPLELINKLIST> precommit_list = nullptr;
    std::unique_ptr<SIMPLELINKLIST> item = nullptr;
    if (first)
    {
        OBJECT *obj;
        for (obj = object_get_first(); obj != nullptr; obj = object_get_next(obj))
        {
            if (obj->oclass->precommit != nullptr)
            {
                /*item = (SIMPLELINKLIST*)malloc(sizeof(SIMPLELINKLIST));*/
                item = std::make_unique<SIMPLELINKLIST>();
                if (item == nullptr)
                {
                    char name[64];
                    output_error("object %s precommit memory allocation failed",
                                 object_name(obj, name, sizeof(name) - 1));
                    /* TROUBLESHOOT
                       Insufficient memory remains to perform the precommit operation.
                       Free up memory and try again.
                     */
                    return FAILED;
                }
                item->data = (void *)obj;
                item->next = std::move(precommit_list);
                precommit_list = std::move(item);
            }
        }
        first = 0;
    }

    TRY
    {
        /* TODO implement this multithreaded */
        for (SIMPLELINKLIST *item = precommit_list.get(); item != nullptr;
             item = item->next.get())
        {
            OBJECT *obj = (OBJECT *)item->data;
            if ((obj->in_svc <= t0 && obj->out_svc >= t0) &&
                (obj->in_svc_micro >= obj->out_svc_micro))
            {
                if (object_precommit(obj, t0) == FAILED)
                {
                    char name[64];
                    output_error("object %s precommit failed",
                                 object_name(obj, name, sizeof(name) - 1));
                    /* TROUBLESHOOT
                            The precommit function of the named object has failed.  Make
                       sure that the object's requirements for precommit'ing are satisfied
                       and try again.  (likely internal state aberations)
                     */
                    rv = FAILED;
                    break;
                }
            }
        }
    }
    CATCH(const char *msg)
    {
        output_error("precommit_all() failure: %s", msg);
        /* TROUBLESHOOT
                The precommit'ing procedure failed.  This is usually preceded
                by a more detailed message that explains why it failed.  Follow
                the guidance for that message and try again.
         */
        rv = FAILED;
    }
    ENDCATCH;
    return rv;
}

/**************************************************************************
 ** COMMIT ITERATOR
 **************************************************************************/
static std::unique_ptr<SIMPLELINKLIST> commit_list[2] = {nullptr, nullptr};

/* initialize commit_list - must be called only once */
static int commit_init()
{
    int n_commits = 0;
    OBJECT *obj;
    std::unique_ptr<SIMPLELINKLIST> item = nullptr;

    /* build commit list */
    for (obj = object_get_first(); obj != nullptr; obj = object_get_next(obj))
    {
        if (obj->oclass->commit != nullptr)
        {
            /* separate observers */
            unsigned int pc =
                ((obj->oclass->passconfig & PC_OBSERVER) == PC_OBSERVER) ? 1 : 0;
            // item = (SIMPLELINKLIST*)malloc(sizeof(SIMPLELINKLIST));
            item = std::make_unique<SIMPLELINKLIST>();
            if (item == nullptr)
                throw_exception("commit_init memory allocation failure");
            item->data = (void *)obj;
            item->next = std::move(commit_list[pc]);
            commit_list[pc] = std::move(item);
            n_commits++;
        }
    }
    return n_commits;
}

/* single / multiple threaded version of commit_all */
static TIMESTAMP commit_all(TIMESTAMP t0, TIMESTAMP t2) {
	std::atomic_long result{static_cast<long>(TS_NEVER)};
	SIMPLELINKLIST *item;
	unsigned int pc;
	static int n_commits = -1;
	TRY	{
        /* build commit list */
        if (n_commits == -1) 
            n_commits = commit_init();

        /* if no commits found, stop here */
        if (n_commits == 0) {
            result = TS_NEVER;
        } 
        else {
            for (pc = 0; pc < 2; pc++) {
                if (global_threadcount == 1) {
                    // Single-threaded fallback
                    for (item = commit_list[pc].get(); item != nullptr; item = item->next.get()) {
                        OBJECT *obj = (OBJECT *)item->data;
                        if (t0 < obj->in_svc)
                        {
                            if (obj->in_svc < result)
                                result = obj->in_svc;
                        }
                        else if ((t0 == obj->in_svc) && (obj->in_svc_micro != 0))
                        {
                            if (obj->in_svc == result)
                                result = obj->in_svc + 1;
                        }
                        else if (obj->out_svc >= t0)
                        {
                            TIMESTAMP next = object_commit(obj, t0, t2);
                            if (next == TS_INVALID) {
                                char name[64];
                                throw_exception("object %s commit failed",
                                    object_name(obj, name, sizeof(name) - 1));
                                /* TROUBLESHOOT
                                    The commit function of the named object has failed.  
                                    Make sure that the object's requirements for committing are 
                                    satisfied and try again. (likely internal state aberrations)
                                */
                            }
                            if (next < result)
                                result = next;
                        }
                    }
                } 
                else {
                    for (item = commit_list[pc].get(); item != nullptr; item = item->next.get()) {
                        OBJECT *obj = (OBJECT *) item->data;
                        threadpool->add_job([=, &obj, &result]() {
                            auto inner_result = result.load();
                            if (t0 < obj->in_svc) {
                                if (obj->in_svc < inner_result) 
                                    result.store(obj->in_svc);
                            } 
                            else if ((t0 == obj->in_svc) && (obj->in_svc_micro != 0)) {
                                if (obj->in_svc == inner_result)
                                    result.store(obj->in_svc + 1);
                            } 
                            else if (obj->out_svc >= t0) {
                                TIMESTAMP next = object_commit(obj, t0, t2);
                                if (next == TS_INVALID) {
                                    char name[64];
                                    throw_exception("object %s commit failed",
                                        object_name(obj, name, sizeof(name) - 1));
                                    /* TROUBLESHOOT
                                        The commit function of the named object has failed.  
                                        Make sure that the object's requirements for committing are 
                                        satisfied and try again. (likely internal state aberrations)
                                    */
                                }
                                if (next < result.load()) 
                                    result.store(next);
                            }
                        });
                    }
                    threadpool->await();
                }
            }
        }
    }
    CATCH(const char *msg)
    {
        output_error("commit_all() failure: %s", msg);
        /* TROUBLESHOOT
            The commit'ing procedure failed.  This is usually preceded
            by a more detailed message that explains why it failed.
            Follow the guidance for that message and try again.
        */
        result = TS_INVALID;
    }
    ENDCATCH;
    return result.load();
}

/**************************************************************************
 ** FINALIZE ITERATOR
 **************************************************************************/
static STATUS finalize_all()
{
    STATUS rv = SUCCESS;
    static int first = 1;
    static std::unique_ptr<SIMPLELINKLIST> finalize_list = nullptr;
    std::unique_ptr<SIMPLELINKLIST> item = nullptr;
    if (first)
    {
        OBJECT *obj;
        for (obj = object_get_first(); obj != nullptr; obj = object_get_next(obj))
        {
            if (obj->oclass->finalize != nullptr)
            {
                // item = (SIMPLELINKLIST*)malloc(sizeof(SIMPLELINKLIST));
                item = std::make_unique<SIMPLELINKLIST>();
                if (item == nullptr)
                {
                    char name[64];
                    output_error("object %s finalize memory allocation failed",
                                 object_name(obj, name, sizeof(name) - 1));
                    /* TROUBLESHOOT
                       Insufficient memory remains to perform the finalize operation.
                       Free up memory and try again.
                     */
                    return FAILED;
                }
                item->data = (void *)obj;
                item->next = std::move(finalize_list);
                finalize_list = std::move(item);
            }
        }
        first = 0;
    }

    TRY
    {
        /* TODO implement this multithreaded */
        for (SIMPLELINKLIST *item = finalize_list.get(); item != nullptr;
             item = item->next.get())
        {
            OBJECT *obj = (OBJECT *)item->data;
            if (object_finalize(obj) == FAILED)
            {
                char name[64];
                output_error("object %s finalize failed",
                             object_name(obj, name, sizeof(name) - 1));
                /* TROUBLESHOOT
                        The finalize function of the named object has failed.  Make sure
                   that the object's requirements for finalizing are satisfied and try
                   again.  (likely internal state aberations)
                 */
                rv = FAILED;
                break;
            }
        }
    }
    CATCH(const char *msg)
    {
        output_error("finalize_all() failure: %s", msg);
        /* TROUBLESHOOT
                The finalizing procedure failed.  This is usually preceded
                by a more detailed message that explains why it failed.  Follow
                the guidance for that message and try again.
         */
        rv = FAILED;
    }
    ENDCATCH;
    return rv;
}

STATUS exec_test(struct sync_data *data, int pass, OBJECT *obj);

STATUS t_setup_ranks() { return setup_ranks(); }

TIMESTAMP sync_heartbeats()
{
    TIMESTAMP t1 = TS_NEVER;
    unsigned int n;
    for (n = 0; n < n_object_heartbeats; n++)
    {
        TIMESTAMP t2 = object_heartbeat(object_heartbeats[n]);
        if (absolute_timestamp(t2) < absolute_timestamp(t1))
            t1 = t2;
    }

    /* heartbeats are always soft updates */
    return t1 < TS_NEVER ? -absolute_timestamp(t1) : TS_NEVER;
}

/* this function synchronizes all internal behaviors */
TIMESTAMP syncall_internals(TIMESTAMP t1)
{
    TIMESTAMP h1, h2, s1, s2, s3, s4, s5, s6, se, sa;

    /* external link must be first */
    h1 = link_syncall(t1);

    /* @todo add other internal syncs here */
    // h2 = instance_syncall(t1);
    s1 = randomvar_syncall(t1);
    s2 = schedule_syncall(t1);
    s3 = loadshape_syncall(t1);
    s4 = transform_syncall(
        t1, static_cast<TRANSFORMSOURCE>(XS_SCHEDULE | XS_LOADSHAPE), nullptr);
    s5 = enduse_syncall(t1);

    /* heartbeats go last */
    s6 = sync_heartbeats();

    /* earliest soft event */
    se = absolute_timestamp(earliest_timestamp(s1, s2, s3, s4, s5, s6, TS_ZERO));

    /* final event */
    // sa = earliest_timestamp(h1, h2, se != TS_NEVER ? -se : TS_NEVER, TS_ZERO);
    sa = earliest_timestamp(h1, se != TS_NEVER ? -se : TS_NEVER, TS_ZERO);

    // Round off to the minimum timestep
    if (global_minimum_timestep > 1 && absolute_timestamp(sa) > global_clock &&
        sa < TS_NEVER)
    {
        if (sa > 0)
            sa = (((sa - 1) / global_minimum_timestep) + 1) * global_minimum_timestep;
        else
            sa = -(((-sa - 1) / global_minimum_timestep) + 1) *
                 global_minimum_timestep;
    }
    return sa;
}

void exec_sleep(unsigned int usec)
{
#ifdef _WIN32
    Sleep(usec / 1000);
#else
    usleep(usec);
#endif
}

typedef struct s_objsyncdata
{
    unsigned int n; // thread id 0~n_threads for this object rank list
    bool ok;
    LISTITEM *ls;
    unsigned int nObj; // number of obj in this object rank list
    unsigned int t0;
    int i; // index of mutex or cond this object rank list uses
} OBJSYNCDATA;

// After the first iteration, setTP = false;
bool setTP = true;
static std::vector<int> n_idx = {0};
static std::vector<std::unique_ptr<OBJSYNCDATA>> thread;
static std::vector<std::unique_ptr<unsigned int>> n_threads;

/**************************************************************************
 * MAIN LOOP CONTROL
 **************************************************************************/

unsigned int mls_svr_lock;
std::condition_variable_any mls_svr_signal;

int mls_created = 0;

void exec_mls_create()
{
    int rv = 0;
    mls_created = 1;

    output_debug("exec_mls_create()");
}

void exec_mls_init()
{
    if (mls_created == 0)
    {
        exec_mls_create();
    }
    if (global_mainloopstate == MLS_PAUSED)
        exec_mls_suspend();
    else
        sched_update(global_clock, global_mainloopstate);
}

void exec_mls_suspend()
{
    int loopctr = 10;
    int rv = 0;
    output_debug("pausing simulation");
    if (global_multirun_mode == MRM_STANDALONE &&
        strcmp(global_environment, "server") != 0)
        output_warning("suspending simulation with no server/multirun active to "
                       "control mainloop state");
    output_debug("lock_ (%x->%x)", &mls_svr_lock, mls_svr_lock);

    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&mls_svr_lock));

    output_debug("sched update_");
    sched_update(global_clock, global_mainloopstate = MLS_PAUSED);
    output_debug("wait loop_");
    while (global_clock == TS_ZERO || (global_clock >= global_mainlooppauseat &&
                                       global_mainlooppauseat < TS_NEVER))
    {
        if (loopctr > 0)
        {
            output_debug(" * tick (%i)", --loopctr);
        }
        mls_svr_signal.wait(lock);
    }
    output_debug("sched update_");
    sched_update(global_clock, global_mainloopstate = MLS_RUNNING);
    output_debug("unlock_");
}

void exec_mls_resume(TIMESTAMP ts)
{
    int rv = 0;
    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&mls_svr_lock));
    global_mainlooppauseat = ts;

    lock.unlock();
    mls_svr_signal.notify_all();
}

void exec_mls_statewait(unsigned states)
{
    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&mls_svr_lock));
    while (((global_mainloopstate & states) | states) == 0)
        mls_svr_signal.wait(lock);
}

void exec_mls_done()
{
    sched_update(global_clock, global_mainloopstate = MLS_DONE);
}

/******************************************************************
 SYNC HANDLING API
 *******************************************************************/
// static struct sync_data main_sync = {TS_NEVER,0,SUCCESS};
static std::shared_ptr<sync_data> main_sync =
    std::make_shared<sync_data>(sync_data{TS_NEVER, 0, SUCCESS});

/** Reset the sync time data structure

        This call clears the sync data structure
        and prepares it for a new interation pass.
        If no sync event are posted, the result of
        the pass will be a successful soft NEVER,
        which usually means the simulation stops at
        steady state.
 **/
void exec_sync_reset(std::shared_ptr<struct sync_data>
                         &d) /**< sync data to reset (nullptr to reset main) **/
{
    if (d == nullptr)
        d = main_sync;
    d->step_to = TS_NEVER;
    d->hard_event = 0;
    d->status = SUCCESS;
}
/** Merge the sync data structure

        This call posts a new sync event \p to into
        an existing sync data structure \p from.
        If \p to is \p nullptr, then the main exec sync
        event is updated.  If the status of \p from
        is \p FAILED, then the \p to sync time is
        set to \p TS_INVALID.  If the time of \p from
        is \p TS_NEVER, then \p to is not updated.  In
        all other cases, the \p to sync time is updated
        with #exec_sync_set.

        @see #exec_sync_set
 **/
// void exec_sync_merge(struct sync_data *to, /**< sync data to merge to
// (nullptr to update main)  **/
//					struct sync_data *from) /**< sync data
// to merge from */
void exec_sync_merge(
    std::shared_ptr<struct sync_data>
        &to,                                 /**< sync data to merge to (nullptr to update main)  **/
    std::shared_ptr<struct sync_data> &from) /**< sync data to merge from */
{
    if (to == nullptr)
        to = main_sync;
    if (from == nullptr)
        from = main_sync;
    if (from == to)
        return;
    if (exec_sync_isinvalid(from))
        exec_sync_set(to, TS_INVALID, false);
    else if (exec_sync_isnever(from))
    {
    } /* do nothing */
    else if (exec_sync_ishard(from))
        exec_sync_set(to, exec_sync_get(from), false);
    else
        exec_sync_set(to, -exec_sync_get(from), false);
}
/** Update the sync data structure

        This call posts a new time to a sync event.
        If the event is \p nullptr, then the main sync event is updated.
        If the new time is \p TS_NEVER, then the event is not updated.
        If the new time is \p TS_INVALID, then the event status is changed to
 FAILED. If the new time is not between -TS_MAX and TS_MAX, then an exception is
 thrown. If the event time is TS_NEVER, then if the time is positive a hard
 event is posted, if the time is negative a soft event is posted, otherwise the
 event status is changed to FAILED. Otherwise, if the event is hard, then the
 hard event count is increment and if the time is earlier it is posted.
        Otherwise, if the event is soft, then if the time is earlier it is
 posted. Otherwise, the event status is changed to FAILED.
 **/
void exec_sync_set(
    std::shared_ptr<struct sync_data>
        &d,         /**< sync data to update (nullptr to update main) */
    TIMESTAMP t,    /**< timestamp to update with (negative time means soft event,
                       0 means failure) */
    bool deltaflag) /**< flag to let us know this was a deltamode exit - force
                       it forward, otherwise can fail to exit */
{
    if (d == nullptr)
        d = main_sync;
    if (t == TS_NEVER)
        return; /* nothing to do */
    if (t == TS_INVALID)
    {
        d->status = FAILED;
        return;
    }
    if (t <= -TS_MAX || t > TS_MAX)
        throw_exception(
            "set_synctime(struct sync_data *d={TIMESTAMP step_to=%lli,int32 "
            "hard_event=%d, STATUS=%s}, TIMESTAMP t=%lli): timestamp is not valid",
            d->step_to, d->hard_event, d->status == SUCCESS ? "SUCCESS" : "FAILED",
            t);
    if (d->step_to == TS_NEVER)
    {
        if (t < TS_NEVER && t > 0)
        {
            d->step_to = t;
            d->hard_event++;
        }
        else if (t < 0)
            d->step_to = -t;
        else
            d->status = FAILED;
    }
    else if (t > 0) /* hard event */
    {
        d->hard_event++;
        if (deltaflag == false)
        {
            if (d->step_to > t)
                d->step_to = t;
        }
        else /* Deltamode exit - override us */
        {
            d->step_to = t;
        }
    }
    else if (t < 0) /* soft event */
    {
        if (d->step_to > -t)
            d->step_to = -t;
    }
    else // t==0 -> invalid
    {
        d->status = FAILED;
    }
}
/** Get the current sync time
        @return the proper (positive) event sync time, TS_NEVER, or TS_INVALID.
 **/
TIMESTAMP exec_sync_get(
    std::shared_ptr<struct sync_data>
        &d) /**< Sync data to get sync time from (nullptr to read main)  */
{
    if (d == nullptr)
        d = main_sync;
    if (exec_sync_isnever(d))
        return TS_NEVER;
    if (exec_sync_isinvalid(d))
        return TS_INVALID;
    return absolute_timestamp(d->step_to);
}
/** Get the current hard event count
        @return the number of hard events associated with this sync event.
 **/
unsigned int exec_sync_getevents(
    std::shared_ptr<struct sync_data>
        &d) /**< Sync data to get sync events from (nullptr to read main)  */
{
    if (d == nullptr)
        d = main_sync;
    return d->hard_event;
}
/** Determine whether the current sync data is a hard sync
        @return non-zero if the event is a hard event, 0 if the event is a soft
 event
 **/
int exec_sync_ishard(std::shared_ptr<struct sync_data>
                         &d) /**< Sync data to read hard sync status from
                                (nullptr to read main)  */
{
    if (d == nullptr)
        d = main_sync;
    return d->hard_event > 0;
}
/** Determine whether the current sync data time is never
        @return non-zero if the event is NEVER, 0 otherwise
 **/
int exec_sync_isnever(std::shared_ptr<struct sync_data>
                          &d) /**< Sync data to read never sync status from
                                (nullptr to read main)  */
{
    if (d == nullptr)
        d = main_sync;
    return d->step_to == TS_NEVER;
}
/** Determine whether the currenet sync time is invalid (nullptr to read main)
        @return non-zero if the status if FAILED, 0 otherwise
 **/
int exec_sync_isinvalid(
    std::shared_ptr<struct sync_data>
        &d) /**< Sync data to read invalid sync status from */
{
    if (d == nullptr)
        d = main_sync;
    return exec_sync_getstatus(d) == FAILED;
}
/** Determine the current sync status
        @return the event status (SUCCESS or FAILED)
 **/
STATUS exec_sync_getstatus(
    std::shared_ptr<struct sync_data>
        &d) /**< Sync data to read sync status from (nullptr to read main)  */
{
    if (d == nullptr)
        d = main_sync;
    return d->status;
}
/** Determine whether sync time is a running simulation
        @return true if the simulation should keep going, false if it should
 stop
 **/
bool exec_sync_isrunning(std::shared_ptr<struct sync_data> d)
{
    return exec_sync_get(d) <= global_stoptime && !exec_sync_isnever(d) &&
           exec_sync_ishard(d);
}

void exec_clock_update_modules()
{
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    TIMESTAMP t1 = exec_sync_get(sync_data_nullptr);
    MODULE *mod;
    int ok = 0;
    while (!ok)
    {
        ok = 1;
        for (mod = module_get_first(); mod != nullptr; mod = mod->next)
        {
            if (mod->clockupdate != nullptr)
            {
                TIMESTAMP t2 = mod->clockupdate(reinterpret_cast<TIMESTAMP *>(t1));
                if (t2 < t1)
                {
                    t1 = t2;
                    ok = 0;
                }
            }
        }
    }
    exec_sync_set(sync_data_nullptr, t1, false);
}

STATUS multi_thread_init()
{
    std::vector<std::shared_ptr<struct arg_data>> arg_data_array = {};
    int j;
    /* set thread count equal to processor count if not passed on command-line */
    if (global_threadcount == 0)
    {
        global_threadcount = processor_count();
        output_verbose("using %d helper thread(s)", global_threadcount);
    }

    /* allocate thread synchronization data */
    thread_data = std::make_shared<struct thread_data>();
    if (!thread_data)
    {
        output_error("thread memory allocation failed");
        /* TROUBLESHOOT
                A thread memory allocation failed.
                Follow the standard process for freeing up memory ang try again.
                */
        return FAILED;
    }
    thread_data->data.resize(global_threadcount);
    thread_data->count = global_threadcount;

    for (j = 0; j < thread_data->count; j++)
    {
        thread_data->data[j] = std::make_shared<struct sync_data>();
        thread_data->data[j]->status = SUCCESS;
    }

    return SUCCESS;
}

static void *obj_syncproc(void *ptr)
{
	OBJSYNCDATA *data = (OBJSYNCDATA*)ptr;
	LISTITEM *s;
	unsigned int n;
	int i = data->i;

	// begin processing loop
	while (data->ok)
	{
		for (s=data->ls, n=0; s!=nullptr, n<data->nObj; s=s->next,n++) {
			OBJECT *obj = static_cast<OBJECT *>(s->data);
            clock_t ts = (clock_t)exec_clock();
            ss_do_object_sync(data->n, s->data);
        	objs_synctime += (clock_t)exec_clock() - ts;
            if (obj->valid_to == TS_INVALID)
            {
                // Get us out of the loop so others don't exec on bad status
                break;
            }
		}
        return nullptr;
	}
   return nullptr;
}

void multithread_stuff(int iObjRankList, int i, int k)
{
	unsigned int n_items, objn = 0, n;
	unsigned int n_obj = ranks[pass]->ordinal[i]->size;
	LISTITEM *ptr;
	int incr;
	int j = 0;
	OBJSYNCDATA *objsyncdata = nullptr;

	// Only create threadpool for each object rank list at the first iteration.
	// Reuse the threadppol of each object rank list at all other iterations.
	if (setTP) {
        incr = (int)ceil((float)n_obj / global_threadcount);
        // if the number of objects is less than or equal to the number of threads, each thread process one object 	
        if (incr <= 1)
        {
            n_threads[iObjRankList] = std::make_unique<unsigned int>(n_obj);
            n_items = 1;
            // if the number of objects is greater than the number of threads, each thread process the same number of
            // objects (incr), except that the last thread may process less objects
        }
        else
        {
            n_threads[iObjRankList] = std::make_unique<unsigned int>((int)ceil((float)n_obj / incr));
            n_items = incr;
        }
        if (*n_threads[iObjRankList] > global_threadcount)
        {
            output_error("Running threads > global_threadcount");
            exit(0);
        }

        // allocate thread list
        objsyncdata = new OBJSYNCDATA();
        for (ptr = ranks[pass]->ordinal[i]->first; ptr != nullptr; ptr = ptr->next)
        {
            if (objsyncdata->nObj == n_items) {
                objn++;
                thread.push_back(std::unique_ptr<OBJSYNCDATA>(objsyncdata));
                objsyncdata = new OBJSYNCDATA();
            }
            if (objsyncdata->nObj == 0)
                objsyncdata->ls = ptr;
            objsyncdata->nObj++;
        }
        if (objn < *n_threads[iObjRankList]) 
            thread.push_back(std::unique_ptr<OBJSYNCDATA>(objsyncdata));
        n_idx.push_back(k + *n_threads[iObjRankList]);
    }

    for (n = 0; n < *n_threads[iObjRankList]; n++) {
        thread[n+k]->ok = true;
        thread[n+k]->t0 = 0;
        thread[n+k]->i = iObjRankList;
        if (!threadpool->add_job([=] { obj_syncproc(&*thread[n+k]); })) {
            output_fatal("obj_sync thread creation failed");
            thread[n+k]->ok = false;
        } 
        else {
            thread[n+k]->n = n;
        }
    }

    threadpool->await();

	for (j = 0; j < thread_data->count; j++)
	{
		if (thread_data->data[j]->status == FAILED)
		{
			exec_sync_set(thread_data->data[j], TS_INVALID, false);
			THROW("synchronization failed");
		}
	}
}

/******************************************************************
 *  MAIN EXEC LOOP
 ******************************************************************/
// Commenting everything related to multithreading
STATUS run_preparation()
{
    struct arg_data *arg_data_array;
    int nObjRankList, iObjRankList;
    int j, k;

    // Ensure deterministic behavior by setting a fixed random seed if not already set
    if (global_randomseed == 0)
    {
        global_randomseed = 42; // Default seed for reproducibility
        output_verbose(
            "Setting default random seed to %d for deterministic behavior",
            global_randomseed);
        srand(global_randomseed);
    }

    /* initialize the main loop state control */
    exec_mls_init();

    /* perform object initialization */
    if (init_all() == FAILED)
    {
        output_error("model initialization failed");
        /* TROUBLESHOOT
                The initialization procedure failed.  This is usually preceded
                by a more detailed message that explains why it failed.  Follow
                the guidance for that message and try again.
         */
        return FAILED;
    }

    /* establish rank index if necessary */
    if (ranks == nullptr && setup_ranks() == FAILED)
    {
        output_error("ranks setup failed");
        /* TROUBLESHOOT
                The rank setup procedure failed.  This is usually preceded
                by a more detailed message that explains why it failed.  Follow
                the guidance for that message and try again.
         */
        return FAILED;
    }

    /* run checks */
    if (global_runchecks)
        return static_cast<STATUS>(module_checkall());

    /* compile only check */
    if (global_compileonly)
        return SUCCESS;

    /* enable non-determinism check, if any */
    if (global_randomseed != 0 && global_threadcount > 1)
        global_nondeterminism_warning = 1;

    if (!global_debug_mode)
    {
        /* schedule progress report event */
        if (global_show_progress)
        {
            realtime_schedule_event(realtime_now() + 1, show_progress);
        }

        if (multi_thread_init() == FAILED)
        {
            return FAILED;
        }
    }
    else
    {
        output_debug("debug mode running single threaded");
        output_message("GridLAB-D entering debug mode");
    }

    /* realtime startup */
    if (global_run_realtime > 0)
    {
        char buffer[64];
        time_t gtime;
        time(&gtime);
        global_clock = gtime;
        output_verbose(
            "realtime mode requires using now (%s) as starttime",
            convert_from_timestamp(global_clock, buffer, sizeof(buffer)) > 0
                ? buffer
                : "invalid time");
        if (global_stoptime < global_clock)
            global_stoptime = TS_NEVER;
    }

    //  maybe that's all we need...
    iteration_counter = global_iteration_limit;
    federation_iteration_counter = global_iteration_limit;

    /* reset sync event */
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    exec_sync_reset(sync_data_nullptr);
    exec_sync_set(sync_data_nullptr, global_clock, false);
    if (global_stoptime < TS_NEVER)
        exec_sync_set(sync_data_nullptr, global_stoptime + 1, false);

    /* signal handler */
    signal(SIGABRT, exec_sighandler);
    signal(SIGINT, exec_sighandler);
    signal(SIGTERM, exec_sighandler);

    /* initialize delta mode */
    if (!delta_init())
    {
        output_error("delta mode initialization failed");
        /* TROUBLESHOOT
           The initialization of the deltamode subsystem failed.
           The failure message is preceded by one or more errors that will provide
           more information.
         */
    }

    // count how many object rank list in one iteration
    nObjRankList = 0;
    /* scan the ranks of objects */
    for (pass = 0; ranks[pass] != nullptr; pass++)
    {
        int i;
        /* process object in order of rank using index */
        for (i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
        {
            /* skip empty lists */
            if (ranks[pass]->ordinal[i] == nullptr)
                continue;
            nObjRankList++; // count how many object rank list in one iteration
        }
    }

    /* allocate and initialize thread data */
    output_debug("nObjRankList=%d ", nObjRankList);

    n_threads.resize(nObjRankList);

    // global test mode
    if (global_test_mode == true)
        return static_cast<STATUS>(test_exec());

    /* check for a model */
    if (object_get_count() == 0)
        /* no object -> nothing to do */
        return SUCCESS;

    // sjin: GetMachineCycleCount
    cstart = (clock_t)exec_clock();
    return SUCCESS;
}

int handle_delta_mode_operation()
{
    DT deltatime = delta_update();
    if (deltatime == DT_INVALID)
    {
        output_error("delta_update() failed, deltamode operation cannot continue");
        /*  TROUBLESHOOT
        An error was encountered while trying to perform a deltamode update.  Look
        for other relevant deltamode messages for indications as to why this may
        have occurred. If the error persists, please submit your code and a bug
        report via the trac website.
        */
        global_simulation_mode = SM_ERROR;
        THROW("Deltamode simulation failure");
        return -1; // Just in case, but probably not needed
    }
    else if (deltatime > 0)
    {
        /* Reset the iteration counter here - if we made it this far, we moved
         * forward */
        /* If a simulate "stays" in deltamode too long, the periodic checks will
         * still exhaust the iteration limit - this fixes that */
        iteration_counter = global_iteration_limit;
        federation_iteration_counter = global_iteration_limit;
    }
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    exec_sync_set(sync_data_nullptr, global_clock + deltatime, true);
    global_simulation_mode = SM_EVENT;
    return 0;
}

void report_performance_after_run(time_t start_time, int64 passes,
                                  int64 tsteps)
{
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    /* report performance */
    if (global_profiler && !exec_sync_isinvalid(sync_data_nullptr))
    {
        double elapsed_sim =
            timestamp_to_hours(global_clock) - timestamp_to_hours(global_starttime);
        double elapsed_wall = (double)(realtime_now() - start_time + 1);
        double sync_time = 0;
        double sim_speed = object_get_count() / 1000.0 * elapsed_sim / elapsed_wall;

        extern clock_t loader_time;
        extern clock_t instance_synctime;
        extern clock_t randomvar_synctime;
        extern clock_t schedule_synctime;
        extern clock_t loadshape_synctime;
        extern clock_t enduse_synctime;
        extern clock_t transform_synctime;
        extern clock_t objs_synctime;

        CLASS *cl;
        DELTAPROFILE *dp = delta_getprofile();
        double delta_runtime = 0, delta_simtime = 0;
        if (global_threadcount == 0)
            global_threadcount = 1;
        for (cl = class_get_first_class(); cl != nullptr; cl = cl->next)
            sync_time += ((double)cl->profiler.clocks) / global_ms_per_second;
        sync_time /= global_threadcount;
        delta_runtime = dp->t_count > 0
                            ? (dp->t_preupdate + dp->t_update + dp->t_postupdate) /
                                  global_ms_per_second
                            : 0;
        delta_simtime =
            dp->t_count * (double)dp->t_delta / (double)dp->t_count / 1e9;

        output_profile("\nCore profiler results");
        output_profile("======================\n");
        output_profile("Total objects           %8d objects", object_get_count());
        output_profile("Parallelism             %8d thread%s", global_threadcount,
                       global_threadcount > 1 ? "s" : "");
        output_profile("Total time              %8.1f seconds", elapsed_wall);
        output_profile("  Core time             %8.1f seconds (%.1f%%)",
                       (elapsed_wall - sync_time - delta_runtime),
                       (elapsed_wall - sync_time - delta_runtime) / elapsed_wall *
                           100);
        output_profile("    Compiler            %8.1f seconds (%.1f%%)",
                       (double)loader_time / global_ms_per_second,
                       ((double)loader_time / global_ms_per_second) / elapsed_wall *
                           100);
        output_profile("    Instances           %8.1f seconds (%.1f%%)",
                       (double)instance_synctime / global_ms_per_second,
                       ((double)instance_synctime / global_ms_per_second) /
                           elapsed_wall * 100);
        output_profile("    Random variables    %8.1f seconds (%.1f%%)",
                       (double)randomvar_synctime / global_ms_per_second,
                       ((double)randomvar_synctime / global_ms_per_second) /
                           elapsed_wall * 100);
        output_profile("    Schedules           %8.1f seconds (%.1f%%)",
                       (double)schedule_synctime / global_ms_per_second,
                       ((double)schedule_synctime / global_ms_per_second) /
                           elapsed_wall * 100);
        output_profile("    Loadshapes          %8.1f seconds (%.1f%%)",
                       (double)loadshape_synctime / global_ms_per_second,
                       ((double)loadshape_synctime / global_ms_per_second) /
                           elapsed_wall * 100);
        output_profile("    Enduses             %8.1f seconds (%.1f%%)",
                       (double)enduse_synctime / global_ms_per_second,
                       ((double)enduse_synctime / global_ms_per_second) /
                           elapsed_wall * 100);
        output_profile("    Transforms          %8.1f seconds (%.1f%%)",
                       (double)transform_synctime / global_ms_per_second,
                       ((double)transform_synctime / global_ms_per_second) /
                           elapsed_wall * 100);
        output_profile("    Objects             %8.1f seconds (%.1f%%)",
                       (double)objs_synctime / global_ms_per_second,
                       ((double)objs_synctime / global_ms_per_second) /
                           elapsed_wall * 100);
        output_profile("  Model time            %8.1f seconds/thread (%.1f%%)",
                       sync_time, sync_time / elapsed_wall * 100);
        if (dp->t_count > 0)
            output_profile("  Deltamode time        %8.1f seconds/thread (%.1f%%)",
                           delta_runtime, delta_runtime / elapsed_wall * 100);
        output_profile("Simulation time         %8.0f days", elapsed_sim / 24);
        if (sim_speed > 10.0)
            output_profile("Simulation speed         %7.0lfk object.hours/second",
                           sim_speed);
        else if (sim_speed > 1.0)
            output_profile("Simulation speed         %7.1lfk object.hours/second",
                           sim_speed);
        else
            output_profile("Simulation speed         %7.0lf object.hours/second",
                           sim_speed * 1000);
        output_profile("Passes completed        %8d passes", passes);
        output_profile("Time steps completed    %8d timesteps", tsteps);
        output_profile("Convergence efficiency  %8.02lf passes/timestep",
                       (double)passes / tsteps);
#ifndef NOLOCKS
        output_profile("Read lock contention    %7.01lf%%",
                       (rlock_spin > 0
                            ? (1 - (double)rlock_count / (double)rlock_spin) * 100
                            : 0));
        output_profile("Write lock contention   %7.01lf%%",
                       (wlock_spin > 0
                            ? (1 - (double)wlock_count / (double)wlock_spin) * 100
                            : 0));
#endif
        output_profile("Average timestep        %7.0lf seconds/timestep",
                       (double)(global_clock - global_starttime) / tsteps);
        output_profile("Simulation rate         %7.0lf x realtime",
                       (double)(global_clock - global_starttime) / elapsed_wall);
        if (dp->t_count > 0)
        {
            double total =
                dp->t_preupdate + dp->t_update + dp->t_interupdate + dp->t_postupdate;
            output_profile("\nDelta mode profiler results");
            output_profile("===========================\n");
            output_profile("Active modules          %s", dp->module_list);
            output_profile("Initialization time     %8.1lf seconds",
                           (double)(dp->t_init) / (double)global_ms_per_second);
            output_profile("Number of updates       %8" FMT_INT64 "u", dp->t_count);
            output_profile("Average update timestep %8.4lf ms",
                           (double)dp->t_delta / (double)dp->t_count / 1e6);
            output_profile("Minumum update timestep %8.4lf ms", dp->t_min / 1e6);
            output_profile("Maximum update timestep %8.4lf ms", dp->t_max / 1e6);
            output_profile("Total deltamode simtime %8.1lf s", delta_simtime / 1000);
            output_profile("Preupdate time          %8.1lf s (%.1f%%)",
                           (double)(dp->t_preupdate) / (double)global_ms_per_second,
                           (double)(dp->t_preupdate) / total * 100);
            output_profile("Object update time      %8.1lf s (%.1f%%)",
                           (double)(dp->t_update) / (double)global_ms_per_second,
                           (double)(dp->t_update) / total * 100);
            output_profile("Interupdate time        %8.1lf s (%.1f%%)",
                           (double)(dp->t_interupdate) / (double)global_ms_per_second,
                           (double)(dp->t_interupdate) / total * 100);
            output_profile("Postupdate time         %8.1lf s (%.1f%%)",
                           (double)(dp->t_postupdate) / (double)global_ms_per_second,
                           (double)(dp->t_postupdate) / total * 100);
            output_profile("Total deltamode runtime %8.1lf s (100%%)", delta_runtime);
            output_profile("Simulation rate         %8.1lf x realtime",
                           delta_simtime / delta_runtime / 1000);
        }
        output_profile("\n");
    }
}

/** Execute a single simulation iteration
        This function executes one iteration of the simulation loop, handling
        realtime control, delta mode, object synchronization, and event
 processing.
        @param passes reference to pass counter
        @param tsteps reference to timestep counter
        @param j reference to loop variable used for thread data
        @param ptr reference to list item pointer
        @param pc_rv reference to precommit return value
        @param iObjRankList reference to object rank list index
        @return true if simulation should continue, false if it should stop
 **/
static bool execute_single_simulation_iteration(int64 &passes, int64 &tsteps,
                                                int &j, LISTITEM *&ptr,
                                                int &pc_rv, int &iObjRankList)
{
    int n_cnt = 0;
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    TIMESTAMP internal_synctime;
    output_debug("*** main loop event at %lli; stoptime=%lli, n_events=%i, "
                 "exitcode=%i ***",
                 exec_sync_get(sync_data_nullptr), global_stoptime,
                 exec_sync_getevents(sync_data_nullptr), exec_getexitcode());

    /* update the process table info */
    sched_update(global_clock, MLS_RUNNING);

    /* main loop control */
    if (global_clock >= global_mainlooppauseat &&
        global_mainlooppauseat < TS_NEVER)
        exec_mls_suspend();

    /* realtime control of global clock */
    if (global_run_realtime == 0 && global_clock >= global_enter_realtime)
        global_run_realtime = 1;

    if (global_run_realtime > 0 && iteration_counter > 0)
    {
        double metric = 0.;
        short fall_behind = 0;
        using std::chrono::system_clock;
        static bool initialized = false;
        static std::chrono::time_point<
            system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>>
            t1;
        static std::chrono::time_point<
            system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>>
            t2;
        if (!initialized)
        { //[[unlikely]] {
            t1 = system_clock::now();
            t2 = t1 + 1s;
            initialized = true;
        }
        else
        { //[[likely]] {
            t1 = t2;
            t2 += 1s; // One second from last time step
        }

        if (system_clock::now() < t2)
        {
            output_verbose(
                "waiting %d nsec",
                std::chrono::nanoseconds(t2 - system_clock::now()).count());
            std::this_thread::sleep_until(t2);
            global_clock += global_run_realtime;
            metric = (1.0 * (t2 - t1)) / std::chrono::seconds(1);
            fall_behind = 0;
        }
        else
        {
            output_error("simulation failed to keep up with real time");
            fall_behind++;
        }

        if (fall_behind > 5)
        { // [[unlikely]] {
            output_fatal(
                "simulation fell behind realtime for more than 5 consecutive cycles");
        }

#define IIR 0.9 /* about 30s for 95% unit step response */
        global_realtime_metric = global_realtime_metric * IIR + metric * (1 - IIR);
        exec_sync_reset(sync_data_nullptr);
        exec_sync_set(sync_data_nullptr, global_clock, false);
        output_verbose("realtime clock advancing to %d", (int)global_clock);
    }

    /* internal control of global clock */
    else
        global_clock = exec_sync_get(sync_data_nullptr);

    /* operate delta mode if necessary (but only when event mode is active, e.g.,
     * not right after init) */
    /* note that delta mode cannot be supported for realtime simulation */
    global_deltaclock = 0;

    /* Update the "double-precision" clock (usually for deltamode) for consistency
     */
    global_delta_curr_clock = (double)global_clock;

    /* determine whether any modules seek delta mode */
    DELTAMODEFLAGS flags = DMF_NONE;
    DT delta_dt = delta_modedesired(&flags);
    TIMESTAMP t = TS_NEVER;
    output_debug("delta_dt is %d", (int)delta_dt);
    switch (delta_dt)
    {
    case DT_INFINITY: /* no dt -> event mode */
        global_simulation_mode = SM_EVENT;
        t = TS_NEVER;
        break;
    case DT_INVALID: /* error dt  */
        global_simulation_mode = SM_ERROR;
        t = TS_INVALID;
        break; /* simulation mode error */
    default:   /* valid dt */
        if (global_minimum_timestep > 1)
        {
            global_simulation_mode = SM_ERROR;
            output_error("minimum_timestep must be 1 second to operate in deltamode");
            t = TS_INVALID;
            break;
        }
        else
        {
            if (delta_dt == 0) /* Delta mode now */
            {
                global_simulation_mode = SM_DELTA;
                t = global_clock;
            }
            else /* Normal sync - get us to delta point */
            {
                global_simulation_mode = SM_EVENT;
                t = global_clock + delta_dt;
            }
        }
        break;
    }
    if (global_simulation_mode == SM_ERROR)
    {
        output_error("a simulation mode error has occurred");
        return false; /* terminate main loop immediately */
    }

    exec_sync_set(sync_data_nullptr, t, false);

    /* synchronize all internal schedules */
    if (global_clock < 0)
        throw_exception("clock time is negative (global_clock=%lli)", global_clock);
    else if (global_debug_output)
    {
        char dt[64] = "(invalid)";
        convert_from_timestamp(global_clock, dt, sizeof(dt));
        output_debug("global_clock -> %s\n", dt);
    }
    /* set time context */
    output_set_time_context(global_clock);

    /* reset for a new sync event */
    exec_sync_reset(sync_data_nullptr);

    /* account for stoptime only if global clock is not already at stoptime */
    if (global_clock <= global_stoptime && global_stoptime != TS_NEVER)
        exec_sync_set(sync_data_nullptr, global_stoptime + 1, false);

    /* synchronize all internal schedules */
    internal_synctime = syncall_internals(global_clock);
    if (internal_synctime != TS_NEVER &&
        absolute_timestamp(internal_synctime) < global_clock)
    {
        // must be able to force reiterations for m/s mode.
        THROW("internal property sync failure");
        /* TROUBLESHOOT
                An internal property such as schedule, enduse or loadshape has
           failed to synchronize and the simulation aborted. This message should be
           preceded by a more informative message that explains which element failed
           and why. Follow the troubleshooting recommendations for that message and
           try again.
         */
    }

    exec_sync_set(sync_data_nullptr, internal_synctime, false);
    /* prepare multithreading */

    if (!global_debug_mode)
    {
        for (j = 0; j < thread_data->count; j++)
        {
            thread_data->data[j]->hard_event = 0;
            thread_data->data[j]->step_to = TS_NEVER;
        }
    }
#ifdef _DEBUG
    if (global_clock >= global_runaway_time)
        throw_exception("running clock detected");
#endif

    /* run precommit only on first iteration */
    if (iteration_counter == global_iteration_limit)
    {
        pc_rv = precommit_all(global_clock);
        if (SUCCESS != pc_rv)
        {

            THROW("precommit failure");
        }
    }
    iObjRankList = -1;

    /* scan the ranks of objects for each pass */
    for (pass = 0; ranks[pass] != nullptr; pass++)
    {
        int i;

        /* process object in order of rank using index */
        for (i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
        {
            /* skip empty lists */
            if (ranks[pass]->ordinal[i] == nullptr)
                continue;

            iObjRankList++;

            if (global_debug_mode)
            {
                LISTITEM *item;
                for (item = ranks[pass]->ordinal[i]->first; item != nullptr;
                     item = item->next)
                {
                    OBJECT *obj = static_cast<OBJECT *>(item->data);
                    // @todo change debug so it uses sync API
                    if (exec_debug(main_sync, pass, i, obj) == FAILED)
                    {
                        THROW("debugger quit");
                    }
                }
            }
            else
            {
                // global_threadcount == 1, no multithreading
                if (global_threadcount == 1)
                {
                    for (ptr = ranks[pass]->ordinal[i]->first; ptr != nullptr; ptr = ptr->next)
                    {
                        OBJECT *obj = static_cast<OBJECT *>(ptr->data);
                        clock_t ts = (clock_t)exec_clock();
                        ss_do_object_sync(0, ptr->data);
                    	objs_synctime += (clock_t)exec_clock() - ts;

                        if (obj->valid_to == TS_INVALID)
                        {
                            // Get us out of the loop so others don't exec on bad status
                            break;
                        }
                        /// printf("%d %s %d\n", obj->id, obj->name, obj->rank);
                    }
                    // printf("\n");
                }
                else
                { // implement multithreading
//                    printf("****PASS: %d, RANK: %d, iObjRankList: %d\n", pass, i, iObjRankList);
                    multithread_stuff(iObjRankList, i, n_idx.at(n_cnt)); // Function
                    n_cnt++;
                }
            }
        }

        /* run all non-schedule transforms */
        {
            TIMESTAMP st = transform_syncall(
                global_clock,
                static_cast<TRANSFORMSOURCE>(XS_DOUBLE | XS_COMPLEX | XS_ENDUSE),
                nullptr); // if (abs(t) < t2) t2 = t;
            exec_sync_set(sync_data_nullptr, st, false);
        }
    }
    setTP = false;

    if (!global_debug_mode)
    {
        for (j = 0; j < thread_data->count; j++)
        {
            exec_sync_merge(sync_data_nullptr, thread_data->data[j]);
        }

        /* report progress */
        realtime_run_schedule();
    }

    /* count number of passes */
    passes++;

    /**** LOOPED SLAVE PAUSE HERE ****/
    if (global_multirun_mode == MRM_SLAVE)
    {
        output_debug("step_to = %lli", exec_sync_get(sync_data_nullptr));
        output_debug("exec_start(), slave waiting for looped time signal");
        output_debug("exec_start(), slave received looped time signal (%lli)",
                     exec_sync_get(sync_data_nullptr));
    }

    // TODO: Run tests to ensure correct behavior before deleting.
    // exec_run_syncscripts() causes compilation issues without proper
    // implementation of function for python bindings. Commenting out for now.
    /* run sync scripts, if any */
    /*
    if ( exec_run_syncscripts()!=XC_SUCCESS )
    {
            output_error("sync script(s) failed");
            THROW("script synchronization failure");
    }
    */
    /* check for clock advance (indicating last pass) */
    if (exec_sync_get(sync_data_nullptr) != global_clock &&
        global_simulation_mode == SM_EVENT)
    {
        /* clock update is the very last chance to change the next time */
        exec_clock_update_modules();
        if (exec_sync_get(sync_data_nullptr) > global_clock)
        {
            global_federation_reiteration = false;
            TIMESTAMP commit_time = commit_all(global_clock, exec_sync_get(sync_data_nullptr));
            if (absolute_timestamp(commit_time) <= global_clock)
            {
                // commit cannot force reiterations, and any event where the time is
                // less than the global clock
                //  indicates that the object is reporting a failure
                output_error("model commit failed");
                /* TROUBLESHOOT
                        The commit procedure failed.  This is usually preceded
                        by a more detailed message that explains why it failed.  Follow
                        the guidance for that message and try again.
                 */
                THROW("commit failure");
            }
            else if (absolute_timestamp(commit_time) <
                     exec_sync_get(sync_data_nullptr))
            {
                exec_sync_set(sync_data_nullptr, commit_time, false);
            }
            /* reset iteration count */
            iteration_counter = global_iteration_limit;
            federation_iteration_counter = global_iteration_limit;

            /* count number of timesteps */
            tsteps++;
        }
        else if (exec_sync_get(sync_data_nullptr) == global_clock)
        {
            iteration_counter = global_iteration_limit;
            global_federation_reiteration = true;
            if (--federation_iteration_counter == 0)
            {
                output_error(
                    "federation convergence iteration limit reached at %s (exec)",
                    simtime());
                /* TROUBLESHOOT
                        This indicates that the federation that this gridlab-d model a
                   part of was unable to determine a steady state any time horizon.
                 */
                exec_sync_set(sync_data_nullptr, TS_INVALID, false);
                THROW("convergence failure");
            }
        }
    }

    /* check iteration limit */
    else if (--iteration_counter == 0)
    {
        output_error("convergence iteration limit reached at %s (exec)", simtime());
        /* TROUBLESHOOT
                This indicates that the core's solver was unable to determine
                a steady state for all objects for any time horizon.  Identify
                the object that is causing the convergence problem and contact
                the developer of the module that implements that object's class.
         */
        exec_sync_set(sync_data_nullptr, TS_INVALID, false);
        THROW("convergence failure");
    }

    /* handle delta mode operation */
    if (global_simulation_mode == SM_DELTA &&
        exec_sync_get(sync_data_nullptr) >= global_clock)
    {
        if (handle_delta_mode_operation() == -1)
        {
            return false; // DELTA MODE FAILURE
        }
    }

    /* Check if simulation should continue */
    return (iteration_counter > 0 && exec_sync_isrunning(sync_data_nullptr) &&
            exec_getexitcode() == XC_SUCCESS);
}

/** Main simulation loop function
        This function encapsulates the main simulation loop that was previously
        embedded in exec_start(). It handles all simulation processing including
        realtime control, delta mode, object synchronization, and event
 processing.

        @param threadpool pointer to the thread pool for multithreading
        @param passes reference to pass counter
        @param tsteps reference to timestep counter
        @param j reference to loop variable used for thread data
        @param ptr reference to list item pointer
        @param pc_rv reference to precommit return value
        @param iObjRankList reference to object rank list index
 **/
static void run_main_simulation_loop(int64 &passes,
                                     int64 &tsteps, int &j, LISTITEM *&ptr,
                                     int &pc_rv, int &iObjRankList)
{
    int i = 0;
    /* main loop runs for iteration limit, or when nothing futher occurs (ignoring
     * soft events) */
    while (execute_single_simulation_iteration(passes, tsteps, j, ptr,
                                               pc_rv, iObjRankList))
    {
        continue;
    }
    /* deallocate threadpool */
    delete threadpool;
}

/** Single step simulation function
        This function executes one iteration of the main simulation loop using
 the extracted iteration function to eliminate code duplication.

        @param threadpool pointer to the thread pool for multithreading
        @param passes reference to pass counter
        @param tsteps reference to timestep counter
        @param j reference to loop variable used for thread data
        @param ptr reference to list item pointer
        @param pc_rv reference to precommit return value
        @param iObjRankList reference to object rank list index
 **/
static void run_single_simulation_step(int64 &passes, int64 &tsteps, int &j,
                                       LISTITEM *&ptr, int &pc_rv,
                                       int &iObjRankList)
{
    /* Execute one iteration using the shared iteration function */
    execute_single_simulation_iteration(passes, tsteps, j, ptr, pc_rv, iObjRankList);
}

/** Check if the simulation has been properly initialized
        @return TRUE if simulation is initialized and ready to step, FALSE
 otherwise.
 **/
bool exec_is_initialized(void)
{
    // Check if ranks have been set up - this indicates proper initialization
    return (ranks != nullptr);
}

/** Finalize all objects in the simulation
        This is the public interface to finalize_all() for external use.
        @return STATUS is SUCCESS if finalization completed successfully, FAILED
 otherwise.
 **/
STATUS exec_finalize_all(void) { return finalize_all(); }

/** Execute a single simulation step
        This is the public interface for single-step simulation execution.
        @return STATUS is SUCCESS if the step completed successfully, FAILED
 otherwise.
 **/
STATUS exec_step(void)
{
    // Setup variables needed for the step (similar to exec_start)
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    int64 passes = 0, tsteps = 0;
    int j = 0, pc_rv = 0, iObjRankList = 0;
    LISTITEM *ptr = nullptr;

    STATUS result = SUCCESS;

    // Check if simulation has been properly initialized
    // exec_step should only be used after exec_start has been called or
    // simulation is initialized
    if (ranks == nullptr)
    {
        output_error(
            "exec_step: simulation not properly initialized - ranks not set up");
        delete threadpool;
        return FAILED;
    }

    // Check if we're in a valid state to step
    if (iteration_counter <= 0)
    {
        output_verbose(
            "exec_step: simulation has completed or iteration limit reached");
        delete threadpool;
        return SUCCESS; // Not an error, just nothing to do
    }

    /* main step exception handler */
    TRY
    {
        /* Store the current clock to detect when it advances */
        TIMESTAMP start_clock = global_clock;

        /* Cap the next event time before stepping to avoid overshooting the API
         * step target. */
        if (global_step_time != TS_NEVER)
        {
            TIMESTAMP next_event = exec_sync_get(sync_data_nullptr);
            if (next_event > global_step_time)
            {
                exec_sync_set(sync_data_nullptr, global_step_time, false);
            }
        }

        /* Keep running iterations until the clock advances or simulation should
         * stop */
        while (execute_single_simulation_iteration(passes, tsteps, j,
                                                   ptr, pc_rv, iObjRankList))
        {
            /* Check if the clock has advanced - if so, we've completed one step */
            if (global_clock > start_clock)
            {
                break;
            }

            /* Check if we need to cap the next event time to avoid overshooting step
             * target */
            if (global_step_time != TS_NEVER)
            {
                TIMESTAMP next_event = exec_sync_get(sync_data_nullptr);
                if (next_event > global_step_time)
                {
                    exec_sync_set(sync_data_nullptr, global_step_time, false);
                }
            }
        }
    }

    CATCH(const char *msg)
    {
        output_error("exec_step halted: %s", msg);
        result = FAILED;
    }
    ENDCATCH

    /* deallocate threadpool */
    delete threadpool;

    return result;
}

/** Force all objects to sync to a specific target time
    This is used by the API's step() function to achieve exact timestep
 intervals
    @param target_time The exact time to sync to
    @return STATUS is SUCCESS if the sync succeeded, FAILED otherwise
 **/
STATUS exec_force_sync_to_time(TIMESTAMP target_time)
{
    // Verify simulation is initialized
    if (ranks == nullptr)
    {
        output_error(
            "exec_force_sync_to_time: simulation not properly initialized");
        return FAILED;
    }

    // Store the next natural event time for commit
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    TIMESTAMP next_event = exec_sync_get(sync_data_nullptr);

    // Set global_clock to the target time
    global_clock = target_time;

    // Reset sync state and set next sync time
    exec_sync_reset(sync_data_nullptr);
    exec_sync_set(sync_data_nullptr, target_time, false);

    output_verbose("Forcing sync to exact time %.2f (next natural event at %.2f)",
                   (double)target_time, (double)next_event);

    // Synchronize internal schedules first
    TIMESTAMP internal_synctime = syncall_internals(global_clock);
    if (internal_synctime != TS_NEVER &&
        absolute_timestamp(internal_synctime) < global_clock)
    {
        output_error("exec_force_sync_to_time: internal property sync failure");
        return FAILED;
    }
    exec_sync_set(sync_data_nullptr, internal_synctime, false);

    // Perform sync passes for all objects
    for (int pass = 0; ranks[pass] != nullptr; pass++)
    {
        for (int i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
        {
            if (ranks[pass]->ordinal[i] == nullptr)
                continue;

            for (LISTITEM *item = ranks[pass]->ordinal[i]->first; item != nullptr;
                 item = item->next)
            {
                OBJECT *obj = static_cast<OBJECT *>(item->data);
                TIMESTAMP sync_result = object_sync(obj, target_time, passtype[pass]);

                if (sync_result == TS_INVALID)
                {
                    output_error("exec_force_sync_to_time: object sync failed for %s",
                                 obj->name ? obj->name : "(unnamed)");
                    return FAILED;
                }

                // Update sync with this object's next event time
                exec_sync_set(sync_data_nullptr, sync_result, false);
            }
        }

        // Run transforms for this pass
        TIMESTAMP st = transform_syncall(
            global_clock,
            static_cast<TRANSFORMSOURCE>(XS_DOUBLE | XS_COMPLEX | XS_ENDUSE),
            nullptr);
        exec_sync_set(sync_data_nullptr, st, false);
    }

    // Commit all object states
    TIMESTAMP commit_result = commit_all(global_clock, next_event);
    if (commit_result == TS_INVALID ||
        absolute_timestamp(commit_result) <= global_clock)
    {
        output_error("exec_force_sync_to_time: commit failed");
        return FAILED;
    }

    output_verbose("Successfully forced sync and commit at time %.2f",
                   (double)target_time);
    return SUCCESS;
}

/** This is the main simulation loop
        @return STATUS is SUCCESS if the simulation reached equilibrium,
        and FAILED if a problem was encountered.
 **/
STATUS exec_start()
{
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    int64 passes = 0, tsteps = 0;
    int ptc_rv = 0;                         // unused
    int ptj_rv = 0;                         // unused
    int pc_rv = 0;                          // precommit return value
    STATUS fnl_rv = static_cast<STATUS>(0); // finalize all return value
    time_t started_at = realtime_now();     // for profiler
    int j, k;
    LISTITEM *ptr;
    int incr, iObjRankList;

    if (run_preparation() == FAILED)
    {
        return FAILED;
    }

    /* main loop exception handler */
    TRY
    {
        /* Run the main simulation loop */
        run_main_simulation_loop(passes, tsteps, j, ptr, pc_rv,
                                 iObjRankList);

        /* disable signal handler */
        signal(SIGINT, nullptr);

        /* check end state */
        if (exec_sync_isnever(sync_data_nullptr))
        {
            char buffer[64];
            output_verbose(
                "simulation at steady state at %s",
                convert_from_timestamp(global_clock, buffer, sizeof(buffer))
                    ? buffer
                    : "invalid time");
        }

        /* terminate main loop state control */
        exec_mls_done();
    }
    CATCH(const char *msg)
    {
        output_error("exec halted: %s", msg);
        exec_sync_set(sync_data_nullptr, TS_INVALID, false);
        /* TROUBLESHOOT
                This indicates that the core's solver shut down.  This message
                is usually preceded by more detailed messages.  Follow the guidance
                for those messages and try again.
         */
    }
    ENDCATCH
    output_debug("*** main loop ended at %lli; stoptime=%lli, n_events=%i, "
                 "exitcode=%i ***",
                 exec_sync_get(sync_data_nullptr), global_stoptime,
                 exec_sync_getevents(sync_data_nullptr), exec_getexitcode());
    if (global_multirun_mode == MRM_MASTER)
    {
        instance_master_done(TS_NEVER); // tell everyone to pack up and go home
    }

    // sjin: GetMachineCycleCount
    clock_end = (clock_t)exec_clock();

    fnl_rv = finalize_all();
    if (FAILED == fnl_rv)
    {
        output_error("finalize_all() failed");
    }

    // TODO: Delete scripts after confirming autotest success for C++.
    /* run term scripts, if any */
    // if ( exec_run_termscripts()!=XC_SUCCESS ) // Function not implemented
    //{
    //	output_error("term script(s) failed");
    //	return FAILED;
    //}

    /* deallocate threadpool */
    if (!global_debug_mode)
    {
        thread_data = nullptr;

#ifdef NEVER
        /* wipe out progress report */
        if (!global_keep_progress)
            output_raw(
                "                                                           \r");
#endif
    }

    report_performance_after_run(started_at, passes, tsteps);

    /** Execute a single simulation iteration
            This function executes one iteration of the simulation loop, handling
            realtime control, delta mode, object synchronization, and event
     processing.

            @param threadpool pointer to the thread pool for multithreading
            @param passes reference to pass counter
            @param tsteps reference to timestep counter
            @param j reference to loop variable used for thread data
            @param ptr reference to list item pointer
            @param pc_rv reference to precommit return value
            @param iObjRankList reference to object rank list index
            @return true if simulation should continue, false if it should stop
     **/

    /** Single step simulation function
            This function executes one iteration of the main simulation loop using
     the extracted iteration function to eliminate code duplication.

            @param threadpool pointer to the thread pool for multithreading
            @param passes reference to pass counter
            @param tsteps reference to timestep counter
            @param j reference to loop variable used for thread data
            @param ptr reference to list item pointer
            @param pc_rv reference to precommit return value
            @param iObjRankList reference to object rank list index
     **/

    /** Check if the simulation has been properly initialized
            @return TRUE if simulation is initialized and ready to step, FALSE
     otherwise.
     **/

    /** Finalize all objects in the simulation
            This is the public interface to finalize_all() for external use.
            @return STATUS is SUCCESS if finalization completed successfully,
     FAILED otherwise.
     **/

    /** Execute a single simulation step
            This is the public interface for single-step simulation execution.
            @return STATUS is SUCCESS if the step completed successfully, FAILED
     otherwise.
     **/
    return SUCCESS;
}

STATUS exec_step(int64 *passes, int64 *tsteps)
{
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    int j = 0, pc_rv = 0, iObjRankList = 0;
    LISTITEM *ptr = nullptr;

    // Create local variables for internal use (use provided values or defaults)
    int64 local_passes = (passes != nullptr) ? *passes : 0;
    int64 local_tsteps = (tsteps != nullptr) ? *tsteps : 0;

    STATUS result = SUCCESS;

    // Check if simulation has been properly initialized
    // exec_step should only be used after exec_start has been called or
    // simulation is initialized
    if (ranks == nullptr)
    {
        output_error(
            "exec_step: simulation not properly initialized - ranks not set up");
        delete threadpool;
        return FAILED;
    }

    // Check if we're in a valid state to step
    if (iteration_counter <= 0)
    {
        output_verbose(
            "exec_step: simulation has completed or iteration limit reached");
        delete threadpool;
        return SUCCESS; // Not an error, just nothing to do
    }

    /* main step exception handler */
    TRY
    {
        /* Store the current clock to detect when it advances */
        TIMESTAMP start_clock = global_clock;

        /* Keep running iterations until the clock advances or simulation should
         * stop */
        while (execute_single_simulation_iteration(local_passes, local_tsteps, j, ptr, pc_rv, iObjRankList))
        {
            /* Check if the clock has advanced - if so, we've completed one step */
            if (global_clock > start_clock)
            {
                break;
            }
        }
    }
    CATCH(const char *msg)
    {
        output_error("exec_step halted: %s", msg);
        result = FAILED;
    }
    ENDCATCH

    /* Copy final values back to caller's variables if provided */
    if (passes != nullptr)
        *passes = local_passes;
    if (tsteps != nullptr)
        *tsteps = local_tsteps;

    /* deallocate threadpool */
    delete threadpool;

    return result;
}

/** This is the main simulation loop with optional parameters
        @return STATUS is SUCCESS if the simulation reached equilibrium,
        and FAILED if a problem was encountered.
 **/
STATUS exec_start(int64 *passes, int64 *tsteps)
{
    std::shared_ptr<sync_data> sync_data_nullptr = nullptr;
    int ptc_rv = 0;                         // unused
    int ptj_rv = 0;                         // unused
    int pc_rv = 0;                          // precommit return value
    STATUS fnl_rv = static_cast<STATUS>(0); // finalize all return value
    time_t started_at = realtime_now();     // for profiler
    int j, k;
    LISTITEM *ptr;
    int incr, iObjRankList;
    // Only setup threadpool for each object rank list at the first iteration;
    threadpool = new cpp_threadpool(global_threadcount);


    // Create local variables for internal use (use provided values or defaults)
    int64 local_passes = (passes != nullptr) ? *passes : 0;
    int64 local_tsteps = (tsteps != nullptr) ? *tsteps : 0;
    FILE *prep_dbg = fopen("/tmp/env_check.log", "a");
    fprintf(prep_dbg, "About to call run_preparation()\n");
    fclose(prep_dbg);

    if (run_preparation() == FAILED)
    {
        FILE *prep_fail = fopen("/tmp/env_check.log", "a");
        fprintf(prep_fail, "run_preparation() returned FAILED\n");
        fclose(prep_fail);

        return FAILED;
    }

    /* main loop exception handler */
    TRY
    {

        /* Run the main simulation loop */
        run_main_simulation_loop(local_passes, local_tsteps, j, ptr, pc_rv, iObjRankList);

        /* disable signal handler */
        signal(SIGINT, nullptr);

        /* check end state */
        if (exec_sync_isnever(sync_data_nullptr))
        {
            char buffer[64];
            output_verbose(
                "simulation at steady state at %s",
                convert_from_timestamp(global_clock, buffer, sizeof(buffer))
                    ? buffer
                    : "invalid time");
        }

        /* terminate main loop state control */
        exec_mls_done();
    }
    CATCH(const char *msg)
    {
        output_error("exec halted: %s", msg);
        exec_sync_set(sync_data_nullptr, TS_INVALID, false);
        /* TROUBLESHOOT
                This indicates that the core's solver shut down.  This message
                is usually preceded by more detailed messages.  Follow the guidance
                for those messages and try again.
         */
    }
    ENDCATCH
    output_debug("*** main loop ended at %lli; stoptime=%lli, n_events=%i, "
                 "exitcode=%i ***",
                 exec_sync_get(sync_data_nullptr), global_stoptime,
                 exec_sync_getevents(sync_data_nullptr), exec_getexitcode());
    if (global_multirun_mode == MRM_MASTER)
    {
        instance_master_done(TS_NEVER); // tell everyone to pack up and go home
    }

    // sjin: GetMachineCycleCount
    clock_end = (clock_t)exec_clock();

    fnl_rv = finalize_all();
    if (FAILED == fnl_rv)
    {
        output_error("finalize_all() failed");
    }

    /* run term scripts, if any */
    // if (exec_run_termscripts() != XC_SUCCESS)
    // {
    // 	output_error("term script(s) failed");
    // 	return FAILED;
    // }

    /* deallocate threadpool */
    if (!global_debug_mode)
    {
        // free(thread_data);
        thread_data = nullptr;

#ifdef NEVER
        /* wipe out progress report */
        if (!global_keep_progress)
            output_raw(
                "                                                           \r");
#endif
    }

    // Copy final values back to caller's variables if provided
    if (passes != nullptr)
        *passes = local_passes;
    if (tsteps != nullptr)
        *tsteps = local_tsteps;

    report_performance_after_run(started_at, local_passes, local_tsteps);

    sched_update(global_clock, MLS_DONE);

    /* terminate links */
    STATUS final_status = exec_sync_getstatus(sync_data_nullptr);
    FILE *status_dbg = fopen("/tmp/env_check.log", "a");
    fprintf(status_dbg, "exec_start returning status=%d\n", final_status);
    fclose(status_dbg);
    // delete threadpool;
    return final_status;
}

/** Starts the executive test loop
        @return STATUS is SUCCESS if all test passed, FAILED is any test failed.
 **/
STATUS exec_test(struct sync_data *data, /**< the synchronization state data */
                 int pass,               /**< the pass number */
                 OBJECT *obj)            /**< the current object */
{
    TIMESTAMP this_t;
    /* check in and out-of-service dates */
    if (global_clock < obj->in_svc)
        this_t = obj->in_svc; /* yet to go in service */
    else if ((global_clock == obj->in_svc) && (obj->in_svc_micro != 0))
        this_t = obj->in_svc +
                 1; /* Round up for service (deltamode handled separately) */
    else if (global_clock <= obj->out_svc)
        this_t = object_sync(obj, global_clock, pass);
    else
        this_t = TS_NEVER; /* already out of service */

    /* check for "soft" event (events that are ignored when stopping) */
    if (this_t < -1)
        this_t = -this_t;
    else if (this_t != TS_NEVER)
        data->hard_event++; /* this counts the number of hard events */

    /* check for stopped clock */
    if (this_t < global_clock)
    {
        char b[64];
        output_error("%s: object %s stopped its clock! (test)", simtime(),
                     object_name(obj, b, 63));
        /* TROUBLESHOOT
                This indicates that one of the objects in the simulator has
           encountered a state where it cannot calculate the time to the next state.
           This usually is caused by a bug in the module that implements that
           object's class.
         */
        data->status = FAILED;
    }
    else
    {
        /* check for iteration limit approach */
        if (iteration_counter == 2 && this_t == global_clock)
        {
            char b[64];
            output_verbose("%s: object %s iteration limit imminent", simtime(),
                           object_name(obj, b, 63));
        }
        else if (iteration_counter == 1 && this_t == global_clock)
        {
            output_error(
                "convergence iteration limit reached for object %s:%d (test)",
                obj->oclass->name, obj->id);
            /* TROUBLESHOOT
                    This indicates that one of the objects in the simulator has
               encountered a state where it cannot calculate the time to the next
               state.  This usually is caused by a bug in the module that implements
               that object's class.
             */
        }

        /* manage minimum timestep */
        if (global_minimum_timestep > 1 && this_t > global_clock &&
            this_t < TS_NEVER)
            this_t =
                ((this_t / global_minimum_timestep) + 1) * global_minimum_timestep;

        /* if this event precedes next step, next step is now this event */
        if (data->step_to > this_t)
            data->step_to = this_t;
        data->status = SUCCESS;
    }
    return data->status;
}

