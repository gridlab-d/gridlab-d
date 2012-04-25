/** $Id
 **/

#include "timestamp.h"
#include "property.h"
#include "compare.h"

#define COMPAREOPS(T) COMPARE_EQ(T) COMPARE_LE(T) COMPARE_GE(T) COMPARE_NE(T) COMPARE_LT(T) COMPARE_GT(T) COMPARE_IN(T) COMPARE_NI(T)
#define COMPARE_EQ(T) int compare_tc_##T##_eq(void* x,void* a,void* b) { return *(T*)x==*(T*)a; }
#define COMPARE_LE(T) int compare_tc_##T##_le(void* x,void* a,void* b) { return (signed)*(T*)x<=(signed)*(T*)a; }
#define COMPARE_GE(T) int compare_tc_##T##_ge(void* x,void* a,void* b) { return (signed)*(T*)x>=(signed)*(T*)a; }
#define COMPARE_NE(T) int compare_tc_##T##_ne(void* x,void* a,void* b) { return *(T*)x!=*(T*)a; }
#define COMPARE_LT(T) int compare_tc_##T##_lt(void* x,void* a,void* b) { return (signed)*(T*)x<(signed)*(T*)a; }
#define COMPARE_GT(T) int compare_tc_##T##_gt(void* x,void* a,void* b) { return (signed)*(T*)x>(signed)*(T*)a; }
#define COMPARE_IN(T) int compare_tc_##T##_in(void* x,void* a,void* b) { return (signed)*(T*)a<(signed)*(T*)x && (signed)*(T*)x<(signed)*(T*)b; }
#define COMPARE_NI(T) int compare_tc_##T##_ni(void* x,void* a,void* b) { return !((signed)*(T*)a<(signed)*(T*)x && (signed)*(T*)x<(signed)*(T*)b); }

#define COMPARESOPS(T) COMPARE_SEQ(T) COMPARE_SLE(T) COMPARE_SGE(T) COMPARE_SNE(T) COMPARE_SLT(T) COMPARE_SGT(T) COMPARE_SIN(T) COMPARE_SNI(T)
#define COMPARE_SEQ(T) int compare_tc_##T##_eq(void* x,void* a,void* b) { return strcmp((char*)x,(char*)a)==0; }
#define COMPARE_SLE(T) int compare_tc_##T##_le(void* x,void* a,void* b) { return strcmp((char*)x,(char*)a)!=1; }
#define COMPARE_SGE(T) int compare_tc_##T##_ge(void* x,void* a,void* b) { return strcmp((char*)x,(char*)a)!=-1; }
#define COMPARE_SNE(T) int compare_tc_##T##_ne(void* x,void* a,void* b) { return strcmp((char*)x,(char*)a)!=0; }
#define COMPARE_SLT(T) int compare_tc_##T##_lt(void* x,void* a,void* b) { return strcmp((char*)x,(char*)a)==-1; }
#define COMPARE_SGT(T) int compare_tc_##T##_gt(void* x,void* a,void* b) { return strcmp((char*)x,(char*)a)==1; }
#define COMPARE_SIN(T) int compare_tc_##T##_in(void* x,void* a,void* b) { return strcmp((char*)x,(char*)a)==1 && strcmp((char*)x,(char*)b)==-1; }
#define COMPARE_SNI(T) int compare_tc_##T##_ni(void* x,void* a,void* b) { return !(strcmp((char*)x,(char*)a)==1 && strcmp((char*)x,(char*)b)==-1); }

/* basic ops */
COMPAREOPS(double)
COMPAREOPS(float)
COMPAREOPS(uint16)
COMPAREOPS(uint32)
COMPAREOPS(uint64)
COMPAREOPS(bool)
COMPARESOPS(string)

