/*
   This file is part of libodbc++.

   Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ODBCXX_TYPES_H
#define __ODBCXX_TYPES_H

#include <odbc++/setup.h>

#include <exception>

#if !defined(ODBCXX_QT)
# include <string>
# else
# include <qstring.h>
#endif

#include <ctime>
#if defined(ODBCXX_NO_STD_TIME_T)
namespace std {
  using ::time_t;
};
#endif

#if defined(ODBCXX_QT)
class QIODevice;
#endif

#if defined(ODBCXX_HAVE_ISQL_H) && defined(ODBCXX_HAVE_ISQLEXT_H)
# include <isql.h>
# include <isqlext.h>
#elif defined(ODBCXX_HAVE_SQL_H) && defined(ODBCXX_HAVE_SQLEXT_H)
# include <sql.h>
# include <sqlext.h>
#else
# error "Whoops. Can not recognize the ODBC subsystem."
#endif

#if defined(ODBCXX_HAVE_SQLUCODE_H)
# include <sqlucode.h>
#endif

// fixups for current iODBC, which kindly doesn't provide SQL_TRUE and
// SQL_FALSE macros

#if !defined(SQL_TRUE)
# define SQL_TRUE 1
#endif

#if !defined(SQL_FALSE)
# define SQL_FALSE 0
#endif

// MS ODBC SDK misses this in some releases
#if ODBCVER >= 0x0300 && !defined(SQL_NOT_DEFERRABLE)
# define SQL_NOT_DEFERRABLE 7
#endif


// Setup our ODBC3_C (odbc3 conditional) macro
#if ODBCVER >= 0x0300

# define ODBC3_C(odbc3_value,old_value) odbc3_value

#else

# define ODBC3_C(odbc3_value,old_value) old_value

#endif


// ODBC3_DC (odbc3 dynamic conditional)
// Every context using this macro should provide
// a this->_getDriverInfo() method returning
// a const DriverInfo*

#if ODBCVER >= 0x0300

# define ODBC3_DC(odbc3_value,old_value) \
(this->_getDriverInfo()->getMajorVersion()>=3?odbc3_value:old_value)

#else

# define ODBC3_DC(odbc3_value,old_value) old_value

#endif

#if defined(ODBCXX_HAVE_INTTYPES_H)
# include <inttypes.h>
#endif

#include <vector>


namespace odbc {

  // We want Long to be at least 64 bits

#if defined(WIN32)

  typedef __int64 Long;

#elif defined(ODBCXX_HAVE_INTTYPES_H)

  typedef int64_t Long;

#else

# if ODBCXX_SIZEOF_INT == 8

  typedef int Long;

# elif ODBCXX_SIZEOF_LONG == 8

  typedef long Long;

# elif ODBCXX_SIZEOF_LONG_LONG == 8

  typedef long long Long;

# else

#  error "Can't find an appropriate at-least-64-bit integer"

# endif

#endif


  //constants:
  //how much we try to fetch with each SQLGetData call
  const int GETDATA_CHUNK_SIZE=4*1024;
  //how much we write with each SQLPutData call
  const int PUTDATA_CHUNK_SIZE=GETDATA_CHUNK_SIZE;

  //how much we read/write in string<->stream conversion
  //better names for those?
  const int STRING_TO_STREAM_CHUNK_SIZE=1024;
  const int STREAM_TO_STRING_CHUNK_SIZE=STRING_TO_STREAM_CHUNK_SIZE;





  /** SQL type constants
   */
  struct Types {
    /** Type constants
     */
    enum SQLType {
      /** An SQL BIGINT */
      BIGINT                = SQL_BIGINT,
      /** An SQL BINARY (fixed length) */
      BINARY                = SQL_BINARY,
      /** An SQL BIT */
      BIT                = SQL_BIT,
      /** An SQL CHAR (fixed length) */
      CHAR                = SQL_CHAR,
      /** An SQL DATE */
      DATE                = ODBC3_C(SQL_TYPE_DATE,SQL_DATE),
      /** An SQL DECIMAL (precision,scale) */
      DECIMAL                = SQL_DECIMAL,
      /** An SQL DOUBLE */
      DOUBLE                = SQL_DOUBLE,
      /** An SQL FLOAT */
      FLOAT                = SQL_FLOAT,
      /** An SQL INTEGER */
      INTEGER                = SQL_INTEGER,
      /** An SQL LONGVARBINARY (variable length, huge) */
      LONGVARBINARY        = SQL_LONGVARBINARY,
      /** An SQL LONGVARCHAR (variable length, huge) */
      LONGVARCHAR        = SQL_LONGVARCHAR,
      /** An SQL NUMERIC (precision,scale) */
      NUMERIC                = SQL_NUMERIC,
      /** An SQL REAL */
      REAL                = SQL_REAL,
      /** An SQL SMALLINT */
      SMALLINT                = SQL_SMALLINT,
      /** An SQL TIME */
      TIME                = ODBC3_C(SQL_TYPE_TIME,SQL_TIME),
      /** An SQL TIMESTAMP */
      TIMESTAMP                = ODBC3_C(SQL_TYPE_TIMESTAMP,SQL_TIMESTAMP),
      /** An SQL TINYINT */
      TINYINT                = SQL_TINYINT,
      /** An SQL VARBINARY (variable length less than 256) */
      VARBINARY                = SQL_VARBINARY,
      /** An SQL VARCHAR (variable length less than 256) */
      VARCHAR                = SQL_VARCHAR
#if defined(ODBCXX_HAVE_SQLUCODE_H)
      ,
      /** A wide SQL CHAR (fixed length less than 256) */
      WCHAR         = SQL_WCHAR,
      /** A wide SQL VARCHAR (variable length less than 256) */
      WVARCHAR      = SQL_WVARCHAR,
      /** A wide SQL LONGVARCHAR (variable length, huge) */
      WLONGVARCHAR   = SQL_WLONGVARCHAR
#endif
    };
  };


