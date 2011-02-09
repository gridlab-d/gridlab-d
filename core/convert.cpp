/** $Id: convert.c 1207 2009-01-12 22:47:29Z d3p988 $
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
int convert_to_void(char *buffer, /**< a pointer to the string buffer that is ignored */
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

		if(prop->unit != ptmp->unit && ptmp->unit != NULL){
			if(0 == unit_convert_ex(ptmp->unit, prop->unit, &scale)){
				output_error("convert_from_double(): unable to convert unit '%s' to '%s' for property '%s' (tape experiment error)", ptmp->unit->name, prop->unit->name, prop->name);
				scale = 1.0;
			}
		}
	}

	count = sprintf(temp, global_double_format, *(double *)data * scale);
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
int convert_to_double(char *buffer, /**< a pointer to the string buffer */
					  void *data, /**< a pointer to the data */
					  PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	return sscanf(buffer,"%lg",data);
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
	if(prop->unit != NULL){

		/* only do conversion if the target unit differs from the class's unit for that property */
		PROPERTY *ptmp = (prop->oclass==NULL ? prop : class_find_property(prop->oclass, prop->name));
	
		if(prop->unit != ptmp->unit){
			if(0 == unit_convert_ex(ptmp->unit, prop->unit, &scale)){
				output_error("convert_from_complex(): unable to convert unit '%s' to '%s' for property '%s' (tape experiment error)", ptmp->unit->name, prop->unit->name, prop->name);
				/*	TROUBLESHOOTING
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
int convert_to_complex(char *buffer, /**< a pointer to the string buffer */
					   void *data, /**< a pointer to the data */
					   PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	complex *v = (complex*)data;
	char notation[2]={'\0','\0'}; /* force detection invalid complex number */
	int n;
	double a=0, b=0; 
	if(buffer[0] == 0){
		/* empty string */
		v->SetRect(0.0, 0.0);
		return 1;
	}
	n = sscanf(buffer,"%lg%lg%1[ijdr]",&a,&b,notation);
	if (n==1) /* only real part */
		v->SetRect(a,0);
	else if (n < 3 || strchr("ijdr",notation[0])==NULL) /* incomplete imaginary part or missing notation */
	{
		output_error("convert_to_complex('%s',%s): complex number format is not valid", buffer,prop->name);
		/* TROUBLESHOOTING
			A complex number was given that doesn't meet the formatting requirements, e.g., <real><+/-><imaginary><notation>.  
			Check the format of your complex numbers and try again.
		 */
		return 0;
	}
	/* appears ok */
	else if (notation[0]==A) /* polar degrees */
		v->SetPolar(a,b*PI/180.0); 
	else if (notation[0]==R) /* polar radians */
		v->SetPolar(a,b); 
	else 
		v->SetRect(a,b); /* rectangular */
	if (v->Notation()==I) /* only override notation when property is using I */
		v->Notation() = (CNOTATION)notation[0];
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
int convert_to_enumeration(char *buffer, /**< a pointer to the string buffer */
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
			*(uint32*)data=keys->value;
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
			value -= keys->value;

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
int convert_to_set(char *buffer, /**< a pointer to the string buffer */
					    void *data, /**< a pointer to the data */
					    PROPERTY *prop) /**< a pointer to keywords that are supported */
{
	KEYWORD *keys=prop->keywords;
	char temp[4096], *ptr;
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
int convert_to_int16(char *buffer, /**< a pointer to the string buffer */
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
int convert_to_int32(char *buffer, /**< a pointer to the string buffer */
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
int convert_to_int64(char *buffer, /**< a pointer to the string buffer */
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
	char *format = "%s";
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
int convert_to_char8(char *buffer, /**< a pointer to the string buffer */
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
	char *format = "%s";
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
int convert_to_char32(char *buffer, /**< a pointer to the string buffer */
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
	char *format = "%s";
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
int convert_to_char256(char *buffer, /**< a pointer to the string buffer */
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
	char *format = "%s";
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
int convert_to_char1024(char *buffer, /**< a pointer to the string buffer */
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
int convert_to_object(char *buffer, /**< a pointer to the string buffer */
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
int convert_to_delegated(char *buffer, /**< a pointer to the string buffer */
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
int convert_to_boolean(char *buffer, void *data, PROPERTY *prop){
	char str[32];
	int i = 0;
	if(buffer == NULL || data == NULL || prop == NULL)
		return 0;
	memcpy(str, buffer, 31);
	for(i = 0; i < 31; ++i){
		if(str[i] == 0)
			break;
		str[i] = toupper(str[i]);
	}
	if(0 == strcmp(str, "TRUE")){
		*(bool *)data = 1;
		return 1;
	}
	if(0 == strcmp(str, "FALSE")){
		*(bool *)data = 0;
		return 1;
	}
	return 0;
}

int convert_from_timestamp_stub(char *buffer, int size, void *data, PROPERTY *prop){
	TIMESTAMP ts = *(int64 *)data;
	return convert_from_timestamp(ts, buffer, size);
	//return 0;
}

int convert_to_timestamp_stub(char *buffer, void *data, PROPERTY *prop){
	TIMESTAMP ts = convert_to_timestamp(buffer);
	*(int64 *)data = ts;
	return 1;
}

/** Convert from a \e double_array data type
	Converts a \e double_array data type reference to a string.  
	@return the number of character written to the string
 **/
int convert_from_double_array(char *buffer, int size, void *data, PROPERTY *prop){
	int i = 0;
	return 0;
}

/** Convert to a \e double_array data type
	Converts a string to a \e double_array data type property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_double_array(char *buffer, void *data, PROPERTY *prop){
	return 0;
}

/** Convert from a \e complex_array data type
	Converts a \e complex_array data type reference to a string.  
	@return the number of character written to the string
 **/
int convert_from_complex_array(char *buffer, int size, void *data, PROPERTY *prop){
	return 0;
}

/** Convert to a \e complex_array data type
	Converts a string to a \e complex_array data type property.  
	@return 1 on success, 0 on failure, -1 if conversion was incomplete
 **/
int convert_to_complex_array(char *buffer, void *data, PROPERTY *prop){
	return 0;
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

/**@}**/
