/** $Id: convert.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file convert.c
	@author David P. Chassin
	@date 2007
	@addtogroup convert Conversion of properties
	@ingroup core
	
	The convert module handles conversion object properties and strings
	
@{
 **/

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "output.h"
#include "globals.h"
#include "convert.h"
#include "object.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
typedef uint32_t  uint32;   /* unsigned 32-bit integers */
#else
typedef unsigned int uint32;
#endif

// we're not really using these yet... -MH
int convert_from_real(char *a, int b, void *c, PROPERTY *d){return 0;}
int convert_to_real(const char *a, void *b, PROPERTY *c){return 0;}
int convert_from_float(char *a, int b, void *c, PROPERTY *d){return 0;}
int convert_to_float(const char *a, void *b, PROPERTY *c){return 0;}

/** Convert from a \e void
	This conversion does not change the data
	@return 6, the number of characters written to the buffer, 0 if not enough space
 **/
int convert_from_void(char *buffer, /**< a pointer to the string buffer */
					  int size, /**< the size of the string buffer */
					  void *data, /**< a pointer to the data that is not changed */
					  PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	if(size < 7)
		return 0;
	return sprintf(buffer,"%s","(void)");
}

/** Convert to a \e void
	This conversion ignores the data
	@return always 1, indicated data was successfully ignored
 **/
int convert_to_void(const char *buffer, /**< a pointer to the string buffer that is ignored */
					  void *data, /**< a pointer to the data that is not changed */
					  PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	return 1;
}

/** Convert from a \e double
	Converts from a \e double property to the string.  This function uses
	the global variable \p global_double_format to perform the conversion.
	@return the number of characters written to the string
 **/
int convert_from_double(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[1025];
	int count = 0;

	double scale = 1.0;
	if(prop->unit != NULL){

		/* only do conversion if the target unit differs from the class's unit for that property */
		PROPERTY *ptmp = (prop->oclass==NULL ? prop : class_find_property(prop->oclass, prop->name));
		scale = *(double *)data;
		if(prop->unit != ptmp->unit && ptmp->unit != NULL){
			if(0 == unit_convert_ex(ptmp->unit, prop->unit, &scale)){
				output_error("convert_from_double(): unable to convert unit '%s' to '%s' for property '%s' (tape experiment error)", ptmp->unit->name, prop->unit->name, prop->name);
				return 0;
			} else {
				count = sprintf(temp, global_double_format, scale);
			}
		} else {
			count = sprintf(temp, global_double_format, *(double *)data);
		}
	} else {
		count = sprintf(temp, global_double_format, *(double *)data);
	}


	if(count < size+1){
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	} else {
		return 0;
	}
}

/** Convert to a \e double
	Converts a string to a \e double property.  This function uses the global
	variable \p global_double_format to perform the conversion.
	@return 1 on success, 0 on failure, -1 is conversion was incomplete
 **/
int convert_to_double(const char *buffer, /**< a pointer to the string buffer */
					  void *data, /**< a pointer to the data */
					  PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char unit[256];
	int n = sscanf(buffer,"%lg%s",data,unit);
	if ( n>1 && prop->unit!=NULL ) /* unit given and unit allowed */
	{
		UNIT *from = unit_find(unit);
		if ( from != prop->unit && unit_convert_ex(from,prop->unit,(double*)data)==0)
		{
			output_error("convert_to_double(const char *buffer='%s', void *data=0x%*p, PROPERTY *prop={name='%s',...}): unit conversion failed", buffer, sizeof(void*), data, prop->name);
			/* TROUBLESHOOT 
			   This error is caused by an attempt to convert a value from a unit that is
			   incompatible with the unit of the target property.  Check your units and
			   try again.
		     */
			return 0;
		}
	}
	return n;
}

/** Convert from a complex
	Converts a complex property to a string.  This function uses
	the global variable \p global_complex_format to perform the conversion.
	@return the number of character written to the string
 **/
