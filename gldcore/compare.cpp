/** $Id
 **/

#include <cmath>
#include <type_traits>

#include "compare.h"
#include "object.h"
#include "property.h"
#include "timestamp.h"

#define COMPAREOPI(T) COMPARE_EQI(T) COMPARE_LEI(T) COMPARE_GEI(T) COMPARE_NEI(T) COMPARE_LTI(T) COMPARE_GTI(T) COMPARE_INI(T) COMPARE_NII(T)
// Integer property storage is unsigned in this table, including signed PT_int*.
// Interpret its bits as a signed integer of the SAME width (not just `signed int`).
#define COMPARE_SIGNED(T,X) (static_cast<typename std::make_signed<T>::type>(*(T*)(X)))
#define COMPARE_EQI(T) int compare_tc_##T##_eq(void* x,void* a,void* b) { return COMPARE_SIGNED(T,x)==COMPARE_SIGNED(T,a); }
#define COMPARE_LEI(T) int compare_tc_##T##_le(void* x,void* a,void* b) { return COMPARE_SIGNED(T,x)<=COMPARE_SIGNED(T,a); }
#define COMPARE_GEI(T) int compare_tc_##T##_ge(void* x,void* a,void* b) { return COMPARE_SIGNED(T,x)>=COMPARE_SIGNED(T,a); }
#define COMPARE_NEI(T) int compare_tc_##T##_ne(void* x,void* a,void* b) { return COMPARE_SIGNED(T,x)!=COMPARE_SIGNED(T,a); }
#define COMPARE_LTI(T) int compare_tc_##T##_lt(void* x,void* a,void* b) { return COMPARE_SIGNED(T,x)<COMPARE_SIGNED(T,a); }
#define COMPARE_GTI(T) int compare_tc_##T##_gt(void* x,void* a,void* b) { return COMPARE_SIGNED(T,x)>COMPARE_SIGNED(T,a); }
#define COMPARE_INI(T) int compare_tc_##T##_in(void* x,void* a,void* b) { return b!=nullptr && COMPARE_SIGNED(T,a)<=COMPARE_SIGNED(T,x) && COMPARE_SIGNED(T,x)<=COMPARE_SIGNED(T,b); }
#define COMPARE_NII(T) int compare_tc_##T##_ni(void* x,void* a,void* b) { return b==nullptr || !(COMPARE_SIGNED(T,a)<=COMPARE_SIGNED(T,x) && COMPARE_SIGNED(T,x)<=COMPARE_SIGNED(T,b)); }

// PT_set stores an unsigned bitmask. Its comparisons must not reinterpret the
// highest set bit as a negative number. Signed PT_int* and PT_timestamp keep
// their existing signed-ordering semantics.
#define COMPAREOPU(T,U) \
int compare_tc_##T##_eq(void* x,void* a,void*) { return *(U*)x==*(U*)a; } \
int compare_tc_##T##_le(void* x,void* a,void*) { return *(U*)x<=*(U*)a; } \
int compare_tc_##T##_ge(void* x,void* a,void*) { return *(U*)x>=*(U*)a; } \
int compare_tc_##T##_ne(void* x,void* a,void*) { return *(U*)x!=*(U*)a; } \
int compare_tc_##T##_lt(void* x,void* a,void*) { return *(U*)x<*(U*)a; } \
int compare_tc_##T##_gt(void* x,void* a,void*) { return *(U*)x>*(U*)a; } \
int compare_tc_##T##_in(void* x,void* a,void* b) { return b!=nullptr && *(U*)a<=*(U*)x && *(U*)x<=*(U*)b; } \
int compare_tc_##T##_ni(void* x,void* a,void* b) { return b==nullptr || !(*(U*)a<=*(U*)x && *(U*)x<=*(U*)b); }

#define COMPAREOPB(T) COMPARE_EQB(T) COMPARE_NEB(T)
#define COMPARE_EQB(T)                                                         \
  int compare_tc_##T##_eq(void *x, void *a, void *b) {                         \
    return *(T *)x == *(T *)a;                                                 \
  }
#define COMPARE_NEB(T)                                                         \
  int compare_tc_##T##_ne(void *x, void *a, void *b) {                         \
    return *(T *)x != *(T *)a;                                                 \
  }

#define COMPAREOPO(T) COMPARE_EQO(T) COMPARE_NEO(T)
#define COMPARE_EQO(T)                                                         \
  int compare_tc_##T##_eq(void *x, void *a, void *b) {                         \
    return strcmp((*(OBJECT **)x)->name, (*(OBJECT **)a)->name) == 0;          \
  }
#define COMPARE_NEO(T)                                                         \
  int compare_tc_##T##_ne(void *x, void *a, void *b) {                         \
    return strcmp((*(OBJECT **)x)->name, (*(OBJECT **)a)->name) != 0;          \
  }

