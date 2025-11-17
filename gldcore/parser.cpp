#include "parser.h"

int parser::findLastIndex(string str, char x) {
    int index = -1;
    for (int i = 0; i < str.length(); i++)
        if (str[i] == x)
            index = i;
    return index;
}

int parser::replaceAll(string& s, string const& toReplace, string const& replaceWith) {
    int cnt = 0;
    string buf;
    size_t pos = 0;
    size_t prevPos;

    // Reserves rough estimate of final size of string.
    buf.reserve(s.size());

    while (true) {
        prevPos = pos;
        pos = s.find(toReplace, pos);
        if (pos == std::string::npos)
            break;
        cnt++;
        buf.append(s, prevPos, pos - prevPos);
        buf += replaceWith;
        pos += toReplace.size();
    }

    buf.append(s, prevPos, s.size() - prevPos);
    s.swap(buf);
    return cnt;
}

string parser::extractBetween(string str, char startChar, char endChar) {
    size_t startPos = str.find(startChar);
    size_t endPos = str.find(startPos + 1);

    if (startPos != string::npos && endPos != string::npos) {
        return str.substr(startPos + 1, endPos);
    }
    return ""; // Return empty string if characters not found
}

string parser::extractBetweenEnd(string str, char startChar, char endChar) {
    size_t startPos = str.find(startChar);
    size_t endPos = str.find(endChar, startPos + 1);
    
    if (startPos != string::npos && endPos != string::npos) {
        return str.substr(startPos + 1, endPos - startPos - 1);
    }
    return ""; // Return empty string if characters not found
}

void parser::forward_slashes(string& str) {
    this->replaceAll(str, "\\", "/");
}

void parser::filename_parts(string file, string& path, string& name, string& ext) {
	/* fix delimiters (result is a static copy) */
	this->forward_slashes(file);

	/* find the last delimiter */
	int s = this->findLastIndex(file, '/');

	/* find the last dot */
	int e = this->findLastIndex(file, '.');

	/* clear results */
	path = "";	name = "";	ext = "";
	
	/* if both found but dot is before delimiter */
	if (e && s && e<s) 
		
		/* there is not extension */
		e = 0;
	
	/* copy extension (if any) and terminate filename at dot */
	if (e)
		ext = file.substr(e+1,file.size());

	/* if path is given */
	if (s)
	{
		/* copy name and terminate path */
		name = file.substr(s+1, file.size());

		/* copy path */
		path = file;
	}

	/* otherwise copy everything */
	else
		name = file;
}

void parser::syntax_error(PARSER) {
	char context[16], *nl;
	strncpy(context,_p,15);
	nl = strchr(context,'\n');
	if (nl!=nullptr) *nl='\0'; else context[15]='\0';
	if (strlen(context)>0)
		output_error_raw("syntax error at '%s...'", context);
	else
		output_error_raw("syntax error");
}

int parser::white(PARSER) {
	int len = 0;
	for(len = 0; *_p != '\0' && isspace((unsigned char)(*_p)); ++_p){
		++len;
	}
	return len;
}

int parser::comment(PARSER) {
	int _n = white(_p);
	if (_p[_n]=='#')
	{
		while (_p[_n]!='\n')
			_n++;
	}
	return _n;
}

int parser::pattern(PARSER, const char *pattern, char *result, int size) {
	char format[64];
	START;
	sprintf(format,"%%%s",pattern);
	if (sscanf(_p,format,result)==1)
		_n = (int)strlen(result);
	DONE;
}

int parser::scan(PARSER, char *format, char *result, int size) {
	START;
	if (sscanf(_p,format,result)==1)
		_n = (int)strlen(result);
	DONE;
}

int parser::literal(PARSER, char *text) {
	if (strncmp(_p,text,strlen(text))==0)
		return (int)strlen(text);
	return 0;
}

int parser::dashed_name(PARSER, char *result, int size) {
	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='-') COPY(result);
	result[_n]='\0';
	DONE;
}

