#include <iostream>
#include <fstream>
#include <string>
#include <regex>

#include "property.h"
#include "class.h"
#include "module.h"
//#include "load.h"
//#include "loader.h"


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

static void syntax_error(char *p)
{
	char context[16], *nl;
	strncpy(context,p,15);
	nl = strchr(context,'\n');
	if (nl!=nullptr) *nl='\0'; else context[15]='\0';
	if (strlen(context)>0)
		output_error_raw("%s(%d): syntax error at '%s...'", filename, linenum, context);
	else
		output_error_raw("%s(%d): syntax error", filename, linenum);
}

static int parser::comment(PARSER) {
	int _n = white(_p);
	if (_p[_n]=='#')
	{
		while (_p[_n]!='\n')
			_n++;
		linenum++;
	}
	return _n;
}

static int parser::pattern(PARSER, const char *pattern, char *result, int size) {
	char format[64];
	START;
	sprintf(format,"%%%s",pattern);
	if (sscanf(_p,format,result)==1)
		_n = (int)strlen(result);
	DONE;
}

static int parser::scan(PARSER, char *format, char *result, int size) {
	START;
	if (sscanf(_p,format,result)==1)
		_n = (int)strlen(result);
	DONE;
}

static int parser::literal(PARSER, char *text) {
	if (strncmp(_p,text,strlen(text))==0)
		return (int)strlen(text);
	return 0;
}

static int parser::dashed_name(PARSER, char *result, int size) {
	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='-') COPY(result);
	result[_n]='\0';
	DONE;
}

static int parser::name(PARSER, char *result, int size) {
	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

static int parser::namelist(PARSER, char *result, int size) {
	/* basic list of names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p=='@' || *_p==' ' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

static int parser::variable_list(PARSER, char *result, int size) {
	/* basic list of variable names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p==' ' || *_p=='.' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

static int parser::property_list(PARSER, char *result, int size) {
	/* basic list of variable names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p==' ' || *_p=='.' || *_p=='_' || *_p==':') COPY(result);
	result[_n]='\0';
	DONE;
}

static int parser::unitspec(PARSER, UNIT **unit) {
	char result[1024];
	size_t size = sizeof(result);
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='$' || *_p=='%' || *_p=='*' || *_p=='/' || *_p=='^') COPY(result);
	result[_n]='\0';
    try {
		if ((*unit=unit_find(result))==nullptr){
			linenum=_l;
			_n = 0;
		} else {
			_n = (int)strlen(result);
		}
	}
    catch (char *msg) {
		linenum=_l;
		_n = 0;
	}
	DONE;
}

static int parser::unitsuffix(PARSER, UNIT **unit) {
	START;
	if (LITERAL("["))
	{
		if (!TERM(unitspec(HERE,unit)))
		{
			output_error_raw("%s(%d): missing valid unit after [", filename, linenum);
			REJECT;
		}
		if (!LITERAL("]"))
		{
			output_error_raw("%s(%d): missing ] after unit '%s'", filename, linenum,(*unit)->name);
		}
		ACCEPT;
		DONE;
	}
	REJECT;
	DONE;
}

static int parser::nameunit(PARSER,char *result,int size,UNIT **unit) {
	START;
	if (TERM(name(HERE,result,size)) && TERM(unitsuffix(HERE,unit))) ACCEPT; DONE;
	REJECT;
}

static int parser::dotted_name(PARSER, char *result, int size) {
	/* basic name */
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='.') COPY(result);
	result[_n]='\0';
	DONE;
}

static int parser::hostname(PARSER, char *result, int size) {
	/* full path name */
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='.' || *_p=='-' || *_p==':' ) COPY(result);
	result[_n]='\0';
	DONE;
}

static int parser::delim_value(PARSER, char *result, int size, const char *delims) {
	/* everything to any of delims */
	int quote=0;
	char *start=_p;
	START;
	if (*_p=='"')
	{
		quote=1;
		*_p++;
		size--;
	}
	while (size>1 && *_p!='\0' && ((quote&&*_p!='"') || strchr(delims,*_p)==nullptr) && *_p!='\n')
	{
		if ( _p[0]=='\\' && _p[1]!='\0' ) _p++; 
		COPY(result);
	}
	result[_n]='\0';
	return (int)(_p - start);
}