#define COMPAREOPF(T)                                                          \
  COMPARE_EQF(T)                                                               \
  COMPARE_LEF(T) COMPARE_GEF(T) COMPARE_NEF(T) COMPARE_LTF(T) COMPARE_GTF(T)   \
      COMPARE_INF(T) COMPARE_NIF(T)
#define COMPARE_EQF(T)                                                         \
  int compare_tc_##T##_eq(void *x, void *a, void *b) {                         \
    if (b == nullptr)                                                          \
      return *(T *)x == *(T *)a;                                               \
    else                                                                       \
      return fabs((*(T *)x) - (*(T *)a)) <= (*(T *)b);                         \
  }
#define COMPARE_LEF(T)                                                         \
  int compare_tc_##T##_le(void *x, void *a, void *b) {                         \
    return *(T *)x <= *(T *)a;                                                 \
  }
#define COMPARE_GEF(T)                                                         \
  int compare_tc_##T##_ge(void *x, void *a, void *b) {                         \
    return *(T *)x >= *(T *)a;                                                 \
  }
#define COMPARE_NEF(T)                                                         \
  int compare_tc_##T##_ne(void *x, void *a, void *b) {                         \
    if (b == nullptr)                                                          \
      return *(T *)x != *(T *)a;                                               \
    else                                                                       \
      return fabs((*(T *)x) - (*(T *)a)) > (*(T *)b);                          \
  }
#define COMPARE_LTF(T)                                                         \
  int compare_tc_##T##_lt(void *x, void *a, void *b) {                         \
    return *(T *)x < *(T *)a;                                                  \
  }
#define COMPARE_GTF(T)                                                         \
  int compare_tc_##T##_gt(void *x, void *a, void *b) {                         \
    return *(T *)x > *(T *)a;                                                  \
  }
#define COMPARE_INF(T)                                                         \
  int compare_tc_##T##_in(void *x, void *a, void *b) {                         \
    return *(T *)a <= *(T *)x && b != nullptr && *(T *)x <= *(T *)b;           \
  }
#define COMPARE_NIF(T)                                                         \
  int compare_tc_##T##_ni(void *x, void *a, void *b) {                         \
    return !(*(T *)a <= *(T *)x && b != nullptr && *(T *)x <= *(T *)b);        \
  }

#define COMPAREOPS(T)                                                          \
  COMPARE_SEQ(T)                                                               \
  COMPARE_SLE(T) COMPARE_SGE(T) COMPARE_SNE(T) COMPARE_SLT(T) COMPARE_SGT(T)   \
      COMPARE_SIN(T) COMPARE_SNI(T)
#define COMPARE_SEQ(T)                                                         \
  int compare_tc_##T##_eq(void *x, void *a, void *b) {                         \
    return strcmp((char *)x, (char *)a) == 0;                                  \
  }
#define COMPARE_SLE(T)                                                         \
  int compare_tc_##T##_le(void *x, void *a, void *b) {                         \
    return strcmp((char *)x, (char *)a) <= 0;                                  \
  }
#define COMPARE_SGE(T)                                                         \
  int compare_tc_##T##_ge(void *x, void *a, void *b) {                         \
    return strcmp((char *)x, (char *)a) >= 0;                                  \
  }
#define COMPARE_SNE(T)                                                         \
  int compare_tc_##T##_ne(void *x, void *a, void *b) {                         \
    return strcmp((char *)x, (char *)a) != 0;                                  \
  }
#define COMPARE_SLT(T)                                                         \
  int compare_tc_##T##_lt(void *x, void *a, void *b) {                         \
    return strcmp((char *)x, (char *)a) < 0;                                   \
  }
#define COMPARE_SGT(T)                                                         \
  int compare_tc_##T##_gt(void *x, void *a, void *b) {                         \
    return strcmp((char *)x, (char *)a) > 0;                                   \
  }
#define COMPARE_SIN(T)                                                         \
  int compare_tc_##T##_in(void *x, void *a, void *b) {                         \
    return strcmp((char *)x, (char *)a) >= 0 && b != nullptr &&                \
           strcmp((char *)x, (char *)b) <= 0;                                  \
  }
#define COMPARE_SNI(T)                                                         \
  int compare_tc_##T##_ni(void *x, void *a, void *b) {                         \
    return !(strcmp((char *)x, (char *)a) >= 0 && b != nullptr &&              \
             strcmp((char *)x, (char *)b) <= 0);                               \
  }

/* basic ops */
COMPAREOPF(double)
COMPAREOPF(float)
COMPAREOPI(uint16)
COMPAREOPI(uint32)
COMPAREOPI(uint64)
COMPAREOPU(set,uint64)
COMPAREOPU(u32,uint32)
COMPAREOPB(bool)
COMPAREOPS(string)
COMPAREOPO(object)