int parser::name(PARSER, char *result, int size) {
	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

int parser::namelist(PARSER, char *result, int size) {
	/* basic list of names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p=='@' || *_p==' ' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

int parser::variable_list(PARSER, char *result, int size) {
	/* basic list of variable names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p==' ' || *_p=='.' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

int parser::property_list(PARSER, char *result, int size) {
	/* basic list of variable names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p==' ' || *_p=='.' || *_p=='_' || *_p==':') COPY(result);
	result[_n]='\0';
	DONE;
}

int parser::unitspec(PARSER, UNIT **unit) {
	char result[1024];
	size_t size = sizeof(result);
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='$' || *_p=='%' || *_p=='*' || *_p=='/' || *_p=='^') COPY(result);
	result[_n]='\0';
    try {
		if ((*unit=unit_find(result))==nullptr){
			_n = 0;
		} else {
			_n = (int)strlen(result);
		}
	}
    catch (char *msg) {
		_n = 0;
	}
	DONE;
}

int parser::unitsuffix(PARSER, UNIT **unit) {
	START;
	if (LITERAL("["))
	{
		if (!TERM(unitspec(HERE,unit)))
		{
			output_error_raw("missing valid unit after [");
			REJECT;
		}
		if (!LITERAL("]"))
		{
			output_error_raw("missing ] after unit '%s'",(*unit)->name);
		}
		ACCEPT;
		DONE;
	}
	REJECT;
	DONE;
}

int parser::nameunit(PARSER,char *result, int size,UNIT **unit) {
	START;
	if (TERM(name(HERE,result,size)) && TERM(unitsuffix(HERE,unit))) ACCEPT; DONE;
	REJECT;
}

int parser::dotted_name(PARSER, char *result, int size) {
	/* basic name */
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='.') COPY(result);
	result[_n]='\0';
	DONE;
}

int parser::hostname(PARSER, char *result, int size) {
	/* full path name */
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='.' || *_p=='-' || *_p==':' ) COPY(result);
	result[_n]='\0';
	DONE;
}

