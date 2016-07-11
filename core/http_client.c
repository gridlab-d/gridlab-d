/* $Id: http_client.c 4738 2014-07-03 00:55:39Z dchassin $
 */

#include <stdio.h>


#include "output.h"
#include "http_client.h"
#include "server.h"


HTTP* hopen(char *url, int maxlen)
{
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char protocol[64];
	char hostname[1024];
	char filespec[1024];
	HTTP *http;
	char request[1024];
	int len;

#ifdef WIN32
	/* init sockets */
	WORD wsaVersion = MAKEWORD(2,2);
	WSADATA wsaData;
	if ( WSAStartup(wsaVersion,&wsaData)!=0 )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): unable to initialize Windows sockets", url, maxlen);
		return NULL;
	}
#endif

	/* extract URL parts */
	if ( sscanf(url,"%63[^:]://%1023[^/]%1023s", protocol,hostname, filespec)!=3 )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): badly formed URL", url, maxlen);
		return NULL;
	}

	/* setup handle */
	http = (HTTP*)malloc(sizeof(HTTP));
	if ( http==NULL )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): memory allocation failure", url, maxlen);
		return NULL;
	}

	/* initialize http */
	http->buf = NULL;
	http->len = 0;
	http->pos = 0;

	/* setup socket */
	http->sd = socket(AF_INET, SOCK_STREAM, 0);
	if ( http->sd < 0 )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): unable to create socket", url, maxlen);
		goto Error;
	}

	/* find server */
	server = gethostbyname(hostname);
	if ( server==NULL )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): server not found", url, maxlen);
		goto Error;
	}

	/* extract server address */
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char*)&serv_addr.sin_addr.s_addr,(char*)server->h_addr,server->h_length);
	serv_addr.sin_port = htons(80);

	/* open connection */
	if ( connect(http->sd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0 )
	{
		output_error("hopen(char *url='%s', maxlen=%d): unable to connect to server", url, maxlen);
		goto Error;
	}

	/* format/send request */
	len = sprintf(request,"GET %s HTTP/1.1\r\nHost: %s:80\r\nUser-Agent: GridLAB-D/%d.%d\r\nConnection: close\r\n\r\n",filespec,hostname,REV_MAJOR,REV_MINOR);
	output_debug("sending HTTP message \n%s", request);
	if ( send(http->sd,request,len,0)<len )
	{
		output_error("hopen(char *url='%s', maxlen=%d): unable to send request", url, maxlen);
		goto Error;
	}

	/* read response header */
	http->buf = (char*)malloc(maxlen+1);
	http->len = 0;
	do {
		len = recv(http->sd,http->buf+http->len,(int)(maxlen-http->len),0x00);
		if ( len==0 )
		{
			output_debug("hopen(char *url='%s', maxlen=%d): connection closed", url, maxlen);
		}
		else if ( len<0 )
		{
			output_error("hopen(char *url='%s', maxlen=%d): unable to read response", url, maxlen);
			goto Error;
		}
		else
		{
			http->len += len;
			http->buf[http->len] = '\0';
		}
	} while ( len>0 );

	if ( http->len>0 )
		output_debug("received HTTP message \n%s", http->buf);
	else
		output_debug("no HTTP response received");
	return http;
Error:
#ifdef WIN32
		output_error("hopen(char *url='%s', int maxlen=%d): WSA error code %d", url, maxlen, WSAGetLastError());
#endif
	return NULL;
}
int hclose(HTTP*http)
{
	if ( http )
	{
		if ( http->sd ) 
#ifdef WIN32
			closesocket(http->sd);
#else
			close(http->sd);
#endif
		if ( http->buf!=NULL ) free(http->buf);
		free(http);
	}
	return 1;
}
size_t hfilelength(HTTP*http)
{
	return http->len;
}
int heof(HTTP*http)
{
	return http->pos>=http->len;
}
size_t hread(char *buffer, size_t size, HTTP* http)
{
	size_t len = http->len - http->pos;
	if ( http->pos >= http->len )
		return 0;
	if ( len > size )
		len = size;
	memcpy(buffer,http->buf+http->pos,len);
	http->pos += len;
	return len;
}


/* URL access */
HTTPRESULT *http_new_result(void)
{
	HTTPRESULT *result = malloc(sizeof(HTTPRESULT));
	result->body.data = NULL;
	result->body.size = 0;
	result->header.data = NULL;
	result->header.size = 0;
	result->status = 0;
	return result;
}

