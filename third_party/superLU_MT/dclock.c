/*
 * -- SuperLU MT routine (version 1.0) --
 * Univ. of California Berkeley, Xerox Palo Alto Research Center,
 * and Lawrence Berkeley National Lab.
 * August 15, 1997
 *
 */
#include <sys/types.h>
#ifdef WIN32
#include <time.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#endif /* WIN32 */

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS) 
	#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64 
#else 
	#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL 
#endif 

#ifdef WIN32
//sj: implement gettimeofday in windows
struct timezone 
{  
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
}; 
	
int gettimeofday(struct timeval *tv, struct timezone *tz) 
{  
	FILETIME ft; 
	unsigned __int64 tmpres = 0; 
	static int tzflag; 
   
	if (NULL != tv) 
	{    
		GetSystemTimeAsFileTime(&ft); 
     
		tmpres |= ft.dwHighDateTime;    
		tmpres <<= 32;     
		tmpres |= ft.dwLowDateTime; 
    
		/*converting file time to unix epoch*/   
		tmpres /= 10;  /*convert into microseconds*/    
		tmpres -= DELTA_EPOCH_IN_MICROSECS;     
		tv->tv_sec = (long)(tmpres / 1000000UL);     
		tv->tv_usec = (long)(tmpres % 1000000UL);   
	}  
   
	if (NULL != tz)  
	{    
		if (!tzflag)     
		{      
			_tzset();       
			tzflag++;     
		}    
		tz->tz_minuteswest = _timezone / 60;     
		tz->tz_dsttime = _daylight;  
	}  
  
	return 0; 
}

#endif /* WIN32 */

double extract(tv)
struct timeval *tv;
{
  double tmp;

  tmp = tv->tv_sec;
  tmp += tv->tv_usec/1000000.0;
 
  return(tmp);
}

double dclock()
{
    struct timeval tp;
    struct timezone tzp;

    /* wall-clock time */
    gettimeofday(&tp,&tzp);

    return(extract(&tp));
}