#if !defined(ODBCXX_QT)
  /** A chunk of bytes.
   *
   * Used for setting and getting binary values.
   * @warning This class uses reference counting. An instance that is
   * referred to from different threads is likely to cause trouble.
   */
  class ODBCXX_EXPORT Bytes {
  private:
    struct Rep {
      ODBCXX_SIGNED_CHAR_TYPE* buf_;
      size_t len_;
      int refCount_;
      Rep(const ODBCXX_SIGNED_CHAR_TYPE* b, size_t l)
        :len_(l), refCount_(0) {
        if(len_>0) {
          buf_=new ODBCXX_SIGNED_CHAR_TYPE[len_];
          memcpy((void*)buf_,(void*)b,len_);
        } else {
          buf_=NULL;
        }
      }
      ~Rep() {
        delete [] buf_;
      }
    };

    Rep* rep_;
  public:
    /** Default constructor */
    Bytes()
      :rep_(new Rep(NULL,0)) {
      rep_->refCount_++;
    }

    /** Constructor */
    Bytes(const ODBCXX_SIGNED_CHAR_TYPE* data, size_t dataLen)
      :rep_(new Rep(data,dataLen)) {
      rep_->refCount_++;
    }

    /** Copy constructor */
    Bytes(const Bytes& b)
      :rep_(b.rep_) {
      rep_->refCount_++;
    }

    /** Assignment */
    Bytes& operator=(const Bytes& b) {
      if(--rep_->refCount_==0) {
        delete rep_;
      }
      rep_=b.rep_;
      rep_->refCount_++;
      return *this;
    }

    /** Comparison */
    bool operator==(const Bytes& b) const {
                        if (getSize()!=b.getSize())
                                return false;
                        for(size_t i=0;i<getSize();i++) {
                                if(*(getData()+i)!=*(b.getData()+i))
                                        return false;
                        }
      return true;
    }

    /** Destructor */
    ~Bytes() {
      if(--rep_->refCount_==0) {
        delete rep_;
      }
    }

    /** Returns a pointer to the data */
    const ODBCXX_SIGNED_CHAR_TYPE* getData() const {
      return rep_->buf_;
    }

    /** Returns the size of the data */
    size_t getSize() const {
      return rep_->len_;
    }
  };
