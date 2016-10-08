#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>  //  for toupper() macro in streq()

#ifdef WIN32
#include <windows.h>
#endif

#include <mysql.h>  //  Definitions for MySQL client API
#include <mex.h>    //  Definitions for Matlab API

const bool debug=false;  //  turn on information messages

/**********************************************************************
 **********************************************************************
 *
 * Matlab interface to MySQL server
 * Documentation on how to use is in "mysql.m"
 *  Robert Almgren, 2000-2005; this version January 2005
 *  Various modifications by Chris Rodgers during 2003-2004.
 *
 *  mexFunction()      Entry point from Matlab, presumably as "mysql"
 *   |
 *   +- hostport()     Get host and port number from combined string
 *   |
 *   +- fix_types()    Some datatype fudges for MySQL C API
 *   +- can_convert()  Whether we can convert a MySQL data type to numeric
 *   +- field2num()    Convert a MySQL data string to a Matlab number
 *   |   |
 *   |   +- daynum()   Convert year,month,day to Matlab date number
 *   |   +- typestr()  Translate MySQL data type into a word, for errors
 *   |
 *   +- fancyprint()   Display query result in nice form
 *
 * Some compile instructions available at
 *          http://www.mmf.utoronto.ca/resrchres/mysql/
 * Briefly, after configuring with "mex -setup", from Matlab do
 *   Windows:  mex -I"C:\mysql\include" -DWIN32 mysql.cpp 
 *                              "C:\mysql\lib\opt\libmySQL.lib"
 *   Linux:    mex -I/usr/include/mysql -L/usr/lib/mysql
 *                             -lmysqlclient mysql.cpp
 *  where "C:\mysql", "/usr/lib/mysql", "/usr/include/mysql", etc
 *  are where you installed the MySQL client libraries.
 **********************************************************************
 **********************************************************************/

/**********************************************************************
 *
 *  streq( s, t [,n] )   Case-insensitive string comparison
 *    provided locally because Windows does not have strcasecmp()
 *  Max length is finite by default because this is intended only
 *  to compare Matlab arguments, which should be quite short.
 *
 **********************************************************************/

static bool streq( const char *s, const char *t, int n=256 )
{
	while (n>0)  //  n is number of characters remaining to compare
		{ if ( !*s && !*t ) return true;    // simultaneous end of string
		  if ( toupper(*s) != toupper(*t) ) return false;  // incl if 0
		  n--; s++; t++; }
	return true;      // checked n characters and found none unequal
}

/**********************************************************************
 *
 * hostport(s):  Given a host name s, possibly containing a port number
 *    separated by the port separation character (normally ':').
 * Modify the input string by putting a null at the end of
 * the host string, and return the integer of the port number.
 * Return zero in most special cases and error cases.
 * Modified string will not contain the port separation character.
 * Examples:  s="myhost:2515" modifies s to "myhost" and returns 2515.
 *            s="myhost"      leaves s unchanged and returns 0.
 *
 **********************************************************************/

const char portsep = ':';   //  separates host name from port number

static int hostport(char *s)
{
	//   Look for first portsep in s; return 0 if null or can't find
	if ( !s || !( s=strchr(s,portsep) ) ) return 0;

	//  If s points to portsep, then truncate and convert tail
	*s++ = 0;
	return atoi(s);   // Returns zero in most special cases
}

/**********************************************************************
 *
 * typestr(s):  Readable translation of MySQL field type specifier
 *              as listed in   mysql_com.h
 *
 **********************************************************************/

static const char *typestr( enum_field_types t )
{
	switch(t)
		{
		//  These are considered numeric by IS_NUM() macro
		case FIELD_TYPE_DECIMAL:      return "decimal";
		case FIELD_TYPE_TINY:         return "tiny";
		case FIELD_TYPE_SHORT:        return "short";
		case FIELD_TYPE_LONG:         return "long";
		case FIELD_TYPE_FLOAT:        return "float";
		case FIELD_TYPE_DOUBLE:       return "double";
		case FIELD_TYPE_NULL:         return "null";
		case FIELD_TYPE_LONGLONG:     return "longlong";
		case FIELD_TYPE_INT24:        return "int24";
		case FIELD_TYPE_YEAR:         return "year";
		case FIELD_TYPE_TIMESTAMP:    return "timestamp";

		//  These are not considered numeric by IS_NUM()
		case FIELD_TYPE_DATE:         return "date";
		case FIELD_TYPE_TIME:         return "time";
		case FIELD_TYPE_DATETIME:     return "datetime";
		case FIELD_TYPE_NEWDATE:      return "newdate";   // not in manual
		case FIELD_TYPE_ENUM:         return "enum";
		case FIELD_TYPE_SET:          return "set";
		case FIELD_TYPE_TINY_BLOB:    return "tiny_blob";
		case FIELD_TYPE_MEDIUM_BLOB:  return "medium_blob";
		case FIELD_TYPE_LONG_BLOB:    return "long_blob";
		case FIELD_TYPE_BLOB:         return "blob";
		case FIELD_TYPE_VAR_STRING:   return "var_string";
		case FIELD_TYPE_STRING:       return "string";
#if MYSQL_VERSION_ID > 32358 // This version could perhaps go higher.
		case FIELD_TYPE_GEOMETRY:     return "geometry";   // not in manual 4.0
#endif
		default:                      return "unknown";
		}
}