static int parser::structured_value(PARSER, char *result, int size) {
	int depth=0;
	char *start=_p;
	START;
	if (*_p!='{') return 0;
	while (size>1 && *_p!='\0' && !(*_p=='}'&&depth==1) ) 
	{
		if ( _p[0]=='\\' && _p[1]!='\0' ) _p++; 
		else if ( *_p=='{' ) depth++; 
		else if ( *_p=='}' ) depth--;
		COPY(result);
	}
	COPY(result);
	result[_n]='\0';
	return (int)(_p - start);
}

static int parser::value(PARSER, char *result, int size) {
	/* everything to a semicolon */
	char delim=';';
	char *start=_p;
	int quote=0;
	START;
	if ( *_p=='{' ) 
		return structured_value(_p,result,size);
	while (size>1 && *_p!='\0' && !(*_p==delim && quote == 0) && *_p!='\n') 
	{
		if ( _p[0]=='\\' && _p[1]!='\0' )
		{
			_p++; COPY(result);
		}
		else if (*_p=='"')
		{
			*_p++;
			size--;
			quote = (1+quote) % 2;
		}
		else
			COPY(result);
	}
	result[_n]='\0';
	if (quote&1)
		output_warning("%s(%d): missing closing double quote", filename, linenum);
	return (int)(_p - start);
}

#if 0
static int parser::functional_int(PARSER, int64 *value) {
	char result[256];
	int size=sizeof(result);
	double pValue;
	char32 fname;
	START;
//	while (size>1 && isdigit(*_p)) COPY(result);
//	result[_n]='\0';
//	*value=atoi64(result);
	/* copy-pasted from functional */
	if (LITERAL("random.") && TERM(name(HERE,fname,sizeof(fname))))
	{
		RANDOMTYPE rtype = random_type(fname);
		int nargs = random_nargs(fname);
		double a;
		if (rtype==RT_INVALID || nargs==0 || (WHITE,!LITERAL("(")))
		{
			output_message("%s(%d): %s is not a valid random distribution", filename,linenum,fname);
			REJECT;
		}
		if (nargs==-1)
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				double b[1024];
				int maxb = sizeof(b)/sizeof(b[0]);
				int n;
				b[0] = a;
				for (n=1; n<maxb && (WHITE,LITERAL(",")); n++)
				{
					if (WHITE,TERM(real_value(HERE,&b[n])))
						continue;
					else
					{
						// variable arg list
						output_message("%s(%d): expected a %s distribution term after ,", filename,linenum, fname);
						REJECT;
					}
				}
				if (WHITE,LITERAL(")"))
				{
					pValue = random_value(rtype,n,b);
					ACCEPT;
				}
				else
				{
					output_message("%s(%d): missing ) after %s distribution terms", filename,linenum, fname);
					REJECT;
				}
			}
			else
			{
				output_message("%s(%d): expected first term of %s distribution", filename,linenum, fname);
				REJECT;
			}
		}
		else 
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				// fixed arg list
				double b,c;
				if (nargs==1)
				{
					if (WHITE,LITERAL(")"))
					{
						pValue = random_value(rtype,a);
						ACCEPT;
					}
					else
					{
						output_message("%s(%d): expected ) after %s distribution term", filename,linenum, fname);
						REJECT;
					}
				}
				else if (nargs==2)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && (WHITE,LITERAL(")")))
					{
						pValue = random_value(rtype,a,b);
						ACCEPT;
					}
					else
					{
						output_message("%s(%d): missing second %s distribution term and/or )", filename,linenum, fname);
						REJECT;
					}
				}
				else if (nargs==3)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && WHITE,LITERAL(",") && (WHITE,TERM(real_value(HERE,&c))) && (WHITE,LITERAL(")")))
					{
						pValue = random_value(rtype,a,b,c);
						ACCEPT;
					}
					else
					{
						output_message("%s(%d): missing terms and/or ) in %s distribution ", filename,linenum, fname);
						REJECT;
					}
				}
				else
				{
					output_message("%s(%d): %d terms is not supported", filename,linenum, nargs);
					REJECT;
				}
			}
			else
			{
				output_message("%s(%d): expected first term of %s distribution", filename,linenum, fname);
				REJECT;
			}
		}
	} // end if "random."
	return _n;
}
#endif