int convert_from_complex(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	int count = 0;
	char temp[1025];
	complex *v = (complex*)data;

	double scale = 1.0;
	if ( prop->unit!=NULL )
	{

		/* only do conversion if the target unit differs from the class's unit for that property */
		PROPERTY *ptmp = (prop->oclass==NULL ? prop : class_find_property(prop->oclass, prop->name));
	
		if(prop->unit != ptmp->unit){
			if(0 == unit_convert_ex(ptmp->unit, prop->unit, &scale)){
				output_error("convert_from_complex(): unable to convert unit '%s' to '%s' for property '%s' (tape experiment error)", ptmp->unit->name, prop->unit->name, prop->name);
				/*	TROUBLESHOOT
					This is an error with the conversion of units from the complex property's units to the requested units.
					Please double check the units of the property and compare them to the units defined in the
					offending tape object.
				*/
				scale = 1.0;
			}
		}
	}

	if (v->Notation()==A)
	{
		double m = v->Mag()*scale;
		double a = v->Arg();
		if (a>PI) a-=(2*PI);
		count = sprintf(temp,global_complex_format,m,a*180/PI,A);
	} 
	else if (v->Notation()==R)
	{
		double m = v->Mag()*scale;
		double a = v->Arg();
		if (a>PI) a-=(2*PI);
		count = sprintf(temp,global_complex_format,m,a,R);
	} 
	else {
		count = sprintf(temp,global_complex_format,v->Re()*scale,v->Im()*scale,v->Notation()?v->Notation():'i');
	}
	if(count < size - 1){
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	} else {
		return 0;
	}
}

/** Convert to a complex
	Converts a string to a complex property.  This function uses the global
	variable \p global_complex_format to perform the conversion.
	@return 1 when only real is read, 2 imaginary part is also read, 3 when notation is also read, 0 on failure, -1 is conversion was incomplete
 **/
int convert_to_complex(const char *buffer, /**< a pointer to the string buffer */
					   void *data, /**< a pointer to the data */
					   PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	complex *v = (complex*)data;
	char unit[256];
	char notation[2]={'\0','\0'}; /* force detection invalid complex number */
	int n;
	double a=0, b=0; 
	if(buffer[0] == 0){
		/* empty string */
		v->SetRect(0.0, 0.0,v->Notation());
		return 1;
	}
	n = sscanf(buffer,"%lg%lg%1[ijdr]%s",&a,&b,notation,unit);
	if (n==1) /* only real part */
		v->SetRect(a,0,v->Notation());
	else if (n < 3 || strchr("ijdr",notation[0])==NULL) /* incomplete imaginary part or missing notation */
	{
		output_error("convert_to_complex('%s',%s): complex number format is not valid", buffer,prop->name);
		/* TROUBLESHOOT
			A complex number was given that doesn't meet the formatting requirements, e.g., <real><+/-><imaginary><notation>.  
			Check the format of your complex numbers and try again.
		 */
		return 0;
	}
	/* appears ok */
	else if (notation[0]==A) /* polar degrees */
		v->SetPolar(a,b*PI/180.0,v->Notation()); 
	else if (notation[0]==R) /* polar radians */
		v->SetPolar(a,b,v->Notation()); 
	else 
		v->SetRect(a,b,v->Notation()); /* rectangular */
	if (v->Notation()==I) /* only override notation when property is using I */
		v->Notation() = (CNOTATION)notation[0];

	if ( n>3 && prop->unit!=NULL ) /* unit given and unit allowed */
	{
		UNIT *from = unit_find(unit);
		double scale=1.0;
		if ( from != prop->unit && unit_convert_ex(from,prop->unit,&scale)==0)
		{
			output_error("convert_to_double(const char *buffer='%s', void *data=0x%*p, PROPERTY *prop={name='%s',...}): unit conversion failed", buffer, sizeof(void*), data, prop->name);
			/* TROUBLESHOOT 
			   This error is caused by an attempt to convert a value from a unit that is
			   incompatible with the unit of the target property.  Check your units and
			   try again.
		     */
			return 0;
		}
		v->Re() *= scale;
		v->Im() *= scale;
	}
	return 1;
}