/**********************************************************************
 *
 * fancyprint():  Print a nice display of a query result
 *     We assume the whole output set is already stored in memory,
 *     as from mysql_store_result(), just so that we can get the
 *     number of rows in case we need to clip the printing.
 *     In any case, we make only one pass through the data.
 *
 *     If the number of rows in the result is greater than NROWMAX,
 *     then we print only the first NHEAD and the last NTAIL.
 *     NROWMAX must be greater than NHEAD+NTAIL, normally at least
 *     2 greater to allow the the extra information
 *     lines printed when we clip (ellipses and total lines).
 *
 *     Display null elements as empty
 *
 **********************************************************************/

const char *contstr = "...";  //  representation of missing rows
const int NROWMAX = 50;       //  max number of rows to print w/o clipping
const int NHEAD = 10;         //  number of rows to print at top if we clip
const int NTAIL = 10;         //  number of rows to print at end if we clip

#ifndef WIN32
static int max(int a,int b)  { return (a>=b) ? a : b; }
#endif

static void fancyprint( MYSQL_RES *res )
{
	unsigned long nrow = mysql_num_rows(res);
	unsigned long nfield=mysql_num_fields(res);

	bool clip = ( nrow > NROWMAX );

	MYSQL_FIELD *f = mysql_fetch_fields(res);

	/************************************************************************/
	//  Determine column widths, and format strings for printing

	//  Find the longest entry in each column header,
	//    and over the rows, using MySQL's max_length
	int *len = (int *) mxMalloc( nfield * sizeof(int) );
	{ for ( int j=0 ; j<nfield ; j++ )
		len[j] = max(strlen(f[j].name),f[j].max_length); }

	//  Compare to the continuation string length if we will clip
	if (clip)
		{ int contstrlen = strlen(contstr);
		  for ( int j=0 ; j<nfield ; j++ )  len[j] = max(len[j],contstrlen); }

	//  Construct the format specification for printing the strings
	char **fmt = (char **) mxMalloc( nfield * sizeof(char *) );
	{ for ( int j=0 ; j<nfield ; j++ )
		{ fmt[j] = (char *) mxCalloc( 10, sizeof(char) );
		  sprintf(fmt[j],"  %%-%ds ",len[j]); }}

	/************************************************************************/
	//  Now print the actual data

	mexPrintf("\n");

	//  Column headers
	{ for ( int j=0 ; j<nfield ; j++ )  mexPrintf( fmt[j], f[j].name ); }
	mexPrintf("\n");

	//  Fancy underlines
	{ for ( int j=0 ; j<nfield ; j++ )
		{ mexPrintf(" +");
		  for ( int k=0 ; k<len[j] ; k++ )  mexPrintf("-");
		  mexPrintf("+"); }}
	mexPrintf("\n");

	//  Print the table entries
	if (nrow<=0) mexPrintf("(zero rows in result set)\n");
	else
	{
	if (!clip)    //  print the entire table
		{
		mysql_data_seek(res,0);
		for ( int i=0 ; i<nrow ; i++ )
			{ MYSQL_ROW row = mysql_fetch_row(res);
			  if (!row)
				{ mexPrintf("Printing full table data from row %d\n",i+1);
				  mexErrMsgTxt("Internal error:  Failed to get a row"); }
			  for ( int j=0 ; j<nfield ; j++ )
				mexPrintf( fmt[j], ( row[j] ? row[j] : "" ) );
			  mexPrintf("\n"); }
		}
	else          //  print half at beginning, half at end
		{
		mysql_data_seek(res,0);
		{ for ( int i=0 ; i<NHEAD ; i++ )
			{ MYSQL_ROW row = mysql_fetch_row(res);
			  if (!row)
				{ mexPrintf("Printing head table data from row %d\n",i+1);
				  mexErrMsgTxt("Internal error:  Failed to get a row"); }
			  for ( int j=0 ; j<nfield ; j++ )
				mexPrintf( fmt[j], ( row[j] ? row[j] : "" ) );
			  mexPrintf("\n"); }}
		{ for ( int j=0 ; j<nfield ; j++ ) mexPrintf(fmt[j],contstr); }
		mexPrintf("\n");
		mysql_data_seek( res, nrow - NTAIL );
		{ for ( int i=0 ; i<NTAIL ; i++ )
			{ MYSQL_ROW row = mysql_fetch_row(res);
			  if (!row)
				{ mexPrintf("Printing tail table data from row %d",nrow-NTAIL+i+1);
				  mexErrMsgTxt("Internal error:  Failed to get a row"); }
			  for ( int j=0 ; j<nfield ; j++ )
				mexPrintf( fmt[j], ( row[j] ? row[j] : "" ) );
			  mexPrintf("\n"); }}
		mexPrintf("(%d rows total)\n",nrow);
		}
	}
	mexPrintf("\n");

	// These should be automatically freed when we return to Matlab,
	//  but just in case ...
	mxFree(len);  mxFree(fmt);
}