static int parser::integer(PARSER, int64 *value) {
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi64(result);
	return _n;
}

static int parser::integer32(PARSER, int32 *value) {
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

static int parser::integer16(PARSER, int16 *value) {
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

static int parser::real_value(PARSER, double *value) {
	char result[256];
	int ndigits=0;
	int size=sizeof(result);
	START;
	if (*_p=='+' || *_p=='-') COPY(result);
	while (size>1 && isdigit(*_p)) {COPY(result);++ndigits;}
	if (*_p=='.') COPY(result);
	while (size>1 && isdigit(*_p)) {COPY(result);ndigits++;}
	if (ndigits>0 && (*_p=='E' || *_p=='e')) 
	{
		COPY(result);
		if (*_p=='+' || *_p=='-') COPY(result);
		while (size>1 && isdigit(*_p)) COPY(result);
	}
	result[_n]='\0';
	*value=atof(result);
	return _n;
}

static int parser::functional(PARSER, double *pValue) {
	char32 fname;
	START;
	if WHITE ACCEPT;
	if (LITERAL("random.") && TERM(name(HERE,fname,sizeof(fname))))
	{
		RANDOMTYPE rtype = random_type(fname);
		int nargs = random_nargs(fname);
		double a;
		if (rtype==RT_INVALID || nargs==0 || (WHITE,!LITERAL("(")))
		{
			output_error_raw("%s(%d): %s is not a valid random distribution", filename,linenum,fname.get_string());
			REJECT;
		}
		if (nargs==-1)
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				double b[1024];
				int maxb = sizeof(b)/sizeof(b[0]);
				int n;
				b[0] = a;
				for (n=1; n<maxb && (WHITE,LITERAL(",")); n++)
				{
					if (WHITE,TERM(real_value(HERE,&b[n])))
						continue;
					else
					{
						// variable arg list
						output_error_raw("%s(%d): expected a %s distribution term after ,", filename,linenum, fname.get_string());
						REJECT;
					}
				}
				if (WHITE,LITERAL(")"))
				{
					*pValue = random_value(rtype,n,b);
					ACCEPT;
				}
				else
				{
					output_error_raw("%s(%d): missing ) after %s distribution terms", filename,linenum, fname.get_string());
					REJECT;
				}
			}
			else
			{
				output_error_raw("%s(%d): expected first term of %s distribution", filename,linenum, fname.get_string());
				REJECT;
			}
		}
		else 
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				// fixed arg list
				double b,c;
				if (nargs==1)
				{
					if (WHITE,LITERAL(")"))
					{
						*pValue = random_value(rtype,a);
						ACCEPT;
					}
					else
					{
						output_error_raw("%s(%d): expected ) after %s distribution term", filename,linenum, fname.get_string());
						REJECT;
					}
				}
				else if (nargs==2)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && (WHITE,LITERAL(")")))
					{
						*pValue = random_value(rtype,a,b);
						ACCEPT;
					}
					else
					{
						output_error_raw("%s(%d): missing second %s distribution term and/or )", filename,linenum, fname.get_string());
						REJECT;
					}
				}
				else if (nargs==3)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && WHITE,LITERAL(",") && (WHITE,TERM(real_value(HERE,&c))) && (WHITE,LITERAL(")")))
					{
						*pValue = random_value(rtype,a,b,c);
						ACCEPT;
					}
					else
					{
						output_error_raw("%s(%d): missing terms and/or ) in %s distribution ", filename,linenum, fname.get_string());
						REJECT;
					}
				}
				else
				{
					output_error_raw("%s(%d): %d terms is not supported", filename,linenum, nargs);
					REJECT;
				}
			}
			else
			{
				output_error_raw("%s(%d): expected first term of %s distribution", filename,linenum, fname.get_string());
				REJECT;
			}
		}
	} else if TERM(real_value(HERE,pValue)){
		ACCEPT;
	} else
	{
		/* possibly valid through expression() -MH */
		//output_message("%s(%d): expected property or functional value", filename,linenum);
		REJECT;
	}
	DONE;
}