void http_delete_result(HTTPRESULT *result)
{
	if ( result->body.size>0 )
		free(result->body.data);
	if ( result->header.size>0 )
		free(result->header.data);
	free(result);
}

HTTPRESULT *http_read(char *url, int maxlen)
{
	HTTPRESULT *result = http_new_result();
	
	if ( strncmp(url,"http://",7)==0 )
	{
		HTTP *fp = hopen(url,maxlen);
		if ( fp==NULL )
		{
			output_error("http_read('%s'): unable to access url", url);
			result->status = errno;
		}
		else
		{
			size_t len = (int)hfilelength(fp);
			size_t hlen = len;
			char *buffer = (char*)malloc(len+1);
			char *data = NULL;
			if ( hread(buffer,len,fp)<=0 )
			{
				output_error("http_read('%s'): unable to read file", url);
				result->status = errno;
			}
			else 
			{
				buffer[len]='\0';
				data = strstr(buffer,"\r\n\r\n");
				if ( data!=NULL )
				{
					hlen = data - buffer;
					*data++ = '\0';
					result->body.size = (int)(len - hlen - 1);
					while ( isspace(*data) ) { data++; result->body.size--; }
//					if ( sscanf(data,"%x",&dlen)==1 )
//					{
//						data = strchr(data,'\n');
//						while ( isspace(*data) ) {data++;}
//						result->body.size = dlen;
//						result->body.data = malloc(dlen+1);
//					}
//					else
//					{
						result->body.data = malloc(result->body.size+1);
//					}
					memcpy(result->body.data,data,result->body.size);
					result->body.data[result->body.size] = '\0';
				}
				else
				{
					result->body.data = "";
					result->body.size = 0;
				}
				result->header.size = (int)hlen;
				result->header.data = malloc(hlen+1);
				strcpy(result->header.data,buffer);
				result->status = 0;
			}
		}
		hclose(fp);
	}

	else if ( strncmp(url,"file://",7)==0 )
	{
		FILE *fp = fopen(url+7,"rt");
		if ( fp==NULL )
		{
			output_error("http_read('%s'): unable to access file", url);
			result->status = errno;
		}
		else
		{
			result->body.size = filelength(fileno(fp))+1;
			result->body.data = malloc(result->body.size);
			memset(result->body.data,0,result->body.size);
			if ( fread(result->body.data,1,result->body.size,fp)<=0 )
			{
				output_error("http_read('%s'): unable to read file", url);
				result->status = errno;
			}
			else
				result->status = 0;
		}
		fclose(fp);
	}
	return result;
}
const char * http_get_header_data(HTTPRESULT *result, const char* param)
{
	static char buffer[1024]="";
	char target[256];
	char *ptr;
	if ( param==NULL ) return result->header.data;
	sprintf(target,"\n%s: ",param);
	ptr = strstr(result->header.data,target);
	if ( ptr==NULL ) return NULL;
	sscanf(ptr+strlen(param)+3,"%1023[^\r\n]",buffer);
	return buffer;
}

unsigned int http_get_status(HTTPRESULT *result)
{
	output_debug("http_get_status(): '%s'", result->header.data+9);
	return atoi(result->header.data+9);
}

static unsigned int64 wget_maxsize = 100000000;
static enum {WU_NEVER, WU_NEWER, WU_ALWAYS} wget_update = WU_NEWER;
static char wget_cachedir[1024]="-";