/** Convert from an \e enumeration
	Converts an \e enumeration property to a string.  
	@return the number of character written to the string
 **/
int convert_from_enumeration(char *buffer, /**< pointer to the string buffer */
						     int size, /**< size of the string buffer */
					         void *data, /**< a pointer to the data */
					         PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	KEYWORD *keys=prop->keywords;
	int count = 0;
	char temp[1025];
	/* get the true value */
	int value = *(uint32*)data;

	/* process the keyword list, if any */
	for ( ; keys!=NULL ; keys=keys->next)
	{
		/* if the key value matched */
		if (keys->value==value){
			/* use the keyword */
			count = strncpy(temp,keys->name,1024)?(int)strlen(temp):0;
			break;
		}
	}

	/* no keyword found, return the numeric value instead */
	if (count == 0){
		 count = sprintf(temp,"%d",value);
	}
	if(count < size - 1){
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	} else {
		return 0;
	}
}

/** Convert to an \e enumeration
	Converts a string to an \e enumeration property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_enumeration(const char *buffer, /**< a pointer to the string buffer */
					       void *data, /**< a pointer to the data */
					       PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	bool found = false;
	KEYWORD *keys=prop->keywords;

	/* process the keyword list */
	for ( ; keys!=NULL ; keys=keys->next)
	{
		if (strcmp(keys->name,buffer)==0)
		{
			*(uint32*)data=(uint32)(keys->value);
			found = true;
			break;
		}
	}
	if (found)
		return 1;
	if (strncmp(buffer,"0x",2)==0)
		return sscanf(buffer+2,"%x",(uint32*)data);
	if (isdigit(*buffer))
		return sscanf(buffer,"%d",(uint32*)data);
	else if (strcmp(buffer,"")==0)
		return 0; // empty string do nothing
	output_error("keyword '%s' is not valid for property %s", buffer,prop->name);
	return 0;
}

/** Convert from an \e set
	Converts a \e set property to a string.  
	@return the number of character written to the string
 **/
#define SETDELIM "|"
int convert_from_set(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	KEYWORD *keys;

	/* get the actual value */
	unsigned int64 value = *(unsigned int64*)data;

	/* keep track of how characters written */
	int count=0;

	int ISZERO = (value == 0);
	/* clear the buffer */
	buffer[0] = '\0';

	/* process each keyword */
	for ( keys=prop->keywords ; keys!=NULL ; keys=keys->next)
	{
		/* if the keyword matches */
		if ((!ISZERO && keys->value!=0 && (keys->value&value)==keys->value) || (keys->value==0 && ISZERO))
		{
			/* get the length of the keyword */
			int len = (int)strlen(keys->name);

			/* remove the key from the copied values */
			value &= ~(keys->value);

			/* if there's room for it in the buffer */
			if (size>count+len+1)
			{
				/* if the buffer already has keywords in it */
				if (buffer[0]!='\0')
				{
					/* add a separator to the buffer */
					if (!(prop->flags&PF_CHARSET))
					{
						count++;
						strcat(buffer,SETDELIM);
					}
				}

				/* add the keyword to the buffer */
				count += len;
				strcat(buffer,keys->name);
			}

			/* no room in the buffer */
			else

				/* fail */
				return 0;
		}
	}

	/* succeed */
	return count;
}