int parser::delim_value(PARSER, char *result, int size, const char *delims) {
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

int parser::structured_value(PARSER, char *result, int size) {
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

int parser::value(string valueString, char *result, int size) {
	/* everything to a semicolon */
    char *_p = valueString.data();
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
		output_warning("missing closing double quote in value text %s", valueString.c_str());
	return (int)(_p - start);
}

#if 0
int parser::functional_int(PARSER, int64 *value) {
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

int parser::integer(PARSER, int64 *value) {
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi64(result);
	return _n;
}

int parser::integer32(PARSER, int32 *value) {
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

int parser::integer16(PARSER, int16 *value) {
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

int parser::real_value(PARSER, double *value) {
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

int parser::functional(PARSER, double *pValue) {
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
			output_error_raw("%s is not a valid random distribution", fname.get_string());
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
						output_error_raw("expected a %s distribution term after ,", fname.get_string());
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
					output_error_raw("missing ) after %s distribution terms", fname.get_string());
					REJECT;
				}
			}
			else
			{
				output_error_raw("expected first term of %s distribution", fname.get_string());
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
						output_error_raw("expected ) after %s distribution term", fname.get_string());
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
						output_error_raw("missing second %s distribution term and/or )", fname.get_string());
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
						output_error_raw("missing terms and/or ) in %s distribution", fname.get_string());
						REJECT;
					}
				}
				else
				{
					output_error_raw("%d terms is not supported", nargs);
					REJECT;
				}
			}
			else
			{
				output_error_raw("expected first term of %s distribution", fname.get_string());
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
int parser::rpnfunc(PARSER, int *val) {
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
	
int parser::expression(string text, double *pValue, UNIT **unit, OBJECT *obj) {
    char *_p = text.data();
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
				output_error_raw("object not found (object must already exist): %s.%s", oname, tname);
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
				output_error_raw("invalid property: %s.%s", oname, tname);
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
				output_error_raw("invalid property: %s.%s", obj->oclass->name, tname);
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
				output_error_raw("unknown reference: %s.%s", oname, pname);
				output_error("may be an order issue, delayed reference checking is a todo");
				REJECT;
			} else {
				double *valptr = object_get_double_by_name(otarg, pname);
				if(valptr == nullptr){
					output_error_raw("invalid property: %s.%s", oname, pname);
					REJECT;
				}
				rpn_stk[rpn_i].op = 0;
				rpn_stk[rpn_i].val = *valptr;
				++rpn_sz;
				++rpn_i;
			}
		} else { /* oops */
			output_error_raw("unrecognized token within: %s9", HERE-2);
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
				output_error_raw("insufficient arguments in equation %i", rpn_stk[i].op);
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
					output_error_raw("unrecognized operator index %i (bug!)", rpn_stk[i].op);
					REJECT;
			}
		} else if(rpn_stk[i].op < 0){ /* rpn_func */
			int j;
			int count = (sizeof(rpn_map)/sizeof(rpn_map[0]));
			for(j = 0; j < count; ++j){
				if(rpn_map[j].index == rpn_stk[i].op){
					double popval = val_q[--val_i];
					if(val_i < 0){
						output_error_raw("insufficient arguments in equation %i", rpn_stk[i].op);
						REJECT;
					}
					val_q[val_i++] = (*rpn_map[j].fptr)(popval);
					break;
				}
			}
			if(j == count){ /* missed */
				output_error_raw("unrecognized function index %i (bug!)", rpn_stk[i].op);
				REJECT;
			}

		}
	}
	if((val_i > 1)){
		output_error_raw("too many values in equation!");
		REJECT;
	}
	*pValue = val_q[0];
	DONE;
}

int parser::functional_unit(string valueString, double *pValue,UNIT **unit) {
    char *_p = valueString.data();
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

int parser::complex_value(PARSER, gld::complex *pValue) {
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

int parser::complex_unit(string valueString, gld::complex *pValue,UNIT **unit) {
    char *_p = valueString.data();
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

int parser::time_value_seconds(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("s")) { *t *= TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("S")) { *t *= TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int parser::time_value_minutes(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("m")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("M")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int parser::time_value_hours(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("h")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("H")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int parser::time_value_days(PARSER, TIMESTAMP *t) {
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("d")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("D")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int parser::time_value_datetime(PARSER, TIMESTAMP *t) {
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

int parser::time_value_datetimezone(PARSER, TIMESTAMP *t) {
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

int parser::time_value(PARSER, TIMESTAMP *t) {
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

string parser::expanded_value(string text) {

	string varname = extractBetween(text, '`', '`');
	if (varname.length() > 0) {
		char val[1024];
		string path;
		string name;
		string ext;
		filename_parts(this->filename, path, name, ext);

		 /* expanded specials variables */
		replaceAll(text, "{file}", this->filename);
		replaceAll(text, "{filename}", name);
		replaceAll(text, "{filepath}", path); 
		replaceAll(text, "{fileext}", ext);
		object_namespace(val, sizeof(val));
		replaceAll(text, "{namespace}", val);
		replaceAll(text, "{class}", this->current_object?this->current_object->oclass->name:"");
		replaceAll(text, "{gridlabd}", global_execdir);
		replaceAll(text, "{hostname}", global_hostname); 
		replaceAll(text, "{hostaddr}", global_hostaddr); 
		sprintf(val,"%d",sched_get_cpuid(0)); 
		replaceAll(text, "{cpu}", val);
		sprintf(val,"%d",sched_get_procid()); 
		replaceAll(text, "{pid}", val);
		sprintf(val,"%d",global_server_portnum);
		replaceAll(text, "{port}", val);
		replaceAll(text, "{mastername}", "localhost"); /* @todo copy actual master name */
		replaceAll(text, "{masteraddr}", "127.0.0.1"); /* @todo copy actual master addr */
		replaceAll(text, "{masterport}", "6267");      /* @todo copy actual master port */
		if (current_object)
			sprintf(val, "%d", current_object->id);
		else
			strcpy(val, "");
		replaceAll(text, "{id}", val);

		while (true) {
			string varname = extractBetween(text, '{', '}');
			if (varname.length() > 0)
				if (object_get_value_by_name(current_object, varname.c_str(), val, sizeof(val)))
					 replaceAll(text, "{"+varname+"}", val); /* val is ok */
				else if (global_getvar(varname.c_str(), val, sizeof(val)) )
					 replaceAll(text, "{"+varname+"}", val); /* val is ok */
			else
				break;
		}
	}
	return text;
}

/** alternate_value allows the use of ternary operations, e.g.,
		 property (expression) ? negzero_value : positive_value ;
 **/
bool parser::alternate_value(string& text) {
	double test;
	
    size_t found_q = text.find("?");
    size_t found_c = text.find(":");
	if (found_q != string::npos && found_c != string::npos) {
		string exp = text.substr(0, found_q);
		if (expression(exp, &test, NULL, current_object)) {
			if (test > 0) {
				text = text.substr(found_q + 1, (found_c-found_q));
				text = expanded_value(text);
				return true;
			}
			else {
				text = text.substr(found_c + 1);
				text = expanded_value(text);
				return true;
			}
		}
	}
	else {
		text = expanded_value(text);
		return true;
	}
	return false;
}

int parser::linear_transform(string valueString, TRANSFORMSOURCE *xstype, void **source, double *scale, double *bias, OBJECT *from)
{
    char *_p = valueString.data();
	START;
	if WHITE ACCEPT;
	/* scale * schedule_name [+ bias]  */
	if (TERM(functional(HERE,scale)) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE, xstype, source, from))))
	{	
		if ((WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,bias)))) { ACCEPT; }
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* scale * schedule_name [- bias]  */
	if (TERM(functional(HERE,scale)) &&( WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		if ((WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,bias)))) { *bias *= -1; ACCEPT; }
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* schedule_name [* scale] [+ bias]  */
	if (TERM(transform_source(HERE,xstype,source,from)))
	{
		if ((WHITE,LITERAL("*")) && (WHITE,TERM(functional(HERE,scale)))) { ACCEPT; }
		else { ACCEPT; *scale = 1;}
		if ((WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,bias)))) { ACCEPT; DONE; }
	 	OR if ((WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,bias)))) { *bias *= -1; ACCEPT; DONE}
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* bias + scale * schedule_name  */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,scale))) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		ACCEPT;
		DONE;
	}
	OR
	/* bias - scale * schedule_name  */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,scale))) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		*scale *= -1;
		ACCEPT;
		DONE;
	}
	OR
	/* bias + schedule_name [* scale] */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("+")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		if (WHITE,LITERAL("*") && WHITE,TERM(functional(HERE,scale))) { ACCEPT; }
		else { ACCEPT; *scale = 1;}
		DONE;
	}
	OR
	/* bias - schedule_name [* scale] */
	if (TERM(functional(HERE,bias)) && WHITE,LITERAL("-") && WHITE,TERM(transform_source(HERE,xstype, source,from)))
	{
		if ((WHITE,LITERAL("*")) && (WHITE,TERM(functional(HERE,scale)))) { ACCEPT; *scale *= -1; }
		else { ACCEPT; *scale = 1;}
		DONE;
	}
	REJECT;
	DONE;
}

