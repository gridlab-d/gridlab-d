/* $Id: match.c 4738 2014-07-03 00:55:39Z dchassin $
 *	From _Beautiful Code_, edited by Andy Oram and Greg Wilson, CR 2007 O'Reilly Media, Inc.
 *	"A Regular Expression Matcher", Brian Kernighan
 *
 *	c - matches any literal character c
 *	. - matches any single character 
 *	^ - matches the beginning of the input string 
 *	$ - matches the end of the input string 
 *	* - matches zero or more occurences of the previous character
 *
 */

#include "match.h"

/* match: search for regexp anywhere in text */ 
int match(char *regexp, char *text){
	if(regexp[0] == '^')
		return matchhere(regexp+1, text);
	do {    /* must look even if string is empty */
		if(matchhere(regexp, text))
			return 1;
	} while (*text++ != '\0');
	return 0;
}

/* matchhere: search for regexp at beginning of text */
int matchhere(char *regexp, char *text){
	int force = 0;
	if(regexp[0] == '\\')
		force = 1;
	if(regexp[0] == '\0')
		return 1;
	if((regexp[1] == '*') && !force)
		return matchstar(regexp[0], regexp+2, text);
	if(regexp[0] == '$' && regexp[1] == '\0')
		return *text == '\0';
	if(*text!='\0' && ((regexp[0]=='.' && !force) || (regexp[1]==*text && force) || regexp[0]==*text))
		return matchhere(regexp+1+force, text+1);
	return 0;
}

int matchhere_orig(char *regexp, char *text){
	if(regexp[0] == '\0')
		return 1;
	if(regexp[1] == '*')
		return matchstar(regexp[0], regexp+2, text);
	if(regexp[0] == '$' && regexp[1] == '\0')
		return *text == '\0';
	if(*text!='\0' && (regexp[0]=='.' || regexp[0]==*text))
		return matchhere(regexp+1, text+1);
	return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int matchstar(int c, char *regexp, char *text){
	do{     /* a * matches zero or more instances */
		if(matchhere(regexp, text))
			return 1;
	} while (*text != '\0' && (*text++ == c || c == '.'));
	return 0;
}

/* EOF */
