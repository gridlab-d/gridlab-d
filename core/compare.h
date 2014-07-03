/* $id */

#ifndef _COMPARE_H

#define TCNONE \
	{TCOP_NOP,"",NULL},\
	{TCOP_NOP,"",NULL},\
	{TCOP_NOP,"",NULL},\
	{TCOP_NOP,"",NULL},\
	{TCOP_NOP,"",NULL},\
	{TCOP_NOP,"",NULL},\
	{TCOP_NOP,"",NULL},\
	{TCOP_NOP,"",NULL}
#define TCOPB(X) \
	{TCOP_EQ,"==",compare_tc_##X##_eq}, \
	{TCOP_NOP,"",NULL}, \
	{TCOP_NOP,"",NULL}, \
	{TCOP_NE,"!=",compare_tc_##X##_ne}, \
	{TCOP_NOP,"",NULL}, \
	{TCOP_NOP,"",NULL}, \
	{TCOP_NOP,"",NULL}, \
	{TCOP_NOP,"",NULL}
#define TCOPS(X) \
	{TCOP_EQ,"==",compare_tc_##X##_eq}, \
	{TCOP_LE,"<=",compare_tc_##X##_le}, \
	{TCOP_GE,">=",compare_tc_##X##_ge}, \
	{TCOP_NE,"!=",compare_tc_##X##_ne}, \
	{TCOP_LT,"<",compare_tc_##X##_lt}, \
	{TCOP_GT,">",compare_tc_##X##_gt}, \
	{TCOP_IN,"inside",compare_tc_##X##_in,1}, \
	{TCOP_NI,"outside",compare_tc_##X##_ni,1}
#define TCOPD(X) \
	int compare_tc_##X##_eq(void*,void*,void*); \
	int compare_tc_##X##_le(void*,void*,void*); \
	int compare_tc_##X##_ge(void*,void*,void*); \
	int compare_tc_##X##_ne(void*,void*,void*); \
	int compare_tc_##X##_lt(void*,void*,void*); \
	int compare_tc_##X##_gt(void*,void*,void*); \
	int compare_tc_##X##_in(void*,void*,void*); \
	int compare_tc_##X##_ni(void*,void*,void*); 

TCOPD(double);
TCOPD(float);
TCOPD(uint16);
TCOPD(uint32);
TCOPD(uint64);
TCOPD(string);
TCOPD(bool);
TCOPD(timestamp);
TCOPD(object);

#endif
