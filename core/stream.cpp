/* $Id: stream.cpp 4738 2014-07-03 00:55:39Z dchassin $
 *
 */

#include "output.h"
#include "stream.h"
#include "module.h"
#include "globals.h"
#include "schedule.h"
#include "transform.h"
#include "class.h"
#include "object.h"

extern "C" {
	struct s_stream {
		STREAMCALL call;
		struct s_stream *next;
	} *stream_list = NULL;

	/** Register a stream 
	 **/
	void stream_register ( STREAMCALL call )
	{
		struct s_stream *stream = (struct s_stream*)malloc(sizeof(struct s_stream));
		if ( stream==NULL ) throw "stream_register(): malloc failed";
		stream->call = call;
		stream->next = stream_list;
		stream_list = stream;
	}
	/** Stream function used by modules
	 **/
	size_t stream_callback(void *ptr, size_t len, int is_str, void *match)
	{
		try {
			return stream(ptr,len,is_str,match);
		}
		catch (...) { return -1; }
	}
}

/* stream name - this should never be changed */
#define STREAM_NAME "GRIDLABD"

/* stream version - change this when the structure of the stream changes */
#define STREAM_VERSION 2

/* stream handle */
static FILE *fp = NULL;

/* stream size */
static size_t count=0;

char *stream_context()
{
	static char buffer[64];
	//sprintf(buffer,"block %d, token %d",b,t);
	return buffer;
}
int stream_error(char *format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);
	//output_error("- stream(%d:%d) - %s", b,t,buffer);
	return -1;
}

int stream_warning(char *format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);
	//output_warning("- stream(%d:%d) - %s", b,t,buffer);
	return -1;
}

/** stream_compress

	Format of compressed stream 

	[US/len] [bit15 runlen flag, bit 0-14 data len]
	
	Bit 15 clear : raw data follows for len given
	[UC/data ... ]

	Bit 15 set : compressed data 
	[SC/delta] differential to apply to each value
	[SC/value] initial value

 **/
size_t stream_compress(char *buf, size_t len)
{
	size_t count = 0, original = len;
	char *p = buf; // current position in input buffer
	char *raw = p; // start of raw buffer
	unsigned short rawlen = 0; // len of raw buffer
	char *run = p; // start of run buffer
	unsigned short runlen = 0; // len of run buffer
	char diff = 0; // current differential run value
	enum {RAW=0, RUNLEN=1} state = RAW; // state of compression
	for ( p = buf ; len-->0 ; p++ ) 
	{
		int dp = p[1]-p[0];

		if (state == RAW)
		{
			if (dp==diff) // pattern repeats
				runlen++;
			else
				runlen=0;

			// if raw buffer in progress and run is long enough to use
			if (runlen==8)
			{
				// dump raw buffer
				if (rawlen>runlen)
				{
					rawlen -= runlen; // don't include new run data
					if (fwrite(&rawlen,1,sizeof(rawlen),fp)<0) return -1;
					if (fwrite(raw,1,rawlen,fp)<0) return -1;
					count += rawlen+2;
				}

				// change to run buffer
				state = RUNLEN;
				rawlen = 0;
				run = p-runlen;
			}

			else if (rawlen==32767)	// long raw buffer
			{
				// dump raw buffer
				if (fwrite(&rawlen,1,sizeof(rawlen),fp)<0) return -1;
				if (fwrite(raw,1,rawlen,fp)<0) return -1;
				count += rawlen+2;

				// restart raw buffer
				rawlen = 0;
				raw = p;
			}
			else
				rawlen++;

		}
		else if (state==RUNLEN)
		{
			if (dp==diff) // run continues
			{
				runlen++;

				// if run buffer is too long
				if (runlen==32767)
				{
					// mark run buffer
					runlen |= 0x8000;

					// dump run buffer
					if (fwrite(&runlen,1,sizeof(runlen),fp)<0) return -1;
					if (fwrite(&diff,1,sizeof(diff),fp)<0) return -1;
					if (fputc(*run,fp)<0) return -1; 
					count+=4;

					// start over with a new run
					runlen = 0;
					run = p;
				}
			}

			else // run ends
			{
				// mark run buffer
				runlen |= 0x8000;

				// dump run buffer
				if (fwrite(&runlen,1,sizeof(runlen),fp)<0) return -1;
				if (fwrite(&diff,1,sizeof(diff),fp)<0) return -1;
				if (fputc(*run,fp)<0) return -1; 
				count+=4;

				// change to a raw buffer
				state = RAW;
				raw = p;
				rawlen = 0;

				// get run candidate
				runlen = 0;
			}
		}

		// get next run candidate
		diff = dp;
	}
	
	// terminate compressed stream
	rawlen=0;
	if (fwrite(&rawlen,sizeof(rawlen),1,fp)<0) return -1; else count+=2;

	// stream len confirmation code
	if (fwrite(&count,sizeof(count),1,fp)<0) return -1; else count+=2;

	output_debug("stream_compress(): %d kB -> %d kB (%.1f%%)", original/1000+1, count/1000+1, (double)count*100/(double)original);
	return count;
}