/** Convert to a \e set
	Converts a string to a \e set property.  
	@return number of values read on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_set(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	KEYWORD *keys=prop->keywords;
	char temp[4096];
	const char *ptr;
	uint32 value=0;
	int count=0;

	/* directly convert numeric strings */
	if (strnicmp(buffer,"0x",2)==0)
		return sscanf(buffer,"0x%x",(uint32*)data);
	else if (isdigit(buffer[0]))
		return sscanf(buffer,"%d",(uint32*)data);

	/* prevent long buffer from being scanned */
	if (strlen(buffer)>sizeof(temp)-1)
		return 0;

	/* make a temporary copy of the buffer */
	strcpy(temp,buffer);

	/* check for CHARSET keys (single character keys) and usage without | */
	if ((prop->flags&PF_CHARSET) && strchr(buffer,'|')==NULL)
	{
		for (ptr=buffer; *ptr!='\0'; ptr++)
		{
			bool found = false;
			KEYWORD *key;
			for (key=keys; key!=NULL; key=key->next)
			{
				if (*ptr==key->name[0])
				{
					value |= key->value;
					count ++;
					found = true;
					break; /* we found our key */
				}
			}
			if (!found)
			{
				output_error("set member '%c' is not a keyword of property %s", *ptr, prop->name);
				return 0;
			}
		}
	}
	else
	{
		/* process each keyword in the temporary buffer*/
		for (ptr=strtok(temp,SETDELIM); ptr!=NULL; ptr=strtok(NULL,SETDELIM))
		{
			bool found = false;
			KEYWORD *key;

			/* scan each of the keywords in the set */
			for (key=keys; key!=NULL; key=key->next)
			{
				if (strcmp(ptr,key->name)==0)
				{
					value |= key->value;
					count ++;
					found = true;
					break; /* we found our key */
				}
			}
			if (!found)
			{
				output_error("set member '%s' is not a keyword of property %s", ptr, prop->name);
				return 0;
			}
		}
	}
	*(uint32*)data = value;
	return count;
}

/** Convert from an \e int16
	Converts an \e int16 property to a string.  
	@return the number of character written to the string
 **/
int convert_from_int16(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[1025];
	int count = sprintf(temp,"%hd",*(short*)data);
	if(count < size - 1){
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	} else {
		return 0;
	}
}

/** Convert to an \e int16
	Converts a string to an \e int16 property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_int16(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	return sscanf(buffer,"%hd",data);
}

/** Convert from an \e int32
	Converts an \e int32 property to a string.  
	@return the number of character written to the string
 **/
int convert_from_int32(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[1025];
	int count = sprintf(temp,"%ld",*(int*)data);
	if(count < size - 1){
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	} else {
		return 0;
	}
}

/** Convert to an \e int32
	Converts a string to an \e int32 property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_int32(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	return sscanf(buffer,"%ld",data);
}

/** Convert from an \e int64
	Converts an \e int64 property to a string.  
	@return the number of character written to the string
 **/
int convert_from_int64(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[1025];
	int count = sprintf(temp,"%" FMT_INT64 "d",*(int64*)data);
	if(count < size - 1){
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	} else {
		return 0;
	}
}

/** Convert to an \e int64
	Converts a string to an \e int64 property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_int64(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	return sscanf(buffer,"%" FMT_INT64 "d",data);
}

/** Convert from a \e char8
	Converts a \e char8 property to a string.  
	@return the number of character written to the string
 **/
int convert_from_char8(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[1025];
	const char *format = "%s";
	int count = 0;
	if (strchr((char*)data,' ')!=NULL || strchr((char*)data,';')!=NULL || ((char*)data)[0]=='\0')
		format = "\"%s\"";
	count = sprintf(temp,format,(char*)data);
	if(count > size - 1){
		return 0;
	} else {
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	}
}

/** Convert to a \e char8
	Converts a string to a \e char8 property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_char8(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char c=((char*)buffer)[0];
	switch (c) {
	case '\0':
		return ((char*)data)[0]='\0', 1;
	case '"':
		return sscanf(buffer+1,"%8[^\"]",data);
	default:
		return sscanf(buffer,"%8s",data);
	}
}

/** Convert from a \e char32
	Converts a \e char32 property to a string.  
	@return the number of character written to the string
 **/
int convert_from_char32(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[1025];
	const char *format = "%s";
	int count = 0;
	if (strchr((char*)data,' ')!=NULL || strchr((char*)data,';')!=NULL || ((char*)data)[0]=='\0')
		format = "\"%s\"";
	count = sprintf(temp,format,(char*)data);
	if(count > size - 1){
		return 0;
	} else {
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	}
}

/** Convert to a \e char32
	Converts a string to a \e char32 property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_char32(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char c=((char*)buffer)[0];
	switch (c) {
	case '\0':
		return ((char*)data)[0]='\0', 1;
	case '"':
		return sscanf(buffer+1,"%32[^\"]",data);
	default:
		return sscanf(buffer,"%32s",data);
	}
}

/** Convert from a \e char256
	Converts a \e char256 property to a string.  
	@return the number of character written to the string
 **/