/* Expression rules:
 *	every value is either a double value, or a PT_double object property of the form "this.propname"
 *	valid operators are {+, -, *, /, ^}
 *	every expression begins and ends with parenthesis
 *	every value is followed by an operator or a close parenthesis
 *	every operator is followed by a value or an open parenthesis
 *	every open parenthesis is followed by a value
 *	every close parenthesis is followed by an operator or the end of the expression
 *	parenthesis must be matched
 *	every value or operator must consume trailing whitespace
 * Step one: form the expression list
 * Step two: break the list into a tree
 * Step three: evaluate the tree bottom-up
 *
 *
 *	Dijkstra's Shunting Yard algorithm
 *	ref: wikipedia (sadly)
 *
 *	- read a token
 *	- if the token is a number, add it to the output queue
 *	- if the token is a function token, then push it onto the stack
 *	- if the token is an operator, o1, then:
 *		- while there is an operator, o2, at the top of the stack, and either
 *			- o1 is associative or left-associative and its precedence is less than or equal to that of o2, or
 *			- o1 is right-associative and its precedence is less than that of o2
 *			then pop o2 off the stack and onto the output queue
 *	- if the token is a left parenthesis, then push it onto the stack.
 *	- if the toekn is a right parenthesis:
 *		- until the token at the top is a left parenthesis, pop operators off the stack onto the output queue
 *		- pop the left parenthesis from the stack, but not onto the output queue
 *		- if the token at the top of the stack is a function token, pop it and onto the output queue
 *		- if the stack runs out without finding a left parenthesis, then there are mismatched parentheses
 *	- when there are no more tokens to read:
 *		- while there are still operator tokens on the stack,
 *			- if the operator token is a parenthesis, we have a mismatched parenthesis
 *			- pop the operator onto the queue
 * - FIN
 */
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
} rpn_map[] = {
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

static int parser::rpnfunc(PARSER, int *val) {
	struct s_rpn_func *ptr = rpn_map;
	int i = 0, count = 0;
	START;
	count = (sizeof(rpn_map)/sizeof(rpn_map[0]));
	for(i = 0; i < count; ++i){
		if(strncmp(rpn_map[i].name, HERE, strlen(rpn_map[i].name)) == 0){
			*val = rpn_map[i].index;
			return (int)strlen(rpn_map[i].name);
		}
	}
	return 0;
}

//static const int OP_END = 0, OP_OPEN = 1, OP_CLOSE = 2, OP_POW = 3,
//		OP_MULT = 4, OP_MOD = 5, OP_DIV = 6, OP_ADD = 7, OP_SUB = 8;
//static int OP_SIN = -1, OP_COS = -2, OP_TAN = -3, OP_ABS = -4;

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

static int parser::op_prec[] = {0, 0, 0, 3, 2, 2, 2, 1, 1};

#define PASS_OP(T) \
	while(op_prec[(T)] <= op_prec[op_stk[op_i]]){	\
		rpn_stk[rpn_i].op = op_stk[op_i];				\
		rpn_stk[rpn_i].val = 0;							\
		++rpn_i;										\
		--op_i;											\
	}													\
	op_stk[++op_i] = (T);							\
	++rpn_sz;							
	
static int parser::expression(PARSER, double *pValue, UNIT **unit, OBJECT *obj) {
	double val_q[128], tVal;
	char tname[128]; /* type name for this.prop */
	char oname[128], pname[128];
	struct s_rpn rpn_stk[256];
	int op_stk[128], val_i = 0, op_i = 1, rpn_i = 0, depth = 0, rfname = 0, rpn_sz = 0;
	int i = 0;
	
	START;
	/* RPN-ify */
	if LITERAL("("){
		ACCEPT;
		if WHITE ACCEPT;
		depth = 1;
		op_stk[0] = OP_OPEN;
		op_i = 0;
	} else {
		REJECT; /* all expressions must be contained within a () block */
	}
	while(depth > 0){ /* grab tokens*/
		if LITERAL(";"){ /* says we're done */
			ACCEPT;
			break;
		} else if LITERAL("("){ /* parantheses */
			ACCEPT;
			op_stk[++op_i] = OP_OPEN;
			//++op_i;
			++depth;
			if WHITE ACCEPT;
		} else if LITERAL(")"){
			ACCEPT;
			if WHITE ACCEPT;
			--depth;
			/* consume operations until OP_OPEN found */
			while((op_i >= 0) && (op_stk[op_i] != OP_OPEN)){
				rpn_stk[rpn_i].op = op_stk[op_i--];
				rpn_stk[rpn_i].val = 0.0;
				++rpn_i;
			}
			/* consume OP_OPEN too */
			op_i--;
			/* rpnfunc lookahead */
			if(op_stk[op_i] < 0){ /* push rpnfunc */
				rpn_stk[rpn_i].op = op_stk[op_i--];
				rpn_stk[rpn_i].val = 0.0;
				++rpn_i;
			}
			/* op_stk[op_i] == OP_CLOSE */
		} else if LITERAL("^"){ /* operators */
			ACCEPT;
			if WHITE ACCEPT;
			op_stk[++op_i] = OP_POW; /* nothing but () and functions hold higher precedence */
			++rpn_sz;
		} else if LITERAL("*"){ /* prec = 4 */
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_MULT);
		} else if LITERAL("/"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_DIV);
		} else if LITERAL("%"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_MOD);
		} else if LITERAL("+"){ /* prec = 6 */
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_ADD);
		} else if LITERAL("-"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_SUB);
		} else if(TERM(rpnfunc(HERE, &rfname))){
			ACCEPT;
			if WHITE ACCEPT;
			op_stk[++op_i] = rfname;
			if LITERAL("("){
				ACCEPT;
				if WHITE ACCEPT;
				op_stk[++op_i] = OP_OPEN;
				++depth;
				++rpn_sz;
			} else {
				REJECT;
			}
		} else if ( TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(name(HERE,tname,sizeof(tname))))
		{
			OBJECT *nobj = object_find_name(oname);
			if ( nobj == nullptr )
			{
				output_error_raw("%s(%d): object not found (object must already exist): %s.%s", filename,linenum, oname, tname);
				REJECT;
			}
			double *valptr = object_get_double_by_name(nobj, tname);
			if ( strcmp(tname,"latitude")==0 )
			{
				valptr = &(obj->latitude);
			}
			else if ( strcmp(tname,"longitude")==0 )
			{
				valptr = &(obj->longitude);
			}
			else if (valptr == nullptr)
			{
				output_error_raw("%s(%d): invalid property: %s.%s", filename,linenum, oname, tname);
				REJECT;
			}
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;
			rpn_stk[rpn_i].val = *valptr;
			++rpn_sz;
			++rpn_i;
		} else if ((LITERAL("$") || LITERAL("this.")) && TERM(name(HERE,tname,sizeof(tname)))){
			double *valptr = object_get_double_by_name(obj, tname);
			if(valptr == nullptr){
				output_error_raw("%s(%d): invalid property: %s.%s", filename,linenum, obj->oclass->name, tname);
				REJECT;
			}
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;
			rpn_stk[rpn_i].val = *valptr;
			++rpn_sz;
			++rpn_i;
		} else if (TERM(functional(HERE, &tVal))){ /* captures reals too */
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;

			rpn_stk[rpn_i].val = tVal;
			++rpn_i;
			++rpn_sz;
		} else if(TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(name(HERE,pname,sizeof(pname)))){
			/* obj.prop*/
			OBJECT *otarg = nullptr;
			ACCEPT;
			if WHITE ACCEPT;
			if(0 == strcmp(oname, "parent")){
				otarg = obj->parent;
			} else {
				otarg = object_find_name(oname);
			}
			if(otarg == nullptr){ // delayed checking
				// disabled for now
				output_error_raw("%s(%d): unknown reference: %s.%s", filename, linenum, oname, pname);
				output_error("may be an order issue, delayed reference checking is a todo");
				REJECT;
			} else {
				double *valptr = object_get_double_by_name(otarg, pname);
				if(valptr == nullptr){
					output_error_raw("%s(%d): invalid property: %s.%s", filename,linenum, oname, pname);
					REJECT;
				}
				rpn_stk[rpn_i].op = 0;
				rpn_stk[rpn_i].val = *valptr;
				++rpn_sz;
				++rpn_i;
			}
		} else { /* oops */
			output_error_raw("%s(%d): unrecognized token within: %s9", filename,linenum, HERE-2);
			REJECT;
			/* It looked like an expression.  Give fair warning. */
		}
	}

	/* depth == 0 ~ pop the op stack to the rpn queue */
	while(op_i >= 0){
		if(op_stk[op_i] != OP_OPEN){
			rpn_stk[rpn_i].op = op_stk[op_i];
			rpn_stk[rpn_i].val = 0.0;
			++rpn_i;
		}
		--op_i;
	}
	/* if no semicolon, there's a bigger error, so we don't check that here */
	
	/* postfix algorithm */
	/*	- while there are input tokens left,
	 *		- read the next input token
	 *		- if the token is a value
	 *			- push the token onto a stack
	 *		- if the token is an operator
	 *			- it is known a priori that the operator takes N arguments
	 *			- if there are fewer than N values on the stack, error.
	 *			- else pop the top n values from the stack
	 *			- evaluate the operator with with the values as arguments
	 *			- push the returned value back onto the stack
	 *	- iff one value remains on the stack, return that value
	 *	- if more values exist on the stack, error
	 */

	rpn_i = 0;

	for(i = 0; i < rpn_sz; ++i){
		if(rpn_stk[i].op == 0){ /* push value */
			val_q[val_i++] = rpn_stk[i].val;
		} else if(rpn_stk[i].op > 0){ /* binary operator */
			double popval = val_q[--val_i];
			if(val_i < 0){
				output_error_raw("%s(%d): insufficient arguments in equation", filename,linenum, rpn_stk[i].op);
				REJECT;
			}
			switch(rpn_stk[i].op){
				case OP_POW:
					val_q[val_i-1] = pow(val_q[val_i-1], popval);
					break;
				case OP_MULT:
					val_q[val_i-1] *= popval;
					break;
				case OP_MOD:
					val_q[val_i-1] = fmod(val_q[val_i-1], popval);
					break;
				case OP_DIV:
					val_q[val_i-1] /= popval;
					break;
				case OP_ADD:
					val_q[val_i-1] += popval;
					break;
				case OP_SUB:
					val_q[val_i-1] -= popval;
					break;
				default:
					output_error_raw("%s(%d): unrecognized operator index %i (bug!)", filename,linenum, rpn_stk[i].op);
					REJECT;
			}
		} else if(rpn_stk[i].op < 0){ /* rpn_func */
			int j;
			int count = (sizeof(rpn_map)/sizeof(rpn_map[0]));
			for(j = 0; j < count; ++j){
				if(rpn_map[j].index == rpn_stk[i].op){
					double popval = val_q[--val_i];
					if(val_i < 0){
						output_error_raw("%s(%d): insufficient arguments in equation", filename,linenum, rpn_stk[i].op);
						REJECT;
					}
					val_q[val_i++] = (*rpn_map[j].fptr)(popval);
					break;
				}
			}
			if(j == count){ /* missed */
				output_error_raw("%s(%d): unrecognized function index %i (bug!)", filename,linenum, rpn_stk[i].op);
				REJECT;
			}

		}
	}
	if((val_i > 1)){
		output_error_raw("%s(%d): too many values in equation!", filename,linenum);
		REJECT;
	}
	*pValue = val_q[0];
	DONE;
}

