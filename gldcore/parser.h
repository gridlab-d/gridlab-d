/*
 *  Created on: Aug 15, 2025
 *      Author: d3j331 - Mitch Pelton, Andy Fisher
 */

#ifndef _PARSER_H_
#define _PARSER_H_


#include "globals.h"
#include "module.h"
#include "load.h"

#include <string>


using namespace std;

#ifdef __cplusplus

class parser {

private:
	char filename[1024];
	unsigned int linenum=1;

#define PARSER char *_p
#define START int _mm=0, _m=0, _n=0, _l=linenum;
#define ACCEPT { _n+=_m; _p+=_m; _m=0; }
#define HERE (_p+_m)
#define OR {_m=0;}
#define REJECT { linenum=_l; return 0; }
#define WHITE (TERM(white(HERE)))
#define LITERAL(X) (_mm=literal(HERE,(const_cast<char*>(X))),_m+=_mm,_mm>0)
#define TERM(X) (_mm=(X),_m+=_mm,_mm>0)
#define COPY(X) {size--; (X)[_n++]=*_p++;}
#define DONE return _n;
#define BEGIN_REPEAT {char *__p=_p; int __mm=_mm, __m=_m, __n=_n, __l=_l; int __ln=linenum;
#define REPEAT _p=__p;_m=__m; _mm=__mm; _n=__n; _l=__l; linenum=__ln;
#define END_REPEAT }

    //string filename;

public:

	int findLastIndex(string str, char x);
    int replaceAll(string& s, string const& toReplace, string const& replaceWith);
	string extractBetween(string str, char startChar, char endChar);
	string extractBetweenEnd(string str, char startChar, char endChar);
	void forward_slashes(string& str);
	void filename_parts(string filename, string& path, string& name, string& ext);
    
	void syntax_error(PARSER);
	int white(PARSER);
	int comment(PARSER);
	int pattern(PARSER, const char *pattern, char *result, int size);
	int scan(PARSER, char *format, char *result, int size);
	int literal(PARSER, char *text);
	int dashed_name(PARSER, char *result, int size);
	int name(PARSER, char *result, int size);
	int namelist(PARSER, char *result, int size);
	int variable_list(PARSER, char *result, int size);
	int property_list(PARSER, char *result, int size);
	int unitspec(PARSER, UNIT **unit);
	int unitsuffix(PARSER, UNIT **unit);
	int nameunit(PARSER, char *result, int size,UNIT **unit);
	int dotted_name(PARSER, char *result, int size);
	int hostname(PARSER, char *result, int size);
	int delim_value(PARSER, char *result, int size, const char *delims);
	int structured_value(PARSER, char *result, int size);
	int value(PARSER, char *result, int size);
	#if 0
	int functional_int(PARSER, int64 *value);
	#endif
	int integer(PARSER, int64 *value);
	int integer32(PARSER, int32 *value);
	int integer16(PARSER, int16 *value);
	int real_value(PARSER, double *value);
	int functional(PARSER, double *pValue);

	struct s_rpn {
		int op;
		double val; // if op = 0, check val
	};

	struct s_rpn_func {
		const char *name;
		int args; /* use a mode instead? else assume only doubles */
		int index;
		double (*fptr)(double);
		/* fptr? for now, just to recognize */
	} rpn_map[12] = {
		{"sin", 1, -1, sin},
		{"cos", 1, -2, cos},
		{"tan", 1, -3, tan},
		{"abs", 1, -4, fabs},
		{"sqrt", 1, -5, sqrt},
		{"acos", 1, -6, acos},
		{"asin", 1, -7, asin},
		{"atan", 1, -8, atan},
	//	{"atan2", 2},	/* only one with two inputs? */
		{"log", 1, -10, log},
		{"log10", 1, -11, log10},
		{"floor", 1, -12, floor},
		{"ceil", 1, -13, ceil}
	};

	int rpnfunc(PARSER, int *val);

	#define OP_END 0
	#define OP_OPEN 1
	#define OP_CLOSE 2
	#define OP_POW 3
	#define OP_MULT 4
	#define OP_MOD 5
	#define OP_DIV 6
	#define OP_ADD 7
	#define OP_SUB 8
	#define OP_SIN -1
	#define OP_COS -2
	#define OP_TAN -3
	#define OP_ABS -4

	int op_prec[9] = {0, 0, 0, 3, 2, 2, 2, 1, 1};

	#define PASS_OP(T) \
		while(op_prec[(T)] <= op_prec[op_stk[op_i]]){	\
			rpn_stk[rpn_i].op = op_stk[op_i];			\
			rpn_stk[rpn_i].val = 0;						\
			++rpn_i;									\
			--op_i;										\
		}												\
		op_stk[++op_i] = (T);							\
		++rpn_sz;							
		
	int expression(string text, double *pValue, UNIT **unit, OBJECT *obj);
	int functional_unit(PARSER, double *pValue, UNIT **unit);
	int complex_value(PARSER, gld::complex *pValue);
	int complex_unit(PARSER, gld::complex *pValue, UNIT **unit);
	int time_value_seconds(PARSER, TIMESTAMP *t);
	int time_value_minutes(PARSER, TIMESTAMP *t);
	int time_value_hours(PARSER, TIMESTAMP *t);
	int time_value_days(PARSER, TIMESTAMP *t);
	int time_value_datetime(PARSER, TIMESTAMP *t);
	int time_value_datetimezone(PARSER, TIMESTAMP *t);
	int time_value(PARSER, TIMESTAMP *t);

	string expanded_value(string text);
	bool alternate_value(string& text);
};

#endif // C++

#endif // _PARSER_H_