int convert_from_char256(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[1025];
	const char *format = "%s";
	int count = 0;
	if (strchr((char*)data,' ')!=NULL || strchr((char*)data,';')!=NULL || ((char*)data)[0]=='\0')
		format = "\"%s\"";
	count = sprintf(temp,format,(char*)data);
	if(count > size - 1){
		return 0;
	} else {
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	}
}

/** Convert to a \e char256
	Converts a string to a \e char256 property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_char256(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char c=((char*)buffer)[0];
	switch (c) {
	case '\0':
		return ((char*)data)[0]='\0', 1;
	case '"':
		return sscanf(buffer+1,"%256[^\"]",data);
	default:
		//return sscanf(buffer,"%256s",data);
		return sscanf(buffer,"%256[^\n\r;]",data);
	}
}

/** Convert from a \e char1024
	Converts a \e char1024 property to a string.  
	@return the number of character written to the string
 **/
int convert_from_char1024(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char temp[4097];
	const char *format = "%s";
	int count = 0;
	if (strchr((char*)data,' ')!=NULL || strchr((char*)data,';')!=NULL || ((char*)data)[0]=='\0')
		format = "\"%s\"";
	count = sprintf(temp,format,(char*)data);
	if(count > size - 1){
		return 0;
	} else {
		memcpy(buffer, temp, count);
		buffer[count] = 0;
		return count;
	}
}

/** Convert to a \e char1024
	Converts a string to a \e char1024 property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_char1024(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	char c=((char*)buffer)[0];
	switch (c) {
	case '\0':
		return ((char*)data)[0]='\0', 1;
	case '"':
		return sscanf(buffer+1,"%1024[^\"]",data);
	default:
		return sscanf(buffer,"%1024[^\n]",data);
	}
}

/** Convert from an \e object
	Converts an \e object reference to a string.  
	@return the number of character written to the string
 **/
int convert_from_object(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	OBJECT *obj = (data ? *(OBJECT**)data : NULL);
	char temp[256];
	memset(temp, 0, 256);
	if (obj==NULL)
		return 0;

	/* get the object's namespace */
	if (object_current_namespace()!=obj->space)
	{
		if (object_get_namespace(obj,buffer,size))
			strcat(buffer,"::");
	}
	else
		strcpy(buffer,"");

	if (obj->name != NULL){
		size_t a = strlen(obj->name);
		size_t b = size - 1;
		//if ((strlen(obj->name) != 0) && (strlen(obj->name) < (size_t)(size - 1))){
		if((a != 0) && (a < b)){
			strcat(buffer, obj->name);
			return 1;
		}
	}

	/* construct the object's name */
	if (sprintf(temp,global_object_format,obj->oclass->name,obj->id)<size)
		strcat(buffer,temp);
	else
		return 0;
	return 1;
}

/** Convert to an \e object
	Converts a string to an \e object property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_object(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	CLASSNAME cname;
	OBJECTNUM id;
	OBJECT **target = (OBJECT**)data;
	char oname[256];
	if (sscanf(buffer,"\"%[^\"]\"",oname)==1 || (strchr(buffer,':')==NULL && strncpy(oname,buffer,sizeof(oname))))
	{
		oname[sizeof(oname)-1]='\0'; /* terminate unterminated string */
		*target = object_find_name(oname);
		return (*target)!=NULL;
	}
	else if (sscanf(buffer,global_object_scan,cname,&id)==2)
	{
		OBJECT *obj = object_find_by_id(id);
		if(obj == NULL){ /* failure case, make noisy if desired. */
			*target = NULL;
			return 0;
		}
		if (obj!=NULL && strcmp(obj->oclass->name,cname)==0)
		{
			*target=obj;
			return 1;
		}
	}
	else
		*target = NULL;
	return 0;
}