#endif

  /** An SQL DATE */
  class ODBCXX_EXPORT Date {
  protected:
    int year_;
    int month_;
    int day_;

    virtual void _invalid(const ODBCXX_CHAR_TYPE* what, int value);

    int _validateYear(int y) {
      return y;
    }

    int _validateMonth(int m) {
      if(m<1 || m>12) {
        this->_invalid(ODBCXX_STRING_CONST("month"),m);
      }
      return m;
    }

    int _validateDay(int d) {
      if(d<1 || d>31) {
        this->_invalid(ODBCXX_STRING_CONST("day"),d);
      }
      return d;
    }

  public:
    /** Constructor.
     */
    Date(int year, int month, int day) {
      this->setYear(year);
      this->setMonth(month);
      this->setDay(day);
    }

    /** Constructor.
     *
     * Sets this date to today.
     */
    explicit Date();

    /** Constructor.
     *
     * Sets this date to the specified time_t value.
     */
    Date(std::time_t t) {
      this->setTime(t);
    }

    /** Constructor.
     *
     * Sets this date to the specified string in the <tt>YYYY-MM-DD</tt> format.
     */
    Date(const ODBCXX_STRING& str) {
      this->parse(str);
    }

    /** Copy constructor */
    Date(const Date& d)
      :year_(d.year_),
       month_(d.month_),
       day_(d.day_) {}

    /** Assignment operator */
    Date& operator=(const Date& d) {
      year_=d.year_;
      month_=d.month_;
      day_=d.day_;
      return *this;
    }

    /** Destructor */
    virtual ~Date() {}

    /** Sets this date to the specified time_t value */
    virtual void setTime(std::time_t t);

    /** Returns the time_t value of <tt>00:00:00</tt> at this date */
    std::time_t getTime() const;

    /** Sets this date from a string in the <tt>YYYY-MM-DD</tt> format */
    void parse(const ODBCXX_STRING& str);

    /** Gets the year of this date */
    int getYear() const {
      return year_;
    }

    /** Gets the month of this date */
    int getMonth() const {
      return month_;
    }

    /** Gets the monthday of this date */
    int getDay() const {
      return day_;
    }

    /** Sets the year of this date */
    void setYear(int year) {
      year_=this->_validateYear(year);
    }

    /** Sets the month of this date */
    void setMonth(int month) {
      month_=this->_validateMonth(month);
    }

    /** Sets the day of this date */
    void setDay(int day) {
      day_=this->_validateDay(day);
    }

    /** Gets the date as a string in the <tt>YYYY-MM-DD</tt> format */
    virtual ODBCXX_STRING toString() const;
  };

  /** An SQL TIME */
  class ODBCXX_EXPORT Time {
  protected:
    int hour_;
    int minute_;
    int second_;

    virtual void _invalid(const ODBCXX_CHAR_TYPE* what, int value);

    int _validateHour(int h) {
      if(h<0 || h>23) {
        this->_invalid(ODBCXX_STRING_CONST("hour"),h);
      }
      return h;
    }

    int _validateMinute(int m) {
      if(m<0 || m>59) {
        this->_invalid(ODBCXX_STRING_CONST("minute"),m);
      }
      return m;
    }

    int _validateSecond(int s) {
      if(s<0 || s>61) {
        this->_invalid(ODBCXX_STRING_CONST("second"),s);
      }
      return s;
    }

  public:
    /** Constructor */
    Time(int hour, int minute, int second) {
      this->setHour(hour);
      this->setMinute(minute);
      this->setSecond(second);
    }

    /** Constructor.
     *
     * Sets the time to now.
     */
    explicit Time();

    /** Constructor.
     *
     * Sets the time to the specified <tt>time_t</tt> value.
     */
    Time(std::time_t t) {
      this->setTime(t);
    }

    /** Constructor.
     *
     * Sets the time to the specified string in the <tt>HH:MM:SS</tt> format.
     */
    Time(const ODBCXX_STRING& str) {
      this->parse(str);
    }

    /** Copy constructor */
    Time(const Time& t)
      :hour_(t.hour_),
       minute_(t.minute_),
       second_(t.second_) {}

    /** Assignment operator */
    Time& operator=(const Time& t) {
      hour_=t.hour_;
      minute_=t.minute_;
      second_=t.second_;
      return *this;
    }

    /** Destructor */
    virtual ~Time() {}

    /** Sets the time to the specified <tt>time_t</tt> value */
    virtual void setTime(std::time_t t);

    /** Returns the <tt>time_t</tt> value of <tt>1970-01-01</tt> at this time */
    std::time_t getTime() const;

    /** Sets this time from a string in the <tt>HH:MM:SS</tt> format */
    void parse(const ODBCXX_STRING& str);

    /** Gets the hour of this time */
    int getHour() const {
      return hour_;
    }

    /** Gets the minute of this time */
    int getMinute() const {
      return minute_;
    }

    /** Gets the second of this time */
    int getSecond() const {
      return second_;
    }

    /** Sets the hour of this time */
    void setHour(int h) {
      hour_=this->_validateHour(h);
    }

    /** Sets the minute of this time */
    void setMinute(int m) {
      minute_=this->_validateMinute(m);
    }

    /** Sets the second of this time */
    void setSecond(int s) {
      second_=this->_validateSecond(s);
    }

    virtual ODBCXX_STRING toString() const;
  };


  /** An SQL TIMESTAMP
   */
  class ODBCXX_EXPORT Timestamp : public Date, public Time {
  private:
    int nanos_;

    virtual void _invalid(const ODBCXX_CHAR_TYPE* what, int value);

    int _validateNanos(int n) {
      if(n<0) {
        this->_invalid(ODBCXX_STRING_CONST("nanoseconds"),n);
      }
      return n;
    }

  public:
    /** Constructor */
    Timestamp(int year, int month, int day,
              int hour, int minute, int second,
              int nanos =0)
      :Date(year,month,day), Time(hour,minute,second) {
      this->setNanos(nanos);
    }

    /** Constructor.
     *
     * Sets the timestamp to now.
     */
    explicit Timestamp();

    /** Constructor.
     *
     * Sets this timestamp to the specified <tt>time_t</tt> value.
     */
    Timestamp(std::time_t t) {
      this->setTime(t);
    }

    /** Constructor.
     *
     * Sets this timestamp from a <tt>YYYY-MM-DD HH:MM:SS[.NNN...]</tt> format
     */
    Timestamp(const ODBCXX_STRING& s) {
      this->parse(s);
    }


    /** Copy constructor */
    Timestamp(const Timestamp& t)
      :Date(t),Time(t),nanos_(t.nanos_) {}

    /** Assignment operator */
    Timestamp& operator=(const Timestamp& t) {
      Date::operator=(t);
      Time::operator=(t);
      nanos_=t.nanos_;
      return *this;
    }

    /** Destructor */
    virtual ~Timestamp() {}

    /** Sets this timestamp to the specified <tt>time_t</tt> value */
    virtual void setTime(std::time_t t);

    /** Gets the time_t value of this timestamp */
    virtual std::time_t getTime() const {
      return Date::getTime()+Time::getTime();
    }

    /** Set this timestamp from a <tt>YYYY-MM-DD HH:MM:SS[.NNN...]</tt> format
     */
    void parse(const ODBCXX_STRING& s);

    /** Gets the nanoseconds value of this timestamp */
    int getNanos() const {
      return nanos_;
    }

    /** Sets the nanoseconds value of this timestamp */
    void setNanos(int nanos) {
      nanos_=this->_validateNanos(nanos);
    }

    virtual ODBCXX_STRING toString() const;
  };


  //this is used for several 'lists of stuff' below
  //expects T to be a pointer-to-something, and
  //the contents will get deleted when the vector
  //itself is deleted
  template <class T> class CleanVector : public std::vector<T> {
  private:
    CleanVector(const CleanVector<T>&); //forbid
    CleanVector<T>& operator=(const CleanVector<T>&); //forbid

  public:
    explicit CleanVector() {}
    virtual ~CleanVector() {
      typename std::vector<T>::iterator i=this->begin();
      typename std::vector<T>::iterator end=this->end();
      while(i!=end) {
        delete *i;
        ++i;
      }
      this->clear();
    }
  };


  /** Used internally - represents the result of an SQLError call
   */
  class ODBCXX_EXPORT DriverMessage {
    friend class ErrorHandler;

  private:
    ODBCXX_CHAR_TYPE state_[SQL_SQLSTATE_SIZE+1];
    ODBCXX_CHAR_TYPE description_[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER nativeCode_;

    DriverMessage() {}
#if ODBCVER < 0x0300
    static DriverMessage* fetchMessage(SQLHENV henv,
                                       SQLHDBC hdbc,
                                       SQLHSTMT hstmt);
#else
    static DriverMessage* fetchMessage(SQLINTEGER handleType,
                                       SQLHANDLE h,
                                       int idx);
#endif

  public:
    virtual ~DriverMessage() {}

    const ODBCXX_CHAR_TYPE* getSQLState() const {
      return state_;
    }

    const ODBCXX_CHAR_TYPE* getDescription() const {
      return description_;
    }

    int getNativeCode() const {
      return nativeCode_;
    }
  };


  /** The exception thrown when errors occur inside the library.
   */
  class SQLException : public std::exception {
  private:
    ODBCXX_STRING reason_;
    ODBCXX_STRING sqlState_;
    int errorCode_;
#if defined(ODBCXX_UNICODE)
    std::string reason8_;
#elif defined(ODBCXX_QT)
    QCString reason8_;
#endif
  public:
    /** Constructor */
    SQLException(const ODBCXX_STRING& reason =ODBCXX_STRING_CONST(""),
                 const ODBCXX_STRING& sqlState =ODBCXX_STRING_CONST(""),
                 int vendorCode =0)
      :reason_(reason),
       sqlState_(sqlState),
       errorCode_(vendorCode)
#if defined(ODBCXX_UNICODE)
{
   const size_t length =sizeof(wchar_t)*reason_.size();
   char* temp =new char[length+1];
   wcstombs(temp,reason_.c_str(),length);
   reason8_ =temp;
   delete[] temp;
}
#else
# if defined(ODBCXX_QT)
      ,reason8_(reason.local8Bit())
# endif
{}
#endif

    /** Copy from a DriverMessage */
    SQLException(const DriverMessage& dm)
      :reason_(dm.getDescription()),
       sqlState_(dm.getSQLState()),
       errorCode_(dm.getNativeCode()) {}

    /** Destructor */
    virtual ~SQLException() throw() {}

    /** Get the vendor error code of this exception */
    int getErrorCode() const {
      return errorCode_;
    }

    /** Gets the SQLSTATE of this exception.
     *
     * Consult your local ODBC reference for SQLSTATE values.
     */
    const ODBCXX_STRING& getSQLState() const {
      return sqlState_;
    }

    /** Gets the description of this message */
    const ODBCXX_STRING& getMessage() const {
      return reason_;
    }


    /** Gets the description of this message */
    virtual const char* what() const throw() {
      // the conversion from QString involves a temporary, which
      // doesn't survive this scope. So here, we do a conditional
#if defined(ODBCXX_QT)
      return reason8_.data();
#else
# if defined(ODBCXX_UNICODE)
      return reason8_.c_str();
# else
      return reason_.c_str();
# endif
#endif
    }
  };


  /** Represents an SQL warning.
   *
   * Contains the same info as an SQLException.
   */
  class SQLWarning : public SQLException {

    SQLWarning(const SQLWarning&); //forbid
    SQLWarning& operator=(const SQLWarning&); //forbid

  public:
    /** Constructor */
    SQLWarning(const ODBCXX_STRING& reason = ODBCXX_STRING_CONST(""),
               const ODBCXX_STRING& sqlState = ODBCXX_STRING_CONST(""),
               int vendorCode =0)
      :SQLException(reason,sqlState,vendorCode) {}

    /** Copy from a DriverMessage */
    SQLWarning(const DriverMessage& dm)
      :SQLException(dm) {}

    /** Destructor */
    virtual ~SQLWarning() throw() {}
  };

  typedef CleanVector<SQLWarning*> WarningList;


  template <class T> class Deleter {
  private:
    T* ptr_;
    bool isArray_;

    Deleter(const Deleter<T>&);
    Deleter<T>& operator=(const Deleter<T>&);

  public:
    explicit Deleter(T* ptr, bool isArray =false)
      :ptr_(ptr), isArray_(isArray) {}
    ~Deleter() {
      if(!isArray_) {
        delete ptr_;
      } else {
        delete[] ptr_;
      }
    }
  };

} // namespace odbc


#endif // __ODBCXX_TYPES_H
