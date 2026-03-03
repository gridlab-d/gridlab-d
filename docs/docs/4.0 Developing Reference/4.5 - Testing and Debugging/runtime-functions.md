# Runtime/gl_* Output  Functions

Sometimes, the model doesn’t work, and extra output is needed, since “domain error” is often the only output message and it doesn’t explain very much. At others time, something just breaks, or the values within the model are needed for reality checking a prototype. To print something to the screen, there are a series of callbacks reaching into output.c. All of the following functions behave much in the same way that printf() does, with variable arguments being concatenated into a null-terminated string.


## Messages
All these message functions use printf formatting:

* **int gl_output(char *msg, ...)**
Send a message to the normal output stream. Matches `output_message()`. It will print an informative statement to `stdout`.

* **int gl_warning(char *msg, ...)**
Send a message to the warning output stream. Matches `output_warning()`. Prefaces a warning message with "WARNING:" and prints it to `stderr` if `--warn` is enabled, or if the global "warn" is nonzero. Used for alerting the user that a state is outside its "intended" range, though the results should still be reasonable.

* **int gl_error(char *msg, ...)** Matches `output_error()`. Prefaces a formatted string with "ERROR:" and prints it to `stderr`. Indicates a situation where the simulation is in an inconsistent state and will not produce valid answers, but can continue processing. The model frequently diverges shortly after an error message in the existing modules. The --quiet flag suppresses this stream.
Send a message to the error output stream.

* **int gl_debug(char *msg, ...)**
Send a message to the debug output stream. Matches `output_debug()`. Prints a string prefaced with "DEBUG:" if `--debug` or `--debugger` are enabled. Using this function is only recommended while running the build-in GridLAB-D debugger.

* **int gl_verbose(char *msg, ...)**
Send a message to the verbose output stream. Matches `output_verbose()`. Used for printing extraneous information when the `--verbose` flag is enabled. Verbose output displays a great deal of data about the inner working of the core in practice.

* **int gl_testmsg(char *msg, ...)**
Send a message to the test output stream. Matches `output_test()`. This function opens a file with the string in the global `testoutputfile` , which is "test.txt" by default, prints a header, and prints any test output to that file. Primarily used with test and check functions to ensure the accuracy of the test or model results.

## Property formatting

* **char *gl_name(object obj)**
Construct a string containing a name for the object.

          
* **char *gl_strftime(timestamp ts)**
Convert a timestamp to a string using a static buffer.

## Property access

* **char *gl_global_getvar(char *name)**
Get the string value of a global variable.

* **gl_get_value(object obj, char *name, T &value)**
Get the value of a property of type T.

* **void *gl_get_addr(object obj, char *name)**
Get a pointer to the property of an object.

* **FUNCTIONADDR gl_get_function(CLASS *oclass, char *fname)**
Get a pointer to a class function.

* **int gl_set_parent(OBJECT *obj, OBJECT *parent)**
Set the parent of an object.

## Exception handling

* **void gl_throw(char *msg, ...)**
Throw an exception with a printf style message (which stops the simulation)

## Random numbers

* **gl_random_D(double a, ...)**
Get a random number of distribution D.

## Time

* **timestamp gl_globalclock**
Get the value of the simulation's global clock.

* **DATETIME *gl_localtime(timestamp ts, DATETIME *dt)**
Convert a timestamp to date/time structure.

* **timestamp gl_timestamp(double t)**
Convert a double (in seconds from 1/1/1970) to a timestamp.

* **double gl_seconds(timestamp ts)**
Convert a timestamp into seconds (from 1/1/1970).

* **double gl_minutes(timestamp ts)**
Convert a timestamp into minutes (from 1/1/1970).

* **double gl_hours(timestamp ts)**
Convert a timestamp into hours (from 1/1/1970).

* **double gl_days(timestamp ts)**
Convert a timestamp into days (from 1/1/1970).

* **double gl_weeks(timestamp ts)**
Convert a timestamp into weeks (from 1/1/1970).

## Structures

### Modules
```
typedef struct {
	char name[1024];	// name of module
	CLASS *oclass;		// list of classes implemented by module
	unsigned short major;	// major version number
	unsigned short minor;	// minor version number
	PROPERTY *globals;	// list of globals used by module
} MODULE;
```

### Classes
```
typedef struct {
	char name[64];	// class name
	MODULE *module;	// module that implement the class
	PROPERTY *pmap;	// list of properties
	FUNCTION *fmap;	// list of functions
	CLASS *parent;	// class parent
	CLASS *next;	// next class in list
} CLASS;
```

### Objects
```
typedef struct {
	unsigned int id;	// id (unique)
	CLASS *oclass;	// class
	object next;	// next object in list
	object parent;	// parent
	unsigned int rank;	// rank
	timestamp clock;	// clock
	double latitude, longitude; // geocoordinates
	timestamp in_svc, out_svc;	// service times
	char *name; // name of object
} OBJECT; 
```

### Properties
```
typedef struct {
	char name[64];	// name
	long ptype;	// type (see PT_* enum, e.g., PT_bool)
	unsigned long size;	// array size
	long access;	// access flags (see PA_* enum, e.g., PA_PUBLIC)
	UNIT *unit;	// units if any, or NULL
	void *addr;	// property location (header offset for objects, address for globals) */
	KEYWORD *keywords;	// list of valid keywords
	struct s_property_map *next;	// next property in property list 
	PROPERTYFLAGS flags;	// property flags (e.g., PF_RECALC) 
} PROPERTY;
```

### Functions
This structure defines a function published by a class.
```
typedef struct {
	char name[64];	// name of function
	int64 (*addr)(void*, ...);	// entry of function
	FUNCTION *next;	// next function in list 
} FUNCTION;
Units

typedef struct {
	char name[64];	// name of unit
	double c,e,h,k,m,s,a,b;	// unit parameters
	int prec;	// precision of unit
	struct s_unit *next;	// the next unit is the unit list
} UNIT;
```