/** Convert from a \e delegated data type
	Converts a \e delegated data type reference to a string.  
	@return the number of character written to the string
 **/
int convert_from_delegated(char *buffer, /**< pointer to the string buffer */
						int size, /**< size of the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	DELEGATEDVALUE *value = (DELEGATEDVALUE*)data;
	if (value==NULL || value->type==NULL || value->type->to_string==NULL) 
		return 0;
	else
		return (*(value->type->to_string))(value->data,buffer,size);
}

/** Convert to a \e delegated data type
	Converts a string to a \e delegated data type property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_delegated(const char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	DELEGATEDVALUE *value = (DELEGATEDVALUE*)data;
	if (value==NULL || value->type==NULL || value->type->from_string==NULL) 
		return 0;
	else
		return (*(value->type->from_string))(value->data,buffer);
}

/** Convert from a \e boolean data type
	Converts a \e boolean data type reference to a string.  
	@return the number of characters written to the string
 **/
int convert_from_boolean(char *buffer, int size, void *data, PROPERTY *prop){
	unsigned int b = 0;
	if(buffer == NULL || data == NULL || prop == NULL)
		return 0;
	b = *(bool *)data;
	if(b == 1 && (size > 4)){
		return sprintf(buffer, "TRUE");
	}
	if(b == 0 && (size > 5)){
		return sprintf(buffer, "FALSE");
	}
	return 0;
}

/** Convert to a \e boolean data type
	Converts a string to a \e boolean data type property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
/* booleans are handled internally as 1-byte uchar's. -MH */
int convert_to_boolean(const char *buffer, void *data, PROPERTY *prop)
{
	char str[32];
	if ( sscanf(buffer,"%31[A-Za-z]",str)==1 )
	{
		if ( stricmp(str, "TRUE")==0 )
		{
			*(bool *)data = 1;
			return 1;
		}
		if ( stricmp(str, "FALSE")==0 )
		{
			*(bool *)data = 0;
			return 1;
		}
		return 0;
	}

	int v;
	if ( sscanf(buffer,"%d",&v)==1 )
	{
		*(bool*)data = (v!=0);
		return 1;
	}

	return 0;
}

int convert_from_timestamp_stub(char *buffer, int size, void *data, PROPERTY *prop){
	TIMESTAMP ts = *(int64 *)data;
	return convert_from_timestamp(ts, buffer, size);
	//return 0;
}

int convert_to_timestamp_stub(const char *buffer, void *data, PROPERTY *prop)
{
	TIMESTAMP ts = convert_to_timestamp(buffer);
	*(int64 *)data = ts;
	return 1;
}

/** Convert from a \e double_array data type
	Converts a \e double_array data type reference to a string.  
	@return the number of character written to the string
 **/
int convert_from_double_array(char *buffer, int size, void *data, PROPERTY *prop)
{
	double_array *a = (double_array*)data;
	unsigned int n,m;
	unsigned int p = 0;
	for ( n=0 ; n<a->get_rows() ; n++ )
	{
		for ( m=0 ; m<a->get_cols() ; m++ )
		{
			if ( a->is_nan(n,m) )
				p += sprintf(buffer+p,"%s","NAN");
			else
				p += convert_from_double(buffer+p,size,(void*)a->get_addr(n,m),prop);
			if ( m<a->get_cols()-1 ) strcpy(buffer+p++," ");
		}
		if ( n<a->get_rows()-1 ) strcpy(buffer+p++,";");
	}
	return p;
}

/** Convert to a \e double_array data type
	Converts a string to a \e double_array data type property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_double_array(const char *buffer, void *data, PROPERTY *prop)
{
	double_array *a=(double_array*)data;
	unsigned row=0, col=0;
	const char *p = buffer;
	
	/* new array */
	/* parse input */
	for ( p=buffer ; *p!='\0' ; )
	{
		char value[256];
		char objectname[64], propertyname[64];
		while ( *p!='\0' && isspace(*p) ) p++; /* skip spaces */
		if ( *p!='\0' && sscanf(p,"%s",value)==1 )
		{

			if ( *p==';' ) /* end row */
			{
				row++;
				col=0;
				p++;
				continue;
			}
			else if ( strnicmp(p,"NAN",3)==0 ) /* NULL value */
			{
				a->grow_to(row,col);
				a->clr_at(row,col);
				col++;
			}
			else if ( isdigit(*p) || *p=='.' || *p=='-' || *p=='+' ) /* probably real value */
			{
				a->grow_to(row,col);
				a->set_at(row,col,atof(p));
				col++;
			}
			else if ( sscanf(value,"%[^.].%[^; \t]",objectname,propertyname)==2 ) /* object property */
			{
				OBJECT *obj = object_find_name(objectname);
				PROPERTY *prop;
				if ( obj==NULL )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d - object '%s' not found", buffer,row,col,objectname);
					return 0;
				}
				prop = object_get_property(obj,propertyname,NULL);
				if ( prop==NULL )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d - property '%s' not found in object '%s'", buffer,row,col,propertyname,objectname);
					return 0;
				}
				a->grow_to(row,col);
				a->set_at(row,col,object_get_double(obj,prop));
				if ( a->is_nan(row,col) )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object '%s' is not accessible", buffer,row,col,propertyname,objectname);
					return 0;
				}
				col++;
			}
			else if ( sscanf(value,"%[^; \t]",propertyname)==1 ) /* object property */
			{
				GLOBALVAR *var = global_find(propertyname);
				if ( var==NULL )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d global '%s' not found", buffer,row,col,propertyname);
					return 0;
				}
				a->grow_to(row,col);
				a->set_at(row,col,(double*)var->prop->addr);
				if ( a->is_nan(row,col) )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object '%s' is not accessible", buffer,row,col,propertyname,objectname);
					return 0;
				}
				col++;
			}
			else /* not a valid entry */
			{
				output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d is not valid (value='%10s')", buffer,row,col,p);
				return 0;
			}
			while ( *p!='\0' && !isspace(*p) && *p!=';' ) p++; /* skip characters just parsed */
		}
	}
	return 1;
}