void http_get_options(void)
{
	char *option=NULL, *last=NULL;
	while ( (option=strtok_s(option==NULL?global_wget_options:NULL,";",&last))!=NULL )
	{
		char name[1024], value[1024];
		int n = sscanf(option,"%[^:]:%[^\n\r]",name,value);
		if ( n>0 )
		{
			if ( strcmp(name,"maxsize")==0 )
			{
				if ( n>1 )
				{
					char unit[1024]="";
					int m = sscanf(value,"%lu%[A-Za-z]",&wget_maxsize,unit);
					if ( m==2 )
					{
						if ( strcmp(unit,"kB")==0 ) wget_maxsize*=1000;
						else if ( strcmp(unit,"MB")==0 ) wget_maxsize*=1000000;
						else if ( strcmp(unit,"GB")==0 ) wget_maxsize*=1000000000;
						else if ( strcmp(unit,"TB")==0 ) wget_maxsize*=1000000000000;
						else
							output_error("wget_options: unit '%s' of maxsize are not valid", unit);
					}
					else if ( m==0 )
						output_error("wget_options: unable to read maxsize value '%s'", value);
				}
				else
					output_error("wget_options: option '%s' requires a value", name);
			}
			else if ( strcmp(name,"update")==0 )
			{
				if ( n>1 )
				{
					if ( strcmp(value,"never")==0 )
						wget_update = WU_NEVER;
					else if ( strcmp(value,"newer")==0 )
						wget_update = WU_NEWER;
					else if ( strcmp(value,"always")==0 )
						wget_update = WU_ALWAYS;
					else
						output_error("wget_options: option value '%s' is not a valid update option", value);
				}
				else
					output_error("wget_options: option '%s' requires a value", name);
			}
			else if ( strcmp(name,"cachedir")==0 )
			{
				if ( n>1 )
					strncpy(wget_cachedir,value,sizeof(wget_cachedir)-1);
				else
					output_error("wget_options: option '%s' requires a value", name);
			}
			else
			{
				output_error("wget_options: invalid option '%s' is ignored", name);
			}
		}
	}
	if ( strcmp(wget_cachedir,"-")==0 ) strcpy(wget_cachedir,global_workdir);
	output_debug("wget_option maxsize = %d", wget_maxsize);
	output_debug("wget_option update = %d", wget_update);
	output_debug("wget_option cachedir = '%s'", wget_cachedir);
	/** @todo read options from global_wget_options */
}

time_t http_read_datetime(const char *timestamp)
{
	/* WWW, dd mmm yyyy HH:MM:SS GMT */
	struct tm dt;
	char month[4], tzone[16];
	char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	if ( timestamp==NULL ) return 0;
	if ( sscanf(timestamp,"%*[A-Za-z], %d %3s %d %d:%d:%d %15s", &dt.tm_mday, month, &dt.tm_year, &dt.tm_hour, &dt.tm_min, &dt.tm_sec, tzone)!=7 )
	{
		output_debug("http_read_datetime(const char *timestamp='%s'): unable to parse string", timestamp);
		return 0;
	}
	dt.tm_year -= 1900;
	for ( dt.tm_mon=0 ; dt.tm_mon<12 ; dt.tm_mon++ )
	{
		if ( strcmp(months[dt.tm_mon],month)==0 )
			break;
	}
	if ( dt.tm_mon==12 )
	{
		output_debug("http_read_datetime(const char *timestamp='%s'): month not recognized", timestamp);
		return 0;
	}
	else
		return mktime(&dt);
}

int http_saveas(char *url, char *file)
{
	HTTPRESULT *result;
	unsigned int status;
// TODO finish update test
//	struct stat st;
	time_t modtime;
	
	http_get_options();

// TODO finish update test
//	if ( wget_update==WU_NEVER && stat(file,&st)==0 )
//		return 1;

	result = http_read(url,0x4000); /* read only enough to get header */
	if ( result==NULL )
	{
		output_error("http_saveas(char *url='%s', char *file='%s'): hopen failed", url, file);
		return 0;
	}
	else if ( (status=http_get_status(result))==200 )
	{
		const char *cl = http_get_header_data(result,"Content-Length");
		size_t len = (cl?atoi(cl):0);
		FILE *fp;

		output_debug("URL '%s' save as '%s'...", url, file);
		output_debug("  result status: %d", status);
		output_debug("  result length: %d", len);
		output_debug("  result last modified: %s", ctime(&modtime));

		modtime = http_read_datetime(http_get_header_data(result,"Last-Modified"));
// TODO finish update test
//		output_debug("  file mtime: %s", ctime(&st.st_mtime));
//		if ( wget_update==WU_NEVER && st.st_mtime>0 && st.st_mtime>modtime )
//			return 1;
	
		result = http_read(url,wget_maxsize);
		fp = fopen(file,"w");
	
		if ( fp==NULL )
		{
			output_error("unable to write file '%s': %s", file, strerror(errno));
			return 0;
		}
		else
		{
			if ( result->header.size+result->body.size<len )
				output_warning("URL '%s' is larger than maxsize %d", url, wget_maxsize);
			else
				output_verbose("URL '%s' saved to '%s' ok", url,file);
			fwrite(result->body.data,1,result->body.size,fp);
			fclose(fp);
			return 1;
		}
	}
	else
	{
		output_error("URL '%s' return code %d", url, status);
		return 0;
	}
}
