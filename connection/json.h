/** $Id$

 Object class header

 **/

#ifndef _JSON_H
#define _JSON_H

#include "gridlabd.h"
#include "native.h"

size_t convert_to_hex(char *hex, size_t hexlen, void *buffer, size_t buflen);
size_t convert_from_hex(void *buffer, size_t buflen, char *hex, size_t hexlen);

typedef enum {
  JT_VOID,
  JT_LIST,
  JT_REAL,
  JT_INTEGER,
  JT_STRING,
} JSONTYPE;
typedef struct _jsonlist {
  JSONTYPE type;
  char tag[32];
  union {
    double real;
    int64 integer;
    struct _jsonlist *list;
  };
  char string[1024];
  struct _jsonlist *parent; // parent list (NULL for head)
  struct _jsonlist *next;
} JSONLIST;

class json : public native {
public:
  // GL_ATOMIC(double,version);
  //  TODO add published properties here

private:
  // TODO add other properties here

protected:
  double version; // Member variable of type `double`.

public:
  static inline json *get_defaults() {
    if (!defaults) {
      defaults = new json(); // Initialize lazily
    }
    return defaults;
  }

  json() {}
  ~json() {
    if (defaults)
      delete defaults;
  }

public:
  // Static inline method to get the byte offset of the member `version`.
  static inline size_t get_version_offset(void) {
    json *current_defaults = get_defaults();
    return reinterpret_cast<const char *>(&(current_defaults->version)) -
           reinterpret_cast<const char *>(current_defaults);
  }

  // Inline function to get the value of `version`.
  inline double get_version(void) { return version; }

  // Inline method to return a gld_property object for `version`.
  inline gld_property get_version_property(void) {
    return gld_property(my(), std::string("version").c_str());
  }

  // Inline method to set the value of `version`.
  inline void set_version(double p) { version = p; }

  // Inline method to get the string representation of the `version` property.
  inline gld_string get_version_string(void) {
    return get_version_property().get_string();
  }

  // Inline method to set the `version` property from a provided string.
  inline void set_version(char *str) {
    get_version_property().from_string(str);
  }

public:
  // required implementations
  json(MODULE *);
  int create(void);
  int init(OBJECT *);
  int precommit(TIMESTAMP);
  TIMESTAMP presync(TIMESTAMP);
  TIMESTAMP sync(TIMESTAMP);
  TIMESTAMP postsync(TIMESTAMP);
  TIMESTAMP commit(TIMESTAMP, TIMESTAMP);
  int prenotify(PROPERTY *, char *);
  int postnotify(PROPERTY *, char *);
  int finalize(void);
  TIMESTAMP plc(TIMESTAMP);
  int link(char *value);
  int option(char *value);
  void term(TIMESTAMP);
  // TODO add other event handlers here

public:
  static JSONLIST *parse(char *buffer);
  static JSONLIST *find(JSONLIST *list, const char *tag);
  static char *get(JSONLIST *list, const char *tag);
  static void destroy(JSONLIST *list);

public:
  // special variables for GridLAB-D classes
  static CLASS *oclass;
  static json *defaults;
};

#endif // _JSON_H