/** Convert from a \e complex_array data type
	Converts a \e complex_array data type reference to a string.  
	@return the number of character written to the string
 **/
int convert_from_complex_array(char *buffer, int size, void *data, PROPERTY *prop)
{
	complex_array *a = (complex_array*)data;
	unsigned int n,m;
	unsigned int p = 0;
	for ( n=0 ; n<a->get_rows() ; n++ )
	{
		for ( m=0 ; m<a->get_cols() ; m++ )
		{
			if ( a->is_nan(n,m) )
				p += sprintf(buffer+p,"%s","NAN");
			else
				p += convert_from_complex(buffer+p,size,(void*)a->get_addr(n,m),prop);
			if ( m<a->get_cols()-1 ) strcpy(buffer+p++," ");
		}
		if ( n<a->get_rows()-1 ) strcpy(buffer+p++,";");
	}
	return p;
}

/** Convert to a \e complex_array data type
	Converts a string to a \e complex_array data type property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_complex_array(const char *buffer, void *data, PROPERTY *prop)
{
	complex_array *a=(complex_array*)data;
	unsigned row=0, col=0;
	const char *p = buffer;
	
	/* new array */
	/* parse input */
	for ( p=buffer ; *p!='\0' ; )
	{
		char value[256];
		char objectname[64], propertyname[64];
		complex c;
		while ( *p!='\0' && isspace(*p) ) p++; /* skip spaces */
		if ( *p!='\0' && sscanf(p,"%s",value)==1 )
		{

			if ( *p==';' ) /* end row */
			{
				row++;
				col=0;
				p++;
				continue;
			}
			else if ( strnicmp(p,"NAN",3)==0 ) /* NULL value */
			{
				a->grow_to(row,col);
				a->clr_at(row,col);
				col++;
			}
			else if ( convert_to_complex(value,(void*)&c,prop) ) /* probably real value */
			{
				a->grow_to(row,col);
				a->set_at(row,col,c);
				col++;
			}
			else if ( sscanf(value,"%[^.].%[^; \t]",objectname,propertyname)==2 ) /* object property */
			{
				OBJECT *obj = object_find_name(objectname);
				PROPERTY *prop;
				if ( obj==NULL )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d - object '%s' not found", buffer,row,col,objectname);
					return 0;
				}
				prop = object_get_property(obj,propertyname,NULL);
				if ( prop==NULL )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d - property '%s' not found in object '%s'", buffer,row,col,propertyname,objectname);
					return 0;
				}
				a->grow_to(row,col);
				a->set_at(row,col,object_get_complex(obj,prop));
				if ( a->is_nan(row,col) )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object '%s' is not accessible", buffer,row,col,propertyname,objectname);
					return 0;
				}
				col++;
			}
			else if ( sscanf(value,"%[^; \t]",propertyname)==1 ) /* object property */
			{
				GLOBALVAR *var = global_find(propertyname);
				if ( var==NULL )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d global '%s' not found", buffer,row,col,propertyname);
					return 0;
				}
				a->grow_to(row,col);
				a->set_at(row,col,(complex*)var->prop->addr);
				if ( a->is_nan(row,col) )
				{
					output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d property '%s' in object '%s' is not accessible", buffer,row,col,propertyname,objectname);
					return 0;
				}
				col++;
			}
			else /* not a valid entry */
			{
				output_error("convert_to_double_array(const char *buffer='%10s...',...): entry at row %d, col %d is not valid (value='%10s')", buffer,row,col,p);
				return 0;
			}
			while ( *p!='\0' && !isspace(*p) && *p!=';' ) p++; /* skip characters just parsed */
		}
	}
	return 1;
}