/** stream_compress

	Format of compressed stream 

	[US/len] [bit15 runlen flag, bit 0-14 data len]
	
	Bit 15 clear : raw data follows for len given
	[UC/data ... ]

	Bit 15 set : compressed data 
	[SC/delta] differential to apply to each value
	[SC/value] initial value

 **/
size_t stream_decompress(char *buf, const size_t len)
{
	size_t count = 0;
	char *ptr = buf;
	size_t buflen=0;
	size_t confirm=0;
	struct {
		unsigned int runlen:15;
		unsigned int is_compressed:1;
	} runstate;
	for (buflen=0; buflen<len; buflen+=runstate.runlen)
	{
		if (fread(&runstate,2,1,fp)!=1)
			return stream_error("stream_decompress(): failed to read runlen");
		else
			count+=2;

		// check for end of compressed stream
		if (runstate.runlen==0)
			break;

		// handle run data
		if (runstate.is_compressed) // compression flag set
		{
			char delta;
			unsigned char value;
			if (fread(&delta,1,1,fp)!=1 || fread(&value,1,1,fp)!=1) 
				return stream_error("stream_decompress(): failed to read delta/value");
			else
				count += 2;
			if (delta==0)
			{
				memset(ptr,value,runstate.runlen+1);
				ptr += runstate.runlen+1;
			}
			else
			{
				unsigned short run = runstate.runlen+1;
				while (run-->0)
				{
					*ptr++ = value;
					value += delta;
				}
			}
		}
		else // no compression
		{
			if (fread(ptr,1,runstate.runlen,fp)!=runstate.runlen)
				return stream_error("stream_decompress(): failed to read raw data");
			else
			{
				ptr += runstate.runlen;
				count += runstate.runlen;
			}
		}
	}

	// check for overrun
	if (buflen>len)
		return stream_error("stream_decompress(): stream overrun--possible invalid stream");

	// read confirmation code
	if (fread(&confirm,sizeof(confirm),1,fp)==1 && confirm==count)
		return count;
	else
		return stream_error("stream_decompress(): stream confirmation code mismatched--probable invalid stream");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static char stream_name[] = STREAM_NAME;
static unsigned int stream_version = STREAM_VERSION;
static unsigned int stream_wordsize = sizeof(void*);
static size_t stream_pos = 0;
static int flags = 0x00;

/** Stream data
    @returns Bytes read/written to/from stream
 **/
size_t stream(void *ptr, ///< pointer to buffer
			  size_t len, ///< length of data (maximum when reading)
			  bool is_str, ///< flag variable length
			  void *match) ///< optional match when reading (mismatch causes an const char* exception)
{
	if ( flags&SF_OUT )
	{
#ifdef _DEBUG
		if ( is_str ) len = strlen((char*)ptr);
		unsigned int a = fprintf(fp,"%d ",len);
		if ( a<0 ) throw;
		unsigned int i;
		for ( i=0 ; i<len ; i++ )
		{
			int c = ((unsigned char*)ptr)[i];
			size_t b=1;
			if ( !is_str || c<32 || c>126 || c=='\\' )
				b = fprintf(fp,"\\%02x",c);
			else if ( fputc(c,fp)==EOF ) b=-1;
			if ( b==-1 ) throw;
			a+=b;
		}
		unsigned int b = fprintf(fp,"\n");
		if ( b<0 ) throw;
		a+=b;
		stream_pos += a;
		return a;
#else
		if ( is_str ) len = strlen((char*)ptr);
		size_t a = fwrite((void*)&len,1,sizeof(len),fp);
		if ( a!=sizeof(len) ) throw;
		size_t b = fwrite((void*)ptr,1,len,fp);
		if ( b!=len ) throw;
		a+=b;
		stream_pos += a;
		return a;
#endif
	}
	if ( flags&SF_IN ) 
	{
#ifdef _DEBUG
		unsigned int a;
		if ( fscanf(fp,"%d",&a)<1 ) throw -1;
		if ( a>len ) throw;
		if ( fgetc(fp)!=' ') throw "FMT";
		memset(ptr,0,len);
		unsigned int i;
		for ( i=0; i<a ; i++ )
		{
			int b = fgetc(fp);
			if ( b=='\\' && fscanf(fp,"%02x",&b)<1 ) throw -1;
			((char*)ptr)[i] = (char)b;
		}
		while ( fgetc(fp)!='\n' ) {}
		if ( match!=NULL && memcmp(ptr,match,a)!=0 ) throw 0;
		unsigned int b = (log((double)a)+2)+a*3;
		stream_pos += b;
		return b;
#else
		size_t a, b = fread(&a,1,sizeof(size_t),fp);
		if ( b<sizeof(size_t) ) throw -1;
		if ( a>len ) throw;
		size_t c = fread((void*)ptr,1,a,fp);
		if ( a!=c ) throw;
		if ( match!=NULL && memcmp(ptr,match,a)!=0 ) throw 0;
		b+=c;
		stream_pos += b;
		return b;
#endif
	}
	throw;
}
void stream(const char *s,size_t max=0) { char t[1024]; strncpy(t,s,sizeof(t)); stream((void*)t,max?max:strlen(s),true,(void*)s); }
void stream(char *s,size_t max=0) { stream((void*)s,max?max:strlen(s),true); }
template<class T> void stream(T &v) { stream(&v,sizeof(T)); }

// module stream
void stream(MODULE *mod)
{
	stream("MOD");

	size_t count = module_getcount();
	stream(count);
	size_t n;
	for ( n=0 ; n<count ; n++ )
	{
		char name[1024]; if (mod) strcpy(name,mod->name);
		stream(name,sizeof(name));

		if ( flags&SF_OUT ) mod = mod->next;
		if ( flags&SF_IN ) module_load(name,0,NULL);
	}
	stream("/MOD");
}

// property stream
void stream(CLASS *oclass, PROPERTY *prop)
{
	stream("RTC");

	size_t count = class_get_extendedcount(oclass);
	stream(count);
	size_t n;
	for ( n=0 ; n<count ; n++ )
	{
		PROPERTYNAME name; if ( prop ) strcpy(name,prop->name);
		stream(name,sizeof(name));

		PROPERTYTYPE ptype; if ( prop ) ptype = prop->ptype;
		stream(ptype);

		char unit[64]; if ( prop ) strcpy(unit,prop->unit?prop->unit->name:"");
		stream(unit,sizeof(unit));

		uint32 width; if ( prop ) width = prop->width;
		stream(width);

		if ( flags&SF_OUT ) prop = prop->next;
		if ( flags&SF_IN ) class_add_extended_property(oclass,name,ptype,unit);
	}
	stream("/RTC");
}

// class stream
void stream(CLASS *oclass)
{
	stream("RTC");

	size_t count = class_get_runtimecount();
	stream(count);
	size_t n;
	for ( n=0 ; n<count ; n++ )
	{
		CLASSNAME name; if ( oclass ) strcpy(name,oclass->name);
		stream(name,sizeof(name));

		unsigned int size; if ( oclass ) size = oclass->size;
		stream(size);

		PASSCONFIG passconfig; if ( oclass ) passconfig = oclass->passconfig;
		stream(passconfig);

		if ( flags&SF_IN ) oclass = class_register(NULL,name,size,passconfig);

		// TODO parent

		stream(oclass,oclass->pmap);

		if ( flags&SF_OUT ) oclass = class_get_next_runtime(oclass);
		if ( flags&SF_IN ) module_load(oclass->name,0,NULL);
	}
	stream("/RTC");
}

// object stream
void stream(OBJECT *obj)
{
	stream("OBJ");
	size_t count = object_get_count();
	stream(count);
	size_t n;
	for ( n=0 ; n<count ; n++)
	{
		char cname[64]; if ( obj ) strcpy(cname,obj->oclass->name);
		stream(cname,sizeof(cname));

		unsigned int size; if ( obj) size = sizeof(OBJECT)+obj->oclass->size;
		stream(&size,sizeof(size));

		char oname[64]; if ( obj ) strcpy(oname,obj->name?obj->name:"");
		stream(oname,sizeof(oname));

		OBJECT *data=(OBJECT*)malloc(size); if ( obj ) memcpy(data,obj,size);
		stream(data,size);

		// TODO forecast and namespace

		if ( flags&SF_OUT ) 
		{
			obj = obj->next;
			free(data);
		}
		else if ( flags&SF_IN ) 
			object_stream_fixup(data,cname,oname);
	}
	stream("/OBJ");
}

// globals stream
void stream(GLOBALVAR *var)
{
	stream("VAR");

	size_t count = global_getcount();
	stream(count);
	size_t n;
	for ( n=0 ; n<count ; n++ )
	{
		PROPERTYNAME name; if ( var ) strcpy(name,var->prop->name);
		stream(name,sizeof(name));

		char value[1024]; if ( var ) global_getvar(name,value,sizeof(value));
		stream(value,sizeof(value));

		if ( flags&SF_OUT ) var = var->next;
		if ( flags&SF_IN ) global_setvar(name,value);
	}

	stream("/VAR");
}

size_t stream(FILE *fileptr,int opts)
{
	stream_pos = 0;
	fp = fileptr;
	flags = opts;
	output_debug("starting stream on file %d with options %x", fileno(fp), flags);
	try {

		// header
		stream("GLD30");

		// runtime classes
		try { stream(class_get_first_runtime()); } catch (int) {};

		// modules
		try { stream(module_get_first()); } catch (int) {}

		// objects
		try { stream(object_get_first()); } catch (int) {};

		// globals
		try { stream(global_getnext(NULL)); } catch (int) {};

		// module data
		struct s_stream *s;
		for ( s=stream_list ; s!=NULL ; s=s->next )
		{	
			s->call((int)flags,(STREAMCALLBACK)stream_callback);
		}
		output_debug("done processing stream on file %d with options %x", fileno(fp), flags);
		return stream_pos;
	}
	catch (const char *msg)
	{
		output_error("stream() unexpected %s at offset %lld", msg, (int64)stream_pos);
		return -1;
	}
	catch (...)
	{
		output_error("stream() failed as offset %lld", (int64)stream_pos);
		return -1;
	}
}

#define stream_type(T) extern "C" size_t stream_##T(void *ptr, size_t len, PROPERTY *prop) { return stream((T*)ptr,len); }
#include "stream_type.h"
#undef stream_type