/**********************************************************************
 *
 * field2num():  Convert field in string format to double number
 *
 **********************************************************************/

const double NaN = mxGetNaN();         //  Matlab NaN for null values
const double secinday = 24.*60.*60.;   //  seconds in one day

//----------------------------------------------
//  year,month,day --> serial date number, based on Matlab's own algorithm

const int cummonday[2][12] =
	{ {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
	  {  0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 } };

inline int cl( int y, int n )  { return ((y-1)/n)+1; }  // ceil(y/n)

static int daynum( int yr, int mon, int day )
{
	//  This is exactly Matlab's formula
	int leap = ( !(yr%4) && (yr%100) ) | !(yr%400);
	return 365*yr + cl(yr,4) - cl(yr,100) + cl(yr,400)
	        + cummonday[leap][mon-1] + day;
}

//------------------------------------------------

static bool can_convert( enum_field_types t )
{
	return ( IS_NUM(t)
	   || ( t == FIELD_TYPE_DATE )
	   || ( t == FIELD_TYPE_TIME )
	   || ( t == FIELD_TYPE_DATETIME ) );
}

static double field2num( const char *s, enum_field_types t )
{
	if (!s) return NaN;  // MySQL null -- nothing there

	if ( IS_NUM(t) )
		{
		double val=NaN;
		if ( sscanf(s,"%lf",&val) != 1 )
			{ mexPrintf("Unreadable value \"%s\" of type %s\n",s,typestr(t));
			  return NaN; }
		return val;
		}
	else if ( t == FIELD_TYPE_DATE )
		{
		int yr, mon, day;
		if ( sscanf(s,"%4d-%2d-%2d",&yr,&mon,&day) != 3
		    || yr<1000 || yr>=3000 || mon<1 || mon>12 || day<1 || day>31 )
			{ mexPrintf("Unreadable value \"%s\" of type %s\n",s,typestr(t));
			  return NaN; }
		return (double) daynum(yr,mon,day);
		}
	else if ( t == FIELD_TYPE_TIME )
		{
		int hr, min, sec;
		if ( sscanf(s,"%2d:%2d:%2d",&hr,&min,&sec) != 3
		           || min>60 || sec>60 )
			{ mexPrintf("Unreadable value \"%s\" of type %s\n",s,typestr(t));
			  return NaN; }
		return (sec+60*(min+60*hr))/secinday;
		}
	else if ( t == FIELD_TYPE_DATETIME )
		{
		int yr, mon, day, hr, min, sec;
		if ( sscanf(s,"%4d-%2d-%2d %2d:%2d:%2d",&yr,&mon,&day,&hr,&min,&sec) != 6
		      || yr<1000 || yr>=3000 || mon<1 || mon>12 || day<1 || day>31
		      || min>60 || sec>60 )
			{ mexPrintf("Unreadable value \"%s\" of type %s\n",s,typestr(t));
			  return NaN; }
		return ((double) daynum(yr,mon,day))
		       + ((sec+60*(min+60*hr))/secinday );
		}
	else
		{
		mexPrintf("Tried to convert \"%s\" of type %s to numeric\n",s,typestr(t));
		mexErrMsgTxt("Internal inconsistency");
		}
}

/**********************************************************************
 *
 * fix_types():   I have seen problems with the MySQL C API reporting
 *   types inconsistently. Returned data value from fields of type
 *   "date" or "time" has type set appropriately to DATE, TIME, etc.
 *   However, if any operation is performed on such data, even something
 *   simple like max(), then the result can sometimes be reported as the
 *   generic type STRING. The developers say this is forced by the SQL
 *   standard but to me it seems strange.
 *
 *   This function looks at the actual result data returned. For each
 *   column that is reported as type STRING, we see if we can make a
 *   more precise classification as DATE, TIME, or DATETIME.
 *
 *   For fields of type STRING, we look at the length, then content
 *    length = 8, 9, or 10:  can be time as HH:MM:SS, HHH:MM:SS, or HHHH:MM:SS
 *    length = 10:           can be date in form YYYY:MM:DD
 *    length = 19:           can be datetime in form YYYY:MM:DD HH:MM:SS
 *
 **********************************************************************/

static void fix_types( MYSQL_FIELD *f, MYSQL_RES *res )
{
	if (!res)    //  This should never happen
		mexErrMsgTxt("Internal error:  fix_types called with res=NULL");

	unsigned long nrow=mysql_num_rows(res), nfield=mysql_num_fields(res);
	if (nrow<1) return;  // nothing to look at, nothing to fix

	bool *is_unknown = (bool *) mxMalloc( nfield * sizeof(bool) );
	int n_unknown=0;   //  count number of fields of unknown type
	{ for ( int j=0 ; j<nfield ; j++ )
		{ is_unknown[j] = ( f[j].type==FIELD_TYPE_STRING
		     && ( f[j].length==8 || f[j].length==9 || f[j].length==10
		         || f[j].length==19 ) );
		  if (is_unknown[j])  n_unknown++; }}

	if (debug)
		{ mexPrintf("| Starting types:");
		  for ( int j=0 ; j<nfield ; j++ )
			mexPrintf("  %s(%d)%s", typestr(f[j].type), f[j].length,
			    ( is_unknown[j] ? "?" : "" ) );
		  mexPrintf("\n"); }

	//  Look at successive rows as long as some columns are still unknown
	//  We go through columns only to find the first non-null data value.
	mysql_data_seek(res,0);
	{ for ( int i=0 ; n_unknown>0 && i<nrow ; i++ )
		{
		MYSQL_ROW row = mysql_fetch_row(res);
		if (!row)
			{ mexPrintf("Scanning row %d for type identification\n",i+1);
			  mexErrMsgTxt("Internal error in fix_types():  Failed to get a row"); }

		if (debug)
			{ mexPrintf("|   row[%d]:",i+1);
			  for ( int j=0 ; j<nfield ; j++ )
				mexPrintf("  \"%s\"%s", ( row[j] ? row[j] : "NULL" ),
				      ( is_unknown[j] ? "?" : "" ) );
			  mexPrintf("\n"); }

		//  Look at each field to see if we can extract information
		for ( int j=0 ; j<nfield ; j++ )
			{
			//  If this column is still a mystery, and if there is data here,
			//    then try extracting a date and/or time out of it 
			if ( is_unknown[j] && row[j] )
				{
				int yr, mon, day, hr, min, sec;

				if ( f[j].length==19
				      && sscanf(row[j],"%4d-%2d-%2d %2d:%2d:%2d",
				            &yr,&mon,&day,&hr,&min,&sec) == 6
				      && yr>=1000 && yr<3000 && mon>=1 && mon<=12 && day>=1 && day<=31
				      && min<=60 && sec<=60 )
					f[j].type = FIELD_TYPE_DATETIME;

				else if ( f[j].length==10
				      && sscanf(row[j],"%4d-%2d-%2d",&yr,&mon,&day) == 3
				      && yr>=1000 && yr<3000 && mon>=1 && mon<=12 && day>=1 && day<=31 )
					f[j].type = FIELD_TYPE_DATE;

				else if ( ( f[j].length==8 || f[j].length==9 || f[j].length==10 )
				      && sscanf(row[j],"%d:%2d:%2d",&hr,&min,&sec) == 3
				      && min<=60 && sec<=60 )
					f[j].type = FIELD_TYPE_TIME;

				//  If the tests above failed, then the type is not date or time;
				//  it really is a string of unknown type.
				//  Whether the tests suceeded or failed, it is no longer a mystery.
				is_unknown[j]=false;  n_unknown--;
				}
			}
		}}

	if (debug)
		{ mexPrintf("|   Ending types:");
		  for ( int j=0 ; j<nfield ; j++ )
			mexPrintf("  %s(%d)%s", typestr(f[j].type), f[j].length,
			     ( is_unknown[j] ? "?" : "" ) );
		  mexPrintf("\n"); }

	mxFree(is_unknown);  // should be automatically freed, but still...
}

/*********************************************************************
 *  Static variables that contain connection state
 *
 *  isopen gets set to true when we execute an "open"
 *  goes false when we execute "close" or a ping or status fails
 *   We do not set it to false when a normal query fails;
 *   this might be due to the server having died, but is much
 *   more likely to be caused by an incorrect query.
 */

typedef MYSQL *mp;
class conninfo {
public:
	mp   conn;     //  MySQL connection information structure
	bool isopen;   //  whether we believe that connection is open
	conninfo()  { conn=NULL;  isopen=false; }
};

const int MAXCONN = 10;

//  List of connection information
static conninfo c[MAXCONN];

//  Stack for most recently used handles:  prevcid[ncid-1] is the most
//  recent and   prevcid[0], ..., prevcid[ncid-1]  is the list.
//  The elements of this list should always be precisely the set
//   of c[] that have  isopen==true.
//  Function stackdelete deletes a particular cid from this list,
//    which of course requires us to find it first. The deletion
//    requires some copying in the case  j < ncid-1.
static int ncid=0, prevcid[MAXCONN];
static void stackprint()
{
	mexPrintf("| Stack is [");
	for ( int j=0 ; j<ncid ; j++ ) mexPrintf(" %d",prevcid[j]);
	mexPrintf(" ]\n");
}
static void stackdelete(int cid)
{
	//  Locate  j = index of cid in prevcid[]
	//  Search downward since commonly cid will be the last element
	int j=ncid-1; while ( j>=0 && prevcid[j]!=cid ) j--;
	if ( j<0 || j>=ncid ) return;

	for ( int i=j+1 ; i<ncid ; i++ ) prevcid[i-1] = prevcid[i];
	ncid--;
}

/*********************************************************************/

static void showusage()
{
	mexPrintf("Usage:  %s( [dbHandle], command, [ host, user, password ] )\n",
		mexFunctionName());
}

static double *setScalarReturn( int nlhs, mxArray *plhs[], double value )
{
	if (nlhs<1) return NULL;
	if (!( plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL) ))
		mexErrMsgTxt("Unable to create matrix for output");
	*mxGetPr(plhs[0]) = value;
	return mxGetPr(plhs[0]);
}