/** Convert a string to a double with a given unit
   @return 1 on success, 0 on failure
 **/
extern "C" int convert_unit_double(char *buffer,char *unit, double *data)
{
	char *from = strchr(buffer,' ');
	*data = atof(buffer);

	if (from==NULL)
		return 1; /* no conversion needed */
	
	/* skip white space in from of unit */
	while (isspace(*from)) from++;

	return unit_convert(from,unit,data);
}

/** Convert a struct object to a string
	The structure is defined as a linked list of PROPERTY entities
	@return length of string on success, 0 for empty, <0 for failure
 **/
int convert_from_struct(char *buffer, size_t len, void *data, PROPERTY *prop)
{
	size_t pos = sprintf(buffer,"%s","{ ");
	while ( prop!=NULL )
	{
		void *addr = (char*)data + (size_t)prop->addr;
		PROPERTYSPEC *spec = property_getspec(prop->ptype);
		char temp[1025];
		size_t n = spec->data_to_string(temp,sizeof(temp),addr,prop);
		if ( pos+n >= len-2 )
			return -pos;
		pos += sprintf(buffer+pos,"%s %s; ",prop->name,temp);
		prop = prop->next;
	}
	strcpy(buffer+pos,"}");
	return pos+1;
}
/** Convert a string to a struct object
	The structure is defined as a linked list of PROPERTY entities
	@return length of string on success, 0 for empty, -1 for failure
 **/int convert_to_struct(const char *buffer, void *data, PROPERTY *structure)
{
	size_t len = 0;
	char temp[1025];
	if ( buffer[0]!='{' ) return -1;
	strncpy(temp,buffer+1,sizeof(temp));
	char *item = NULL;
	char *last = NULL;
	while ( (item=strtok_s(item?NULL:temp,";",&last))!=NULL )
	{
		char name[64], value[1024];
		while ( isspace(*item) ) item++;
		if ( *item=='}' ) return len;
		if ( sscanf(item,"%s %[^\n]",name,value)!=2 )
			return -len;
		PROPERTY *prop;
		for ( prop=structure ; prop!=NULL ; prop=prop->next )
		{
			if ( strcmp(prop->name,name)==0 )
			{
				void *addr = (char*)data + (size_t)prop->addr;
				PROPERTYSPEC *spec = property_getspec(prop->ptype);
				len += spec->string_to_data(value,addr,prop);
				break;
			}
		}
		if ( prop==NULL ) return -len;
	}
	return -len;
}

/**@}**/