static int parser::functional_unit(PARSER,double *pValue,UNIT **unit) {
	START;
	if TERM(functional(HERE,pValue))
	{
		*unit = nullptr;
		if WHITE ACCEPT;
		if TERM(unitspec(HERE,unit)) ACCEPT;
		ACCEPT;
		DONE;
	}
	REJECT;
}

static int parser::complex_value(PARSER, gld::complex *pValue) {
	double r, i, m, a;
	START;
	if ((WHITE,TERM(real_value(HERE,&r))) && (WHITE,TERM(real_value(HERE,&i))) && LITERAL("i"))
	{
        pValue->SetRect(r, i, I);
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&r))) && (WHITE,TERM(real_value(HERE,&i))) && LITERAL("j"))
	{
        pValue->SetRect(r, i, J);
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&m))) && (WHITE,TERM(real_value(HERE,&a))) && LITERAL("d"))
	{
        pValue->SetRect(m * cos(a * PI / 180), m * sin(a * PI / 180), A);
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&m))) && (WHITE,TERM(real_value(HERE,&a))) && LITERAL("r"))
	{
        pValue->SetRect(m * cos(a), m * sin(a), R);
		ACCEPT;
		DONE;
	} 
	OR
	if ((WHITE,TERM(real_value(HERE,&m))))
	{
        pValue->SetRect(m, 0.0, I);
		ACCEPT;
		DONE;
	}

	REJECT;
}