extern "C" void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[]);

void mexFunction(int nlhs, mxArray *plhs[],
      int nrhs, const mxArray *prhs[])
{
	int cid=-1;  // Handle of the current connection (invalid until we identify)
	int jarg=0;  // Number of first string arg: becomes 1 if id is specified

	/*********************************************************************/
	// Parse the first argument to see if it is a specific database handle
	if ( nrhs>0 && mxIsNumeric(prhs[0]) )
		{ if ( mxGetM(prhs[0])!=1 || mxGetN(prhs[0])!=1 )
			{ showusage();
			  mexPrintf("First argument is array %d x %d, but it should be a scalar\n",
			      mxGetM(prhs[0]),mxGetN(prhs[0]) );
			  mexErrMsgTxt("Invalid connection handle"); }
		  double xid = *mxGetPr(prhs[0]);
		  cid = int(xid);
		  if ( double(cid)!=xid || cid<0 || cid>=MAXCONN )
			{ showusage();
			  mexPrintf("dbHandle = %g -- Must be integer between 0 and %d\n",
			      xid,MAXCONN-1);
			  mexErrMsgTxt("Invalid connection handle"); }
		  jarg = 1;
		  if (debug) mexPrintf("| Explicit  cid = %d\n",cid); }

	/*********************************************************************/
	//  Check that the remaining arguments are all character strings
	{ for ( int j=jarg ; j<nrhs ; j++ )
		{ if (!mxIsChar(prhs[j]))
			{ showusage();
			  mexErrMsgTxt("All args must be strings, except dbHandle"); }}}

	/*********************************************************************/
	//  Identify what action he wants to do

	enum querytype { OPEN, CLOSE, CLOSEALL, USENAMED, USE,
	                     STATUS, STATSALL, QUOTE, CMD } q;
	char *query = NULL;

	if (nrhs<=jarg)   q = STATSALL;
	else
		{ query = mxArrayToString(prhs[jarg]);
		  if (streq(query,"open"))          q = OPEN;
		  else if (streq(query,"close"))    q = CLOSE;
		  else if (streq(query,"closeall")) q = CLOSEALL;
		  else if (streq(query,"use"))      q = USENAMED;
		  else if (streq(query,"use",3))    q = USE;
		  else if (streq(query,"status"))   q = STATUS;
		  else if (streq(query,"quote"))    q = QUOTE;
		  else q = CMD; }

	if (debug)
		{ switch(q)
			{ case OPEN:     mexPrintf("| q = OPEN\n");     break;
			  case CLOSE:    mexPrintf("| q = CLOSE\n");    break;
			  case CLOSEALL: mexPrintf("| q = CLOSEALL\n"); break;
			  case USENAMED: mexPrintf("| q = USENAMED\n"); break;
			  case USE:      mexPrintf("| q = USE\n");      break;
			  case STATUS:   mexPrintf("| q = STATUS\n");   break;
			  case STATSALL: mexPrintf("| q = STATSALL\n"); break;
			  case QUOTE:    mexPrintf("| q = QUOTE\n");    break;
			  case CMD:      mexPrintf("| q = CMD\n");      break;
			                 mexPrintf("| q = ??\n");              }}

	/*********************************************************************/
	//  If he did not specify the handle, choose the appropriate one
	//  If there are no previous connections, then we will still have
	//     cid=-1, and this will be handled in the appropriate place.
	if (jarg==0)
		{
		if (q==OPEN)
			{ for ( cid=0 ; cid<MAXCONN & c[cid].isopen ; cid++ );
			  if (cid>=MAXCONN) mexErrMsgTxt("Can\'t find free handle"); }
		else if (ncid>0)
			cid=prevcid[ncid-1];
		}

	if (debug) mexPrintf("| cid = %d\n",cid);

	//  Shorthand notation so we don't need to write c[cid]...
	//  These values must not be used if  cid<0
	mp dummyconn;     mp &conn = (cid>=0) ? c[cid].conn : dummyconn;
	bool dummyisopen; bool &isopen = (cid>=0) ? c[cid].isopen : dummyisopen;

	if (q==OPEN)
		{
		if (cid<0)
			{ mexPrintf("cid = %d !\n",cid);
			  mexErrMsgTxt("Internal code error\n"); }
		//  Close connection if it is open
		if (isopen)
			{ mexWarnMsgIdAndTxt("mysql:ConnectionAlreadyOpen",
			    "Connection %d has been closed and overwritten",cid);
			  mysql_close(conn);
			  conn=NULL;  isopen=false;  stackdelete(cid); }

		//  Extract information from input arguments
		char *host=NULL;   if (nrhs>=jarg+2)  host = mxArrayToString(prhs[jarg+1]);
		char *user=NULL;   if (nrhs>=jarg+3)  user = mxArrayToString(prhs[jarg+2]);
		char *pass=NULL;   if (nrhs>=jarg+4)  pass = mxArrayToString(prhs[jarg+3]);
		int port = hostport(host);  // returns zero if there is no port

		if (nlhs<1)
			{ mexPrintf("Connecting to  host=%s", (host) ? host : "localhost" );
			  if (port) mexPrintf("  port=%d",port);
			  if (user) mexPrintf("  user=%s",user);
			  if (pass) mexPrintf("  password=%s",pass);
			  mexPrintf("\n"); }

		//  Establish and test the connection
		//  If this fails, then conn is still set, but isopen stays false
		if (!(conn=mysql_init(conn)))
			mexErrMsgTxt("Couldn\'t initialize MySQL connection object");
		if (!mysql_real_connect( conn, host, user, pass, NULL,port,NULL,0 ))
			mexErrMsgTxt(mysql_error(conn));
		const char *c=mysql_stat(conn);
		if (c)  { if (nlhs<1) mexPrintf("%s\n",c); }
		else    mexErrMsgTxt(mysql_error(conn));

		isopen=true;
		ncid++;
		if (ncid>MAXCONN)
			{ mexPrintf("ncid = %d ?\n",ncid);
			  mexErrMsgTxt("Internal logic error\n"); }
		prevcid[ncid-1] = cid;

		if (debug) stackprint();

		//  Now we are OK -- return the connection handle opened.
		setScalarReturn(nlhs,plhs,cid);
		}

	else if (q==CLOSE)
		{
		if ( cid>=0 && isopen )
			{ if (debug) mexPrintf("| Closing %d\n",cid);
			  mysql_close(conn);
			  conn = NULL; isopen=false;  stackdelete(cid); }
		if (debug) stackprint();
		}

	else if (q==CLOSEALL)
		{ while (ncid>0)
			{ if (debug) stackprint();
			  cid = prevcid[ncid-1];
			  if (debug) mexPrintf("| Closing %d\n",cid);
			  if (!(c[cid].isopen))
				{ mexPrintf("Connection %d is not marked open!\n",cid);
				  mexErrMsgTxt("Internal logic error"); }
			  mysql_close(c[cid].conn);
			  c[cid].conn=NULL;  c[cid].isopen=false;  ncid--; }}

	else if ( q==USE || q==USENAMED )
		{
		if ( cid<0 || !isopen ) mexErrMsgTxt("Not connected");
		if (mysql_ping(conn))
			{ stackdelete(cid);  isopen=false;
			  mexPrintf(mysql_error(conn));
			  mexPrintf("\nClosing connection %d\n",cid);
			  mexErrMsgTxt("Use command failed"); }
		char *db=NULL;
		if (q==USENAMED)
			{ if (nrhs>=2) db=mxArrayToString(prhs[jarg+1]);
			  else         mexErrMsgTxt("Must specify a database to use"); }
		else
			{ db = query + 3;
			  while ( *db==' ' || *db=='\t' ) db++; }
		if (mysql_select_db(conn,db))  mexErrMsgTxt(mysql_error(conn));
		if (nlhs<1) mexPrintf("Current database is \"%s\"\n",db); 
		else        setScalarReturn(nlhs,plhs,1.);
		}

	else if (q==STATUS)
		{
		if (nlhs<1)  //  He wants a report
			{ // print connection handle only if multiple connections
			  char idstr[10];  idstr[0]=0;
			  if ( cid>=0 && ncid>1 )  sprintf(idstr,"(%d) ",cid);
			  if ( cid<0 || !isopen )
			   { mexPrintf("%sNot connected\n",idstr,cid);  return; }
			  if (mysql_ping(conn))
				{ mexErrMsgTxt(mysql_error(conn)); }
			  mexPrintf("%s%-30s   Server version %s\n",
			    idstr, mysql_get_host_info(conn), mysql_get_server_info(conn) ); }
		else         //  He wants a return value for this connection
			{ double *pr=setScalarReturn(nlhs,plhs,0.);
			  if ( cid<0 || !isopen ) { *pr=1.; return; }
			  if (mysql_ping(conn))   { *pr=2.; return; }}
		}

	else if (q==STATSALL)
		{
		if (debug) stackprint();
		if (ncid==0)      mexPrintf("No connections open\n");
		else if (ncid==1) mexPrintf("1 connection open\n");
		else              mexPrintf("%d connections open\n",ncid);
		for ( int j=0 ; j<ncid ; j++ )
			{ cid = prevcid[j];
			  if (mysql_ping(c[cid].conn))
				  mexPrintf("%2d:  %s\n",cid,mysql_error(conn));
			  else
				  mexPrintf("%2d:  %-30s   Server version %s\n",
				        cid, mysql_get_host_info(c[cid].conn),
				        mysql_get_server_info(c[cid].conn) ); }
		}

	// Quote the second string argument and return it (Chris Rodgers)
	else if (q==QUOTE)
		{
		if ((nrhs-jarg)!=2)
			mexErrMsgTxt("mysql('quote','string_to_quote') takes two string arguments!");

		//  Check that we have a valid connection
		if ( cid<0 || !isopen ) mexErrMsgTxt("No connection open");
		if (mysql_ping(conn))
			{ stackdelete(cid);  isopen=false;
			  mexErrMsgTxt(mysql_error(conn)); }

		const mxArray *a = prhs[jarg+1];
		int llen = mxGetM(a)*mxGetN(a)*sizeof(mxChar);
		char *from = (char *) mxCalloc(llen+1,sizeof(char));
		if (mxGetString(a,from,llen)) mexErrMsgTxt("Can\'t copy string");
		int l = strlen(from);

		/* Allocate memory for input and output strings. */
		char *to = (char*) mxCalloc( llen*2+3, sizeof(char));

		/* Call the C subroutine. */
		to[0] = '\'';
		int n = mysql_real_escape_string( conn, to+1, from, l );
		to[n+1] = '\'';

		/* Set C-style string output_buf to MATLAB mexFunction output*/
		plhs[0] = mxCreateString(to);

		mxFree(from);  mxFree(to);  // just in case Matlab forgets
		}

	else if (q==CMD)
		{
		//  Check that we have a valid connection
		if ( cid<0 || !isopen ) mexErrMsgTxt("No connection open");
		if (mysql_ping(conn))
			{ stackdelete(cid);  isopen=false;
			  mexPrintf(mysql_error(conn));
			  mexPrintf("Closing connection %d\n",cid);
			  mexErrMsgTxt("Query failed"); }

		//  Execute the query (data stays on server)
		if (mysql_query(conn,query))  mexErrMsgTxt(mysql_error(conn));

		//  Download the data from server into our memory
		//     We need to be careful to deallocate res before returning.
		//  Matlab's allocation routines return instantly if there is not
		//  enough free space, without giving us time to dealloc res.
		//  This is a potential memory leak but I don't see how to fix it.
		MYSQL_RES *res = mysql_store_result(conn);

		//  As recommended in Paul DuBois' MySQL book (New Riders, 1999):
		//  A NULL result set after the query can indicate either
		//    (1) the query was an INSERT, DELETE, REPLACE, or UPDATE, that
		//        affect rows in the table but do not return a result set; or
		//    (2) an error, if the query was a SELECT, SHOW, or EXPLAIN
		//        that should return a result set but didn't.
		//  Distinguish between the two by checking mysql_field_count()
		//  We return in either case, either correctly or with an error
		if (!res)
			{
			if (!mysql_field_count(conn))
				{ unsigned long nrows=mysql_affected_rows(conn);
				  if (nlhs<1)
					{ mexPrintf("%u rows affected\n",nrows);
					  return; }
				  else
					{ setScalarReturn(nlhs,plhs,nrows);
					  return; }}
			else
				  mexErrMsgTxt(mysql_error(conn));
			}

		unsigned long nrow=mysql_num_rows(res), nfield=mysql_num_fields(res);

		//  If he didn't ask for any output (nlhs=0),
		//       then display the output and return
		if ( nlhs<1 )
			{ fancyprint(res);
			  mysql_free_result(res);
			  return; }

		//  If we are here, he wants output
		//  He must give exactly the right number of output arguments
		if ( nlhs != nfield )
			{ mysql_free_result(res);
			  mexPrintf("You specified %d output arguments, "
			         "and got %d columns of data\n",nlhs,nfield);
			  mexErrMsgTxt("Must give one output argument for each column"); }

		//  Fix the column types to fix MySQL C API sloppiness
		MYSQL_FIELD *f = mysql_fetch_fields(res);
		fix_types( f, res );

		//  Create the Matlab arrays for output
		double **pr = (double **) mxMalloc( nfield * sizeof(double *) );
		{ for ( int j=0 ; j<nfield ; j++ )
			{ if ( can_convert(f[j].type) )
				{ if (!( plhs[j] = mxCreateDoubleMatrix( nrow, 1, mxREAL ) ))
					{ mysql_free_result(res);
					  mexErrMsgTxt("Unable to create numeric matrix for output"); }
				  pr[j] = mxGetPr(plhs[j]); }
			  else
				{ if (!( plhs[j] = mxCreateCellMatrix( nrow, 1 ) ))
					{ mysql_free_result(res);
					  mexErrMsgTxt("Unable to create cell matrix for output"); }
				  pr[j] = NULL; }}}

		//  Load the data into the cells
		mysql_data_seek(res,0);
		{ for ( int i=0 ; i<nrow ; i++ )
			{ MYSQL_ROW row = mysql_fetch_row(res);
			  if (!row)
				{ mexPrintf("Scanning row %d for data extraction\n",i+1);
				  mexErrMsgTxt("Internal error:  Failed to get a row"); }
			  for ( int j=0 ; j<nfield ; j++ )
				{ if (can_convert(f[j].type))
					{ pr[j][i] = field2num(row[j],f[j].type); }
				  else
					{ mxArray *c = mxCreateString(row[j]);
					  mxSetCell(plhs[j],i,c); }}}}
		mysql_free_result(res);
		}
	else
		{ mexPrintf("Unknown query type q = %d\n",q);
		  mexErrMsgTxt("Internal code error"); }
}