int parser::transform_source(PARSER, TRANSFORMSOURCE *xstype, void **source, OBJECT *from)
{
	SCHEDULE *sch;
	START;
	if WHITE ACCEPT;
	if (TERM(schedule_ref(HERE,&sch)))
	{
		*source = (void*)&(sch->value);
		*xstype = XS_SCHEDULE;
		ACCEPT;
	}
	else if (TERM(property_ref(HERE,xstype,source,from)))
	{	ACCEPT; }
	else
	{	REJECT; }
	DONE;
}

int parser::schedule_ref(PARSER, SCHEDULE **sch)
{
	char name[64];
	START;
	if WHITE ACCEPT;
	if (TERM(dashed_name(HERE,name,sizeof(name))))
	{
		ACCEPT;
		if (((*sch)=schedule_find_byname(name))==nullptr)
			REJECT;
	}
	else
		REJECT;
	DONE;
}

int parser::property_ref(PARSER, TRANSFORMSOURCE *xstype, void **ref, OBJECT *from)
{
	FULLNAME oname;
	PROPERTYNAME pname;
	START;
	if WHITE ACCEPT;
	if (TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(dotted_name(HERE,pname,sizeof(pname))))
	{
		OBJECT *obj = (strcmp(oname,"this")==0 ? from : object_find_name(oname));

		// object isn't defined yet
		if (obj==nullptr)
		{
			// add to unresolved list
			char id[1024];
			sprintf(id,"%s.%s",oname,pname);
			*ref = (void*)add_unresolved(from,PT_double,nullptr,from->oclass,id,this->filename.data(),UR_TRANSFORM);
			ACCEPT;
		}
		else 
		{
			PROPERTY *prop = object_get_property(obj,pname,nullptr);
			if (prop==nullptr)
			{
				output_error_raw("property '%s' of object '%s' not found", oname, pname);
				REJECT;
			}
			else if (prop->ptype==PT_double)
			{
				*ref = (void*)object_get_addr(obj,pname); 
				*xstype = XS_DOUBLE;
				ACCEPT;
			}
			else if (prop->ptype==PT_complex)
			{
				// TODO support R,I parts
				*ref = (void*)object_get_addr(obj,pname); // get R part only
				*xstype = XS_COMPLEX;
				ACCEPT;
			}
			else if (prop->ptype==PT_loadshape)
			{
                loadshape *ls = static_cast<loadshape *>(object_get_addr(obj, pname));
				*ref = &(ls->load);
				*xstype = XS_LOADSHAPE;
				ACCEPT;
			}
			else if (prop->ptype==PT_enduse)
			{
                enduse *eu = static_cast<enduse *>(object_get_addr(obj, pname));
                *ref = &(eu->total.Re());
				*xstype = XS_ENDUSE;
				ACCEPT;
			}
			else if ( prop->ptype==PT_random )
			{
                randomvar_struct *rv = static_cast<randomvar_struct *>(object_get_addr(obj, pname));
				*ref = &(rv->value);
				*xstype = XS_RANDOMVAR;
				ACCEPT;
			}
			else
			{
				output_error_raw("transform '%s.%s' does not reference a double or a double container like a loadshape", oname,pname);
				REJECT;
			}
		}
	}
	else
	{	REJECT;	}
	DONE;
}

UNRESOLVED *parser::add_unresolved(OBJECT *by, PROPERTYTYPE ptype, void *ref, CLASS *oclass, char *id, char *file, int flags)
{
	UNRESOLVED *item;
	if ( strlen(id)>=sizeof(item->id))
	{
		output_error("add_unresolved(...): id '%s' is too long to resolve", id);
		return nullptr;
	}
    item = static_cast<UNRESOLVED *>(malloc(sizeof(UNRESOLVED)));
	if (item==nullptr) { errno = ENOMEM; return nullptr; }
	item->by = by;
	item->ptype = ptype;
	item->ref = ref;
	item->oclass = oclass;
	strncpy(item->id,id,sizeof(item->id));
	if (this->first_unresolved!=nullptr && strcmp(this->first_unresolved->file,file)==0)
	{
		item->file = this->first_unresolved->file; // means keep using the same file
		first_unresolved->file = nullptr;
	}
	else
	{
		item->file = (char*)malloc(strlen(file)+1);
		strcpy(item->file,file);
	}
	item->next = first_unresolved;
	item->flags = flags;
	first_unresolved = item;
	return item;
}