static int parser::complex_unit(PARSER,gld::complex *pValue,UNIT **unit) {
	START;
	if TERM(complex_value(HERE,pValue))
	{
		*unit = nullptr;
		if WHITE ACCEPT;
		if TERM(unitspec(HERE,unit)) ACCEPT;
		ACCEPT;
		DONE;
	}
	REJECT;
}

static int parser::time_value_seconds(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("s")) { *t *= TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("S")) { *t *= TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int parser::time_value_minutes(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("m")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("M")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int parser::time_value_hours(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("h")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("H")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int parser::time_value_days(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("d")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("D")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int time_value_datetime(PARSER, TIMESTAMP *t) {
	DATETIME dt;
	START;
	if WHITE ACCEPT;
	if LITERAL("'") ACCEPT;
	if (TERM(integer16(HERE,&dt.year)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.month)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.day)) && LITERAL(" ")
		&& TERM(integer16(HERE,&dt.hour)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.minute)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.second)) && LITERAL("'"))
	{
		dt.nanosecond = 0;
		dt.weekday = -1;
		dt.is_dst = -1;
		strcpy(dt.tz,"");
		*t = mkdatetime(&dt);
		if (*t!=-1) 
		{
			ACCEPT;
        } else REJECT;
    } else REJECT;
	DONE;
}

int time_value_datetimezone(PARSER, TIMESTAMP *t) {
	DATETIME dt;
	START;
	if WHITE ACCEPT;
	if (LITERAL("'")||LITERAL("\"")) ACCEPT;
	if (TERM(integer16(HERE,&dt.year)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.month)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.day)) && LITERAL(" ")
		&& TERM(integer16(HERE,&dt.hour)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.minute)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.second)) && LITERAL(" ")
		&& TERM(name(HERE,dt.tz,sizeof(dt.tz))) && (LITERAL("'")||LITERAL("\"")))
	{
		dt.nanosecond = 0;
		dt.weekday = -1;
		dt.is_dst = -1;
		*t = mkdatetime(&dt);
		if (*t!=-1) 
		{
			ACCEPT;
        } else REJECT;
    } else REJECT;
	DONE;
}

int time_value(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(time_value_seconds(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_minutes(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_hours(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_days(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_datetime(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_datetimezone(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(integer(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	else REJECT;
	DONE;
}
